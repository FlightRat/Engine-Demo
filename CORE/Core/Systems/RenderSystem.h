#pragma once
#include "../ECS/Registry.h"
#include "SSRViewState.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <Physics/RP3D_Wrappers.h>
#include <Rendering/Core/Camera3D.h>

namespace ENGINE_CORE::ECS {
	class TransformComponent;
	class Identification;
}

namespace ENGINE_RENDERING {
	class Framebuffer;
}

namespace ENGINE_CORE::Systems {
	class RenderSystem
	{
	private:
		GLuint m_DebugVAO, m_DebugVBO;
	public:
		RenderSystem();
		~RenderSystem() = default;

        void DeferredRenderPipeline(
			std::shared_ptr<ENGINE_RENDERING::Camera3D> camera,
			ENGINE_CORE::ECS::Registry& runtimeRegistry,
			std::shared_ptr<ENGINE_RENDERING::Framebuffer> intermediateGB,
			std::shared_ptr<ENGINE_RENDERING::Framebuffer> aoFB,
			std::shared_ptr<ENGINE_RENDERING::Framebuffer> aoBlurFB,
			std::shared_ptr<ENGINE_RENDERING::Framebuffer> hdrFB,
			SSRViewState& reflection,
			std::shared_ptr<ENGINE_RENDERING::Framebuffer> shadeOutputFB,
            std::shared_ptr<ENGINE_RENDERING::Framebuffer> fxaaOutputFB
		);
	private:
		void Prepare_Pass(std::shared_ptr<ENGINE_RENDERING::Camera3D> camera, ENGINE_CORE::ECS::Registry& runtimeRegistry);
		void Shadow_Pass(ENGINE_CORE::ECS::Registry& runtimeRegistry);
		void Geometry_Pass(std::shared_ptr<ENGINE_RENDERING::Camera3D> camera, ENGINE_CORE::ECS::Registry& runtimeRegistry, SSRViewState& reflection);
		void HBAO_Pass(std::shared_ptr<ENGINE_RENDERING::Camera3D> camera, ENGINE_CORE::ECS::Registry& runtimeRegistry, std::shared_ptr<ENGINE_RENDERING::Framebuffer> intermediateGB);
		void AOBlur_Pass(std::shared_ptr<ENGINE_RENDERING::Framebuffer> source, std::shared_ptr<ENGINE_RENDERING::Framebuffer> intermediateGB, const glm::vec2& direction);
		void Lighting_Pass(
			std::shared_ptr<ENGINE_RENDERING::Camera3D> camera, ENGINE_CORE::ECS::Registry& runtimeRegistry, 
			std::shared_ptr<ENGINE_RENDERING::Framebuffer> intermediateGB, std::shared_ptr<ENGINE_RENDERING::Framebuffer> aoFB);
		void SSRDepth_Pass(std::shared_ptr<ENGINE_RENDERING::Framebuffer> intermediateGB, ENGINE_RENDERING::DepthPyramid& depth);
		void SSR_Pass(std::shared_ptr<ENGINE_RENDERING::Camera3D> camera,
			std::shared_ptr<ENGINE_RENDERING::Framebuffer> intermediateGB, std::shared_ptr<ENGINE_RENDERING::Framebuffer> hdrFB, SSRViewState& reflection);
		void SSRTemporal_Pass(std::shared_ptr<ENGINE_RENDERING::Framebuffer> intermediateGB, SSRViewState& reflection);
		void SSRSpatial_Pass(std::shared_ptr<ENGINE_RENDERING::Framebuffer> intermediateGB, SSRViewState& reflection);
		void SSRSnapshot_Pass(std::shared_ptr<ENGINE_RENDERING::Framebuffer> intermediateGB, SSRViewState& reflection);
		void SSRComposite_Pass(std::shared_ptr<ENGINE_RENDERING::Camera3D> camera,
			std::shared_ptr<ENGINE_RENDERING::Framebuffer> intermediateGB, std::shared_ptr<ENGINE_RENDERING::Framebuffer> aoFB,
			std::shared_ptr<ENGINE_RENDERING::Framebuffer> hdrFB, std::shared_ptr<ENGINE_RENDERING::Framebuffer> ssrFB);
		void Postprocess_Pass(std::shared_ptr<ENGINE_RENDERING::Camera3D> camera, ENGINE_CORE::ECS::Registry& runtimeRegistry);
		void FXAA_Pass(const std::shared_ptr<ENGINE_RENDERING::Framebuffer>& source, const std::shared_ptr<ENGINE_RENDERING::Framebuffer>& output);

		glm::mat4 CalculateModelMatrix(
			const ENGINE_CORE::ECS::TransformComponent& transform,
			const ENGINE_CORE::ECS::Identification& id,
			ENGINE_CORE::ECS::Registry& runtimeRegistry);
	};
}
