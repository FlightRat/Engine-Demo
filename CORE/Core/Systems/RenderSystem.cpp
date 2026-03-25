#include "RenderSystem.h"
#include<glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include<glm/gtx/quaternion.hpp>
#include<glm/gtc/quaternion.hpp>
#include<Rendering/Core/Camera3D.h>
#include<Rendering/Essentials/Shader.h>
#include<Rendering/Essentials/Lights.h>
#include<Rendering/Essentials/TextureCommon.h>
#include<Rendering/Buffers/Framebuffer.h>
#include<Rendering/Buffers/ShadowMap.h>
#include<Rendering/Buffers/render_uniformbuffers.h>
#include<Rendering/Buffers/render_shadowmaps.h>
#include<Rendering/Buffers/Gbuffer.h>
#include<Logger/Logger.h>
#include<../CORE/Core/ECS/MainRegistry.h>
#include "../CORE/Core/Systems/LightSystem.h"
#include "../ECS/Entity.h"
#include "../Resources/AssetManager.h"
#include "../ECS/Components/TransformComponent.h"
#include "../ECS/Components/Identification.h"
#include "../ECS/Components/MeshFilter.h"
#include "../ECS/Components/MeshRender.h"
#include "../ECS/Components/LightComponent.h"
#include "../CoreUtilities/CoreEngineData.h"

namespace {
	struct TextureSlot {
		const char* key;            // 材质 Map 中的 key (如 "diffuse")
		const char* useUniform;     // Shader bool 开关 (如 "material.useDiffuse")
		const char* samplerUniform; // Shader sampler2D 名字 (如 "material.diffuse")
		int unitIndex;              // 纹理单元 (0, 1)
	};

	// 使用 constexpr 让它在编译期就确定，性能最高
	constexpr std::array<TextureSlot, 4> TEXTURE_SLOTS = { {
		{ "diffuse",  "material.useDiffuse",  "material.diffuse",    0},
		{ "specular", "material.useSpecular", "material.specular",   1 },
		{ "normal",   "material.useNormal",   "material.normal",     2 },
		{ "reflect",  "material.useReflect",  "material.reflection", 3 }
	} };
}

using namespace ENGINE_CORE::ECS;
using namespace ENGINE_RENDERING;
using namespace ENGINE_RESOURCES;

namespace ENGINE_CORE::Systems {
	RenderSystem::RenderSystem()
	{
		glGenVertexArrays(1, &m_DebugVAO);
		glGenBuffers(1, &m_DebugVBO);
	}

	void RenderSystem::ForwardRenderPipeline(std::shared_ptr<ENGINE_RENDERING::Camera3D> camera, ENGINE_CORE::ECS::Registry& runtimeRegistry, std::shared_ptr<ENGINE_RENDERING::Framebuffer> finalOutputFB)
	{
		//auto& mainRegistry = MAIN_REGISTRY();
		//auto& RenderShadowMap = mainRegistry.GetContext<std::shared_ptr<ENGINE_RENDERING::RenderShadowMaps>>();
		//auto& shadowMap = RenderShadowMap->mapShadowmaps["shadow_map"];
		//auto& shadowCubemap_1 = RenderShadowMap->mapShadowmaps["shadow_cubemap_1"];
		//auto& shadowCubemap_2 = RenderShadowMap->mapShadowmaps["shadow_cubemap_2"];
		//auto& shadowCubemap_3 = RenderShadowMap->mapShadowmaps["shadow_cubemap_3"];
		//auto& shadowCubemap_4 = RenderShadowMap->mapShadowmaps["shadow_cubemap_4"];
		//if (shadowMap->Width() != finalOutputFB->Width() || shadowMap->Height() != finalOutputFB->Height())
		//{
		//	shadowMap->Resize(static_cast<int>(finalOutputFB->Width()), static_cast<int>(finalOutputFB->Height()));
		//	shadowCubemap_1->Resize(static_cast<int>(finalOutputFB->Width()), static_cast<int>(finalOutputFB->Height()));
		//	shadowCubemap_2->Resize(static_cast<int>(finalOutputFB->Width()), static_cast<int>(finalOutputFB->Height()));
		//	shadowCubemap_3->Resize(static_cast<int>(finalOutputFB->Width()), static_cast<int>(finalOutputFB->Height()));
		//	shadowCubemap_4->Resize(static_cast<int>(finalOutputFB->Width()), static_cast<int>(finalOutputFB->Height()));
		//}

		Param_Pass(camera, runtimeRegistry);

		Shadow_Pass(runtimeRegistry);

		finalOutputFB->Bind();
		glViewport(0, 0, finalOutputFB->Width(), finalOutputFB->Height());
		glClearColor(0.f, 0.f, 0.f, 1.f);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		Forward_Pass(camera, runtimeRegistry);
		finalOutputFB->Unbind();
		finalOutputFB->CheckResize();
	}

	void RenderSystem::Param_Pass(std::shared_ptr<ENGINE_RENDERING::Camera3D> camera, ENGINE_CORE::ECS::Registry& runtimeRegistry)
	{
		auto& mainRegistry = MAIN_REGISTRY();
		auto& lightSystem = mainRegistry.GetContext<std::shared_ptr<ENGINE_CORE::Systems::LightSystem>>();
		auto& assetManager = mainRegistry.GetAssetManager();
		auto skybox_texture = assetManager.GetTexture("skybox");

		// get shaders
		auto mainShader = assetManager.GetShader("mainShader");
		if (mainShader->ShaderProgramID() == 0)
		{
			ENGINE_ERROR("Shader has not been set correctly!");
			return;
		}
		auto colorShader = assetManager.GetShader("colorShader");
		if (colorShader->ShaderProgramID() == 0)
		{
			ENGINE_ERROR("Shader has not been set correctly!");
			return;
		}
		auto colliderShader = assetManager.GetShader("colliderShader");
		if (colliderShader->ShaderProgramID() == 0)
		{
			ENGINE_ERROR("Shader has not been set correctly!");
			return;
		}

		// bind uniform block index
		mainShader->BindUniformBlock("Matrices", 0);
		mainShader->BindUniformBlock("DirLights", 1);
		mainShader->BindUniformBlock("PointLights", 2);
		colorShader->BindUniformBlock("Matrices", 0);
		colliderShader->BindUniformBlock("Matrices", 0);

		auto defer_gbuffer = assetManager.GetShader("deferGbuffer");
		if (defer_gbuffer->ShaderProgramID() == 0)
		{
			ENGINE_ERROR("Shader has not been set correctly!");
			return;
		}
		auto defer_lighting = assetManager.GetShader("deferLighting");
		if (defer_lighting->ShaderProgramID() == 0)
		{
			ENGINE_ERROR("Shader has not been set correctly!");
			return;
		}
		defer_gbuffer->BindUniformBlock("Matrices", 0);
		defer_lighting->BindUniformBlock("DirLights", 1);
		defer_lighting->BindUniformBlock("PointLights", 2);

		// uniform block buffers
		auto& RenderUniformbuffers = mainRegistry.GetContext<std::shared_ptr<ENGINE_RENDERING::RenderUniformbuffers>>();
		auto& matrixUbo = RenderUniformbuffers->mapUniformbuffers["matrix"];
		auto& dirLightsUbo = RenderUniformbuffers->mapUniformbuffers["DirLights"];
		auto& pointLightsUbo = RenderUniformbuffers->mapUniformbuffers["PointLights"];

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
		auto depthShader = assetManager.GetShader("depthShader");
		auto depthCubeShader = assetManager.GetShader("depthCubeShader");
		
		auto& lightSystem = mainRegistry.GetContext<std::shared_ptr<ENGINE_CORE::Systems::LightSystem>>();
		auto& dirLightData = lightSystem->GetDirLightData();
		auto& pointLightData = lightSystem->GetPointLightData();

		auto& RenderShadowMap = mainRegistry.GetContext<std::shared_ptr<ENGINE_RENDERING::RenderShadowMaps>>();

		// 方向光
		depthShader->Enable();
		for (int dir_light_index = 0; dir_light_index < dirLightData.size(); dir_light_index++)
		{
			auto& shadowMap = RenderShadowMap->mapShadowmaps["shadow_map_" + std::to_string(dir_light_index)];
			shadowMap->Bind();

			glViewport(0, 0, shadowMap->Width(), shadowMap->Height());
			glClearColor(0.f, 0.f, 0.f, 1.f);
			glClear(GL_DEPTH_BUFFER_BIT);

			auto dirLight = dirLightData[dir_light_index];

			// calculate lightSpaceMatrix
			glm::vec3 dirLightDir = dirLight.direction;
			glm::vec3 dirLightPos = glm::vec3(0.0f) - (dirLightDir * 10.0f);
			float near_plane = 0.1f, far_plane = 50.0f;
			glm::vec3 upVector = glm::abs(dirLightDir.y) > 0.99f
				? glm::vec3(0.0f, 0.0f, 1.0f)
				: glm::vec3(0.0f, 1.0f, 0.0f);
			glm::mat4 lightViewMatrix = glm::lookAt(dirLightPos, glm::vec3(0.0f), upVector);
			glm::mat4 lightProjectionMatrix = glm::ortho(-40.0f, 40.0f, -40.0f, 40.0f, near_plane, far_plane);
			glm::mat4 lightSpaceMatrix = lightProjectionMatrix * lightViewMatrix;
			shadowMap->SetLightSpaceMatrix(lightSpaceMatrix);
			depthShader->SetUniformMat4("lightSpaceMatrix", lightSpaceMatrix);

			// render scene
			auto view = runtimeRegistry.GetRegistry().view<TransformComponent, MeshFilter, MeshRender, Identification>();
			for (auto [entity, transform, meshF, meshR, id] : view.each())
			{
				if (!meshR.shouldRender)
					continue;

				glm::mat4 model = CalculateModelMatrix(transform, id, runtimeRegistry);
				depthShader->SetUniformMat4("model", model);
				const std::vector<Mesh>& meshes = assetManager.GetModel(meshF.mesh)->GetMeshes();
				for (size_t meshIdx = 0; meshIdx < meshes.size(); meshIdx++)
					meshes[meshIdx].Draw();
			}

			shadowMap->Unbind();
			// shadowMap->CheckResize(); // no need

		}

		// 点光源
		depthCubeShader->Enable();
		for (int lightIdx = 0; lightIdx < lightSystem->GetActivatedPointLights(); lightIdx++)
		{	
			auto& shadowCubemap = RenderShadowMap->mapShadowmaps["shadow_cubemap_" + std::to_string(lightIdx)];
			shadowCubemap->Bind();

			glViewport(0, 0, shadowCubemap->Width(), shadowCubemap->Height());
			glClearColor(0.f, 0.f, 0.f, 1.f);
			glClear(GL_DEPTH_BUFFER_BIT);

			auto& point_light = pointLightData[lightIdx];
			glm::vec3 point_light_pos = glm::vec3(point_light.position);

			// light space matrix
			GLfloat aspect = (GLfloat)shadowCubemap->Width() / (GLfloat)shadowCubemap->Height();
			float near_plane = 0.1f, far_plane = 50.0f;
			glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), aspect, near_plane, far_plane);
			std::vector<glm::mat4> shadowTransforms;
			shadowTransforms.push_back(shadowProj * glm::lookAt(point_light_pos, point_light_pos + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
			shadowTransforms.push_back(shadowProj * glm::lookAt(point_light_pos, point_light_pos + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
			shadowTransforms.push_back(shadowProj * glm::lookAt(point_light_pos, point_light_pos + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)));
			shadowTransforms.push_back(shadowProj * glm::lookAt(point_light_pos, point_light_pos + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0)));
			shadowTransforms.push_back(shadowProj * glm::lookAt(point_light_pos, point_light_pos + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0)));
			shadowTransforms.push_back(shadowProj * glm::lookAt(point_light_pos, point_light_pos + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0)));
			depthCubeShader->SetUniformFloat("far_plane", far_plane);
			depthCubeShader->SetUniformVec3("lightPos", point_light_pos);
			for (int face = 0; face < 6; ++face)
				depthCubeShader->SetUniformMat4("shadowMatrices[" + std::to_string(face) + "]", shadowTransforms[face]);

			// render scene
			auto view = runtimeRegistry.GetRegistry().view<TransformComponent, MeshFilter, MeshRender, Identification>();
			for (auto [entity, transform, meshF, meshR, id] : view.each())
			{
				if (!meshR.shouldRender)
					continue;

				glm::mat4 model = CalculateModelMatrix(transform, id, runtimeRegistry);
				depthCubeShader->SetUniformMat4("model", model);
				const std::vector<Mesh>& meshes = assetManager.GetModel(meshF.mesh)->GetMeshes();
				for (size_t meshIdx = 0; meshIdx < meshes.size(); meshIdx++)
					meshes[meshIdx].Draw();
			}

			shadowCubemap->Unbind();
			// shadowCubemap->CheckResize(); no need
		}
	}

	void RenderSystem::Forward_Pass(std::shared_ptr<ENGINE_RENDERING::Camera3D> camera, ENGINE_CORE::ECS::Registry& runtimeRegistry)
	{
		auto& mainRegistry = MAIN_REGISTRY();
		auto& assetManager = mainRegistry.GetAssetManager();
		auto skybox_texture = assetManager.GetTexture("skybox");

		auto& RenderShadowMap = mainRegistry.GetContext<std::shared_ptr<ENGINE_RENDERING::RenderShadowMaps>>();

		// get shaders
		auto mainShader = assetManager.GetShader("mainShader");
		if (mainShader->ShaderProgramID() == 0)
		{
			ENGINE_ERROR("Shader has not been set correctly!");
			return;
		}
		auto colorShader = assetManager.GetShader("colorShader");
		if (colorShader->ShaderProgramID() == 0)
		{
			ENGINE_ERROR("Shader has not been set correctly!");
			return;
		}
		auto skyboxShader = assetManager.GetShader("skyboxShader");
		if (skyboxShader->ShaderProgramID() == 0)
		{
			ENGINE_ERROR("Shader has not been set correctly!");
			return;
		}
		auto colliderShader = assetManager.GetShader("colliderShader");
		if (colliderShader->ShaderProgramID() == 0)
		{
			ENGINE_ERROR("Shader has not been set correctly!");
			return;
		}

		// render point light sphere
		colorShader->Enable();
		auto& lightSystem = mainRegistry.GetContext<std::shared_ptr<ENGINE_CORE::Systems::LightSystem>>();
		auto& pointLightData = lightSystem->GetPointLightData();
		const std::vector<Mesh>& sphere = assetManager.GetModel("sphere")->GetMeshes();
		for (int point_light_index = 0; point_light_index < lightSystem->GetActivatedPointLights(); point_light_index++)
		{
			auto point_light = pointLightData[point_light_index];
			// TODO: check point_light.render to decide if render
			glm::mat4 light_sphere_model = glm::mat4(1.0f);
			light_sphere_model = glm::translate(light_sphere_model, glm::vec3(point_light.position));
			light_sphere_model = glm::scale(light_sphere_model, glm::vec3(0.25));
			colorShader->SetUniformMat4("model", light_sphere_model);
			colorShader->SetUniformVec3("color", glm::vec3(point_light.diffuse));
			colorShader->SetUniformBool("outline", false);
			sphere[0].Draw();
		}

		// render object
		mainShader->Enable();
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

			const std::vector<Mesh>& meshes = assetManager.GetModel(meshF.mesh)->GetMeshes();
			if (meshF.changed || meshR.CheckMaterialEmpty())
			{
				meshF.changed = false;
				meshR.ResetMaterial(meshes);
			}

			glm::mat4 model = CalculateModelMatrix(transform, id, runtimeRegistry);
			for (int mesh_index = 0; mesh_index < meshes.size(); mesh_index++)
			{
				ENGINE_CORE::ECS::Material& cur_material = meshR.GetMaterial(mesh_index);

				bool emptyDiffuse = cur_material.m_textures.find("diffuse")->second.empty();
				bool textureBug = (cur_material.m_useTexture == true) && (emptyDiffuse);

				std::string shaderName = cur_material.shaderName;

				mainShader->Enable();	// NOTE: now the shader is fixed
				mainShader->SetUniformMat4("model", model);
				mainShader->SetUniformVec3("viewPos", camera->GetPosition());
				mainShader->SetUniformBool("bug", textureBug);
				mainShader->SetUniformBool("flipUV", meshR.flipUV);
				mainShader->SetUniformVec4("material.color", cur_material.color);
				mainShader->SetUniformFloat("material.shininess", cur_material.shininess);
				mainShader->SetUniformBool("useTexture", cur_material.m_useTexture);

				mainShader->SetUniformFloat("far_plane", 50.0f);

				glActiveTexture(GL_TEXTURE10);
				glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture->GetID());
				mainShader->SetUniformInt("skybox", 10);

				// set direction light shadowMap
				for (int dir_light_index = 0; dir_light_index < lightSystem->GetMaxDirLights(); dir_light_index++)
				{
					std::string key = "shadow_map_" + std::to_string(dir_light_index);
					auto it = RenderShadowMap->mapShadowmaps.find(key);
					if (it != RenderShadowMap->mapShadowmaps.end())
					{
						int texUnit = 11 + dir_light_index; // 纹理单元 11, 12, 13, 14
						glActiveTexture(GL_TEXTURE0 + texUnit);
						glBindTexture(GL_TEXTURE_2D, it->second->GetTextureID());
						mainShader->SetUniformInt("shadowMaps[" + std::to_string(dir_light_index) + "]", texUnit);
						mainShader->SetUniformMat4("lightSpaceMatrices[" + std::to_string(dir_light_index) + "]", it->second->GetLightSpaceMatrix());
					}
				}

				// set point light shadowCubemap
				int cubeMapBaseUnit = 11 + lightSystem->GetMaxDirLights(); // = 15
				for (int point_light_index = 0; point_light_index < lightSystem->GetMaxPointLights(); point_light_index++)
				{
					std::string key = "shadow_cubemap_" + std::to_string(point_light_index);
					auto it = RenderShadowMap->mapShadowmaps.find(key);
					if (it != RenderShadowMap->mapShadowmaps.end())
					{
						int texUnit = cubeMapBaseUnit + point_light_index; // 15, 16, 17, 18
						glActiveTexture(GL_TEXTURE0 + texUnit);
						glBindTexture(GL_TEXTURE_CUBE_MAP, it->second->GetTextureID());
						mainShader->SetUniformInt("shadowCubeMap[" + std::to_string(point_light_index) + "]", texUnit);
					}
				}

				// set uniform textures
				if (cur_material.m_useTexture) {
					const auto& slots = TextureRegistry::GetSlots();
					for (size_t slot_index = 0; slot_index < slots.size(); ++slot_index) {
						const auto& slot = slots[slot_index];

						auto it = cur_material.m_textures.find(slot.key);
						bool hasTexture = (it != cur_material.m_textures.end() && !it->second.empty());

						mainShader->SetUniformBool(slot.shaderFlag, hasTexture);

						if (hasTexture) {
							glActiveTexture(GL_TEXTURE0 + (GLenum)slot_index); // 按索引自动分配纹理单元
							auto tex = assetManager.GetTexture(it->second);
							if (tex) {
								glBindTexture(GL_TEXTURE_2D, tex->GetID());
								mainShader->SetUniformInt(slot.shaderSampler, (int)slot_index);
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
		skyboxShader->Enable();
		skyboxShader->SetUniformMat4("model", glm::mat4(1.0f));
		skyboxShader->SetUniformMat4("view", glm::mat4(glm::mat3(viewMatrix)));	//移除观察矩阵中的位移
		skyboxShader->SetUniformMat4("projection", PerspectiveMatrix);
		skyboxShader->SetUniformInt("skybox", 0);
		const std::vector<Mesh>& skybox = assetManager.GetModel("skybox")->GetMeshes();
		skybox[0].Draw();
		glDepthFunc(GL_LESS);

		//模板测试 TODO:修复scale相同导致无法显示轮廓的bug
		glStencilFunc(GL_NOTEQUAL, 1, 0xFF);	// 当目标像素的模板值不等于1时，通过测试
		glStencilMask(0x00);					// 禁止写入模板值
		glDepthMask(GL_FALSE);					//禁止深度写入
		colorShader->Enable();
		view = runtimeRegistry.GetRegistry().view<TransformComponent, MeshFilter, MeshRender, Identification>();
		for (auto [entity, transform, meshF, meshR, id] : view.each())
		{
			if (!meshR.shouldRender || !id.selected)
			{
				continue;
			}

			const std::vector<Mesh>& meshes = assetManager.GetModel(meshF.mesh)->GetMeshes();
			if (meshF.changed || meshR.CheckMaterialEmpty())
			{
				meshF.changed = false;
				meshR.ResetMaterial(meshes);
			}

			glm::mat4 model = CalculateModelMatrix(transform, id, runtimeRegistry);
			for (int mesh_index = 0; mesh_index < meshes.size(); mesh_index++)
			{
				ENGINE_CORE::ECS::Material& cur_material = meshR.GetMaterial(mesh_index);

				bool emptyDiffuse = cur_material.m_textures.find("diffuse")->second.empty();
				bool textureBug = (cur_material.m_useTexture == true) && (emptyDiffuse);

				std::string shaderName = cur_material.shaderName;

				colorShader->Enable();	// NOTE: now the shader is fixed
				colorShader->SetUniformMat4("model", model);
				colorShader->SetUniformVec3("color", glm::vec3(1.0f, 1.0f, 0.0f));
				colorShader->SetUniformBool("outline", true);

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

			colliderShader->Enable();
			colliderShader->SetUniformMat4("model", glm::mat4(1.0f));

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

	void RenderSystem::DeferredRenderPipeline(std::shared_ptr<ENGINE_RENDERING::Camera3D> camera, ENGINE_CORE::ECS::Registry& runtimeRegistry, std::shared_ptr<ENGINE_RENDERING::Framebuffer> finalOutputFB)
	{
		auto& mainRegistry = MAIN_REGISTRY();
		auto& Gbuffer = mainRegistry.GetContext<std::shared_ptr<ENGINE_RENDERING::Gbuffer>>();
		if (Gbuffer->Width() != finalOutputFB->Width() || Gbuffer->Height() != finalOutputFB->Height())
		{
			Gbuffer->Resize(static_cast<int>(finalOutputFB->Width()), static_cast<int>(finalOutputFB->Height()));
		}

		Param_Pass(camera, runtimeRegistry);

		Shadow_Pass(runtimeRegistry);

		GeometryPass(camera, runtimeRegistry);

		finalOutputFB->Bind();
		glViewport(0, 0, finalOutputFB->Width(), finalOutputFB->Height());
		glClearColor(0.f, 0.f, 0.f, 1.f);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		LightingPass(camera, runtimeRegistry);
		finalOutputFB->Unbind();
		finalOutputFB->CheckResize();
	}

	void RenderSystem::GeometryPass(std::shared_ptr<ENGINE_RENDERING::Camera3D> camera, ENGINE_CORE::ECS::Registry& runtimeRegistry)
	{
		auto& mainRegistry = MAIN_REGISTRY();

		auto& assetManager = mainRegistry.GetAssetManager();
		auto gbufferShader = assetManager.GetShader("deferGbuffer");

		auto& Gbuffer = mainRegistry.GetContext<std::shared_ptr<ENGINE_RENDERING::Gbuffer>>();
		Gbuffer->Bind();
		glDisable(GL_BLEND);         // <-- MUST DISABLE BLENDING
		glDisable(GL_STENCIL_TEST);  // <-- DISABLE STENCIL FOR STANDARD PASS
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // 保证 Position 为 0
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glViewport(0, 0, Gbuffer->Width(), Gbuffer->Height());

		// render object
		gbufferShader->Enable();
		auto view = runtimeRegistry.GetRegistry().view<TransformComponent, MeshFilter, MeshRender, Identification>();
		for (auto [entity, transform, meshF, meshR, id] : view.each())
		{
			if (!meshR.shouldRender)
			{
				continue;
			}
			//if (id.selected)
			//{
			//	glStencilFunc(GL_ALWAYS, 1, 0xFF);		// 总是通过模板测试，且ref为1
			//	glStencilMask(0xFF);					// 允许写入模板值
			//}
			//else
			//{
			//	glStencilMask(0x00);					// 禁止写入模板值
			//}

			const std::vector<Mesh>& meshes = assetManager.GetModel(meshF.mesh)->GetMeshes();
			if (meshF.changed || meshR.CheckMaterialEmpty())
			{
				meshF.changed = false;
				meshR.ResetMaterial(meshes);
			}

			glm::mat4 model = CalculateModelMatrix(transform, id, runtimeRegistry);
			for (int mesh_index = 0; mesh_index < meshes.size(); mesh_index++)
			{
				ENGINE_CORE::ECS::Material& cur_material = meshR.GetMaterial(mesh_index);

				bool emptyDiffuse = cur_material.m_textures.find("diffuse")->second.empty();
				bool textureBug = (cur_material.m_useTexture == true) && (emptyDiffuse);

				std::string shaderName = cur_material.shaderName;

				gbufferShader->SetUniformMat4("model", model);
				gbufferShader->SetUniformBool("bug", textureBug);
				gbufferShader->SetUniformBool("flipUV", meshR.flipUV);
				gbufferShader->SetUniformBool("useTexture", cur_material.m_useTexture);
				gbufferShader->SetUniformVec4("material.color", cur_material.color);
				//gbufferShader->SetUniformFloat("material.shininess", cur_material.shininess);

				// set uniform textures
				if (cur_material.m_useTexture) {
					const auto& slots = TextureRegistry::GetSlots();
					for (size_t slot_index = 0; slot_index < slots.size(); ++slot_index) {
						const auto& slot = slots[slot_index];

						auto it = cur_material.m_textures.find(slot.key);
						bool hasTexture = (it != cur_material.m_textures.end() && !it->second.empty());

						gbufferShader->SetUniformBool(slot.shaderFlag, hasTexture);

						if (hasTexture) {
							glActiveTexture(GL_TEXTURE0 + (GLenum)slot_index); // 按索引自动分配纹理单元
							auto tex = assetManager.GetTexture(it->second);
							if (tex) {
								glBindTexture(GL_TEXTURE_2D, tex->GetID());
								gbufferShader->SetUniformInt(slot.shaderSampler, (int)slot_index);
							}
						}
					}
				}
				meshes[mesh_index].Draw();
			}
		}
	
		Gbuffer->Unbind();
		Gbuffer->CheckResize();
	}

	void RenderSystem::LightingPass(std::shared_ptr<ENGINE_RENDERING::Camera3D> camera, ENGINE_CORE::ECS::Registry& runtimeRegistry)
	{
		auto& mainRegistry = MAIN_REGISTRY();

		auto& lightSystem = mainRegistry.GetContext<std::shared_ptr<ENGINE_CORE::Systems::LightSystem>>();
		auto& pointLightData = lightSystem->GetPointLightData();
		auto& dirLightData = lightSystem->GetDirLightData();

		auto& Gbuffer = mainRegistry.GetContext<std::shared_ptr<ENGINE_RENDERING::Gbuffer>>();
		auto& RenderShadowMap = mainRegistry.GetContext<std::shared_ptr<ENGINE_RENDERING::RenderShadowMaps>>();

		auto& assetManager = mainRegistry.GetAssetManager();
		auto skybox_texture = assetManager.GetTexture("skybox");
		auto gbufferShader = assetManager.GetShader("deferGbuffer");
		auto lightingShader = assetManager.GetShader("deferLighting");
	
		lightingShader->Enable();

		lightingShader->SetUniformFloat("far_plane", 50.0f);
		lightingShader->SetUniformVec3("viewPos", camera->GetPosition());

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Gbuffer->GetPosition());
		lightingShader->SetUniformInt("gPosition", 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, Gbuffer->GetNormal());
		lightingShader->SetUniformInt("gNormal", 1);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, Gbuffer->GetAlbedoSpec());
		lightingShader->SetUniformInt("gAlbedoSpec", 2);

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, Gbuffer->GetRefl());
		lightingShader->SetUniformInt("gRefl", 3);

		glActiveTexture(GL_TEXTURE10);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture->GetID());
		lightingShader->SetUniformInt("skybox", 10);

		// set direction light shadowMap
		for (int dir_light_index = 0; dir_light_index < lightSystem->GetMaxDirLights(); dir_light_index++)
		{
			std::string key = "shadow_map_" + std::to_string(dir_light_index);
			auto it = RenderShadowMap->mapShadowmaps.find(key);
			if (it != RenderShadowMap->mapShadowmaps.end())
			{
				int texUnit = 11 + dir_light_index; // 纹理单元 11, 12, 13, 14
				glActiveTexture(GL_TEXTURE0 + texUnit);
				glBindTexture(GL_TEXTURE_2D, it->second->GetTextureID());
				lightingShader->SetUniformInt("shadowMaps[" + std::to_string(dir_light_index) + "]", texUnit);
				lightingShader->SetUniformMat4("lightSpaceMatrices[" + std::to_string(dir_light_index) + "]", it->second->GetLightSpaceMatrix());
			}
		}

		// set point light shadowCubemap
		int cubeMapBaseUnit = 11 + lightSystem->GetMaxDirLights(); // = 15
		for (int point_light_index = 0; point_light_index < lightSystem->GetMaxPointLights(); point_light_index++)
		{
			std::string key = "shadow_cubemap_" + std::to_string(point_light_index);
			auto it = RenderShadowMap->mapShadowmaps.find(key);
			if (it != RenderShadowMap->mapShadowmaps.end())
			{
				int texUnit = cubeMapBaseUnit + point_light_index; // 15, 16, 17, 18
				glActiveTexture(GL_TEXTURE0 + texUnit);
				glBindTexture(GL_TEXTURE_CUBE_MAP, it->second->GetTextureID());
				lightingShader->SetUniformInt("shadowCubeMap[" + std::to_string(point_light_index) + "]", texUnit);
			}
		}

		glDisable(GL_DEPTH_TEST);
		const std::vector<Mesh>& gbuffer_quad = assetManager.GetModel("gbuffer_quad")->GetMeshes();
		gbuffer_quad[0].Draw();
		glEnable(GL_DEPTH_TEST);
	}
}

