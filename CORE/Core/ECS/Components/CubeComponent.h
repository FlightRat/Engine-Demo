#pragma once
#include<glm/glm.hpp>

namespace CORE::ECS {
	struct CubeComponent
	{
		glm::vec3 position{ glm::vec3{0.f} };
		glm::vec3 scale{ glm::vec3{1.f} };
	};
}