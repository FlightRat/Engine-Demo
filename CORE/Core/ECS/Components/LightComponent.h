#pragma once
#include<glm/glm.hpp>
#include<sol/sol.hpp>

namespace ENGINE_CORE::ECS {
	struct LightComponent 
	{
		glm::vec3 color;

		std::string type = "point_light"; //TODO: make this enum

		// for point light
		glm::vec3 pos;
		float constant = 1.0;
		float linear = 0.09;
		float quadratic = 0.032;
		bool render = false;

		// for direction light
		glm::vec3 direction = glm::vec3(-0.2f, -1.0f, -0.3f);

		static void CreateLuaLightBind(sol::state& lua);
	};
}