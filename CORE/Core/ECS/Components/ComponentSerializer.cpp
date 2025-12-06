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

	TransformComponent ComponentSerializer::DeserializeTransform(const rapidjson::Value& jsonValue)
	{
		return TransformComponent{
			.position = glm::vec3{jsonValue["position"]["x"].GetFloat(), jsonValue["position"]["y"].GetFloat(), jsonValue["position"]["z"].GetFloat()},
			.scale = glm::vec3{jsonValue["scale"]["x"].GetFloat(), jsonValue["scale"]["y"].GetFloat(), jsonValue["scale"]["z"].GetFloat()},
			.rotation_eular = glm::vec3{jsonValue["rotation_eular"]["x"].GetFloat(), jsonValue["rotation_eular"]["y"].GetFloat(), jsonValue["rotation_eular"]["z"].GetFloat()},
			.rotation_quat = glm::quat{jsonValue["rotation_quat"]["x"].GetFloat(), jsonValue["rotation_quat"]["y"].GetFloat(), jsonValue["rotation_quat"]["z"].GetFloat(),jsonValue["rotation_quat"]["w"].GetFloat()}
		};
	}

	PhysicsComponent ComponentSerializer::DeserializePhysics(const rapidjson::Value& jsonValue)
	{
		return PhysicsComponent();
	}

	MeshFilter ComponentSerializer::DeserializeMeshFilter(const rapidjson::Value& jsonValue)
	{
		return MeshFilter{
			.mesh = jsonValue["mesh"].GetString()
		};
	}

	MeshRender ComponentSerializer::DeserializeMeshRender(const rapidjson::Value& jsonValue)
	{
		return MeshRender{
			.shouldRender = jsonValue["shouldRender"].GetBool(),
			.shaderName = jsonValue["shaderName"].GetString(),
			.textureName = jsonValue["textureName"].GetString(),
			.color = glm::vec4{jsonValue["color"]["x"].GetFloat(),jsonValue["color"]["y"].GetFloat(),jsonValue["color"]["z"].GetFloat(),jsonValue["color"]["w"].GetFloat()}
		};
	}
}
