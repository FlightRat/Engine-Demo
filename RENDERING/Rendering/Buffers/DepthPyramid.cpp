#include "DepthPyramid.h"

namespace ENGINE_RENDERING {
    DepthPyramid::~DepthPyramid()
    {
        CleanUp();
    }

    void DepthPyramid::CleanUp()
    {
        glDeleteTextures(static_cast<GLsizei>(m_Views.size()), m_Views.data());
        m_Views.clear();
        glDeleteFramebuffers(1, &m_Framebuffer);
        glDeleteTextures(1, &m_Texture);
        m_Framebuffer = m_Texture = 0;
    }

    void DepthPyramid::Resize(int width, int height)
    {
        if (width == m_Width && height == m_Height) return;
        CleanUp();
        m_Width = width;
        m_Height = height;
        int levels = 1;
        for (int size = std::max(width, height); size > 1; size >>= 1) ++levels;
        glCreateTextures(GL_TEXTURE_2D, 1, &m_Texture);
        glTextureStorage2D(m_Texture, levels, GL_RG32F, width, height);
        glTextureParameteri(m_Texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glTextureParameteri(m_Texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(m_Texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_Texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        m_Views.resize(levels);
        glGenTextures(levels, m_Views.data());
        for (int level = 0; level < levels; ++level) {
            glTextureView(m_Views[level], GL_TEXTURE_2D, m_Texture, GL_RG32F, level, 1, 0, 1);
            glTextureParameteri(m_Views[level], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTextureParameteri(m_Views[level], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }
        glCreateFramebuffers(1, &m_Framebuffer);
    }

    void DepthPyramid::BindLevel(int level)
    {
        glNamedFramebufferTexture(m_Framebuffer, GL_COLOR_ATTACHMENT0, m_Texture, level);
        glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);
        glViewport(0, 0, Width(level), Height(level));
    }
}
