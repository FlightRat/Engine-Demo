#include "TextureLoader.h"
#include <filesystem>
#include <SOIL/SOIL.h>
#include "Logger/Logger.h"

namespace ENGINE_RENDERING{
	bool TextureLoader::LoadTexture(const std::string& filepath, GLuint& id, int& width, int& height, bool blended)
	{
		int channels = 0;
		
		// clang-format off
		unsigned char* image = SOIL_load_image(filepath.c_str(), // Filename			-- Image file to be loaded
			&width,			  // Width				-- Width of the image
			&height,		  // height				-- Height of the image
			&channels,		  // channels			-- Number of channels
			SOIL_LOAD_AUTO	  // force_channels		-- Force the channels count
		);
		// clang-format on

		// Check to see if the image is successful
		if (!image)
		{
			ENGINE_ERROR("SOIL failed to load image [{0}] -- {1}", filepath, SOIL_last_result());
			return false;
		}

		GLint format = GL_RGBA;

		switch (channels)
		{
		case 3: format = GL_RGB; break;
		case 4: format = GL_RGBA; break;
		}

		glTexImage2D(
			GL_TEXTURE_2D,	// target			-- Specifies the target texture
			0,				// level			-- Level of detail. 0 is the base image level
			format,			// internal format	-- The number of color components
			width,			// width			-- width of the texture image
			height,			// height			-- height of the texture image
			0,				// border
			format,			// format			-- format of the pixel data
			GL_UNSIGNED_BYTE, // type				-- The data type of the pixel data
			image				// data
		);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT); // for this tutorial: use GL_CLAMP_TO_EDGE to prevent semi-transparent borders. Due to interpolation it takes texels from next repeat 
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
		
		if (!blended)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		}
		else
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		}

		// Delete the image data from SOIL
		SOIL_free_image_data(image);

		return true;
	}

	bool TextureLoader::LoadFBTexture_normal(GLuint& id, int& width, int& height)
	{
		glBindTexture(GL_TEXTURE_2D, id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0);
		return true;
	}

	bool TextureLoader::LoadFBTexture_multisample(GLuint& id, int& width, int& height)
	{
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, id);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA, width, height, GL_TRUE);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
		return true;
	}

	bool TextureLoader::LoadShadowmapTexture(GLuint& id, int& width, int& height)
	{
		float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };	//边界填充

		glBindTexture(GL_TEXTURE_2D, id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
		glBindTexture(GL_TEXTURE_2D, 0);
		return true;
	}

	bool TextureLoader::LoadSkyboxTexture(const std::string filepath, GLuint& id, int& width, int& height, bool blended)
	{
		// 1. 定义 OpenGL 要求的 Cubemap 标准顺序
			// 对应：右 (px), 左 (nx), 上 (py), 下 (ny), 前 (pz), 后 (nz)
		std::vector<std::string> suffixes = {"right", "left", "top", "bottom", "front", "back"};

		std::vector<std::string> faces(6, ""); // 预留6个位置供排序

		try {
			if (!std::filesystem::exists(filepath) || !std::filesystem::is_directory(filepath)) {
				ENGINE_ERROR("Skybox path does not exist or is not a directory: {0}", filepath);
				return false;
			}

			// 2. 遍历文件夹并根据关键字匹配顺序
			for (const auto& entry : std::filesystem::directory_iterator(filepath)) {
				std::string fileName = entry.path().filename().string();
				std::string fullPath = entry.path().string();

				// 将文件名转为小写进行模糊匹配
				std::string lowerName = fileName;
				std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

				for (int i = 0; i < 6; ++i) {
					if (lowerName.find(suffixes[i]) != std::string::npos) {
						faces[i] = fullPath;
						break;
					}
				}
			}
		}
		catch (const std::exception& e) {
			ENGINE_ERROR("Filesystem error: {0}", e.what());
			return false;
		}

		// 检查是否找齐了6张图
		for (int i = 0; i < 6; i++) {
			if (faces[i].empty()) {
				ENGINE_ERROR("Skybox face [{0}] missing in directory: {1}", suffixes[i], filepath);
				return false;
			}
		}

		int channels = 0;
		for (unsigned int i = 0; i < faces.size(); i++)
		{
			// clang-format off
			unsigned char* image = SOIL_load_image(faces[i].c_str(), // Filename			-- Image file to be loaded
				&width,			  // Width				-- Width of the image
				&height,		  // height				-- Height of the image
				&channels,		  // channels			-- Number of channels
				SOIL_LOAD_AUTO	  // force_channels		-- Force the channels count
			);
			// clang-format on

			// Check to see if the image is successful
			if (!image)
			{
				ENGINE_ERROR("SOIL failed to load image [{0}] -- {1}", faces[i], SOIL_last_result());
				return false;
			}

			GLint format = GL_RGBA;

			switch (channels)
			{
			case 3: format = GL_RGB; break;
			case 4: format = GL_RGBA; break;
			}

			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, image);

			SOIL_free_image_data(image);
		}

		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		return true;
	}

	bool TextureLoader::LoadTextureFromMemory(const unsigned char* imageData, size_t length, GLuint& id, int& width, int& height, bool blended)
	{
		id = SOIL_load_OGL_texture_from_memory(imageData, length, SOIL_LOAD_RGBA, SOIL_CREATE_NEW_ID, NULL);
		if (id == 0)
		{
			ENGINE_ERROR("Failed to load texture from memory!");
			return false;
		}

		glBindTexture(GL_TEXTURE_2D, id);
		glad_glGetTextureLevelParameteriv(id, 0, GL_TEXTURE_HEIGHT, &height);
		glad_glGetTextureLevelParameteriv(id, 0, GL_TEXTURE_WIDTH, &width);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		if (!blended)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		}
		else
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		}
		return true;
	}

    std::shared_ptr<Texture> TextureLoader::Create(Texture::TextureType type, const std::string& texturePath)
    {
		GLuint id;
		int width, height;

		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_2D, id);

		switch (type)
		{
		case Texture::TextureType::PIXEL:
			LoadTexture(texturePath, id, width, height, false);
			break;
		case Texture::TextureType::BLENDED:
			LoadTexture(texturePath, id, width, height, true);
			break;
		default:
			assert(false && "The current type is not defined, Please use a defined texture type!");
			return nullptr;
		}
		return std::make_shared<Texture>(id, width, height, type, texturePath);
    }

	std::shared_ptr<Texture> TextureLoader::Create(Texture::TextureType type, int width, int height, const bool multiSample)
	{
		GLuint id;
		glGenTextures(1, &id);
		switch (type)
		{
		case ENGINE_RENDERING::Texture::TextureType::FRAMEBUFFER:
			if (multiSample)
			{
				LoadFBTexture_multisample(id, width, height);
			}
			else {
				LoadFBTexture_normal(id, width, height);
			}
			break;
		case ENGINE_RENDERING::Texture::TextureType::SHADOWMAP:
			LoadShadowmapTexture(id, width, height);
			break;
		default:
			assert(false && "The current type is not defined, Please use a defined texture type!");
			return nullptr;
		}
		
		return std::make_shared<Texture>(id, width, height, type);
	}
	
	std::shared_ptr<Texture> TextureLoader::CreateSkybox(Texture::TextureType type, const std::string& texturePath)
	{
		GLuint id;
		int width, height;
		
		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_CUBE_MAP, id);

		switch (type)
		{
		case Texture::TextureType::PIXEL:
			LoadSkyboxTexture(texturePath, id, width, height, false);
			break;
		case Texture::TextureType::BLENDED:
			LoadSkyboxTexture(texturePath, id, width, height, true);
			break;
		default:
			assert(false && "The current type is not defined, Please use a defined texture type!");
			return nullptr;
		}
		return std::make_shared<Texture>(id, width, height, type, texturePath);
	}

	std::shared_ptr<Texture> TextureLoader::CreateFromMemory(const unsigned char* imageData, size_t length, bool blended, bool bTileset)
	{
		GLuint id;
		int width, height;
		if (LoadTextureFromMemory(imageData, length, id, width, height, blended))
		{
			return std::make_shared<Texture>(id, width, height, blended ? Texture::TextureType::BLENDED : Texture::TextureType::PIXEL, "");
		}
	}
}

