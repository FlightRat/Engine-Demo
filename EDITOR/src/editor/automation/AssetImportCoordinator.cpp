#include "AssetImportCoordinator.h"

namespace ENGINE_EDITOR::Automation {
AssetImportCoordinator::AssetImportCoordinator(CommandQueue& value, EditorOperations& ops, ImportHooks testing) : queue(value), operations(ops), hooks(std::move(testing)) {
    queue.SetImportHandler([this](const auto& ticket) { Begin(ticket); });
}
AssetImportCoordinator::~AssetImportCoordinator() { Stop(); }
Json AssetImportCoordinator::FailureResult(const Error& error) const {
    auto out = Failure(pending ? pending->request.Id() : "", error); auto& a = out.GetAllocator();
    out.AddMember("session_id", Text(operations.Session(), a), a); out.AddMember("editor_revision", operations.Revision(), a); return out;
}
void AssetImportCoordinator::Begin(const CommandQueue::Handle& ticket) {
    if (stopped) { queue.CompleteDeferred(ticket, FailureResult(Error("editor_unavailable", "Import coordinator stopped"))); return; }
    auto context = operations.CheckImport(ticket->request);
    if (Str(context, "status") != "succeeded") { queue.CompleteDeferred(ticket, std::move(context)); return; }
    if (pending || worker.valid()) {
        auto result = Failure(ticket->request.Id(), Error("import_busy", "Previous import is still finishing", true));
        result.AddMember("session_id", Text(operations.Session(), result.GetAllocator()), result.GetAllocator());
        result.AddMember("editor_revision", operations.Revision(), result.GetAllocator());
        queue.CompleteDeferred(ticket, std::move(result)); return;
    }
    pending = ticket; cancelled = std::make_shared<std::atomic_bool>(false);
    deadline = std::chrono::steady_clock::now() + hooks.timeout;
    Json params; params.CopyFrom(ticket->request.params, params.GetAllocator());
    worker = std::async(std::launch::async, [args = std::move(params), flag = cancelled, prepare = hooks.prepare]() mutable { return prepare ? prepare(args, flag) : PrepareImport(args, flag); });
}
void AssetImportCoordinator::Discard() {
    if (cancelled) cancelled->store(true);
    upload.reset(); pending.reset();
}
void AssetImportCoordinator::Tick() {
    if (stopped) return;
    if (!pending) {
        if (worker.valid() && worker.wait_for(std::chrono::seconds(0)) == std::future_status::ready) { try { worker.get(); } catch (...) {} }
        return;
    }
    if (!queue.Active(pending)) { Discard(); return; }
    try {
        if (std::chrono::steady_clock::now() >= deadline) throw Error("import_timeout", "Import exceeded its 60 second deadline", true);
        auto context = operations.CheckImport(pending->request);
        if (Str(context, "status") != "succeeded") { queue.CompleteDeferred(pending, std::move(context)); Discard(); return; }
        if (!upload) {
            if (!worker.valid() || worker.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return;
            upload = std::make_unique<ImportUpload>(worker.get(), operations.Assets());
        }
        const bool complete = upload->Tick();
        if (hooks.afterUploadStep) hooks.afterUploadStep(upload->PendingBundle());
        if (complete) {
            if (!queue.BeginImportCommit(pending)) { Discard(); return; }
            queue.CompleteDeferred(pending, operations.CommitImport(pending->request, upload->Result()));
            Discard();
        }
    } catch (const Error& e) { queue.CompleteDeferred(pending, FailureResult(e)); Discard(); }
    catch (const std::exception& e) { queue.CompleteDeferred(pending, FailureResult(Error("import_failed", e.what()))); Discard(); }
}
void AssetImportCoordinator::Stop() {
    if (stopped) return;
    stopped = true;
    if (pending) queue.Cancel(pending);
    Discard();
    if (worker.valid()) { try { worker.get(); } catch (...) {} }
    queue.SetImportHandler({});
}
}
