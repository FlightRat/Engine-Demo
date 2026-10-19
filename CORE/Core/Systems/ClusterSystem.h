#pragma once
#include <map>
#include <memory>
#include <Rendering/Buffers/Framebuffer.h>
#include <Rendering/Buffers/ShaderStorageBuffer.h>
#include <Rendering/Core/Camera3D.h>
#include <Rendering/Essentials/LightCulling.h>
#include <Rendering/Essentials/RenderMetrics.h>

using ViewportKey = std::weak_ptr<ENGINE_RENDERING::Framebuffer>;

namespace ENGINE_CORE::Systems {

	struct ClusterHeader {
		GLuint count{ 0 }, overflow{ 0 }, validPixelCount{ 0 }, reserved{ 0 };
	};
	static_assert(sizeof(ClusterHeader) == 16);

	struct ClusterBuffers {
		int width{ 0 }, height{ 0 }, tileCountX{ 0 }, tileCountY{ 0 }, sliceCount{ 0 };
		float clusterNear{ 0.1f }, clusterFar{ 100.0f }, depthScale{ 1.0f };
		size_t count{ 0 };
		ENGINE_RENDERING::ShaderStorageBuffer headers{ 4 }, indices{ 5 };
		ENGINE_RENDERING::RenderMetrics metrics;
	};

	class ClusterSystem {
	private:
		std::map<ViewportKey, ClusterBuffers, std::owner_less<ViewportKey>> m_ClusterBuffers;
	public:
		ClusterBuffers& Prepare(const std::shared_ptr<ENGINE_RENDERING::Framebuffer>& gbuffer,
			const ENGINE_RENDERING::Camera3D& camera, ENGINE_RENDERING::LightingMode mode);
		void Update(const std::shared_ptr<ENGINE_RENDERING::Framebuffer>& gbuffer,
			const ENGINE_RENDERING::Camera3D& camera, ENGINE_RENDERING::LightingMode mode);
		void Bind(const std::shared_ptr<ENGINE_RENDERING::Framebuffer>& gbuffer);
		const auto& GetViews() const { return m_ClusterBuffers; }
		void StartCapture();
	};
}
