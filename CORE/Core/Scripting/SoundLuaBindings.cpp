#include "SoundLuaBindings.h"
#include "../ECS/Registry.h"
#include "../Resources/AssetManager.h"
#include <Sounds/MusicPlayer/MusicPlayer.h>
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
}
