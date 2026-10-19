#include "RenderSystem.h"
#include <utility>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>
#include <Rendering/Core/Camera3D.h>
#include <Rendering/Essentials/Shader.h>
#include <Rendering/Essentials/Lights.h>
#include <Rendering/Essentials/TextureCommon.h>
#include <Logger/Logger.h>
#include "../CORE/Core/ECS/MainRegistry.h"
#include "../CORE/Core/Systems/ClusterSystem.h"
#include "../CORE/Core/Systems/LightSystem.h"
#include "../ECS/Entity.h"
#include "../Buffers/BufferManager.h"
#include "../Resources/AssetManager.h"
#include "../ECS/Components/TransformComponent.h"
#include "../ECS/Components/Identification.h"
#include "../ECS/Components/MeshFilter.h"
#include "../ECS/Components/MeshRender.h"
#include "../ECS/Components/LightComponent.h"
#include "../CoreUtilities/CoreEngineData.h"
#include "../Core/Core/Inputs/InputManager.h"

using namespace ENGINE_CORE::ECS;
using namespace ENGINE_RENDERING;
using namespace ENGINE_CORE::RESOURCES;

namespace ENGINE_CORE::Systems {
	RenderSystem::RenderSystem()
	{
		glGenVertexArrays(1, &m_DebugVAO);
		glGenBuffers(1, &m_DebugVBO);
	}

	glm::mat4 RenderSystem::CalculateModelMatrix(const TransformComponent& transform, const Identification& id, ENGINE_CORE::ECS::Registry& runtimeRegistry)
	{
		glm::mat4 model = glm::translate(glm::mat4(1.0f), transform.position);
		model *= glm::toMat4(transform.rotation_quat);
		model = glm::scale(model, transform.scale);

		if (id.parent_id != -1) {
			auto parent_entity = static_cast<entt::entity>(id.parent_id);
			if (runtimeRegistry.GetRegistry().valid(parent_entity)) {
				auto& p_transform = runtimeRegistry.GetRegistry().get<TransformComponent>(parent_entity);
				glm::mat4 parentModel = glm::translate(glm::mat4(1.0f), p_transform.position);
				parentModel *= glm::toMat4(p_transform.rotation_quat);
				parentModel = glm::scale(parentModel, p_transform.scale);
				model = parentModel * model;
			}
		}
		return model;
	}

	void RenderSystem::DeferredRenderPipeline(
		std::shared_ptr<ENGINE_RENDERING::Camera3D> camera, ENGINE_CORE::ECS::Registry& runtimeRegistry,
		std::shared_ptr<ENGINE_RENDERING::Framebuffer> intermediateGB,
		std::shared_ptr<ENGINE_RENDERING::Framebuffer> aoFB,
		std::shared_ptr<ENGINE_RENDERING::Framebuffer> aoBlurFB,
		std::shared_ptr<ENGINE_RENDERING::Framebuffer> shadeOutputFB,
        std::shared_ptr<ENGINE_RENDERING::Framebuffer> fxaaOutputFB)
    {
        auto submitStart = std::chrono::steady_clock::now();
        Prepare_Pass(camera, runtimeRegistry);
        auto clusterSystem = MAIN_REGISTRY().GetContext<std::shared_ptr<ClusterSystem>>();
        auto& view = clusterSystem->Prepare(intermediateGB, *camera, lightingMode);
        if (profilingEnabled || collectLightStatistics) {
            std::ostringstream configuration;
            configuration << std::setprecision(9);
            configuration << '"' << reinterpret_cast<const char*>(glGetString(GL_RENDERER)) << "\",";
#ifdef _DEBUG
            configuration << "Debug,";
#else
            configuration << "Release,";
#endif
            configuration << intermediateGB.get() << ',' << view.width << ',' << view.height << ',' << SDL_GL_GetSwapInterval() << ','
                << static_cast<int>(lightingMode) << ',' << collectLightStatistics << ',' << pointShadowsEnabled << ',' << showLightVisualizations << ',';
            std::string label = benchmarkLabel;
            for (size_t i = 0; (i = label.find('"', i)) != std::string::npos; i += 2) label.insert(i, 1, '"');
            configuration << '"' << label << "\"," << MAIN_REGISTRY().GetContext<std::shared_ptr<LightSystem>>()->GetActivatedPointLights()
                << ",5,2048,100,16," << view.sliceCount << ",128," << view.count << ',' << view.headers.Capacity() << ',' << view.indices.Capacity()
                << ',' << camera->GetNearPlane() << ',' << camera->GetFarPlane();
            for (const auto& matrix : {camera->GetViewMatrix(), camera->GetProjectionMatrix()}) {
                configuration << ",\"";
                for (int i = 0; i < 16; ++i) configuration << glm::value_ptr(matrix)[i] << ';';
                configuration << '"';
            }
            configuration << ",120,300";
            view.metrics.Configure(configuration.str());
        }
        if (profilingEnabled) view.metrics.BeginFrame(lightingMode != LightingMode::Full);

		//glCullFace(GL_FRONT);		// 解决acne，但导致漏光
		if (profilingEnabled) view.metrics.BeginPass(3);
		Shadow_Pass(runtimeRegistry);
		if (profilingEnabled) view.metrics.EndPass(3);
		//glCullFace(GL_BACK);

		// 几何pass
		intermediateGB->Bind();
		glDisable(GL_BLEND);					// 关闭混合
		glEnable(GL_STENCIL_TEST);				// 开启模板测试
		// glCullFace(GL_BACK);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);	// 保证 Position 为 0
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glViewport(0, 0, intermediateGB->Width(), intermediateGB->Height());
		if (profilingEnabled) view.metrics.BeginPass(0);
		Geometry_Pass(camera, runtimeRegistry);
		if (profilingEnabled) view.metrics.EndPass(0);
		intermediateGB->Unbind();

		if (profilingEnabled) view.metrics.BeginPass(1);
		Tiling_Pass(intermediateGB, *camera);
		if (profilingEnabled) view.metrics.EndPass(1);
		if (collectLightStatistics && lightingMode != LightingMode::Full)
			view.metrics.SampleCandidates(view.headers.GetID(), view.count,
				MAIN_REGISTRY().GetContext<std::shared_ptr<LightSystem>>()->GetActivatedPointLights());

		// HBAO
		glDisable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_STENCIL_TEST);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		aoFB->Bind();
		glClear(GL_COLOR_BUFFER_BIT);
		glViewport(0, 0, aoFB->Width(), aoFB->Height());
		HBAO_Pass(camera, runtimeRegistry, intermediateGB);
		aoFB->Unbind();

		// Cross-bilateral blur: A -> B -> A
		// 第一轮模糊
		aoBlurFB->Bind();
		glClear(GL_COLOR_BUFFER_BIT);
		glViewport(0, 0, aoBlurFB->Width(), aoBlurFB->Height());
		AOBlur_Pass(aoFB, intermediateGB, glm::vec2(1.0f, 0.0f));
		aoBlurFB->Unbind();
		// 第二轮模糊
		aoFB->Bind();
		glClear(GL_COLOR_BUFFER_BIT);
		glViewport(0, 0, aoFB->Width(), aoFB->Height());
		AOBlur_Pass(aoBlurFB, intermediateGB, glm::vec2(0.0f, 1.0f));
		aoFB->Unbind();
		glEnable(GL_DEPTH_TEST);

		// 光照pass
		shadeOutputFB->Bind();
		glDisable(GL_STENCIL_TEST);				// 光照计算不需要模板
		glViewport(0, 0, shadeOutputFB->Width(), shadeOutputFB->Height());
		glClearColor(0.f, 0.f, 0.f, 1.f);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		if (profilingEnabled) view.metrics.BeginPass(2);
		Lighting_Pass(camera, runtimeRegistry, intermediateGB, aoFB);
		if (profilingEnabled) view.metrics.EndPass(2);
		shadeOutputFB->Unbind();

		// 后处理pass（点光源+模板测试+天空盒）
		glBindFramebuffer(GL_READ_FRAMEBUFFER, intermediateGB->GetFboID());
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, shadeOutputFB->GetFboID());
		glBlitFramebuffer(0, 0, intermediateGB->Width(), intermediateGB->Height(),
			0, 0, shadeOutputFB->Width(), shadeOutputFB->Height(),
			GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		shadeOutputFB->Bind();
		Postprocess_Pass(camera, runtimeRegistry);
		shadeOutputFB->Unbind();
		// FXAA
		FXAA_Pass(shadeOutputFB, fxaaOutputFB);
		m_CpuMs[2] = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - submitStart).count() - m_CpuMs[0] - m_CpuMs[1];
		if (profilingEnabled) view.metrics.EndFrame(m_CpuMs);
	}

	void RenderSystem::Prepare_Pass(std::shared_ptr<ENGINE_RENDERING::Camera3D> camera, ENGINE_CORE::ECS::Registry& runtimeRegistry)
	{
		auto& mainRegistry = MAIN_REGISTRY();

		auto& bufferManager = mainRegistry.GetBufferManager();
		// uniform block buffers
		const auto& matrixUbo = bufferManager.GetUniformBuffer("matrix");
		// shader storage buffers
		const auto& dirLightsSSbo = bufferManager.GetShaderStorageBuffer("DirLights");
		const auto& pointLightsSSbo = bufferManager.GetShaderStorageBuffer("PointLights");
		const auto& areaLightsSSbo = bufferManager.GetShaderStorageBuffer("AreaLights");

		// uniform block -- camera param 
		auto viewMatrix = camera->GetViewMatrix();
		glm::mat4 orthoMatrix = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
		glm::mat4 PerspectiveMatrix = camera->GetProjectionMatrix();
		// uniform block -- view/projction matrix
		matrixUbo->UpdateUniformBuffer(glm::value_ptr(viewMatrix), sizeof(viewMatrix), 0);
		matrixUbo->UpdateUniformBuffer(glm::value_ptr(PerspectiveMatrix), sizeof(PerspectiveMatrix), sizeof(glm::mat4));

		// 更新并填充光源SSBO数据
		auto& lightSystem = mainRegistry.GetContext<std::shared_ptr<ENGINE_CORE::Systems::LightSystem>>();
		auto collectStart = std::chrono::steady_clock::now();
		lightSystem->Update(runtimeRegistry, *camera);
		m_CpuMs[0] = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - collectStart).count();
		auto uploadStart = std::chrono::steady_clock::now();
		auto& dirLightData = lightSystem->GetDirLightData();
		auto& pointLightData = lightSystem->GetPointLightData();
		auto& areaLightData = lightSystem->GetAreaLightData();
		dirLightsSSbo->UpdateShaderStorageBuffer(dirLightData.data(), dirLightData.size() * sizeof(ENGINE_RENDERING::DirLight));
		pointLightsSSbo->UpdateShaderStorageBuffer(pointLightData.data(), pointLightData.size() * sizeof(ENGINE_RENDERING::PointLight));
		areaLightsSSbo->UpdateShaderStorageBuffer(areaLightData.data(), areaLightData.size() * sizeof(ENGINE_RENDERING::AreaLight));
		m_CpuMs[1] = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - uploadStart).count();

	}

	void RenderSystem::Shadow_Pass(ENGINE_CORE::ECS::Registry& runtimeRegistry)
	{
		auto& mainRegistry = MAIN_REGISTRY();

		auto& assetManager = mainRegistry.GetAssetManager();
		auto Shader_ShadowMap = assetManager.GetShader("shadow_Map");
		if (Shader_ShadowMap->ShaderProgramID() == 0)
		{
			ENGINE_ERROR("Shader has not been set correctly!");
			return;
		}
		auto Shader_ShadowCubemap = assetManager.GetShader("shadow_Cubemap");
		if (Shader_ShadowCubemap->ShaderProgramID() == 0)
		{
			ENGINE_ERROR("Shader has not been set correctly!");
			return;
		}
		auto Shader_moment = assetManager.GetShader("vssm_moments");
		if (Shader_moment->ShaderProgramID() == 0)
		{
			ENGINE_ERROR("Shader has not been set correctly!");
			return;
		}
		auto Shader_sat = assetManager.GetShader("vssm_sat");
		if (Shader_sat->ShaderProgramID() == 0)
		{
			ENGINE_ERROR("Shader has not been set correctly!");
			return;
		}
		const auto& quad = assetManager.GetModel("quad")->GetMeshes();

		auto& lightSystem = mainRegistry.GetContext<std::shared_ptr<ENGINE_CORE::Systems::LightSystem>>();
		auto& dirLightData = lightSystem->GetDirLightData();
		auto& pointLightData = lightSystem->GetPointLightData();
		auto& areaLightData = lightSystem->GetAreaLightData();
		const int dirShadowCount = (std::min)(lightSystem->GetActivatedDirLights(), ENGINE_RENDERING::MAX_DIR_SHADOWS);
		const int pointShadowCount = (std::min)(lightSystem->GetActivatedPointLights(), ENGINE_RENDERING::MAX_POINT_SHADOWS);
		const int areaShadowCount = (std::min)(lightSystem->GetActivatedAreaLights(), ENGINE_RENDERING::MAX_AREA_SHADOWS);

		auto& bufferManager = mainRegistry.GetBufferManager();

		// 面光源
		Shader_ShadowMap->Enable();
		for (int index = 0; index < areaShadowCount; index++)
		{
			const auto& shadowMap = bufferManager.GetFrameBuffer("areaLight_ShadowMap_" + std::to_string(index));
			shadowMap->Bind();

			glViewport(0, 0, shadowMap->Width(), shadowMap->Height());
			glClearColor(0.f, 0.f, 0.f, 1.f);
			glClear(GL_DEPTH_BUFFER_BIT);

			const auto& areaLight = areaLightData[index];
			Shader_ShadowMap->SetUniformMat4("lightSpaceMatrix", areaLight.lightSpaceMatrix);

			// render scene
			auto view = runtimeRegistry.GetRegistry().view<TransformComponent, MeshFilter, MeshRender, Identification>();
			for (auto [entity, transform, meshF, meshR, id] : view.each())
			{
				if (!meshR.shouldRender)
					continue;

				glm::mat4 model = CalculateModelMatrix(transform, id, runtimeRegistry);
				Shader_ShadowMap->SetUniformMat4("model", model);
				auto pModel = assetManager.GetModel(meshF.mesh);
				if (!pModel)
				{
					pModel = assetManager.GetModel("cube");
				}
				const std::vector<Mesh>& meshes = pModel->GetMeshes();
				for (size_t meshIdx = 0; meshIdx < meshes.size(); meshIdx++)
					meshes[meshIdx].Draw();
			}

			shadowMap->Unbind();
		}

		// 方向光
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(1.5f, 2.0f);
		Shader_ShadowMap->Enable();
		const auto& shadowCascade = bufferManager.GetFrameBuffer("dirLight_CSM");
		for (int index = 0; index < dirShadowCount; index++)
		{
			int layerCount = DIR_CASCADE_COUNT;
			for (int layer = 0; layer < layerCount; layer++)
			{
				shadowCascade->SetDepthLayer(index * DIR_CASCADE_COUNT + layer);
				shadowCascade->Bind();
				glViewport(0, 0, shadowCascade->Width(), shadowCascade->Height());
				glClear(GL_DEPTH_BUFFER_BIT);
				Shader_ShadowMap->SetUniformMat4("lightSpaceMatrix", dirLightData[index].lightSpaceMatrices[layer]);

				// render scene
				auto view = runtimeRegistry.GetRegistry().view<TransformComponent, MeshFilter, MeshRender, Identification>();
				for (auto [entity, transform, meshF, meshR, id] : view.each())
				{
					if (!meshR.shouldRender)
						continue;

					glm::mat4 model = CalculateModelMatrix(transform, id, runtimeRegistry);
					Shader_ShadowMap->SetUniformMat4("model", model);
					auto pModel = assetManager.GetModel(meshF.mesh);
					if (!pModel)
					{
						pModel = assetManager.GetModel("cube");
					}
					const std::vector<Mesh>& meshes = pModel->GetMeshes();
					for (size_t meshIdx = 0; meshIdx < meshes.size(); meshIdx++)
						meshes[meshIdx].Draw();
				}
				shadowCascade->Unbind();
			}
		}
		glDisable(GL_POLYGON_OFFSET_FILL);

		// 点光源
		const auto& shadowCubemap = bufferManager.GetFrameBuffer("pointLight_ShadowMap");
		if (pointShadowsEnabled) {
			shadowCubemap->Bind();
			glViewport(0, 0, shadowCubemap->Width(), shadowCubemap->Height());
			glClear(GL_DEPTH_BUFFER_BIT);
		}
		Shader_ShadowCubemap->Enable();
		for (int index = 0; index < (pointShadowsEnabled ? pointShadowCount : 0); index++)
		{
			const auto& point_light = pointLightData[index];
			if (point_light.attenuation.w <= 0.0f) continue;	// range不大于0的点光源没作用

			Shader_ShadowCubemap->SetUniformInt("shadowIndex", index);
			
			// light space matrix
			glm::vec3 point_light_pos = glm::vec3(point_light.position);
			GLfloat aspect = (GLfloat)shadowCubemap->Width() / (GLfloat)shadowCubemap->Height();
			float near_plane = 0.1f, far_plane = 100.0f;
			glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), aspect, near_plane, far_plane);
			std::vector<glm::mat4> shadowTransforms;
			shadowTransforms.push_back(shadowProj * glm::lookAt(point_light_pos, point_light_pos + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
			shadowTransforms.push_back(shadowProj * glm::lookAt(point_light_pos, point_light_pos + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
			shadowTransforms.push_back(shadowProj * glm::lookAt(point_light_pos, point_light_pos + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)));
			shadowTransforms.push_back(shadowProj * glm::lookAt(point_light_pos, point_light_pos + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0)));
			shadowTransforms.push_back(shadowProj * glm::lookAt(point_light_pos, point_light_pos + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0)));
			shadowTransforms.push_back(shadowProj * glm::lookAt(point_light_pos, point_light_pos + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0)));
			Shader_ShadowCubemap->SetUniformFloat("far_plane", far_plane);
			Shader_ShadowCubemap->SetUniformVec3("lightPos", point_light_pos);
			for (int face = 0; face < 6; ++face)
				Shader_ShadowCubemap->SetUniformMat4("shadowMatrices[" + std::to_string(face) + "]", shadowTransforms[face]);

			// render scene
			auto view = runtimeRegistry.GetRegistry().view<TransformComponent, MeshFilter, MeshRender, Identification>();
			for (auto [entity, transform, meshF, meshR, id] : view.each())
			{
				if (!meshR.shouldRender)
					continue;

				glm::mat4 model = CalculateModelMatrix(transform, id, runtimeRegistry);
				Shader_ShadowCubemap->SetUniformMat4("model", model);
				auto pModel = assetManager.GetModel(meshF.mesh);
				if (!pModel)
				{
					pModel = assetManager.GetModel("cube");
				}
				const std::vector<Mesh>& meshes = pModel->GetMeshes();
				for (size_t meshIdx = 0; meshIdx < meshes.size(); meshIdx++)
					meshes[meshIdx].Draw();
			}
		}
		shadowCubemap->Unbind();

		// VSSM
		const auto& scratch = bufferManager.GetFrameBuffer("areaLight_VSSM_scratch"); // 乒乓缓冲
		for (int index = 0; index < areaShadowCount; index++)
		{
			const auto& areaLight = areaLightData[index];
			const auto shadowMap = bufferManager.GetFrameBuffer("areaLight_ShadowMap_" + std::to_string(index));
			const auto vssm = bufferManager.GetFrameBuffer("areaLight_VSSM_" + std::to_string(index)); // 0号纹理是深度和深度平方SAT，1号纹理是覆盖SAT
			vssm->Bind();
			glViewport(0, 0, vssm->Width(), vssm->Height());
			// 根据shadowMap，算出每个像素的moment和是否有效
			Shader_moment->Enable();
			Shader_moment->SetUniformVec2("nearFar", areaLight.shadowParams.x, areaLight.shadowParams.y);
			glBindTextureUnit(0, shadowMap->GetTextureID());
			quad[0].Draw();
			// moment转sat
			auto source = vssm;
			auto target = scratch;
			Shader_sat->Enable();
			//先横向累加，在竖向累加
			for (int axis = 0; axis < 2; axis++)
			{
				Shader_sat->SetUniformInt("axis", axis);
				const int size = axis == 0 ? vssm->Width() : vssm->Height();
				// Hillis–Steele 扫描
				for (int stride = 1; stride < size; stride *= 2)
				{
					target->Bind();
					glBindTextureUnit(0, source->GetTextureID(0));
					glBindTextureUnit(1, source->GetTextureID(1));
					Shader_sat->SetUniformInt("stride", stride);
					quad[0].Draw();
					std::swap(source, target);
				}
			}
			vssm->Unbind();
		}
	}

	void RenderSystem::Geometry_Pass(std::shared_ptr<ENGINE_RENDERING::Camera3D> camera, ENGINE_CORE::ECS::Registry& runtimeRegistry)
	{
		auto& mainRegistry = MAIN_REGISTRY();

		auto& assetManager = mainRegistry.GetAssetManager();
		auto Shader_Gbuffer = assetManager.GetShader("defer_Gbuffer");
		if (Shader_Gbuffer->ShaderProgramID() == 0)
		{
			ENGINE_ERROR("Shader has not been set correctly!");
			return;
		}

		// render object
		Shader_Gbuffer->Enable();
		auto view = runtimeRegistry.GetRegistry().view<TransformComponent, MeshFilter, MeshRender, Identification>();
		for (auto [entity, transform, meshF, meshR, id] : view.each())
		{
			if (!meshR.shouldRender)
			{
				continue;
			}
			if (id.selected)
			{
				glStencilFunc(GL_ALWAYS, 1, 0xFF);		// 总是通过模板测试，且ref为1
				glStencilMask(0xFF);					// 允许写入模板值
			}
			else
			{
				glStencilMask(0x00);					// 禁止写入模板值
			}

			auto pModel = assetManager.GetModel(meshF.mesh);
			if (!pModel)
			{
				pModel = assetManager.GetModel("cube");
			}
			const std::vector<Mesh>& meshes = pModel->GetMeshes();
			if (meshF.changed || meshR.CheckMaterialEmpty())
			{
				meshF.changed = false;
				meshR.ResetMaterial(meshes);
			}

			glm::mat4 model = CalculateModelMatrix(transform, id, runtimeRegistry);
			glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
			for (int mesh_index = 0; mesh_index < meshes.size(); mesh_index++)
			{
				ENGINE_CORE::ECS::Material& cur_material = meshR.GetMaterial(mesh_index);

				bool emptyAlbedo = cur_material.m_textures.find("albedo")->second.empty();
				bool textureBug = (cur_material.m_useTexture == true) && (emptyAlbedo);

				std::string shaderName = cur_material.shadingModel;

				Shader_Gbuffer->SetUniformMat4("model", model);
				Shader_Gbuffer->SetUniformMat3("normalMatrix", normalMatrix);
				Shader_Gbuffer->SetUniformBool("bug", textureBug);
				Shader_Gbuffer->SetUniformBool("flipUV", meshR.flipUV);
				Shader_Gbuffer->SetUniformBool("useTexture", cur_material.m_useTexture);
				Shader_Gbuffer->SetUniformVec4("material.color", cur_material.color);
				Shader_Gbuffer->SetUniformFloat("material.metallic", cur_material.metallic);
				Shader_Gbuffer->SetUniformFloat("material.roughness", cur_material.roughness);
				Shader_Gbuffer->SetUniformFloat("material.ao", cur_material.ao);

				// set uniform textures
				if (cur_material.m_useTexture) {
					const auto& slots = TextureRegistry::GetSlots();
					for (size_t slot_index = 0; slot_index < slots.size(); ++slot_index) {
						const auto& slot = slots[slot_index];
						auto it = cur_material.m_textures.find(slot.key);
						bool hasTexture = (it != cur_material.m_textures.end() && !it->second.empty());
						Shader_Gbuffer->SetUniformBool(slot.shaderFlag, hasTexture);
						if (hasTexture) {
							auto tex = assetManager.GetTexture(it->second);
							glBindTextureUnit(slot_index, tex->GetID());
						}
					}
				}
				meshes[mesh_index].Draw();
			}
		}
	}

	void RenderSystem::Tiling_Pass(const std::shared_ptr<ENGINE_RENDERING::Framebuffer>& intermediateGB, const ENGINE_RENDERING::Camera3D& camera)
	{
		auto& mainRegistry = MAIN_REGISTRY();
		auto& clusterSystem = mainRegistry.GetContext<std::shared_ptr<ENGINE_CORE::Systems::ClusterSystem>>();
		clusterSystem->Update(intermediateGB, camera, lightingMode);
	}

	void RenderSystem::HBAO_Pass(std::shared_ptr<ENGINE_RENDERING::Camera3D> camera, ENGINE_CORE::ECS::Registry& runtimeRegistry, std::shared_ptr<ENGINE_RENDERING::Framebuffer> intermediateGB)
	{
		auto& assetManager = MAIN_REGISTRY().GetAssetManager();
		auto shader = assetManager.GetShader("defer_HBAO");
		auto noise = assetManager.GetTexture("aoNoise");

		shader->Enable();
		glBindTextureUnit(0, intermediateGB->GetTextureID(0));
		glBindTextureUnit(2, noise->GetID());
		const std::vector<Mesh>& quad = assetManager.GetModel("quad")->GetMeshes();
		quad[0].Draw();
	}

	void RenderSystem::AOBlur_Pass(std::shared_ptr<ENGINE_RENDERING::Framebuffer> source, std::shared_ptr<ENGINE_RENDERING::Framebuffer> intermediateGB, const glm::vec2& direction)
	{
		auto& assetManager = MAIN_REGISTRY().GetAssetManager();
		auto shader = assetManager.GetShader("defer_AOBlur");

		shader->Enable();
		shader->SetUniformVec2("direction", direction);
		glBindTextureUnit(0, source->GetTextureID(0));
		glBindTextureUnit(1, intermediateGB->GetTextureID(0));
		const std::vector<Mesh>& quad = assetManager.GetModel("quad")->GetMeshes();
		quad[0].Draw();
	}

	void RenderSystem::Lighting_Pass(
		std::shared_ptr<ENGINE_RENDERING::Camera3D> camera, ENGINE_CORE::ECS::Registry& runtimeRegistry,
		std::shared_ptr<ENGINE_RENDERING::Framebuffer> intermediateGB, std::shared_ptr<ENGINE_RENDERING::Framebuffer> aoFB)
	{
		auto& mainRegistry = MAIN_REGISTRY();

		auto& clusterSystem = mainRegistry.GetContext<std::shared_ptr<ENGINE_CORE::Systems::ClusterSystem>>();
		auto& lightSystem = mainRegistry.GetContext<std::shared_ptr<ENGINE_CORE::Systems::LightSystem>>();
		auto& dirLightData = lightSystem->GetDirLightData();
		auto& pointLightData = lightSystem->GetPointLightData();
		auto& areaLightData = lightSystem->GetAreaLightData();

		auto& bufferManager = mainRegistry.GetBufferManager();
		const auto& ibl_fb = bufferManager.GetFrameBuffer("IBL");

		auto& assetManager = mainRegistry.GetAssetManager();
		// const auto env_cubemap_id = assetManager.GetTexture("skybox")->GetID();
		auto Shader_Lighting = assetManager.GetShader("defer_Lighting");
		if (Shader_Lighting->ShaderProgramID() == 0)
		{
			ENGINE_ERROR("Shader has not been set correctly!");
			return;
		}

		Shader_Lighting->Enable();
		Shader_Lighting->SetUniformFloat("far_plane", 100.0f);
		Shader_Lighting->SetUniformVec3("viewPos", camera->GetPosition());
		Shader_Lighting->SetUniformInt("dirLightCount", lightSystem->GetActivatedDirLights());
		Shader_Lighting->SetUniformInt("pointLightCount", lightSystem->GetActivatedPointLights());
		Shader_Lighting->SetUniformInt("areaLightCount", lightSystem->GetActivatedAreaLights());

		if (lightingMode != LightingMode::Full) clusterSystem->Bind(intermediateGB);
		Shader_Lighting->SetUniformInt("lightingMode", static_cast<int>(lightingMode));
		Shader_Lighting->SetUniformBool("pointShadowsEnabled", pointShadowsEnabled);
		Shader_Lighting->SetUniformFloat("clusterNear", camera->GetNearPlane());
		Shader_Lighting->SetUniformFloat("clusterDepthScale", CLUSTER_DEPTH_SLICES / std::log(camera->GetFarPlane() / camera->GetNearPlane()));
		glBindTextureUnit(0, intermediateGB->GetTextureID(0));
		glBindTextureUnit(1, intermediateGB->GetTextureID(1));
		glBindTextureUnit(2, intermediateGB->GetTextureID(2));
		glBindTextureUnit(3, intermediateGB->GetTextureID(3));
		glBindTextureUnit(4, aoFB->GetTextureID(0));
		glBindTextureUnit(5, ibl_fb->GetTextureID(1));
		glBindTextureUnit(6, ibl_fb->GetTextureID(2));
		glBindTextureUnit(7, ibl_fb->GetTextureID(3));

		// 纹理单元
		int pointShadowBaseUnit = 10;
		int dirShadowBaseUnit = 11;
		int areaMomentBaseUnit = 12;
		int areaCoverageBaseUnit = 13;

		// set directional light CSM
		const auto& shadowCascade = bufferManager.GetFrameBuffer("dirLight_CSM");
		glBindTextureUnit(dirShadowBaseUnit, shadowCascade->GetTextureID());
		Shader_Lighting->SetUniformInt("dirLight_CSM", dirShadowBaseUnit);

		// set point light shadow cubemap array
		const auto& shadowMap = bufferManager.GetFrameBuffer("pointLight_ShadowMap");
		glBindTextureUnit(pointShadowBaseUnit, shadowMap->GetTextureID());
		Shader_Lighting->SetUniformInt("pointLight_ShadowMap", pointShadowBaseUnit);

		// set area light VSSM
		for (int area_light_index = 0; area_light_index < ENGINE_RENDERING::MAX_AREA_SHADOWS; area_light_index++)
		{
			int momentUnit = areaMomentBaseUnit + area_light_index;
			int coverageUnit = areaCoverageBaseUnit + area_light_index;
			const auto& vssm = bufferManager.GetFrameBuffer("areaLight_VSSM_" + std::to_string(area_light_index));
			glBindTextureUnit(momentUnit, vssm->GetTextureID(0));	// depth SAT
			glBindTextureUnit(coverageUnit, vssm->GetTextureID(1));	// count SAT
			Shader_Lighting->SetUniformInt("areaMomentSAT[" + std::to_string(area_light_index) + "]", momentUnit);
			Shader_Lighting->SetUniformInt("areaCoverageSAT[" + std::to_string(area_light_index) + "]", coverageUnit);
		}
		glDisable(GL_DEPTH_TEST);
		const std::vector<Mesh>& quad = assetManager.GetModel("quad")->GetMeshes();
		quad[0].Draw();
		glEnable(GL_DEPTH_TEST);
	}

	void RenderSystem::Postprocess_Pass(std::shared_ptr<ENGINE_RENDERING::Camera3D> camera, ENGINE_CORE::ECS::Registry& runtimeRegistry)
	{
		auto& mainRegistry = MAIN_REGISTRY();

		auto& lightSystem = mainRegistry.GetContext<std::shared_ptr<ENGINE_CORE::Systems::LightSystem>>();
		auto& pointLightData = lightSystem->GetPointLightData();
		auto& dirLightData = lightSystem->GetDirLightData();
		auto& areaLightData = lightSystem->GetAreaLightData();

		auto& assetManager = mainRegistry.GetAssetManager();
		const auto& cube = assetManager.GetModel("cube");
		const auto& sphere = assetManager.GetModel("sphere");
		//const auto skybox_texture_id = assetManager.GetTexture("skybox")->GetID();
		const auto skybox_texture_id = mainRegistry.GetBufferManager().GetFrameBuffer("IBL")->GetTextureID(0);
		auto Shader_Skybox = assetManager.GetShader("skybox");
		if (Shader_Skybox->ShaderProgramID() == 0)
		{
			ENGINE_ERROR("Shader has not been set correctly!");
			return;
		}
		auto Shader_Color = assetManager.GetShader("forward_Color");
		if (Shader_Color->ShaderProgramID() == 0)
		{
			ENGINE_ERROR("Shader has not been set correctly!");
			return;
		}
		auto Shader_AreaLight = assetManager.GetShader("forward_AreaLight");
		if (Shader_AreaLight->ShaderProgramID() == 0)
		{
			ENGINE_ERROR("Shader has not been set correctly!");
			return;
		}

		// 绘制面光源
		Shader_AreaLight->Enable();
		for (int area_light_index = 0; area_light_index < lightSystem->GetActivatedAreaLights(); area_light_index++) {
			auto area_light = areaLightData[area_light_index];

			glm::vec3 U = glm::vec3(area_light.rightVector_halfWidth);
			glm::vec3 V = glm::vec3(area_light.upVector_halfHeight);
			glm::vec3 F = glm::vec3(area_light.direction);
			glm::mat4 orientation(1.0f);
			orientation[0] = glm::vec4(U, 0.0f);
			orientation[1] = glm::vec4(F, 0.0f);
			orientation[2] = glm::vec4(V, 0.0f);

			glm::mat4 light_rect_model = glm::mat4(1.0f);
			light_rect_model = glm::translate(light_rect_model, glm::vec3(area_light.center_pos));
			light_rect_model = light_rect_model * orientation;
			light_rect_model = glm::scale(light_rect_model, glm::vec3(area_light.rightVector_halfWidth.w, 0.001f, area_light.upVector_halfHeight.w));
			glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(light_rect_model)));
			Shader_AreaLight->SetUniformMat4("model", light_rect_model);
			Shader_AreaLight->SetUniformMat3("normalMatrix", normalMatrix);
			Shader_AreaLight->SetUniformVec3("color", glm::vec3(area_light.color));
			Shader_AreaLight->SetUniformVec3("direction", glm::vec3(area_light.direction));
			Shader_AreaLight->SetUniformBool("outline", false);
			cube->Draw();
		}

		// 绘制点光源
		Shader_Color->Enable();
		for (int point_light_index = 0; point_light_index < lightSystem->GetActivatedPointLights(); point_light_index++)
		{
			if (!showLightVisualizations) continue;
			auto point_light = pointLightData[point_light_index];
			// TODO: check point_light.render to decide if render
			glm::mat4 light_sphere_model = glm::mat4(1.0f);
			light_sphere_model = glm::translate(light_sphere_model, glm::vec3(point_light.position));
			light_sphere_model = glm::scale(light_sphere_model, glm::vec3(0.25));
			glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(light_sphere_model)));
			Shader_Color->SetUniformMat4("model", light_sphere_model);
			Shader_Color->SetUniformMat3("normalMatrix", normalMatrix);
			Shader_Color->SetUniformVec3("color", glm::vec3(point_light.color));
			Shader_Color->SetUniformBool("outline", false);
			sphere->Draw();
		}

		// 绘制方向光
		for (int dir_light_index = 0; dir_light_index < lightSystem->GetActivatedDirLights(); dir_light_index++)
		{
			auto dir_light = dirLightData[dir_light_index];
			glm::vec3 pos = glm::vec3(0.0f) - glm::vec3((dir_light.direction * 10.0f));
			glm::mat4 light_sphere_model = glm::mat4(1.0f);
			light_sphere_model = glm::translate(light_sphere_model, pos);
			light_sphere_model = glm::scale(light_sphere_model, glm::vec3(0.25));
			glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(light_sphere_model)));
			Shader_Color->SetUniformMat4("model", light_sphere_model);
			Shader_Color->SetUniformMat3("normalMatrix", normalMatrix);
			Shader_Color->SetUniformVec3("color", glm::vec3(dir_light.color));
			Shader_Color->SetUniformBool("outline", false);
			sphere->Draw();
		}

		// 模板测试
		glEnable(GL_STENCIL_TEST);					// 开启模板测试
		glStencilFunc(GL_NOTEQUAL, 1, 0xFF);		// 当目标像素的模板值不等于1时，通过测试
		glStencilMask(0x00);						// 禁止写入模板值
		Shader_Color->Enable();
		auto view = runtimeRegistry.GetRegistry().view<TransformComponent, MeshFilter, MeshRender, Identification>();
		for (auto [entity, transform, meshF, meshR, id] : view.each())
		{
			if (!meshR.shouldRender || !id.selected)
			{
				continue;
			}

			auto pModel = assetManager.GetModel(meshF.mesh);
			if (!pModel)
			{
				pModel = assetManager.GetModel("cube");
			}
			const std::vector<Mesh>& meshes = pModel->GetMeshes();
			if (meshF.changed || meshR.CheckMaterialEmpty())
			{
				meshF.changed = false;
				meshR.ResetMaterial(meshes);
			}

			glm::mat4 model = CalculateModelMatrix(transform, id, runtimeRegistry);
			for (int mesh_index = 0; mesh_index < meshes.size(); mesh_index++)
			{
				ENGINE_CORE::ECS::Material& cur_material = meshR.GetMaterial(mesh_index);

				bool emptyAlbedo = cur_material.m_textures.find("albedo")->second.empty();
				bool textureBug = (cur_material.m_useTexture == true) && (emptyAlbedo);

				std::string shaderName = cur_material.shadingModel;

				Shader_Color->Enable();	// NOTE: now the shader is fixed
				Shader_Color->SetUniformMat4("model", model);
				Shader_Color->SetUniformVec3("color", glm::vec3(1.0f, 1.0f, 0.0f));
				Shader_Color->SetUniformBool("outline", true);

				meshes[mesh_index].Draw();
			}
		}
		glStencilMask(0xFF);						// 允许写入模板值
		glStencilFunc(GL_ALWAYS, 0, 0xFF);			// 总是通过模板测试，且ref为0
		glDisable(GL_STENCIL_TEST);					// 关闭模板测试

		// draw skybox
		glDisable(GL_CULL_FACE);
		glDepthFunc(GL_LEQUAL);
		glm::mat4 viewMatrix = camera->GetViewMatrix();
		glm::mat4 PerspectiveMatrix = camera->GetProjectionMatrix();
		Shader_Skybox->Enable();
		Shader_Skybox->SetUniformMat4("view", viewMatrix);
		Shader_Skybox->SetUniformMat4("projection", PerspectiveMatrix);
		glBindTextureUnit(0, skybox_texture_id);
		cube->Draw();
		glDepthFunc(GL_LESS);
		glEnable(GL_CULL_FACE);

		// 绘制物理调试线框
		if (ENGINE_CORE::CoreEngineData::GetInstance().RenderCollidersEnabled())
		{
			auto& physicsWorld = runtimeRegistry.GetContext<std::shared_ptr<rp3d::PhysicsWorld>>();
			auto& physicsDebugger = physicsWorld->getDebugRenderer();

			auto Shader_PhysicsDebug = assetManager.GetShader("physics_Debug");
			if (Shader_PhysicsDebug->ShaderProgramID() == 0)
			{
				ENGINE_ERROR("Shader has not been set correctly!");
				return;
			}
			Shader_PhysicsDebug->Enable();
			Shader_PhysicsDebug->SetUniformMat4("model", glm::mat4(1.0f));

			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

			const uint nbTriangles = physicsDebugger.getNbTriangles();
			GLsizei sizeVertices = static_cast<GLsizei>(nbTriangles * sizeof(rp3d::DebugRenderer::DebugTriangle));

			glBindVertexArray(m_DebugVAO);
			glBindBuffer(GL_ARRAY_BUFFER, m_DebugVBO);

			const void* data = physicsDebugger.getTrianglesArray();
			glBufferData(GL_ARRAY_BUFFER, sizeVertices, data, GL_STATIC_DRAW);

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(rp3d::Vector3) + sizeof(rp3d::uint32), (char*)nullptr);
			glEnableVertexAttribArray(1);
			glVertexAttribIPointer(1, 3, GL_UNSIGNED_INT, sizeof(rp3d::Vector3) + sizeof(rp3d::uint32), (void*)sizeof(rp3d::Vector3));

			glDrawArrays(GL_TRIANGLES, 0, physicsDebugger.getNbTriangles() * 3);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
	}

	void RenderSystem::FXAA_Pass(
		const std::shared_ptr<ENGINE_RENDERING::Framebuffer>& source,
		const std::shared_ptr<ENGINE_RENDERING::Framebuffer>& output)
	{
		auto& assetManager = MAIN_REGISTRY().GetAssetManager();
		auto shader = assetManager.GetShader("fxaa");
		auto quad = assetManager.GetModel("quad");

		output->Bind();
		glViewport(0, 0, output->Width(), output->Height());
		glBindTextureUnit(0, source->GetTextureID());
		shader->Enable();
		shader->SetUniformVec2("inverseInputSize", 1.0f / source->Width(), 1.0f / source->Height());
		quad->GetMeshes()[0].Draw();
		output->Unbind();
	}

    void RenderSystem::StartLightBenchmark()
    {
        profilingEnabled = true;
        MAIN_REGISTRY().GetContext<std::shared_ptr<ClusterSystem>>()->StartCapture();
    }

    void RenderSystem::ExportLightBenchmark(const std::string& path) const
    {
        auto system = MAIN_REGISTRY().GetContext<std::shared_ptr<ClusterSystem>>();
        std::ofstream output(path);
        output << "gpu,build,viewport,width,height,vsync,mode,diagnostics,point_shadows,visualizations,label,lights,shadow_slots,shadow_resolution,shadow_far,tile_size,depth_slices,list_capacity,list_count,header_bytes,index_bytes,near,far,view,projection,warmup,target,record,geometry_ms,culling_ms,lighting_ms,shadow_ms,collect_ms,upload_ms,submit_ms,culling_lighting_ms,active_clusters,average_candidates,max_candidates,overflow_clusters,weighted_candidates,weighted_evaluations\n";
        output << std::setprecision(9);
        for (const auto& [key, view] : system->GetViews()) {
            const auto& metrics = view.metrics;
            const auto& samples = metrics.Samples();
            if (metrics.IsRecording() || samples.size() != 300) continue;
            auto write = [&](const LightRenderSample& sample, const std::string& record, double combined) {
                output << metrics.Configuration() << ',' << record;
                for (double value : sample.gpuMs) output << ',' << value;
                for (double value : sample.cpuMs) output << ',' << value;
                output << ',' << combined;
                if (sample.statisticsAvailable)
                    output << ',' << sample.activeClusters << ',' << sample.averageCandidates << ',' << sample.maxCandidates
                        << ',' << sample.overflowClusters << ',' << sample.weightedCandidates << ',' << sample.weightedEvaluations;
                else output << ",NA,NA,NA,NA,NA,NA";
                output << '\n';
            };
            for (size_t i = 0; i < samples.size(); ++i)
                write(samples[i], std::to_string(i), samples[i].gpuMs[1] + samples[i].gpuMs[2]);
            for (double percentile : {0.5, 0.95}) {
                auto quantile = [&](auto get) {
                    std::vector<double> values;
                    for (const auto& sample : samples) values.push_back(get(sample));
                    std::sort(values.begin(), values.end());
                    return values[static_cast<size_t>(percentile * (values.size() - 1))];
                };
                LightRenderSample summary;
                for (int i = 0; i < 4; ++i) summary.gpuMs[i] = quantile([&](const auto& s) { return s.gpuMs[i]; });
                for (int i = 0; i < 3; ++i) summary.cpuMs[i] = quantile([&](const auto& s) { return s.cpuMs[i]; });
                summary.statisticsAvailable = std::all_of(samples.begin(), samples.end(), [](const auto& s) { return s.statisticsAvailable; });
                summary.activeClusters = static_cast<unsigned int>(quantile([](const auto& s) { return double(s.activeClusters); }));
                summary.averageCandidates = quantile([](const auto& s) { return s.averageCandidates; });
                summary.maxCandidates = static_cast<unsigned int>(quantile([](const auto& s) { return double(s.maxCandidates); }));
                summary.overflowClusters = static_cast<unsigned int>(quantile([](const auto& s) { return double(s.overflowClusters); }));
                summary.weightedCandidates = quantile([](const auto& s) { return s.weightedCandidates; });
                summary.weightedEvaluations = quantile([](const auto& s) { return s.weightedEvaluations; });
                write(summary, percentile == 0.5 ? "median" : "p95", quantile([](const auto& s) { return s.gpuMs[1] + s.gpuMs[2]; }));
            }
        }
    }

}

