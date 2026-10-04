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
		float half_width;
		float half_height;
		glm::vec4 color{ 0.0f };
		glm::vec4 center_pos{ 0.0f };
		glm::vec4 direction{ 0.0f };
		glm::mat4 lightSpaceMatrix{ 0.0f };
	};
}