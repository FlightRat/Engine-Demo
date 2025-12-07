#pragma once
#include "TransformComponent.h"
#include "PhysicsComponent.h"
#include "MeshFilter.h"
#include "MeshRender.h"
#include "ScriptComponent.h"
#include "Identification.h"
#include "rapidjson/document.h"

namespace ENGINE_FileSystem {
	class JSONSerializer;
}

#define SERIALIZE_COMPONENT(serializer,comp) ENGINE_CORE::ECS::ComponentSerializer::Serialize(serializer, comp)
#define DESERIALIZE_COMPONENT(table, comp) ENGINE_CORE::ECS::ComponentSerializer::Deserialize(table, comp)

namespace ENGINE_CORE::ECS {
	class ComponentSerializer
	{
	public:
		ComponentSerializer() = delete;

		template <typename TComponent, typename TSerializer>
		static void Serialize(TSerializer& serializer, const TComponent& component);

		template <typename TComponent, typename TTable>
		static void Deserialize(const TTable& table, TComponent& component);


	private:
		// JSON serializer
		static void SerializeComponent(ENGINE_FileSystem::JSONSerializer& serializer, const TransformComponent& transform);
		static void SerializeComponent(ENGINE_FileSystem::JSONSerializer& serializer, const PhysicsComponent& physics);
		static void SerializeComponent(ENGINE_FileSystem::JSONSerializer& serializer, const MeshFilter& meshFilter);
		static void SerializeComponent(ENGINE_FileSystem::JSONSerializer& serializer, const MeshRender& meshRender);

		static void DeserializeComponent(const rapidjson::Value& jsonValue, TransformComponent& transform);
		static void DeserializeComponent(const rapidjson::Value& jsonValue, PhysicsComponent& physics);
		static void DeserializeComponent(const rapidjson::Value& jsonValue, MeshFilter& meshFilter);
		static void DeserializeComponent(const rapidjson::Value& jsonValue, MeshRender& meshRender);

	};
}

#include "ComponentSerializer.inl"