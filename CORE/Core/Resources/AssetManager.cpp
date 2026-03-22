#include "AssetManager.h"
#include "Utilities/EngineUtilities.h"
#include<../CORE/Core/ECS/MainRegistry.h>
#include<Rendering/Essentials/ModelLoader.h>
#include<Rendering/Essentials/TextureLoader.h>
#include<Rendering/Essentials/ShaderLoader.h>
#include<Logger/Logger.h>

namespace ENGINE_RESOURCES {

    Mix_MusicType AssetManager::DetectAudioFormat(const unsigned char* audioData, size_t dataSize)
    {
        if (!audioData || dataSize < 12)
        {
            ENGINE_ERROR("Failed to detect the audio format. Data or size is invalid.");
            return MUS_NONE;
        }

        // WAV Format
        if (std::memcmp(audioData, "RIFF", 4) == 0 && std::memcmp(audioData + 8, "WAVE", 4) == 0)
        {
            return MUS_WAV;
        }

        // MP3 Format
        if (std::memcmp(audioData, "ID3", 3) == 0 || audioData[0] == 0xFF && (audioData[1] & 0xE0) == 0xE0)
        {
            return MUS_MP3;
        }

        // OGG Format
        if (std::memcmp(audioData, "OggS", 4) == 0)
        {
            return MUS_OGG;
        }

        // Flac Format
        if (std::memcmp(audioData, "fLaC", 4) == 0)
        {
            return MUS_FLAC;
        }

        if (dataSize >= 36 && std::memcmp(audioData + 28, "OpusHead", 8) == 0)
        {
            return MUS_OPUS;
        }

        ENGINE_ERROR("Failed to detect audio type - Unknown or unsupported format.");

        return MUS_NONE;
    }

    // model
    bool AssetManager::AddModel(const std::string& modelName, const std::string& modelPath, std::map<std::string, std::string>& textures)
    {
        if (m_mapModel.find(modelName) != m_mapModel.end())
        {
            ENGINE_ERROR("Failed to add model [{0}] -- Already exists!", modelName);
            return false;
        }

        auto model = std::move(ENGINE_RENDERING::ModelLoader::CreateModel(modelPath, textures));
        if (!model)
        {
            ENGINE_ERROR("Failed to load model [{0}] at path [{1}]", modelName, modelPath);
            return false;
        }
        model->SetDir(modelPath.substr(0, modelPath.find_last_of('/')));

        m_mapModel.emplace(modelName, std::move(model));
        return true;
    }
    bool AssetManager::AddModelFromMemory(const std::string& modelName, const std::string& shapeName)
    {
        if (m_mapModel.contains(modelName))
        {
            ENGINE_ERROR("AssetManager: Model [{}] -- Already exists!", modelName);
            return false;
        }
        auto pModel = ENGINE_RENDERING::ModelLoader::CreateModelFromMemory(shapeName);
        if (!pModel)
        {
            ENGINE_ERROR("Failed to load model [{}] from memory!", modelName);
            return false;
        }

        auto [itr, bSuccess] = m_mapModel.emplace(modelName, std::move(pModel));
        return bSuccess;
    }
    std::shared_ptr<ENGINE_RENDERING::Model> AssetManager::GetModel(const std::string& modelName)
    {
        auto modelItr = m_mapModel.find(modelName);
        if (modelItr == m_mapModel.end())
        {
            ENGINE_ERROR("Failed to get model [{0}] -- Does not exist!", modelName);
            return nullptr;
        }
        return modelItr->second;
    }

    // texture
    bool AssetManager::AddTexture(const std::string& textureName, const std::string& texturePath, bool pixelArt)
    {
        // check if texture already loaded
        if (m_mapTexture.find(textureName) != m_mapTexture.end())
        {
            ENGINE_ERROR("Failed to add texture [{0}] -- Already exists!", textureName);
            return false;
        }

        auto texture = std::move(ENGINE_RENDERING::TextureLoader::Create(
            pixelArt ? ENGINE_RENDERING::Texture::TextureType::PIXEL : ENGINE_RENDERING::Texture::TextureType::BLENDED,
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
    bool AssetManager::AddTextureFromMemory(const std::string& texName, const unsigned char* imageData, size_t length, bool pixelArt)
    {
        if (m_mapTexture.contains(texName))
        {
            ENGINE_ERROR("AssetManager: Texture [{}] -- Already exists!", texName);
            return false;
        }

        auto pTexture = ENGINE_RENDERING::TextureLoader::CreateFromMemory(imageData, length, !pixelArt);
        if (!pTexture)
        {
            ENGINE_ERROR("Failed to load texture [{}] from memory!", texName);
            return false;
        }

        auto [itr, bSuccess] = m_mapTexture.emplace(texName, std::move(pTexture));
        return bSuccess;
    }
    bool AssetManager::AddSkyboxTexture(const std::string& textureName, const std::string& texturePath, bool pixelArt)
    {
        // check if texture already loaded
        if (m_mapTexture.find(textureName) != m_mapTexture.end())
        {
            ENGINE_ERROR("Failed to add texture [{0}] -- Already exists!", textureName);
            return false;
        }

        auto texture = std::move(ENGINE_RENDERING::TextureLoader::CreateSkybox(
            pixelArt ? ENGINE_RENDERING::Texture::TextureType::PIXEL : ENGINE_RENDERING::Texture::TextureType::BLENDED,
            texturePath));
        if (!texture)
        {
            ENGINE_ERROR("Failed to load texture [{0}] at path [{1}]", textureName, texturePath);
            return false;
        }

        m_mapTexture.emplace(textureName, std::move(texture));
        return true;
    }
    std::shared_ptr<ENGINE_RENDERING::Texture> AssetManager::GetTexture(const std::string& textureName)
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
    bool AssetManager::AddShader(const std::string& shaderName, const std::string& vertexPath, const std::string& fragmentPath, const std::string& geometryPath)
    {
        // check if shader already loaded
        if (m_mapShader.find(shaderName) != m_mapShader.end())
        {
            ENGINE_ERROR("Failed to add shader [{0}] -- Already exists!", shaderName);
            return false;
        }
        auto shader = std::move(ENGINE_RENDERING::ShaderLoader::Create(vertexPath, fragmentPath, geometryPath));
        if (!shader)
        {
            ENGINE_ERROR("Failed to load shader [{0}] at vert path [{1}], frag path [{2}] and geom path [{3}]", shaderName, vertexPath, fragmentPath, geometryPath);
            return false;
        }
        m_mapShader.emplace(shaderName, std::move(shader));
        return true;
    }
    bool AssetManager::AddShaderFromMemory(const std::string& shaderName, const char* vertexShader, const char* fragmentShader, const char* geometryShader)
    {
        if (m_mapShader.contains(shaderName))
        {
            ENGINE_ERROR("Failed to add shader - [{0}] -- Already exists!", shaderName);
            return false;
        }

        auto pShader = ENGINE_RENDERING::ShaderLoader::CreateFromMemory(vertexShader, fragmentShader, geometryShader);
        auto [itr, bSuccess] = m_mapShader.insert(std::make_pair(shaderName, std::move(pShader)));

        return bSuccess;
    }
    std::shared_ptr<ENGINE_RENDERING::Shader> AssetManager::GetShader(const std::string& shaderName)
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
        ENGINE_SOUNDS::SoundParams params{
            .name = musicName,
            .filename = musicPath,
            .duration = Mix_MusicDuration(music)
        };

        // music pointer
        auto musicPtr = std::make_shared<ENGINE_SOUNDS::Music>(params, MusicPtr{ music });
        if (!musicPtr)
        {
            ENGINE_ERROR("Failed to create the must ptr for [{}]", musicName);
            return false;
        }

        m_mapMusic.emplace(musicName, std::move(musicPtr));
        
        return true;
    }
    bool AssetManager::AddMusicFromMemory(const std::string& musicName, const unsigned char* musicData, size_t dataSize)
    {
        if (m_mapMusic.contains(musicName))
        {
            ENGINE_ERROR("Failed to add music [{}] -- Already exists!", musicName);
            return false;
        }

        SDL_RWops* rw = SDL_RWFromMem((void*)musicData, static_cast<int>(dataSize));
        Mix_MusicType type = DetectAudioFormat(musicData, dataSize);
        
        if (type == MUS_NONE)
        {
            ENGINE_ERROR("Failed to add music [{}] from memory. Unable to determine music type.", musicName);
            return false;
        }

        auto pMusic = Mix_LoadMUSType_RW(rw, type, 1);
        if (!pMusic)
        {
            ENGINE_ERROR("Failed to add music [{}] from memory.", musicName);
            return false;
        }

        // Create the sound parameters
        ENGINE_SOUNDS::SoundParams params{
            .name = musicName, .filename = "From Data", .duration = Mix_MusicDuration(pMusic) };

        // Create the music Pointer
        auto pMusicPtr = std::make_shared<ENGINE_SOUNDS::Music>(params, MusicPtr{ pMusic });
        if (!pMusicPtr)
        {
            ENGINE_ERROR("Failed to create the music ptr for [{}]", musicName);
            return false;
        }

        auto [itr, bSuccess] = m_mapMusic.emplace(musicName, std::move(pMusicPtr));

        return bSuccess;
    }
    std::shared_ptr<ENGINE_SOUNDS::Music> AssetManager::GetMusic(const std::string& musicName)
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
        ENGINE_SOUNDS::SoundParams params{
            .name = soundFxName,
            .filename = soundFxPath,
            .duration = chunk->alen / 179.4
        };

        // chunk pointer
        auto chunkPtr = std::make_shared<ENGINE_SOUNDS::SoundFx>(params, SoundFxPtr{ chunk });
        if (!chunkPtr)
        {
            ENGINE_ERROR("Failed to create the must ptr for [{}]", soundFxName);
            return false;
        }

        m_mapSoundFx.emplace(soundFxName, std::move(chunkPtr));

        return true;
    }
    bool AssetManager::AddSoundFxFromMemory(const std::string& soundFxName, const unsigned char* soundFxData, size_t dataSize)
    {
        if (m_mapSoundFx.contains(soundFxName))
        {
            ENGINE_ERROR("Failed to add soundfx [{}] -- Already exists!", soundFxName);
            return false;
        }

        SDL_RWops* rw = SDL_RWFromMem((void*)soundFxData, static_cast<int>(dataSize));
        auto pChunk = Mix_LoadWAV_RW(rw, 1);
        if (!pChunk)
        {
            ENGINE_ERROR("Failed to add soundfx [{}] from memory.", soundFxName);
            return false;
        }

        ENGINE_SOUNDS::SoundParams params{ .name = soundFxName, .filename = "From Data", .duration = pChunk->alen / 179.4 };

        auto pSoundFx = std::make_shared<ENGINE_SOUNDS::SoundFx>(params, SoundFxPtr{ pChunk });
        auto [itr, bSuccess] = m_mapSoundFx.emplace(soundFxName, std::move(pSoundFx));

        return bSuccess;
    }
    std::shared_ptr<ENGINE_SOUNDS::SoundFx> AssetManager::GetSoundFx(const std::string& soundFxName)
    {
        auto soundFxItr = m_mapSoundFx.find(soundFxName);
        if (soundFxItr == m_mapSoundFx.end())
        {
            ENGINE_ERROR("Failed to get soundFx [{}] -- Does not exists!", soundFxName);
            return nullptr;
        }
        return soundFxItr->second;
    }

    /* get all asset key names of specific asset type*/
    std::vector<std::string> AssetManager::GetAssetKeyName(ENGINE_UTIL::AssetType eAssetType) const
    {
        switch (eAssetType)
        {
            case ENGINE_UTIL::AssetType::TEXTURE:
            {
                return ENGINE_UTIL::GetKeys(m_mapTexture, [](const auto& pair) {return !pair.second->IsEditorTexture(); });
                break;
            }
            case ENGINE_UTIL::AssetType::MODEL:
            {
                return ENGINE_UTIL::GetKeys(m_mapModel, [](const auto& pair) {return !pair.second->IsEditorModel(); });
                break;
            }
            case ENGINE_UTIL::AssetType::MUSIC:
            {
                return ENGINE_UTIL::GetKeys(m_mapMusic);
                break;
            }
            case ENGINE_UTIL::AssetType::SOUNDFX:
            {
                return ENGINE_UTIL::GetKeys(m_mapSoundFx);
                break;
            }
            case ENGINE_UTIL::AssetType::SHADER:
            {
                return ENGINE_UTIL::GetKeys(m_mapShader);
                break;
            }
            default:
            {
                assert(false && "Cannot get this type!");
                break;
            }
        }
    }

    /* change the asset name of specific asset type from "sOldName" to "sNewName */
    bool AssetManager::ChangeAssetName(const std::string& sOldName, const std::string& sNewName, ENGINE_UTIL::AssetType eAssetType)
    {
        switch (eAssetType)
        {
        case ENGINE_UTIL::AssetType::TEXTURE:return ENGINE_UTIL::ChangeKey(m_mapTexture, sOldName, sNewName);
        case ENGINE_UTIL::AssetType::MUSIC:return ENGINE_UTIL::ChangeKey(m_mapMusic, sOldName, sNewName);
        case ENGINE_UTIL::AssetType::SOUNDFX:return ENGINE_UTIL::ChangeKey(m_mapSoundFx, sOldName, sNewName);
        default:assert(false && "Cannot get this type!");
        }
        return false;
    }

    bool AssetManager::CheckHasAsset(const std::string& checkName, ENGINE_UTIL::AssetType eAssetType)
    {
        switch (eAssetType)
        {
        case ENGINE_UTIL::AssetType::TEXTURE:return m_mapTexture.contains(checkName);
        case ENGINE_UTIL::AssetType::MUSIC:return m_mapMusic.contains(checkName);
        case ENGINE_UTIL::AssetType::SOUNDFX:return m_mapSoundFx.contains(checkName);
        default:assert(false && "Cannot get this type!");
        }
        return false;
    }

    bool AssetManager::DeleteAsset(const std::string& assetName, ENGINE_UTIL::AssetType eAssetType)
    {
        switch (eAssetType)
        {
        case ENGINE_UTIL::AssetType::TEXTURE:return std::erase_if(m_mapTexture, [&](const auto& pair) {return pair.first == assetName; }) > 0;
        case ENGINE_UTIL::AssetType::MUSIC:return std::erase_if(m_mapMusic, [&](const auto& pair) {return pair.first == assetName; }) > 0;
        case ENGINE_UTIL::AssetType::SOUNDFX:return std::erase_if(m_mapSoundFx, [&](const auto& pair) {return pair.first == assetName; }) > 0;
        default:assert(false && "Cannot get this type!");
        }
        return false;
    }

    // lua register
    void AssetManager::CreateLuaAssetManagerBind(sol::state& lua, ENGINE_CORE::ECS::Registry& registry)
    {
        //auto& assetManager = registry.GetContext<std::shared_ptr<ENGINE_RESOURCES::AssetManager>>();
        auto& mainRegistry = MAIN_REGISTRY();
        auto& assetManager = mainRegistry.GetAssetManager();

        //TODO: add_shader
        lua.new_usertype<AssetManager>(
            "AssetManager",
            sol::no_constructor,
            "add_model", [&](const std::string& meshName, const std::string& meshPath) {
                std::map<std::string, std::string> textures;
                if (!assetManager.AddModel(meshName, meshPath, textures)) return false;
                for (const auto& [texName, texPath] : textures) {
                    if (!assetManager.CheckHasAsset(texName, ENGINE_UTIL::AssetType::TEXTURE)) {
                        assetManager.AddTexture(texName, texPath, false);
                    }
                }
                return true;
            },
            "add_texture",[&](const std::string& texName,const std::string& texPath, bool pixelArt){
                return assetManager.AddTexture(texName, texPath, pixelArt);
            },
            "add_music", [&](const std::string& musicName, const std::string& musicPath) {
                return assetManager.AddMusic(musicName, musicPath);
            },
            "add_soundFx", [&](const std::string& soundFxName, const std::string& soundFxPath) {
                return assetManager.AddSoundFx(soundFxName, soundFxPath);
            }
        );
    }
}


