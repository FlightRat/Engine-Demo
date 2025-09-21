#pragma once
#include<glm/glm.hpp>
#include<sol/sol.hpp>

namespace CORE::ECS {
	struct SphereColliderComponent {
		float radius{ 0.0f };
		glm::vec3 offset{ glm::vec3(0.0f) };
		bool bColliding{ false };

		static void CreateLuaSphereColliderBind(sol::state& lua);
	};
}