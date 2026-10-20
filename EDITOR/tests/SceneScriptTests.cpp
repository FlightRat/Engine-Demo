#include "../src/editor/scene/SceneManager.h"
#include "../src/editor/scene/SceneObject.h"
#include <fstream>
#include <Core/Systems/ScriptingSystem.h>
#include <Core/ECS/MetaUtilities.h>
#include <Core/ECS/Components/TransformComponent.h>
#include <stdexcept>
using namespace ENGINE_EDITOR;
namespace fs = std::filesystem;
static void Check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
static void Write(const fs::path& path, const std::string& text) { std::ofstream stream(path, std::ios::binary); stream << text; }
void SceneScriptTests() {
    ENGINE_CORE::Systems::ScriptingSystem::RegisterComponentMetadata();
    {
        using namespace entt::literals;
        ENGINE_CORE::ECS::Registry registry;
        ENGINE_CORE::ECS::Entity entity(registry, "before-first-play", "");
        auto added = ENGINE_CORE::Utils::InvokeMetaFunction(entt::type_hash<ENGINE_CORE::ECS::TransformComponent>::value(), "add_component_default"_hs, entity);
        Check(bool(added) && entity.HasComponent<ENGINE_CORE::ECS::TransformComponent>(), "Editing components require running Lua");
    }
    auto& scenes = SCENE_MANAGER(); scenes.CleanUp(); auto& scripts = scenes.Scripts();
    for (const std::string name : {"", "../bad", "CON", "CON .txt", "com1.txt", "a/b", "a\\b", "x:", "ends.", "ends ", "<bad>"}) {
        bool rejected = false; try { SceneScriptFiles::ValidateName(name); } catch (...) { rejected = true; }
        Check(rejected, "Unsafe Scene name accepted");
    }
    SceneScriptFiles::ValidateName(".hidden");
    Check(scenes.CreateDefault(), "Default creation failed");
    Check(scenes.GetCurrentSceneName() == "default" && scenes.GetCurrentScene()->GetRegistry().GetRegistry().view<entt::entity>().size() == 0, "Default not empty/active");
    auto path = scripts.ScriptPath("default"); Check(SceneScriptFiles::Read(path) == SceneScriptFiles::Template(), "Wrong template");
    Write(path, "error('must never execute during edit')\nreturn {}\n");
    Check(SceneScriptFiles::SyntaxError(path).empty(), "Syntax check executed the module");
    scenes.CleanUp(); Check(scenes.CreateDefault(), "Existing default not reused");
    Check(SceneScriptFiles::Read(path).starts_with("error("), "Existing script overwritten");
    Check(!scenes.AddScene("DEFAULT"), "Case insensitive duplicate accepted");
    Check(scenes.CreateAndActivate("中文场景"), "Unicode Scene failed");
    auto scene = scenes.GetCurrentScene(); auto original = SceneScriptFiles::Read(scripts.ScriptPath("中文场景"));
    Check(scenes.ChangeSceneName("中文场景", "Renamed"), "Scene rename failed");
    Check(scene == scenes.GetCurrentScene() && scene->GetName() == "Renamed", "Rename changed scene identity");
    Check(SceneScriptFiles::Read(scripts.ScriptPath("Renamed")) == original, "Rename changed script bytes");
    Check(scenes.ChangeSceneName("Renamed", "renamed"), "Case only rename failed");
    Write(scripts.ScriptPath("occupied"), "user content");
    Check(!scenes.ChangeSceneName("renamed", "occupied"), "Rename overwrote destination");
    scripts.SetHook([](const char* point) { if (std::string(point) == "during_rename") throw std::runtime_error("injected rename error"); });
    Check(!scenes.ChangeSceneName("renamed", "rollback"), "Rename fault ignored");
    Check(fs::exists(scripts.ScriptPath("renamed")) && !fs::exists(scripts.ScriptPath("rollback")), "Rename failed to roll back");
    scripts.SetHook([](const char*) { throw std::runtime_error("injected publication error"); });
    Check(!scenes.AddScene("failed") && !fs::exists(scripts.ScriptPath("failed")), "Failed publish left a Scene/file");
    scripts.SetHook([](const char* point) { if (std::string(point) == "after_publish") throw std::bad_alloc(); });
    Check(!scenes.AddScene("failed_after") && !fs::exists(scripts.ScriptPath("failed_after")), "Failed allocation left file");
    scripts.SetHook({});
    fs::create_directory(scripts.ScriptPath("directory"));
    Check(!scenes.AddScene("directory"), "Directory script path accepted");
    scenes.SetCurrentScene("default"); Check(scenes.DeleteScene("renamed"), "Scene delete failed");
    Check(fs::exists(scripts.ScriptPath("renamed")), "Delete removed script");
    Write(path, "invalid lua !"); Check(!SceneScriptFiles::SyntaxError(path).empty(), "Syntax error not reported");
    scenes.CleanUp();
}
