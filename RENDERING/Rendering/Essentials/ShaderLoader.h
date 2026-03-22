#pragma once
#include"Shader.h"
#include<memory>

namespace ENGINE_RENDERING {
	class ShaderLoader 
	{
	private:
		static GLuint CreateProgram(const std::string& vertexShader, const std::string& fragmentShader, const std::string& geometryShader);
		static GLuint CompileShader(GLuint shaderType, const std::string& filepath);
		//from memory
		static GLuint CreateProgram(const char* vertexShader, const char* fragmentShader, const char* geometryShader);
		static GLuint CompileShader(GLuint type, const char* shader);

		static bool CompileSuccess(GLuint shader);
		static bool LinkShader(GLuint program, GLuint vertexShader, GLuint fragmentShader, GLuint geometryShader);
		static bool IsProgramValid(GLuint program);

	public:
		ShaderLoader() = delete;
		static std::shared_ptr<Shader> Create(const std::string& vertexShaderPath, const std::string& fragmentShaderPath, const std::string& geometryShaderPath);
		static std::shared_ptr<Shader> CreateFromMemory(const char* vertexShader, const char* fragmentShader, const char* geometryShader);
		static bool Destroy(Shader* pShader);
	};
}