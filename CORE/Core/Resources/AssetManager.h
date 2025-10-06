#pragma once
#include<map>
#include<memory>
#include<string>
#include<sol/sol.hpp>
#include"../ECS/Registry.h"
#include<Rendering/Essentials/Shader.h>
#include<Rendering/Essentials/Texture.h> 
#include<Sounds/Essentials/Music.h>
#include<Sounds/Essentials/SoundFx.h>

namespace RESOURCES {
	class AssetManager
	{
	private:
		std::map<std::string, std::shared_ptr<RENDERING::Texture>> m_mapTexture{};
		std::map<std::string, std::shared_ptr<RENDERING::Shader>> m_mapShader{};
		std::map<std::string, std::shared_ptr<SOUNDS::Music>> m_mapMusic{};
		std::map<std::string, std::shared_ptr<SOUNDS::SoundFx>> m_mapSoundFx{};
	public:
		AssetManager() = default;
		~AssetManager() = default;

		bool AddTexture(const std::string& textureName, const std::string& texturePath, bool pixelArt = true);
		std::shared_ptr<RENDERING::Texture> GetTexture(const std::string& textureName);

		bool AddShader(const std::string& shaderName, const std::string& vertexPath, const std::string& fragmentPath);
		std::shared_ptr<RENDERING::Shader> GetShader(const std::string& shaderName);

		bool AddMusic(const std::string& musicName, const std::string& musicPath);
		std::shared_ptr<SOUNDS::Music> GetMusic(const std::string& musicName);

		bool AddSoundFx(const std::string& soundFxName, const std::string& soundFxPath);
		std::shared_ptr<SOUNDS::SoundFx> GetSoundFx(const std::string& soundFxName);

		static void CreateLuaAssetManager(sol::state& lua, CORE::ECS::Registry& registry);
	};
}