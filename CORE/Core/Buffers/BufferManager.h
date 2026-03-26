#pragma once
#include<map>
#include<memory>
#include<string>
#include"Rendering/Buffers/Framebuffer.h"
#include"Rendering/Buffers/UniformBuffer.h"

namespace ENGINE_CORE::BUFFERS {
	class BufferManager
	{
	private:
		// CAN std::map<std::string, std::shared_ptr<xxx>> «∑Ò∫œ  £ø£ø£ø
		std::map<std::string, std::shared_ptr<ENGINE_RENDERING::Framebuffer>> m_mapFBO{};
		std::map<std::string, std::shared_ptr<ENGINE_RENDERING::UniformBuffer>> m_mapUBO{};

	public:
		BufferManager() = default;
		~BufferManager() = default;

		bool AddFrameBuffer(const std::string& fboNme, ENGINE_RENDERING::BufferType type, int width, int height, bool useRBO);
		bool AddUniformBuffer(const std::string& uboName, size_t size, int bindingPoint);

		inline const std::map<std::string, std::shared_ptr<ENGINE_RENDERING::Framebuffer>>& GetAllFBO() const { return m_mapFBO; }
		inline const std::map<std::string, std::shared_ptr<ENGINE_RENDERING::UniformBuffer>>& GetAllUBO() const { return m_mapUBO; }

		std::shared_ptr<ENGINE_RENDERING::Framebuffer> GetFrameBuffer(const std::string& fboNme);
		std::shared_ptr<ENGINE_RENDERING::UniformBuffer> GetUniformBuffer(const std::string& uboName);
	};
}