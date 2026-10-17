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
		std::vector<ENGINE_RENDERING::DirLight> m_DirLightData;
		std::vector<ENGINE_RENDERING::PointLight> m_PointLightData;
		std::vector<ENGINE_RENDERING::AreaLight> m_AreaLightData;

	public:
		LightSystem() = default;
		~LightSystem() = default;

		void Update(ENGINE_CORE::ECS::Registry& runtimeRegistry, const ENGINE_RENDERING::Camera3D& camera);

		void PrepareDirLightCSM(const ENGINE_RENDERING::Camera3D& camera, ENGINE_RENDERING::DirLight& dirLight);

		inline int GetActivatedDirLights() const { return static_cast<int>(m_DirLightData.size()); }
		inline int GetActivatedPointLights() const { return static_cast<int>(m_PointLightData.size()); }
		inline int GetActivatedAreaLights() const { return static_cast<int>(m_AreaLightData.size()); }

		inline const std::vector<ENGINE_RENDERING::DirLight>& GetDirLightData() const { return m_DirLightData; }
		inline const std::vector<ENGINE_RENDERING::PointLight>& GetPointLightData() const { return m_PointLightData; }
		inline const std::vector<ENGINE_RENDERING::AreaLight>& GetAreaLightData() const { return m_AreaLightData; }
	};
}