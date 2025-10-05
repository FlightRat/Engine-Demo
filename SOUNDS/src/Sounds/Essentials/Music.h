#pragma once
#include<Utilities/SDL_Wrappers.h>
#include"SoundParams.h"

namespace SOUNDS {
	class Music
	{
	private:
		SoundParams m_Params{};
		MusicPtr m_pMusic{ nullptr };
	public:
		Music(const SoundParams& params, MusicPtr pMusic);
		~Music() = default;

		inline const std::string& GetName() const { return m_Params.name; }
		inline const std::string& GetFileName() const { return m_Params.filename; }
		inline const std::string& GetDescription() const { return m_Params.description; }
		inline const double GetDuration() const { return m_Params.duration; }

		inline Mix_Music* GetMusicPtr() const { if (!m_pMusic) return nullptr; return m_pMusic.get(); }
	};
}