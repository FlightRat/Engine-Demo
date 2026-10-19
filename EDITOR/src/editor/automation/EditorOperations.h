#pragma once
#include "Protocol.h"
#include <functional>
namespace ENGINE_CORE::RESOURCES { class AssetManager; }
namespace ENGINE_EDITOR { class SceneManager; }
namespace ENGINE_EDITOR::Automation {
struct OperationHooks {
    // Failure injection for transaction tests; never exposed over IPC.
    std::function<void(size_t)> beforeApply;
    std::function<void()> beforeRollback;
};
class EditorOperations {
    struct Impl;
    std::unique_ptr<Impl> m_Impl;
public:
    EditorOperations(SceneManager&, ENGINE_CORE::RESOURCES::AssetManager&, Contract&, std::string instance, OperationHooks = {});
    ~EditorOperations();
    EditorOperations(const EditorOperations&) = delete;
    EditorOperations& operator=(const EditorOperations&) = delete;
    Json Execute(const Request&);
    void ObserveExternalChanges();
    const std::string& Session() const;
    uint64_t Revision() const;
};
}
