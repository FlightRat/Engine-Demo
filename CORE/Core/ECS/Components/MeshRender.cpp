#include "MeshRender.h"
#include <entt.hpp>

ENGINE_CORE::ECS::MeshRender::MeshRender():material{Material{}}
{
}

ENGINE_CORE::ECS::MeshRender::MeshRender(const Material& pMaterial):material{pMaterial}
{
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
        "addTexture", &Material::AddTexture
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
