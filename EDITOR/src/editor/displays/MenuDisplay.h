#pragma once
#include"IDisplay.h"
#include <Core/ECS/Registry.h>
#include <Rendering/Core/Camera3D.h>
#include <string>
#include <Rendering/Essentials/LightCulling.h>
#include <utility>

namespace ENGINE_EDITOR {
	class SceneObject;
	class MenuDisplay: public IDisplay
	{
	private:
		bool m_ShowLightBenchmark{ false };
		int m_LightCount{ 100 }, m_LightLayout{ 0 }, m_Seed{ 42 }, m_GeometryCount{ 0 };
		float m_LightRange{ 4.0f };
		bool m_PointShadows{ false }, m_SavedPointShadows{ true };
		std::shared_ptr<SceneObject> m_BenchmarkScene;
		std::vector<entt::entity> m_BenchmarkEntities;
		std::vector<std::pair<std::shared_ptr<ENGINE_RENDERING::Camera3D>, ENGINE_RENDERING::Camera3D>> m_SavedCameras;
		ENGINE_RENDERING::LightingMode m_SavedMode{ ENGINE_RENDERING::LightingMode::Clustered };
		bool m_SavedVisualizations{ true }, m_SavedProfiling{ false }, m_SavedStatistics{ false };
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