#include "SceneDisplay.h"
#include "Core/ECS/MainRegistry.h"
#include "Core/Systems/RenderSystem.h"
#include "Core/Systems/PhysicsSystem.h"
#include "Core/Systems/ScriptingSystem.h"
#include "Core/Resources/AssetManager.h"
#include "Core/Scripting/InputManager.h"
#include "Core/CoreUtilities/CoreEngineData.h"
#include "Rendering/Core/Camera3D.h"
#include "Physics/RP3D_Wrappers.h"
#include "Physics/ContactListener.h"
#include "../utilities/editor_framebuffers.h"
#include "../utilities/editor_utilities.h"
#include "../scene/SceneManager.h"
#include "../scene/SceneObject.h"
#include "../tools/ToolManager.h"
#include "Logger/Logger.h"
#include <imgui.h>
#include "Windowing/Inputs/Keyboard.h"
#include "Windowing/Inputs/Mouse.h"

using namespace ENGINE_CORE::Systems;

namespace ENGINE_EDITOR {
	void SceneDisplay::LoadScnne()
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
		auto scriptSystem = std::make_shared<ScriptingSystem>();
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
		if (!scriptSystem->LoadMainScript(runtimeRegistry, *lua))
		{
			ENGINE_ERROR("Failed to load the main lua script!");
			return;
		}
	}

	void SceneDisplay::RenderScene()
	{
		auto pCurrentScene = SCENE_MANAGER().GetCurrentScene();
		if (!pCurrentScene) 
			return;

		auto& mainRegistry = MAIN_REGISTRY();
		auto& renderSystem = mainRegistry.GetContext<std::shared_ptr<ENGINE_CORE::Systems::RenderSystem>>();
		auto& editorFramebuffer = mainRegistry.GetContext<std::shared_ptr<ENGINE_EDITOR::Editorframebuffers>>();
		const auto& fb = editorFramebuffer->mapFramebuffers[ENGINE_EDITOR::FramebufferType::SCENE];
		auto& runtimeRegistry = pCurrentScene->GetRegistry();
		m_pSceneCam->SetWidth(fb->Width());
		m_pSceneCam->SetHeight(fb->Height());
		renderSystem->ExecuteRenderPipeline(m_pSceneCam, runtimeRegistry, fb);
	}

	void SceneDisplay::LoadNewScene()
	{
		// TODO
	}

	SceneDisplay::SceneDisplay() :m_pSceneCam{ std::make_shared<ENGINE_RENDERING::Camera3D>(glm::vec3(0.0f, 20.0f, -20.0f), glm::vec3(0.0f, 1.0f, 0.0f), 90.0f, -45.0f) }
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

			/*
			* TODO: 
			check and change the bool for controling camera, and move "ControlCam" to update
			refer Ep.66 - Tilemap Editor (Part 6): Finish Tile Tool, Camera Zoom, and More!  --Time 50min
			*/ 
			ControlCam();

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

					LoadScnne();
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::EndChild();
		}
		ImGui::End();
	}

	void SceneDisplay::Update()
	{
		auto pCurrentScene = SCENE_MANAGER().GetCurrentScene();
		if (!pCurrentScene)
			return;
		//ControlCam();
		//TODO:move the camera in scene display
	}

	void SceneDisplay::ControlCam()
	{
		// 1. 判断是否处于交互状态
		// IsWindowHovered: 鼠标是否悬停在当前窗口（SceneChild）上
		// IsMouseDown(1): 鼠标右键是否按下 (0:左键, 1:右键, 2:中键)
		bool isHovered = ImGui::IsWindowHovered();
		bool isRightClicking = ImGui::IsMouseDown(ImGuiMouseButton_Right);

		if (isHovered && isRightClicking)
		{
			// 锁定该窗口为焦点，防止鼠标移出后操作中断（可选，视体验而定）
			// ImGui::SetWindowFocus(); 

			// 获取 ImGui 的 IO 状态
			ImGuiIO& io = ImGui::GetIO();
			auto& coreGlobals = CORE_GLOBALS();
			double deltaTime = coreGlobals.GetDeltaTime();

			// --- 鼠标旋转 ---
			// 使用 ImGui 提供的 MouseDelta，这是相对于上一帧的鼠标位移，不依赖屏幕绝对坐标
			if (io.MouseDelta.x != 0 || io.MouseDelta.y != 0)
			{
				// 注意：ImGui 的 Y 轴通常向下，而 OpenGL 相机可能需要反转 Y 轴，
				// 如果感觉旋转方向反了，把 delta_y 改为 -io.MouseDelta.y
				float delta_x = io.MouseDelta.x;
				float delta_y = -io.MouseDelta.y;
				m_pSceneCam->ProcessMouseMovement(delta_x, delta_y);
			}

			// --- 键盘移动 (上下左右) ---
			// 这里我们可以直接用 ImGui 的键盘状态，比全局 InputManager 更适合编辑器环境
			// 这样当你按 W 时，如果焦点在其他输入框，不会导致相机移动
			if (ImGui::IsKeyDown(ImGuiKey_UpArrow))
				m_pSceneCam->ProcessKeyboard(ENGINE_RENDERING::FORWARD, deltaTime);
			if (ImGui::IsKeyDown(ImGuiKey_DownArrow))
				m_pSceneCam->ProcessKeyboard(ENGINE_RENDERING::BACKWARD, deltaTime);
			if (ImGui::IsKeyDown(ImGuiKey_LeftArrow))
				m_pSceneCam->ProcessKeyboard(ENGINE_RENDERING::LEFT, deltaTime);
			if (ImGui::IsKeyDown(ImGuiKey_RightArrow))
				m_pSceneCam->ProcessKeyboard(ENGINE_RENDERING::RIGHT, deltaTime);
			//if (ImGui::IsKeyDown(ImGuiKey_Q)) // 下降
			//	m_pSceneCam->ProcessKeyboard(ENGINE_RENDERING::DOWN, deltaTime); // 假设你的相机类支持 DOWN
			//if (ImGui::IsKeyDown(ImGuiKey_E)) // 上升
			//	m_pSceneCam->ProcessKeyboard(ENGINE_RENDERING::UP, deltaTime);   // 假设你的相机类支持 UP
		}
	}
}

