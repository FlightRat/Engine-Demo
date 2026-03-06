#pragma once
#include<glm/glm.hpp>
#include<sol/sol.hpp>
#include<string>
#include<vector>
#include<map>
#include"MeshFilter.h"

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
        std::vector<Material> materials;

        MeshRender() = default;
        MeshRender(const std::vector<Material>& pMaterials);
        ~MeshRender() = default;

        Material& GetMaterial(size_t index);
        void AddMaterial(const Material& material);

        static void CreateLuaMeshRendererBind(sol::state& lua);
    };
}