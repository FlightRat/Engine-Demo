#include "AssetManager.h"
#include<Rendering/Essentials/TextureLoader.h>
#include<Rendering/Essentials/ShaderLoader.h>
#include<Logger/Logger.h>

namespace RESOURCES {
    // texture
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
    std::shared_ptr<RENDERING::Texture> AssetManager::GetTexture(const std::string& textureName)
    {
        auto texItr = m_mapTexture.find(textureName);
        if (texItr == m_mapTexture.end())
        {
            ENGINE_ERROR("Failed to get texture [{0}] -- Does not exist!", textureName);
            return nullptr;
        }
        return texItr->second;
    }

    // shader
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
    std::shared_ptr<RENDERING::Shader> AssetManager::GetShader(const std::string& shaderName)
    {
        auto shaderItr = m_mapShader.find(shaderName);
        if (shaderItr == m_mapShader.end())
        {
            ENGINE_ERROR("Failed to get shader [{0}] -- Does not exist!", shaderName);
            return nullptr;
        }
        return shaderItr->second;
    }

    // music
    bool AssetManager::AddMusic(const std::string& musicName, const std::string& musicPath)
    {   
        // check if exists
        if (m_mapMusic.find(musicName) != m_mapMusic.end())
        {
            ENGINE_ERROR("Failed to add Music [{0}] -- Already exists!", musicName);
            return false;
        }

        // load music data
        Mix_Music* music = Mix_LoadMUS(musicPath.c_str());
        if (!music)
        {
            std::string error{ Mix_GetError() };
            ENGINE_ERROR("Failed to load [{}] at path [{}]-- Mixer Error:{}", musicName, musicPath, error);
            return false;
        }

        // sound param
        SOUNDS::SoundParams params{
            .name = musicName,
            .filename = musicPath,
            .duration = Mix_MusicDuration(music)
        };

        // music pointer
        auto musicPtr = std::make_shared<SOUNDS::Music>(params, MusicPtr{ music });
        if (!musicPtr)
        {
            ENGINE_ERROR("Failed to create the must ptr for [{}]", musicName);
            return false;
        }

        m_mapMusic.emplace(musicName, std::move(musicPtr));
        
        return true;
    }
    std::shared_ptr<SOUNDS::Music> AssetManager::GetMusic(const std::string& musicName)
    {
        auto musicItr = m_mapMusic.find(musicName);
        if (musicItr == m_mapMusic.end())
        {
            ENGINE_ERROR("Failed to get music [{0}] -- Does not exists!", musicName);
            return nullptr;
        }
        return musicItr->second;
    }

    // soundFx
    bool AssetManager::AddSoundFx(const std::string& soundFxName, const std::string& soundFxPath)
    {
        // check if exists
        if (m_mapSoundFx.find(soundFxName) != m_mapSoundFx.end())
        {
            ENGINE_ERROR("Failed to add SoundFx [{0}] -- Already exists!", soundFxName);
            return false;
        }

        // load soundfx data
        Mix_Chunk* chunk = Mix_LoadWAV(soundFxPath.c_str());
        if (!chunk)
        {
            std::string error{ Mix_GetError() };
            ENGINE_ERROR("Failed to load [{}] at path [{}] -- Mixer Error [{}]", soundFxName, soundFxPath, error);
            return false;
        }

        // params
        SOUNDS::SoundParams params{
            .name = soundFxName,
            .filename = soundFxPath,
            .duration = chunk->alen / 179.4
        };

        // chunk pointer
        auto chunkPtr = std::make_shared<SOUNDS::SoundFx>(params, SoundFxPtr{ chunk });
        if (!chunkPtr)
        {
            ENGINE_ERROR("Failed to create the must ptr for [{}]", soundFxName);
            return false;
        }

        m_mapSoundFx.emplace(soundFxName, std::move(chunkPtr));

        return true;
    }
    std::shared_ptr<SOUNDS::SoundFx> AssetManager::GetSoundFx(const std::string& soundFxName)
    {
        auto soundFxItr = m_mapSoundFx.find(soundFxName);
        if (soundFxItr == m_mapSoundFx.end())
        {
            ENGINE_ERROR("Failed to get soundFx [{}] -- Does not exists!", soundFxName);
            return nullptr;
        }
        return soundFxItr->second;
    }

    // lua register
    void AssetManager::CreateLuaAssetManager(sol::state& lua, CORE::ECS::Registry& registry)
    {
        auto& assetManager = registry.GetContext<std::shared_ptr<RESOURCES::AssetManager>>();
        if (!assetManager)
        {
            ENGINE_ERROR("Failed to bind the asset manager to lua - Does not exist in the registry!");
            return;
        }

        //TODO: add_shader
        lua.new_usertype<AssetManager>(
            "AssetManager",
            sol::no_constructor,
            "add_texture",[&](const std::string& texName,const std::string& texPath, bool pixelArt){
                return assetManager->AddTexture(texName, texPath, pixelArt);
            },
            "add_music", [&](const std::string& musicName, const std::string& musicPath) {
                return assetManager->AddMusic(musicName, musicPath);
            },
            "add_soundFx", [&](const std::string& soundFxName, const std::string& soundFxPath) {
                return assetManager->AddSoundFx(soundFxName, soundFxPath);
            }
        );
    }
}


