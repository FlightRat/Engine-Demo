#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <memory>
#include "../Essentials/Texture.h"

namespace ENGINE_RENDERING {
	class ShadowMap
	{
	private:
		GLuint m_FboID;
		std::shared_ptr<Texture> m_pDepthTexture;
		int m_Width, m_Height;
		bool m_bCube;
		bool m_bShouldResize;
		glm::mat4 m_lightSpaceMatrix;
	private:
		bool Initialize();
		void CleanUp();
	public:
		ShadowMap();
		ShadowMap(int width, int height, bool bCube=false);
		~ShadowMap();

		void Bind();
		void Unbind();

		void Resize(int width, int height);
		void CheckResize();

		inline const GLuint GetTextureID() const { return m_pDepthTexture ? m_pDepthTexture->GetID() : 0; }

		inline const int Width() const { return m_Width; }
		inline const int Height() const { return m_Height; }

		inline void SetLightSpaceMatrix(const glm::mat4 lightSpaceMatrix) { m_lightSpaceMatrix = lightSpaceMatrix; }
		inline const glm::mat4 GetLightSpaceMatrix() const { return m_lightSpaceMatrix; }
	};
}
