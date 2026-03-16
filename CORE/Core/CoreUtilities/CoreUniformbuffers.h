#pragma once
#include <map>
#include <string>
#include "Rendering/Buffers/UniformBuffer.h"

namespace ENGINE_CORE {
	struct CoreUniformbuffers
	{
		std::map<std::string, std::shared_ptr<ENGINE_RENDERING::UniformBuffer>> mapUniformbuffers;
	};
}