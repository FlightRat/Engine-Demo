#pragma once
#include "Rendering/Buffers/Gbuffer.h"
#include <map>

namespace ENGINE_EDITOR {
	enum class GbufferType
	{
		GAME, SCENE, NO_TYPE
	};

	struct Editorgbuffers
	{
		std::map<GbufferType, std::shared_ptr<ENGINE_RENDERING::Gbuffer>> mapGbuffers;
	};
}