#pragma once
#include "Model.h"
#include <memory>

namespace ENGINE_RENDERING {
	class ModelLoader
	{
	private:
		static bool LoadModel(const std::string& modelPath, std::vector<Mesh>& meshes);
		static bool LoadModelFromMemory(const std::string& shapeName, std::vector<Mesh>& meshes);

		//void processNode(aiNode* node, const aiScene* scene);
		//Mesh processMesh(aiMesh* mesh, const aiScene* scene);

	public:
		ModelLoader() = delete;
		static std::shared_ptr<Model> CreateModel(const std::string& modelPath);
		static std::shared_ptr<Model> CreateModelFromMemory(const std::string& shapeName);
	};
}