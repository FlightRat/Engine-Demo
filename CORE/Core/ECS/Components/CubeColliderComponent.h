#pragma once
#include<glm/glm.hpp>
#include<sol/sol.hpp>

namespace ENGINE_CORE::ECS {
	struct CubeColliderComponent {
		float width{ 0 }, height{ 0 };
		glm::vec3 offset{ glm::vec3(0.0f)};
		bool bColliding{ false };

		static void CreateLuaCubeColliderBind(sol::state& lua);
	};
}