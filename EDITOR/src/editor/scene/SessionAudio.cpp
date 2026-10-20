#include "SessionAudio.h"
#include <Core/ECS/MainRegistry.h>
#include <Core/Resources/AssetManager.h>
#include <SDL_mixer.h>
#include <stdexcept>
#include <vector>

namespace ENGINE_EDITOR {
namespace {
class SDLBackend final : public AudioBackend {
    std::map<int, uintptr_t> m_Identities;
public:
    int Start(const std::string& key, int loops, int voice) override {
        auto& assets = MAIN_REGISTRY().GetAssetManager();
        if (voice == Music) {
            auto sound = assets.GetMusic(key);
            if (!sound || !sound->GetMusicPtr()) throw std::runtime_error("Music asset is not loaded: " + key);
            if (Mix_PlayMusic(sound->GetMusicPtr(), loops) < 0) throw std::runtime_error(Mix_GetError());
            ENGINE_SOUNDS::Music::ActiveMusic = sound->GetMusicPtr();
            m_Identities[Music] = reinterpret_cast<uintptr_t>(sound->GetMusicPtr()); return Music;
        }
        auto sound = assets.GetSoundFx(key);
        if (!sound || !sound->GetSoundFxPtr()) throw std::runtime_error("Sound asset is not loaded: " + key);
        const int channel = Mix_PlayChannel(voice, sound->GetSoundFxPtr(), loops);
        if (channel < 0) throw std::runtime_error(Mix_GetError());
        m_Identities[channel] = reinterpret_cast<uintptr_t>(sound->GetSoundFxPtr()); return channel;
    }
    bool Active(int voice) override { return voice == Music ? Mix_PlayingMusic() != 0 : Mix_Playing(voice) != 0; }
    bool Paused(int voice) override { return voice == Music ? Mix_PausedMusic() != 0 : Mix_Paused(voice) != 0; }
    uintptr_t Identity(int voice) override {
        if (voice == Music) return reinterpret_cast<uintptr_t>(ENGINE_SOUNDS::Music::ActiveMusic);
        return reinterpret_cast<uintptr_t>(Mix_GetChunk(voice));
    }
    void Halt(int voice) override { if (voice == Music) Mix_HaltMusic(); else Mix_HaltChannel(voice); }
    void Pause(int voice) override { if (voice == Music) Mix_PauseMusic(); else Mix_Pause(voice); }
    void Resume(int voice) override { if (voice == Music) Mix_ResumeMusic(); else Mix_Resume(voice); }
    int Volume(int voice, int value) override { return voice == Music ? Mix_VolumeMusic(value) : Mix_Volume(voice, value); }
};
}
std::shared_ptr<AudioBackend> CreateSDLAudioBackend() { return std::make_shared<SDLBackend>(); }
SessionAudio::~SessionAudio() { try { Stop(); } catch (...) {} }
int SessionAudio::Percent(int value) {
    if (value < 0 || value > 100) throw std::invalid_argument("Volume must be in [0,100]");
    return value * MIX_MAX_VOLUME / 100;
}
bool SessionAudio::Owns(int voice) {
    auto found = m_Voices.find(voice);
    return found != m_Voices.end() && found->second.identity == m_Backend->Identity(voice);
}
int SessionAudio::Play(const std::string& key, int loops, int voice) {
    std::vector<int> expired;
    for (const auto& [v, data] : m_Voices) if (!Owns(v) || !m_Backend->Active(v)) expired.push_back(v);
    for (int v : expired) Finish(v);
    if (voice != -1 && m_Backend->Active(voice) && !Owns(voice)) throw std::runtime_error("Audio voice is owned outside this runtime session");
    if (voice != -1 && m_Voices.contains(voice)) Finish(voice);
    // Automatic channel playback never replaces a channel already playing.
    const int channel = m_Backend->Start(key, loops, voice);
    const int baseline = m_Backend->Volume(channel);
    m_Voices.emplace(channel, Voice{m_Backend->Identity(channel), baseline});
    const auto volume = channel == AudioBackend::Music ? m_MusicVolume : m_FxVolume;
    if (volume) m_Backend->Volume(channel, *volume);
    return channel;
}
void SessionAudio::Finish(int voice) {
    auto found = m_Voices.find(voice); if (found == m_Voices.end()) return;
    if (Owns(voice)) { m_Backend->Halt(voice); m_Backend->Volume(voice, found->second.oldVolume); }
    m_Voices.erase(found);
}
void SessionAudio::PauseMusic() {
    if (Owns(AudioBackend::Music)) { m_Backend->Pause(AudioBackend::Music); m_Voices.at(AudioBackend::Music).pausedBySession = false; }
}
void SessionAudio::ResumeMusic() { if (Owns(AudioBackend::Music)) m_Backend->Resume(AudioBackend::Music); }
void SessionAudio::MusicVolume(int volume) {
    m_MusicVolume = Percent(volume);
    if (Owns(AudioBackend::Music)) m_Backend->Volume(AudioBackend::Music, *m_MusicVolume);
}
int SessionAudio::PlayFx(const std::string& key, int loops, int channel) {
    if (channel < -1) throw std::invalid_argument("Invalid sound channel");
    return Play(key, loops, channel);
}
void SessionAudio::StopFx(int channel) {
    if (channel == -1) {
        std::vector<int> voices; for (const auto& [v, value] : m_Voices) if (v >= 0) voices.push_back(v);
        for (int voice : voices) Finish(voice);
    } else if (channel >= 0) Finish(channel);
}
void SessionAudio::FxVolume(int volume, int channel) {
    const int value = Percent(volume);
    if (channel == -1) {
        m_FxVolume = value;
        for (const auto& [voice, data] : m_Voices) if (voice >= 0 && Owns(voice)) m_Backend->Volume(voice, value);
    } else if (channel >= 0 && Owns(channel)) m_Backend->Volume(channel, value);
}
bool SessionAudio::FxPlaying(int channel) {
    if (channel >= 0) return Owns(channel) && m_Backend->Active(channel);
    for (const auto& [voice, data] : m_Voices) if (voice >= 0 && Owns(voice) && m_Backend->Active(voice)) return true;
    return false;
}
void SessionAudio::Pause() {
    for (auto& [voice, data] : m_Voices) {
        data.pausedBySession = Owns(voice) && m_Backend->Active(voice) && !m_Backend->Paused(voice);
        if (data.pausedBySession) m_Backend->Pause(voice);
    }
}
void SessionAudio::Resume() {
    for (auto& [voice, data] : m_Voices) {
        if (data.pausedBySession && Owns(voice) && m_Backend->Active(voice)) m_Backend->Resume(voice);
        data.pausedBySession = false;
    }
}
void SessionAudio::Stop() { while (!m_Voices.empty()) Finish(m_Voices.begin()->first); }
}
