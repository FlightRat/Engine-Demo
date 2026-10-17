#include "ShaderStorageBuffer.h"
#include <algorithm>
#include <stdexcept>
#include <Logger/Logger.h>

namespace ENGINE_RENDERING {
	ShaderStorageBuffer::ShaderStorageBuffer()
	{
		glGetInteger64v(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &m_MaxSize);
		Reserve(16);
	}

	ShaderStorageBuffer::~ShaderStorageBuffer()
	{
		glDeleteBuffers(1, &m_SsboID);
	}

	void ShaderStorageBuffer::Reserve(size_t size)
	{
		if (size <= m_Capacity) return;
		if (size > static_cast<size_t>(m_MaxSize)) {
			ENGINE_ERROR("SSBO request [{}] exceeds device limit [{}].", size, m_MaxSize);
			throw std::length_error("Shader storage buffer capacity exceeded");
		}
		m_Capacity = (std::min)(static_cast<size_t>(m_MaxSize), (std::max)(size, m_Capacity * 2));
		glDeleteBuffers(1, &m_SsboID);
		glCreateBuffers(1, &m_SsboID);
		glNamedBufferStorage(m_SsboID, m_Capacity, nullptr, GL_DYNAMIC_STORAGE_BIT);
	}

	void ShaderStorageBuffer::Update(const void* data, size_t size)
	{
		Reserve(size);
		if (size != 0) glNamedBufferSubData(m_SsboID, 0, size, data);
	}

	void ShaderStorageBuffer::Bind(GLuint bindingPoint) const
	{
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, m_SsboID);
	}
}
