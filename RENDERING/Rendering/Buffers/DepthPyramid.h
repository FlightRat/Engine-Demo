#pragma once
#include <glad/glad.h>
#include <vector>
#include <algorithm>

namespace ENGINE_RENDERING {
    class DepthPyramid
    {
    private:
        GLuint m_Texture = 0, m_Framebuffer = 0;
        int m_Width = 0, m_Height = 0;
        std::vector<GLuint> m_Views;
        void CleanUp();
    public:
        DepthPyramid() = default;
        DepthPyramid(const DepthPyramid&) = delete;
        DepthPyramid& operator=(const DepthPyramid&) = delete;
        ~DepthPyramid();
        void Resize(int width, int height);
        void BindLevel(int level);
        inline GLuint TextureID() const { return m_Texture; }
        inline GLuint ViewID(int level) const { return m_Views[level]; }
        inline GLuint FramebufferID() const { return m_Framebuffer; }
        inline int Levels() const { return static_cast<int>(m_Views.size()); }
        inline int Width(int level = 0) const { return std::max(1, m_Width >> level); }
        inline int Height(int level = 0) const { return std::max(1, m_Height >> level); }
    };
}
