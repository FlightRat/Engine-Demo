#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "NamedPipeServer.h"
#include <Windows.h>
#include <sddl.h>
#include <condition_variable>
#include <regex>
#include <optional>
#include <system_error>
#include <vector>

namespace ENGINE_EDITOR::Automation {
namespace {
std::wstring UserSid() {
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) throw std::runtime_error("Cannot query Windows user");
    DWORD size = 0; GetTokenInformation(token, TokenUser, nullptr, 0, &size);
    std::vector<unsigned char> data(size);
    const bool ok = GetTokenInformation(token, TokenUser, data.data(), size, &size) != FALSE;
    CloseHandle(token);
    if (!ok) throw std::runtime_error("Cannot read Windows user SID");
    LPWSTR text = nullptr;
    if (!ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER*>(data.data())->User.Sid, &text)) throw std::runtime_error("Cannot encode Windows SID");
    std::wstring sid(text); LocalFree(text); return sid;
}
struct WinHandle {
    HANDLE value;
    explicit WinHandle(HANDLE h) : value(h) {}
    ~WinHandle() { if (value && value != INVALID_HANDLE_VALUE) CloseHandle(value); }
};
}
std::wstring PipeName(const std::string& instance) {
    if (!std::regex_match(instance, std::regex("[A-Za-z0-9_-]{1,64}"))) throw std::invalid_argument("Instance must be 1..64 ASCII letters, digits, _ or -");
    return L"\\\\.\\pipe\\EngineDemo." + UserSid() + L"." + std::wstring(instance.begin(), instance.end());
}
struct NamedPipeServer::Impl {
    CommandQueue& queue;
    Contract& contract;
    HANDLE pipe = INVALID_HANDLE_VALUE;
    WinHandle stopEvent{CreateEventW(nullptr, TRUE, FALSE, nullptr)};
    std::thread worker;
    std::atomic<bool> stopping{false};
    Impl(CommandQueue& q, Contract& c) : queue(q), contract(c) {}
    bool Wait(HANDLE pipeHandle, OVERLAPPED& op, DWORD& transferred) {
        HANDLE events[]{stopEvent.value, op.hEvent};
        const DWORD result = WaitForMultipleObjects(2, events, FALSE, INFINITE);
        if (result != WAIT_OBJECT_0 + 1) {
            CancelIoEx(pipeHandle, &op); GetOverlappedResult(pipeHandle, &op, &transferred, TRUE); return false;
        }
        return GetOverlappedResult(pipeHandle, &op, &transferred, FALSE) != FALSE;
    }
    bool Transfer(void* data, size_t size, bool write) {
        size_t offset = 0;
        while (offset < size && !stopping) {
            WinHandle event(CreateEventW(nullptr, TRUE, FALSE, nullptr));
            OVERLAPPED op{}; op.hEvent = event.value;
            DWORD count = 0;
            const auto length = static_cast<DWORD>((std::min)(size - offset, size_t(65536)));
            auto* bytes = static_cast<unsigned char*>(data) + offset;
            const BOOL done = write ? WriteFile(pipe, bytes, length, &count, &op) : ReadFile(pipe, bytes, length, &count, &op);
            if (!done && (GetLastError() != ERROR_IO_PENDING || !Wait(pipe, op, count))) return false;
            if (count == 0) return false;
            offset += count;
        }
        return offset == size;
    }
    bool ReadFrame(std::string& out) {
        uint32_t size = 0;
        if (!Transfer(&size, 4, false) || size == 0 || size > MaxMessageBytes) return false;
        out.resize(size); return Transfer(out.data(), size, false);
    }
    bool WriteFrame(std::string text) {
        if (text.empty() || text.size() > MaxMessageBytes) return false;
        uint32_t size = static_cast<uint32_t>(text.size());
        return Transfer(&size, 4, true) && Transfer(text.data(), size, true);
    }
    struct Pending {
        CommandQueue::Handle ticket;
        std::string immediate, correlation;
        std::chrono::steady_clock::time_point started = std::chrono::steady_clock::now();
    };
    void Connection() {
        std::mutex mutex;
        std::deque<Pending> pending;
        std::atomic<bool> connected{true};
        std::thread sender([&] {
            try {
            while (connected && !stopping) {
                std::optional<Pending> selected;
                {
                    std::lock_guard lock(mutex);
                    for (auto it = pending.begin(); it != pending.end(); ++it) {
                        if (!it->immediate.empty() || it->ticket->future.wait_for(std::chrono::seconds(0)) == std::future_status::ready ||
                            (it->ticket && it->ticket->request.method == "capture_viewport" && std::chrono::steady_clock::now() >= it->ticket->request.deadline) ||
                            std::chrono::steady_clock::now() - it->started >= std::chrono::seconds(10)) {
                            selected = std::move(*it); pending.erase(it); break;
                        }
                    }
                }
                if (!selected) { Sleep(2); continue; }
                auto& p = *selected;
                Json result;
                if (!p.immediate.empty()) result = Parse(p.immediate);
                else if (p.ticket->future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) result = Parse(p.ticket->future.get());
                else {
                    if (p.ticket->request.method == "capture_viewport") {
                        auto snapshot = queue.Snapshot(p.ticket);
                        auto timeout = Failure("", Error("capture_timeout", "Capture deadline expired", true));
                        auto& a = timeout.GetAllocator();
                        timeout.AddMember("session_id", Copy(snapshot["session_id"], a), a);
                        timeout.AddMember("editor_revision", Copy(snapshot["editor_revision"], a), a);
                        queue.CompleteDeferred(p.ticket, std::move(timeout));
                        result = Parse(p.ticket->future.get());
                    } else {
                    result = queue.Snapshot(p.ticket);
                    Set(result, "execution_status", Text(Str(result, "status"), result.GetAllocator()), result.GetAllocator());
                    Set(result, "status", Text("pending", result.GetAllocator()), result.GetAllocator());
                    }
                }
                Set(result, "correlation_id", Text(p.correlation, result.GetAllocator()), result.GetAllocator());
                if (!WriteFrame(Stringify(result))) { if (p.ticket) queue.Cancel(p.ticket); connected = false; CancelIoEx(pipe, nullptr); break; }
                if (p.ticket) queue.ReleaseCapture(p.ticket);
            }
            } catch (...) {
                // A failed response must close this client, not terminate the editor.
                connected = false;
                CancelIoEx(pipe, nullptr);
            }
        });
        try {
            std::string text;
            while (connected && !stopping && ReadFrame(text)) {
                Pending response;
                try {
                    auto d = Parse(text);
                    if (!d.IsObject() || !UniqueKeys(d)) throw Error("invalid_arguments", "Expected unique envelope fields");
                    response.correlation = Str(d, "correlation_id");
                    if (response.correlation.empty() || response.correlation.size() > 256) throw Error("invalid_arguments", "Missing correlation_id");
                    for (auto it = d.MemberBegin(); it != d.MemberEnd(); ++it) {
                        const std::string key = it->name.GetString();
                        if (key != "protocol_version" && key != "correlation_id" && key != "method" && key != "params") throw Error("invalid_arguments", "Unknown envelope field");
                    }
                    if (!d.HasMember("protocol_version") || !d["protocol_version"].IsUint() || d["protocol_version"].GetUint() != ProtocolVersion) throw Error("unsupported_version", "Expected bridge protocol version 1");
                    if (!d.HasMember("params") || !d["params"].IsObject()) throw Error("invalid_arguments", "Missing params object");
                    const auto method = Str(d, "method");
                    if (method == "get_operation_status" || method == "cancel") {
                        contract.Validate("get_operation_status", d["params"]);
                        auto status = method == "cancel" ? queue.Cancel(Str(d["params"], "session_id"), Str(d["params"], "request_id")) : queue.Status(Str(d["params"], "session_id"), Str(d["params"], "request_id"));
                        response.immediate = Stringify(status);
                    } else response.ticket = queue.Submit(Request(method, d["params"], response.correlation));
                } catch (const Error& e) { response.immediate = Stringify(Failure("", e)); }
                catch (const std::exception& e) { response.immediate = Stringify(Failure("", Error("invalid_arguments", e.what()))); }
                std::lock_guard lock(mutex);
                if (pending.size() >= DefaultQueueCapacity * 2) { if (response.ticket) queue.Cancel(response.ticket); break; }
                pending.push_back(std::move(response));
            }
        } catch (...) { /* Broken client cannot terminate the editor. */ }
        connected = false;
        CancelIoEx(pipe, nullptr);
        if (sender.joinable()) sender.join();
        std::lock_guard lock(mutex);
        for (auto& p : pending) if (p.ticket) queue.Cancel(p.ticket);
    }
    void Run() {
        while (!stopping) {
            WinHandle event(CreateEventW(nullptr, TRUE, FALSE, nullptr)); OVERLAPPED op{}; op.hEvent = event.value;
            const BOOL connected = ConnectNamedPipe(pipe, &op);
            DWORD ignored = 0;
            if (!connected) {
                const DWORD error = GetLastError();
                if (error != ERROR_PIPE_CONNECTED && (error != ERROR_IO_PENDING || !Wait(pipe, op, ignored))) {
                    if (stopping) break;
                    DisconnectNamedPipe(pipe); continue;
                }
            }
            if (!stopping) Connection();
            DisconnectNamedPipe(pipe);
        }
    }
};
NamedPipeServer::NamedPipeServer(CommandQueue& q, Contract& c) : m_Impl(std::make_unique<Impl>(q, c)) {}
NamedPipeServer::~NamedPipeServer() { Stop(); }
void NamedPipeServer::Start(const std::string& instance) {
    auto& impl = *m_Impl;
    if (impl.worker.joinable()) throw std::logic_error("Bridge is already started");
    const auto name = PipeName(instance);
    const auto security = L"D:P(A;;GA;;;" + UserSid() + L")";
    PSECURITY_DESCRIPTOR descriptor = nullptr;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(security.c_str(), SDDL_REVISION_1, &descriptor, nullptr)) throw std::runtime_error("Cannot create local pipe ACL");
    SECURITY_ATTRIBUTES attributes{sizeof(SECURITY_ATTRIBUTES), descriptor, FALSE};
    impl.pipe = CreateNamedPipeW(name.c_str(), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS, 1, 65536, 65536, 0, &attributes);
    const auto error = GetLastError(); LocalFree(descriptor);
    if (impl.pipe == INVALID_HANDLE_VALUE) throw std::system_error(error, std::system_category(), "Cannot create editor bridge (instance may already be in use)");
    impl.stopping = false; ResetEvent(impl.stopEvent.value);
    try { impl.worker = std::thread([&impl] { impl.Run(); }); }
    catch (...) { CloseHandle(impl.pipe); impl.pipe = INVALID_HANDLE_VALUE; throw; }
}
void NamedPipeServer::Stop() {
    auto& impl = *m_Impl;
    impl.stopping = true; SetEvent(impl.stopEvent.value);
    if (impl.pipe != INVALID_HANDLE_VALUE) CancelIoEx(impl.pipe, nullptr);
    if (impl.worker.joinable()) impl.worker.join();
    if (impl.pipe != INVALID_HANDLE_VALUE) { CloseHandle(impl.pipe); impl.pipe = INVALID_HANDLE_VALUE; }
}
}
