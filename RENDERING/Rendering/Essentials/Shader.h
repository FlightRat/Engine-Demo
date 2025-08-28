#pragma once
#include <string>
#include<unordered_map>
#include<glad/glad.h>
#include<glm/glm.hpp>

namespace RENDERING {
	class Shader
	{
	private:
		GLuint m_ShaderProgramID;
		std::string m_sVertexPath, m_sFragmentPath;
		std::unordered_map<std::string, GLuint> m_UniformLocationMap;
		GLuint GetUniformLocation(const std::string& uniformName);
	public:
		Shader(GLuint program, const std::string vertexPath, const std::string& fragmentPath);
		~Shader();

		// Setters
		void SetUniformInt(const std::string& name, int value);
		void SetUniformFloat(const std::string& name, float value);
		void SetUniformBool(const std::string& name, bool value);
		// Vec
		void SetUniformVec2(const std::string& name, const glm::vec2& value);
		void SetUniformVec2(const std::string& name, float x, float y);
		void SetUniformVec3(const std::string& name, const glm::vec3& value);
		void SetUniformVec3(const std::string& name, float x, float y, float z);
		void SetUniformVec4(const std::string& name, const glm::vec4& value);
		void SetUniformVec4(const std::string& name, float x, float y, float z, float w);
		// Mat
		void SetUniformMat2(const std::string& name, glm::mat2& mat);
		void SetUniformMat3(const std::string& name, glm::mat3& mat);
		void SetUniformMat4(const std::string& name, glm::mat4& mat);
		

		//TODU: getters

		void Enable();
		void Disable();

		inline const GLuint ShaderProgramID() const { return m_ShaderProgramID; }
	};
}