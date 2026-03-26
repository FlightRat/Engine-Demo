#pragma once
#include <glm/glm.hpp>

namespace ENGINE_RENDERING {
	struct DirLight {
		glm::vec4 direction = glm::vec4(0.0f);
		glm::vec4 diffuse = glm::vec4(0.0f);
		glm::vec4 specular = glm::vec4(0.0f);
		glm::vec4 ambient = glm::vec4(0.0f);
		glm::mat4 lightSpaceMatrix = glm::mat4(0.0f);
	};

	struct PointLight {
		glm::vec4 position = glm::vec4(0.0f);
		glm::vec4 diffuse = glm::vec4(0.0f);
		glm::vec4 specular = glm::vec4(0.0f);
		glm::vec4 ambient = glm::vec4(0.0f);
		glm::vec4 attenuation = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
		//bool render{ false };
	};
}