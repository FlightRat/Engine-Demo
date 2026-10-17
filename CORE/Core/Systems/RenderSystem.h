#pragma once
#include "../ECS/Registry.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <Physics/RP3D_Wrappers.h>
#include <Rendering/Buffers/PointShadowArray.h>
#include <Rendering/Essentials/RenderMetrics.h>
#include <unordered_map>
#include <memory>
#include <string>
#include <array>
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
		ENGINE_RENDERING::PointShadowArray m_PointShadows;
		std::array<double, 3> m_CpuMs{};
		unsigned int m_CaptureGeneration{ 0 };
	public:
		struct LightViewData {
			std::string prefix;
			int width{ 0 }, height{ 0 };
			unsigned int captureGeneration{ 0 };
			glm::mat4 view{ 1.0f }, projection{ 1.0f };
			ENGINE_RENDERING::RenderMetrics metrics;
		};
	private:
		std::unordered_map<const ENGINE_RENDERING::Framebuffer*, std::unique_ptr<LightViewData>> m_LightViews;
		void LightCulling_Pass(std::shared_ptr<ENGINE_RENDERING::Camera3D> camera,
			std::shared_ptr<ENGINE_RENDERING::Framebuffer> gbuffer, LightViewData& data);
	public:
		bool useTiledLighting{ true };
		bool showLightVisualizations{ true };
		bool profilingEnabled{ false };
		bool collectLightStatistics{ false };
		std::string benchmarkLabel{ "manual" };
		void StartLightBenchmark();
		void ExportLightBenchmark(const std::string& path) const;
		const auto& GetLightViews() const { return m_LightViews; }
		RenderSystem();
		~RenderSystem() = default;

		void DeferredRenderPipeline(
			std::shared_ptr<ENGINE_RENDERING::Camera3D> camera,
			ENGINE_CORE::ECS::Registry& runtimeRegistry,
			std::shared_ptr<ENGINE_RENDERING::Framebuffer> intermediateGB,
			std::shared_ptr<ENGINE_RENDERING::Framebuffer> aoFB,
			std::shared_ptr<ENGINE_RENDERING::Framebuffer> aoBlurFB,
			std::shared_ptr<ENGINE_RENDERING::Framebuffer> shadeOutputFB,
			std::shared_ptr<ENGINE_RENDERING::Framebuffer> fxaaOutputFB
		);
	private:
		void Prepare_Pass(std::shared_ptr<ENGINE_RENDERING::Camera3D> camera, ENGINE_CORE::ECS::Registry& runtimeRegistry);
		void Shadow_Pass(ENGINE_CORE::ECS::Registry& runtimeRegistry);
		void Geometry_Pass(std::shared_ptr<ENGINE_RENDERING::Camera3D> camera, ENGINE_CORE::ECS::Registry& runtimeRegistry);
		void HBAO_Pass(std::shared_ptr<ENGINE_RENDERING::Camera3D> camera, ENGINE_CORE::ECS::Registry& runtimeRegistry, std::shared_ptr<ENGINE_RENDERING::Framebuffer> intermediateGB);
		void AOBlur_Pass(std::shared_ptr<ENGINE_RENDERING::Framebuffer> source, std::shared_ptr<ENGINE_RENDERING::Framebuffer> intermediateGB, const glm::vec2& direction);
		void Lighting_Pass(
			std::shared_ptr<ENGINE_RENDERING::Camera3D> camera, ENGINE_CORE::ECS::Registry& runtimeRegistry, 
			std::shared_ptr<ENGINE_RENDERING::Framebuffer> intermediateGB, std::shared_ptr<ENGINE_RENDERING::Framebuffer> aoFB);
		void Postprocess_Pass(std::shared_ptr<ENGINE_RENDERING::Camera3D> camera, ENGINE_CORE::ECS::Registry& runtimeRegistry);
		void FXAA_Pass(const std::shared_ptr<ENGINE_RENDERING::Framebuffer>& source, const std::shared_ptr<ENGINE_RENDERING::Framebuffer>& output);

		glm::mat4 CalculateModelMatrix(
			const ENGINE_CORE::ECS::TransformComponent& transform,
			const ENGINE_CORE::ECS::Identification& id,
			ENGINE_CORE::ECS::Registry& runtimeRegistry);
	};
}