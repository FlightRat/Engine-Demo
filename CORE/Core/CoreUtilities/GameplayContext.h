#pragma once
#include "GameClock.h"
#include <memory>
#include <set>
#include <string>
#include <utility>

namespace ENGINE_CORE {
class GameInput {
    bool m_Allowed = false;
    std::set<int> m_Keys, m_Buttons, m_KeyDown, m_KeyUp, m_MouseDown, m_MouseUp;
    int m_X = 0, m_Y = 0, m_WheelX = 0, m_WheelY = 0;
public:
    void EndFrame() { m_KeyDown.clear(); m_KeyUp.clear(); m_MouseDown.clear(); m_MouseUp.clear(); m_X = m_Y = m_WheelX = m_WheelY = 0; }
    void Allow(bool allowed) { if (m_Allowed != allowed) EndFrame(); m_Allowed = allowed; }
    void Key(int key, bool down) {
        if (down) { if (m_Keys.insert(key).second && m_Allowed) m_KeyDown.insert(key); }
        else if (m_Keys.erase(key) && m_Allowed) m_KeyUp.insert(key);
    }
    void Button(int key, bool down) {
        if (down) { if (m_Buttons.insert(key).second && m_Allowed) m_MouseDown.insert(key); }
        else if (m_Buttons.erase(key) && m_Allowed) m_MouseUp.insert(key);
    }
    void Move(int x, int y) { if (m_Allowed) { m_X += x; m_Y += y; } }
    void Wheel(int x, int y) { if (m_Allowed) { m_WheelX += x; m_WheelY += y; } }
    bool KeyPressed(int key) const { return m_Allowed && m_Keys.contains(key); }
    bool KeyJustPressed(int key) const { return m_Allowed && m_KeyDown.contains(key); }
    bool KeyJustReleased(int key) const { return m_Allowed && m_KeyUp.contains(key); }
    bool MousePressed(int key) const { return m_Allowed && m_Buttons.contains(key); }
    bool MouseJustPressed(int key) const { return m_Allowed && m_MouseDown.contains(key); }
    bool MouseJustReleased(int key) const { return m_Allowed && m_MouseUp.contains(key); }
    std::pair<int, int> Offset() const { return m_Allowed ? std::pair{m_X, m_Y} : std::pair{0, 0}; }
    int WheelX() const { return m_Allowed ? m_WheelX : 0; }
    int WheelY() const { return m_Allowed ? m_WheelY : 0; }
};
class GameplayAudio {
public:
    virtual ~GameplayAudio() = default;
    virtual void PlayMusic(const std::string&, int) = 0;
    virtual void StopMusic() = 0;
    virtual void PauseMusic() = 0;
    virtual void ResumeMusic() = 0;
    virtual void MusicVolume(int) = 0;
    virtual bool MusicPlaying() = 0;
    virtual int PlayFx(const std::string&, int, int) = 0;
    virtual void StopFx(int) = 0;
    virtual void FxVolume(int, int) = 0;
    virtual bool FxPlaying(int) = 0;
    virtual void Pause() = 0;
    virtual void Resume() = 0;
    virtual void Stop() = 0;
};
struct GameplayContext {
    GameClock clock;
    GameInput input;
    std::shared_ptr<GameplayAudio> audio;
    bool physicsEnabled = true, renderColliders = false;
};
}
