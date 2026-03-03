#pragma once
#include<glad/glad.h>
#include<glm/glm.hpp>
#include<sol/sol.hpp>
#include<string>
#include<map>
#include"MeshFilter.h"

namespace ENGINE_CORE::ECS {
	struct Material
	{
		std::string shaderName;

		glm::vec4 color{ 1.0f };
		float shininess = 32.0f;

		bool m_useTexture{ false };
		std::map<std::string, std::string> m_textures;
		void AddTexture(const std::string& key, const std::string& texName)
		{
			m_textures[key] = texName;
		}
	};

	struct MeshRender
	{
		bool m_loaded{ false };
		bool shouldRender{ true };

		Material material;
		GLuint m_VAO = 0, m_VBO = 0, m_EBO = 0;

		MeshRender();
		MeshRender(const Material& pMaterial);
		~MeshRender() = default;

		Material& GetMaterial() { return material; }

		void UploadMesh(MeshFilter MF);

		static void CreateLuaMeshRendererBind(sol::state& lua);
	};
}