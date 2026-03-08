#include "RenderSystem.h"
#include<glm/glm.hpp>
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
	struct TextureSlot {
		const char* key;            // 材质 Map 中的 key (如 "diffuse")
		const char* useUniform;     // Shader bool 开关 (如 "material.useDiffuse")
		const char* samplerUniform; // Shader sampler2D 名字 (如 "material.diffuse")
		int unitIndex;              // 纹理单元 (0, 1)
	};

	// 使用 constexpr 让它在编译期就确定，性能最高
	constexpr std::array<TextureSlot, 2> TEXTURE_SLOTS = { {
		{ "diffuse",  "material.useDiffuse",  "material.diffuse",  0 },
		{ "specular", "material.useSpecular", "material.specular", 1 }
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
		auto& physicsWorld = runtimeRegistry.GetContext<std::shared_ptr<rp3d::PhysicsWorld>>();
		auto& physicsDebugger = physicsWorld->getDebugRenderer();

		// shader
		auto mainShader = assetManager.GetShader("mainShader");
		if (mainShader->ShaderProgramID() == 0)
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

		// camera
		auto viewMatrix = camera->GetViewMatrix();
		glm::mat4 orthoMatrix = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
		glm::mat4 PerspectiveMatrix = glm::perspective(glm::radians(camera->Zoom), (float)camera->GetWidth() / (float)camera->GetHeight(), 0.1f, 100.0f);

		glm::mat4 model = glm::mat4(1.0f);
		auto view = runtimeRegistry.GetRegistry().view<TransformComponent, MeshFilter, MeshRender, Identification>();
		for (auto [entity, transform, meshF, meshR, id] : view.each())
		{
			if (!meshR.shouldRender)
			{
				continue;
			}

			const std::vector<Mesh>& meshes = assetManager.GetModel(meshF.mesh)->meshes;
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
				mainShader->SetUniformMat4("view", viewMatrix);
				mainShader->SetUniformMat4("projection", PerspectiveMatrix);

				mainShader->SetUniformVec3("viewPos", camera->GetPosition());
				mainShader->SetUniformBool("bug", textureBug);
				mainShader->SetUniformBool("flipUV", meshR.flipUV);
				mainShader->SetUniformVec4("material.color", cur_material.color);
				mainShader->SetUniformFloat("material.shininess", cur_material.shininess);
				mainShader->SetUniformBool("useTexture", cur_material.m_useTexture);
				// set uniform textures
				if (cur_material.m_useTexture)
				{
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

				//// 重置点光源（设为0向量/0值，代表无贡献）
				//mainShader->SetUniformVec3("pointLights[0].diffuse", glm::vec3(0.0f));
				//mainShader->SetUniformVec3("pointLights[0].specular", glm::vec3(0.0f));
				//mainShader->SetUniformVec3("pointLights[0].ambient", glm::vec3(0.0f));
				//mainShader->SetUniformVec3("pointLights[0].position", glm::vec3(0.0f));
				//mainShader->SetUniformFloat("pointLights[0].constant", 0.0f);
				//mainShader->SetUniformFloat("pointLights[0].linear", 0.0f);
				//mainShader->SetUniformFloat("pointLights[0].quadratic", 0.0f);

				//// 重置方向光（同理设为0）
				//mainShader->SetUniformVec3("dirLight.diffuse", glm::vec3(0.0f));
				//mainShader->SetUniformVec3("dirLight.specular", glm::vec3(0.0f));
				//mainShader->SetUniformVec3("dirLight.ambient", glm::vec3(0.0f));
				//mainShader->SetUniformVec3("dirLight.direction", glm::vec3(0.0f));

				int point_light_num = -1;
				for (auto [_, light] : runtimeRegistry.GetRegistry().view<LightComponent>().each())
				{
					if (light.type == "point_light")
					{
						point_light_num++;
						std::string uniformPrefix = "pointLights[" + std::to_string(point_light_num) + "].";
						mainShader->SetUniformVec3(uniformPrefix + "diffuse", light.diffuse);
						mainShader->SetUniformVec3(uniformPrefix + "specular", light.specular);
						mainShader->SetUniformVec3(uniformPrefix + "ambient", light.ambient);
						mainShader->SetUniformVec3(uniformPrefix + "position", light.pos);
						mainShader->SetUniformFloat(uniformPrefix + "constant", light.constant);
						mainShader->SetUniformFloat(uniformPrefix + "linear", light.linear);
						mainShader->SetUniformFloat(uniformPrefix + "quadratic", light.quadratic);
					}
					else if (light.type == "direction_light")
					{
						mainShader->SetUniformVec3("dirLight.diffuse", light.diffuse);
						mainShader->SetUniformVec3("dirLight.specular", light.specular);
						mainShader->SetUniformVec3("dirLight.ambient", light.ambient);
						mainShader->SetUniformVec3("dirLight.direction", light.direction);
					}
				}
				
				meshes[i].Draw();
			}

		}
		if (ENGINE_CORE::CoreEngineData::GetInstance().RenderCollidersEnabled())
		{
			colliderShader->Enable();
			model = glm::mat4(1.0f);
			colliderShader->SetUniformMat4("model", model);
			colliderShader->SetUniformMat4("view", viewMatrix);
			colliderShader->SetUniformMat4("projection", PerspectiveMatrix);

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

