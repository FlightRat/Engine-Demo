#pragma once
#include "../ECS/Registry.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <Physics/RP3D_Wrappers.h>
#include <Rendering/Core/Camera3D.h>
#include <Rendering/Buffers/Framebuffer.h>

namespace ENGINE_CORE::Systems {
	class RenderSystem
	{
	private:
		GLuint m_DebugVAO, m_DebugVBO;
	public:
		RenderSystem();
		~RenderSystem() = default;

		void ExecuteRenderPipeline(
			std::shared_ptr<ENGINE_RENDERING::Camera3D> camera,
			ENGINE_CORE::ECS::Registry& runtimeRegistry,
			std::shared_ptr<ENGINE_RENDERING::Framebuffer> finalOutputFB
		);
	private:
		void Param_Pass(std::shared_ptr<ENGINE_RENDERING::Camera3D> camera, ENGINE_CORE::ECS::Registry& runtimeRegistry);
		void Shadow_Pass(ENGINE_CORE::ECS::Registry& runtimeRegistry);
		void Forward_Pass(std::shared_ptr<ENGINE_RENDERING::Camera3D> camera, ENGINE_CORE::ECS::Registry& runtimeRegistry);
	};
}