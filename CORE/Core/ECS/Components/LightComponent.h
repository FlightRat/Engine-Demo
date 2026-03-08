#pragma once
#include<glm/glm.hpp>
#include<sol/sol.hpp>

namespace ENGINE_CORE::ECS {
	struct LightComponent 
	{
		glm::vec3 diffuse;
		glm::vec3 specular;
		glm::vec3 ambient;

		std::string type; //TODO: make this enum

		// for point light
		glm::vec3 pos;
		float constant;
		float linear;
		float quadratic;
		bool render;

		// for direction light
		glm::vec3 direction;

		static void CreateLuaLightBind(sol::state& lua);
	};
}