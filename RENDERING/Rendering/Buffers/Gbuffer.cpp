#include "Gbuffer.h"
#include "Logger/Logger.h"
#include "../Essentials/TextureLoader.h"

namespace ENGINE_RENDERING {
	Gbuffer::Gbuffer():Gbuffer(600, 600)
	{
	}

	Gbuffer::Gbuffer(int width, int height):
		m_FboID{ 0 }, m_RboID{ 0 }, 
		m_PositionTex{ nullptr }, m_NormalTex{ nullptr }, m_AlbedoSpecTex{ nullptr }, m_ReflTex{nullptr},
		m_Width{ width }, m_Height{ height },
		m_bShouldResize{ false }
	{
		// create textures
		m_PositionTex = std::move(TextureLoader::Create(Texture::TextureType::GBUFFER_TYPE1, width, height, false));
		m_NormalTex = std::move(TextureLoader::Create(Texture::TextureType::GBUFFER_TYPE1, width, height, false));
		m_AlbedoSpecTex = std::move(TextureLoader::Create(Texture::TextureType::GBUFFER_TYPE2, width, height, false));
		m_ReflTex = std::move(TextureLoader::Create(Texture::TextureType::GBUFFER_TYPE2, width, height, false));
		if (!m_PositionTex || !m_NormalTex || !m_AlbedoSpecTex || !m_ReflTex || !Initialize())
		{
			assert(false && "Failed to create Framebuffer!");
			ENGINE_ERROR("Framebuffer creation failed!");
		}
	}

	Gbuffer::~Gbuffer()
	{
		CleanUp();
	}

	bool Gbuffer::Initialize()
	{
		glGenFramebuffers(1, &m_FboID);
		glBindFramebuffer(GL_FRAMEBUFFER, m_FboID);
		// color attachments
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_PositionTex->GetID(), 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_NormalTex->GetID(), 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, m_AlbedoSpecTex->GetID(), 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, m_ReflTex->GetID(), 0);
		// attach
		unsigned int attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
		glDrawBuffers(4, attachments);
		// RBO
		glGenRenderbuffers(1, &m_RboID);
		glBindRenderbuffer(GL_RENDERBUFFER, m_RboID);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_Width, m_Height);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_RboID);
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

	void Gbuffer::CleanUp()
	{
		glDeleteFramebuffers(1, &m_FboID);
		glDeleteRenderbuffers(1, &m_RboID);
		m_FboID = 0;
		m_RboID = 0;

		auto textureID = m_PositionTex->GetID();
		glDeleteTextures(1, &textureID);

		textureID = m_AlbedoSpecTex->GetID();
		glDeleteTextures(1, &textureID);

		textureID = m_NormalTex->GetID();
		glDeleteTextures(1, &textureID);

		textureID = m_ReflTex->GetID();
		glDeleteTextures(1, &textureID);
	}

	void Gbuffer::Bind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_FboID);
	}

	void Gbuffer::Unbind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void Gbuffer::Resize(int width, int height)
	{
		m_Width = width;
		m_Height = height;
		m_bShouldResize = true;
	}

	void Gbuffer::CheckResize()
	{
		if (!m_bShouldResize)
			return;

		CleanUp();

		m_PositionTex.reset();
		m_PositionTex = std::move(TextureLoader::Create(Texture::TextureType::GBUFFER_TYPE1, m_Width, m_Height, false));
		assert(m_PositionTex && "New Texture cannot be nullptr!");

		m_NormalTex.reset();
		m_NormalTex = std::move(TextureLoader::Create(Texture::TextureType::GBUFFER_TYPE1, m_Width, m_Height, false));
		assert(m_NormalTex && "New Texture cannot be nullptr!");

		m_AlbedoSpecTex.reset();
		m_AlbedoSpecTex = std::move(TextureLoader::Create(Texture::TextureType::GBUFFER_TYPE2, m_Width, m_Height, false));
		assert(m_AlbedoSpecTex && "New Texture cannot be nullptr!");

		m_ReflTex.reset();
		m_ReflTex = std::move(TextureLoader::Create(Texture::TextureType::GBUFFER_TYPE2, m_Width, m_Height, false));
		assert(m_ReflTex && "New Texture cannot be nullptr!");

		Initialize();
		m_bShouldResize = false;
	}
}

