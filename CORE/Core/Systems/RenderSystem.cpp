#include "RenderSystem.h"
#include<glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include<glm/gtx/quaternion.hpp>
#include<glm/gtc/quaternion.hpp>
#include<Rendering/Core/Camera3D.h>
#include<Rendering/Essentials/Shader.h>
#include<Rendering/Buffers/Framebuffer.h>
#include<Logger/Logger.h>
#include<../CORE/Core/ECS/MainRegistry.h>
#include "../ECS/Entity.h"
#include "../Resources/AssetManager.h"
#include "../ECS/Components/TransformComponent.h"
#include "../ECS/Components/Identification.h"
#include "../ECS/Components/MeshFilter.h"
#include "../ECS/Components/MeshRender.h"
#include "../ECS/Components/LightComponent.h"
#include "../CoreUtilities/CoreEngineData.h"

namespace {
	struct DirLight {
		glm::vec4 direction = glm::vec4(0.0f);
		glm::vec4 diffuse = glm::vec4(0.0f);
		glm::vec4 specular = glm::vec4(0.0f);
		glm::vec4 ambient = glm::vec4(0.0f);
	};

	struct PointLight {
		glm::vec4 position = glm::vec4(0.0f);
		glm::vec4 diffuse = glm::vec4(0.0f);
		glm::vec4 specular = glm::vec4(0.0f);
		glm::vec4 ambient = glm::vec4(0.0f);
		glm::vec4 attenuation = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
	};

	struct LightBlock {
		DirLight dirLight;
		PointLight pointLights[4];
	};

	struct TextureSlot {
		const char* key;            // 材质 Map 中的 key (如 "diffuse")
		const char* useUniform;     // Shader bool 开关 (如 "material.useDiffuse")
		const char* samplerUniform; // Shader sampler2D 名字 (如 "material.diffuse")
		int unitIndex;              // 纹理单元 (0, 1)
	};

	// 使用 constexpr 让它在编译期就确定，性能最高
	constexpr std::array<TextureSlot, 3> TEXTURE_SLOTS = { {
		{ "diffuse",  "material.useDiffuse",  "material.diffuse",    1 },
		{ "specular", "material.useSpecular", "material.specular",   2 },
		{ "reflect",  "material.useReflect",  "material.reflection", 3 },
		//{ "normal",   "material.useNormal",   "material.normal",     4 },
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

	void RenderSystem::Render(std::shared_ptr<ENGINE_RENDERING::Camera3D> camera, ENGINE_CORE::ECS::Registry& runtimeRegistry)
	{
		auto& mainRegistry = MAIN_REGISTRY();
		auto& assetManager = mainRegistry.GetAssetManager();
		auto skybox_texture = assetManager.GetTexture("skybox");

		// camera param
		auto viewMatrix = camera->GetViewMatrix();
		glm::mat4 orthoMatrix = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
		glm::mat4 PerspectiveMatrix = glm::perspective(glm::radians(camera->Zoom), (float)camera->GetWidth() / (float)camera->GetHeight(), 0.1f, 100.0f);
		// uniform block -- view/projction matrix
		unsigned int uboMatrices;
		glGenBuffers(1, &uboMatrices);
		glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
		glBindBufferRange(GL_UNIFORM_BUFFER, 0, uboMatrices, 0, 2 * sizeof(glm::mat4));
		glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(viewMatrix));
		glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(PerspectiveMatrix));
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		// lighting param
		LightBlock lightData;
		int activePointLights = 0;
		const int MAX_POINT_LIGHTS = 4;
		auto lightView = runtimeRegistry.GetRegistry().view<LightComponent>();
		for (auto [_, light] : lightView.each())
		{
			if (light.type == "direction_light")
			{
				// 设置方向光
				lightData.dirLight.direction = glm::vec4(light.direction, 1.0);
				lightData.dirLight.diffuse = glm::vec4(light.diffuse, 1.0);
				lightData.dirLight.specular = glm::vec4(light.specular, 1.0);
				lightData.dirLight.ambient = glm::vec4(light.ambient, 1.0);
			}
			else if (light.type == "point_light")
			{
				// 检查是否超过 Shader 允许的最大数量
				if (activePointLights < MAX_POINT_LIGHTS)
				{
					lightData.pointLights[activePointLights].position = glm::vec4(light.pos, 1.0);

					lightData.pointLights[activePointLights].diffuse = glm::vec4(light.diffuse, 1.0);
					lightData.pointLights[activePointLights].specular = glm::vec4(light.specular, 1.0);
					lightData.pointLights[activePointLights].ambient = glm::vec4(light.ambient, 1.0);

					lightData.pointLights[activePointLights].attenuation = glm::vec4(light.constant, light.linear, light.quadratic, 1.0);

					activePointLights++;
				}
				else
				{
					// 可选：打印警告，提示场景光源过多
					ENGINE_WARN("Too many point lights! limit is 4");
				}
			}
		}
		// uniform block -- lighting
		unsigned int uboLights;
		glGenBuffers(1, &uboLights);
		glBindBuffer(GL_UNIFORM_BUFFER, uboLights);
		glBindBufferRange(GL_UNIFORM_BUFFER, 1, uboLights, 0, sizeof(LightBlock));
		glBufferData(GL_UNIFORM_BUFFER, sizeof(LightBlock), NULL, GL_STATIC_DRAW); // GL_DYNAMIC_DRAW for changeing data, otherwise GL_STATIC_DRAW
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(LightBlock), &lightData);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

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

		// bind uniform block index
		mainShader->BindUniformBlock("Matrices", 0);
		mainShader->BindUniformBlock("Lighting", 1);
		colorShader->BindUniformBlock("Matrices", 0);
		//skyboxShader->BindUniformBlock("Matrices", 0);
		colliderShader->BindUniformBlock("Matrices", 0);

		// render point light sphere
		colorShader->Enable();
		int rendered_point_light = 0;
		const std::vector<Mesh>& sphere = assetManager.GetModel("sphere")->GetMeshes();
		for (auto [_, light] : lightView.each())
		{
			if (light.type == "point_light" && rendered_point_light < MAX_POINT_LIGHTS)
			{
				rendered_point_light++;
				if (light.render)
				{
					glm::mat4 light_sphere_model = glm::mat4(1.0f);
					light_sphere_model = glm::translate(light_sphere_model, light.pos);
					light_sphere_model = glm::scale(light_sphere_model, glm::vec3(0.25));

					colorShader->SetUniformMat4("model", light_sphere_model);
					colorShader->SetUniformVec3("color", light.diffuse);
					sphere[0].Draw();
				}
			}
		}

		// render object
		mainShader->Enable();
		glm::mat4 model = glm::mat4(1.0f);
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


			model = glm::mat4(1.0f);
			//translate
			model = glm::translate(model, transform.position);
			//rotation
			glm::mat4 rotation = glm::toMat4(transform.rotation_quat);
			model = model * rotation;
			//scale
			model = glm::scale(model, transform.scale);
			// parent MVP
			if (id.parent_id != -1)
			{
				auto parent_entity = static_cast<entt::entity>(id.parent_id);
				if (runtimeRegistry.GetRegistry().valid(parent_entity))
				{
					auto parent_transform = runtimeRegistry.GetRegistry().get<TransformComponent>(parent_entity);
					glm::mat4 parentModel = glm::mat4(1.0f);
					parentModel = glm::translate(parentModel, parent_transform.position);
					parentModel = parentModel * glm::toMat4(parent_transform.rotation_quat);
					parentModel = glm::scale(parentModel, parent_transform.scale);
					model = parentModel * model;
				}
			}

			for (int i = 0; i < meshes.size(); i++)
			{
				ENGINE_CORE::ECS::Material& cur_material = meshR.GetMaterial(i);

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
				// set uniform textures
				if (cur_material.m_useTexture)
				{
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture->GetID());
					mainShader->SetUniformInt("material.skybox", 0);
					for (const auto& slot : TEXTURE_SLOTS)
					{
						// 1. 查找材质中是否存在该类型的贴图
						auto it = cur_material.m_textures.find(slot.key);
						bool hasTexture = (it != cur_material.m_textures.end() && !it->second.empty());

						// 2. 设置 Shader 的 bool 开关
						mainShader->SetUniformBool(slot.useUniform, hasTexture);

						if (hasTexture)
						{
							// 3. 激活对应的纹理单元 (GL_TEXTURE0 + 0, GL_TEXTURE0 + 1, ...)
							glActiveTexture(GL_TEXTURE0 + slot.unitIndex);

							// 4. 获取并绑定纹理
							// 注意：使用迭代器 it->second 获取纹理名，比再次用 [] 查找更快
							auto tex = assetManager.GetTexture(it->second);
							if (tex)
							{
								glBindTexture(GL_TEXTURE_2D, tex->GetID());
							}
							else
							{
								// 防御性编程：名字存在但资源未加载，绑定0防止错误的纹理采样
								glBindTexture(GL_TEXTURE_2D, 0);
							}

							// 5. 告诉 Shader 该采样器应该去读哪个纹理单元
							mainShader->SetUniformInt(slot.samplerUniform, slot.unitIndex);
						}
					}
				}
				meshes[i].Draw();
			}
		}

		//模板测试
		glStencilFunc(GL_NOTEQUAL, 1, 0xFF);	// 当目标像素的模板值不等于1时，通过测试
		glStencilMask(0x00);					// 禁止写入模板值
		//glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);					//禁止深度写入
		colorShader->Enable();
		model = glm::mat4(1.0f);
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

			model = glm::mat4(1.0f);
			//translate
			model = glm::translate(model, transform.position);
			//rotation
			glm::mat4 rotation = glm::toMat4(transform.rotation_quat);
			model = model * rotation;
			//scale
			model = glm::scale(model, transform.scale * glm::vec3(1.025f));
			// parent MVP
			if (id.parent_id != -1)
			{
				auto parent_entity = static_cast<entt::entity>(id.parent_id);
				if (runtimeRegistry.GetRegistry().valid(parent_entity))
				{
					auto parent_transform = runtimeRegistry.GetRegistry().get<TransformComponent>(parent_entity);
					glm::mat4 parentModel = glm::mat4(1.0f);
					parentModel = glm::translate(parentModel, parent_transform.position);
					parentModel = parentModel * glm::toMat4(parent_transform.rotation_quat);
					parentModel = glm::scale(parentModel, parent_transform.scale);
					model = parentModel * model;
				}
			}


			for (int i = 0; i < meshes.size(); i++)
			{
				ENGINE_CORE::ECS::Material& cur_material = meshR.GetMaterial(i);

				bool emptyDiffuse = cur_material.m_textures.find("diffuse")->second.empty();
				bool textureBug = (cur_material.m_useTexture == true) && (emptyDiffuse);

				std::string shaderName = cur_material.shaderName;

				colorShader->Enable();	// NOTE: now the shader is fixed
				colorShader->SetUniformMat4("model", model);
				colorShader->SetUniformVec3("color", glm::vec3(1.0f, 1.0f, 0.0f));

				meshes[i].Draw();
			}
		}
		glStencilMask(0xFF);						// 允许写入模板值
		glStencilFunc(GL_ALWAYS, 0, 0xFF);			// 总是通过模板测试，且ref为0
		//glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);						// 恢复深度写入

		// draw skybox
		glDepthFunc(GL_LEQUAL);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture->GetID());
		model = glm::mat4(1.0f);
		model = glm::scale(model, glm::vec3(5.0f));
		glm::mat4 skybox_view = glm::mat4(glm::mat3(viewMatrix));	//移除观察矩阵中的位移
		skyboxShader->Enable();
		skyboxShader->SetUniformMat4("model", model);
		skyboxShader->SetUniformMat4("view", skybox_view);
		skyboxShader->SetUniformMat4("projection", PerspectiveMatrix);
		skyboxShader->SetUniformInt("skybox", 0);
		const std::vector<Mesh>& skybox = assetManager.GetModel("skybox")->GetMeshes();
		skybox[0].Draw();
		glDepthFunc(GL_LESS);

		// physics debug render
		if (ENGINE_CORE::CoreEngineData::GetInstance().RenderCollidersEnabled())
		{
			auto& physicsWorld = runtimeRegistry.GetContext<std::shared_ptr<rp3d::PhysicsWorld>>();
			auto& physicsDebugger = physicsWorld->getDebugRenderer();

			colliderShader->Enable();
			model = glm::mat4(1.0f);
			colliderShader->SetUniformMat4("model", model);

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

