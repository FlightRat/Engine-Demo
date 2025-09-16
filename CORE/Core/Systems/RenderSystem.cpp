#include "RenderSystem.h"
#include<glm/glm.hpp>
#include<Rendering/Core/Camera3D.h>
#include<Rendering/Essentials/Shader.h>
#include<Logger/Logger.h>
#include "../ECS/Entity.h"
#include "../Resources/AssetManager.h"
#include "../ECS/Components/MeshComponent.h"
#include "../ECS/Components/TransformComponent.h"

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
		auto& colorShader = assetManager->GetShader("colorShader");
		if (colorShader.ShaderProgramID() == 0)
		{
			ENGINE_ERROR("Shader has not been set correctly!");
			return;
		}
		colorShader.Enable();

		// texture
		const auto& texture = assetManager->GetTexture("mafuyu");
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture.GetID());

		// camera
		auto& camera = m_Registry.GetContext<std::shared_ptr<Camera3D>>();
		auto viewMatrix = camera->GetViewMatrix();
		glm::mat4 projectionMatrix = glm::perspective(glm::radians(camera->Zoom), (float)600 / (float)600, 0.1f, 100.0f);
		colorShader.SetUniformMat4("view", viewMatrix);
		colorShader.SetUniformMat4("projection", projectionMatrix);

		glm::mat4 model = glm::mat4(1.0f);
		auto view = m_Registry.GetRegistry().view<MeshComponent, TransformComponent>();
		for (auto [_, mesh, transform] : view.each())
		{
			model = glm::mat4(1.0f);
			model = glm::translate(model, transform.position);
			model = glm::scale(model, transform.scale);	// NOTE::do scale after translate!!!
			colorShader.SetUniformMat4("model", model);
			colorShader.SetUniformVec3("color", mesh.color);
			mesh.Render();
		}
	}
}

