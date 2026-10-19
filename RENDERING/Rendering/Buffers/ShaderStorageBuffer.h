#pragma once
#include <glad/glad.h>

namespace ENGINE_RENDERING {
	class ShaderStorageBuffer {
	private:
		GLuint m_SSboID;
		GLsizeiptr m_Capacity{ 16 };
		GLint64 m_MaxSize{ 0 };

	public:
		ShaderStorageBuffer(int bindingPoint);
		~ShaderStorageBuffer();

		ShaderStorageBuffer(const ShaderStorageBuffer&) = delete;
		ShaderStorageBuffer& operator=(const ShaderStorageBuffer&) = delete;

		GLuint GetID() const { return m_SSboID; }
		GLsizeiptr Capacity() const { return m_Capacity; }
		void Bind(int bindingPoint) const;
		void Reserve(GLsizeiptr size);
		void UpdateShaderStorageBuffer(const void* data, GLsizeiptr size);
	};
}