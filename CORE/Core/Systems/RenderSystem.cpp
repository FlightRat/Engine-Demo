#include "RenderSystem.h"
#include<glm/glm.hpp>
#include<glm/gtx/quaternion.hpp>
#include<glm/gtc/quaternion.hpp>
#include<Rendering/Core/Camera3D.h>
#include<Rendering/Essentials/Shader.h>
#include<Logger/Logger.h>
#include "../ECS/Entity.h"
#include "../Resources/AssetManager.h"
#include "../ECS/Components/TransformComponent.h"
#include "../ECS/Components/MeshFilter.h"
#include "../ECS/Components/MeshRenderer.h"

using namespace CORE::ECS;
using namespace RENDERING;
using namespace RESOURCES;

namespace CORE::Systems {
	RenderSystem::RenderSystem(CORE::ECS::Registry& registry):m_Registry{registry}
	{
	}

	void RenderSystem::Render()
	{
		auto& assetManager = m_Registry.GetContext<std::shared_ptr<AssetManager>>();

		// shader
		auto colorShader = assetManager->GetShader("colorShader");
		if (colorShader->ShaderProgramID() == 0)
		{
			ENGINE_ERROR("Shader has not been set correctly!");
			return;
		}
		auto texShader = assetManager->GetShader("texShader");
		if (texShader->ShaderProgramID() == 0)
		{
			ENGINE_ERROR("Shader has not been set correctly!");
			return;
		}
		auto hudShader = assetManager->GetShader("hudShader");
		if (hudShader->ShaderProgramID() == 0)
		{
			ENGINE_ERROR("Shader has not been set correctly!");
			return;
		}

		//// mafuyu
		//const auto mafuyu = assetManager->GetTexture("mafuyu");
		//glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D, mafuyu->GetID());
		//// football
		//const auto& football = assetManager->GetTexture("football");
		//glActiveTexture(GL_TEXTURE3);
		//glBindTexture(GL_TEXTURE_2D, football->GetID());
		//// brick
		//const auto& brick = assetManager->GetTexture("brick");
		//glActiveTexture(GL_TEXTURE4);
		//glBindTexture(GL_TEXTURE_2D, brick->GetID());
		// wood
		const auto& wood = assetManager->GetTexture("wood");
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, wood->GetID());
		// container
		const auto& container = assetManager->GetTexture("container");
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, container->GetID());
		// rust
		const auto& rust = assetManager->GetTexture("rust");
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, rust->GetID());

		// camera
		auto& camera = m_Registry.GetContext<std::shared_ptr<Camera3D>>();
		auto viewMatrix = camera->GetViewMatrix();
		glm::mat4 orthoMatrix = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
		glm::mat4 PerspectiveMatrix = glm::perspective(glm::radians(camera->Zoom), (float)600 / (float)600, 0.1f, 100.0f);


		glm::mat4 model = glm::mat4(1.0f);
		auto view = m_Registry.GetRegistry().view<TransformComponent, MeshFilter, MeshRenderer>();
		for (auto [_, transform, meshF, meshR] : view.each())
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
			glm::vec3 eulerAngle(transform.rotation.x, transform.rotation.y, transform.rotation.z);
			glm::quat quaternion = glm::quat(glm::radians(eulerAngle));
			glm::mat4 rotation = glm::toMat4(quaternion);
			model = model * rotation;
			//scale
			model = glm::scale(model, transform.scale);

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
	}
}

