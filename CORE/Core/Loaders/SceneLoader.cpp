#include "SceneLoader.h"
#include "Logger/Logger.h"
#include "FileSystem/Serializers/JSONSerializer.h"
#include "../ECS/Registry.h"
#include "../ECS/Entity.h"
#include "../ECS/Components/ComponentSerializer.h"
#include "../ECS/Components/ScriptComponent.h"
#include "../ECS/Components/TransformComponent.h"
#include "../ECS/Components/MeshFilter.h"
#include "../ECS/Components/MeshRender.h"
#include "../ECS/Components/Identification.h"
#include "../ECS/Components/PhysicsComponent.h"
#include "../ECS/Components/LightComponent.h"
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
		// 遍历所有实体（不限定组件组合）
		auto& enttRegistry = registry.GetRegistry();
		auto allEntities = enttRegistry.view<entt::entity>(entt::exclude<ENGINE_CORE::ECS::ScriptComponent>);

		for (auto entity : allEntities)
		{
			pSerializer->StartNewObject("");  // 每个实体是数组中的一个对象
			pSerializer->StartNewObject("Components");

			// --- Identification（几乎每个实体都有，优先写）---
			if (auto* id = enttRegistry.try_get<Identification>(entity))
				SERIALIZE_COMPONENT(*pSerializer, *id);

			// --- TransformComponent ---
			if (auto* transform = enttRegistry.try_get<TransformComponent>(entity))
				SERIALIZE_COMPONENT(*pSerializer, *transform);

			// --- MeshFilter ---
			if (auto* meshFilter = enttRegistry.try_get<MeshFilter>(entity))
				SERIALIZE_COMPONENT(*pSerializer, *meshFilter);

			// --- MeshRender ---
			if (auto* meshRender = enttRegistry.try_get<MeshRender>(entity))
				SERIALIZE_COMPONENT(*pSerializer, *meshRender);

			// --- PhysicsComponent ---
			if (auto* physics = enttRegistry.try_get<PhysicsComponent>(entity))
				SERIALIZE_COMPONENT(*pSerializer, *physics);

			// --- LightComponent ---
			if (auto* light = enttRegistry.try_get<LightComponent>(entity))
				SERIALIZE_COMPONENT(*pSerializer, *light);

			// 后续新增组件在这里加一行即可，不影响其他逻辑

			pSerializer->EndObject();  // 结束 Components
			pSerializer->EndObject();  // 结束实体对象
		}
		pSerializer->EndArray();

		return pSerializer->EndDocument();
	}

	bool SceneLoader::LoadSceneJSON(ENGINE_CORE::ECS::Registry& registry, const std::string& sSceneFile)
	{
		// std::ifstream -→ std::stringstream -→ rapidjson::StringStream -→ rapidjson::Document解析 -→ scene = doc["Scene"] -→ 遍历

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
			//bool check = obj.HasMember("Components");
			const auto& components = obj["Components"];

			//id
			const auto& jsonID = components["id"];

			ENGINE_CORE::ECS::Entity newObj{ registry, jsonID["name"].GetString(), jsonID["group"].GetString() };

			//transform
			if (components.HasMember("transform"))
			{
				auto& transform = newObj.AddComponent<TransformComponent>();
				DESERIALIZE_COMPONENT(components["transform"], transform);
			}

			//mesh filter
			if (components.HasMember("meshFilter"))
			{
				auto& meshFilter = newObj.AddComponent<MeshFilter>();
				DESERIALIZE_COMPONENT(components["meshFilter"], meshFilter);
			}

			//mesh render
			if (components.HasMember("meshRender"))
			{
				auto& meshRender = newObj.AddComponent<MeshRender>();
				DESERIALIZE_COMPONENT(components["meshRender"], meshRender);
			}

			//physics
			if (components.HasMember("physics"))
			{
				auto& common = registry.GetRegistry().ctx().get<std::shared_ptr<PhysicsCommon>>();
				auto& world = registry.GetRegistry().ctx().get<std::shared_ptr<PhysicsWorld>>();
				if (!common || !world)
					return false;

				auto& physics = newObj.AddComponent<PhysicsComponent>();
				DESERIALIZE_COMPONENT(components["physics"], physics);
				physics.GetAttr().objectData.entityID = static_cast<int32_t>(newObj.GetEntity());
				physics.Init(common, world);
				physics.initialized = true;
			}

			//light
			if (components.HasMember("light"))
			{
				auto& light = newObj.AddComponent<LightComponent>();
				DESERIALIZE_COMPONENT(components["light"], light);
			}

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


