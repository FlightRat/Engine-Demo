#pragma once
#include"SoundParams.h"
#include<Utilities/SDL_Wrappers.h>

namespace ENGINE_SOUNDS {
	class SoundFx
	{
	private:
		// SoundFx consist of some params and a ptr of soundfx data
		SoundParams m_Params{};
		SoundFxPtr m_pSoundFx{ nullptr };
	public:
		SoundFx(const SoundParams& params, SoundFxPtr pSoundFx);
		~SoundFx() = default;

		// Getters
		inline const std::string& GetName() const { return m_Params.name; }
		inline const std::string& GetFileName() const { return m_Params.filename; }
		inline const std::string& GetDescription() const { return m_Params.description; }
		inline const double GetDuration() const { return m_Params.duration; }
		inline Mix_Chunk* GetSoundFxPtr() const { if (!m_pSoundFx) return nullptr; return m_pSoundFx.get(); }
	};
}