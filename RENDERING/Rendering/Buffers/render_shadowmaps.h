#pragma once
#include <map>
#include <string>
#include "../Buffers/ShadowMap.h"

namespace ENGINE_RENDERING {
	struct RenderShadowMaps
	{
		std::map <std::string, std::shared_ptr<ENGINE_RENDERING::ShadowMap>> mapShadowmaps;
	};
}