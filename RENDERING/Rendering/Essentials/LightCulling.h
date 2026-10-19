#pragma once

namespace ENGINE_RENDERING {

	enum class LightingMode { Full, Tiled, Clustered };

	constexpr int LIGHT_TILE_SIZE = 16;
	constexpr int CLUSTER_DEPTH_SLICES = 24;
	constexpr int MAX_CLUSTER_LIGHTS = 128;

}
