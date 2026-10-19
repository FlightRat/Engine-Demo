#include "AutomationRuntime.h"
#include "EditorOperations.h"
#include "../bridge/NamedPipeServer.h"
#include "../scene/SceneManager.h"
#include <Core/ECS/MainRegistry.h>
#include <Logger/Logger.h>
namespace ENGINE_EDITOR::Automation {
struct AutomationRuntime::Impl {
    Contract contract{"../tools/engine_mcp/contracts/tools.json"};
    EditorOperations operations;
    CommandQueue queue;
    NamedPipeServer server;
    explicit Impl(const std::string& instance)
        : operations(SCENE_MANAGER(), MAIN_REGISTRY().GetAssetManager(), contract, instance),
          queue(contract, operations.Session(), [this](const Request& r) { return operations.Execute(r); }), server(queue, contract) {
        server.Start(instance);
        ENGINE_LOG("MCP editor bridge started: {}", instance);
    }
    ~Impl() { queue.Stop(); server.Stop(); }
};
AutomationRuntime::AutomationRuntime(const std::string& instance) : m_Impl(std::make_unique<Impl>(instance)) {}
AutomationRuntime::~AutomationRuntime() = default;
void AutomationRuntime::Update() { m_Impl->operations.ObserveExternalChanges(); m_Impl->queue.Process(); }
void AutomationRuntime::AfterUI() { m_Impl->operations.ObserveExternalChanges(); }
}
