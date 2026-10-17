#pragma once
#include <glm/glm.hpp>
#include <cstddef>

namespace ENGINE_RENDERING {
	constexpr int DIR_CASCADE_COUNT = 4;
	constexpr float DIR_CASTER_DISTANCE = 100.0f;

	struct DirLight {
		glm::vec4 color{ 0.0f };
		glm::vec4 direction{ 0.0f };
		glm::mat4 lightSpaceMatrices[DIR_CASCADE_COUNT]{};
		glm::vec4 splitDepths{ 0.0f };	// 各级联在相机视空间中的远端深度（正值）
		glm::vec4 blendStarts{ 0.0f };	// 各级联向下一级过渡的起始深度，最后一级不使用
		glm::vec4 texelSizes{ 0.0f };	// 各级联阴影图单个纹素对应的世界空间宽度
		glm::vec4 depthRanges{ 0.0f };	// 各级联光空间投影的深度跨度，用于计算深度偏移
		glm::vec4 params{ 0.0f };		// x/y：相机近/远裁剪面；z：CSM 有效标记；w：未使用
	};

	struct alignas(16) PointLight {
		glm::vec4 color{ 0.0f };
		glm::vec4 position{ 0.0f };
		glm::vec4 attenuation{ 0.0f };
		glm::ivec4 metadata{ -1, 0, 0, 0 }; // shadow slot, priority, casts shadow, visualization
	};

	static_assert(sizeof(PointLight) == 64);
	static_assert(offsetof(PointLight, position) == 16);
	static_assert(offsetof(PointLight, attenuation) == 32);
	static_assert(offsetof(PointLight, metadata) == 48);

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