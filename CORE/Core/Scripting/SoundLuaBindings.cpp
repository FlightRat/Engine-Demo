#include "SoundLuaBindings.h"
#include "../ECS/Registry.h"
#include "../Resources/AssetManager.h"
#include <Sounds/MusicPlayer/MusicPlayer.h>
#include <Sounds/SoundFxPlayer/SoundFxPlayer.h>
#include <Logger/Logger.h>

using namespace SOUNDS;
using namespace RESOURCES;

void CORE::Scripting::SoundBindings::CreateSoundBindings(sol::state& lua, CORE::ECS::Registry& registry)
{
	// get music player
	auto& musicPlayer = registry.GetContext<std::shared_ptr<MusicPlayer>>();
	if (!musicPlayer)
	{
		ENGINE_ERROR("Failed to bind the Music Player to Lua -- Not in the registry!");
		return;
	}

	// get soundfx player
	auto& soundFxPlayer = registry.GetContext<std::shared_ptr<SoundFxPlayer>>();
	if (!soundFxPlayer)
	{
		ENGINE_ERROR("Failed to bind the SoundFx Player to Lua -- Not in the registry!");
		return;
	}

	// get asset manager
	auto& assetManager = registry.GetContext<std::shared_ptr<AssetManager>>();
	if (!assetManager)
	{
		ENGINE_ERROR("Failed to bind the Music Player to Lua -- AssetManager does not exists in the registry!");
		return;
	}

	// register "Music" into lua
	lua.new_usertype<MusicPlayer>(
		"Music",
		sol::no_constructor,
		"play", sol::overload(
			[&](const std::string& musicName, int loops) {
				auto music = assetManager->GetMusic(musicName);
				if (!music)
				{
					ENGINE_ERROR("Failed to get music [{}] - From the asset manager!", musicName);
					return;
				}
				musicPlayer->Play(*music, loops);
			},
			[&](const std::string& musicName) {
				auto music = assetManager->GetMusic(musicName);
				if (!music)
				{
					ENGINE_ERROR("Failed to get music [{}] - From the asset manager!", musicName);
					return;
				}
				musicPlayer->Play(*music, -1);
			}
		),
		"stop", [&]() {musicPlayer->Stop(); },
		"pause", [&]() {musicPlayer->Pause(); },
		"resume", [&]() {musicPlayer->Resume(); },
		"set_volume", [&](int volume) {musicPlayer->SetVolume(volume); },
		"is_playing", [&]() {return musicPlayer->IsPlaying(); }
	);

	// register "SoundFx" into lua
	lua.new_usertype<SoundFxPlayer>(
		"SoundFx",
		sol::no_constructor,
		"play", sol::overload(
			[&](const std::string& soundFxName) {
				auto soundFx = assetManager->GetSoundFx(soundFxName);
				if (!soundFx)
				{
					ENGINE_ERROR("Failed to get soundFx [{}] - From the asset manager!", soundFxName);
					return;
				}
				soundFxPlayer->Play(*soundFx, 0, -1);
			},
			[&](const std::string& soundFxName, int loops, int channel) {
				auto soundFx = assetManager->GetSoundFx(soundFxName);
				if (!soundFx)
				{
					ENGINE_ERROR("Failed to get soundFx [{}] - From the asset manager!", soundFxName);
					return;
				}
				soundFxPlayer->Play(*soundFx, loops, channel);
			}
		),
		"stop", [&](int channel) {soundFxPlayer->Stop(channel); },
		"set_volume", [&](int volume, int channel) {soundFxPlayer->SetVolume(volume, channel); },
		"is_playing", [&](int channel) {return soundFxPlayer->IsPlaying(channel); }
	);
}
