#pragma once

namespace ENGINE_SOUNDS {
	class SoundFxPlayer
	{
	public:
		SoundFxPlayer() = default;		// the construct for audio device is done in MusicPlayer, so omit here
		~SoundFxPlayer() = default;

		void Play(class SoundFx& soundFx, int loops, int channel);
		void Stop(int channel);
		void SetVolume(int volume, int channel);
		bool IsPlaying(int channel);
	};
}