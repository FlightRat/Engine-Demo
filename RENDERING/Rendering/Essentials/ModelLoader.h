#pragma once
#include "Model.h"
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace ENGINE_RENDERING {
	class ModelLoader
	{
	private:
		static std::string loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName,
			std::map<std::string, std::string>& textures, const std::string& directory);

		static void processNode(aiNode* node, const aiScene* scene, std::vector<Mesh>& meshes,
			std::map<std::string, std::string>& textures, const std::string& directory);

		static Mesh processMesh(aiMesh* mesh, const aiScene* scene,
			std::map<std::string, std::string>& textures, const std::string& directory);

		static bool LoadModel(const std::string& modelPath, std::vector<Mesh>& meshes, std::map<std::string, std::string>& textures);
		static bool LoadModelFromMemory(const std::string& shapeName, std::vector<Mesh>& meshes);

	public:
		ModelLoader() = delete;
		static std::shared_ptr<Model> CreateModel(const std::string& modelPath, std::map<std::string, std::string>& textures);
		static std::shared_ptr<Model> CreateModelFromMemory(const std::string& shapeName);
	};
}