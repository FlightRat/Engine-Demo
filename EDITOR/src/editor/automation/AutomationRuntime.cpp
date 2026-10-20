#include "AutomationRuntime.h"
#include "EditorOperations.h"
#include "CaptureCoordinator.h"
#include "AssetImportCoordinator.h"
#include "SceneCapture.h"
#include "../displays/SceneDisplay.h"
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
    std::unique_ptr<CaptureCoordinator> capture;
    std::unique_ptr<AssetImportCoordinator> importer;
    explicit Impl(const std::string& instance)
        : operations(SCENE_MANAGER(), MAIN_REGISTRY().GetAssetManager(), contract, instance),
          queue(contract, operations.Session(), [this](const Request& r) { return operations.Execute(r); }), server(queue, contract) {
        auto holder = MAIN_REGISTRY().GetContext<std::shared_ptr<DisplayHolder>>();
        for (const auto& display : holder->displays) if (auto* scene = dynamic_cast<SceneDisplay*>(display.get())) {
            capture = std::make_unique<CaptureCoordinator>(queue, operations, MakeSceneCapture(scene->GetSceneCamera())); break;
        }
        importer = std::make_unique<AssetImportCoordinator>(queue, operations);
        server.Start(instance);
        ENGINE_LOG("MCP editor bridge started: {}", instance);
    }
    ~Impl() { queue.Stop(); if (importer) importer->Stop(); if (capture) capture->Stop(); server.Stop(); }
};
AutomationRuntime::AutomationRuntime(const std::string& instance) : m_Impl(std::make_unique<Impl>(instance)) {}
AutomationRuntime::~AutomationRuntime() = default;
void AutomationRuntime::Update() { m_Impl->operations.ObserveExternalChanges(); m_Impl->queue.Process(); m_Impl->importer->Tick(); }
void AutomationRuntime::AfterUI() { m_Impl->operations.ObserveExternalChanges(); if (m_Impl->capture) m_Impl->capture->Tick(); }
}
