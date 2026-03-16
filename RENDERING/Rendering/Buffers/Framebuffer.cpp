#include "Framebuffer.h"
#include <Logger/Logger.h>
#include "../Essentials/TextureLoader.h"

namespace ENGINE_RENDERING {
	bool Framebuffer::Initialize()
	{
		// create normal framebuffer
		glGenFramebuffers(1, &m_ResolvedFboID);
		glBindFramebuffer(GL_FRAMEBUFFER, m_ResolvedFboID);
		// color attach 0
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_pResolvedTexture->GetID(), 0);
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

		// create multisample framebuffer (render happens on it)
		glGenFramebuffers(1, &m_MultisampleFboID);
		glBindFramebuffer(GL_FRAMEBUFFER, m_MultisampleFboID);
		// color attach 0
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, m_pMultisampleTexture->GetID(), 0);
		// renderbuffer
		if (m_bUseRbo)
		{
			glGenRenderbuffers(1, &m_MultisampleRboID);
			glBindRenderbuffer(GL_RENDERBUFFER, m_MultisampleRboID);
			glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, m_Width, m_Height);
			glBindRenderbuffer(GL_RENDERBUFFER, 0);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_MultisampleRboID);
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
		glDeleteFramebuffers(1, &m_ResolvedFboID);
		glDeleteFramebuffers(1, &m_MultisampleFboID);
		if (m_bUseRbo) {
			glDeleteRenderbuffers(1, &m_MultisampleRboID);
			m_MultisampleRboID = 0;
		}
		if (m_pMultisampleTexture)
		{
			auto textureID = m_pMultisampleTexture->GetID();
			glDeleteTextures(1, &textureID);
		}
		if (m_pResolvedTexture)
		{
			auto textureID = m_pResolvedTexture->GetID();
			glDeleteTextures(1, &textureID);
		}
	}

	Framebuffer::Framebuffer():Framebuffer(600, 600, true)
	{
	}

	Framebuffer::Framebuffer(int width, int height, bool bUseRbo):
		m_MultisampleFboID{ 0 }, m_MultisampleRboID{0}, m_ResolvedFboID{0},
		m_pMultisampleTexture{nullptr}, m_pResolvedTexture{nullptr},
		m_Width{width}, m_Height{height},
		m_bShouldResize{false}, m_bUseRbo{ bUseRbo }
	{
		// create a empty texture for framebuffer
		m_pMultisampleTexture = std::move(TextureLoader::Create(Texture::TextureType::FRAMEBUFFER, width, height, true));
		m_pResolvedTexture = std::move(TextureLoader::Create(Texture::TextureType::FRAMEBUFFER, width, height, false));
		if (!m_pMultisampleTexture || !m_pResolvedTexture || !Initialize())
		{
			assert(false && "Failed to create Framebuffer!");
			ENGINE_ERROR("Framebuffer creation failed!");
		}
	}

	Framebuffer::~Framebuffer()
	{
		CleanUp();
	}

	void Framebuffer::Resolve()
	{
		glBindFramebuffer(GL_READ_FRAMEBUFFER, m_MultisampleFboID);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_ResolvedFboID);
		glBlitFramebuffer(0, 0, m_Width, m_Height, 0, 0, m_Width, m_Height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

		// TODO: blit m_MultisampleRboID too, for ssao
	}

	void Framebuffer::Bind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_MultisampleFboID);
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

		m_pMultisampleTexture.reset();
		m_pMultisampleTexture = std::move(TextureLoader::Create(Texture::TextureType::FRAMEBUFFER, m_Width, m_Height, true));
		assert(m_pMultisampleTexture && "New Texture cannot be nullptr!");

		m_pResolvedTexture.reset();
		m_pResolvedTexture = std::move(TextureLoader::Create(Texture::TextureType::FRAMEBUFFER, m_Width, m_Height, false));
		assert(m_pResolvedTexture && "New Texture cannot be nullptr!");
		
		Initialize();
		m_bShouldResize = false;
	}
}

