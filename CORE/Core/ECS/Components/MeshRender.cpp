#include "MeshRender.h"
#include <entt.hpp>
#include <stdexcept>

void ENGINE_CORE::ECS::Material::AddTexture(const std::string& key, const std::string& texName)
{
    m_textures[key] = texName;
}

ENGINE_CORE::ECS::MeshRender::MeshRender(const std::vector<Material>& pMaterials): materials{ pMaterials }
{
}

// MeshRender::GetMaterial 实现（增加异常提示，更易调试）
ENGINE_CORE::ECS::Material& ENGINE_CORE::ECS::MeshRender::GetMaterial(size_t index)
{
    if (index >= materials.size()) {
        // 可选：抛出异常（比直接 resize 更易发现逻辑错误）
        // throw std::out_of_range("MeshRender::GetMaterial: index out of bounds");
        materials.resize(index + 1); // 保留原逻辑：自动扩容
    }
    return materials[index];
}

// MeshRender::AddMaterial 实现
void ENGINE_CORE::ECS::MeshRender::AddMaterial(const Material& material)
{
    materials.push_back(material);
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
            []() {
                return MeshRender();
            }
        ),
        "add_material", &MeshRender::AddMaterial
    );
}
