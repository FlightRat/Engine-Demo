#include "AssetManager.h"
#include<Rendering/Essentials/TextureLoader.h>
#include<Rendering/Essentials/ShaderLoader.h>
#include<Logger/Logger.h>

namespace RESOURCES {
    bool AssetManager::AddTexture(const std::string& textureName, const std::string& texturePath, bool pixelArt)
    {
        // check if texture already loaded
        if (m_mapTexture.find(textureName) != m_mapTexture.end())
        {
            ENGINE_ERROR("Failed to add texture [{0}] -- Already exists!", textureName);
            return false;
        }

        auto texture = std::move(RENDERING::TextureLoader::Create(
            pixelArt ? RENDERING::Texture::TextureType::PIXEL : RENDERING::Texture::TextureType::BLENDED,
            texturePath
        ));
        if (!texture)
        {
            ENGINE_ERROR("Failed to load texture [{0}] at path [{1}]", textureName, texturePath);
            return false;
        }

        m_mapTexture.emplace(textureName, std::move(texture));
        return true;
    }

    const RENDERING::Texture& AssetManager::GetTexture(const std::string& textureName)
    {
        auto texItr = m_mapTexture.find(textureName);
        if (texItr == m_mapTexture.end())
        {
            ENGINE_ERROR("Failed to get texture [{0}] -- Does not exist!", textureName);
            return RENDERING::Texture();
        }
        return *texItr->second;
    }

    bool AssetManager::AddShader(const std::string& shaderName, const std::string& vertexPath, const std::string& fragmentPath)
    {
        // check if shader already loaded
        if (m_mapShader.find(shaderName) != m_mapShader.end())
        {
            ENGINE_ERROR("Failed to add shader [{0}] -- Already exists!", shaderName);
            return false;
        }
        auto shader = std::move(RENDERING::ShaderLoader::Create(vertexPath, fragmentPath));
        if (!shader)
        {
            ENGINE_ERROR("Failed to load shader [{0}] at vert path [{1}] and frag path [{2}]", shaderName, vertexPath, fragmentPath);
            return false;
        }
        m_mapShader.emplace(shaderName, std::move(shader));
        return true;
    }

    RENDERING::Shader& AssetManager::GetShader(const std::string& shaderName)
    {
        auto shaderItr = m_mapShader.find(shaderName);
        if (shaderItr == m_mapShader.end())
        {
            ENGINE_ERROR("Failed to get shader [{0}] -- Does not exist!", shaderName);
            RENDERING::Shader shader{};
            return shader;
        }
        return *shaderItr->second;
    }
    void AssetManager::CreateLuaAssetManager(sol::state& lua, CORE::ECS::Registry& registry)
    {
        auto& assetManager = registry.GetContext<std::shared_ptr<RESOURCES::AssetManager>>();
        if (!assetManager)
        {
            ENGINE_ERROR("Failed to bind the asset manager to lua - Does not exist in the registry!");
            return;
        }

        lua.new_usertype<AssetManager>(
            "AssetManager",
            sol::no_constructor,
            "add_texture",[&](const std::string& texName,const std::string& texPath, bool pixelArt){
                return assetManager->AddTexture(texName, texPath, pixelArt);
            }
        );
    }
}


