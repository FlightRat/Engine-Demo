#pragma once
#include<map>
#include<memory>
#include<string>
#include<sol/sol.hpp>
#include"../ECS/Registry.h"
#include<Rendering/Essentials/Shader.h>
#include<Rendering/Essentials/Texture.h> 
#include<Rendering/Essentials/Model.h> 
#include<Sounds/Essentials/Music.h>
#include<Sounds/Essentials/SoundFx.h>

namespace ENGINE_UTIL
{
	enum class AssetType;
}

namespace ENGINE_RESOURCES {
	class AssetManager
	{
	private:
		std::map<std::string, std::shared_ptr< ENGINE_RENDERING::Model>> m_mapModel{};
		std::map<std::string, std::shared_ptr<ENGINE_RENDERING::Texture>> m_mapTexture{};
		std::map<std::string, std::shared_ptr<ENGINE_RENDERING::Shader>> m_mapShader{};
		std::map<std::string, std::shared_ptr<ENGINE_SOUNDS::Music>> m_mapMusic{};
		std::map<std::string, std::shared_ptr<ENGINE_SOUNDS::SoundFx>> m_mapSoundFx{};

		const std::vector<std::string> m_SelectableMesh{ "cube", "sphere", "capsule"};
	public:
		AssetManager() = default;
		~AssetManager() = default;

		Mix_MusicType DetectAudioFormat(const unsigned char* audioData, size_t dataSize);

		bool AddModel(const std::string& modelName, const std::string& modelPath);
		bool AddModelFromMemory(const std::string& modelName, const std::string& shapeName);
		std::shared_ptr<ENGINE_RENDERING::Model> GetModel(const std::string& modelName);

		bool AddTexture(const std::string& textureName, const std::string& texturePath, bool pixelArt = true);
		bool AddTextureFromMemory(const std::string& texName, const unsigned char* imageData, size_t length, bool pixelArt = true);
		std::shared_ptr<ENGINE_RENDERING::Texture> GetTexture(const std::string& textureName);

		bool AddShader(const std::string& shaderName, const std::string& vertexPath, const std::string& fragmentPath);
		bool AddShaderFromMemory(const std::string& shaderName, const char* vertexShader, const char* fragmentShader);
		std::shared_ptr<ENGINE_RENDERING::Shader> GetShader(const std::string& shaderName);

		bool AddMusic(const std::string& musicName, const std::string& musicPath);
		bool AddMusicFromMemory(const std::string& musicName, const unsigned char* musicData, size_t dataSize);
		std::shared_ptr<ENGINE_SOUNDS::Music> GetMusic(const std::string& musicName);

		bool AddSoundFx(const std::string& soundFxName, const std::string& soundFxPath);
		bool AddSoundFxFromMemory(const std::string& soundFxName, const unsigned char* soundFxData, size_t dataSize);
		std::shared_ptr<ENGINE_SOUNDS::SoundFx> GetSoundFx(const std::string& soundFxName);

		inline const std::map<std::string, std::shared_ptr<ENGINE_RENDERING::Texture>>& GetAllTextures() const { return m_mapTexture; }
		inline const std::vector<std::string> GetSelectableMesh(){ return m_SelectableMesh; }

		std::vector<std::string> GetAssetKeyName(ENGINE_UTIL::AssetType eAssetType) const;
		bool ChangeAssetName(const std::string& sOldName, const std::string& sNewName, ENGINE_UTIL::AssetType eAssetType);
		bool CheckHasAsset(const std::string& checkName, ENGINE_UTIL::AssetType eAssetType);
		bool DeleteAsset(const std::string& assetName, ENGINE_UTIL::AssetType eAssetType);

		static void CreateLuaAssetManagerBind(sol::state& lua, ENGINE_CORE::ECS::Registry& registry);
	};
}