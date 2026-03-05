#pragma once
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
		bool shouldRender{ true };

		Material material;

		MeshRender();
		MeshRender(const Material& pMaterial);
		~MeshRender() = default;

		Material& GetMaterial() { return material; }

		static void CreateLuaMeshRendererBind(sol::state& lua);
	};
}