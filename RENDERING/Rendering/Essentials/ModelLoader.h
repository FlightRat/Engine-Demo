#pragma once
#include "Model.h"
#include <memory>
#include <string>
#include <vector>

struct aiNode;
struct aiMesh;
struct aiScene;

namespace ENGINE_RENDERING {
	class ModelLoader
	{
	private:
		static void processNode(aiNode* node, const aiScene* scene, std::vector<Mesh>& meshes);
		static Mesh processMesh(aiMesh* mesh, const aiScene* scene);

		static bool LoadModel(const std::string& modelPath, std::vector<Mesh>& meshes);
		static bool LoadModelFromMemory(const std::string& shapeName, std::vector<Mesh>& meshes);

	public:
		ModelLoader() = delete;
		static std::shared_ptr<Model> CreateModel(const std::string& modelPath);
		static std::shared_ptr<Model> CreateModelFromMemory(const std::string& shapeName);
	};
}