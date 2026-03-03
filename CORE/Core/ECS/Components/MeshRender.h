#pragma once
#include<glad/glad.h>
#include<glm/glm.hpp>
#include<sol/sol.hpp>
#include<string>
#include"MeshFilter.h"

namespace ENGINE_CORE::ECS {
	struct MeshRender
	{
		bool m_loaded{ false };
		bool shouldRender{ true };
		bool m_useTexture{ false };
		// TODO:find better way for the shader/texture
		std::string shaderName;
		std::string textureName;
		glm::vec4 color;

		GLuint m_VAO = 0, m_VBO = 0, m_EBO = 0;

		void UploadMesh(MeshFilter MF);

		static void CreateLuaMeshRendererBind(sol::state& lua);
	};
}