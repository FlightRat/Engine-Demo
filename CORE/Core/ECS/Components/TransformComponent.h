#pragma once
#include<glm/glm.hpp>
#include<sol/sol.hpp>
#include<glm/gtx/quaternion.hpp>
#include<glm/gtc/quaternion.hpp>

namespace ENGINE_CORE::ECS {
	struct TransformComponent
	{
		glm::vec3 position{ glm::vec3{0.0f} };
		glm::vec3 scale{ glm::vec3{1.f} };
		glm::vec3 rotation_eular{ glm::vec3{0.0f} };
		glm::quat rotation_quat{ glm::quat{1.0f, 0.0f, 0.0f, 0.0f} };
		static void CreateLuaTransformBind(sol::state& lua);
	};
}