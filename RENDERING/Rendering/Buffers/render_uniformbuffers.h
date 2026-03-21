#pragma once
#include <map>
#include <string>
#include "../Buffers/UniformBuffer.h"

namespace ENGINE_RENDERING{
	struct RenderUniformbuffers
	{
		std::map<std::string, std::shared_ptr<ENGINE_RENDERING::UniformBuffer>> mapUniformbuffers;
	};
}