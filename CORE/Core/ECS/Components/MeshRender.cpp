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
        const auto& tex_map_ref = meshes[i].GetDefaultTextures();
        if (!tex_map_ref.empty())
        {
            // 使用find避免自动插入不存在的键
            auto albedo_it = tex_map_ref.find("albedo");
            auto normal_it = tex_map_ref.find("normal");
            auto metallic_it = tex_map_ref.find("metallic");
            auto roughness_it = tex_map_ref.find("roughness");
            auto ao_it = tex_map_ref.find("ao");

            // 存在则取对应值，不存在则设为空字符串
            tex_map["albedo"] = (albedo_it != tex_map_ref.end()) ? albedo_it->second : "";
            tex_map["normal"] = (normal_it != tex_map_ref.end()) ? normal_it->second : "";
            tex_map["metallic"] = (metallic_it != tex_map_ref.end()) ? metallic_it->second : "";
            tex_map["roughness"] = (roughness_it != tex_map_ref.end()) ? roughness_it->second : "";
            tex_map["ao"] = (ao_it != tex_map_ref.end()) ? ao_it->second : "";
        }
        else
        {
            tex_map["albedo"] = "";
            tex_map["normal"] = "";
            tex_map["metallic"] = "";
            tex_map["roughness"] = "";
            tex_map["ao"] = "";
        }
        materials.push_back(
            Material{
                .shadingModel = "PBR",
                .color = glm::vec4 {1.0f},
                .metallic = 0.0f,
                .roughness = 1.0f,
                .ao = 1.0f,
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
                    .shadingModel = material["shadingModel"],
                    .color = material["color"],
                    .metallic = material["metallic"],
                    .roughness = material["roughness"],
                    .ao = material["ao"],
                    .m_useTexture = material["useTex"]
				};
                return m;
			}
		),
        "shadingModel", &Material::shadingModel,
        "color", &Material::color,
        "metallic", &Material::metallic,
        "roughness", &Material::roughness,
        "ao", &Material::ao,
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
