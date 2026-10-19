#include "../src/editor/bridge/CommandQueue.h"
#include <iostream>
using namespace ENGINE_EDITOR::Automation;
namespace {
void Check(bool b, const char* m) { if (!b) throw std::runtime_error(m); }
Request Write(const char* id = "r") {
    auto p = Parse(R"({"session_id":"s","expected_revision":0,"name":"Display"})");
    p.AddMember("request_id", Text(id, p.GetAllocator()), p.GetAllocator()); return Request("create_scene", p);
}
}
void QueueTests(Contract& contract) {
    unsigned count = 0;
    CommandQueue queue(contract, "s", [&](const Request& r) {
        ++count;
        Check(Str(queue.Status("s", r.Id()), "status") == "running", "Running state not observable");
        Check(Str(queue.Cancel("s", r.Id()), "status") == "running", "Running cancellation was misreported");
        auto out = Parse(R"({"status":"succeeded","session_id":"s","editor_revision":1,"operation_id":"o1","data":{"entity_id":"e1"}})");
        return out;
    }, 1, 2);
    auto request = Write(); auto t = queue.Submit(Write());
    auto full = queue.Submit(Write("full")); Check(Str(Parse(full->future.get())["error"], "code") == "queue_full", "Queue must be bounded");
    queue.Process(); Check(count == 1, "Command did not execute");
    Check(t == queue.Submit(Write()), "Retry should return original ticket before old revision check");
    Check(count == 1, "Retry duplicated mutation");
    auto conflict = Write(); conflict.params["name"].SetString("different", conflict.params.GetAllocator());
    auto clash = queue.Submit(std::move(conflict)); Check(Str(Parse(clash->future.get())["error"], "code") == "request_id_conflict", "ID conflict accepted");
    auto expired = Write("expired"); expired.deadline = std::chrono::steady_clock::now() - std::chrono::seconds(1);
    auto e = queue.Submit(std::move(expired)); queue.Process(); Check(Str(Parse(e->future.get()), "status") == "cancelled" && count == 1, "Expired request wrote state");
    auto capacity = queue.Submit(Write("new")); Check(Str(Parse(capacity->future.get())["error"], "code") == "request_capacity_exceeded", "Ledger evicted old receipt");
    Check(Str(queue.Status("s", "r"), "status") == "succeeded", "Receipt lost");
    Check(Str(queue.Status("old", "r")["error"], "code") == "stale_session", "Old session query accepted");
    bool threadRejected = false;
    std::thread other([&] { try { queue.Process(); } catch (const std::logic_error&) { threadRejected = true; } }); other.join();
    Check(threadRejected, "Worker drained main thread queue");
    CommandQueue shutdown(contract, "s", [](const Request&) { throw std::runtime_error("must not execute"); return Json(); });
    auto pending = shutdown.Submit(Write()); shutdown.Stop();
    Check(Str(Parse(pending->future.get()), "status") == "cancelled", "Shutdown left queued request running");
    CommandQueue cancelling(contract, "s", [](const Request&) { throw std::runtime_error("must not execute"); return Json(); });
    auto queued = cancelling.Submit(Write());
    Check(Str(cancelling.Cancel("s", "r"), "status") == "cancelled", "Queued cancellation lost race");
    cancelling.Process();
    Check(Str(Parse(queued->future.get()), "status") == "cancelled", "Cancelled command executed later");
    std::cout << "Queue lifecycle tests passed\n";
}
