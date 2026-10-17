#pragma once
#include <glad/glad.h>

namespace ENGINE_RENDERING {
	class ShaderStorageBuffer {
	private:
		GLuint m_SSboID;
		GLsizeiptr m_Capacity{ 16 };

	public:
		ShaderStorageBuffer(int bindingPoint);
		~ShaderStorageBuffer();

		ShaderStorageBuffer(const ShaderStorageBuffer&) = delete;
		ShaderStorageBuffer& operator=(const ShaderStorageBuffer&) = delete;

		void Bind(int bindingPoint) const;
		void Reserve(GLsizeiptr size);
		void UpdateShaderStorageBuffer(const void* data, GLsizeiptr size);
	};
}