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
#define DESERIALIZE_COMPONENT(serializer, comp) ENGINE_CORE::ECS::ComponentSerializer::Deserialize<comp>(serializer)

namespace ENGINE_CORE::ECS {
	class ComponentSerializer
	{
	public:
		ComponentSerializer() = delete;

		template <typename TComponent, typename TSerializer>
		static void Serialize(TSerializer& serializer, const TComponent& component);

		template <typename TComponent, typename TTable>
		static auto Deserialize(const TTable& table);

	private:
		// JSON serializer
		static void SerializeComponent(ENGINE_FileSystem::JSONSerializer& serializer, const TransformComponent& transform);
		static void SerializeComponent(ENGINE_FileSystem::JSONSerializer& serializer, const PhysicsComponent& physics);
		static void SerializeComponent(ENGINE_FileSystem::JSONSerializer& serializer, const MeshFilter& meshFilter);
		static void SerializeComponent(ENGINE_FileSystem::JSONSerializer& serializer, const MeshRender& meshRender);

		static TransformComponent DeserializeTransform(const rapidjson::Value& jsonValue);
		static PhysicsComponent DeserializePhysics(const rapidjson::Value& jsonValue);
		static MeshFilter DeserializeMeshFilter(const rapidjson::Value& jsonValue);
		static MeshRender DeserializeMeshRender(const rapidjson::Value& jsonValue);
	};
}

#include "ComponentSerializer.inl"