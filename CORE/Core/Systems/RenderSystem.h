#pragma once
#include"../ECS/Registry.h"
#include<glad/glad.h>
#include<glm/glm.hpp>
#include <Physics/RP3D_Wrappers.h>

namespace ENGINE_CORE::Systems {
	class RenderSystem
	{
	private:
		ENGINE_CORE::ECS::Registry& m_Registry;
	public:
		RenderSystem(ENGINE_CORE::ECS::Registry& registry);
		~RenderSystem() = default;
		void Render();

		GLuint m_DebugVAO, m_DebugVBO;
	};
}