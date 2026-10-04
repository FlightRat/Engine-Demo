#pragma once
#include<glm/glm.hpp>
#include<sol/sol.hpp>

namespace ENGINE_CORE::ECS {
	struct LightComponent 
	{
		glm::vec3 color{ 1.0f };
		float intensity{ 255.0f };

		std::string type = "point_light"; //TODO: make this enum

		glm::vec3 pos{ 0.0f };	// for pointLight & areaLight
		float constant = 1.0;
		float linear = 0.09;
		float quadratic = 0.032;
		bool render = false;

		// for directionLight & areaLight
		glm::vec3 direction = glm::vec3(-0.2f, -1.0f, -0.3f);

		// for areaLight
		float half_width{ 0.0f };
		float half_height{ 0.0f };

		static void CreateLuaLightBind(sol::state& lua);
	};
}