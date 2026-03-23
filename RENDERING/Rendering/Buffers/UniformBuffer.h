#pragma once
#include <memory>
#include <glad/glad.h>

namespace ENGINE_RENDERING {
	class UniformBuffer
	{
	private:
		GLuint m_UboID;
	private:
		void CleanUp();
	public:
		UniformBuffer(size_t size, int bindingPoint);
		~UniformBuffer();

		void UpdateUniformBuffer(const void* data, size_t size, size_t offset);
	};
}