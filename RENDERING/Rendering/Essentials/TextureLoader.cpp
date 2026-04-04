#include "TextureLoader.h"
#include <random>
#include <filesystem>
#include <fstream>
#include <vector>
#include <glm/glm.hpp>
#include "Logger/Logger.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace ENGINE_RENDERING{
	bool TextureLoader::LoadTexture(const std::string& filepath, GLuint& id, int& width, int& height, bool blended)
	{
		int channels = 0;

		// 1. [核心架构转换] 使用 C++20 filesystem 安全处理 UTF-8 中文路径
		std::filesystem::path safePath = reinterpret_cast<const char8_t*>(filepath.c_str());

		// 2. [底层 I/O] 以二进制模式并在文件尾部打开，为了快速获取文件大小
		std::ifstream file(safePath, std::ios::binary | std::ios::ate);
		if (!file.is_open())
		{
			ENGINE_ERROR("Failed to open file via filesystem [{0}]", filepath);
			return false;
		}

		// 3. [内存管理] 一次性分配整块内存并完整读取，极致的 Cache-friendly 做法
		std::streamsize fileSize = file.tellg();
		file.seekg(0, std::ios::beg); // 光标移回文件头

		std::vector<unsigned char> fileBuffer(static_cast<size_t>(fileSize));
		if (!file.read(reinterpret_cast<char*>(fileBuffer.data()), fileSize))
		{
			ENGINE_ERROR("Failed to read file data to memory [{0}]", filepath);
			return false;
		}
		file.close(); // 尽早释放句柄

		// 4. [图像解码] 放弃有缺陷的底层 fopen，强制要求 SOIL 从我们构造的安全内存中解码
		unsigned char* image = stbi_load_from_memory(
			fileBuffer.data(),
			static_cast<int>(fileBuffer.size()),
			&width,
			&height,
			&channels,
			0  // 0 = 保持原始通道数
		);

		// Check to see if the image is successful
		if (!image) 
		{
			ENGINE_ERROR("stb_image failed [{0}] -- {1}", filepath, stbi_failure_reason());
			return false;
		}

		GLint format = GL_RGBA;

		switch (channels)
		{
		case 1: format = GL_RED; break;   // 单通道灰度图（metallic/roughness）
		case 2: format = GL_RG; break;    // 双通道（如 metallic+roughness 打包）
		case 3: format = GL_RGB; break;
		case 4: format = GL_RGBA; break;
		default:
			ENGINE_ERROR("Unsupported channel count [{0}] for texture [{1}]", channels, filepath);
			stbi_image_free(image);
			return false;
		}

		glTexImage2D(
			GL_TEXTURE_2D,   // target      -- Specifies the target texture
			0,               // level       -- Level of detail
			format,          // internal format
			width,           // width
			height,          // height
			0,               // border
			format,          // format
			GL_UNSIGNED_BYTE,// type
			image            // data
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
		stbi_image_free(image);

		return true;
	}

	bool TextureLoader::LoadTextureFromMemory(const unsigned char* imageData, size_t length, GLuint& id, int& width, int& height, bool blended)
	{
		int channels;
		unsigned char* image = stbi_load_from_memory(imageData, static_cast<int>(length), &width, &height, &channels, 0);

		if (!image)
		{
			ENGINE_ERROR("stbi_image failed to load from memory -- {0}", stbi_failure_reason());
			return false;
		}

		GLint format = (channels == 3) ? GL_RGB : GL_RGBA;

		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_2D, id);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, image);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		if (blended)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}
		else
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		}

		stbi_image_free(image);
		return true;
	}

	bool TextureLoader::LoadFBTexture_singlesample(GLuint& id, int& width, int& height)
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
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
		glBindTexture(GL_TEXTURE_2D, 0);
		return true;
	}

	bool TextureLoader::LoadShadowCubemapTexture(GLuint& id, int& width, int& height)
	{
		glBindTexture(GL_TEXTURE_CUBE_MAP, id);
		for (unsigned int i = 0; i < 6; ++i)
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
		return true;
	}

	bool TextureLoader::LoadGbufferTexture(GLuint& id, int& width, int& height)
	{
		glBindTexture(GL_TEXTURE_2D, id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_2D, 0);
		return true;
	}

	bool TextureLoader::LoadSSAOTexture(GLuint& id, int& width, int& height)
	{
		glBindTexture(GL_TEXTURE_2D, id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);
		return true;
	}

	bool TextureLoader::LoadEnvCubeMapTexture(GLuint& id, int& width, int& height)
	{
		glBindTexture(GL_TEXTURE_CUBE_MAP, id);
		for (unsigned int i = 0; i < 6; ++i)
		{
			// note that we store each face with 16 bit floating point values
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // enable pre-filter mipmap sampling (combatting visible dots artifact)
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
		return true;
	}

	bool TextureLoader::LoadIrradianceMap(GLuint& id, int& width, int& height)
	{
		glBindTexture(GL_TEXTURE_CUBE_MAP, id);
		for (unsigned int i = 0; i < 6; ++i)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
		return true;
	}

	bool TextureLoader::LoadFilterMap(GLuint& id, int& width, int& height)
	{
		glBindTexture(GL_TEXTURE_CUBE_MAP, id);
		for (unsigned int i = 0; i < 6; i++)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // be sure to set minification filter to mip_linear 
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// generate mipmaps for the cubemap so OpenGL automatically allocates the required memory.
		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
		return true;
	}

	bool TextureLoader::LoadBRDFLUT(GLuint& id, int& width, int& height)
	{
		// pre-allocate enough memory for the LUT texture.
		glBindTexture(GL_TEXTURE_2D, id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, width, height, 0, GL_RG, GL_FLOAT, 0);
		// be sure to set wrapping mode to GL_CLAMP_TO_EDGE
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0);
		return true;
	}

	bool TextureLoader::LoadSkyboxTexture(const std::string filepath, GLuint& id, int& width, int& height, bool blended)
	{
		std::vector<std::string> suffixes = { "right", "left", "top", "bottom", "front", "back" };
		std::vector<std::string> faces(6, "");

		try {
			if (!std::filesystem::exists(filepath) || !std::filesystem::is_directory(filepath)) {
				ENGINE_ERROR("Skybox path does not exist or is not a directory: {0}", filepath);
				return false;
			}

			for (const auto& entry : std::filesystem::directory_iterator(filepath)) {
				std::string fileName = entry.path().filename().string();
				std::string fullPath = entry.path().string();
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

		for (int i = 0; i < 6; i++) {
			if (faces[i].empty()) {
				ENGINE_ERROR("Skybox face [{0}] missing in directory: {1}", suffixes[i], filepath);
				return false;
			}
		}

		int nrComponents = 0; //  移到循环外声明
		for (unsigned int i = 0; i < faces.size(); i++)
		{
			unsigned char* image = stbi_load(faces[i].c_str(), &width, &height, &nrComponents, 0);

			if (!image)
			{
				ENGINE_ERROR("stbi_image failed to load [{0}] -- {1}", faces[i], stbi_failure_reason()); // 改为 stbi_failure_reason
				return false;
			}

			GLint format = (nrComponents == 3) ? GL_RGB : GL_RGBA; // 改为 nrComponents

			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, image);

			stbi_image_free(image);
		}

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		return true;
	}

	bool TextureLoader::LoadHDRTexture(const std::string filepath, GLuint& id, int& width, int& height)
	{
		stbi_set_flip_vertically_on_load(true);
		int nrComponents = 0;

		float *image = stbi_loadf(filepath.c_str(), &width, &height, &nrComponents, 0);
		if (!image)
		{
			stbi_set_flip_vertically_on_load(false);
			ENGINE_ERROR("Failed to load HDR texture at path [{0}] -- [{1}]", filepath, stbi_failure_reason());
			return false;
		}
		
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, image);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		
		stbi_image_free(image);
		stbi_set_flip_vertically_on_load(false);
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
				LoadFBTexture_singlesample(id, width, height);
			}
			break;
		case ENGINE_RENDERING::Texture::TextureType::SHADOWMAP:
			LoadShadowmapTexture(id, width, height);
			break;
		case ENGINE_RENDERING::Texture::TextureType::SHADOWCUBEMAP:
			LoadShadowCubemapTexture(id, width, height);
			break;
		case ENGINE_RENDERING::Texture::TextureType::GBUFFER:
			LoadGbufferTexture(id, width, height);
			break;
		case ENGINE_RENDERING::Texture::TextureType::SSAO:
			LoadSSAOTexture(id, width, height);
			break;
		case ENGINE_RENDERING::Texture::TextureType::ENVCUBEMAP:
			LoadEnvCubeMapTexture(id, width, height);
			break;
		case ENGINE_RENDERING::Texture::TextureType::IRRADIANCEMAP:
			LoadIrradianceMap(id, width, height);
			break;
		case ENGINE_RENDERING::Texture::TextureType::PREFILTERMAP:
			LoadFilterMap(id, width, height);
			break;
		case ENGINE_RENDERING::Texture::TextureType::BRDFLUT:
			LoadBRDFLUT(id, width, height);
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

	std::shared_ptr<Texture> TextureLoader::CreateHDR(const std::string& texturePath)
	{
		GLuint id;
		int width, height;

		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_2D, id);
		LoadHDRTexture(texturePath, id, width, height);
		return std::make_shared<Texture>(id, width, height, Texture::TextureType::NONE, texturePath);
	}

	std::shared_ptr<Texture> TextureLoader::CreateNoise()
	{
		std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
		std::default_random_engine generator;
		std::vector<glm::vec3> ssaoNoise;
		for (unsigned int i = 0; i < 16; i++)
		{
			glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f); // rotate around z-axis (in tangent space)
			ssaoNoise.push_back(noise);
		}

		GLuint id;
		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_2D, id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		return std::make_shared<Texture>(id, 0, 0, Texture::TextureType::PIXEL, "");
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

