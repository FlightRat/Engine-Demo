#include "SceneLoader.h"
#include "Logger/Logger.h"
#include "FileSystem/Serializers/JSONSerializer.h"
#include "../ECS/Registry.h"
#include "../ECS/Entity.h"
#include "../ECS/Components/ComponentSerializer.h"
#include "../ECS/Components/TransformComponent.h"
#include "../ECS/Components/MeshFilter.h"
#include <rapidjson/error/en.h>
#include <filesystem>

using namespace ENGINE_CORE::ECS;
using namespace ENGINE_FileSystem;

namespace ENGINE_CORE::Loaders {
	bool SceneLoader::SaveSceneJSON(Registry& registry, const std::string& sSceneFile)
	{
		std::unique_ptr<JSONSerializer> pSerializer = { nullptr };
		try
		{
			pSerializer = std::make_unique<JSONSerializer>(sSceneFile);
		}
		catch (const std::exception& ex)
		{
			ENGINE_ERROR("Failed to save tilemap [{}] - [{}]", sSceneFile, ex.what());
			return false;
		}
		
		std::filesystem::path scenePath{ sSceneFile };
		if (!std::filesystem::exists(scenePath))
		{
			ENGINE_ERROR("Failed to save scene - Scene path does not exist! [{}]", sSceneFile);
			return false;
		}

		pSerializer->StartDocument();
		pSerializer->StartNewArray("Scene");
		auto view = registry.GetRegistry().view<TransformComponent, MeshFilter, MeshRender>();
		for (auto [entity, transform, meshF, meshR] : view.each())
		{
			auto e = ENGINE_CORE::ECS::Entity(registry, entity);
			pSerializer->StartNewObject();

			pSerializer->StartNewObject("Components");
			SERIALIZE_COMPONENT(*pSerializer, transform);
			SERIALIZE_COMPONENT(*pSerializer, meshF);
			SERIALIZE_COMPONENT(*pSerializer, meshR);
			pSerializer->EndObject();

			pSerializer->EndObject();
		}
		pSerializer->EndArray();

		return pSerializer->EndDocument();
	}

	bool SceneLoader::LoadSceneJSON(ENGINE_CORE::ECS::Registry& registry, const std::string& sSceneFile)
	{
		// std::ifstream -¡ú std::stringstream -¡ú rapidjson::StringStream -¡ú rapidjson::Document½âÎö -¡ú scene = doc["Scene"] -¡ú ±éÀú

		std::ifstream sceneFile;
		sceneFile.open(sSceneFile);
		if (!sceneFile.is_open())
		{
			ENGINE_ERROR("Failed to open scene file [{}]", sSceneFile);
			return false;
		}

		std::stringstream ss;
		ss << sceneFile.rdbuf();
		std::string contents = ss.str();
		rapidjson::StringStream jsonStr{ contents.c_str() };

		rapidjson::Document doc;
		doc.ParseStream(jsonStr);
		if (doc.HasParseError() || !doc.IsObject())
		{
			ENGINE_ERROR("Failed to load scene [{}] -- not valid json. - {} - {}",
				sSceneFile,
				rapidjson::GetParseError_En(doc.GetParseError()),
				doc.GetErrorOffset());
			return false;
		}
		const rapidjson::Value& scene = doc["Scene"];
		if (!scene.IsArray() || scene.Size() < 1)
		{
			ENGINE_ERROR("Failed to load scene file [{}] - There needs to be at least 1 scene!", sSceneFile);
			return false;
		}

		for (const auto& obj : scene.GetArray())
		{
			ENGINE_CORE::ECS::Entity newObj{ registry, "", "" };
			//bool check = obj.HasMember("Components");
			const auto& components = obj["Components"];

			//transform
			const auto& jsonTransform = components["transform"];
			auto& transform = newObj.AddComponent<TransformComponent>();
			DESERIALIZE_COMPONENT(jsonTransform, transform);

			//mesh filter
			const auto& jsonMeshFilter = components["meshFilter"];
			auto& meshFilter = newObj.AddComponent<MeshFilter>();
			DESERIALIZE_COMPONENT(jsonMeshFilter, meshFilter);
			meshFilter.load_mesh();

			//mesh render
			const auto& jsonMeshRender = components["meshRender"];
			auto& meshRender = newObj.AddComponent<MeshRender>();
			DESERIALIZE_COMPONENT(jsonMeshRender, meshRender);
		}
		sceneFile.close();
		return true;
	}

	bool SceneLoader::SaveScene(ENGINE_CORE::ECS::Registry& registry, const std::string& sSceneFile, bool bUseJSON)
	{
		if (bUseJSON)
			return SaveSceneJSON(registry, sSceneFile);
		return false;
	}

	bool SceneLoader::LoadScene(ENGINE_CORE::ECS::Registry& registry, const std::string& sSceneFile, bool bUseJSON)
	{
		if (bUseJSON)
			return LoadSceneJSON(registry, sSceneFile);
		return false;
	}
}


