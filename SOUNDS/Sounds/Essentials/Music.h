#pragma once
#include<Utilities/SDL_Wrappers.h>
#include"SoundParams.h"
#include<vector>

namespace ENGINE_SOUNDS {
	class Music
	{
	private:
		// Music consist of some params and a ptr of music data
		SoundParams m_Params{};
		std::shared_ptr<const std::vector<unsigned char>> m_Source;
        MusicPtr m_pMusic{ nullptr };
	public:
		Music(const SoundParams& params, MusicPtr pMusic, std::shared_ptr<const std::vector<unsigned char>> source = {});
		~Music();
        inline static Mix_Music* ActiveMusic = nullptr;

		// Getters
		inline const std::string& GetName() const { return m_Params.name; }
		inline const std::string& GetFileName() const { return m_Params.filename; }
		inline const std::string& GetDescription() const { return m_Params.description; }
		inline const double GetDuration() const { return m_Params.duration; }
		inline Mix_Music* GetMusicPtr() const { if (!m_pMusic) return nullptr; return m_pMusic.get(); }
	};
}