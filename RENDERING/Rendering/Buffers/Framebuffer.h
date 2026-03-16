#pragma once
#include <glad/glad.h>
#include <memory>

#include "../Essentials/Texture.h"

namespace ENGINE_RENDERING {
	class Framebuffer
	{
	private:
		GLuint m_MultisampleFboID, m_MultisampleRboID, m_ResolvedFboID;
		std::shared_ptr<Texture> m_pMultisampleTexture;
		std::shared_ptr<Texture> m_pResolvedTexture;
		int m_Width, m_Height;
		bool m_bShouldResize, m_bUseRbo;

	private:
		bool Initialize();
		void CleanUp();
	public:
		Framebuffer();
		Framebuffer(int width, int height, bool bUseRbo);
		~Framebuffer();

		void Resolve();

		void Bind();
		void Unbind();

		void Resize(int width, int height);
		void CheckResize();

		inline const GLuint GetID() const { return m_MultisampleFboID; }
		inline const GLuint GetTextureID() const { return m_pResolvedTexture ? m_pResolvedTexture->GetID() : 0; }
		inline const int Width() const { return m_Width; }
		inline const int Height() const { return m_Height; }
	};
}