#include "MeshRender.h"
#include <entt.hpp>

ENGINE_CORE::ECS::MeshRender::MeshRender():material{Material{}}
{
}

ENGINE_CORE::ECS::MeshRender::MeshRender(const Material& pMaterial):material{pMaterial}
{
}

void ENGINE_CORE::ECS::MeshRender::UploadMesh(MeshFilter mf)
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

void ENGINE_CORE::ECS::MeshRender::CreateLuaMeshRendererBind(sol::state& lua)
{
    lua.new_usertype<Material>(
		"Material",
		sol::call_constructor,
		sol::factories(
			[] {return Material{}; },
			[](const sol::table& material)
			{
                Material m = Material{
                    .shaderName = material["shaderName"],
                    .color = material["color"],
                    .shininess = material["shininess"],
                    .m_useTexture = material["useTex"]
				};
                return m;
			}
		),
        "shaderName", &Material::shaderName,
        "color", &Material::color,
        "shininess", &Material::shininess,
        "useTex", &Material::m_useTexture,
        "textures", &Material::m_textures,
        "addTexture", &Material::SetTexture
    );

    lua.new_usertype<MeshRender>(
        "MeshRender",
        "type_id", &entt::type_hash<MeshRender>::value,
        sol::call_constructor,
        sol::factories(
            [&](const Material& material) {
                MeshRender MR{ material };
                return MR;
            }
        ),
        "material",&MeshRender::material
    );
}
