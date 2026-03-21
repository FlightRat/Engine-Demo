#pragma once
#include <map>
#include "../Buffers/ShadowMap.h"

namespace ENGINE_RENDERING {
	enum class ShadowmapType
	{
		DIRLIGHT, POINTLIGHT, NO_TYPE
	};

	struct RenderShadowMaps
	{
		std::map < ShadowmapType, std::shared_ptr<ENGINE_RENDERING::ShadowMap>> mapShadowmaps;
	};
}