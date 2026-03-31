#pragma once
#include <string>
#include <vector>
#include <assimp/material.h>

namespace ENGINE_RENDERING {
    struct TextureSlotDefinition {
        std::string key;            // 内部存储 Key (如 "diffuse")
        std::string displayName;    // UI 显示名称 (如 "Diffuse Map")
        std::string shaderFlag;     // Shader 中的布尔开关 (如 "material.useDiffuse")
        std::string shaderSampler;  // Shader 中的采样器名 (如 "material.diffuse")
        aiTextureType assimpType;   // 对应的 Assimp 贴图类型
    };

    class TextureRegistry {
    public:
        static const std::vector<TextureSlotDefinition>& GetSlots()
        {
            static std::vector<TextureSlotDefinition> slots = {
                { "albedo",  "Albedo",  "material.useAlbedo",  "material.albedoMap",    aiTextureType_DIFFUSE },
                { "normal",   "Normal",   "material.useNormal",   "material.normalMap",     aiTextureType_HEIGHT },
                { "metallic", "Metallic", "material.useMetallic", "material.metallic", aiTextureType_METALNESS },
                { "roughness", "Roughness", "material.useRoughness", "material.roughness", aiTextureType_MAYA_SPECULAR_ROUGHNESS },
                { "ao", "Ao", "material.useAo", "material.ao", aiTextureType_AMBIENT_OCCLUSION}
            };
            return slots;
        }
    };
}