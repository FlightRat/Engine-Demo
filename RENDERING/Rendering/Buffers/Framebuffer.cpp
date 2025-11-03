#include "Framebuffer.h"
#include <Logger/Logger.h>
#include "../Essentials/TextureLoader.h"

namespace ENGINE_RENDERING {
	bool Framebuffer::Initialize()
	{
		// create framebuffer
		glGenFramebuffers(1, &m_FboID);
		glBindFramebuffer(GL_FRAMEBUFFER, m_FboID);

		// color attach 0
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_pTexture->GetID(), 0);

		// renderbuffer
		if (m_bUseRbo)
		{
			glGenRenderbuffers(1, &m_RboID);
			glBindRenderbuffer(GL_RENDERBUFFER, m_RboID);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, m_Width, m_Height);
			glBindRenderbuffer(GL_RENDERBUFFER, 0);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_RboID);
		}

		// check complete
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			assert(false && "Failed to create an OpenGL framebuffer!");

			std::string error = std::to_string(glGetError());
			ENGINE_ERROR("Failed to create an OpenGL framebuffer!");
			return false;
		}

		// unbind
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return true;
	}

	void Framebuffer::CleanUp()
	{
		glDeleteBuffers(1, &m_FboID);
		if (m_bUseRbo)
			glDeleteRenderbuffers(1, &m_RboID);
		if (m_pTexture)
		{
			auto textureID = m_pTexture->GetID();
			glDeleteTextures(1, &textureID);
		}
	}

	Framebuffer::Framebuffer():Framebuffer(600, 600, true)
	{
	}

	Framebuffer::Framebuffer(int width, int height, bool bUseRbo):
		m_FboID{ 0 }, m_RboID{0},m_pTexture{nullptr},
		m_Width{width},m_Height{height},
		m_bShouldResize{false},m_bUseRbo{ bUseRbo }
	{
		// create a empty texture for framebuffer
		m_pTexture = std::move(TextureLoader::Create(Texture::TextureType::FRAMEBUFFER, width, height));
		if (!m_pTexture || !Initialize())
		{
			assert(false && "Failed to create Framebuffer!");
			ENGINE_ERROR("Framebuffer creation failed!");
		}
	}

	Framebuffer::~Framebuffer()
	{
		CleanUp();
	}

	void Framebuffer::Bind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_FboID);
	}

	void Framebuffer::Unbind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void Framebuffer::Resize(int width, int height)
	{
		m_Width = width;
		m_Height = height;
		m_bShouldResize = true;
	}

	void Framebuffer::CheckResize()
	{
		if (!m_bShouldResize)
			return;

		CleanUp();
		m_pTexture.reset();
		m_pTexture = std::move(TextureLoader::Create(Texture::TextureType::FRAMEBUFFER, m_Width, m_Height));
		
		assert(m_pTexture && "New Texture cannot be nullptr!");

		Initialize();
		m_bShouldResize = false;
	}
}

