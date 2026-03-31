#include "RenderSystem.h"
#include<glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include<glm/gtx/quaternion.hpp>
#include<glm/gtc/quaternion.hpp>
#include<Rendering/Core/Camera3D.h>
#include<Rendering/Essentials/Shader.h>
#include<Rendering/Essentials/Lights.h>
#include<Rendering/Essentials/TextureCommon.h>
#include<Logger/Logger.h>
#include<../CORE/Core/ECS/MainRegistry.h>
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

	void RenderSystem::Prepare_Pass(std::shared_ptr<ENGINE_RENDERING::Camera3D> camera, ENGINE_CORE::ECS::Registry& runtimeRegistry)
	{
		auto& mainRegistry = MAIN_REGISTRY();
		auto& lightSystem = mainRegistry.GetContext<std::shared_ptr<ENGINE_CORE::Systems::LightSystem>>();

		// uniform block buffers
		auto& bufferManager = mainRegistry.GetBufferManager();
		const auto& matrixUbo = bufferManager.GetUniformBuffer("matrix");
		const auto& dirLightsUbo = bufferManager.GetUniformBuffer("DirLights");
		const auto& pointLightsUbo = bufferManager.GetUniformBuffer("PointLights");

		// uniform block -- camera param 
		auto viewMatrix = camera->GetViewMatrix();
		glm::mat4 orthoMatrix = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
		glm::mat4 PerspectiveMatrix = glm::perspective(glm::radians(camera->Zoom), (float)camera->GetWidth() / (float)camera->GetHeight(), 0.1f, 100.0f);
		// uniform block -- view/projction matrix
		matrixUbo->UpdateUniformBuffer(glm::value_ptr(viewMatrix), sizeof(viewMatrix), 0);
		matrixUbo->UpdateUniformBuffer(glm::value_ptr(PerspectiveMatrix), sizeof(PerspectiveMatrix), sizeof(glm::mat4));

		// uniform block -- lighting param
		lightSystem->Update(runtimeRegistry);
		auto& dirLightData = lightSystem->GetDirLightData();
		auto& pointLightData = lightSystem->GetPointLightData();

		// 填充方向光 UBO
		// 数据已经是固定 MAX_DIR_LIGHTS 大小，直接一次性上传，性能最优
		dirLightsUbo->UpdateUniformBuffer(
			dirLightData.data(),// 数据指针
			lightSystem->GetMaxDirLights() * sizeof(ENGINE_RENDERING::DirLight), // 总大小
			0                // 偏移量
		);

		// 填充点光源 UBO
		pointLightsUbo->UpdateUniformBuffer(
			pointLightData.data(),
			lightSystem->GetMaxPointLights() * sizeof(ENGINE_RENDERING::PointLight),
			0
		);

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

		auto& lightSystem = mainRegistry.GetContext<std::shared_ptr<ENGINE_CORE::Systems::LightSystem>>();
		auto& dirLightData = lightSystem->GetDirLightData();
		auto& pointLightData = lightSystem->GetPointLightData();

		auto& bufferManager = mainRegistry.GetBufferManager();

		// 方向光
		Shader_ShadowMap->Enable();
		for (int dirlight_index = 0; dirlight_index < lightSystem->GetActivatedDirLights(); dirlight_index++)
		{
			const auto& shadowMap = bufferManager.GetFrameBuffer("shadow_map_" + std::to_string(dirlight_index));
			shadowMap->Bind();

			glViewport(0, 0, shadowMap->Width(), shadowMap->Height());
			glClearColor(0.f, 0.f, 0.f, 1.f);
			glClear(GL_DEPTH_BUFFER_BIT);

			auto dirLight = dirLightData[dirlight_index];
			Shader_ShadowMap->SetUniformMat4("lightSpaceMatrix", dirLight.lightSpaceMatrix);

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
			// shadowMap->CheckResize(); // no need

		}

		// 点光源
		Shader_ShadowCubemap->Enable();
		for (int pointlight_index = 0; pointlight_index < lightSystem->GetActivatedPointLights(); pointlight_index++)
		{
			const auto& shadowCubemap = bufferManager.GetFrameBuffer("shadow_cubemap_" + std::to_string(pointlight_index));
			shadowCubemap->Bind();

			glViewport(0, 0, shadowCubemap->Width(), shadowCubemap->Height());
			glClearColor(0.f, 0.f, 0.f, 1.f);
			glClear(GL_DEPTH_BUFFER_BIT);

			auto& point_light = pointLightData[pointlight_index];
			glm::vec3 point_light_pos = glm::vec3(point_light.position);

			// light space matrix
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

			shadowCubemap->Unbind();
			// shadowCubemap->CheckResize(); no need
		}
	}

	void RenderSystem::ForwardRenderPipeline(std::shared_ptr<ENGINE_RENDERING::Camera3D> camera, ENGINE_CORE::ECS::Registry& runtimeRegistry, std::shared_ptr<ENGINE_RENDERING::Framebuffer> finalOutputFB)
	{
		Prepare_Pass(camera, runtimeRegistry);

		Shadow_Pass(runtimeRegistry);

		finalOutputFB->Bind();
		glViewport(0, 0, finalOutputFB->Width(), finalOutputFB->Height());
		glClearColor(0.f, 0.f, 0.f, 1.f);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		Forward_Pass(camera, runtimeRegistry);
		finalOutputFB->Unbind();
		finalOutputFB->CheckResize();
	}

	// not updated any more, useless
	void RenderSystem::Forward_Pass(std::shared_ptr<ENGINE_RENDERING::Camera3D> camera, ENGINE_CORE::ECS::Registry& runtimeRegistry)
	{
		auto& mainRegistry = MAIN_REGISTRY();
		auto& assetManager = mainRegistry.GetAssetManager();
		auto skybox_texture = assetManager.GetTexture("skybox");

		auto& bufferManager = mainRegistry.GetBufferManager();
		const auto& map_FBO = bufferManager.GetAllFBO();

		// get shaders
		auto Shader_BlinnPhong = assetManager.GetShader("forward_BlinnPhong");
		if (Shader_BlinnPhong->ShaderProgramID() == 0)
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
		auto Shader_Skybox = assetManager.GetShader("skybox");
		if (Shader_Skybox->ShaderProgramID() == 0)
		{
			ENGINE_ERROR("Shader has not been set correctly!");
			return;
		}
		auto Shader_PhysicsDebug = assetManager.GetShader("physics_Debug");
		if (Shader_PhysicsDebug->ShaderProgramID() == 0)
		{
			ENGINE_ERROR("Shader has not been set correctly!");
			return;
		}

		// render point light sphere
		Shader_Color->Enable();
		auto& lightSystem = mainRegistry.GetContext<std::shared_ptr<ENGINE_CORE::Systems::LightSystem>>();
		auto& dirLightData = lightSystem->GetDirLightData();
		auto& pointLightData = lightSystem->GetPointLightData();
		const std::vector<Mesh>& sphere = assetManager.GetModel("sphere")->GetMeshes();
		for (int point_light_index = 0; point_light_index < lightSystem->GetActivatedPointLights(); point_light_index++)
		{
			auto point_light = pointLightData[point_light_index];
			// TODO: check point_light.render to decide if render
			glm::mat4 light_sphere_model = glm::mat4(1.0f);
			light_sphere_model = glm::translate(light_sphere_model, glm::vec3(point_light.position));
			light_sphere_model = glm::scale(light_sphere_model, glm::vec3(0.25));
			Shader_Color->SetUniformMat4("model", light_sphere_model);
			Shader_Color->SetUniformVec3("color", glm::vec3(1.0f));
			Shader_Color->SetUniformBool("outline", false);
			sphere[0].Draw();
		}

		// render object
		Shader_BlinnPhong->Enable();
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
			for (int mesh_index = 0; mesh_index < meshes.size(); mesh_index++)
			{
				ENGINE_CORE::ECS::Material& cur_material = meshR.GetMaterial(mesh_index);

				bool emptyAlbedo = cur_material.m_textures.find("albedo")->second.empty();
				bool textureBug = (cur_material.m_useTexture == true) && (emptyAlbedo);

				std::string shaderName = cur_material.shadingModel;

				Shader_BlinnPhong->Enable();	// NOTE: now the shader is fixed
				Shader_BlinnPhong->SetUniformMat4("model", model);
				Shader_BlinnPhong->SetUniformVec3("viewPos", camera->GetPosition());
				Shader_BlinnPhong->SetUniformBool("bug", textureBug);
				Shader_BlinnPhong->SetUniformBool("flipUV", meshR.flipUV);
				Shader_BlinnPhong->SetUniformVec4("material.color", cur_material.color);
				Shader_BlinnPhong->SetUniformBool("useTexture", cur_material.m_useTexture);

				Shader_BlinnPhong->SetUniformFloat("far_plane", 100.0f);

				glActiveTexture(GL_TEXTURE10);
				glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture->GetID());
				Shader_BlinnPhong->SetUniformInt("skybox", 10);

				// set direction light shadowMap
				for (int dir_light_index = 0; dir_light_index < lightSystem->GetMaxDirLights(); dir_light_index++)
				{
					std::string key = "shadow_map_" + std::to_string(dir_light_index);
					auto it = map_FBO.find(key);
					if (it != map_FBO.end())
					{
						int texUnit = 11 + dir_light_index; // 纹理单元 11, 12, 13, 14
						glActiveTexture(GL_TEXTURE0 + texUnit);
						glBindTexture(GL_TEXTURE_2D, it->second->GetTextureID());
						Shader_BlinnPhong->SetUniformInt("shadowMaps[" + std::to_string(dir_light_index) + "]", texUnit);
					}
				}

				// set point light shadowCubemap
				int cubeMapBaseUnit = 11 + lightSystem->GetMaxDirLights(); // = 15
				for (int point_light_index = 0; point_light_index < lightSystem->GetMaxPointLights(); point_light_index++)
				{
					std::string key = "shadow_cubemap_" + std::to_string(point_light_index);
					auto it = map_FBO.find(key);
					if (it != map_FBO.end())
					{
						int texUnit = cubeMapBaseUnit + point_light_index; // 15, 16, 17, 18
						glActiveTexture(GL_TEXTURE0 + texUnit);
						glBindTexture(GL_TEXTURE_CUBE_MAP, it->second->GetTextureID());
						Shader_BlinnPhong->SetUniformInt("shadowCubeMap[" + std::to_string(point_light_index) + "]", texUnit);
					}
				}

				// set uniform textures
				if (cur_material.m_useTexture) {
					const auto& slots = TextureRegistry::GetSlots();
					for (size_t slot_index = 0; slot_index < slots.size(); ++slot_index) {
						const auto& slot = slots[slot_index];

						auto it = cur_material.m_textures.find(slot.key);
						bool hasTexture = (it != cur_material.m_textures.end() && !it->second.empty());

						Shader_BlinnPhong->SetUniformBool(slot.shaderFlag, hasTexture);

						if (hasTexture) {
							glActiveTexture(GL_TEXTURE0 + (GLenum)slot_index); // 按索引自动分配纹理单元
							auto tex = assetManager.GetTexture(it->second);
							if (tex) {
								glBindTexture(GL_TEXTURE_2D, tex->GetID());
								Shader_BlinnPhong->SetUniformInt(slot.shaderSampler, (int)slot_index);
							}
						}
					}
				}
				meshes[mesh_index].Draw();
			}
		}

		// draw skybox
		auto viewMatrix = camera->GetViewMatrix();
		glm::mat4 PerspectiveMatrix = glm::perspective(glm::radians(camera->Zoom), (float)camera->GetWidth() / (float)camera->GetHeight(), 0.1f, 100.0f);
		glDepthFunc(GL_LEQUAL);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture->GetID());
		Shader_Skybox->Enable();
		Shader_Skybox->SetUniformMat4("model", glm::mat4(1.0f));
		Shader_Skybox->SetUniformMat4("view", glm::mat4(glm::mat3(viewMatrix)));	//移除观察矩阵中的位移
		Shader_Skybox->SetUniformMat4("projection", PerspectiveMatrix);
		Shader_Skybox->SetUniformInt("skybox", 0);
		const std::vector<Mesh>& skybox = assetManager.GetModel("skybox")->GetMeshes();
		skybox[0].Draw();
		glDepthFunc(GL_LESS);

		//模板测试 TODO:修复scale相同导致无法显示轮廓的bug
		glStencilFunc(GL_NOTEQUAL, 1, 0xFF);	// 当目标像素的模板值不等于1时，通过测试
		glStencilMask(0x00);					// 禁止写入模板值
		glDepthMask(GL_FALSE);					//禁止深度写入
		Shader_Color->Enable();
		view = runtimeRegistry.GetRegistry().view<TransformComponent, MeshFilter, MeshRender, Identification>();
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
		glDepthMask(GL_TRUE);						// 恢复深度写入

		// physics debug pass
		if (ENGINE_CORE::CoreEngineData::GetInstance().RenderCollidersEnabled())
		{
			auto& physicsWorld = runtimeRegistry.GetContext<std::shared_ptr<rp3d::PhysicsWorld>>();
			auto& physicsDebugger = physicsWorld->getDebugRenderer();

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

	void RenderSystem::DeferredRenderPipeline(
		std::shared_ptr<ENGINE_RENDERING::Camera3D> camera, ENGINE_CORE::ECS::Registry& runtimeRegistry,
		std::shared_ptr<ENGINE_RENDERING::Framebuffer> intermediateGB, 
		std::shared_ptr<ENGINE_RENDERING::Framebuffer> ssaoFB,
		std::shared_ptr<ENGINE_RENDERING::Framebuffer> ssaoBlurFB,
		std::shared_ptr<ENGINE_RENDERING::Framebuffer> finalOutputFB)
	{
		Prepare_Pass(camera, runtimeRegistry);

		Shadow_Pass(runtimeRegistry);

		// 几何pass
		intermediateGB->Bind();
		glDisable(GL_BLEND);					// 关闭混合
		glEnable(GL_STENCIL_TEST);				// 开启模板测试
		// glCullFace(GL_BACK);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);	// 保证 Position 为 0
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glViewport(0, 0, intermediateGB->Width(), intermediateGB->Height());
		Geometry_Pass(camera, runtimeRegistry);
		intermediateGB->Unbind();

		// ssao pass
		ssaoFB->Bind();
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glViewport(0, 0, ssaoFB->Width(), ssaoFB->Height());
		SSAO_Pass(camera, runtimeRegistry, intermediateGB);
		ssaoFB->Unbind();

		// ssao模糊pass
		ssaoBlurFB->Bind();
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glViewport(0, 0, ssaoBlurFB->Width(), ssaoBlurFB->Height());
		SSAOBlur_Pass(camera, runtimeRegistry, ssaoFB);
		ssaoBlurFB->Unbind();

		// 光照pass
		finalOutputFB->Bind();
		glDisable(GL_STENCIL_TEST);				// 光照计算不需要模板
		glViewport(0, 0, finalOutputFB->Width(), finalOutputFB->Height());
		glClearColor(0.f, 0.f, 0.f, 1.f);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		Lighting_Pass(camera, runtimeRegistry, intermediateGB, ssaoBlurFB);
		finalOutputFB->Unbind();

		// 后处理pass（点光源+模板测试+天空盒）
		glBindFramebuffer(GL_READ_FRAMEBUFFER, intermediateGB->GetFboID());
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, finalOutputFB->GetFboID());
		glBlitFramebuffer(0, 0, intermediateGB->Width(), intermediateGB->Height(),
			0, 0, finalOutputFB->Width(), finalOutputFB->Height(),
			GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		finalOutputFB->Bind();
		Postprocess_Pass(camera, runtimeRegistry);
		finalOutputFB->Unbind();

		intermediateGB->CheckResize();
		ssaoFB->CheckResize();
		ssaoBlurFB->CheckResize();
		finalOutputFB->CheckResize();
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
			for (int mesh_index = 0; mesh_index < meshes.size(); mesh_index++)
			{
				ENGINE_CORE::ECS::Material& cur_material = meshR.GetMaterial(mesh_index);

				bool emptyAlbedo = cur_material.m_textures.find("albedo")->second.empty();
				bool textureBug = (cur_material.m_useTexture == true) && (emptyAlbedo);

				std::string shaderName = cur_material.shadingModel;

				Shader_Gbuffer->SetUniformMat4("model", model);
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
							glActiveTexture(GL_TEXTURE0 + (GLenum)slot_index); // 按索引自动分配纹理单元
							auto tex = assetManager.GetTexture(it->second);
							if (tex) {
								glBindTexture(GL_TEXTURE_2D, tex->GetID());
								Shader_Gbuffer->SetUniformInt(slot.shaderSampler, (int)slot_index);
							}
						}
					}
				}
				meshes[mesh_index].Draw();
			}
		}
	}

	void RenderSystem::SSAO_Pass(std::shared_ptr<ENGINE_RENDERING::Camera3D> camera, ENGINE_CORE::ECS::Registry& runtimeRegistry, std::shared_ptr<ENGINE_RENDERING::Framebuffer> intermediateGB)
	{
		auto& mainRegistry = MAIN_REGISTRY();

		auto& assetManager = mainRegistry.GetAssetManager();
		auto Shader_SSAO = assetManager.GetShader("defer_SSAO");
		if (Shader_SSAO->ShaderProgramID() == 0)
		{
			ENGINE_ERROR("Shader has not been set correctly!");
			return;
		}
		auto noise_texture = assetManager.GetTexture("ssaoNoise");

		Shader_SSAO->Enable();
		Shader_SSAO->SetUniformVec2("screenSize", glm::vec2(intermediateGB->Width(), intermediateGB->Height()));

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, intermediateGB->GetTextureID(0));
		Shader_SSAO->SetUniformInt("gPosition", 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, intermediateGB->GetTextureID(1));
		Shader_SSAO->SetUniformInt("gNormal", 1);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, noise_texture->GetID());
		Shader_SSAO->SetUniformInt("texNoise", 2);

		const std::vector<Mesh>& quad = assetManager.GetModel("quad")->GetMeshes();
		quad[0].Draw();
	}

	void RenderSystem::SSAOBlur_Pass(std::shared_ptr<ENGINE_RENDERING::Camera3D> camera, ENGINE_CORE::ECS::Registry& runtimeRegistry, std::shared_ptr<ENGINE_RENDERING::Framebuffer> ssaoFB)
	{
		auto& mainRegistry = MAIN_REGISTRY();

		auto& assetManager = mainRegistry.GetAssetManager();
		auto Shader_SSAOBlur = assetManager.GetShader("defer_SSAOBlur");
		if (Shader_SSAOBlur->ShaderProgramID() == 0)
		{
			ENGINE_ERROR("Shader has not been set correctly!");
			return;
		}

		Shader_SSAOBlur->Enable();
		
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, ssaoFB->GetTextureID(0));
		Shader_SSAOBlur->SetUniformInt("ssaoInput", 0);

		const std::vector<Mesh>& quad = assetManager.GetModel("quad")->GetMeshes();
		quad[0].Draw();
	}

	void RenderSystem::Lighting_Pass(
		std::shared_ptr<ENGINE_RENDERING::Camera3D> camera, ENGINE_CORE::ECS::Registry& runtimeRegistry, 
		std::shared_ptr<ENGINE_RENDERING::Framebuffer> intermediateGB, std::shared_ptr<ENGINE_RENDERING::Framebuffer> ssaoBlurFB)
	{
		auto& mainRegistry = MAIN_REGISTRY();

		auto& lightSystem = mainRegistry.GetContext<std::shared_ptr<ENGINE_CORE::Systems::LightSystem>>();
		auto& dirLightData = lightSystem->GetDirLightData();
		auto& pointLightData = lightSystem->GetPointLightData();

		auto& bufferManager = mainRegistry.GetBufferManager();
		const auto& map_FBO = bufferManager.GetAllFBO();

		auto& assetManager = mainRegistry.GetAssetManager();
		auto skybox_texture = assetManager.GetTexture("skybox");
		auto Shader_Lighting = assetManager.GetShader("defer_Lighting");
		if (Shader_Lighting->ShaderProgramID() == 0)
		{
			ENGINE_ERROR("Shader has not been set correctly!");
			return;
		}

		Shader_Lighting->Enable();

		Shader_Lighting->SetUniformFloat("far_plane", 100.0f);
		Shader_Lighting->SetUniformVec3("viewPos", camera->GetPosition());

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, intermediateGB->GetTextureID(0));
		Shader_Lighting->SetUniformInt("gPosition", 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, intermediateGB->GetTextureID(1));
		Shader_Lighting->SetUniformInt("gNormal", 1);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, intermediateGB->GetTextureID(2));
		Shader_Lighting->SetUniformInt("gAlbedo", 2);

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, intermediateGB->GetTextureID(3));
		Shader_Lighting->SetUniformInt("gMRA", 3);

		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, ssaoBlurFB->GetTextureID(0));
		Shader_Lighting->SetUniformInt("ssao", 4);

		glActiveTexture(GL_TEXTURE10);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture->GetID());
		Shader_Lighting->SetUniformInt("skybox", 10);

		auto& inputManager = ENGINE_CORE::INPUTS::InputManager::GetInstance();
		auto& keyboard = inputManager.GetKeyBoard();
		Shader_Lighting->SetUniformBool("use_SSAO", keyboard.IsKeyPressed(ENGINE_KEY_2));

		// set direction light shadowMap
		for (int dir_light_index = 0; dir_light_index < lightSystem->GetMaxDirLights(); dir_light_index++)
		{
			std::string key = "shadow_map_" + std::to_string(dir_light_index);
			auto it = map_FBO.find(key);
			if (it != map_FBO.end())
			{
				int texUnit = 11 + dir_light_index; // 纹理单元 11, 12, 13, 14
				glActiveTexture(GL_TEXTURE0 + texUnit);
				glBindTexture(GL_TEXTURE_2D, it->second->GetTextureID());
				Shader_Lighting->SetUniformInt("shadowMaps[" + std::to_string(dir_light_index) + "]", texUnit);
			}
		}

		// set point light shadowCubemap
		int cubeMapBaseUnit = 11 + lightSystem->GetMaxDirLights(); // = 15
		for (int point_light_index = 0; point_light_index < lightSystem->GetMaxPointLights(); point_light_index++)
		{
			std::string key = "shadow_cubemap_" + std::to_string(point_light_index);
			auto it = map_FBO.find(key);
			if (it != map_FBO.end())
			{
				int texUnit = cubeMapBaseUnit + point_light_index; // 15, 16, 17, 18
				glActiveTexture(GL_TEXTURE0 + texUnit);
				glBindTexture(GL_TEXTURE_CUBE_MAP, it->second->GetTextureID());
				Shader_Lighting->SetUniformInt("shadowCubeMap[" + std::to_string(point_light_index) + "]", texUnit);
			}
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

		auto& assetManager = mainRegistry.GetAssetManager();
		const std::vector<Mesh>& sphere = assetManager.GetModel("sphere")->GetMeshes();
		const std::vector<Mesh>& skybox = assetManager.GetModel("skybox")->GetMeshes();
		auto skybox_texture = assetManager.GetTexture("skybox");
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

		// 绘制点光源
		Shader_Color->Enable();
		for (int point_light_index = 0; point_light_index < lightSystem->GetActivatedPointLights(); point_light_index++)
		{
			auto point_light = pointLightData[point_light_index];
			// TODO: check point_light.render to decide if render
			glm::mat4 light_sphere_model = glm::mat4(1.0f);
			light_sphere_model = glm::translate(light_sphere_model, glm::vec3(point_light.position));
			light_sphere_model = glm::scale(light_sphere_model, glm::vec3(0.25));
			Shader_Color->SetUniformMat4("model", light_sphere_model);
			Shader_Color->SetUniformVec3("color", glm::vec3(1.0f));
			Shader_Color->SetUniformBool("outline", false);
			sphere[0].Draw();
		}

		// 绘制方向光
		for (int dir_light_index = 0; dir_light_index < lightSystem->GetActivatedDirLights(); dir_light_index++)
		{
			auto dir_light = dirLightData[dir_light_index];
			glm::vec3 pos = glm::vec3(0.0f) - glm::vec3((dir_light.direction * 10.0f));
			glm::mat4 light_sphere_model = glm::mat4(1.0f);
			light_sphere_model = glm::translate(light_sphere_model, pos);
			light_sphere_model = glm::scale(light_sphere_model, glm::vec3(0.25));
			Shader_Color->SetUniformMat4("model", light_sphere_model);
			Shader_Color->SetUniformVec3("color", glm::vec3(1.0,1.0,0.0));
			Shader_Color->SetUniformBool("outline", false);
			sphere[0].Draw();
		}

		// 绘制天空盒
		glDepthFunc(GL_LEQUAL);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture->GetID());
		glm::mat4 viewMatrix = camera->GetViewMatrix();
		glm::mat4 PerspectiveMatrix = glm::perspective(glm::radians(camera->Zoom), (float)camera->GetWidth() / (float)camera->GetHeight(), 0.1f, 100.0f);
		Shader_Skybox->Enable();
		Shader_Skybox->SetUniformMat4("model", glm::mat4(1.0f));
		Shader_Skybox->SetUniformMat4("view", glm::mat4(glm::mat3(viewMatrix)));	//移除观察矩阵中的位移
		Shader_Skybox->SetUniformMat4("projection", PerspectiveMatrix);
		Shader_Skybox->SetUniformInt("skybox", 0);
		skybox[0].Draw();
		glDepthFunc(GL_LESS);

		// 模板测试
		glEnable(GL_STENCIL_TEST);					// 开启模板测试
		glStencilFunc(GL_NOTEQUAL, 1, 0xFF);		// 当目标像素的模板值不等于1时，通过测试
		glStencilMask(0x00);						// 禁止写入模板值
		glDepthMask(GL_FALSE);						//禁止深度写入
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
		glDepthMask(GL_TRUE);						// 恢复深度写入
		glDisable(GL_STENCIL_TEST);					// 关闭模板测试

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
}

