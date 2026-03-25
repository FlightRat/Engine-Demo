#pragma once
#include "Rendering/Buffers/Framebuffer.h"
#include <map>

namespace ENGINE_EDITOR {
	enum class FramebufferType
	{
		GAME, SCENE, NO_TYPE
	};

	struct Editorframebuffers
	{
		std::map<FramebufferType, std::shared_ptr<ENGINE_RENDERING::Framebuffer>> mapFramebuffers;
	};
}