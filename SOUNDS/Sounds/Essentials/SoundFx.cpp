#include "SoundFx.h"

SOUNDS::SoundFx::SoundFx(const SoundParams& params, SoundFxPtr pSoundFx) :m_Params{ params }, m_pSoundFx{ std::move(pSoundFx) }
{

}
