#include "GameDisplay.h"
#include "imgui.h"
#include "Logger/Logger.h"
#include "Core/ECS/MainRegistry.h"
#include "Core/Systems/RenderSystem.h"
#include "Core/Systems/PhysicsSystem.h"
#include "Core/Systems/ScriptingSystem.h"
#include "Core/Buffers/BufferManager.h"
#include "Core/Resources/AssetManager.h"
#include "Core/CoreUtilities/CoreEngineData.h"
#include "Sounds/MusicPlayer/MusicPlayer.h"
#include "Sounds/SoundFxPlayer/SoundFxPlayer.h"
#include "Physics/RP3D_Wrappers.h"
#include "Physics/ContactListener.h"
#include <Core/Systems/ScriptingSystem.h>
#include <Core/Systems/RenderSystem.h>
#include <Rendering/Core/Camera3D.h>
#include "../scene/SceneManager.h"
#include "../scene/SceneObject.h"
#include "../utilities/ImGuiUtils.h"
#include "Core/ECS/Components/TransformComponent.h"
#include "Core/ECS/Components/PhysicsComponent.h"
#include "Core/ECS/Components/Identification.h"
#include "Core/CoreUtilities/CoreEngineData.h"

using namespace ENGINE_CORE::ECS;
using namespace reactphysics3d;
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
		if (!pCurrentScene) 
			return;
		pCurrentScene->SetPlay(true);
		m_bPlayGame = true;

		auto& registry = pCurrentScene->GetRegistry();
		auto view = registry.GetRegistry().view<TransformComponent, PhysicsComponent>();
		for (auto [entity, transform, physics] : view.each())
		{
			// 获取底层的 RP3D 刚体指针
			rp3d::RigidBody* body = physics.GetRigidBody();
			if (!body) continue;

			// --- A. 准备数据 ---
			glm::vec3 pos = transform.position;
			glm::quat rot = transform.rotation_quat;

			rp3d::Transform rp3dTransform(
				rp3d::Vector3(pos.x, pos.y, pos.z),
				rp3d::Quaternion(rot.x, rot.y, rot.z, rot.w)
			);

			// --- B. 核心修复：直接设置刚体位置 ---
			// 这一步告诉物理引擎物体瞬移到了新位置
			body->setTransform(rp3dTransform);

			// --- C. 防止插值抖动 ---
			// 你的 PhysicsSystem 使用了插值 (Previous -> Current)
			// 如果不重置 Previous，第一帧会从 旧位置 插值到 新位置，导致视觉上的“飞入”效果
			physics.SetCurrentTransform(rp3dTransform);
			physics.SetPreviousTransform(rp3dTransform);

			// --- D. 重置动力学状态 (建议) ---
			// 清楚残留的速度，防止物体带着之前的动量飞出去
			//body->setLinearVelocity(rp3d::Vector3(0, 0, 0));
			//body->setAngularVelocity(rp3d::Vector3(0, 0, 0));
			//body->setIsSleeping(false);
		}
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

	GLuint GameDisplay::RenderGame(int width, int height)
    {
        auto scene = SCENE_MANAGER().GetCurrentScene();
        if (!scene) return 0;
        auto& runtimeRegistry = scene->GetRegistry();
        if (!runtimeRegistry.GetRegistry().ctx().contains<std::shared_ptr<ENGINE_RENDERING::Camera3D>>()) return 0;
        const auto& camera = runtimeRegistry.GetContext<std::shared_ptr<ENGINE_RENDERING::Camera3D>>();
        auto& mainRegistry = MAIN_REGISTRY();
        const auto& renderSystem = mainRegistry.GetContext<std::shared_ptr<ENGINE_CORE::Systems::RenderSystem>>();
        auto& buffers = mainRegistry.GetBufferManager();
        const auto fb = buffers.GetFrameBuffer("GAME_FB");
        const auto gb = buffers.GetFrameBuffer("GAME_GB");
        const auto ssao = buffers.GetFrameBuffer("GAME_SSAO");
        const auto blur = buffers.GetFrameBuffer("GAME_SSAO_Blur");
        const auto fxaa = buffers.GetFrameBuffer("GAME_FXAA");
        // Finish all allocation changes before camera setup or any draw command.
        for (const auto& target : {fb, gb, ssao, blur, fxaa}) {
            target->Resize(width, height);
            target->CheckResize();
        }
        camera->SetWidth(width);
        camera->SetHeight(height);
        renderSystem->DeferredRenderPipeline(camera, runtimeRegistry, gb, ssao, blur, fb, fxaa);
		return fxaa->GetTextureID();
    }

	void GameDisplay::DrawToolbar()
	{
		auto& mainRegistry = MAIN_REGISTRY();
		auto& assetManager = mainRegistry.GetAssetManager();

		auto pPlayTexture = assetManager.GetTexture("play_button");
		auto pStopTexture = assetManager.GetTexture("stop_button");
		assert(pPlayTexture && pStopTexture);

		ImGui::Separator();

		// play button
		auto playTextureID = (ImTextureID)(intptr_t)pPlayTexture->GetID();
		if (m_bPlayGame)
		{
			ImGui::ActiveImageButton("##playButton", playTextureID);
		}
		else
		{
			if (ImGui::ImageButton("##playButton", playTextureID, TOOL_BUTTON_SIZE) && SCENE_MANAGER().GetCurrentScene())
			{
				PlayGame();
			}
		}
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
			ImGui::SetTooltip("Play Game");

		ImGui::SameLine();

		// stop button
		auto stopTextureID = (ImTextureID)(intptr_t)pStopTexture->GetID();
		if (!m_bPlayGame)
		{
			ImGui::ActiveImageButton("##stopButton", stopTextureID);
		}
		else
		{
			if (ImGui::ImageButton("##stopButton", stopTextureID, TOOL_BUTTON_SIZE) && SCENE_MANAGER().GetCurrentScene())
			{
				StopGame();
			}
		}
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
			ImGui::SetTooltip("Stop Game");

		ImGui::Separator();
	}

    void GameDisplay::Draw()
    {
        static bool open = true;
        if (!ImGui::Begin("Game", &open)) {
            ImGui::End();
            return;
        }
        DrawToolbar();
        if (ImGui::BeginChild("##GameChild", ImVec2{0.f, 0.f}, false, ImGuiWindowFlags_NoScrollWithMouse)) {
            const ImVec2 available = ImGui::GetContentRegionAvail();
            const int width = static_cast<int>(available.x);
            const int height = static_cast<int>(available.y);
			const GLuint texture = RenderGame(width, height);
            if (texture != 0) {
                ImGui::Image((ImTextureID)(intptr_t)texture,
                    ImVec2{static_cast<float>(width), static_cast<float>(height)},
                    ImVec2{0.f, 1.f}, ImVec2{1.f, 0.f});
            }
        }
        ImGui::EndChild();
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

		scriptSystem->Update(runtimeRegistry);

		// 插值version
		//while (accumulator >= timeStep) {	// TODO: add check "if (coreGlobals.IsPhysicsEnabled())"
		//	physicsWorld->update(timeStep);
		//	accumulator -= timeStep;
		//}
		//decimal factor = accumulator / timeStep;
		//physicsSystem->Update(runtimeRegistry.GetRegistry(), factor);

		// 不插值version
		physicsWorld->update(1.0f/60.0f);
		physicsSystem->Update(runtimeRegistry, 0.0f);
	}
}
