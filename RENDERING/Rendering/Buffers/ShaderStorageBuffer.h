#pragma once
#include <glad/glad.h>
#include <cstddef>

namespace ENGINE_RENDERING {
	class ShaderStorageBuffer
	{
	private:
		GLuint m_SsboID{ 0 };
		size_t m_Capacity{ 0 };
		GLint64 m_MaxSize{ 0 };
	public:
		ShaderStorageBuffer();
		~ShaderStorageBuffer();
		ShaderStorageBuffer(const ShaderStorageBuffer&) = delete;
		ShaderStorageBuffer& operator=(const ShaderStorageBuffer&) = delete;
		void Reserve(size_t size);
		void Update(const void* data, size_t size);
		void Bind(GLuint bindingPoint) const;
		inline GLuint GetID() const { return m_SsboID; }
		inline size_t Capacity() const { return m_Capacity; }
	};
}
