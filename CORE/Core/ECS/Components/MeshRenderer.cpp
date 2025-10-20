#include "MeshRenderer.h"
#include <entt.hpp>

void CORE::ECS::MeshRenderer::UploadMesh(MeshFilter mf)
{
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, mf.vertex_data.size() * sizeof(Vertex), mf.vertex_data.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mf.index_data.size() * sizeof(unsigned int), mf.index_data.data(), GL_STATIC_DRAW);
    unsigned int stride = (3 + 2 + 3) * sizeof(float);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));

    m_loaded = { true };
}

void CORE::ECS::MeshRenderer::CreateLuaMeshRendererBind(sol::state& lua)
{
    lua.new_usertype<MeshRenderer>(
        "MeshRenderer",
        "type_id", &entt::type_hash<MeshRenderer>::value,
        sol::call_constructor,
        sol::factories(
            [&](const std::string& shader, glm::vec4 color, int texture) {
                MeshRenderer MR{
                    .shader = shader,
                    .color = color,
                    .texture = texture
                };
                return MR;
            }
        )
    );
}
