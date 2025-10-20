#include "MeshFilter.h"
#include <entt.hpp>

void ENGINE_CORE::ECS::MeshFilter::load_mesh()
{
    if (mesh == "cube")
        load_cube();
    else if (mesh == "plane")
        load_plane();
    else if (mesh == "sphere")
        load_sphere();
    else if (mesh == "hud_quad")
        load_hud_quad();
}

void ENGINE_CORE::ECS::MeshFilter::load_hud_quad()
{
    // 1. 清除旧数据
    vertex_data.clear();
    index_data.clear();

    // 2. 定义 HUD Quad 的 4 个唯一的角顶点
    // 顶点格式：Pos(3) | Normal(3) | TexCoord(2)
    // HUD Quad 位于 XY 平面 (Z=0)，法线指向 Z+ (0, 0, 1)

    // A. 顶点数据 (4 个唯一的顶点)
    Vertex unique_vertices[] = {
        // Pos(-1, -1, 0) | Normal(0, 0, 1) | UV(0, 0)
        { glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f) }, // 0: bottom-left

        // Pos( 1, -1, 0) | Normal(0, 0, 1) | UV(1, 0)
        { glm::vec3(1.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 0.0f) }, // 1: bottom-right

        // Pos( 1,  1, 0) | Normal(0, 0, 1) | UV(1, 1)
        { glm::vec3(1.0f,  1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f) }, // 2: top-right

        // Pos(-1,  1, 0) | Normal(0, 0, 1) | UV(0, 1)
        { glm::vec3(-1.0f,  1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f) }  // 3: top-left
    };

    // 将唯一的 4 个顶点添加到组件的 vector 中
    vertex_data.assign(unique_vertices, unique_vertices + 4);


    // 3. 定义索引数据 (6 个索引)
    // Quad 由两个三角形组成：
    // 三角形 1: 0, 1, 2 (bottom-left, bottom-right, top-right)
    // 三角形 2: 0, 2, 3 (bottom-left, top-right, top-left)
    unsigned int indices[] = {
        0, 1, 2,  // 第一个三角形
        0, 2, 3   // 第二个三角形
    };

    // 将索引数据添加到组件的 vector 中
    index_data.assign(indices, indices + 6);

    // 渲染注意事项：
    // 在 GPU 端设置 VBO/EBO 后，渲染时应使用 glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void ENGINE_CORE::ECS::MeshFilter::load_plane()
{
    // 1. 清除旧数据
    vertex_data.clear();
    index_data.clear();

    // 2. 定义 Plane 的 4 个唯一的角顶点
    // Plane 位于 XZ 平面 (Y=0)，法线指向 Y+ (0, 1, 0)
    // 注意：原始代码使用了 ±10.0 的坐标和纹理坐标，这通常用于平铺 (Tiling) 大地块。

    // A. 顶点数据 (4 个唯一的顶点)
    Vertex unique_vertices[] = {
        // Pos(X, Y, Z) | Normal(0, 1, 0) | UV(U, V)

        // 0: 右上 (10, 0, 10)
        { glm::vec3(10.0f, 0.0f,  10.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(10.0f, 0.0f) },

        // 1: 左上 (-10, 0, 10)
        { glm::vec3(-10.0f, 0.0f,  10.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 0.0f) },

        // 2: 左下 (-10, 0, -10)
        { glm::vec3(-10.0f, 0.0f, -10.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 10.0f) },

        // 3: 右下 (10, 0, -10)
        { glm::vec3(10.0f, 0.0f, -10.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(10.0f, 10.0f) }
    };

    // 将唯一的 4 个顶点添加到组件的 vector 中
    vertex_data.assign(unique_vertices, unique_vertices + 4);


    // 3. 定义索引数据 (6 个索引)
    // Quad 由两个三角形组成：
    // 三角形 1: 0, 2, 1 (右上, 左下, 左上)
    // 三角形 2: 0, 3, 2 (右上, 右下, 左下)
    unsigned int indices[] = {
        0, 2, 1,  // 第一个三角形 (确保缠绕顺序正确)
        0, 3, 2   // 第二个三角形
    };

    // 将索引数据添加到组件的 vector 中
    index_data.assign(indices, indices + 6);

    // 渲染注意事项：
    // 在 GPU 端设置 VBO/EBO 后，渲染时应使用 glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void ENGINE_CORE::ECS::MeshFilter::load_cube()
{
    // 1. 清除旧数据
    vertex_data.clear();
    index_data.clear();

    // 2. 定义唯一的顶点 (8个角)
    // 格式：Position (3 floats) | Normal (3 floats) | UV (2 floats)
    // 注意：立方体需要14个独特的顶点才能正确处理法线和UV缝隙，但为了简单起见，我们先用8个角作为基础。
    // *真正的* 渲染立方体通常需要 24 个顶点 (6 面 * 4 顶点/面) 或 36 个顶点 (6 面 * 6 顶点/面, 非索引)
    // 我们将采用 24 个独特的顶点 + 索引的方案，确保正确的法线和 UV。

    // ----------------------------------------------------------------------
    // 立方体的 8 个物理角坐标
    glm::vec3 p0(-1.0f, -1.0f, 1.0f); // 前下左
    glm::vec3 p1(1.0f, -1.0f, 1.0f); // 前下右
    glm::vec3 p2(1.0f, 1.0f, 1.0f); // 前上右
    glm::vec3 p3(-1.0f, 1.0f, 1.0f); // 前上左
    glm::vec3 p4(-1.0f, -1.0f, -1.0f); // 后下左
    glm::vec3 p5(1.0f, -1.0f, -1.0f); // 后下右
    glm::vec3 p6(1.0f, 1.0f, -1.0f); // 后上右
    glm::vec3 p7(-1.0f, 1.0f, -1.0f); // 后上左
    // ----------------------------------------------------------------------

    // ** 优化：为每个面创建独立的顶点，以确保正确的法线和UV **

    // Front face (index: 0-3, Normal: Z+)
    vertex_data.push_back({ p0, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f} }); // 0
    vertex_data.push_back({ p1, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f} }); // 1
    vertex_data.push_back({ p2, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f} }); // 2
    vertex_data.push_back({ p3, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f} }); // 3

    // Back face (index: 4-7, Normal: Z-)
    vertex_data.push_back({ p4, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f} }); // 4 (UV flipped for example)
    vertex_data.push_back({ p5, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f} }); // 5
    vertex_data.push_back({ p6, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f} }); // 6
    vertex_data.push_back({ p7, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f} }); // 7

    // Right face (index: 8-11, Normal: X+)
    vertex_data.push_back({ p1, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f} }); // 8 (p1)
    vertex_data.push_back({ p5, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f} }); // 9 (p5)
    vertex_data.push_back({ p6, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f} }); // 10 (p6)
    vertex_data.push_back({ p2, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f} }); // 11 (p2)

    // Left face (index: 12-15, Normal: X-)
    vertex_data.push_back({ p4, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f} }); // 12 (p4)
    vertex_data.push_back({ p0, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f} }); // 13 (p0)
    vertex_data.push_back({ p3, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f} }); // 14 (p3)
    vertex_data.push_back({ p7, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f} }); // 15 (p7)

    // Top face (index: 16-19, Normal: Y+)
    vertex_data.push_back({ p3, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f} }); // 16 (p3)
    vertex_data.push_back({ p2, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f} }); // 17 (p2)
    vertex_data.push_back({ p6, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f} }); // 18 (p6)
    vertex_data.push_back({ p7, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f} }); // 19 (p7)

    // Bottom face (index: 20-23, Normal: Y-)
    vertex_data.push_back({ p4, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f} }); // 20 (p4)
    vertex_data.push_back({ p5, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f} }); // 21 (p5)
    vertex_data.push_back({ p1, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f} }); // 22 (p1)
    vertex_data.push_back({ p0, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f} }); // 23 (p0)

    // 3. 定义索引 (每 4 个顶点构成一个四边形，用 2 个三角形表示)

    unsigned int indices[] = {
        // Front (0, 1, 2, 3)
        0, 1, 2,  0, 2, 3,
        // Back (4, 5, 6, 7)
        4, 5, 6,  4, 6, 7,
        // Right (8, 9, 10, 11)
        8, 9, 10, 8, 10, 11,
        // Left (12, 13, 14, 15)
        12, 13, 14, 12, 14, 15,
        // Top (16, 17, 18, 19)
        16, 17, 18, 16, 18, 19,
        // Bottom (20, 21, 22, 23)
        20, 21, 22, 20, 22, 23
    };

    // 4. 将索引数据拷贝到 vector
    size_t index_count = sizeof(indices) / sizeof(unsigned int);
    index_data.assign(indices, indices + index_count);
}

void ENGINE_CORE::ECS::MeshFilter::load_sphere()
{
    // 1. 清除旧数据
    vertex_data.clear();
    index_data.clear();

    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> uv;
    std::vector<glm::vec3> normals;

    const unsigned int X_SEGMENTS = 64; // 经线/扇区数量
    const unsigned int Y_SEGMENTS = 64; // 纬线/堆栈数量
    const float PI = 3.14159265359f;

    // -----------------------------------------------------------
    // 2. 生成唯一的顶点、法线和纹理坐标 (与你现有的代码相同)
    // -----------------------------------------------------------
    for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
    {
        for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
        {
            float xSegment = (float)x / (float)X_SEGMENTS;
            float ySegment = (float)y / (float)Y_SEGMENTS;

            // 球坐标参数化 (r=1.0)
            float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
            float yPos = std::cos(ySegment * PI);
            float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

            positions.push_back(glm::vec3(xPos, yPos, zPos));
            uv.push_back(glm::vec2(xSegment, ySegment));
            normals.push_back(glm::vec3(xPos, yPos, zPos));
        }
    }

    // -----------------------------------------------------------
    // 3. 生成 GL_TRIANGLES 模式的索引
    // -----------------------------------------------------------
    // 每个四边形网格面需要 6 个索引 (2 个三角形)
    for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
    {
        for (unsigned int x = 0; x < X_SEGMENTS; ++x)
        {
            // 当前四边形的四个顶点索引 (P0, P1, P2, P3)
            unsigned int p0 = y * (X_SEGMENTS + 1) + x;         // 左下 (或左上，取决于极点定义)
            unsigned int p1 = y * (X_SEGMENTS + 1) + x + 1;     // 右下
            unsigned int p2 = (y + 1) * (X_SEGMENTS + 1) + x + 1; // 右上
            unsigned int p3 = (y + 1) * (X_SEGMENTS + 1) + x;     // 左上

            // 三角形 1: p0, p2, p1
            index_data.push_back(p0);
            index_data.push_back(p2);
            index_data.push_back(p1);

            // 三角形 2: p0, p3, p2
            index_data.push_back(p0);
            index_data.push_back(p3);
            index_data.push_back(p2);
        }
    }

    // -----------------------------------------------------------
    // 4. 组织最终的 Vertex 数据
    // -----------------------------------------------------------
    // 将 Pos/Normal/UV 组合成 Vertex 结构体列表 (与你现有的代码相同)
    for (size_t i = 0; i < positions.size(); ++i)
    {
        Vertex v;
        v.pos_ = positions[i];
        v.normal_ = normals[i];
        v.uv_ = uv[i];
        vertex_data.push_back(v);
    }

    // 渲染注意事项：
    // 在 GPU 端设置 VBO/EBO 后，渲染时应使用 glDrawElements(GL_TRIANGLES, index_data.size(), GL_UNSIGNED_INT, 0);
}

void ENGINE_CORE::ECS::MeshFilter::CreateLuaMeshFilterBind(sol::state& lua)
{
    lua.new_usertype<MeshFilter>(
        "MeshFilter",
        "type_id", &entt::type_hash<MeshFilter>::value,
        sol::call_constructor,
        sol::factories(
            [&](const std::string& mesh) {
                MeshFilter MF{
                    .mesh = mesh,
                };
                MF.load_mesh();
                return MF;
            }
        )
    );
}
