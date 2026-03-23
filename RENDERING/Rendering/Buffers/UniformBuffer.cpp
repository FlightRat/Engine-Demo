#include "UniformBuffer.h"
#include <Logger/Logger.h>

namespace ENGINE_RENDERING {
	UniformBuffer::UniformBuffer(size_t size, int bindingPoint)
	{
		glGenBuffers(1, &m_UboID);
		glBindBuffer(GL_UNIFORM_BUFFER, m_UboID);
		glBufferData(GL_UNIFORM_BUFFER, size, NULL, GL_STATIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		glBindBufferRange(GL_UNIFORM_BUFFER, bindingPoint, m_UboID, 0, size);
	}

	UniformBuffer::~UniformBuffer()
	{
		CleanUp();
	}

	void UniformBuffer::UpdateUniformBuffer(const void* data, size_t size, size_t offset)
	{
		glBindBuffer(GL_UNIFORM_BUFFER, m_UboID);
		glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	void UniformBuffer::CleanUp()
	{
		glDeleteBuffers(1, &m_UboID);
	}
}


