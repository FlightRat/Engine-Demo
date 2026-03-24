#pragma once
#include <glad/glad.h>
#include <memory>

#include "../Essentials/Texture.h"

namespace ENGINE_RENDERING {
	class Framebuffer
	{
	private:
		GLuint m_RboID, m_FboID;
		std::shared_ptr<Texture> m_pTexture;
		int m_Width, m_Height;
		bool m_bShouldResize, m_bUseRbo;

	private:
		bool Initialize();
		void CleanUp();
	public:
		Framebuffer();
		Framebuffer(int width, int height, bool bUseRbo);
		~Framebuffer();

		void Bind();
		void Unbind();

		void Resize(int width, int height);
		void CheckResize();

		inline const GLuint GetID() const { return m_FboID; }
		inline const GLuint GetTextureID() const { return m_pTexture ? m_pTexture->GetID() : 0; }
		inline const int Width() const { return m_Width; }
		inline const int Height() const { return m_Height; }
	};
}