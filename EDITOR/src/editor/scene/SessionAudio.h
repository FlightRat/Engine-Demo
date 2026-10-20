#pragma once
#include <Core/CoreUtilities/GameplayContext.h>
#include <map>
#include <optional>

namespace ENGINE_EDITOR {
// -2 is music; -1 is an automatic sound channel. Nonnegative values are channels.
class AudioBackend {
public:
    static constexpr int Music = -2;
    virtual ~AudioBackend() = default;
    virtual int Start(const std::string& key, int loops, int voice) = 0;
    virtual bool Active(int voice) = 0;
    virtual bool Paused(int voice) = 0;
    virtual uintptr_t Identity(int voice) = 0;
    virtual void Halt(int voice) = 0;
    virtual void Pause(int voice) = 0;
    virtual void Resume(int voice) = 0;
    virtual int Volume(int voice, int value = -1) = 0;
};
std::shared_ptr<AudioBackend> CreateSDLAudioBackend();
class SessionAudio final : public ENGINE_CORE::GameplayAudio {
    struct Voice { uintptr_t identity; int oldVolume; bool pausedBySession = false; };
    std::shared_ptr<AudioBackend> m_Backend;
    std::map<int, Voice> m_Voices;
    std::optional<int> m_MusicVolume, m_FxVolume;
    bool Owns(int voice);
    int Play(const std::string& key, int loops, int voice);
    void Finish(int voice);
    static int Percent(int value);
public:
    explicit SessionAudio(std::shared_ptr<AudioBackend> backend) : m_Backend(std::move(backend)) {}
    ~SessionAudio() override;
    void PlayMusic(const std::string& key, int loops) override { Play(key, loops, AudioBackend::Music); }
    void StopMusic() override { Finish(AudioBackend::Music); }
    void PauseMusic() override;
    void ResumeMusic() override;
    void MusicVolume(int volume) override;
    bool MusicPlaying() override { return Owns(AudioBackend::Music) && m_Backend->Active(AudioBackend::Music); }
    int PlayFx(const std::string& key, int loops, int channel) override;
    void StopFx(int channel) override;
    void FxVolume(int volume, int channel) override;
    bool FxPlaying(int channel) override;
    void Pause() override;
    void Resume() override;
    void Stop() override;
};
}
