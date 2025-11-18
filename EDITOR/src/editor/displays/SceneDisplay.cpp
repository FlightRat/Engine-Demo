#include "SceneDisplay.h"
#include "Core/ECS/MainRegistry.h"
#include "Core/Systems/RenderSystem.h"
#include "Core/Systems/ScriptingSystem.h"
#include "Core/Resources/AssetManager.h"
#include "Rendering/Core/Camera3D.h"
#include "../utilities/editor_framebuffers.h"
#include "../utilities/editor_utilities.h"
#include "../scene/SceneManager.h"
#include "../scene/SceneObject.h"
#include "Logger/Logger.h"
#include <imgui.h>

namespace ENGINE_EDIOTR {
	void SceneDisplay::RenderScene()
	{
		// TODO: conflict with game display -- everything is loaded when play button is pressed
		//auto pCurrentScene = SCENE_MANAGER().GetCurrentScene();
		//if (!pCurrentScene )
		//	return;
		//auto& runtimeRegistry = pCurrentScene->GetRegistry();
		//auto& scriptSystem = runtimeRegistry.GetContext<std::shared_ptr<ENGINE_CORE::Systems::ScriptingSystem>>();

		//auto& mainRegistry = MAIN_REGISTRY();
		//auto& renderSystem = mainRegistry.GetContext<std::shared_ptr<ENGINE_CORE::Systems::RenderSystem>>();
		//auto& editorFramebuffer = mainRegistry.GetContext<std::shared_ptr<ENGINE_EDITOR::Editorframebuffers>>();

		//const auto& fb = editorFramebuffer->mapFramebuffers[ENGINE_EDITOR::FramebufferType::SCENE];
		//fb->Bind();
		//glViewport(0, 0, fb->Width(), fb->Height());
		//glClearColor(0.f, 0.f, 0.f, 1.f);
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//m_pSceneCam->SetWidth(fb->Width());
		//m_pSceneCam->SetHeight(fb->Height());
		//scriptSystem->Render();
		//renderSystem->Render(m_pSceneCam, runtimeRegistry);
		//fb->Unbind();
		//fb->CheckResize();
	}

	SceneDisplay::SceneDisplay():m_pSceneCam{std::make_shared<ENGINE_RENDERING::Camera3D>(glm::vec3(0.0f, 10.0f, 10.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -45.0f)}
	{
	}

	void SceneDisplay::Draw()
	{
		if (!ImGui::Begin("Scene"))
		{
			ImGui::End();
			return;
		}
		RenderScene();
		auto& mainRegistry = MAIN_REGISTRY();
		if (ImGui::BeginChild("##SceneChild", ImVec2{ 0,0 }, false, ImGuiWindowFlags_NoScrollWithMouse))
		{
			auto& editorFramebuffers = mainRegistry.GetContext<std::shared_ptr<ENGINE_EDITOR::Editorframebuffers>>();
			const auto& fb = editorFramebuffers->mapFramebuffers[ENGINE_EDITOR::FramebufferType::SCENE];
			ImVec2 imageSize{ static_cast<float>(fb->Width()),static_cast<float>(fb->Height()) };
			ImVec2 windowSize{ ImGui::GetWindowSize() };

			float x = (windowSize.x - imageSize.x) * 0.5f;
			float y = (windowSize.y - imageSize.y) * 0.5f;
			ImGui::SetCursorPos(ImVec2{ x,y });

			ImGui::Image((ImTextureID)fb->GetTextureID(), imageSize, ImVec2{ 0.f,1.f }, ImVec2{ 1.f,0.f });

			// check size
			if (fb->Width() != static_cast<int>(windowSize.x) || fb->Height() != static_cast<int>(windowSize.y))
				fb->Resize(static_cast<int>(windowSize.x), static_cast<int>(windowSize.y));

			if (ImGui::BeginDragDropTarget())
			{
				const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(DROP_SCENE_SRC);
				if (payload)
				{
					ENGINE_LOG("BEFORE: {}", SCENE_MANAGER().GetCurrentSceneName());
					SCENE_MANAGER().SetCurrentScene(std::string{ (const char*)payload->Data });
					ENGINE_LOG("AFTER: {}", SCENE_MANAGER().GetCurrentSceneName());
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::EndChild();
		}
		ImGui::End();
	}

	void SceneDisplay::Update()
	{
		//TODO:move the camera in scene display
	}
}

