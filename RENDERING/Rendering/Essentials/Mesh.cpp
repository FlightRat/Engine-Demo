#include "Mesh.h"

namespace ENGINE_RENDERING {
    // 使用 std::move 转移所有权，避免拷贝
    Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices)
        : vertices(std::move(vertices)), indices(std::move(indices))
    {
        SetupMesh();
    }

    Mesh::~Mesh() {
        // OpenGL 规范允许删除 0，不需要 if 判断
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }

    Mesh::Mesh(Mesh&& other) noexcept
        : vertices(std::move(other.vertices)), // vector 自带移动逻辑，极其高效
        indices(std::move(other.indices)),
        VAO(other.VAO), // 偷取 ID
        VBO(other.VBO),
        EBO(other.EBO)
    {
        // 【关键一步】把原来的 ID 设为 0，防止原来的对象析构时删除显存
        other.VAO = 0;
        other.VBO = 0;
        other.EBO = 0;
    }

    Mesh& Mesh::operator=(Mesh&& other) noexcept
    {
        if (this != &other) {
            // 1. 先释放自己当前的显存（如果有的话）
            if (VAO) glDeleteVertexArrays(1, &VAO);
            if (VBO) glDeleteBuffers(1, &VBO);
            if (EBO) glDeleteBuffers(1, &EBO);

            // 2. 偷取数据
            vertices = std::move(other.vertices);
            indices = std::move(other.indices);
            VAO = other.VAO;
            VBO = other.VBO;
            EBO = other.EBO;

            // 3. 把对方置空
            other.VAO = 0;
            other.VBO = 0;
            other.EBO = 0;
        }
        return *this;
    }

    void Mesh::Draw() const {
        if (VAO == 0) return;
        glBindVertexArray(VAO);
        // 使用索引绘制
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    void Mesh::SetupMesh()
    {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        // 顶点位置
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        // 法线
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal_));
        // UV
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv_));

        glBindVertexArray(0);
    }
}