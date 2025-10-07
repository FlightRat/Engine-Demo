#include "SoundFxPlayer.h"
#include "../Essentials/SoundFx.h"
#include <Logger/Logger.h>

namespace SOUNDS {
	void SoundFxPlayer::Play(SoundFx& soundFx, int loops, int channel)
	{
		if (!soundFx.GetSoundFxPtr())
		{
			ENGINE_ERROR("Failed to play soundFx [{}] on channel [{}] - Mix Sound was Null!", soundFx.GetName(), channel);
			return;
		}

		if (Mix_PlayChannel(channel, soundFx.GetSoundFxPtr(), loops) == -1)
		{
			std::string error{ Mix_GetError() };
			ENGINE_ERROR("Failed to play soundFx [{}] on channel [{}] -- Mix Error: {}", soundFx.GetName(), channel, error);
		}
	}

	void SoundFxPlayer::Stop(int channel)
	{
		if (Mix_HaltChannel(channel) == -1)
		{
			std::string error{ Mix_GetError() };
			ENGINE_ERROR("Failed to halt soundfx for channel [{}]", channel == -1 ? "all channels" : std::to_string(channel));
		}
	}

	void SoundFxPlayer::SetVolume(int volume, int channel)
	{
		if (volume < 0 || volume > 100)
		{
			ENGINE_ERROR("Failed to set the volume with [{}], it must between 0 and 100!", volume);
			return;
		}
		// scale the volume to 0~128
		int volume_scaled = static_cast<int>((volume / 100.f) * 128);
		Mix_Volume(channel, volume_scaled);
	}

	bool SoundFxPlayer::IsPlaying(int channel)
	{
		return Mix_Playing(channel);
	}
}