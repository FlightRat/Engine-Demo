#include "GameDisplay.h"
#include "imgui.h"
#include "Logger/Logger.h"
#include "Rendering/Buffers/Framebuffer.h"
#include "Core/ECS/MainRegistry.h"
#include "Core/Systems/RenderSystem.h"
#include "Core/Systems/PhysicsSystem.h"
#include "Core/Systems/ScriptingSystem.h"
#include "Core/Resources/AssetManager.h"
#include "Core/CoreUtilities/CoreEngineData.h"
#include "Sounds/MusicPlayer/MusicPlayer.h"
#include "Sounds/SoundFxPlayer/SoundFxPlayer.h"
#include "Physics/RP3D_Wrappers.h"
#include "Physics/ContactListener.h"
#include "../utilities/editor_framebuffers.h"
#include <Core/Systems/ScriptingSystem.h>
#include <Core/Systems/RenderSystem.h>
#include <Rendering/Core/Camera3D.h>
#include "../scene/SceneManager.h"
#include "../scene/SceneObject.h"

using namespace ENGINE_CORE::Systems;
using namespace ENGINE_RENDERING;

namespace ENGINE_EDITOR
{
	GameDisplay::GameDisplay() :m_bPlayGame{ false }, m_bSceneLoaded{ false }
	{
	}

	void GameDisplay::PlayGame()
	{
		auto pCurrentScene = SCENE_MANAGER().GetCurrentScene();
		pCurrentScene->SetPlay(true);
		m_bPlayGame = true;
	}

	void GameDisplay::StopGame()
	{
		auto pCurrentScene = SCENE_MANAGER().GetCurrentScene();
		pCurrentScene->SetPlay(false);
		m_bPlayGame = false;

		//auto& runtimeRegistry = pCurrentScene->GetRegistry();
		//runtimeRegistry.ClearRegistry();
		//runtimeRegistry.RemoveContext<std::shared_ptr<ENGINE_RENDERING::Camera3D>>();
		//runtimeRegistry.RemoveContext<std::shared_ptr<sol::state>>();
		//runtimeRegistry.RemoveContext<std::shared_ptr<rp3d::PhysicsCommon>>();
		//runtimeRegistry.RemoveContext<std::shared_ptr<rp3d::PhysicsWorld>>();
		//runtimeRegistry.RemoveContext<std::shared_ptr<ENGINE_PHYSICS::ContactListener>>();
		//runtimeRegistry.RemoveContext<std::shared_ptr<ENGINE_CORE::Systems::PhysicsSystem>>();
		//runtimeRegistry.RemoveContext<std::shared_ptr<ENGINE_CORE::Systems::ScriptingSystem>>();

		//auto& mainRegistry = MAIN_REGISTRY();
		//mainRegistry.GetMusicPlayer().Stop();
		//mainRegistry.GetSoundFxPlayer().Stop(-1);
	}

	void GameDisplay::RenderGame()
	{
		auto& mainRegistry = MAIN_REGISTRY();
		auto& renderSystem = mainRegistry.GetContext<std::shared_ptr<ENGINE_CORE::Systems::RenderSystem>>();
		auto& editorFramebuffer = mainRegistry.GetContext<std::shared_ptr<ENGINE_EDITOR::Editorframebuffers>>();
		const auto& fb = editorFramebuffer->mapFramebuffers[ENGINE_EDITOR::FramebufferType::GAME];

		fb->Bind();
		glViewport(0, 0, fb->Width(), fb->Height());
		glClearColor(0.f, 0.f, 0.f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		auto pCurrentScene = SCENE_MANAGER().GetCurrentScene();
		if (pCurrentScene && pCurrentScene->CheckPlay())
		{
			auto& runtimeRegistry = pCurrentScene->GetRegistry();
			auto& camera = runtimeRegistry.GetContext<std::shared_ptr<ENGINE_RENDERING::Camera3D>>();
			camera->SetWidth(fb->Width());
			camera->SetHeight(fb->Height());
			auto& scriptSystem = runtimeRegistry.GetContext<std::shared_ptr<ENGINE_CORE::Systems::ScriptingSystem>>();
			scriptSystem->Render();
			renderSystem->Render(camera, runtimeRegistry);
		}
		fb->Unbind();
		fb->CheckResize();
	}

	void GameDisplay::Draw()
	{
		static bool pOpen{ true };
		if (!ImGui::Begin("Game", &pOpen))
		{
			ImGui::End();
			return;
		}

		auto& mainRegistry = MAIN_REGISTRY();
		auto& assetManager = mainRegistry.GetAssetManager();

		auto pPlayTexture = assetManager.GetTexture("play_button");
		auto pStopTexture = assetManager.GetTexture("stop_button");

		if (m_bPlayGame)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.9f, 0.0f, 0.3f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.9f, 0.0f, 0.3f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.9f, 0.0f, 0.3f));
		}
		if (ImGui::ImageButton(
			"play",
			ImTextureID{ pPlayTexture->GetID() },
			ImVec2{ (float)pPlayTexture->GetWidth() * 0.25f, (float)pPlayTexture->GetHeight() * 0.25f, })
			&& SCENE_MANAGER().GetCurrentScene())
		{
			PlayGame();
		}
		if (ImGui::GetColorStackSize() > 0)
			ImGui::PopStyleColor(ImGui::GetColorStackSize());
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
			ImGui::SetTooltip("Play Game");

		ImGui::SameLine();

		if (!m_bPlayGame)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.9f, 0.0f, 0.3f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.9f, 0.0f, 0.3f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.9f, 0.0f, 0.3f));
		}

		RenderGame();

		if (ImGui::ImageButton(
			"stop",
			ImTextureID{ pStopTexture->GetID() },
			ImVec2{ (float)pStopTexture->GetWidth() * 0.25f, (float)pStopTexture->GetHeight() * 0.25f, })
			&& SCENE_MANAGER().GetCurrentScene())
		{
			StopGame();
		}
		if (ImGui::GetColorStackSize() > 0)
			ImGui::PopStyleColor(ImGui::GetColorStackSize());
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
			ImGui::SetTooltip("Stop Game");


		if (ImGui::BeginChild("##GameChild", ImVec2{ 0.f,0.f }, NULL, ImGuiWindowFlags_NoScrollWithMouse))
		{
			auto& mainRegistry = MAIN_REGISTRY();
			auto& editorFramebuffer = mainRegistry.GetContext<std::shared_ptr<ENGINE_EDITOR::Editorframebuffers>>();
			const auto& fb = editorFramebuffer->mapFramebuffers[ENGINE_EDITOR::FramebufferType::GAME];
			//const auto& fb = m_Registry.GetContext<std::shared_ptr<ENGINE_RENDERING::Framebuffer>>();
			ImGui::SetCursorPos(ImVec2{ 0.f,0.f });
			ImGui::Image(
				(ImTextureID)fb->GetTextureID(),
				ImVec2{
					static_cast<float>(fb->Width()),
					static_cast<float>(fb->Height())
				},
				ImVec2{ 0.f,1.f }, ImVec2{ 1.f,0.f }
			);
			ImGui::EndChild();

			ImVec2 windowSize{ ImGui::GetWindowSize() };
			if (fb->Width() != static_cast<int>(windowSize.x) || fb->Height() != static_cast<int>(windowSize.y))
				fb->Resize(static_cast<int>(windowSize.x), static_cast<int>(windowSize.y));
		}
		ImGui::End();
	}
	
	void GameDisplay::Update()
	{
		// TODO: if play, update lua
		auto pCurrentScene = SCENE_MANAGER().GetCurrentScene();
		if (!pCurrentScene || !pCurrentScene->CheckPlay())
			return;
		//ControlCam();
		//TODO:move the camera in scene display

		auto& mainRegistry = MAIN_REGISTRY();
		auto& coreGlobals = CORE_GLOBALS();
		auto& runtimeRegistry = pCurrentScene->GetRegistry();

		const decimal timeStep = coreGlobals.GetPhysicsTimeStep();
		double deltaTime = coreGlobals.GetDeltaTime();
		double& accumulator = coreGlobals.GetAccumulator();

		const double MAX_DELTA_TIME = 0.25;
		double dt = deltaTime > MAX_DELTA_TIME ? 0.25 : deltaTime;
		accumulator += dt;

		auto& scriptSystem = runtimeRegistry.GetContext<std::shared_ptr<ENGINE_CORE::Systems::ScriptingSystem>>();
		auto& physicsWorld = runtimeRegistry.GetContext<std::shared_ptr<rp3d::PhysicsWorld>>();
		auto& physicsSystem = runtimeRegistry.GetContext<std::shared_ptr<ENGINE_CORE::Systems::PhysicsSystem>>();

		scriptSystem->Update();
		while (accumulator >= timeStep) {	// TODO: add check "if (coreGlobals.IsPhysicsEnabled())"
			physicsWorld->update(timeStep);
			accumulator -= timeStep;
		}
		decimal factor = accumulator / timeStep;
		physicsSystem->Update(runtimeRegistry.GetRegistry(), factor);
	}
}
