#pragma once
#include <glad/glad.h>
#include <vector>
#include <memory>
#include <map>
#include "../Essentials/Texture.h"

namespace ENGINE_RENDERING {
    enum class BufferType {
        FRAMEBUFFER,    // 普通场景渲染：1 Color + 1 Depth RBO
        SHADOWMAP,      // 方向光阴影：1 Depth Texture (2D)
        SHADOWCUBEMAP,  // 点光源阴影：1 Depth Texture (Cube)
        GBUFFER,        // 延迟渲染：Position, Normal, Albedo, Refl + Depth RBO
        SSAO,           // SSAO: 1 Color
        HDR
    };

    class Framebuffer {
    private:
        GLuint m_FboID = 0;
        GLuint m_RboID = 0;
        BufferType m_Type;
        int m_Width, m_Height;
        bool m_bUseRbo;
        bool m_bShouldResize = false;

        std::vector<std::shared_ptr<Texture>> m_pTextures;

    private:
        bool CreateTextures();
        bool Initialize();
        void CleanUp();

    public:
        Framebuffer(BufferType type, int width, int height, bool bUseRbo = true);
        ~Framebuffer();

        void Bind();
        void Unbind();
        void Resize(int width, int height);
        void CheckResize();
        
        inline const int Width() const { return m_Width; }
        inline const int Height() const { return m_Height; }
        inline GLuint GetFboID() const { return m_FboID; }
        inline BufferType GetType() const { return m_Type; }
        inline GLuint GetTextureID(size_t index = 0) const { return (index < m_pTextures.size()) ? m_pTextures[index]->GetID() : 0; }
    };
}