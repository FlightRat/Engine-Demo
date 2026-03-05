#pragma once
#include "Mesh.h"
#include <memory>

namespace ENGINE_RENDERING {
	class MeshLoader
	{
	private:
		static bool LoadMesh(const std::string& meshPath, std::vector <Vertex>& vertices, std::vector <unsigned int>& indices);
		static bool LoadMeshFromMemory(const std::string& shapeName, std::vector <Vertex>& vertices, std::vector <unsigned int>& indices);

	public:
		MeshLoader() = delete;
		static std::shared_ptr<Mesh> Create(const std::string& meshPath);
		static std::shared_ptr<Mesh> CreateFromMemory(const std::string& shapeName);
	};
}