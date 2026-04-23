#include "Mesh.h"
#include"Logger/Logger.h"

namespace ENGINE_RENDERING {
    // 使用 std::move 转移所有权，避免拷贝
    Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::map<std::string, std::string> default_texture)
        : vertices(std::move(vertices)), indices(std::move(indices)), default_texture(std::move(default_texture))
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
        default_texture(std::move(other.default_texture)),
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
        // 1. 初始化对象内存空间（DSA第一步）
        glCreateVertexArrays(1, &VAO);
        glCreateBuffers(1, &VBO);
        glCreateBuffers(1, &EBO);

        // 2. 显存分配与数据上传 —— 彻底抛弃 glBufferData
        // 使用 glNamedBufferStorage 进行【不可变存储 (Immutable Storage)】
        // 这告诉显卡驱动：这块内存的大小未来绝对不会发生变化，请做最极致的底层寻址优化！
        const GLsizeiptr vboSize = vertices.size() * sizeof(Vertex);
        // 最后的参数 0 表示我们不需要 CPU 端动态更新修改权限 (对应原先的 GL_STATIC_DRAW)
        // 如果你要做动态骨骼网格体(CPU蒙皮)，这里可以用 GL_DYNAMIC_STORAGE_BIT
        glNamedBufferStorage(VBO, vboSize, vertices.data(), 0);

        const GLsizeiptr eboSize = indices.size() * sizeof(unsigned int);
        glNamedBufferStorage(EBO, eboSize, indices.data(), 0);

        // 3. 配置 VAO —— 格式(Format)与绑定(Binding)解耦
        const GLuint bindingIndex = 0; // 我们所有的顶点数据打包在一个VBO里，所以统一使用 0 号绑定槽

        // 3.1 挂载 VBO 和 EBO 到 VAO
        // (VAO的ID, 绑定槽索引, VBO的ID, 内存起始偏移, Stride跨步大小)
        glVertexArrayVertexBuffer(VAO, bindingIndex, VBO, 0, sizeof(Vertex));
        // 直接把 EBO 贴到 VAO 上
        glVertexArrayElementBuffer(VAO, EBO);

        // 4. 配置顶点属性 (格式定义 + 连接绑定槽)
        // --- Location 0: 顶点位置 (Position) ---
        glEnableVertexArrayAttrib(VAO, 0);
        // 定义格式: VAO, 属性位置, 数据数量(3), 类型(float), 是否归一化, 相对于 struct 起点的偏移量
        glVertexArrayAttribFormat(VAO, 0, 3, GL_FLOAT, GL_FALSE, 0);
        // 链接映射: 告诉 0 号属性，去 bindingIndex (即0号槽) 拿数据
        glVertexArrayAttribBinding(VAO, 0, bindingIndex);

        // --- Location 1: 法线 (Normal) ---
        glEnableVertexArrayAttrib(VAO, 1);
        glVertexArrayAttribFormat(VAO, 1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, Normal));
        glVertexArrayAttribBinding(VAO, 1, bindingIndex);

        // --- Location 2: UV (TexCoords) ---
        glEnableVertexArrayAttrib(VAO, 2);
        glVertexArrayAttribFormat(VAO, 2, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, TexCoords));
        glVertexArrayAttribBinding(VAO, 2, bindingIndex);

        // --- Location 3: 切线 (Tangent) ---
        glEnableVertexArrayAttrib(VAO, 3);
        glVertexArrayAttribFormat(VAO, 3, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, Tangent));
        glVertexArrayAttribBinding(VAO, 3, bindingIndex);

        // --- Location 4: 副切线 (Bitangent) ---
        glEnableVertexArrayAttrib(VAO, 4);
        glVertexArrayAttribFormat(VAO, 4, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, Bitangent));
        glVertexArrayAttribBinding(VAO, 4, bindingIndex);
    }
}