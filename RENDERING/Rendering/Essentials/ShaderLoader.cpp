#include "ShaderLoader.h"
#include<iostream>
#include<fstream>
#include<Logger/Logger.h>

namespace ENGINE_RENDERING {
    GLuint ShaderLoader::CreateProgram(const std::string& vertexShader, const std::string& fragmentShader, const std::string& geometryShader)
    {
        const GLuint program = glCreateProgram();
        const GLuint vertex = CompileShader(GL_VERTEX_SHADER, vertexShader);
        const GLuint fragment = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

        if (vertex == 0 || fragment == 0) {
            if (vertex != 0) glDeleteShader(vertex);
            if (fragment != 0) glDeleteShader(fragment);
            glDeleteProgram(program);
            return 0;
        }

        GLuint geometry = 0;
        if (!geometryShader.empty()) {
            geometry = CompileShader(GL_GEOMETRY_SHADER, geometryShader);
            // 如果几何着色器编译失败，也需要清理
            if (geometry == 0) {
                glDeleteShader(vertex);
                glDeleteShader(fragment);
                glDeleteProgram(program);
                return 0;
            }
        }

        if (!LinkShader(program, vertex, fragment, geometry))
        {
            ENGINE_ERROR("Failed to link Shaders!");
            return 0;
        }
        return program;
    }

    GLuint ShaderLoader::CompileShader(GLuint shaderType, const std::string& filepath)
    {
        // read the shader file
        std::ifstream ifs(filepath);
        if(ifs.fail())
        {
            ENGINE_ERROR("Shader Failed to open [{}]!", filepath);
            return 0;
        }
        std::string contents{ "" };
        std::string line;
        while (std::getline(ifs, line))
        {
            contents += line + "\n";
        }
        ifs.close();

        const GLuint shaderID = glCreateShader(shaderType);
        const char* contentsPtr = contents.c_str();
        glShaderSource(shaderID, 1, &contentsPtr, nullptr);
        glCompileShader(shaderID);
        if (!CompileSuccess(shaderID))
        {
            ENGINE_ERROR("Failed to compile shader [{}]!", filepath);
            return 0;
        }
        return shaderID;
    }

    GLuint ShaderLoader::CreateProgram(const char* vertexShader, const char* fragmentShader, const char* geometryShader)
    {
        const GLuint program = glCreateProgram();
        const GLuint vertex = CompileShader(GL_VERTEX_SHADER, vertexShader);
        const GLuint fragment = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);
        if (vertex == 0 || fragment == 0) {
            if (vertex != 0) glDeleteShader(vertex);
            if (fragment != 0) glDeleteShader(fragment);
            glDeleteProgram(program);
            return 0;
        }

        GLuint geometry = 0;
        if (geometryShader!=nullptr) {
            geometry = CompileShader(GL_GEOMETRY_SHADER, geometryShader);
            // 如果几何着色器编译失败，也需要清理
            if (geometry == 0) {
                glDeleteShader(vertex);
                glDeleteShader(fragment);
                glDeleteProgram(program);
                return 0;
            }
        }

        if (!LinkShader(program, vertex, fragment, geometry))
        {
            ENGINE_ERROR("Failed to link Shaders!");
            return 0;
        }
        return program;
    }

    GLuint ShaderLoader::CompileShader(GLuint type, const char* shader)
    {
        const GLuint id = glCreateShader(type);
        glShaderSource(id, 1, &shader, nullptr);
        glCompileShader(id);
        if (!CompileSuccess(id))
        {
            ENGINE_ERROR("Failed to compile shader from memory!");
            return 0;
        }
        return id;
    }

    bool ShaderLoader::CompileSuccess(GLuint shader)
    {
        GLint status;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if (status != GL_TRUE)
        {
            GLint maxLength;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
            std::string errorLog(maxLength, ' ');
            glGetShaderInfoLog(shader, maxLength, &maxLength, errorLog.data());
            ENGINE_ERROR("Shader Compile failed: [{0}]", std::string(errorLog));
            glDeleteShader(shader);
            return false;
        }
        return true;
    }

    bool ShaderLoader::IsProgramValid(GLuint program)
    {
        GLint status;
        glGetProgramiv(program, GL_LINK_STATUS, &status);
        if (status != GL_TRUE)
        {
            GLint maxLength;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);
            std::string errorLog(maxLength, ' ');
            glGetProgramInfoLog(program, maxLength, &maxLength, errorLog.data());
            ENGINE_ERROR("Shader Program link failed: [{0}]", std::string(errorLog));
            return false;
        }
        return true;
    }

    bool ShaderLoader::LinkShader(GLuint program, GLuint vertexShader, GLuint fragmentShader, GLuint geometryShader)
    {
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        if (geometryShader != 0)
            glAttachShader(program, geometryShader);
        glLinkProgram(program);

        if (!IsProgramValid(program))
        {
            glDeleteProgram(program);
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);
            if (geometryShader != 0)
                glDeleteShader(geometryShader);
            return false;
        }

        glDetachShader(program, vertexShader);
        glDetachShader(program, fragmentShader);
        if (geometryShader != 0)
            glDetachShader(program, geometryShader);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        if (geometryShader != 0)
            glDeleteShader(geometryShader);

        return true;
    }

    std::shared_ptr<Shader> ShaderLoader::Create(const std::string& vertexShaderPath, const std::string& fragmentShaderPath, const std::string& geometryShaderPath)
    {
        GLuint program = CreateProgram(vertexShaderPath, fragmentShaderPath, geometryShaderPath);
        if (program)
            return std::make_shared<Shader>(program, vertexShaderPath, fragmentShaderPath, geometryShaderPath);
        return nullptr;
    }

    std::shared_ptr<Shader> ShaderLoader::CreateFromMemory(const char* vertexShader, const char* fragmentShader, const char* geometryShader)
    {
        GLuint program = CreateProgram(vertexShader, fragmentShader, geometryShader);
        if (program)
            return std::make_shared<Shader>(program, vertexShader, fragmentShader, geometryShader);
        return nullptr;
    }

    bool ShaderLoader::Destroy(Shader* pShader)
    {
        if (pShader->ShaderProgramID() <= 0)
            return false;

        glDeleteProgram(pShader->ShaderProgramID());
        return true;
    }

}