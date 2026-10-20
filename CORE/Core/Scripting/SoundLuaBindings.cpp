#include "../CoreUtilities/GameplayContext.h"
#include "SoundLuaBindings.h"
#include "../ECS/Registry.h"
#include "../Resources/AssetManager.h"
#include <../CORE/Core/ECS/MainRegistry.h>
#include <Sounds/MusicPlayer/MusicPlayer.h>
#include <Sounds/SoundFxPlayer/SoundFxPlayer.h>
#include <Logger/Logger.h>

using namespace ENGINE_SOUNDS;
using namespace ENGINE_CORE::RESOURCES;

void ENGINE_CORE::Scripting::SoundBindings::CreateLuaSoundBind(sol::state& lua, ENGINE_CORE::ECS::Registry& registry)
{
    if (auto* context = registry.GetRegistry().ctx().find<std::shared_ptr<ENGINE_CORE::GameplayContext>>()) {
        auto audio = (*context)->audio;
        if (!audio) throw std::runtime_error("Missing runtime audio service");
        lua.new_usertype<MusicPlayer>("Music", sol::no_constructor,
            "play", sol::overload([audio](const std::string& key, int loops) { audio->PlayMusic(key, loops); }, [audio](const std::string& key) { audio->PlayMusic(key, -1); }),
            "stop", [audio] { audio->StopMusic(); }, "pause", [audio] { audio->PauseMusic(); }, "resume", [audio] { audio->ResumeMusic(); },
            "set_volume", [audio](int v) { audio->MusicVolume(v); }, "is_playing", [audio] { return audio->MusicPlaying(); });
        lua.new_usertype<SoundFxPlayer>("SoundFx", sol::no_constructor,
            "play", sol::overload([audio](const std::string& key) { return audio->PlayFx(key, 0, -1); }, [audio](const std::string& key, int loops, int channel) { return audio->PlayFx(key, loops, channel); }),
            "stop", [audio](int channel) { audio->StopFx(channel); }, "set_volume", [audio](int v, int channel) { audio->FxVolume(v, channel); },
            "is_playing", [audio](int channel) { return audio->FxPlaying(channel); });
        return;
    }

	auto& mainRegistry = MAIN_REGISTRY();
	auto& musicPlayer = mainRegistry.GetMusicPlayer();
	auto& soundFxPlayer = mainRegistry.GetSoundFxPlayer();
	auto& assetManager = mainRegistry.GetAssetManager();

	// register "Music" into lua
	lua.new_usertype<MusicPlayer>(
		"Music",
		sol::no_constructor,
		"play", sol::overload(
			[&](const std::string& musicName, int loops) {
				auto music = assetManager.GetMusic(musicName);
				if (!music)
				{
					ENGINE_ERROR("Failed to get music [{}] - From the asset manager!", musicName);
					return;
				}
				musicPlayer.Play(*music, loops);
			},
			[&](const std::string& musicName) {
				auto music = assetManager.GetMusic(musicName);
				if (!music)
				{
					ENGINE_ERROR("Failed to get music [{}] - From the asset manager!", musicName);
					return;
				}
				musicPlayer.Play(*music, -1);
			}
		),
		"stop", [&]() {musicPlayer.Stop(); },
		"pause", [&]() {musicPlayer.Pause(); },
		"resume", [&]() {musicPlayer.Resume(); },
		"set_volume", [&](int volume) {musicPlayer.SetVolume(volume); },
		"is_playing", [&]() {return musicPlayer.IsPlaying(); }
	);

	// register "SoundFx" into lua
	lua.new_usertype<SoundFxPlayer>(
		"SoundFx",
		sol::no_constructor,
		"play", sol::overload(
			[&](const std::string& soundFxName) {
				auto soundFx = assetManager.GetSoundFx(soundFxName);
				if (!soundFx)
				{
					ENGINE_ERROR("Failed to get soundFx [{}] - From the asset manager!", soundFxName);
					return;
				}
				soundFxPlayer.Play(*soundFx, 0, -1);
			},
			[&](const std::string& soundFxName, int loops, int channel) {
				auto soundFx = assetManager.GetSoundFx(soundFxName);
				if (!soundFx)
				{
					ENGINE_ERROR("Failed to get soundFx [{}] - From the asset manager!", soundFxName);
					return;
				}
				soundFxPlayer.Play(*soundFx, loops, channel);
			}
		),
		"stop", [&](int channel) {soundFxPlayer.Stop(channel); },
		"set_volume", [&](int volume, int channel) {soundFxPlayer.SetVolume(volume, channel); },
		"is_playing", [&](int channel) {return soundFxPlayer.IsPlaying(channel); }
	);
}
