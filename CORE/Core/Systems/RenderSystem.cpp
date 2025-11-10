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
#include "../CoreUtilities/CoreEngineData.h"

using namespace ENGINE_CORE::ECS;
using namespace ENGINE_RENDERING;
using namespace ENGINE_RESOURCES;

namespace ENGINE_CORE::Systems {
	RenderSystem::RenderSystem(ENGINE_CORE::ECS::Registry& registry):m_Registry{registry}
	{
		glGenVertexArrays(1, &m_DebugVAO);
		glGenBuffers(1, &m_DebugVBO);
	}

	void RenderSystem::Render(std::shared_ptr<ENGINE_RENDERING::Camera3D> camera)
	{
		auto& mainRegistry = MAIN_REGISTRY();
		auto& assetManager = mainRegistry.GetAssetManager();
		//auto& assetManager = m_Registry.GetContext<std::shared_ptr<AssetManager>>();
		auto& physicsWorld = m_Registry.GetContext<std::shared_ptr<rp3d::PhysicsWorld>>();
		auto& physicsDebugger = physicsWorld->getDebugRenderer();

		// shader
		auto colorShader = assetManager.GetShader("colorShader");
		if (colorShader->ShaderProgramID() == 0)
		{
			ENGINE_ERROR("Shader has not been set correctly!");
			return;
		}
		auto texShader = assetManager.GetShader("texShader");
		if (texShader->ShaderProgramID() == 0)
		{
			ENGINE_ERROR("Shader has not been set correctly!");
			return;
		}
		auto hudShader = assetManager.GetShader("hudShader");
		if (hudShader->ShaderProgramID() == 0)
		{
			ENGINE_ERROR("Shader has not been set correctly!");
			return;
		}
		auto debugShader = assetManager.GetShader("debugShader");
		if (debugShader->ShaderProgramID() == 0)
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
		auto view = m_Registry.GetRegistry().view<TransformComponent, MeshFilter, MeshRender, Identification>();
		for (auto [entity, transform, meshF, meshR, id] : view.each())
		{
			if (!meshR.shouldRender)
			{
				continue;
			}
			if (!meshR.m_loaded)
			{
				meshR.UploadMesh(meshF);
			}
			model = glm::mat4(1.0f);
			//translate
			model = glm::translate(model, transform.position);
			//rotation
			glm::mat4 rotation = glm::toMat4(transform.rotation_quat);
			model = model * rotation;
			//scale
			model = glm::scale(model, transform.scale);
			if (id.parent_id != -1)
			{
				auto parent_entity = static_cast<entt::entity>(id.parent_id);
				if (m_Registry.GetRegistry().valid(parent_entity))
				{
					auto parent_transform = m_Registry.GetRegistry().get<TransformComponent>(parent_entity);
					glm::mat4 parentModel = glm::mat4(1.0f);
					parentModel = glm::translate(parentModel, parent_transform.position);
					parentModel = parentModel * glm::toMat4(parent_transform.rotation_quat);
					parentModel = glm::scale(parentModel, parent_transform.scale);
					model = parentModel * model;
				}
			}

			if (meshR.shader == "colorShader")
			{
				colorShader->Enable();
				colorShader->SetUniformMat4("model", model);
				colorShader->SetUniformMat4("view", viewMatrix);
				colorShader->SetUniformMat4("projection", PerspectiveMatrix);
				colorShader->SetUniformVec4("color", meshR.color);
			}
			else if (meshR.shader == "texShader")
			{
				texShader->Enable();
				texShader->Enable();
				texShader->SetUniformMat4("model", model);
				texShader->SetUniformMat4("view", viewMatrix);
				texShader->SetUniformMat4("projection", PerspectiveMatrix);
				texShader->SetUniformInt("tex", meshR.texture);
			}
			else if (meshR.shader == "hudShader")
			{
				hudShader->Enable();
				hudShader->SetUniformMat4("model", model);
				hudShader->SetUniformMat4("projection", orthoMatrix);
				hudShader->SetUniformVec4("color", meshR.color);
			}

			glBindVertexArray(meshR.m_VAO);
			glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(meshF.index_data.size()), GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);
		}
		if (ENGINE_CORE::CoreEngineData::GetInstance().RenderCollidersEnabled())
		{
			debugShader->Enable();
			model = glm::mat4(1.0f);
			debugShader->SetUniformMat4("model", model);
			debugShader->SetUniformMat4("view", viewMatrix);
			debugShader->SetUniformMat4("projection", PerspectiveMatrix);

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

