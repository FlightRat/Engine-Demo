#pragma once
#include <map>
#include <memory>
#include <Rendering/Buffers/Framebuffer.h>
#include <Rendering/Buffers/ShaderStorageBuffer.h>

using ViewportKey = std::weak_ptr<ENGINE_RENDERING::Framebuffer>;

namespace ENGINE_CORE::Systems {
	struct TileHeader
	{
		GLuint count{ 0 };
		GLfloat minDepth{ 0.0f };
		GLfloat maxDepth{ 0.0f };
		GLuint validPixelCount{ 0 };
	};

	struct TileBuffers
	{
		int tileCountX{ 0 };
		int tileCountY{ 0 };

		ENGINE_RENDERING::ShaderStorageBuffer headers{ 4 };
		ENGINE_RENDERING::ShaderStorageBuffer indices{ 5 };
	};

	class TileSystem
	{
	private:
		
		static constexpr int TILE_SIZE = 16;
		static constexpr int MAX_TILE_LIGHTS = 128;
		std::map<ViewportKey, TileBuffers, std::owner_less<ViewportKey>> m_TileBuffers;

	public:
		TileSystem() = default;
		~TileSystem() = default;

		void Bind(const std::shared_ptr<ENGINE_RENDERING::Framebuffer>& intermediateGB);
		void Prepare(const std::shared_ptr<ENGINE_RENDERING::Framebuffer>& intermediateGB);
		void Update(const std::shared_ptr<ENGINE_RENDERING::Framebuffer>& intermediateGB);
	};
}

