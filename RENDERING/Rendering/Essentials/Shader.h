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

		// TODO: more setters
		void SetUniformInt(const std::string& name, int value);
		void SetUniformMat4(const std::string& name, glm::mat4& mat);

		//TODU: getters

		void Enable();
		void Disable();

		inline const GLuint ShaderProgramID() const { return m_ShaderProgramID; }
	};
}