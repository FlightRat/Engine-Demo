#include "ClusterSystem.h"
#include "LightSystem.h"
#include "../ECS/MainRegistry.h"
#include "../Resources/AssetManager.h"
#include <cmath>

using namespace ENGINE_RENDERING;

namespace ENGINE_CORE::Systems {

	ClusterBuffers& ClusterSystem::Prepare(const std::shared_ptr<Framebuffer>& gbuffer,
		const Camera3D& camera, LightingMode mode)
	{
		std::erase_if(m_ClusterBuffers, [](const auto& entry) { return entry.first.expired(); });
		auto& buffers = m_ClusterBuffers[ViewportKey(gbuffer)];
		buffers.width = gbuffer->Width();
		buffers.height = gbuffer->Height();
		buffers.tileCountX = (buffers.width + LIGHT_TILE_SIZE - 1) / LIGHT_TILE_SIZE;
		buffers.tileCountY = (buffers.height + LIGHT_TILE_SIZE - 1) / LIGHT_TILE_SIZE;
		buffers.sliceCount = mode == LightingMode::Clustered ? CLUSTER_DEPTH_SLICES : 1;
		buffers.clusterNear = camera.GetNearPlane();
		buffers.clusterFar = camera.GetFarPlane();
		buffers.depthScale = CLUSTER_DEPTH_SLICES / std::log(buffers.clusterFar / buffers.clusterNear);
		buffers.count = mode == LightingMode::Full ? 0 :
			static_cast<size_t>(buffers.tileCountX) * buffers.tileCountY * buffers.sliceCount;
		if (buffers.count != 0) {
			buffers.headers.Reserve(buffers.count * sizeof(ClusterHeader));
			buffers.indices.Reserve(buffers.count * MAX_CLUSTER_LIGHTS * sizeof(GLuint));
		}
		return buffers;
	}

	void ClusterSystem::Bind(const std::shared_ptr<Framebuffer>& gbuffer)
	{
		const auto& buffers = m_ClusterBuffers.at(ViewportKey(gbuffer));
		buffers.headers.Bind(4);
		buffers.indices.Bind(5);
	}

	void ClusterSystem::Update(const std::shared_ptr<Framebuffer>& gbuffer,
		const Camera3D& camera, LightingMode mode)
	{
		auto& buffers = Prepare(gbuffer, camera, mode);
		if (mode == LightingMode::Full) return;
		Bind(gbuffer);
		auto& registry = MAIN_REGISTRY();
		auto shader = registry.GetAssetManager().GetShader(mode == LightingMode::Clustered ? "cluster_Culling" : "light_Culling");
		auto lights = registry.GetContext<std::shared_ptr<LightSystem>>();
		shader->Enable();
		shader->SetUniformInt("pointLightCount", lights->GetActivatedPointLights());
		if (mode == LightingMode::Clustered) {
			shader->SetUniformFloat("clusterNear", buffers.clusterNear);
			shader->SetUniformFloat("clusterFar", buffers.clusterFar);
			shader->SetUniformFloat("clusterDepthScale", buffers.depthScale);
		}
		glBindTextureUnit(0, gbuffer->GetTextureID(0));
		glDispatchCompute(buffers.tileCountX, buffers.tileCountY, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}

	void ClusterSystem::StartCapture()
	{
		for (auto& [key, buffers] : m_ClusterBuffers) buffers.metrics.Start();
	}
}
