#pragma once
#include"IDisplay.h"
#include <Core/ECS/Registry.h>
#include <Rendering/Core/Camera3D.h>
#include <string>
#include <utility>

namespace ENGINE_EDITOR {
	class SceneObject;
	class MenuDisplay: public IDisplay
	{
	private:
		bool m_ShowLightBenchmark{ false };
		int m_LightCount{ 100 }, m_LightLayout{ 0 }, m_Seed{ 42 }, m_GeometryCount{ 0 };
		float m_LightRange{ 4.0f };
		int m_ShadowSlots{ 0 }, m_ShadowResolution{ 512 };
		std::shared_ptr<SceneObject> m_BenchmarkScene;
		std::vector<entt::entity> m_BenchmarkEntities;
		std::vector<std::pair<std::shared_ptr<ENGINE_RENDERING::Camera3D>, ENGINE_RENDERING::Camera3D>> m_SavedCameras;
		int m_SavedSlots{ 8 }, m_SavedResolution{ 2048 };
		bool m_SavedTiled{ true }, m_SavedVisualizations{ true }, m_SavedProfiling{ false }, m_SavedStatistics{ false };
		std::string m_SavedLabel;
		void GenerateLightBenchmark();
		void ClearLightBenchmark();
		void DrawLightBenchmark();
	public:
		MenuDisplay() = default;
		~MenuDisplay() = default;
		virtual void Draw() override;
	};
}