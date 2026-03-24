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
                { "diffuse",  "Diffuse",  "material.useDiffuse",  "material.diffuse",    aiTextureType_DIFFUSE },
                { "specular", "Specular", "material.useSpecular", "material.specular",   aiTextureType_SPECULAR },
                { "normal",   "Normal",   "material.useNormal",   "material.normal",     aiTextureType_HEIGHT }, // Assimp中法线常映射到Height
                { "reflect",  "Reflect",  "material.useReflect",  "material.reflection", aiTextureType_AMBIENT }
                // 如果要新增 Roughness，只需在这里添加一行：
                // { "roughness", "Roughness", "material.useRoughness", "material.roughness", aiTextureType_SHININESS }
            };
            return slots;
        }
    };
}