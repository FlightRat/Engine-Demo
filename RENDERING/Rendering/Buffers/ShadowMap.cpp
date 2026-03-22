#include "ShadowMap.h"
#include <Logger/Logger.h>
#include "../Essentials/TextureLoader.h"

namespace ENGINE_RENDERING {
	ShadowMap::ShadowMap():ShadowMap(600, 600)
	{
	}

	ShadowMap::ShadowMap(int width, int height, bool bCube):
		m_FboID{0}, m_pDepthTexture{nullptr}, m_Width{width}, m_Height{height}, m_bCube{bCube}, m_bShouldResize{false}, m_lightSpaceMatrix{glm::mat4{1.0f}}
	{
		if (bCube)
		{
			m_pDepthTexture = std::move(TextureLoader::Create(Texture::TextureType::SHADOWCUBEMAP, width, height));
		}
		else
		{
			m_pDepthTexture = std::move(TextureLoader::Create(Texture::TextureType::SHADOWMAP, width, height));
		}
		if (!m_pDepthTexture || !Initialize())
		{
			assert(false && "Failed to create Shadowmap");
			ENGINE_ERROR("Shadowmap creaton failed!");
		}
	}

	ShadowMap::~ShadowMap()
	{
		CleanUp();
	}

	bool ShadowMap::Initialize()
	{
		glGenFramebuffers(1, &m_FboID);
		glBindFramebuffer(GL_FRAMEBUFFER, m_FboID);
		if (m_bCube)
		{
			// [修复] 对于立方体贴图 (Cubemap)，必须使用 glFramebufferTexture (无 2D 后缀)
			// 这样才能将 6 个面作为一个整体挂载，允许几何着色器通过 gl_Layer 进行多层渲染
			glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_pDepthTexture->GetID(), 0);
		}
		else
		{
			// 对于普通的 2D 阴影贴图 (方向光)
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_pDepthTexture->GetID(), 0);
		}
		glReadBuffer(GL_NONE);
		glDrawBuffer(GL_NONE);

		// 检查完整性
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			// 打印出是 Cube 还是 2D 出了错，以及对应的 16 进制错误码
			ENGINE_ERROR("Shadow Framebuffer ({0}) is not complete! Status Code: 0x{1:x}", m_bCube ? "Cube" : "2D", status);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			return false;
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return true;
	}

	void ShadowMap::Bind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_FboID);
	}

	void ShadowMap::Unbind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void ShadowMap::Resize(int width, int height)
	{
		m_Width = width;
		m_Height = height;
		m_bShouldResize = true;
	}

	void ShadowMap::CheckResize()
	{
		if (!m_bShouldResize)
			return;

		CleanUp();

		m_pDepthTexture.reset();
		if(m_bCube)
			m_pDepthTexture = std::move(TextureLoader::Create(Texture::TextureType::SHADOWCUBEMAP, m_Width, m_Height));
		else
			m_pDepthTexture = std::move(TextureLoader::Create(Texture::TextureType::SHADOWMAP, m_Width, m_Height));
		assert(m_pDepthTexture && "New Texture cannot be nullptr!");

		Initialize();
		m_bShouldResize = false;
	}

	void ShadowMap::CleanUp()
	{
		glDeleteFramebuffers(1, &m_FboID);
		if (m_pDepthTexture)
		{
			auto textureID = m_pDepthTexture->GetID();
			glDeleteTextures(1, &textureID);
		}
	}

}