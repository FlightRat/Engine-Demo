#pragma once
#include "PlaybackState.h"
#include <Core/CoreUtilities/GameplayContext.h>
#include <memory>
#include <string>
#include <cstdint>
union SDL_Event;

namespace ENGINE_CORE::ECS { class Registry; }
namespace ENGINE_EDITOR {
class SceneObject;
class AudioBackend;
class PlaybackController {
    struct RuntimeSession;
    std::unique_ptr<RuntimeSession> m_Session;
    std::shared_ptr<AudioBackend> m_Backend;
    PlaybackState m_State = PlaybackState::Editing;
    std::string m_Error;
    bool m_InputAllowed = false;
    uint64_t m_TransitionSerial = 0;
    float m_Left = 0, m_Top = 0, m_Right = 0, m_Bottom = 0;
    uint32_t m_GameWindow = 0;
    PlaybackController();
    ~PlaybackController();
    void Transition(PlaybackState state);
public:
    static PlaybackController& Get();
    PlaybackController(const PlaybackController&) = delete;
    PlaybackController& operator=(const PlaybackController&) = delete;
    PlaybackState State() const { return m_State; }
    uint64_t TransitionSerial() const { return m_TransitionSerial; }
    bool CanEdit() const { return ENGINE_EDITOR::CanEdit(m_State); }
    bool HasRuntimeSession() const { return bool(m_Session); }
    bool IsSceneRunning(const SceneObject* scene) const;
    bool Play();
    void Pause();
    void Stop();
    void Tick(double dt);
    void SetGameInputAllowed(bool allowed);
    void SetGameViewport(float left, float top, float right, float bottom, uint32_t window, bool focused, bool textInput);
    void HandleInput(const SDL_Event& event, bool textInput);
    std::shared_ptr<ENGINE_CORE::GameplayContext> Context() const;
    ENGINE_CORE::ECS::Registry* VisibleRegistry() const;
    uint64_t PhysicsSteps() const;
    const std::string& LastError() const { return m_Error; }
    void SetAudioBackend(std::shared_ptr<AudioBackend> backend);
};
}
