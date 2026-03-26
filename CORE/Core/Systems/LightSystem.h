#pragma once
#include"../ECS/Registry.h"
#include <glm/glm.hpp> 
#include <vector>
#include "Rendering/Essentials/Lights.h"

namespace ENGINE_CORE::Systems {
	class LightSystem
	{
	private:
		const int MAX_DIR_LIGHTS;
		const int MAX_POINT_LIGHTS;  // 用 const 防止运行时被意外修改
		int ACTIVATED_DIR_LIGHTS{ 0 };
		int ACTIVATED_POINT_LIGHTS{ 0 };

		std::vector<ENGINE_RENDERING::DirLight> m_DirLightData;
		std::vector<ENGINE_RENDERING::PointLight> m_PointLightData;
		std::vector<ENGINE_RENDERING::DirLightExtra> m_DirLightDataExtra;
		std::vector<ENGINE_RENDERING::PointLightExtra> m_PointLightDataExtra;

		// 存储每个方向光的 lightSpaceMatrix，供 Shadow Pass 和 Forward Pass 共用
		// std::vector<glm::mat4> m_DirLightSpaceMatrices;

	public:
		LightSystem(int max_dir_light, int max_point_light);
		~LightSystem() = default;

		void Update(ENGINE_CORE::ECS::Registry& runtimeRegistry);

		inline const int GetMaxDirLights() const{ return MAX_DIR_LIGHTS; }
		inline const int GetMaxPointLights() const { return MAX_POINT_LIGHTS; }
		inline const int GetActivatedDirLights() const { return ACTIVATED_DIR_LIGHTS; }
		inline const int GetActivatedPointLights() const { return ACTIVATED_POINT_LIGHTS; }
	
		inline const std::vector<ENGINE_RENDERING::DirLight>& GetDirLightData() const { return m_DirLightData; }
		inline const std::vector<ENGINE_RENDERING::PointLight>& GetPointLightData() const { return m_PointLightData; }
		inline std::vector<ENGINE_RENDERING::DirLightExtra>& GetDirLightDataExtra() { return m_DirLightDataExtra; }
		inline std::vector<ENGINE_RENDERING::PointLightExtra>& GetPointLightDataExtra() { return m_PointLightDataExtra; }
	};
}