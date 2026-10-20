#include "../src/editor/scene/SessionAudio.h"
#include <Core/ECS/MainRegistry.h>
#include <Core/Resources/AssetManager.h>
#include <SDL.h>
#include <SDL_mixer.h>
#include <iostream>
#include <stdexcept>
using namespace ENGINE_EDITOR;
static void Check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
void RuntimeAudioBackendTests() {
    SDL_SetMainReady();
    SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
    Check(SDL_InitSubSystem(SDL_INIT_AUDIO) == 0, SDL_GetError());
    Check(Mix_OpenAudio(22050, AUDIO_S16SYS, 2, 512) == 0, Mix_GetError());
    struct Cleanup {
        ~Cleanup() { MAIN_REGISTRY().CleanUp(); Mix_CloseAudio(); SDL_QuitSubSystem(SDL_INIT_AUDIO); }
    } cleanup;
    MAIN_REGISTRY().Initialize();
    auto& assets = MAIN_REGISTRY().GetAssetManager();
    const std::string source = "tools/engine_mcp/tests/fixtures/imports/tone.wav";
    Check(assets.AddSoundFx("runtime-fx", source), "Cannot load real SDL effect");
    Check(assets.AddMusic("runtime-music", source), "Cannot load real SDL music");
    Mix_AllocateChannels(8);
    const int externalBaseline = Mix_Volume(7, -1), musicBaseline = Mix_VolumeMusic(-1);
    Check(Mix_PlayChannel(7, assets.GetSoundFx("runtime-fx")->GetSoundFxPtr(), -1) == 7, "Cannot start external voice");
    Mix_Volume(7, 20);
    {
        SessionAudio audio(CreateSDLAudioBackend());
        audio.MusicVolume(30); audio.PlayMusic("runtime-music", -1);
        const int channel = audio.PlayFx("runtime-fx", -1, -1);
        Check(channel != 7 && Mix_Playing(channel), "Runtime effect did not play");
        SDL_Delay(3); // SDL_mixer uses a nonzero SDL tick as its paused-channel marker.
        audio.Pause();
        Check(Mix_PausedMusic() && Mix_Paused(channel) && !Mix_Paused(7),
            ("Real backend pause ownership: music=" + std::to_string(Mix_PausedMusic()) + " effect=" + std::to_string(Mix_Paused(channel)) + " external=" + std::to_string(Mix_Paused(7))).c_str());
        audio.Resume(); Check(!Mix_PausedMusic() && !Mix_Paused(channel), "Real backend resume");
        audio.PauseMusic(); audio.Pause(); audio.Resume(); Check(Mix_PausedMusic(), "Real backend restarted script-paused music");
        audio.FxVolume(50, -1); Check(Mix_Volume(7, -1) == 20, "Real backend changed external volume");
        audio.Stop(); Check(!Mix_PlayingMusic() && !Mix_Playing(channel) && Mix_Playing(7), "Real backend stop ownership");
        Check(Mix_VolumeMusic(-1) == musicBaseline, "Real backend music volume was not restored");
    }
    Mix_HaltChannel(7); Mix_Volume(7, externalBaseline);
    std::cout << "SDL dummy audio backend ownership/pause/volume tests passed (no audible output)\n";
}
