#pragma once
#include "../ECS/Registry.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <Physics/RP3D_Wrappers.h>
#include <Rendering/Core/Camera3D.h>
#include <Rendering/Buffers/Framebuffer.h>
#include <Rendering/Buffers/Gbuffer.h>

namespace ENGINE_CORE::ECS {
	class TransformComponent;
	class Identification;
}

namespace ENGINE_CORE::Systems {
	class RenderSystem
	{
	private:
		GLuint m_DebugVAO, m_DebugVBO;
	public:
		RenderSystem();
		~RenderSystem() = default;

		void ForwardRenderPipeline(
			std::shared_ptr<ENGINE_RENDERING::Camera3D> camera,
			ENGINE_CORE::ECS::Registry& runtimeRegistry,
			std::shared_ptr<ENGINE_RENDERING::Framebuffer> finalOutputFB
		);
		void DeferredRenderPipeline(
			std::shared_ptr<ENGINE_RENDERING::Camera3D> camera,
			ENGINE_CORE::ECS::Registry& runtimeRegistry,
			std::shared_ptr<ENGINE_RENDERING::Gbuffer> intermediateGB,
			std::shared_ptr<ENGINE_RENDERING::Framebuffer> finalOutputFB
		);
	private:
		void Prepare_Pass(std::shared_ptr<ENGINE_RENDERING::Camera3D> camera, ENGINE_CORE::ECS::Registry& runtimeRegistry);
		void Shadow_Pass(ENGINE_CORE::ECS::Registry& runtimeRegistry);

		void Forward_Pass(std::shared_ptr<ENGINE_RENDERING::Camera3D> camera, ENGINE_CORE::ECS::Registry& runtimeRegistry);

		void Geometry_Pass(std::shared_ptr<ENGINE_RENDERING::Camera3D> camera, ENGINE_CORE::ECS::Registry& runtimeRegistry);
		void Lighting_Pass(std::shared_ptr<ENGINE_RENDERING::Camera3D> camera, ENGINE_CORE::ECS::Registry& runtimeRegistry, std::shared_ptr<ENGINE_RENDERING::Gbuffer> intermediateGB);
		void Postprocess_Pass(std::shared_ptr<ENGINE_RENDERING::Camera3D> camera, ENGINE_CORE::ECS::Registry& runtimeRegistry);

		glm::mat4 CalculateModelMatrix(
			const ENGINE_CORE::ECS::TransformComponent& transform,
			const ENGINE_CORE::ECS::Identification& id,
			ENGINE_CORE::ECS::Registry& runtimeRegistry);
	};
}