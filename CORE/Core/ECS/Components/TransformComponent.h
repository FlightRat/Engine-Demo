#pragma once
#include<glm/glm.hpp>
#include<sol/sol.hpp>

namespace CORE::ECS {
	struct TransformComponent
	{
		glm::vec3 position{ glm::vec3{0.f} };
		glm::vec3 scale{ glm::vec3{1.f} };
		static void CreateLuaTransformBind(sol::state& lua);
	};
}