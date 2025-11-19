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

namespace ENGINE_EDIOTR
{
	GameDisplay::GameDisplay() :m_bPlayScene{ false }, m_bSceneLoaded{ false }
	{
	}

	void GameDisplay::LoadScene()
	{
		auto pCurrentScene = SCENE_MANAGER().GetCurrentScene();
		if (!pCurrentScene)
			return;
		auto& runtimeRegistry = pCurrentScene->GetRegistry();

		// Camera
		auto camera = std::make_shared<ENGINE_RENDERING::Camera3D>(glm::vec3(0.0f, 10.0f, 10.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -45.0f);
		runtimeRegistry.AddToContext<std::shared_ptr<ENGINE_RENDERING::Camera3D>>(camera);

		// Physics Common
		std::shared_ptr<PhysicsCommon> physicsCommon = std::make_shared<PhysicsCommon>();
		runtimeRegistry.AddToContext<std::shared_ptr<rp3d::PhysicsCommon>>(physicsCommon);

		// Physics World
		std::shared_ptr<PhysicsWorld> physicsWorld = ENGINE_PHYSICS::MakeSharedPhysicsWorld(physicsCommon);
		runtimeRegistry.AddToContext<std::shared_ptr<rp3d::PhysicsWorld>>(physicsWorld);
		physicsWorld->getDebugRenderer().setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::COLLISION_SHAPE, true);

		// Contact Listener
		auto contactListener = std::make_shared<ENGINE_PHYSICS::ContactListener>();
		runtimeRegistry.AddToContext<std::shared_ptr< ENGINE_PHYSICS::ContactListener>>(contactListener);
		physicsWorld->setEventListener(contactListener.get());

		// Physics System
		auto physicsSystem = std::make_shared<PhysicsSystem>(runtimeRegistry);
		runtimeRegistry.AddToContext<std::shared_ptr< PhysicsSystem>>(physicsSystem);

		// Script system
		auto scriptSystem = std::make_shared<ScriptingSystem>(runtimeRegistry);
		runtimeRegistry.AddToContext<std::shared_ptr<ScriptingSystem>>(scriptSystem);

		// lua
		auto lua = runtimeRegistry.AddToContext<std::shared_ptr<sol::state>>(std::make_shared<sol::state>());
		if (!lua)
		{
			lua = std::make_shared<sol::state>();
		}
		lua->open_libraries(sol::lib::base, sol::lib::math, sol::lib::os, sol::lib::table, sol::lib::io, sol::lib::string);
		ENGINE_CORE::Systems::ScriptingSystem::RegisterLuaBindings(*lua, runtimeRegistry);
		ENGINE_CORE::Systems::ScriptingSystem::RegisterLuaFunctions(*lua);
		if (!scriptSystem->LoadMainScript(*lua))
		{
			ENGINE_ERROR("Failed to load the main lua script!");
			return;
		}
		
		m_bPlayScene = true;
		m_bSceneLoaded = true;
		pCurrentScene->SetLoad(true);
	}

	void GameDisplay::UnloadScene()
	{
		m_bPlayScene = false;
		m_bSceneLoaded = false;
		
		auto pCurrentScene = SCENE_MANAGER().GetCurrentScene();
		pCurrentScene->SetLoad(false);
		auto& runtimeRegistry = pCurrentScene->GetRegistry();
		runtimeRegistry.ClearRegistry();
		runtimeRegistry.RemoveContext<std::shared_ptr<ENGINE_RENDERING::Camera3D>>();
		runtimeRegistry.RemoveContext<std::shared_ptr<sol::state>>();
		runtimeRegistry.RemoveContext<std::shared_ptr<rp3d::PhysicsCommon>>();
		runtimeRegistry.RemoveContext<std::shared_ptr<rp3d::PhysicsWorld>>();
		runtimeRegistry.RemoveContext<std::shared_ptr<ENGINE_PHYSICS::ContactListener>>();
		runtimeRegistry.RemoveContext<std::shared_ptr<ENGINE_CORE::Systems::PhysicsSystem>>();
		runtimeRegistry.RemoveContext<std::shared_ptr<ENGINE_CORE::Systems::ScriptingSystem>>();

		auto& mainRegistry = MAIN_REGISTRY();
		mainRegistry.GetMusicPlayer().Stop();
		mainRegistry.GetSoundFxPlayer().Stop(-1);
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
		if (pCurrentScene && m_bPlayScene)
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
			/* now, the scene is loaded when:
			* 1.a scene is drag and drop in scene display
			* 2.play button is pressed
			*/
			LoadScene();
		}
		if (ImGui::GetColorStackSize() > 0)
			ImGui::PopStyleColor(ImGui::GetColorStackSize());
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
			ImGui::SetTooltip("Play Game");

		ImGui::SameLine();

		if (!m_bPlayScene)
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
			&& m_bSceneLoaded)
		{
			UnloadScene();
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
		auto pCurrentScene = SCENE_MANAGER().GetCurrentScene();
		if (!pCurrentScene || !m_bPlayScene)
			return;

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
		//TODO:update camera here
	}
}
