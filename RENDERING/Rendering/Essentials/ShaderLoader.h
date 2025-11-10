#pragma once
#include"Shader.h"
#include<memory>

namespace ENGINE_RENDERING {
	class ShaderLoader 
	{
	private:
		static GLuint CreateProgram(const std::string& vertexShader, const std::string& fragmentShader);
		static GLuint CompileShader(GLuint shaderType, const std::string& filepath);
		//from memory
		static GLuint CreateProgram(const char* vertexShader, const char* fragmentShader);
		static GLuint CompileShader(GLuint type, const char* shader);

		static bool CompileSuccess(GLuint shader);
		static bool LinkShader(GLuint program, GLuint vertexShader, GLuint fragmentShader);
		static bool IsProgramValid(GLuint program);

	public:
		ShaderLoader() = delete;
		static std::shared_ptr<Shader> Create(const std::string& vertexShaderPath, const std::string& fragmentShaderPath);
		static std::shared_ptr<Shader> CreateFromMemory(const char* vertexShader, const char* fragmentShader);
		static bool Destroy(Shader* pShader);
	};
}