#include "PlaybackController.h"
#include "SceneManager.h"
#include "SceneObject.h"
#include "SessionAudio.h"
#include "../automation/StateRules.h"
#include <Core/Systems/ScriptingSystem.h>
#include <Core/Systems/PhysicsSystem.h>
#include <Core/Systems/PhysicsResources.h>
#include <Physics/ContactListener.h>
#include <Logger/Logger.h>
#include <SDL.h>

namespace ENGINE_EDITOR {
using namespace ENGINE_CORE;
struct PlaybackController::RuntimeSession {
    std::shared_ptr<SceneObject> scene;
    std::shared_ptr<GameplayContext> context = std::make_shared<GameplayContext>();
    std::unique_ptr<sol::state> lua;
    Systems::ScriptingSystem script;
    Systems::PhysicsSystem physics;
    std::shared_ptr<ENGINE_PHYSICS::ContactListener> listener;
    uint64_t steps = 0;
    ~RuntimeSession() {
        try { if (context->audio) context->audio->Stop(); } catch (...) {}
        script.Release(); lua.reset();
        if (scene) scene->ClearRuntimeScene();
    }
};
PlaybackController::PlaybackController() = default;
PlaybackController::~PlaybackController() { m_Session.reset(); }
PlaybackController& PlaybackController::Get() { static PlaybackController controller; return controller; }
void PlaybackController::SetAudioBackend(std::shared_ptr<AudioBackend> backend) {
    if (!CanEdit()) throw std::logic_error("Cannot replace audio backend during playback");
    m_Backend = std::move(backend);
}
bool PlaybackController::IsSceneRunning(const SceneObject* scene) const { return m_Session && m_Session->scene.get() == scene; }
void PlaybackController::Transition(PlaybackState state) {
    if (state == m_State) return;
    m_State = state; ++m_TransitionSerial; Automation::NotifyManualEdit();
}
std::shared_ptr<GameplayContext> PlaybackController::Context() const { return m_Session ? m_Session->context : nullptr; }
uint64_t PlaybackController::PhysicsSteps() const { return m_Session ? m_Session->steps : 0; }
ECS::Registry* PlaybackController::VisibleRegistry() const {
    if (m_Session) return &m_Session->scene->GetRuntimeRegistry();
    auto scene = SCENE_MANAGER().GetCurrentScene(); return scene ? &scene->GetRegistry() : nullptr;
}
bool PlaybackController::Play() {
    if (m_State == PlaybackState::Playing) return true;
    if (m_State == PlaybackState::Paused) {
        m_Session->context->audio->Resume(); m_Session->context->clock.Freeze();
        m_Session->context->input.EndFrame(); Transition(PlaybackState::Playing); return true;
    }
    auto scene = SCENE_MANAGER().GetCurrentScene(); if (!scene) return false;
    try {
        auto candidate = std::make_unique<RuntimeSession>(); candidate->scene = scene;
        scene->CopySceneToRuntime();
        candidate->context->audio = std::make_shared<SessionAudio>(m_Backend ? m_Backend : CreateSDLAudioBackend());
        auto& registry = scene->GetRuntimeRegistry();
        registry.AddToContext<std::shared_ptr<GameplayContext>>(candidate->context);
        candidate->listener = std::make_shared<ENGINE_PHYSICS::ContactListener>();
        registry.AddToContext<std::shared_ptr<ENGINE_PHYSICS::ContactListener>>(candidate->listener);
        auto& world = registry.GetContext<std::shared_ptr<rp3d::PhysicsWorld>>();
        world->setEventListener(candidate->listener.get());
        world->getDebugRenderer().setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::COLLISION_SHAPE, true);
        candidate->lua = std::make_unique<sol::state>();
        candidate->lua->open_libraries(sol::lib::base, sol::lib::math, sol::lib::os, sol::lib::table, sol::lib::io, sol::lib::string, sol::lib::package);
        Systems::ScriptingSystem::RegisterLuaBindings(*candidate->lua, registry);
        Systems::ScriptingSystem::RegisterLuaFunctions(*candidate->lua, candidate->context);
        if (!candidate->script.LoadSceneScript(*candidate->lua, SCENE_MANAGER().Scripts().ScriptPath(scene->GetName())) || !candidate->script.Start())
            throw std::runtime_error(candidate->script.LastError());
        Automation::ResetSelection(); TOOL_CTX().runtimeSelectedEntity.reset();
        m_Session = std::move(candidate); m_Error.clear(); m_InputAllowed = false;
        Transition(PlaybackState::Playing); return true;
    } catch (const std::exception& error) { m_Error = error.what(); ENGINE_ERROR("Cannot Play: {}", m_Error); return false; }
}
void PlaybackController::Pause() {
    if (m_State != PlaybackState::Playing) return;
    m_Session->context->clock.Freeze(); m_Session->context->input.Allow(false);
    m_Session->context->audio->Pause(); Transition(PlaybackState::Paused);
}
void PlaybackController::Stop() {
    if (!m_Session) return;
    m_Session->context->input.Allow(false); TOOL_CTX().runtimeSelectedEntity.reset();
    if (!m_Session->script.Stop()) { m_Error = m_Session->script.LastError(); ENGINE_ERROR("Stop callback failed: {}", m_Error); }
    m_Session.reset(); m_InputAllowed = false; Transition(PlaybackState::Editing);
}
void PlaybackController::SetGameInputAllowed(bool allowed) {
    m_InputAllowed = allowed;
    if (m_Session) m_Session->context->input.Allow(allowed && m_State == PlaybackState::Playing);
}
void PlaybackController::SetGameViewport(float left, float top, float right, float bottom, uint32_t window, bool focused, bool textInput) {
    m_Left = left; m_Top = top; m_Right = right; m_Bottom = bottom; m_GameWindow = window;
    SetGameInputAllowed(focused && !textInput);
}
void PlaybackController::HandleInput(const SDL_Event& e, bool textInput) {
    if (!m_Session) return;
    auto& input = m_Session->context->input;
    if (textInput) SetGameInputAllowed(false);
    auto inside = [&](uint32_t id, int x, int y) {
        auto* window = SDL_GetWindowFromID(id); int wx = 0, wy = 0;
        if (window) SDL_GetWindowPosition(window, &wx, &wy);
        return id == m_GameWindow && x + wx >= m_Left && x + wx < m_Right && y + wy >= m_Top && y + wy < m_Bottom;
    };
    switch (e.type) {
    case SDL_KEYDOWN:
        if (e.key.windowID != m_GameWindow) SetGameInputAllowed(false);
        if (!e.key.repeat) input.Key(e.key.keysym.sym, true); break;
    case SDL_KEYUP: input.Key(e.key.keysym.sym, false); break;
    case SDL_MOUSEBUTTONDOWN:
        if (!inside(e.button.windowID, e.button.x, e.button.y)) SetGameInputAllowed(false);
        input.Button(e.button.button, true); break;
    case SDL_MOUSEBUTTONUP: input.Button(e.button.button, false); break;
    case SDL_MOUSEMOTION:
        if (inside(e.motion.windowID, e.motion.x, e.motion.y)) input.Move(e.motion.xrel, e.motion.yrel); break;
    case SDL_MOUSEWHEEL: {
        int x = 0, y = 0; SDL_GetGlobalMouseState(&x, &y);
        if (e.wheel.windowID == m_GameWindow && x >= m_Left && x < m_Right && y >= m_Top && y < m_Bottom) input.Wheel(e.wheel.x, e.wheel.y);
        break;
    }
    case SDL_WINDOWEVENT:
        if (e.window.windowID == m_GameWindow && e.window.event == SDL_WINDOWEVENT_FOCUS_LOST) { SetGameInputAllowed(false); input = GameInput{}; }
        break;
    }
}
void PlaybackController::Tick(double dt) {
    if (m_State != PlaybackState::Playing) return;
    try {
    auto& session = *m_Session; auto& context = *session.context;
    context.input.Allow(m_InputAllowed); context.clock.Advance(dt);
    if (!session.script.Tick(context.clock.Delta())) {
        m_Error = session.script.LastError(); ENGINE_ERROR("Runtime paused: {}", m_Error); Pause(); return;
    }
    auto& registry = session.scene->GetRuntimeRegistry(); auto& entities = registry.GetRegistry();
    auto& world = registry.GetContext<std::shared_ptr<rp3d::PhysicsWorld>>();
    for (auto e : entities.view<ECS::TransformComponent, ECS::PhysicsComponent>()) {
        auto& component = entities.get<ECS::PhysicsComponent>(e);
        if (!component.GetRigidBody()) Systems::BuildPhysicsBody(entities, e, component.GetAttr());
        component.SetDebug(context.renderColliders);
    }
    world->setIsDebugRenderingEnabled(context.renderColliders);
    while (context.clock.ConsumeStep()) if (context.physicsEnabled) { world->update(float(GameClock::FixedStep)); ++session.steps; }
    if (context.physicsEnabled) session.physics.Update(registry, 0);
    context.input.EndFrame();
    } catch (const std::exception& error) { m_Error = std::string("Runtime update: ") + error.what(); ENGINE_ERROR("{}", m_Error); Pause(); }
}
}
