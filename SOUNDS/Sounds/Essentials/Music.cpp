#include "Music.h"
ENGINE_SOUNDS::Music::Music(const SoundParams& params, MusicPtr music, std::shared_ptr<const std::vector<unsigned char>> source)
    : m_Params(params), m_Source(std::move(source)), m_pMusic(std::move(music)) {}
ENGINE_SOUNDS::Music::~Music() {
    if (ActiveMusic == m_pMusic.get()) { Mix_HaltMusic(); ActiveMusic = nullptr; }
}
