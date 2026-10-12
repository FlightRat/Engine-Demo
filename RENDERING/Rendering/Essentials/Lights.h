#pragma once
#include <glm/glm.hpp>
#include <cstddef>

namespace ENGINE_RENDERING {
	constexpr int DIR_CASCADE_COUNT = 4;
	constexpr int DIR_SHADOW_SIZE = 2048;
	constexpr float DIR_CASTER_DISTANCE = 100.0f;

	struct DirLight {
		glm::vec4 color{ 0.0f };
		glm::vec4 direction{ 0.0f };
		glm::mat4 lightSpaceMatrices[DIR_CASCADE_COUNT]{};
		glm::vec4 splitDepths{ 0.0f };
		glm::vec4 blendStarts{ 0.0f };
		glm::vec4 texelSizes{ 0.0f };
		glm::vec4 depthRanges{ 0.0f };
		glm::vec4 params{ 0.0f };
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
		glm::vec4 rightVector_halfWidth{ 0.0f };
		glm::vec4 upVector_halfHeight{ 0.0f };
		glm::mat4 lightSpaceMatrix{ 0.0f };
		glm::vec4 shadowParams{ 0.0f }; // 进平面N  远平面F  进平面半宽  进平面半高
	};
}