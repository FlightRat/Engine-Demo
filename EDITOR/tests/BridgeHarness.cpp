#include "../src/editor/bridge/NamedPipeServer.h"
#include "../src/editor/automation/EditorOperations.h"
#include "../src/editor/scene/SceneManager.h"
#include <Core/Resources/AssetManager.h>
#include <Logger/Logger.h>
#include <iostream>
using namespace ENGINE_EDITOR;
using namespace ENGINE_EDITOR::Automation;
int BridgeHarness(Contract& contract, const std::string& instance, int duration) {
    ENGINE_INIT_LOGS(false, true);
    ENGINE_CORE::RESOURCES::AssetManager assets;
    auto& scenes = SCENE_MANAGER(); scenes.CleanUp();
    {
        OperationHooks hooks;
        if (instance.starts_with("slow_")) hooks.beforeApply = [](size_t) { std::this_thread::sleep_for(std::chrono::milliseconds(500)); };
        EditorOperations operations(scenes, assets, contract, instance, hooks);
        CommandQueue queue(contract, operations.Session(), [&](const Request& r) { return operations.Execute(r); });
        NamedPipeServer server(queue, contract); server.Start(instance);
        std::cout << "BRIDGE_READY\n" << std::flush;
        const auto end = std::chrono::steady_clock::now() + std::chrono::seconds(duration);
        while (std::chrono::steady_clock::now() < end) {
            operations.ObserveExternalChanges(); queue.Process();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        queue.Stop(); server.Stop();
    }
    scenes.CleanUp();
    return 0;
}
