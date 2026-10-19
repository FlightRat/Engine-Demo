#include "ShaderStorageBuffer.h"
#include <algorithm>
#include <stdexcept>
#include <Logger/Logger.h>

ENGINE_RENDERING::ShaderStorageBuffer::ShaderStorageBuffer(int bindingPoint)
{
	glGetInteger64v(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &m_MaxSize);
	glCreateBuffers(1, &m_SSboID);
	glNamedBufferData(m_SSboID, m_Capacity, nullptr, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, m_SSboID);
}

ENGINE_RENDERING::ShaderStorageBuffer::~ShaderStorageBuffer()
{
	glDeleteBuffers(1, &m_SSboID);
}

void ENGINE_RENDERING::ShaderStorageBuffer::Bind(int bindingPoint) const
{
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, m_SSboID);
}

void ENGINE_RENDERING::ShaderStorageBuffer::Reserve(GLsizeiptr size)
{
	if (size <= m_Capacity)
		return;
	if (size > m_MaxSize) {
		ENGINE_ERROR("SSBO request [{}] exceeds device limit [{}].", size, m_MaxSize);
		throw std::length_error("Shader storage buffer capacity exceeded");
	}
	m_Capacity = (std::max)(size, m_Capacity > m_MaxSize / 2 ? m_MaxSize : m_Capacity * 2);
	glNamedBufferData(m_SSboID, m_Capacity, nullptr, GL_DYNAMIC_DRAW);
}

void ENGINE_RENDERING::ShaderStorageBuffer::UpdateShaderStorageBuffer(const void* data, GLsizeiptr size)
{
	if (size == 0) return;
	Reserve(size);
	glNamedBufferSubData(m_SSboID, 0, size, data);
}