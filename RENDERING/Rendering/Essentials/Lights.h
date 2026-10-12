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
	static_assert(offsetof(DirLight, color) == 0);
	static_assert(offsetof(DirLight, direction) == 16);
	static_assert(offsetof(DirLight, lightSpaceMatrices) == 32);
	static_assert(offsetof(DirLight, splitDepths) == 288);
	static_assert(offsetof(DirLight, blendStarts) == 304);
	static_assert(offsetof(DirLight, texelSizes) == 320);
	static_assert(offsetof(DirLight, depthRanges) == 336);
	static_assert(offsetof(DirLight, params) == 352);
	static_assert(sizeof(DirLight) == 368);

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