#pragma once
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <vector>
#include <string>

namespace ENGINE_RENDERING {
    struct Vertex
    {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec2 TexCoords;
        glm::vec3 Tangent;
        glm::vec3 Bitangent;
    };

    class Mesh {
    public:
        GLuint VAO = 0, VBO = 0, EBO = 0;
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        Mesh() = default;
        Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices);
        ~Mesh();

        // 禁用拷贝，防止意外的 OpenGL 对象重复释放
        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;

        // 允许移动
        Mesh(Mesh&& other) noexcept;
        Mesh& operator=(Mesh&& other) noexcept;

        void Draw() const;

    private:
        void SetupMesh();
    };
}