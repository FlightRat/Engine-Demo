#include "ComponentSerializer.h"
#include "FileSystem/Serializers/JSONSerializer.h"

namespace ENGINE_CORE::ECS {
	void ComponentSerializer::SerializeComponent(ENGINE_FileSystem::JSONSerializer& serializer, const TransformComponent& transform)
	{
		serializer.StartNewObject("transform")

			.StartNewObject("position")
			.AddKeyValuePair("x", transform.position.x)
			.AddKeyValuePair("y", transform.position.y)
			.AddKeyValuePair("z", transform.position.z)
			.EndObject()

			.StartNewObject("scale")
			.AddKeyValuePair("x", transform.scale.x)
			.AddKeyValuePair("y", transform.scale.y)
			.AddKeyValuePair("z", transform.scale.z)
			.EndObject()

			.StartNewObject("rotation_eular")
			.AddKeyValuePair("x", transform.rotation_eular.x)
			.AddKeyValuePair("y", transform.rotation_eular.y)
			.AddKeyValuePair("z", transform.rotation_eular.z)
			.EndObject()

			.StartNewObject("rotation_quat")
			.AddKeyValuePair("x", transform.rotation_quat.x)
			.AddKeyValuePair("y", transform.rotation_quat.y)
			.AddKeyValuePair("z", transform.rotation_quat.z)
			.AddKeyValuePair("w", transform.rotation_quat.w)
			.EndObject()

		.EndObject();
	}

	void ComponentSerializer::SerializeComponent(ENGINE_FileSystem::JSONSerializer& serializer, const PhysicsComponent& physics)
	{
	}

	void ComponentSerializer::SerializeComponent(ENGINE_FileSystem::JSONSerializer& serializer, const MeshFilter& meshFilter)
	{
		serializer.StartNewObject("meshFilter")
			.AddKeyValuePair("mesh", meshFilter.mesh)
		.EndObject();
	}

	void ComponentSerializer::SerializeComponent(ENGINE_FileSystem::JSONSerializer& serializer, const MeshRender& meshRender)
	{
		serializer.StartNewObject("meshRender")
			.AddKeyValuePair("shaderName", meshRender.shaderName)
			.AddKeyValuePair("textureName", meshRender.textureName)
			.StartNewObject("color")
			.AddKeyValuePair("R", meshRender.color.x)
			.AddKeyValuePair("G", meshRender.color.y)
			.AddKeyValuePair("B", meshRender.color.z)
			.AddKeyValuePair("A", meshRender.color.w)
			.EndObject()
			.AddKeyValuePair("shouldRende", meshRender.shouldRender)
		.EndObject();

	}

	void ComponentSerializer::SerializeComponent(ENGINE_FileSystem::JSONSerializer& serializer, const Identification& id)
	{
		serializer.StartNewObject("id")
			.AddKeyValuePair("name", id.name)
			.AddKeyValuePair("group", id.group)
		.EndObject();
	}


	void ComponentSerializer::DeserializeComponent(const rapidjson::Value& jsonValue, TransformComponent& transform)
	{
		transform.position = glm::vec3{ jsonValue["position"]["x"].GetFloat(), jsonValue["position"]["y"].GetFloat(), jsonValue["position"]["z"].GetFloat() };
		transform.scale = glm::vec3{ jsonValue["scale"]["x"].GetFloat(), jsonValue["scale"]["y"].GetFloat(), jsonValue["scale"]["z"].GetFloat() };
		transform.rotation_eular = glm::vec3{ jsonValue["rotation_eular"]["x"].GetFloat(), jsonValue["rotation_eular"]["y"].GetFloat(), jsonValue["rotation_eular"]["z"].GetFloat() };
		transform.rotation_quat = glm::quat{ jsonValue["rotation_quat"]["x"].GetFloat(), jsonValue["rotation_quat"]["y"].GetFloat(), jsonValue["rotation_quat"]["z"].GetFloat(),jsonValue["rotation_quat"]["w"].GetFloat() };
	}

	void ComponentSerializer::DeserializeComponent(const rapidjson::Value& jsonValue, PhysicsComponent& physics)
	{
		//TODO
	}

	void ComponentSerializer::DeserializeComponent(const rapidjson::Value& jsonValue, MeshFilter& meshFilter)
	{
		meshFilter.mesh = jsonValue["mesh"].GetString();
	}

	void ComponentSerializer::DeserializeComponent(const rapidjson::Value& jsonValue, MeshRender& meshRender)
	{
		meshRender.shaderName = jsonValue["shaderName"].GetString();
		meshRender.textureName = jsonValue["textureName"].GetString();
		meshRender.color = glm::vec4{ jsonValue["color"]["R"].GetFloat(),jsonValue["color"]["G"].GetFloat(),jsonValue["color"]["B"].GetFloat(),jsonValue["color"]["A"].GetFloat() };
	}

	void ComponentSerializer::DeserializeComponent(const rapidjson::Value& jsonValue, Identification& id)
	{
		id.name = jsonValue["name"].GetString();
		id.group = jsonValue["group"].GetString();
	}
}
