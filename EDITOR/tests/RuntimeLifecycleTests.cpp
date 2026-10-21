#include "../src/editor/scene/PlaybackController.h"
#include "../src/editor/scene/SceneManager.h"
#include "../src/editor/scene/SceneObject.h"
#include "../src/editor/scene/SessionAudio.h"
#include "../src/editor/scene/ProjectAssets.h"
#include <Core/ECS/Components/MeshFilter.h>
#include <set>
#include <SDL.h>
#include "../src/editor/automation/StateRules.h"
#include "../src/editor/automation/PhysicsEditing.h"
#include <Core/Systems/PhysicsResources.h>
#include <Core/Systems/ScriptingSystem.h>
#include <Core/ECS/Components/MeshRender.h>
#include <Core/ECS/Components/ScriptComponent.h>
#include <Rendering/Core/Camera3D.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <limits>
#include <Core/CoreUtilities/CoreEngineData.h>
using namespace ENGINE_EDITOR;
using namespace ENGINE_CORE::ECS;
static void Check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
static void Write(const std::filesystem::path& path, const std::string& bytes) { std::ofstream stream(path, std::ios::binary); stream << bytes; }
namespace {
class RecordedAudio final : public AudioBackend {
public:
    struct Voice { bool active = false, paused = false; int volume = 80; uintptr_t identity = 0; };
    std::map<int, Voice> voices;
    int starts = 0, pauses = 0, resumes = 0, stops = 0;
    int Start(const std::string&, int, int channel) override {
        if (channel == -1) { channel = 0; while (voices[channel].active) ++channel; }
        auto& v = voices[channel]; v.active = true; v.paused = false; v.identity = ++starts; return channel;
    }
    bool Active(int voice) override { return voices[voice].active; }
    bool Paused(int voice) override { return voices[voice].paused; }
    uintptr_t Identity(int voice) override { return voices[voice].identity; }
    void Halt(int voice) override { ++stops; voices[voice].active = false; voices[voice].paused = false; }
    void Pause(int voice) override { ++pauses; voices[voice].paused = true; }
    void Resume(int voice) override { ++resumes; voices[voice].paused = false; }
    int Volume(int voice, int value) override { auto& v = voices[voice]; int old = v.volume; if (value >= 0) v.volume = value; return old; }
};
entt::entity Named(entt::registry& registry, const std::string& name) {
    for (auto e : registry.view<Identification>()) if (registry.get<Identification>(e).name == name) return e;
    return entt::null;
}
}
void RuntimeLifecycleTests() {
    auto backend = std::make_shared<RecordedAudio>();
    {
        backend->Start("external", 0, 7);
        SessionAudio audio(backend); audio.MusicVolume(50); audio.PlayMusic("music", -1);
        audio.PlayFx("fx", 0, 0); audio.PauseMusic(); audio.Pause();
        const int resumes = backend->resumes; audio.Resume();
        Check(backend->resumes == resumes + 1 && backend->Paused(AudioBackend::Music), "Session Resume restarted script-paused music");
        audio.FxVolume(10, -1); Check(backend->voices[7].volume == 80, "Runtime changed external volume");
        audio.StopFx(-1); Check(backend->Active(7) && !backend->Active(0), "Runtime stopped an external channel");
        audio.Stop(); Check(backend->voices[0].volume == 80 && backend->voices[AudioBackend::Music].volume == 80, "Audio baseline not restored");
        bool refused = false; try { audio.PlayFx("other", 0, 7); } catch (...) { refused = true; }
        Check(refused && backend->Active(7), "Runtime replaced external audio");
        const int first = audio.PlayFx("short", 0, -1); backend->voices[first].active = false;
        const int next = audio.PlayFx("again", 0, -1); Check(backend->Active(next), "Reused finished channel was halted");
    }
    ENGINE_CORE::GameInput input;
    input.Allow(true); input.Key(1, true); input.Button(1, true); input.Move(4, 5); input.Wheel(0, 1);
    Check(input.KeyJustPressed(1) && input.MouseJustPressed(1) && input.Offset().first == 4, "Game input missing");
    input.Allow(false); Check(!input.KeyPressed(1) && input.Offset() == std::pair{0, 0}, "Inactive input leaked");
    input.Allow(true); Check(!input.KeyJustPressed(1) && !input.MouseJustPressed(1) && input.WheelY() == 0, "Focus replayed stale events");

    auto& playback = PlaybackController::Get(); auto& scenes = SCENE_MANAGER();
    {
        auto documentation = SceneScriptFiles::Read("EDITOR/assets/scripts/README.md");
        auto first = documentation.find("```lua\n"), last = documentation.find("\n```", first);
        Check(first != std::string::npos && last != std::string::npos, "Missing documented Lua example");
        auto example = scenes.Scripts().ScriptPath("documented-example");
        std::filesystem::create_directories(example.parent_path()); Write(example, documentation.substr(first + 7, last - first - 7));
        sol::state lua; lua.open_libraries(sol::lib::base); ENGINE_CORE::Systems::ScriptingSystem host;
        Check(host.LoadSceneScript(lua, example) && host.Start() && host.Tick(.1) && host.Stop(), "Documented Scene example does not run");
    }
    playback.Stop(); scenes.CleanUp(); playback.SetAudioBackend(backend);
    Check(scenes.CreateAndActivate("Runtime"), "Runtime Scene creation failed");
    auto scene = scenes.GetCurrentScene(); auto path = scenes.Scripts().ScriptPath("Runtime");
    auto root = Automation::CreateEditableEntity(scene->GetRegistry(), "root");
    auto child = Automation::CreateEditableEntity(scene->GetRegistry(), "child");
    child.GetComponent<Identification>().parent_id = int32_t(entt::to_integral(root.GetEntity()));
    root.GetComponent<TransformComponent>().position.y = 10;
    root.AddComponent<MeshRender>().materials.emplace_back();
    ENGINE_CORE::ECS::PhysicsAttributes attrs; attrs.rb_type = rp3d::BodyType::DYNAMIC;
    ENGINE_CORE::Systems::BuildPhysicsBody(root.GetRegistry(), root.GetEntity(), attrs);
    auto* editBody = root.GetComponent<PhysicsComponent>().GetRigidBody();
    scene->CopySceneToRuntime();
    auto& clone = scene->GetRuntimeRegistry().GetRegistry(); auto runtimeRoot = Named(clone, "root"), runtimeChild = Named(clone, "child");
    Check(clone.get<Identification>(runtimeChild).parent_id == int32_t(entt::to_integral(runtimeRoot)), "Runtime hierarchy not remapped");
    Check(clone.get<PhysicsComponent>(runtimeRoot).GetRigidBody() != editBody, "Runtime shares edit body");
    Check(scene->GetRegistry().GetContext<std::shared_ptr<rp3d::PhysicsWorld>>() != scene->GetRuntimeRegistry().GetContext<std::shared_ptr<rp3d::PhysicsWorld>>(), "Runtime shares edit world");
    clone.get<MeshRender>(runtimeRoot).materials[0].metallic = .25f;
    Check(root.GetComponent<MeshRender>().materials[0].metallic == 1, "Runtime shares material values");
    std::weak_ptr<rp3d::PhysicsWorld> oldWorld = scene->GetRuntimeRegistry().GetContext<std::shared_ptr<rp3d::PhysicsWorld>>();
    clone.destroy(runtimeChild); Check(root.GetRegistry().valid(child.GetEntity()), "Runtime deletion changed edit entity");
    scene->ClearRuntimeScene(); Check(!scene->GetRuntimeRegistry().GetRegistry().ctx().contains<std::shared_ptr<rp3d::PhysicsWorld>>(), "Runtime retained context");
    Check(oldWorld.expired(), "Runtime physics world retained after reset");
    root.GetComponent<TransformComponent>().position.x = std::numeric_limits<float>::quiet_NaN();
    Check(!playback.Play() && !scene->GetRuntimeRegistry().GetRegistry().ctx().contains<std::shared_ptr<rp3d::PhysicsWorld>>(), "Partial build retained physics context");
    Check(std::isnan(root.GetComponent<TransformComponent>().position.x), "Failed build changed edit pose"); root.GetComponent<TransformComponent>().position.x = 0;
    struct Unsupported { int value = 0; }; root.AddComponent<Unsupported>();
    Check(!playback.Play() && playback.CanEdit() && !playback.HasRuntimeSession(), "Unsupported component silently copied"); root.RemoveComponent<Unsupported>();
    root.AddComponent<ScriptComponent>(); Check(!playback.Play(), "Legacy script closure copied"); root.RemoveComponent<ScriptComponent>();
    root.GetComponent<Identification>().parent_id = int32_t(entt::to_integral(child.GetEntity()));
    Check(!playback.Play(), "Cyclic hierarchy accepted"); root.GetComponent<Identification>().parent_id = -1;
    Write(path, "assert(module_loaded==nil);module_loaded=true;return {on_start=function() assert(started==nil);started=true;local born=Entity('spawned','');born:add_component(Transform(0,1,2,1,1,1,0,0,0));local v=Registry.get_entities(Transform);v:for_each(function(e) if e:name()=='child' then e:get_component(Transform).position.x=42 end end) end,on_update=function(dt) assert(started and dt==ENGINE_DeltaTime()) end,on_stop=function() assert(started);started=false end}");
    Check(playback.Play(), playback.LastError().c_str()); auto context = playback.Context();
    Check(!playback.CanEdit() && playback.VisibleRegistry() == &scene->GetRuntimeRegistry(), "Wrong Playing registry/permissions");
    Check(Named(scene->GetRuntimeRegistry().GetRegistry(), "spawned") != entt::null && Named(root.GetRegistry(), "spawned") == entt::null, "Lua used edit registry");
    Check(playback.Play() && playback.Context() == context, "Repeated Play restarted session");
    Check(scene->GetRuntimeRegistry().GetRegistry().get<TransformComponent>(Named(scene->GetRuntimeRegistry().GetRegistry(), "child")).position.x == 42 && child.GetComponent<TransformComponent>().position.x == 0, "Lua component/view binding escaped runtime Registry");
    for (int i = 0; i < 30; ++i) playback.Tick(1.0/30);
    Check(playback.PhysicsSteps() == 60 && editBody->getTransform().getPosition().y == 10, "Physics leaked or wrong number of steps");
    playback.Pause(); const auto time = context->clock.Seconds(); const auto steps = playback.PhysicsSteps();
    playback.Pause(); playback.Tick(200);
    auto& pausedWorld = scene->GetRuntimeRegistry().GetContext<std::shared_ptr<rp3d::PhysicsWorld>>();
    ENGINE_CORE::Systems::RefreshDebugGeometry(*pausedWorld);
    Check(playback.PhysicsSteps() == steps && editBody->getTransform().getPosition().y == 10, "Debug rendering advanced physics");
    Check(context->clock.Seconds() == time && playback.PhysicsSteps() == steps && !playback.CanEdit(), "Pause advanced simulation");
    Check(!scenes.AddScene("forbidden") && !scenes.SetCurrentScene("") && !scenes.ChangeSceneName("Runtime", "bad"), "Paused Scene write accepted");
    Check(playback.Play() && playback.Context() == context, "Resume replaced Lua/session");
    Check(scene->GetRuntimeRegistry().GetRegistry().view<entt::entity>().size() == 3, "Resume/Play repeated on_start"); playback.Tick(1.0/60);
    Check(playback.PhysicsSteps() == steps + 1, "Resume caught up paused wall time");
    playback.Stop(); playback.Stop();
    Check(playback.CanEdit() && !playback.HasRuntimeSession() && playback.VisibleRegistry() == &scene->GetRegistry(), "Stop did not restore edit registry");
    Check(root.GetRegistry().view<entt::entity>().size() == 2 && root.GetComponent<TransformComponent>().position.y == 10, "Stop failed restoration");

    for (const auto& script : {"", "invalid lua !", "return {}", "error('top level error')", "return {on_start=function() error('start') end,on_update=function() end,on_stop=function() end}",
        "return {on_start=function() AssetManager.add_texture('bad','bad',false) end,on_update=function() end,on_stop=function() end}",
        "ENGINE_RunScript('missing-scene-helper.lua')"}) {
        Write(path, script); Check(!playback.Play() && playback.CanEdit() && !playback.HasRuntimeSession(), "Invalid script started or leaked runtime");
        Check(!playback.LastError().empty(), "Missing script failure diagnostic");
    }
    const bool editPhysics = CORE_GLOBALS().IsPhysicsEnabled();
    Write(path, "return {on_start=function() t=Timer();t:start();ENGINE_DisablePhysics();assert(not ENGINE_IsPhysicsEnabled());Camera.set_position(vec3(2,3,4)) end,on_update=function(dt) assert(t:elapsed_ms()==ENGINE_GetTicks());ENGINE_EnablePhysics() end,on_stop=function() error('stop') end}");
    for (int i = 0; i < 3; ++i) { Check(playback.Play(), playback.LastError().c_str()); playback.Tick(1.0/60); playback.Stop(); Check(playback.CanEdit(), "Stop error prevented cleanup"); }
    Check(CORE_GLOBALS().IsPhysicsEnabled() == editPhysics, "Runtime mutated editor physics settings");
    Check(scene->GetRegistry().GetContext<std::shared_ptr<ENGINE_RENDERING::Camera3D>>()->GetPosition() == glm::vec3(0,10,10), "Runtime mutated editor camera");
    Write(path, SceneScriptFiles::Template());
    float lastY = 0;
    for (int fps : {30, 60, 144}) {
        Check(playback.Play(), playback.LastError().c_str());
        for (int frame = 0; frame < fps; ++frame) playback.Tick(1.0 / fps);
        auto& runtime = scene->GetRuntimeRegistry().GetRegistry(); auto e = Named(runtime, "root");
        const float y = runtime.get<TransformComponent>(e).position.y;
        Check(playback.PhysicsSteps() == 60 && (fps == 30 || std::abs(y - lastY) < 1e-5f), "Real physics differs by editor FPS");
        lastY = y; playback.Stop();
    }
    Write(path, "return {on_start=function() end,on_update=function() error('update') end,on_stop=function() end}");
    Check(playback.Play(), "Error test failed start"); playback.Tick(.1);
    Check(playback.State() == PlaybackState::Paused && playback.PhysicsSteps() == 0 && playback.LastError().find("on_update") != std::string::npos, "Update error did not pause before physics");
    scenes.CleanUp(); Check(playback.CanEdit(), "Shutdown did not stop paused session"); playback.SetAudioBackend({});
    {
        struct Restore {
            std::filesystem::path cwd = std::filesystem::current_path();
            std::filesystem::path assets = SCENE_MANAGER().Scripts().AssetRoot();
            ~Restore() { PlaybackController::Get().Stop(); SCENE_MANAGER().CleanUp(); PlaybackController::Get().SetAudioBackend({}); SCENE_MANAGER().Scripts().SetAssetRoot(assets); std::filesystem::current_path(cwd); }
        } restore;
        const auto editor = restore.cwd / "EDITOR";
        const auto assets = editor / "assets";
        auto manifest = ReadProjectAssets(assets);
        Check(!manifest.empty(), "Default demo preload manifest is empty");
        std::set<std::string> models{"cube", "sphere", "capsule", "plane"}, textures;
        for (const auto& request : manifest) {
            // Exercise the real CPU importer and dependency decoding without a GL window.
            auto prepared = Automation::PrepareImport(request, std::make_shared<std::atomic_bool>(false));
            const auto type = Automation::Str(request, "asset_type");
            if (type == "model") models.insert(Automation::Str(request, "asset_key"));
            if (type == "texture") textures.insert(Automation::Str(request, "asset_key"));
            for (const char* key : {"path", "vertex_path", "fragment_path"}) {
                const auto text = Automation::Str(request, key);
                if (!text.empty()) Check(std::filesystem::is_regular_file(std::filesystem::path(std::u8string(text.begin(), text.end()))), "Default preload source is missing");
            }
        }
        std::filesystem::current_path(editor);
        scenes.Scripts().SetAssetRoot(assets);
        playback.SetAudioBackend(backend);
        Check(scenes.CreateDefault(), scenes.LastError().c_str());
        auto demo = scenes.GetCurrentScene();
        auto& authored = demo->GetRegistry().GetRegistry();
        Check(authored.view<entt::entity>().size() == 24 && !backend->Active(AudioBackend::Music), "Default construction did not expose editable, silent initial content");
        const auto authoredBody = Named(authored, "body");
        authored.get<TransformComponent>(authoredBody).position = glm::vec3(7,4,0);
        authored.get<MeshRender>(authoredBody).materials[0].color = glm::vec4(.2f,.3f,.4f,1);
        auto changedPhysics = authored.get<PhysicsComponent>(authoredBody).GetAttr(); changedPhysics.rb_Mass = 14;
        Automation::ApplyPhysics(authored, authoredBody, changedPhysics);
        auto* authoredRigidBody = authored.get<PhysicsComponent>(authoredBody).GetRigidBody();
        scenes.Scripts().SetAssetRoot(restore.assets);
        Check(scenes.CreateAndActivate("Other"), "Other Scene creation failed");
        scenes.Scripts().SetAssetRoot(assets);
        Check(scenes.SetCurrentScene("default"), "Default switch test failed");
        Check(authored.get<TransformComponent>(authoredBody).position.x == 7 && authored.view<entt::entity>().size() == 24, "Switch reset authored defaults");
        for (int repeat = 0; repeat < 2; ++repeat) {
            const bool started = playback.Play(); Check(started, playback.LastError().c_str());
            auto context = playback.Context(); auto& runtime = demo->GetRuntimeRegistry().GetRegistry();
            const auto count = runtime.view<entt::entity>().size();
            Check(count == 24, "Default demo entity population changed");
            auto& body = runtime.get<PhysicsComponent>(Named(runtime, "body"));
            Check(body.GetRigidBody() != authoredRigidBody && body.GetAttr().rb_Mass == 14, "Play lost edited physics or shared its body");
            Check(runtime.get<TransformComponent>(Named(runtime, "body")).position == glm::vec3(7,4,0) && runtime.get<MeshRender>(Named(runtime, "body")).materials[0].color == glm::vec4(.2f,.3f,.4f,1), "Play reset edited transform/material");
            Check(body.GetRigidBody()->getLinearDamping() == 5 && body.GetRigidBody()->getAngularDamping() == 10, "Legacy player damping was clamped or lost");
            for (const char* name : {"body", "glass", "platform_2", "ground", "cyborg", "nanosuit", "gold_ball", "direction_light_1"})
                Check(Named(runtime, name) != entt::null, "Default demo entity missing");
            for (auto entity : runtime.view<MeshFilter>()) Check(models.contains(runtime.get<MeshFilter>(entity).mesh), "Default demo model not preloaded");
            for (auto entity : runtime.view<MeshRender>()) for (const auto& material : runtime.get<MeshRender>(entity).materials)
                for (const auto& [slot, key] : material.m_textures) Check(key.empty() || textures.contains(key), "Default demo texture not preloaded");
            Check(backend->Active(AudioBackend::Music), "Default demo music did not start");
            for (int frame = 0; frame < 90; ++frame) playback.Tick(1.0/60);
            Check(playback.State() == PlaybackState::Playing, playback.LastError().c_str());
            playback.Pause(); const auto time = context->clock.Seconds();
            playback.Tick(10); Check(context->clock.Seconds() == time, "Default demo advanced while Paused");
            Check(playback.Play() && playback.Context() == context && runtime.view<entt::entity>().size() == count, "Default demo Resume reinitialized entities");
            playback.SetGameInputAllowed(true); context->input.Allow(true); context->input.Key(SDLK_f, true);
            playback.Tick(1.0/60);
            Check(playback.State() == PlaybackState::Playing && runtime.view<entt::entity>().size() == count + 1, playback.LastError().c_str());
            context->input.Key(SDLK_f, false);
            for (int frame = 0; frame < 65; ++frame) playback.Tick(1.0/60);
            Check(playback.State() == PlaybackState::Playing && runtime.view<entt::entity>().size() == count, "Default demo bullet update/timer failed");
            playback.Stop();
            Check(!backend->Active(AudioBackend::Music) && authored.view<entt::entity>().size() == 24 && authored.valid(authoredBody), "Stop discarded authored initial entities");
            Check(authored.get<TransformComponent>(authoredBody).position == glm::vec3(7,4,0) && authored.get<PhysicsComponent>(authoredBody).GetAttr().rb_Mass == 14 && authored.get<PhysicsComponent>(authoredBody).GetRigidBody() == authoredRigidBody, "Stop lost authored edits or replaced their identity");
        }
        std::cout << "Default demo migration, preload coverage and repeated Play/Pause/Stop passed\n";
    }
    std::cout << "Runtime lifecycle, isolation, Lua, clock, input and audio ownership tests passed\n";
}
