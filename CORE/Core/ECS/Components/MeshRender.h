#pragma once
#include<glm/glm.hpp>
#include<sol/sol.hpp>
#include<string>
#include<vector>
#include<map>
#include"Rendering/Essentials/Mesh.h"

namespace ENGINE_CORE::ECS {
    struct Material
    {
        std::string shaderName;

        glm::vec4 color{ 1.0f };
        float shininess = 32.0f;

        bool m_useTexture{ false };
		std::map<std::string, std::string> m_textures;
        void AddTexture(const std::string& key, const std::string& texName);
    };

    struct MeshRender
    {
        bool shouldRender{ true };
        bool flipUV{ false };
        std::vector<Material> materials;

        MeshRender() = default;
        MeshRender(const std::vector<Material>& pMaterials);
        ~MeshRender() = default;

        void SetColor(glm::vec4 color);
        Material& GetMaterial(size_t index);
        inline const bool CheckMaterialEmpty() { return materials.empty(); }
        void AddMaterial(const Material& material);
        void ResetMaterial(const std::vector<ENGINE_RENDERING::Mesh>& meshes);

        static void CreateLuaMeshRendererBind(sol::state& lua);
    };
}