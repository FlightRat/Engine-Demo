#include "Framebuffer.h"
#include "Logger/Logger.h"
#include "../Essentials/TextureLoader.h"

namespace ENGINE_RENDERING {
	Framebuffer::Framebuffer(BufferType type, int width, int height, bool bUseRbo)
		: m_Type(type), m_Width(width), m_Height(height), m_bUseRbo(bUseRbo) {

		if (!CreateTextures() || !Initialize())
			ENGINE_ERROR("Failed to initialize Framebuffer of type: {0}", (int)type);
	}

	Framebuffer::~Framebuffer()
	{
		CleanUp();
	}

	void Framebuffer::CleanUp() {
		glDeleteFramebuffers(1, &m_FboID);
		m_FboID = 0;
		if (m_bUseRbo) {
			glDeleteRenderbuffers(1, &m_RboID);
			m_RboID = 0;
		}
		m_pTextures.clear();
	}

	bool Framebuffer::CreateTextures() {
		m_pTextures.clear();

		switch (m_Type) {
		case BufferType::FRAMEBUFFER:
			m_pTextures.push_back(TextureLoader::Create(Texture::TextureType::FRAMEBUFFER, m_Width, m_Height, false));
			break;

		case BufferType::SHADOWMAP:
			m_pTextures.push_back(TextureLoader::Create(Texture::TextureType::SHADOWMAP, m_Width, m_Height));
			break;

		case BufferType::SHADOWCUBEMAP:
			m_pTextures.push_back(TextureLoader::Create(Texture::TextureType::SHADOWCUBEMAP, m_Width, m_Height));
			break;

		case BufferType::GBUFFER:
			// 严格遵循 GBuffer 的协议：0:Pos, 1:Norm, 2:Albedo, 3:Refl
			m_pTextures.push_back(TextureLoader::Create(Texture::TextureType::GBUFFER_TYPE1, m_Width, m_Height, false));
			m_pTextures.push_back(TextureLoader::Create(Texture::TextureType::GBUFFER_TYPE1, m_Width, m_Height, false));
			m_pTextures.push_back(TextureLoader::Create(Texture::TextureType::GBUFFER_TYPE2, m_Width, m_Height, false));
			m_pTextures.push_back(TextureLoader::Create(Texture::TextureType::GBUFFER_TYPE2, m_Width, m_Height, false));
			break;
		case BufferType::SSAO:
			m_pTextures.push_back(TextureLoader::Create(Texture::TextureType::SSAO, m_Width, m_Height, false));
			break;
		}
		return true;
	}

	bool Framebuffer::Initialize()
	{
		if (m_pTextures.empty() || !m_pTextures[0]) return false;

		// create framebuffer
		glGenFramebuffers(1, &m_FboID);
		glBindFramebuffer(GL_FRAMEBUFFER, m_FboID);

		switch (m_Type)
		{
		case ENGINE_RENDERING::BufferType::FRAMEBUFFER:
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_pTextures[0]->GetID(), 0);
			glGenRenderbuffers(1, &m_RboID);
			glBindRenderbuffer(GL_RENDERBUFFER, m_RboID);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_Width, m_Height);
			glBindRenderbuffer(GL_RENDERBUFFER, 0);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_RboID);
			break;
		}
		case ENGINE_RENDERING::BufferType::SHADOWMAP:
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_pTextures[0]->GetID(), 0);
			glReadBuffer(GL_NONE);
			glDrawBuffer(GL_NONE);
			break;
		}
		case ENGINE_RENDERING::BufferType::SHADOWCUBEMAP:
		{
			glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_pTextures[0]->GetID(), 0);
			glReadBuffer(GL_NONE);
			glDrawBuffer(GL_NONE);
			break;
		}
		case ENGINE_RENDERING::BufferType::GBUFFER:
		{
			// color attachments
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_pTextures[0]->GetID(), 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_pTextures[1]->GetID(), 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, m_pTextures[2]->GetID(), 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, m_pTextures[3]->GetID(), 0);

			// attach：attachments仅在本作用域内有效
			unsigned int attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
			glDrawBuffers(4, attachments);

			// RBO
			glGenRenderbuffers(1, &m_RboID);
			glBindRenderbuffer(GL_RENDERBUFFER, m_RboID);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_Width, m_Height);
			glBindRenderbuffer(GL_RENDERBUFFER, 0);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_RboID);
			break;
		}
		case ENGINE_RENDERING::BufferType::SSAO:
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_pTextures[0]->GetID(), 0);
			break;
		}
		default:
			break;
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

	void Framebuffer::CheckResize() {
		if (!m_bShouldResize) return;

		// 关键：阴影贴图通常不随窗口缩放，可以在这里加个过滤
		if (m_Type == BufferType::SHADOWMAP || m_Type == BufferType::SHADOWCUBEMAP) {
			m_bShouldResize = false;
			return;
		}

		CleanUp();        // 销毁旧的 FBO/RBO/Textures
		CreateTextures(); // 重新按 m_Type 创建正确格式的纹理
		Initialize();     // 重新绑定

		m_bShouldResize = false;
	}
}