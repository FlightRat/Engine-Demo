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
		auto bugShader = assetManager.GetShader("bugShader");
		if (bugShader->ShaderProgramID() == 0)
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

		// TODO: change the way use and bind texture
		//// wood
		//const auto& wood = assetManager.GetTexture("wood");
		//glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D, wood->GetID());
		//// container
		//const auto& container = assetManager.GetTexture("container");
		//glActiveTexture(GL_TEXTURE1);
		//glBindTexture(GL_TEXTURE_2D, container->GetID());
		//// rust
		//const auto& rust = assetManager.GetTexture("rust");
		//glActiveTexture(GL_TEXTURE2);
		//glBindTexture(GL_TEXTURE_2D, rust->GetID());

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
			if (!meshR.m_loaded || meshF.m_bChanged)
			{
				meshR.UploadMesh(meshF);
				meshF.m_bChanged = false;
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

			bool bug = ((meshR.m_useTexture == true) && (meshR.textureName == ""));
			if (bug)
			{
				bugShader->Enable();
				bugShader->Enable();
				bugShader->SetUniformMat4("model", model);
				bugShader->SetUniformMat4("view", viewMatrix);
				bugShader->SetUniformMat4("projection", PerspectiveMatrix);

				bugShader->SetUniformVec3("viewPos", camera->GetPosition());

				for (auto [_, light] : runtimeRegistry.GetRegistry().view<LightComponent>().each())
				{
					if (light.type == "point_light")
					{
						bugShader->SetUniformVec3("pointLights[0].diffuse", light.diffuse);
						bugShader->SetUniformVec3("pointLights[0].specular", light.specular);
						bugShader->SetUniformVec3("pointLights[0].ambient", light.ambient);
						bugShader->SetUniformVec3("pointLights[0].position", light.pos);
						bugShader->SetUniformFloat("pointLights[0].constant", light.constant);
						bugShader->SetUniformFloat("pointLights[0].linear", light.linear);
						bugShader->SetUniformFloat("pointLights[0].quadratic", light.quadratic);
					}
					else if (light.type == "direction_light")
					{
						bugShader->SetUniformVec3("dirLight.diffuse", light.diffuse);
						bugShader->SetUniformVec3("dirLight.specular", light.specular);
						bugShader->SetUniformVec3("dirLight.ambient", light.ambient);
						bugShader->SetUniformVec3("dirLight.direction", light.direction);
					}
				}
			}
			else
			{
				mainShader->Enable();
				mainShader->Enable();
				mainShader->SetUniformMat4("model", model);
				mainShader->SetUniformMat4("view", viewMatrix);
				mainShader->SetUniformMat4("projection", PerspectiveMatrix);

				mainShader->SetUniformVec3("viewPos", camera->GetPosition());
				mainShader->SetUniformVec4("objectColor", meshR.color);
				mainShader->SetUniformInt("objectTexture", 0);
				mainShader->SetUniformBool("useTexture", meshR.m_useTexture);
				if (meshR.m_useTexture)
				{
					const auto& tex = assetManager.GetTexture(meshR.textureName);
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, tex->GetID());
				}

				for (auto [_, light] : runtimeRegistry.GetRegistry().view<LightComponent>().each())
				{
					if (light.type == "point_light")
					{
						mainShader->SetUniformVec3("pointLights[0].diffuse", light.diffuse);
						mainShader->SetUniformVec3("pointLights[0].specular", light.specular);
						mainShader->SetUniformVec3("pointLights[0].ambient", light.ambient);
						mainShader->SetUniformVec3("pointLights[0].position", light.pos);
						mainShader->SetUniformFloat("pointLights[0].constant", light.constant);
						mainShader->SetUniformFloat("pointLights[0].linear", light.linear);
						mainShader->SetUniformFloat("pointLights[0].quadratic", light.quadratic);
					}
					else if (light.type == "direction_light")
					{
						mainShader->SetUniformVec3("dirLight.diffuse", light.diffuse);
						mainShader->SetUniformVec3("dirLight.specular", light.specular);
						mainShader->SetUniformVec3("dirLight.ambient", light.ambient);
						mainShader->SetUniformVec3("dirLight.direction", light.direction);
					}
				}
			}

			glBindVertexArray(meshR.m_VAO);
			glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(meshF.index_data.size()), GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);
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

