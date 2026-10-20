#pragma once
#include "../automation/Protocol.h"
#include <atomic>
#include <deque>
#include <functional>
#include <future>
#include <map>
#include <mutex>
#include <thread>

namespace ENGINE_EDITOR::Automation {
class CommandQueue {
public:
    struct Ticket {
        Request request;
        std::string fingerprint, status = "queued", result;
        bool importCommitting = false;
        std::promise<std::string> promise;
        std::shared_future<std::string> future;
        explicit Ticket(Request request) : request(std::move(request)), future(promise.get_future().share()) {}
    };
    using Handle = std::shared_ptr<Ticket>;
private:
    Contract& m_Contract;
    std::string m_Session;
    std::thread::id m_Owner = std::this_thread::get_id();
    std::function<Json(const Request&)> m_Execute;
    std::mutex m_Mutex;
    std::deque<Handle> m_Queue;
    std::map<std::string, Handle> m_Ledger;
    size_t m_QueueCapacity, m_LedgerCapacity;
    std::atomic<uint64_t> m_Revision{0};
    bool m_Stopped = false;
    Handle m_Capture;
    std::function<void(const Handle&)> m_CaptureHandler;
    Handle m_Import;
    std::function<void(const Handle&)> m_ImportHandler;
    Json SnapshotLocked(const Handle&) const;
    void CompleteLocked(const Handle&, Json);
    Json Fail(const Request&, const Error&) const;
public:
    CommandQueue(Contract&, std::string session, std::function<Json(const Request&)>,
        size_t queueCapacity = DefaultQueueCapacity, size_t ledgerCapacity = DefaultLedgerCapacity);
    Handle Submit(Request);
    Json Status(const std::string& session, const std::string& requestId);
    Json Cancel(const std::string& session, const std::string& requestId);
    void Cancel(const Handle&);
    Json Snapshot(const Handle&);
    void Process(size_t maxCommands = 8, std::chrono::milliseconds budget = std::chrono::milliseconds(2));
    void Stop();
    void SetCaptureHandler(std::function<void(const Handle&)> handler) { m_CaptureHandler = std::move(handler); }
    void SetImportHandler(std::function<void(const Handle&)> handler) { m_ImportHandler = std::move(handler); }
    bool BeginImportCommit(const Handle&);
    bool Active(const Handle&);
    void CompleteDeferred(const Handle&, Json);
    void ReleaseCapture(const Handle&);
};
}
