#pragma once
#include"../ECS/Registry.h"
#include <glm/glm.hpp> 
#include <vector>
#include "Rendering/Essentials/Lights.h"
#include "Rendering/Core/Camera3D.h"

namespace ENGINE_CORE::Systems {
	class LightSystem
	{
	private:
		const int MAX_DIR_LIGHTS;
		const int MAX_AREA_LIGHTS;
		int ACTIVATED_DIR_LIGHTS{ 0 };
		int ACTIVATED_POINT_LIGHTS{ 0 };
		int ACTIVATED_AREA_LIGHTS{ 0 };
		std::vector<ENGINE_RENDERING::DirLight> m_DirLightData;
		std::vector<ENGINE_RENDERING::PointLight> m_PointLightData;
		std::vector<ENGINE_RENDERING::AreaLight> m_AreaLightData;

		std::vector<entt::entity> m_PointEntities;
		std::vector<unsigned int> m_GlobalPointIndices, m_LocalPointIndices;
		ENGINE_CORE::ECS::Registry* m_ShadowRegistry{ nullptr };
		std::vector<entt::entity> m_ShadowOwners;
		int m_ShadowedPointLights{ 0 };
		void AssignPointShadows(ENGINE_CORE::ECS::Registry& registry);
	public:
		LightSystem(int max_dir_light, int max_area_light);
		~LightSystem() = default;

		void Update(ENGINE_CORE::ECS::Registry& runtimeRegistry, const ENGINE_RENDERING::Camera3D& camera);

		void PrepareDirLightCSM(const ENGINE_RENDERING::Camera3D& camera, ENGINE_RENDERING::DirLight& dirLight);

		inline const int GetMaxDirLights() const{ return MAX_DIR_LIGHTS; }
		int shadowSlotCount{ 8 };
		int shadowResolution{ 2048 };
		const std::vector<unsigned int>& GetGlobalPointIndices() const { return m_GlobalPointIndices; }
		const std::vector<unsigned int>& GetLocalPointIndices() const { return m_LocalPointIndices; }
		int GetShadowedPointLights() const { return m_ShadowedPointLights; }
		inline const int GetMaxAreaLights() const { return MAX_AREA_LIGHTS; }

		inline const int GetActivatedDirLights() const { return ACTIVATED_DIR_LIGHTS; }
		inline const int GetActivatedPointLights() const { return ACTIVATED_POINT_LIGHTS; }
		inline const int GetActivatedAreaLights() const { return ACTIVATED_AREA_LIGHTS; }

		inline const std::vector<ENGINE_RENDERING::DirLight>& GetDirLightData() const { return m_DirLightData; }
		inline const std::vector<ENGINE_RENDERING::PointLight>& GetPointLightData() const { return m_PointLightData; }
		inline const std::vector<ENGINE_RENDERING::AreaLight>& GetAreaLightData() const { return m_AreaLightData; }
	};
}