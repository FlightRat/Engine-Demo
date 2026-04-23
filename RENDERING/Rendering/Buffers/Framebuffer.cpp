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
			// 严格遵循 GBuffer 的协议：0:Pos, 1:Norm, 2:Albedo, 3:metallic + roughness + ao
			m_pTextures.push_back(TextureLoader::Create(Texture::TextureType::GBUFFER, m_Width, m_Height, false));
			m_pTextures.push_back(TextureLoader::Create(Texture::TextureType::GBUFFER, m_Width, m_Height, false));
			m_pTextures.push_back(TextureLoader::Create(Texture::TextureType::GBUFFER, m_Width, m_Height, false));
			m_pTextures.push_back(TextureLoader::Create(Texture::TextureType::GBUFFER, m_Width, m_Height, false));
			break;
		case BufferType::SSAO:
			m_pTextures.push_back(TextureLoader::Create(Texture::TextureType::SSAO, m_Width, m_Height, false));
			break;
		case BufferType::IBL:
			m_pTextures.push_back(TextureLoader::Create(Texture::TextureType::ENVCUBEMAP, 512, 512));
			m_pTextures.push_back(TextureLoader::Create(Texture::TextureType::IRRADIANCEMAP, 32, 32));
			m_pTextures.push_back(TextureLoader::Create(Texture::TextureType::PREFILTERMAP, 128, 128));
			m_pTextures.push_back(TextureLoader::Create(Texture::TextureType::BRDFLUT, 512, 512));
			break;
		}
		return true;
	}

    bool Framebuffer::Initialize()
    {
        if (m_pTextures.empty() || !m_pTextures[0]) return false;

        // 1. 直接创建 FBO (对象内存分配)
        // 告别 glGenFramebuffers，直接完成内存注册
        glCreateFramebuffers(1, &m_FboID);

        switch (m_Type)
        {
        case ENGINE_RENDERING::BufferType::FRAMEBUFFER:
        {
            // 直接将纹理 ID 挂载到 FBO ID 上
            glNamedFramebufferTexture(m_FboID, GL_COLOR_ATTACHMENT0, m_pTextures[0]->GetID(), 0);

            // DSA 方式创建和配置 Renderbuffer (无需 bind)
            glCreateRenderbuffers(1, &m_RboID);
            glNamedRenderbufferStorage(m_RboID, GL_DEPTH24_STENCIL8, m_Width, m_Height);

            // 将 RBO 挂载到 FBO
            glNamedFramebufferRenderbuffer(m_FboID, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_RboID);
            break;
        }
        case ENGINE_RENDERING::BufferType::SHADOWMAP:
        {
            glNamedFramebufferTexture(m_FboID, GL_DEPTH_ATTACHMENT, m_pTextures[0]->GetID(), 0);
            // 显式告知 GPU：此 FBO 不进行颜色写入/读取 (仅深度)
            glNamedFramebufferReadBuffer(m_FboID, GL_NONE);
            glNamedFramebufferDrawBuffer(m_FboID, GL_NONE);
            break;
        }
        case ENGINE_RENDERING::BufferType::SHADOWCUBEMAP:
        {
            // DSA 中的 glNamedFramebufferTexture 同样支持 Cubemap/Layered 附着
            glNamedFramebufferTexture(m_FboID, GL_DEPTH_ATTACHMENT, m_pTextures[0]->GetID(), 0);
            glNamedFramebufferReadBuffer(m_FboID, GL_NONE);
            glNamedFramebufferDrawBuffer(m_FboID, GL_NONE);
            break;
        }
        case ENGINE_RENDERING::BufferType::GBUFFER:
        {
            // 组装 G-Buffer 的高光/法线/反照率等多颜色附件
            glNamedFramebufferTexture(m_FboID, GL_COLOR_ATTACHMENT0, m_pTextures[0]->GetID(), 0);
            glNamedFramebufferTexture(m_FboID, GL_COLOR_ATTACHMENT1, m_pTextures[1]->GetID(), 0);
            glNamedFramebufferTexture(m_FboID, GL_COLOR_ATTACHMENT2, m_pTextures[2]->GetID(), 0);
            glNamedFramebufferTexture(m_FboID, GL_COLOR_ATTACHMENT3, m_pTextures[3]->GetID(), 0);

            // 声明 MRT (Multi-Render Targets) 布局
            unsigned int attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
            glNamedFramebufferDrawBuffers(m_FboID, 4, attachments);

            // G-Buffer 同样需要深度模板缓冲进行几何剔除等操作
            glCreateRenderbuffers(1, &m_RboID);
            glNamedRenderbufferStorage(m_RboID, GL_DEPTH24_STENCIL8, m_Width, m_Height);
            glNamedFramebufferRenderbuffer(m_FboID, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_RboID);
            break;
        }
        case ENGINE_RENDERING::BufferType::SSAO:
        {
            glNamedFramebufferTexture(m_FboID, GL_COLOR_ATTACHMENT0, m_pTextures[0]->GetID(), 0);
            break;
        }
        case ENGINE_RENDERING::BufferType::IBL:
        {
            // IBL (通常用于捕获环境光积分或卷积，仅需深度)
            glCreateRenderbuffers(1, &m_RboID);
            glNamedRenderbufferStorage(m_RboID, GL_DEPTH_COMPONENT24, m_Width, m_Height);
            glNamedFramebufferRenderbuffer(m_FboID, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_RboID);
            break;
        }
        default:
            break;
        }

        // 2. 检查 FBO 状态，同样无需 Bind
        if (glCheckNamedFramebufferStatus(m_FboID, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            // 记录错误日志
            std::string error = std::to_string(glGetError());
            ENGINE_ERROR("Failed to create an OpenGL framebuffer! Error Code: {0}", error);
            // 清理失败的资源，避免内存泄漏
            glDeleteFramebuffers(1, &m_FboID);
            m_FboID = 0;
            return false;
        }

        // 注意：不再需要 glBindFramebuffer(GL_FRAMEBUFFER, 0)
        // 因为这期间我们从未在全局状态中绑定过任何东西！

        return true;
    }

	void Framebuffer::Bind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_FboID);
		if (m_bUseRbo)
			glBindRenderbuffer(GL_RENDERBUFFER, m_RboID);
	}

	void Framebuffer::Unbind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		if (m_bUseRbo)
			glBindRenderbuffer(GL_RENDERBUFFER, 0);
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