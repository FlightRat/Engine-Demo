#pragma once
#include<map>
#include<memory>
#include<string>
#include<Rendering/Essentials/Shader.h>
#include<Rendering/Essentials/Texture.h> 

namespace RESOURCES {
	class AssetManager
	{
	private:
		std::map<std::string, std::shared_ptr<RENDERING::Texture>> m_mapTexture{};
		std::map<std::string, std::shared_ptr<RENDERING::Shader>> m_mapShader{};
	public:
		AssetManager() = default;
		~AssetManager() = default;

		bool AddTexture(const std::string& textureName, const std::string& texturePath, bool pixelArt = true);
		const RENDERING::Texture& GetTexture(const std::string& textureName);

		bool AddShader(const std::string& shaderName, const std::string& vertexPath, const std::string& fragmentPath);
		RENDERING::Shader& GetShader(const std::string& shaderName);
	};
}