#pragma once
#include "AssetImport.h"
#include "EditorOperations.h"
#include "../bridge/CommandQueue.h"

namespace ENGINE_EDITOR::Automation {
struct ImportHooks {
    // Native-test seams, never accepted from MCP/IPC arguments.
    std::function<PreparedImport(const Value&, const std::shared_ptr<std::atomic_bool>&)> prepare;
    std::function<void(const ImportBundle&)> afterUploadStep;
    std::chrono::milliseconds timeout{60000};
};
class AssetImportCoordinator {
    CommandQueue& queue;
    EditorOperations& operations;
    CommandQueue::Handle pending;
    std::future<PreparedImport> worker;
    std::shared_ptr<std::atomic_bool> cancelled;
    std::unique_ptr<ImportUpload> upload;
    std::chrono::steady_clock::time_point deadline;
    bool stopped = false;
    ImportHooks hooks;
    void Begin(const CommandQueue::Handle&);
    Json FailureResult(const Error&) const;
    void Discard();
public:
    AssetImportCoordinator(CommandQueue&, EditorOperations&, ImportHooks = {});
    ~AssetImportCoordinator();
    void Tick();
    void Stop();
};
}
