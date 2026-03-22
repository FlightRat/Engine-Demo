#pragma once
#include"Texture.h"
#include<memory>

namespace ENGINE_RENDERING {
	class TextureLoader
	{
	private:
		static bool LoadTexture(const std::string& filepath, GLuint& id, int& width, int& height, bool blended = false);
		static bool LoadFBTexture_normal(GLuint& id, int& width, int& height);
		static bool LoadFBTexture_multisample(GLuint& id, int& width, int& height);
		static bool LoadShadowmapTexture(GLuint& id, int& width, int& height);
		static bool LoadShadowCubemapTexture(GLuint& id, int& width, int& height);
		static bool LoadSkyboxTexture(const std::string filepath, GLuint& id, int& width, int& height, bool blended = false);
		static bool LoadTextureFromMemory(const unsigned char* imageData, size_t length, GLuint& id, int& width, 
			int& height, bool blended = false);
	public:
		TextureLoader() = delete;
		static std::shared_ptr<Texture> Create(Texture::TextureType type, const std::string& texturePath);
		static std::shared_ptr<Texture> Create(Texture::TextureType type, int width, int height, const bool multiSample=false);
		static std::shared_ptr<Texture> CreateSkybox(Texture::TextureType type, const std::string& texturePath);
		static std::shared_ptr<Texture> CreateFromMemory(const unsigned char* imageData, size_t length,
			bool blended = false, bool bTileset = false);
	};
}