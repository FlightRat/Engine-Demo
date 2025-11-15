#include "SceneDisplay.h"
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
#include "../utilities/editor_framebuffers.h"
#include <Core/Systems/ScriptingSystem.h>
#include <Core/Systems/RenderSystem.h>
#include <Rendering/Core/Camera3D.h>

namespace ENGINE_EDIOTR
{
	SceneDisplay::SceneDisplay(ENGINE_CORE::ECS::Registry& registry) 
		:m_Registry{ registry }, m_bPlayScene{ false }, m_bSceneLoaded{ false }
	{
	}

	void SceneDisplay::LoadScene()
	{
		auto& scriptSystem = m_Registry.GetContext<std::shared_ptr<ENGINE_CORE::Systems::ScriptingSystem>>();
		auto& lua = m_Registry.GetContext<std::shared_ptr<sol::state>>();
		if (!lua)
		{
			lua = std::make_shared<sol::state>();
		}
		lua->open_libraries(sol::lib::base, sol::lib::math, sol::lib::os, sol::lib::table, sol::lib::io, sol::lib::string);
		ENGINE_CORE::Systems::ScriptingSystem::RegisterLuaBindings(*lua, m_Registry);
		ENGINE_CORE::Systems::ScriptingSystem::RegisterLuaFunctions(*lua);
		if (!scriptSystem->LoadMainScript(*lua))
		{
			ENGINE_ERROR("Failed to load the main lua script!");
			return;
		}
		m_bPlayScene = true;
		m_bSceneLoaded = true;
	}

	void SceneDisplay::UnloadScene()
	{
		m_bPlayScene = false;
		m_bSceneLoaded = false;
		m_Registry.GetRegistry().clear();
		auto& lua = m_Registry.GetContext<std::shared_ptr<sol::state>>();
		lua.reset();

		auto& mainRegistry = MAIN_REGISTRY();
		mainRegistry.GetMusicPlayer().Stop();
		mainRegistry.GetSoundFxPlayer().Stop(-1);
	}

	void SceneDisplay::RenderScene()
	{
		auto& mainRegistry = MAIN_REGISTRY();
		auto& renderSystem = mainRegistry.GetContext<std::shared_ptr<ENGINE_CORE::Systems::RenderSystem>>();
		//auto& scriptSystem = mainRegistry.GetContext<std::shared_ptr<ENGINE_CORE::Systems::ScriptingSystem>>();
		auto& camera = m_Registry.GetContext<std::shared_ptr<ENGINE_RENDERING::Camera3D>>();
		auto& editorFramebuffer = mainRegistry.GetContext<std::shared_ptr<ENGINE_EDITOR::Editorframebuffers>>();

		const auto& fb = editorFramebuffer->mapFramebuffers[ENGINE_EDITOR::FramebufferType::GAME];
		fb->Bind();
		glViewport(0, 0, fb->Width(), fb->Height());
		glClearColor(0.f, 0.f, 0.f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		camera->SetWidth(fb->Width());
		camera->SetHeight(fb->Height());
		//scriptSystem->Render();
		renderSystem->Render(camera);
		fb->Unbind();
		fb->CheckResize();
	}

	void SceneDisplay::Draw()
	{
		static bool pOpen{ true };
		if (!ImGui::Begin("Scene", &pOpen))
		{
			ImGui::End();
			return;
		}

		auto& mainRegistry = MAIN_REGISTRY();
		auto& assetManager = mainRegistry.GetAssetManager();

		auto pPlayTexture = assetManager.GetTexture("play_button");
		auto pStopTexture = assetManager.GetTexture("stop_button");

		if (m_bPlayScene)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.9f, 0.0f, 0.3f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.9f, 0.0f, 0.3f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.9f, 0.0f, 0.3f));
		}
		if (ImGui::ImageButton(
			"play",
			ImTextureID{ pPlayTexture->GetID() },
			ImVec2{ (float)pPlayTexture->GetWidth() * 0.25f, (float)pPlayTexture->GetHeight() * 0.25f, })
			&& !m_bSceneLoaded)
		{
			LoadScene();
		}
		if (ImGui::GetColorStackSize() > 0)
			ImGui::PopStyleColor(ImGui::GetColorStackSize());
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
			ImGui::SetTooltip("Play Scene");

		ImGui::SameLine();

		if (!m_bPlayScene)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.9f, 0.0f, 0.3f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.9f, 0.0f, 0.3f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.9f, 0.0f, 0.3f));
		}

		RenderScene();

		if (ImGui::ImageButton(
			"stop",
			ImTextureID{ pStopTexture->GetID() },
			ImVec2{ (float)pStopTexture->GetWidth() * 0.25f, (float)pStopTexture->GetHeight() * 0.25f, })
			&& m_bSceneLoaded)
		{
			UnloadScene();
		}
		if (ImGui::GetColorStackSize() > 0)
			ImGui::PopStyleColor(ImGui::GetColorStackSize());
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
			ImGui::SetTooltip("Stop Scene");


		if (ImGui::BeginChild("##SceneChild", ImVec2{ 0.f,0.f }, NULL, ImGuiWindowFlags_NoScrollWithMouse))
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
	
	void SceneDisplay::Update()
	{
		if (!m_bPlayScene)
			return;

		auto& mainRegistry = MAIN_REGISTRY();
		auto& coreGlobals = CORE_GLOBALS();

		const decimal timeStep = coreGlobals.GetPhysicsTimeStep();
		double deltaTime = coreGlobals.GetDeltaTime();
		double& accumulator = coreGlobals.GetAccumulator();
		
		const double MAX_DELTA_TIME = 0.25;
		double dt = deltaTime > MAX_DELTA_TIME ? 0.25 : deltaTime;
		accumulator += dt;
		
		auto& scriptSystem = m_Registry.GetContext<std::shared_ptr<ENGINE_CORE::Systems::ScriptingSystem>>();
		auto& physicsWorld = m_Registry.GetContext<std::shared_ptr<rp3d::PhysicsWorld>>();
		auto& physicsSystem = m_Registry.GetContext<std::shared_ptr<ENGINE_CORE::Systems::PhysicsSystem>>();
		
		scriptSystem->Update();
		while (accumulator >= timeStep) {	// TODO: add check "if (coreGlobals.IsPhysicsEnabled())"
			physicsWorld->update(timeStep);
			accumulator -= timeStep;
		}
		decimal factor = accumulator / timeStep;
		physicsSystem->Update(m_Registry.GetRegistry(), factor);
		//TODO:update camera here
	}
}
