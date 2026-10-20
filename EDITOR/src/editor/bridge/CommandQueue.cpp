#include "CommandQueue.h"

namespace ENGINE_EDITOR::Automation {
CommandQueue::CommandQueue(Contract& contract, std::string session, std::function<Json(const Request&)> execute, size_t q, size_t l)
    : m_Contract(contract), m_Session(std::move(session)), m_Execute(std::move(execute)), m_QueueCapacity(q), m_LedgerCapacity(l) {}
Json CommandQueue::Fail(const Request& r, const Error& e) const {
    auto d = Failure(r.Id(), e); auto& a = d.GetAllocator();
    d.AddMember("session_id", Text(m_Session, a), a); d.AddMember("editor_revision", m_Revision.load(), a);
    return d;
}
Json CommandQueue::SnapshotLocked(const Handle& ticket) const {
    if (!ticket->result.empty()) return Parse(ticket->result);
    Json d(rapidjson::kObjectType); auto& a = d.GetAllocator();
    d.AddMember("request_id", Text(ticket->request.Id(), a), a); d.AddMember("status", Text(ticket->status, a), a);
    d.AddMember("session_id", Text(m_Session, a), a); d.AddMember("editor_revision", m_Revision.load(), a);
    return d;
}
void CommandQueue::CompleteLocked(const Handle& t, Json result) {
    t->status = Str(result, "status"); t->result = Stringify(result); t->promise.set_value(t->result);
}
CommandQueue::Handle CommandQueue::Submit(Request request) {
    auto ticket = std::make_shared<Ticket>(std::move(request));
    std::lock_guard lock(m_Mutex);
    try {
        if (ticket->request.method == "hello") {
            if (!ticket->request.params.IsObject() || !ticket->request.params.ObjectEmpty()) throw Error("invalid_arguments", "hello has no arguments");
        } else m_Contract.Validate(ticket->request.method, ticket->request.params);
        auto& r = ticket->request;
        if (m_Stopped) throw Error("editor_unavailable", "Editor is shutting down", true);
        if (r.method == "capture_viewport" && m_Capture && (m_Capture->status == "queued" || m_Capture->status == "running"))
            throw Error("capture_busy", "Another capture is in progress", true);
        if (r.IsWrite()) {
            if (Str(r.params, "session_id") != m_Session) throw Error("stale_session", "Editor session has changed");
            ticket->fingerprint = r.Fingerprint();
            if (auto it = m_Ledger.find(r.Id()); it != m_Ledger.end()) {
                if (it->second->fingerprint != ticket->fingerprint) throw Error("request_id_conflict", "Request ID is already used with other arguments");
                return it->second;
            }
            if (m_Ledger.size() >= m_LedgerCapacity) throw Error("request_capacity_exceeded", "Write ledger is full; restart editor for a new session");
        }
        if (r.method == "import_asset" && m_Import && (m_Import->status == "queued" || m_Import->status == "running")) throw Error("import_busy", "Another import is in progress", true);
        // Discard cancelled/finished queue entries before checking admission capacity.
        std::erase_if(m_Queue, [](const Handle& t) { return t->status != "queued"; });
        if (m_Queue.size() >= m_QueueCapacity) throw Error("queue_full", "Editor command queue is full", true);
        m_Queue.push_back(ticket);
        if (r.method == "capture_viewport") m_Capture = ticket;
        if (r.method == "import_asset") m_Import = ticket;
        if (r.IsWrite()) m_Ledger.emplace(r.Id(), ticket);
    } catch (const Error& e) { CompleteLocked(ticket, Fail(ticket->request, e)); }
    return ticket;
}
Json CommandQueue::Status(const std::string& session, const std::string& id) {
    std::lock_guard lock(m_Mutex);
    Request r("get_operation_status", Parse("{}"));
    r.params.AddMember("request_id", Text(id, r.params.GetAllocator()), r.params.GetAllocator());
    if (session != m_Session) return Fail(r, Error("stale_session", "Editor session has changed"));
    auto it = m_Ledger.find(id);
    if (it == m_Ledger.end()) return Fail(r, Error("request_not_found", "Write request is not known in this session"));
    return SnapshotLocked(it->second);
}
Json CommandQueue::Cancel(const std::string& session, const std::string& id) {
    std::lock_guard lock(m_Mutex);
    Request r("cancel", Parse("{}")); r.params.AddMember("request_id", Text(id, r.params.GetAllocator()), r.params.GetAllocator());
    if (session != m_Session) return Fail(r, Error("stale_session", "Editor session has changed"));
    auto it = m_Ledger.find(id);
    if (it == m_Ledger.end()) return Fail(r, Error("request_not_found", "Write request is not known"));
    if (it->second->status == "queued" || (it->second->request.method == "import_asset" && it->second->status == "running" && !it->second->importCommitting)) {
        auto result = Fail(it->second->request, Error("cancelled", "Request cancelled before execution"));
        Set(result, "status", Text("cancelled", result.GetAllocator()), result.GetAllocator());
        CompleteLocked(it->second, std::move(result));
    }
    return SnapshotLocked(it->second);
}
void CommandQueue::Cancel(const Handle& t) {
    std::lock_guard lock(m_Mutex);
    if (t->status != "queued" && !(t->request.method == "capture_viewport" && t->status == "running") && !(t->request.method == "import_asset" && t->status == "running" && !t->importCommitting)) {
        if (m_Capture == t) m_Capture.reset();
        return;
    }
    auto result = Fail(t->request, Error("cancelled", "Connection closed before execution"));
    Set(result, "status", Text("cancelled", result.GetAllocator()), result.GetAllocator());
    CompleteLocked(t, std::move(result));
    if (m_Capture == t) m_Capture.reset();
}
Json CommandQueue::Snapshot(const Handle& t) { std::lock_guard lock(m_Mutex); return SnapshotLocked(t); }
void CommandQueue::Process(size_t maxCommands, std::chrono::milliseconds budget) {
    if (std::this_thread::get_id() != m_Owner) throw std::logic_error("Queue must drain on its owner thread");
    const auto start = std::chrono::steady_clock::now();
    for (size_t n = 0; n < maxCommands; ++n) {
        Handle ticket;
        {
            std::lock_guard lock(m_Mutex);
            if (m_Queue.empty() || m_Stopped) break;
            ticket = m_Queue.front(); m_Queue.pop_front();
            if (ticket->status != "queued") continue;
            if (std::chrono::steady_clock::now() >= ticket->request.deadline) {
                const bool capture = ticket->request.method == "capture_viewport";
                auto result = Fail(ticket->request, Error(capture ? "capture_timeout" : "cancelled", "Request expired before execution", capture));
                if (!capture) Set(result, "status", Text("cancelled", result.GetAllocator()), result.GetAllocator());
                CompleteLocked(ticket, std::move(result)); continue;
            }
            ticket->status = "running";
        }
        Json result;
        try {
            if (ticket->request.method == "capture_viewport" && m_CaptureHandler) {
                m_CaptureHandler(ticket);
                if (std::chrono::steady_clock::now() - start >= budget) break;
                continue;
            }
            if (ticket->request.method == "import_asset" && m_ImportHandler) {
                m_ImportHandler(ticket);
                if (std::chrono::steady_clock::now() - start >= budget) break;
                continue;
            }
            result = m_Execute(ticket->request);
        }
        catch (const std::exception& e) { result = Fail(ticket->request, Error("execution_failed", e.what())); }
        if (result.HasMember("editor_revision") && result["editor_revision"].IsUint64()) m_Revision = result["editor_revision"].GetUint64();
        { std::lock_guard lock(m_Mutex); CompleteLocked(ticket, std::move(result)); }
        if (std::chrono::steady_clock::now() - start >= budget) break;
    }
}
void CommandQueue::Stop() {
    std::lock_guard lock(m_Mutex); m_Stopped = true;
    if (m_Capture && (m_Capture->status == "queued" || m_Capture->status == "running")) {
        auto result = Fail(m_Capture->request, Error("cancelled", "Editor is shutting down"));
        Set(result, "status", Text("cancelled", result.GetAllocator()), result.GetAllocator());
        CompleteLocked(m_Capture, std::move(result));
    }
    for (auto& t : m_Queue) if (t->status == "queued") {
        auto result = Fail(t->request, Error("cancelled", "Editor is shutting down"));
        Set(result, "status", Text("cancelled", result.GetAllocator()), result.GetAllocator());
        CompleteLocked(t, std::move(result));
    }
    if (m_Import && (m_Import->status == "queued" || m_Import->status == "running")) {
        auto result = Fail(m_Import->request, Error("cancelled", "Editor is shutting down"));
        Set(result, "status", Text("cancelled", result.GetAllocator()), result.GetAllocator()); CompleteLocked(m_Import, std::move(result));
    }
    m_Queue.clear();
    m_Capture.reset(); m_Import.reset();
}
bool CommandQueue::BeginImportCommit(const Handle& ticket) {
    std::lock_guard lock(m_Mutex);
    if (m_Stopped || ticket->status != "running") return false;
    ticket->importCommitting = true; return true;
}
bool CommandQueue::Active(const Handle& t) {
    std::lock_guard lock(m_Mutex);
    return !m_Stopped && (t->status == "queued" || t->status == "running");
}
void CommandQueue::CompleteDeferred(const Handle& t, Json result) {
    std::lock_guard lock(m_Mutex);
    if (m_Stopped || (t->status != "queued" && t->status != "running")) return;
    if (result.HasMember("editor_revision")) {
        auto value = result["editor_revision"].GetUint64(), previous = m_Revision.load();
        while (previous < value && !m_Revision.compare_exchange_weak(previous, value)) {}
    }
    CompleteLocked(t, std::move(result));
}
void CommandQueue::ReleaseCapture(const Handle& t) {
    std::lock_guard lock(m_Mutex);
    if (m_Capture == t && t->status != "queued" && t->status != "running") m_Capture.reset();
}
}
