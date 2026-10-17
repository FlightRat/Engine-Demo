#define NOMINMAX
#include "MenuDisplay.h"
#include <imgui.h>
#include <random>
#include <algorithm>
#include <cmath>
#include <Core/ECS/MainRegistry.h>
#include <Core/Systems/LightSystem.h>
#include <Core/Systems/RenderSystem.h>
#include <Core/Resources/AssetManager.h>
#include <Core/ECS/Components/LightComponent.h>
#include <Core/ECS/Components/TransformComponent.h>
#include <Core/ECS/Components/MeshFilter.h>
#include <Core/ECS/Components/MeshRender.h>
#include <Core/ECS/Components/Identification.h>
#include "SceneDisplay.h"
#include "SDL.h"
#include "FileSystem/Dialogs/FileDialog.h"
#include "Core/Loaders/SceneLoader.h"
#include "../scene/SceneManager.h"
#include "../scene/SceneObject.h"

#include "Logger/Logger.h"

namespace ENGINE_EDITOR {

	void MenuDisplay::ClearLightBenchmark()
	{
		if (!m_BenchmarkScene) return;
		auto& registry = m_BenchmarkScene->GetRegistry().GetRegistry();
		for (auto entity : m_BenchmarkEntities)
			if (registry.valid(entity)) registry.destroy(entity);
		m_BenchmarkEntities.clear();
		for (auto& [camera, saved] : m_SavedCameras) *camera = saved;
		m_SavedCameras.clear();
		auto renderer = MAIN_REGISTRY().GetContext<std::shared_ptr<ENGINE_CORE::Systems::RenderSystem>>();
		auto lights = MAIN_REGISTRY().GetContext<std::shared_ptr<ENGINE_CORE::Systems::LightSystem>>();
		lights->shadowSlotCount = m_SavedSlots;
		lights->shadowResolution = m_SavedResolution;
		renderer->useTiledLighting = m_SavedTiled;
		renderer->showLightVisualizations = m_SavedVisualizations;
		renderer->profilingEnabled = m_SavedProfiling;
		renderer->collectLightStatistics = m_SavedStatistics;
		renderer->benchmarkLabel = m_SavedLabel;
		m_BenchmarkScene.reset();
	}

	void MenuDisplay::GenerateLightBenchmark()
	{
		using namespace ENGINE_CORE::ECS;
		ClearLightBenchmark();
		auto scene = SCENE_MANAGER().GetCurrentScene();
		if (!scene) return;
		m_BenchmarkScene = scene;
		auto& registry = scene->GetRegistry().GetRegistry();
		auto renderer = MAIN_REGISTRY().GetContext<std::shared_ptr<ENGINE_CORE::Systems::RenderSystem>>();
		auto lights = MAIN_REGISTRY().GetContext<std::shared_ptr<ENGINE_CORE::Systems::LightSystem>>();
		m_SavedSlots = lights->shadowSlotCount;
		m_SavedResolution = lights->shadowResolution;
		m_SavedTiled = renderer->useTiledLighting;
		m_SavedVisualizations = renderer->showLightVisualizations;
		m_SavedProfiling = renderer->profilingEnabled;
		m_SavedStatistics = renderer->collectLightStatistics;
		m_SavedLabel = renderer->benchmarkLabel;
		lights->shadowSlotCount = m_ShadowSlots;
		lights->shadowResolution = m_ShadowResolution;
		renderer->showLightVisualizations = false;
		renderer->benchmarkLabel = "seed=" + std::to_string(m_Seed) + ";layout=" + std::to_string(m_LightLayout)
			+ ";range=" + std::to_string(m_LightRange) + ";geometry=" + std::to_string(m_GeometryCount);
		auto saveCamera = [&](std::shared_ptr<ENGINE_RENDERING::Camera3D> camera) {
			m_SavedCameras.emplace_back(camera, *camera);
			camera->Position = glm::vec3(0.0f, 28.0f, 35.0f);
			camera->LookAt(glm::vec3(0.0f));
			camera->Zoom = 60.0f;
			camera->FarPlane = 200.0f;
		};
		auto& holder = MAIN_REGISTRY().GetContext<std::shared_ptr<DisplayHolder>>();
		for (auto& display : holder->displays)
			if (auto sceneDisplay = dynamic_cast<SceneDisplay*>(display.get())) saveCamera(sceneDisplay->GetCamera());
		if (registry.ctx().contains<std::shared_ptr<ENGINE_RENDERING::Camera3D>>())
			saveCamera(scene->GetRegistry().GetContext<std::shared_ptr<ENGINE_RENDERING::Camera3D>>());
		auto addMesh = [&](glm::vec3 position, glm::vec3 scale) {
			auto entity = registry.create();
			m_BenchmarkEntities.push_back(entity);
			auto& id = registry.emplace<Identification>(entity);
			id.name = "Benchmark geometry";
			id.group = "benchmark";
			id.entity_id = static_cast<int32_t>(entt::to_integral(entity));
			auto& transform = registry.emplace<TransformComponent>(entity);
			transform.position = position;
			transform.scale = scale;
			registry.emplace<MeshFilter>(entity);
			auto& mesh = registry.emplace<MeshRender>(entity);
			mesh.ResetMaterial(MAIN_REGISTRY().GetAssetManager().GetModel("cube")->GetMeshes());
			for (auto& material : mesh.materials) {
				material.color = glm::vec4(0.6f, 0.6f, 0.6f, 1.0f);
				material.metallic = 0.0f;
				material.roughness = 0.8f;
			}
		};
		addMesh(glm::vec3(0.0f, -0.2f, 0.0f), glm::vec3(64.0f, 0.2f, 64.0f));
		if (m_LightLayout == 3) addMesh(glm::vec3(0.0f, -4.0f, -50.0f), glm::vec3(64.0f, 0.2f, 64.0f));
		for (int i = 0; i < m_GeometryCount; ++i)
			addMesh(glm::vec3((i % 32) * 2.0f - 31.0f, 0.5f, (i / 32) * 2.0f - 31.0f), glm::vec3(0.5f));
		std::mt19937 random(static_cast<unsigned int>(m_Seed));
		auto next = [&]() { return static_cast<float>(random() >> 8) / 16777216.0f; };
		for (int i = 0; i < m_LightCount; ++i) {
			auto entity = registry.create();
			m_BenchmarkEntities.push_back(entity);
			auto& id = registry.emplace<Identification>(entity);
			id.name = "Benchmark light " + std::to_string(i);
			id.group = "benchmark";
			id.entity_id = static_cast<int32_t>(entt::to_integral(entity));
			auto& light = registry.emplace<LightComponent>(entity);
			light.pos = glm::vec3(next() * 60.0f - 30.0f, 1.5f, next() * 60.0f - 30.0f);
			if (m_LightLayout == 1) light.pos *= glm::vec3(0.04f, 1.0f, 0.04f);
			if (m_LightLayout == 2) light.pos.x += 100.0f;
			if (m_LightLayout == 3 && i % 2 == 0) { light.pos.z -= 50.0f; light.pos.y -= 4.0f; }
			light.color = glm::vec3(0.2f + next() * 0.8f, 0.2f + next() * 0.8f, 0.2f + next() * 0.8f);
			light.intensity = 3.0f;
			light.range = m_LightRange;
			light.castsShadow = true;
			registry.emplace<TransformComponent>(entity).position = light.pos;
		}
		renderer->StartLightBenchmark();
	}

	void MenuDisplay::DrawLightBenchmark()
	{
		if (!ImGui::Begin("Multi-light Benchmark", &m_ShowLightBenchmark)) { ImGui::End(); return; }
		auto renderer = MAIN_REGISTRY().GetContext<std::shared_ptr<ENGINE_CORE::Systems::RenderSystem>>();
		auto lights = MAIN_REGISTRY().GetContext<std::shared_ptr<ENGINE_CORE::Systems::LightSystem>>();
		ImGui::InputInt("Lights", &m_LightCount);
		m_LightCount = (std::max)(0, m_LightCount);
		for (int count : {5, 100, 256, 1024}) {
			ImGui::SameLine();
			if (ImGui::Button(std::to_string(count).c_str())) m_LightCount = count;
		}
		ImGui::InputInt("Seed", &m_Seed);
		ImGui::Combo("Layout", &m_LightLayout, "Sparse\0Overlapping\0Offscreen\0Depth discontinuity\0");
		ImGui::DragFloat("Range", &m_LightRange, 0.1f, 0.1f, 1000.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
		ImGui::SliderInt("Extra geometry", &m_GeometryCount, 0, 1024);
		ImGui::SliderInt("Shadow slots", &m_ShadowSlots, 0, 16);
		ImGui::InputInt("Shadow face size", &m_ShadowResolution);
		m_ShadowResolution = (std::max)(1, m_ShadowResolution);
		if (ImGui::Button("Generate / reset camera")) GenerateLightBenchmark();
		ImGui::SameLine();
		if (ImGui::Button("Clear / restore")) ClearLightBenchmark();
		if (ImGui::Checkbox("Tiled lighting", &renderer->useTiledLighting)) renderer->StartLightBenchmark();
		ImGui::Checkbox("Light visualizations", &renderer->showLightVisualizations);
		ImGui::Checkbox("GPU timing", &renderer->profilingEnabled);
		if (ImGui::Checkbox("Candidate diagnostics (separate run)", &renderer->collectLightStatistics)) renderer->StartLightBenchmark();
		if (ImGui::Button("Capture 120 warmup + 300 samples")) renderer->StartLightBenchmark();
		ImGui::SameLine();
		if (ImGui::Button("Export CSV")) renderer->ExportLightBenchmark("multi-light-benchmark.csv");
		ImGui::Text("Actual lights: %d | shadowed: %d / %d", lights->GetActivatedPointLights(), lights->GetShadowedPointLights(), lights->shadowSlotCount);
		for (const auto& [id, view] : renderer->GetLightViews()) {
			const auto& s = view->metrics.Latest();
			ImGui::Text("View %p (%dx%d), samples %d/300", static_cast<const void*>(id), view->width, view->height, static_cast<int>(view->metrics.Samples().size()));
			ImGui::Text("GPU ms: geometry %.3f, culling %.3f, lighting %.3f, shadow %.3f", s.gpuMs[0], s.gpuMs[1], s.gpuMs[2], s.gpuMs[3]);
			ImGui::Text("Candidates avg %.1f, max %u, overflow %u", s.averageCandidates, s.maxCandidates, s.overflowTiles);
		}
		ImGui::End();
	}

	void MenuDisplay::Draw()
	{
        if (m_BenchmarkScene && m_BenchmarkScene != SCENE_MANAGER().GetCurrentScene()) ClearLightBenchmark();
        if (m_ShowLightBenchmark) DrawLightBenchmark();
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("New", "Ctrl + N"))
				{
					ENGINE_LOG("New is pressed!")
				}
				if (ImGui::MenuItem("Open", "Ctrl + O"))
				{
					ENGINE_FileSystem::FileDialog fd{};
					auto file = fd.OpenFileDialog("Open test", SDL_GetBasePath(), { "*.json" });
					if (!file.empty())
					{
						auto pCurrentScene = SCENE_MANAGER().GetCurrentScene();
						if (pCurrentScene)
						{
							ENGINE_CORE::Loaders::SceneLoader sl{};
							if (!sl.LoadScene(pCurrentScene->GetRegistry(), file, true))
							{
								ENGINE_ERROR("Failed to load scene.");
							}
						}
						else
						{
							ENGINE_ERROR("Failed to save scene.No scene activated!");
						}
					}
				}
				if (ImGui::MenuItem("Save", "Ctrl + S"))
				{
					ENGINE_FileSystem::FileDialog fd{};
					auto file = fd.SaveFileDialog("Save scene test", SDL_GetBasePath(), { "*.json"});
					if (!file.empty())
					{
						auto pCurrentScene = SCENE_MANAGER().GetCurrentScene();
						if (pCurrentScene)
						{
							ENGINE_CORE::Loaders::SceneLoader sl{};
							if (!sl.SaveScene(pCurrentScene->GetRegistry(), file, true))
							{
								ENGINE_ERROR("Failed to save scene!");
							}
						}
						else
						{
							ENGINE_ERROR("Failed to save scene.No scene activated!");
						}
					}
				}
				if (ImGui::MenuItem("Exit"))
				{
					ENGINE_LOG("There is no exit func now!")
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Edit"))
			{
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Tools"))
			{
                ImGui::MenuItem("Multi-light Benchmark", nullptr, &m_ShowLightBenchmark);
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Settings"))
			{
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Help"))
			{
				ImGui::EndMenu();
			}
			
			ImGui::EndMainMenuBar();
		}
	}
}


