#pragma once
#include<glad/glad.h>
#include<glm/glm.hpp>

namespace RENDERING {
	struct Color
	{
		GLubyte r, g, b, a;
	};
	struct Vertex
	{
		glm::vec2 position{ 0.f }, uvs{ 0.f };
		Color color{ .r = 255,.g = 255,.b = 255,.a = 255 };

		void SetColor(GLubyte r, GLubyte g, GLubyte b, GLubyte a)
		{
			color.r = r;
			color.g = g;
			color.b = b;
			color.a = a;
		}

		void SetColor(GLuint newColor)
		{
			color.r = (newColor >> 24) & 0xFF;
			color.g = (newColor >> 16) & 0xFF;
			color.b = (newColor >> 8) & 0xFF;
			color.a = (newColor >> 0) & 0xFF;
		}
	};
}