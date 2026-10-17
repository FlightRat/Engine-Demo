#include "PointShadowArray.h"
#include <stdexcept>
#include <Logger/Logger.h>

namespace ENGINE_RENDERING {
	PointShadowArray::~PointShadowArray()
	{
		glDeleteTextures(1, &m_TextureID);
		glDeleteFramebuffers(1, &m_FboID);
	}

	void PointShadowArray::Resize(int slots, int resolution)
	{
		if (m_Slots == slots && m_Resolution == resolution) return;
		if (slots > 0) {
			GLint layers, maxSize;
			glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &layers);
			glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &maxSize);
			if (slots > layers / 6 || resolution > maxSize) {
				ENGINE_ERROR("Point shadow array exceeds device limits: {} slots at {}.", slots, resolution);
				throw std::length_error("Point shadow array capacity exceeded");
			}
		}
		glDeleteTextures(1, &m_TextureID);
		glDeleteFramebuffers(1, &m_FboID);
		m_TextureID = m_FboID = 0;
		m_Slots = slots;
		m_Resolution = resolution;
		if (slots == 0) return;
		glCreateTextures(GL_TEXTURE_CUBE_MAP_ARRAY, 1, &m_TextureID);
		glTextureStorage3D(m_TextureID, 1, GL_DEPTH_COMPONENT24, resolution, resolution, slots * 6);
		glTextureParameteri(m_TextureID, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(m_TextureID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTextureParameteri(m_TextureID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(m_TextureID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTextureParameteri(m_TextureID, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glCreateFramebuffers(1, &m_FboID);
		glNamedFramebufferTexture(m_FboID, GL_DEPTH_ATTACHMENT, m_TextureID, 0);
		glNamedFramebufferDrawBuffer(m_FboID, GL_NONE);
		glNamedFramebufferReadBuffer(m_FboID, GL_NONE);
	}

	void PointShadowArray::BindSlot(int slot) const
	{
		const float depth = 1.0f;
		glClearTexSubImage(m_TextureID, 0, 0, 0, slot * 6,
			m_Resolution, m_Resolution, 6, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
		glBindFramebuffer(GL_FRAMEBUFFER, m_FboID);
		glViewport(0, 0, m_Resolution, m_Resolution);
	}
}
