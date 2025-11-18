#pragma once
#include"../ECS/Registry.h"
#include<glad/glad.h>
#include<glm/glm.hpp>
#include <Physics/RP3D_Wrappers.h>
#include <Rendering/Core/Camera3D.h>

namespace ENGINE_CORE::Systems {
	class RenderSystem
	{
	public:
		RenderSystem();
		~RenderSystem() = default;
		void Render(std::shared_ptr<ENGINE_RENDERING::Camera3D> camera, ENGINE_CORE::ECS::Registry& runtimeRegistry);

		GLuint m_DebugVAO, m_DebugVBO;
	};
}