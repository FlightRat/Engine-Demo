#include "ShaderLoader.h"
#include<iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
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
        // 1. [核心修复] 使用 C++20 filesystem 安全无损地转换 UTF-8 中文路径
        std::filesystem::path safePath = reinterpret_cast<const char8_t*>(filepath.c_str());

        // 2. 将安全的 path 对象直接传给 ifstream 构造函数
        // 这样在 Windows 底层，C++ 标准库会自动调用安全宽字符 API (CreateFileW)
        std::ifstream ifs(safePath);
        if (!ifs.is_open())  // 使用 is_open() 比 fail() 具有多一层防御语义
        {
            ENGINE_ERROR("Shader Failed to open [{}]!", filepath);
            return 0;
        }

        // 3. [性能优化] 废弃原先极为低效的 while(getline) 字符串堆内存拼接操作
        // 利用底层 rdbuf 将文件流的数据一次性块状读入，这是现代 C++ 极为高效且优雅的做法
        std::stringstream buffer;
        buffer << ifs.rdbuf();
        std::string contents = buffer.str();
        ifs.close();

        // ========= OpenGL编译核心逻辑 =========
        const GLuint shaderID = glCreateShader(shaderType);
        const char* contentsPtr = contents.c_str();

        glShaderSource(shaderID, 1, &contentsPtr, nullptr);
        glCompileShader(shaderID);

        if (!CompileSuccess(shaderID))
        {
            // 注意这里报错依然打印原始的 UTF-8 string，如果终端不支持显示中文可能看上去是乱码，
            // 但这不会影响你的引擎逻辑，因为底层路径是绝对正确的
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