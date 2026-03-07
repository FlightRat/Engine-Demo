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

void ENGINE_CORE::ECS::MeshRender::SetColor(glm::vec4 color)
{
    const int MaterialCount = materials.size();
    for (int i = 0; i < MaterialCount; i++)
    {
        materials[i].color = color;
    }
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

void ENGINE_CORE::ECS::MeshRender::ResetMaterial(const std::vector<ENGINE_RENDERING::Mesh>& meshes)
{
    materials.clear();
    const int MeshCount = meshes.size();
    for (int i = 0; i < MeshCount; i++)
    {
        std::map<std::string, std::string> tex_map;
        // 核心修复：先检查键是否存在，再安全访问
        auto& tex_map_ref = meshes[i].default_texture;
        if (!tex_map_ref.empty())
        {
            // 使用find避免自动插入不存在的键
            auto diffuse_it = tex_map_ref.find("diffuse");
            auto specular_it = tex_map_ref.find("specular");

            // 存在则取对应值，不存在则设为空字符串
            tex_map["diffuse"] = (diffuse_it != tex_map_ref.end()) ? diffuse_it->second : "";
            tex_map["specular"] = (specular_it != tex_map_ref.end()) ? specular_it->second : "";
        }
        else
        {
            tex_map["diffuse"] = "";
            tex_map["specular"] = "";
        }
        materials.push_back(
            Material{
                .shaderName = "mainShader",
                .color = glm::vec4 {1.0f},
                .shininess = 64.0f,
                .m_useTexture = true,
                .m_textures = tex_map
            }
        );
    }
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
        "flip_uv",&MeshRender::flipUV,
        "set_color",&MeshRender::SetColor,
        "add_material", &MeshRender::AddMaterial
    );
}
