#pragma once
#include <glad/glad.h>

namespace ENGINE_RENDERING {
	class PointShadowArray
	{
	private:
		GLuint m_TextureID{ 0 }, m_FboID{ 0 };
		int m_Slots{ 0 }, m_Resolution{ 0 };
	public:
		~PointShadowArray();
		void Resize(int slots, int resolution);
		void BindSlot(int slot) const;
		inline GLuint GetTextureID() const { return m_TextureID; }
		inline int Resolution() const { return m_Resolution; }
	};
}
