#pragma once
#include <glm/glm.hpp>

namespace ENGINE_RENDERING {
	struct DirLight {
		glm::vec4 color{ 0.0f };
		glm::vec4 direction{ 0.0f };
		glm::mat4 lightSpaceMatrix{ 0.0f };
	};

	struct PointLight {
		glm::vec4 color{ 0.0f };
		glm::vec4 position{ 0.0f };
		glm::vec4 attenuation{ 0.0f };
		//bool render{ false };
	};

	struct AreaLight {
		glm::vec4 color{ 0.0f };
		glm::vec4 center_pos{ 0.0f };
		glm::vec4 direction{ 0.0f };
		glm::vec4 half_width{ 0.0f };
		glm::vec4 half_height{ 0.0f };
		glm::mat4 lightSpaceMatrix{ 0.0f };
		glm::vec4 shadowParams{ 0.0f }; // 进平面N  远平面F  进平面半宽  进平面半高
	};
}