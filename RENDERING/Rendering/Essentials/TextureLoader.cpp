#include "TextureLoader.h"
#include <algorithm>
#include <random>
#include <filesystem>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cmath>

#include <glm/glm.hpp>
#include "Logger/Logger.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

namespace ENGINE_RENDERING
{
    // -----------------------------
    // 工具函数：根据通道数推导上传格式与内部格式
    // -----------------------------
    static bool DeduceTextureFormats(int channels, GLenum& internalFormat, GLenum& dataFormat)
    {
        switch (channels)
        {
        case 1:
            internalFormat = GL_R8;
            dataFormat = GL_RED;
            return true;
        case 2:
            internalFormat = GL_RG8;
            dataFormat = GL_RG;
            return true;
        case 3:
            internalFormat = GL_RGB8;
            dataFormat = GL_RGB;
            return true;
        case 4:
            internalFormat = GL_RGBA8;
            dataFormat = GL_RGBA;
            return true;
        default:
            return false;
        }
    }

    // -----------------------------
    // 普通图片纹理（从文件）
    // -----------------------------
    bool TextureLoader::LoadTexture(const std::string& filepath, GLuint& id, int& width, int& height, bool blended)
    {
        int channels = 0;

        // 使用 C++20 filesystem 安全处理 UTF-8 / 中文路径
        std::filesystem::path safePath = reinterpret_cast<const char8_t*>(filepath.c_str());

        std::ifstream file(safePath, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            ENGINE_ERROR("Failed to open file via filesystem [{0}]", filepath);
            return false;
        }

        const std::streamsize fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<unsigned char> fileBuffer(static_cast<size_t>(fileSize));
        if (!file.read(reinterpret_cast<char*>(fileBuffer.data()), fileSize))
        {
            ENGINE_ERROR("Failed to read file data to memory [{0}]", filepath);
            return false;
        }

        unsigned char* image = stbi_load_from_memory(
            fileBuffer.data(),
            static_cast<int>(fileBuffer.size()),
            &width,
            &height,
            &channels,
            0
        );

        if (!image)
        {
            ENGINE_ERROR("stb_image failed [{0}] -- {1}", filepath, stbi_failure_reason());
            return false;
        }

        GLenum internalFormat = GL_RGBA8;
        GLenum dataFormat = GL_RGBA;
        if (!DeduceTextureFormats(channels, internalFormat, dataFormat))
        {
            ENGINE_ERROR("Unsupported channel count [{0}] for texture [{1}]", channels, filepath);
            stbi_image_free(image);
            return false;
        }

        // DSA：创建纹理对象
        glCreateTextures(GL_TEXTURE_2D, 1, &id);

        // 纹理是否需要 mipmap
        const GLsizei mipLevels = blended
            ? static_cast<GLsizei>(std::floor(std::log2(std::max(width, height)))) + 1
            : 1;

        // DSA：分配不可变存储
        glTextureStorage2D(id, mipLevels, internalFormat, width, height);

        // DSA：上传数据
        glTextureSubImage2D(id, 0, 0, 0, width, height, dataFormat, GL_UNSIGNED_BYTE, image);

        // DSA：生成 mipmap
        if (blended)
        {
            glGenerateTextureMipmap(id);
        }

        // DSA：设置参数
        glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_REPEAT);

        if (!blended)
        {
            glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        }
        else
        {
            glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        }

        stbi_image_free(image);
        return true;
    }

    // -----------------------------
    // 从内存加载纹理
    // -----------------------------
    bool TextureLoader::LoadTextureFromMemory(const unsigned char* imageData, size_t length, GLuint& id, int& width, int& height, bool blended)
    {
        int channels = 0;
        unsigned char* image = stbi_load_from_memory(
            imageData,
            static_cast<int>(length),
            &width,
            &height,
            &channels,
            0
        );

        if (!image)
        {
            ENGINE_ERROR("stbi_image failed to load from memory -- {0}", stbi_failure_reason());
            return false;
        }

        GLenum internalFormat = GL_RGBA8;
        GLenum dataFormat = GL_RGBA;
        if (!DeduceTextureFormats(channels, internalFormat, dataFormat))
        {
            ENGINE_ERROR("Unsupported channel count from memory texture");
            stbi_image_free(image);
            return false;
        }

        glCreateTextures(GL_TEXTURE_2D, 1, &id);

        const GLsizei mipLevels = blended
            ? static_cast<GLsizei>(std::floor(std::log2(std::max(width, height)))) + 1
            : 1;

        glTextureStorage2D(id, mipLevels, internalFormat, width, height);
        glTextureSubImage2D(id, 0, 0, 0, width, height, dataFormat, GL_UNSIGNED_BYTE, image);

        if (blended)
        {
            glGenerateTextureMipmap(id);
        }

        glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        if (blended)
        {
            glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        }
        else
        {
            glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        }

        stbi_image_free(image);
        return true;
    }

    // -----------------------------
    // Framebuffer 单采样贴图
    // -----------------------------
    bool TextureLoader::LoadFBTexture_singlesample(GLuint& id, int& width, int& height)
    {
        glCreateTextures(GL_TEXTURE_2D, 1, &id);
        glTextureStorage2D(id, 1, GL_RGBA8, width, height);

        glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        return true;
    }

    // -----------------------------
    // Framebuffer 多采样贴图
    // -----------------------------
    bool TextureLoader::LoadFBTexture_multisample(GLuint& id, int& width, int& height)
    {
        glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, &id);
        glTextureStorage2DMultisample(id, 4, GL_RGBA8, width, height, GL_TRUE);
        return true;
    }

    // -----------------------------
    // Shadow Map
    // -----------------------------
    bool TextureLoader::LoadShadowmapTexture(GLuint& id, int& width, int& height)
    {
        glCreateTextures(GL_TEXTURE_2D, 1, &id);
        glTextureStorage2D(id, 1, GL_DEPTH_COMPONENT24, width, height);

        glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        const float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTextureParameterfv(id, GL_TEXTURE_BORDER_COLOR, borderColor);

        return true;
    }

    // -----------------------------
    // Shadow Cubemap
    // -----------------------------
    bool TextureLoader::LoadShadowCubemapTexture(GLuint& id, int& width, int& height)
    {
        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &id);
        glTextureStorage2D(id, 1, GL_DEPTH_COMPONENT24, width, height);

        glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(id, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        return true;
    }

    // -----------------------------
    // GBuffer
    // -----------------------------
    bool TextureLoader::LoadGbufferTexture(GLuint& id, int& width, int& height)
    {
        glCreateTextures(GL_TEXTURE_2D, 1, &id);
        glTextureStorage2D(id, 1, GL_RGBA16F, width, height);

        glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        return true;
    }

    // -----------------------------
    // SSAO
    // -----------------------------
    bool TextureLoader::LoadSSAOTexture(GLuint& id, int& width, int& height)
    {
        glCreateTextures(GL_TEXTURE_2D, 1, &id);
        glTextureStorage2D(id, 1, GL_R8, width, height);

        glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        return true;
    }

    // -----------------------------
    // 环境 Cubemap
    // -----------------------------
    bool TextureLoader::LoadEnvCubeMapTexture(GLuint& id, int& width, int& height)
    {
        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &id);
        glTextureStorage2D(id, 1, GL_RGB16F, width, height);

        glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(id, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        return true;
    }

    // -----------------------------
    // Irradiance Map
    // -----------------------------
    bool TextureLoader::LoadIrradianceMap(GLuint& id, int& width, int& height)
    {
        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &id);
        glTextureStorage2D(id, 1, GL_RGB16F, width, height);

        glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(id, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        return true;
    }

    // -----------------------------
    // Prefilter Map
    // -----------------------------
    bool TextureLoader::LoadFilterMap(GLuint& id, int& width, int& height)
    {
        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &id);

        const GLsizei mipLevels = static_cast<GLsizei>(std::floor(std::log2(std::max(width, height)))) + 1;
        glTextureStorage2D(id, mipLevels, GL_RGB16F, width, height);

        glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(id, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glGenerateTextureMipmap(id);
        return true;
    }

    // -----------------------------
    // BRDF LUT
    // -----------------------------
    bool TextureLoader::LoadBRDFLUT(GLuint& id, int& width, int& height)
    {
        glCreateTextures(GL_TEXTURE_2D, 1, &id);
        glTextureStorage2D(id, 1, GL_RG16F, width, height);

        glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        return true;
    }

    // -----------------------------
    // Skybox Cubemap
    // -----------------------------
    bool TextureLoader::LoadSkyboxTexture(const std::string filepath, GLuint& id, int& width, int& height, bool blended)
    {
        std::vector<std::string> suffixes = { "right", "left", "top", "bottom", "front", "back" };
        std::vector<std::string> faces(6, "");

        try
        {
            if (!std::filesystem::exists(filepath) || !std::filesystem::is_directory(filepath))
            {
                ENGINE_ERROR("Skybox path does not exist or is not a directory: {0}", filepath);
                return false;
            }

            for (const auto& entry : std::filesystem::directory_iterator(filepath))
            {
                std::string fileName = entry.path().filename().string();
                std::string fullPath = entry.path().string();
                std::string lowerName = fileName;
                std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

                for (int i = 0; i < 6; ++i)
                {
                    if (lowerName.find(suffixes[i]) != std::string::npos)
                    {
                        faces[i] = fullPath;
                        break;
                    }
                }
            }
        }
        catch (const std::exception& e)
        {
            ENGINE_ERROR("Filesystem error: {0}", e.what());
            return false;
        }

        for (int i = 0; i < 6; i++)
        {
            if (faces[i].empty())
            {
                ENGINE_ERROR("Skybox face [{0}] missing in directory: {1}", suffixes[i], filepath);
                return false;
            }
        }

        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &id);

        int nrComponents = 0;
        bool firstFace = true;

        for (unsigned int i = 0; i < faces.size(); i++)
        {
            unsigned char* image = stbi_load(faces[i].c_str(), &width, &height, &nrComponents, 0);
            if (!image)
            {
                ENGINE_ERROR("stbi_image failed to load [{0}] -- {1}", faces[i], stbi_failure_reason());
                return false;
            }

            GLenum internalFormat = (nrComponents == 3) ? GL_RGB8 : GL_RGBA8;
            GLenum dataFormat = (nrComponents == 3) ? GL_RGB : GL_RGBA;

            if (firstFace)
            {
                // 先按第一张图的尺寸分配存储
                glTextureStorage2D(id, 1, internalFormat, width, height);
                firstFace = false;
            }

            glTextureSubImage3D(
                id,
                0,
                0, 0, static_cast<GLint>(i),
                width, height, 1,
                dataFormat,
                GL_UNSIGNED_BYTE,
                image
            );

            stbi_image_free(image);
        }

        glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(id, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        return true;
    }

    // -----------------------------
    // HDR 纹理
    // -----------------------------
    bool TextureLoader::LoadHDRTexture(const std::string filepath, GLuint& id, int& width, int& height)
    {
        stbi_set_flip_vertically_on_load(true);
        int nrComponents = 0;

        float* image = stbi_loadf(filepath.c_str(), &width, &height, &nrComponents, 0);
        if (!image)
        {
            stbi_set_flip_vertically_on_load(false);
            ENGINE_ERROR("Failed to load HDR texture at path [{0}] -- [{1}]", filepath, stbi_failure_reason());
            return false;
        }

        glCreateTextures(GL_TEXTURE_2D, 1, &id);
        glTextureStorage2D(id, 1, GL_RGB16F, width, height);
        glTextureSubImage2D(id, 0, 0, 0, width, height, GL_RGB, GL_FLOAT, image);

        glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(image);
        stbi_set_flip_vertically_on_load(false);
        return true;
    }

    // -----------------------------
    // 工厂：从文件创建普通纹理
    // -----------------------------
    std::shared_ptr<Texture> TextureLoader::Create(Texture::TextureType type, const std::string& texturePath)
    {
        GLuint id = 0;
        int width = 0;
        int height = 0;

        switch (type)
        {
        case Texture::TextureType::PIXEL:
            if (!LoadTexture(texturePath, id, width, height, false)) return nullptr;
            break;
        case Texture::TextureType::BLENDED:
            if (!LoadTexture(texturePath, id, width, height, true)) return nullptr;
            break;
        default:
            assert(false && "The current type is not defined, Please use a defined texture type!");
            return nullptr;
        }

        return std::make_shared<Texture>(id, width, height, type, texturePath);
    }

    // -----------------------------
    // 工厂：从尺寸创建功能纹理
    // -----------------------------
    std::shared_ptr<Texture> TextureLoader::Create(Texture::TextureType type, int width, int height, const bool multiSample)
    {
        GLuint id = 0;

        switch (type)
        {
        case Texture::TextureType::FRAMEBUFFER:
            if (multiSample)
            {
                if (!LoadFBTexture_multisample(id, width, height)) return nullptr;
            }
            else
            {
                if (!LoadFBTexture_singlesample(id, width, height)) return nullptr;
            }
            break;

        case Texture::TextureType::SHADOWMAP:
            if (!LoadShadowmapTexture(id, width, height)) return nullptr;
            break;

        case Texture::TextureType::SHADOWCUBEMAP:
            if (!LoadShadowCubemapTexture(id, width, height)) return nullptr;
            break;

        case Texture::TextureType::GBUFFER:
            if (!LoadGbufferTexture(id, width, height)) return nullptr;
            break;

        case Texture::TextureType::SSAO:
            if (!LoadSSAOTexture(id, width, height)) return nullptr;
            break;

        case Texture::TextureType::ENVCUBEMAP:
            if (!LoadEnvCubeMapTexture(id, width, height)) return nullptr;
            break;

        case Texture::TextureType::IRRADIANCEMAP:
            if (!LoadIrradianceMap(id, width, height)) return nullptr;
            break;

        case Texture::TextureType::PREFILTERMAP:
            if (!LoadFilterMap(id, width, height)) return nullptr;
            break;

        case Texture::TextureType::BRDFLUT:
            if (!LoadBRDFLUT(id, width, height)) return nullptr;
            break;

        default:
            assert(false && "The current type is not defined, Please use a defined texture type!");
            return nullptr;
        }

        return std::make_shared<Texture>(id, width, height, type);
    }

    // -----------------------------
    // Skybox 工厂
    // -----------------------------
    std::shared_ptr<Texture> TextureLoader::CreateSkybox(Texture::TextureType type, const std::string& texturePath)
    {
        GLuint id = 0;
        int width = 0;
        int height = 0;

        switch (type)
        {
        case Texture::TextureType::PIXEL:
            if (!LoadSkyboxTexture(texturePath, id, width, height, false)) return nullptr;
            break;
        case Texture::TextureType::BLENDED:
            if (!LoadSkyboxTexture(texturePath, id, width, height, true)) return nullptr;
            break;
        default:
            assert(false && "The current type is not defined, Please use a defined texture type!");
            return nullptr;
        }

        return std::make_shared<Texture>(id, width, height, type, texturePath);
    }

    // -----------------------------
    // HDR 工厂
    // -----------------------------
    std::shared_ptr<Texture> TextureLoader::CreateHDR(const std::string& texturePath)
    {
        GLuint id = 0;
        int width = 0;
        int height = 0;

        if (!LoadHDRTexture(texturePath, id, width, height))
            return nullptr;

        return std::make_shared<Texture>(id, width, height, Texture::TextureType::NONE, texturePath);
    }

    // -----------------------------
    // SSAO Noise
    // -----------------------------
    std::shared_ptr<Texture> TextureLoader::CreateNoise()
    {
        std::uniform_real_distribution<GLfloat> randomFloats(0.0f, 1.0f);
        std::default_random_engine generator;

        std::vector<glm::vec3> ssaoNoise;
        ssaoNoise.reserve(16);

        for (unsigned int i = 0; i < 16; i++)
        {
            glm::vec3 noise(
                randomFloats(generator) * 2.0f - 1.0f,
                randomFloats(generator) * 2.0f - 1.0f,
                0.0f
            );
            ssaoNoise.push_back(noise);
        }

        GLuint id = 0;
        glCreateTextures(GL_TEXTURE_2D, 1, &id);

        // 这里更合理的是 RGB16F/RGB32F
        glTextureStorage2D(id, 1, GL_RGB16F, 4, 4);
        glTextureSubImage2D(id, 0, 0, 0, 4, 4, GL_RGB, GL_FLOAT, ssaoNoise.data());

        glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_REPEAT);

        return std::make_shared<Texture>(id, 4, 4, Texture::TextureType::PIXEL, "");
    }

    // -----------------------------
    // 从内存创建纹理
    // -----------------------------
    std::shared_ptr<Texture> TextureLoader::CreateFromMemory(const unsigned char* imageData, size_t length, bool blended, bool bTileset)
    {
        GLuint id = 0;
        int width = 0;
        int height = 0;

        if (LoadTextureFromMemory(imageData, length, id, width, height, blended))
        {
            return std::make_shared<Texture>(
                id,
                width,
                height,
                blended ? Texture::TextureType::BLENDED : Texture::TextureType::PIXEL,
                ""
            );
        }

        return nullptr;
    }

} // namespace ENGINE_RENDERING