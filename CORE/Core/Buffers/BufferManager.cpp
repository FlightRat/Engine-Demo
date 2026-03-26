#include "BufferManager.h"
#include "Logger/Logger.h"

namespace ENGINE_CORE::BUFFERS {
	bool BufferManager::AddFrameBuffer(
		const std::string& fboName, 
		ENGINE_RENDERING::BufferType type, 
		int width, int height, bool useRBO)
	{
		if (m_mapFBO.find(fboName) != m_mapFBO.end())
		{
			ENGINE_ERROR("Failed to add FrameBuffer [{0}] -- Already exists!", fboName);
			return false;
		}

		auto framebuffer = std::make_shared<ENGINE_RENDERING::Framebuffer>(type, width, height, useRBO);
		if (!framebuffer)
		{
			ENGINE_ERROR("Failed to create framebuffer [{}]", fboName);
			return false;
		}

		m_mapFBO.emplace(fboName, std::move(framebuffer));
		return true;
	}

	bool BufferManager::AddUniformBuffer(const std::string& uboName, size_t size, int bindingPoint)
	{
		if (m_mapUBO.find(uboName) != m_mapUBO.end())
		{
			ENGINE_ERROR("Failed to add UniformBuffer [{0}] -- Already exists!", uboName);
			return false;
		}

		auto uniformbuffer = std::make_shared<ENGINE_RENDERING::UniformBuffer>(size, bindingPoint);
		if (!uniformbuffer)
		{
			ENGINE_ERROR("Failed to create uniformbuffer [{}]", uboName);
			return false;
		}

		m_mapUBO.emplace(uboName, std::move(uniformbuffer));
		return true;
	}

	std::shared_ptr<ENGINE_RENDERING::Framebuffer> BufferManager::GetFrameBuffer(const std::string& fboName)
	{
		auto framebufferItr = m_mapFBO.find(fboName);
		if (framebufferItr == m_mapFBO.end())
		{
			ENGINE_ERROR("Failed to get framebuffer [{}] -- Does not exists!", fboName);
			return nullptr;
		}
		return framebufferItr->second;
	}

	std::shared_ptr<ENGINE_RENDERING::UniformBuffer> BufferManager::GetUniformBuffer(const std::string& uboName)
	{
		auto uniformbufferItr = m_mapUBO.find(uboName);
		if (uniformbufferItr == m_mapUBO.end())
		{
			ENGINE_ERROR("Failed to get uniformbuffer [{}] -- Does not exists!", uboName);
			return nullptr;
		}
		return uniformbufferItr->second;
	}
}


