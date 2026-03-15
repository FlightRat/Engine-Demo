#include "Shader.h"
#include<iostream>
#include<Logger/Logger.h>

namespace ENGINE_RENDERING {
	Shader::Shader():Shader(0, "", ""){ }

	Shader::Shader(GLuint program, const std::string vertexPath, const std::string& fragmentPath):
		m_ShaderProgramID{ program }, m_sVertexPath{vertexPath}, m_sFragmentPath{fragmentPath}
	{
	}

	Shader::~Shader()
	{
		if (m_ShaderProgramID > 0)
			glDeleteProgram(m_ShaderProgramID);

	}

	GLuint Shader::GetUniformLocation(const std::string& uniformName)
	{
		auto uniformItr = m_UniformLocationMap.find(uniformName);
		if (uniformItr != m_UniformLocationMap.end())
			return uniformItr->second; // return location if found in the map
		GLuint location = glGetUniformLocation(m_ShaderProgramID, uniformName.c_str());
		if (location == GL_INVALID_INDEX)
		{
			ENGINE_ERROR("Uniform[{0}] not found in the shader!", uniformName);
			return -1; // not in the shader
		}

		m_UniformLocationMap.emplace(uniformName, location); // add to the map if it in the shader but not in the map

		return location;
	}

	GLuint Shader::GetUniformBlockIndex(const std::string& uniformBlockName)
	{
		auto uniformBlockItr = m_UniformBlockIndexMap.find(uniformBlockName);
		if (uniformBlockItr != m_UniformBlockIndexMap.end())
			return uniformBlockItr->second;
		GLuint index = glGetUniformBlockIndex(m_ShaderProgramID, uniformBlockName.c_str());
		if (index == GL_INVALID_INDEX)
		{
			ENGINE_ERROR("Uniform Block [{0}] not find in the shader!", uniformBlockName);
			return -1;
		}
		m_UniformBlockIndexMap.emplace(uniformBlockName, index);

		return index;
	}

	void Shader::BindUniformBlock(const std::string& name, int index)
	{
		glUniformBlockBinding(m_ShaderProgramID, GetUniformBlockIndex(name), index);
	}

	void Shader::SetUniformInt(const std::string& name, int value)
	{
		glUniform1i(GetUniformLocation(name), value);
	}

	void Shader::SetUniformFloat(const std::string& name, float value)
	{
		glUniform1f(GetUniformLocation(name), value);
	}

	void Shader::SetUniformBool(const std::string& name, bool value)
	{
		glUniform1i(GetUniformLocation(name), (int)value);
	}

	void Shader::SetUniformVec2(const std::string& name, const glm::vec2& value)
	{
		glUniform2fv(GetUniformLocation(name), 1, &value[0]);
	}

	void Shader::SetUniformVec2(const std::string& name, float x, float y)
	{
		glUniform2f(GetUniformLocation(name), x, y);
	}

	void Shader::SetUniformVec3(const std::string& name, const glm::vec3& value)
	{
		glUniform3fv(GetUniformLocation(name), 1, &value[0]);
	}

	void Shader::SetUniformVec3(const std::string& name, float x, float y, float z)
	{
		glUniform3f(GetUniformLocation(name), x, y, z);
	}

	void Shader::SetUniformVec4(const std::string& name, const glm::vec4& value)
	{
		glUniform4fv(GetUniformLocation(name), 1, &value[0]);
	}

	void Shader::SetUniformVec4(const std::string& name, float x, float y, float z, float w)
	{
		glUniform4f(GetUniformLocation(name), x, y, z, w);
	}

	void Shader::SetUniformMat2(const std::string& name, glm::mat2& mat)
	{
		glUniformMatrix2fv(GetUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
	}

	void Shader::SetUniformMat3(const std::string& name, glm::mat3& mat)
	{
		glUniformMatrix3fv(GetUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
	}

	void Shader::SetUniformMat4(const std::string& name, glm::mat4& mat)
	{
		glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
	}

	void Shader::Enable()
	{
		glUseProgram(m_ShaderProgramID);
	}

	void Shader::Disable()
	{
		glUseProgram(0);
	}

}