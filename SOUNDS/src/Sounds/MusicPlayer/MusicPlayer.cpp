#include "MusicPlayer.h"
#include "../Essentials/Music.h"
#include <Logger/Logger.h>

namespace SOUNDS {
	MusicPlayer::MusicPlayer()
	{	
		// create the audio device
		if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096) == -1)
		{
			std::string error{ Mix_GetError() };
			ENGINE_ERROR("Unable to open SDL Music Mixer - {}", error);
			return;
		}
		ENGINE_LOG("CHANNELS ALLOCATED [{}]", Mix_AllocateChannels(16));
	}

	MusicPlayer::~MusicPlayer()
	{
		Mix_HaltMusic();
		Mix_Quit();
		ENGINE_LOG("Music Player closed!");
	}

	void MusicPlayer::Play(Music& music, int loops)
	{
		if (!music.GetMusicPtr())
		{
			ENGINE_ERROR("Falied to play music [{}] - Mix Music was Null!", music.GetName());
			return;
		}

		if (Mix_PlayMusic(music.GetMusicPtr(), loops) != 0)
		{
			std::string error{ Mix_GetError() };
			ENGINE_ERROR("Failed to play music [{}] Mix Error - {}", music.GetName(), error);
		}
	}

	void MusicPlayer::Pause()
	{
		if (!Mix_PausedMusic())
		{
			Mix_PauseMusic();
		}
	}

	void MusicPlayer::Resume()
	{
		Mix_ResumeMusic();
	}

	void MusicPlayer::Stop()
	{
		Mix_HaltMusic();
	}

	void MusicPlayer::SetVolume(int volume)
	{
		Mix_VolumeMusic(volume);
	}

	bool MusicPlayer::IsPlaying()
	{
		return Mix_PlayingMusic();
	}
}


