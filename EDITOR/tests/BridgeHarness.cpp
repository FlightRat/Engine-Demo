#include "../src/editor/bridge/NamedPipeServer.h"
#include "../src/editor/automation/EditorOperations.h"
#include "../src/editor/automation/AssetImportCoordinator.h"
#include "../src/editor/scene/SceneManager.h"
#include <Core/Resources/AssetManager.h>
#include <Logger/Logger.h>
#include <iostream>
using namespace ENGINE_EDITOR;
using namespace ENGINE_EDITOR::Automation;
int BridgeHarness(Contract& contract, const std::string& instance, int duration) {
    ENGINE_INIT_LOGS(false, true);
    ENGINE_CORE::RESOURCES::AssetManager assets;
    // A test-only empty mesh provides a material slot without pretending to render.
    // Actual GPU loading/rendering is verified separately in --gl-tests and EDITOR.
    std::vector<ENGINE_RENDERING::Mesh> meshes(1);
    ENGINE_CORE::RESOURCES::LoadedAsset fixture;
    fixture.type = ENGINE_UTIL::AssetType::MODEL; fixture.key = "test_mesh";
    fixture.payload = std::make_shared<ENGINE_RENDERING::Model>(std::move(meshes));
    std::vector<ENGINE_CORE::RESOURCES::LoadedAsset> entries; entries.push_back(std::move(fixture));
    assets.Publish(entries); entries.clear();
    auto& scenes = SCENE_MANAGER(); scenes.CleanUp();
    {
        OperationHooks hooks;
        if (instance.starts_with("slow_")) hooks.beforeApply = [](size_t) { std::this_thread::sleep_for(std::chrono::milliseconds(500)); };
        EditorOperations operations(scenes, assets, contract, instance, hooks);
        CommandQueue queue(contract, operations.Session(), [&](const Request& r) { return operations.Execute(r); });
        ImportHooks importHooks;
        if (instance.starts_with("slow_import_")) importHooks.prepare = [](const Value& args, const auto& cancelled) {
            const auto end = std::chrono::steady_clock::now() + std::chrono::seconds(11);
            while (std::chrono::steady_clock::now() < end && !cancelled->load()) std::this_thread::sleep_for(std::chrono::milliseconds(10));
            return PrepareImport(args, cancelled);
        };
        AssetImportCoordinator importer(queue, operations, importHooks);
        NamedPipeServer server(queue, contract); server.Start(instance);
        std::cout << "BRIDGE_READY\n" << std::flush;
        const auto end = std::chrono::steady_clock::now() + std::chrono::seconds(duration);
        while (std::chrono::steady_clock::now() < end) {
            operations.ObserveExternalChanges(); queue.Process(); importer.Tick();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        queue.Stop(); importer.Stop(); server.Stop();
    }
    scenes.CleanUp();
    return 0;
}
