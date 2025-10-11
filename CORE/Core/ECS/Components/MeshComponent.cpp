#include "MeshComponent.h"

void CORE::ECS::MeshComponent::load_plane()
{
    data.clear();
    vertexNum = 6;
    float vertices[] = {
        // positions            // normals         // texcoords
         10.0f, 0.0f,  10.0f,  0.0f, 1.0f, 0.0f,  10.0f,  0.0f,
        -10.0f, 0.0f,  10.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
        -10.0f, 0.0f, -10.0f,  0.0f, 1.0f, 0.0f,   0.0f, 10.0f,

         10.0f, 0.0f,  10.0f,  0.0f, 1.0f, 0.0f,  10.0f,  0.0f,
        -10.0f, 0.0f, -10.0f,  0.0f, 1.0f, 0.0f,   0.0f, 10.0f,
         10.0f, 0.0f, -10.0f,  0.0f, 1.0f, 0.0f,  10.0f, 10.0f
    };
    size_t vertexSize = sizeof(vertices) / sizeof(float);
    data.reserve(vertexSize);
    for (size_t i = 0; i < vertexSize; ++i)
    {
        data.push_back(vertices[i]);
    }
}

void CORE::ECS::MeshComponent::load_hud_quad()
{
    data.clear();
    vertexNum = 6;
    float vertices[] = {
        // positions            // normals         // texcoords
        -1.0f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
         1.0f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
         1.0f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
         1.0f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
        -1.0f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
        -1.0f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
    };
    size_t vertexSize = sizeof(vertices) / sizeof(float);
    data.reserve(vertexSize);
    for (size_t i = 0; i < vertexSize; ++i)
    {
        data.push_back(vertices[i]);
    }
}

void CORE::ECS::MeshComponent::load_cube()
{
    data.clear();
    vertexNum = 36;
    float vertices[] = {
        // back face
        -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
         1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
         1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
         1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
        -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
        -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
        // front face
        -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
         1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
         1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
         1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
        -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
        -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
        // left face
        -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
        -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
        -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
        -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
        -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
        -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
        // right face
         1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
         1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
         1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
         1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
         1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
         1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
         // bottom face
         -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
          1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
          1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
          1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
         -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
         -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
         // top face
         -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
          1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
          1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
          1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
         -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
         -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left
    };
    size_t vertexSize = sizeof(vertices) / sizeof(float);
    data.reserve(vertexSize);
    for (size_t i = 0; i < vertexSize; ++i)
    {
        data.push_back(vertices[i]);
    }
}

void CORE::ECS::MeshComponent::load_sphere()
{
    data.clear();
    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<float> texcoords;
    std::vector<unsigned int> indices;

    const float radius = 1.0f;
    const int X_SEGMENTS = 64; // 经线/扇区数量
    const int Y_SEGMENTS = 64; // 纬线/堆栈数量
    const float PI = 3.14159265359f;

    // 1. 生成顶点、法线和纹理坐标
    for (int y = 0; y <= Y_SEGMENTS; ++y)
    {
        for (int x = 0; x <= X_SEGMENTS; ++x)
        {
            float xSegment = (float)x / (float)X_SEGMENTS;
            float ySegment = (float)y / (float)Y_SEGMENTS;

            // 球坐标参数化
            // theta (phi) - 纬度角 (0 到 pi)
            // phi (theta) - 经度角 (0 到 2*pi)
            float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
            float yPos = std::cos(ySegment * PI);
            float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

            // 位置 (Position)
            positions.push_back(radius * xPos);
            positions.push_back(radius * yPos);
            positions.push_back(radius * zPos);

            // 法线 (Normal) - 对于半径为 1 的球体，法线与位置向量（归一化后）相同
            normals.push_back(xPos);
            normals.push_back(yPos);
            normals.push_back(zPos);

            // 纹理坐标 (TexCoord)
            texcoords.push_back(xSegment);
            texcoords.push_back(ySegment);
        }
    }

    // 2. 生成索引 (使用 GL_TRIANGLES)
    bool oddRow = false;
    for (int y = 0; y < Y_SEGMENTS; ++y)
    {
        if (!oddRow) // 奇数行 (0, 2, 4...)：从左到右
        {
            for (int x = 0; x <= X_SEGMENTS; ++x)
            {
                indices.push_back(y * (X_SEGMENTS + 1) + x);
                indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
            }
        }
        else // 偶数行 (1, 3, 5...)：从右到左
        {
            for (int x = X_SEGMENTS; x >= 0; --x)
            {
                indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                indices.push_back(y * (X_SEGMENTS + 1) + x);
            }
        }
        oddRow = !oddRow;
    }

    // 3. 组织最终的顶点数据（位置 + 法线 + 纹理坐标）
    // 顶点格式：Pos(3) + Normal(3) + TexCoord(2) = 8 floats
    // 使用索引来构建三角形列表（而不是三角形带）
    for (size_t i = 0; i < indices.size() - 2; ++i)
    {
        // 索引 i, i+1, i+2 构成一个三角形
        // 需要处理三角形带/扇区转化为三角形列表的逻辑
        // 由于 load_mesh/Render 使用 glDrawArrays(GL_TRIANGLES, 0, vertexNum);
        // 我们需要将带状结构（strip）或扇区结构（fan）转换为非索引的 GL_TRIANGLES 列表。

        // 重新组织 data，使之成为一个完整的 GL_TRIANGLES 数组
        // 由于 MeshComponent::Render 最终调用的是 glDrawArrays(GL_TRIANGLES, 0, vertexNum);
        // 我们必须生成一个**非索引**的顶点数据列表。

        // 使用索引来获取顶点属性，然后按三角形顺序添加到 data
        unsigned int i0 = indices[i];
        unsigned int i1 = indices[i + 1];
        unsigned int i2 = indices[i + 2];

        // 这里的索引生成逻辑（三角形带）需要调整为按三角形列表生成。
        // 或者，我们使用一个更直接的方法生成三角形列表的顶点数据。
    }

    // ----------------------------------------------------
    // 由于 load_mesh 和 Render 函数使用的是**非索引**的 GL_TRIANGLES 数组，
    // 我们需要重新生成一个简单的非索引版本，或者使用一个更适合 GL_TRIANGLES 的生成方法。
    // 采用**重新生成**的方法，直接构建 GL_TRIANGLES 顶点列表。
    // ----------------------------------------------------

    std::vector<float> temp_data;
    unsigned int k = 0;
    for (int i = 0; i < Y_SEGMENTS; ++i)
    {
        for (int j = 0; j < X_SEGMENTS; ++j)
        {
            // 两个三角形构成一个四边形网格面
            unsigned int p0 = i * (X_SEGMENTS + 1) + j;
            unsigned int p1 = i * (X_SEGMENTS + 1) + j + 1;
            unsigned int p2 = (i + 1) * (X_SEGMENTS + 1) + j + 1;
            unsigned int p3 = (i + 1) * (X_SEGMENTS + 1) + j;

            // 三角形 1: p0, p2, p1
            std::vector<unsigned int> triangle1 = { p0, p2, p1 };
            // 三角形 2: p0, p3, p2
            std::vector<unsigned int> triangle2 = { p0, p3, p2 };
            std::vector<unsigned int> face_indices = { p0, p2, p1, p0, p3, p2 }; // 6 个顶点

            for (unsigned int idx : face_indices)
            {
                // 位置
                temp_data.push_back(positions[idx * 3 + 0]);
                temp_data.push_back(positions[idx * 3 + 1]);
                temp_data.push_back(positions[idx * 3 + 2]);
                // 法线
                temp_data.push_back(normals[idx * 3 + 0]);
                temp_data.push_back(normals[idx * 3 + 1]);
                temp_data.push_back(normals[idx * 3 + 2]);
                // 纹理坐标
                temp_data.push_back(texcoords[idx * 2 + 0]);
                temp_data.push_back(texcoords[idx * 2 + 1]);
                k++;
            }
        }
    }

    data = std::move(temp_data); // 将临时数据移动到成员变量 data
    vertexNum = data.size() / 8; // 总浮点数 / 每个顶点的浮点数

    // 注意：顶点的法线与位置向量（归一化）相同
    // 在 load_mesh 中调用 glBufferData 之前，确保 data 成员变量已填充。
}

void CORE::ECS::MeshComponent::load_mesh()
{
    if (mesh == "cube")
        load_cube();
    else if (mesh == "plane")
        load_plane();
    else if (mesh == "sphere")
        load_sphere();
    else if (mesh == "hud_quad")
        load_hud_quad();
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    // fill the data
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);
    // attributes
    glBindVertexArray(m_VAO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void CORE::ECS::MeshComponent::Render()
{
    glBindVertexArray(m_VAO);
    int x = data.size();
    glDrawArrays(GL_TRIANGLES, 0, vertexNum);
    glBindVertexArray(0);
}

/*register a "Mesh" type into lua*/
void CORE::ECS::MeshComponent::CreateLuaMeshBind(sol::state& lua)
{
    lua.new_usertype<MeshComponent>(
        "Mesh",
        "type_id", &entt::type_hash<MeshComponent>::value,
        sol::call_constructor,
        sol::factories(
            [&](const std::string& mesh, const std::string& shader, glm::vec4 color, int texture) {
                MeshComponent m{
                    .mesh= mesh,
                    .shader= shader,
                    .color = color,
                    .texture = texture
                };
                m.load_mesh();
                return m;
            }
        ),
        "load_mesh", [](MeshComponent& mesh) {mesh.load_mesh(); },
        "set_shader", [](MeshComponent& mesh, const std::string mesh_shader) { mesh.set_shader(mesh_shader); },
        "set_color", [](MeshComponent& mesh, const glm::vec4 mesh_color) {mesh.set_color(mesh_color); },
        "set_texture", [](MeshComponent& mesh, const int mesh_texture) {mesh.set_texture(mesh_texture); },
        "color", &MeshComponent::color,
        "bHidden",&MeshComponent::bHidden
        //"render", [&](MeshComponent& mesh) {mesh.Render(); }
    );
}
