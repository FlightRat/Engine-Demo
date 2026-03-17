#include "ShadowMap.h"
#include <Logger/Logger.h>
#include "../Essentials/TextureLoader.h"

namespace ENGINE_RENDERING {
	ShadowMap::ShadowMap():ShadowMap(600, 600)
	{
	}

	ShadowMap::ShadowMap(int width, int height):
		m_FboID{0}, m_pDepthTexture{nullptr}, m_Width{width}, m_Height{height}, m_bShouldResize{false}, m_lightSpaceMatrix{glm::mat4{1.0f}}
	{
		m_pDepthTexture = std::move(TextureLoader::Create(Texture::TextureType::SHADOWMAP, width, height));
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
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_pDepthTexture->GetID(), 0);
		glReadBuffer(GL_NONE);
		glDrawBuffer(GL_NONE);
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