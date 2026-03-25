#pragma once
#include <glad/glad.h>
#include <memory>

#include "../Essentials/Texture.h"

namespace ENGINE_RENDERING {
	class Gbuffer
	{
	private:
		GLuint m_FboID, m_RboID;
		std::shared_ptr<Texture> m_PositionTex;
		std::shared_ptr<Texture> m_NormalTex;
		std::shared_ptr<Texture> m_AlbedoSpecTex;
		std::shared_ptr<Texture> m_ReflTex;
		int m_Width, m_Height;
		bool m_bShouldResize;
	private:
		bool Initialize();
		void CleanUp();
	public:
		Gbuffer();
		Gbuffer(int width, int height);
		~Gbuffer();

		void Bind();
		void Unbind();

		void Resize(int width, int height);
		void CheckResize();

		inline const GLuint GetID() const { return m_FboID; }
		inline const GLuint GetPosition() const { return m_PositionTex ? m_PositionTex->GetID() : 0; }
		inline const GLuint GetNormal() const { return m_NormalTex ? m_NormalTex->GetID() : 0; }
		inline const GLuint GetAlbedoSpec() const { return m_AlbedoSpecTex ? m_AlbedoSpecTex->GetID() : 0; }
		inline const GLuint GetRefl() const { return m_ReflTex ? m_ReflTex->GetID() : 0; }
		inline const int Width() const { return m_Width; }
		inline const int Height() const { return m_Height; }
	};

}