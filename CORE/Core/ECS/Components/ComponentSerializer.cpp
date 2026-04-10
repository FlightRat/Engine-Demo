#include "ComponentSerializer.h"
#include "FileSystem/Serializers/JSONSerializer.h"
#include <Logger/Logger.h>

namespace ENGINE_CORE::ECS {
	void ComponentSerializer::SerializeMaterial(ENGINE_FileSystem::JSONSerializer& serializer, const ENGINE_CORE::ECS::Material& material)
	{
		serializer.StartNewObject("")
			.AddKeyValuePair("shadingModel", material.shadingModel)
			.AddKeyValuePair("useTexture", material.m_useTexture)
			
			// pbr attribute
			.AddKeyValuePair("metallic", material.metallic)
			.AddKeyValuePair("roughness", material.roughness)
			.AddKeyValuePair("ao", material.ao)
			
			// color
			.StartNewObject("color")
			.AddKeyValuePair("R", material.color.r)
			.AddKeyValuePair("G", material.color.g)
			.AddKeyValuePair("B", material.color.b)
			.AddKeyValuePair("A", material.color.a)
			.EndObject()
			
			// texture
			.StartNewObject("textures");
		
		for (const auto& [slot, textureName] : material.m_textures)
		{
			serializer.AddKeyValuePair(slot, textureName);
			}

		serializer.EndObject()
			.EndObject();
	}

	void ComponentSerializer::SerializePhysicsAttr(ENGINE_FileSystem::JSONSerializer& serializer, const ENGINE_CORE::ECS::PhysicsAttributes& attributes)
	{
		serializer.StartNewObject("PhysicsAttr")

			.StartNewObject("position")
				.AddKeyValuePair("x", attributes.position.x)
				.AddKeyValuePair("y", attributes.position.y)
				.AddKeyValuePair("z", attributes.position.z)
			.EndObject()

			.StartNewObject("rotation")
				.AddKeyValuePair("x", attributes.rotation.x)
				.AddKeyValuePair("y", attributes.rotation.y)
				.AddKeyValuePair("z", attributes.rotation.z)
			.EndObject()

			.StartNewObject("scale")
				.AddKeyValuePair("x", attributes.scale.x)
				.AddKeyValuePair("y", attributes.scale.y)
				.AddKeyValuePair("z", attributes.scale.z)
			.EndObject()

			.AddKeyValuePair("rb_type", PhysicsAttributes::BodyTypeToString(attributes.rb_type))
			.AddKeyValuePair("rb_EnableGravity", attributes.rb_EnableGravity)
			.AddKeyValuePair("rb_Mass", attributes.rb_Mass)
			.AddKeyValuePair("rb_LinearDamping", attributes.rb_LinearDamping)
			.AddKeyValuePair("rb_AngularDamping", attributes.rb_AngularDamping)
			.StartNewObject("rb_LinearAxisFactor")
				.AddKeyValuePair("x", attributes.rb_LinearAxisFactor.x)
				.AddKeyValuePair("y", attributes.rb_LinearAxisFactor.y)
				.AddKeyValuePair("z", attributes.rb_LinearAxisFactor.z)
			.EndObject()
			.StartNewObject("rb_AngularAxisFactor")
				.AddKeyValuePair("x", attributes.rb_AngularAxisFactor.x)
				.AddKeyValuePair("y", attributes.rb_AngularAxisFactor.y)
				.AddKeyValuePair("z", attributes.rb_AngularAxisFactor.z)
			.EndObject()

			.AddKeyValuePair("shape", attributes.shape)
			.StartNewObject("box_halfExtents")
				.AddKeyValuePair("x", attributes.box_halfExtents.x)
				.AddKeyValuePair("y", attributes.box_halfExtents.y)
				.AddKeyValuePair("z", attributes.box_halfExtents.z)
			.EndObject()
			.AddKeyValuePair("sphere_radius", attributes.sphere_radius)
			.AddKeyValuePair("capsule_radius", attributes.capsule_radius)
			.AddKeyValuePair("capsule_halfHeight", attributes.capsule_halfHeight)

			.AddKeyValuePair("c_Trigger", attributes.c_Trigger)
			.AddKeyValuePair("c_Bounciness", attributes.c_Bounciness)
			.AddKeyValuePair("c_FrictionCoefficient", attributes.c_FrictionCoefficient)
			.AddKeyValuePair("c_MassDensity", attributes.c_MassDensity)

			.StartNewObject("objectData")
			.AddKeyValuePair("tag", attributes.objectData.tag)
			.AddKeyValuePair("group", attributes.objectData.group)
			.AddKeyValuePair("bCollider", attributes.objectData.bCollider)
			.AddKeyValuePair("bTrigger", attributes.objectData.bTrigger)
			.EndObject()

		.EndObject();

	}

	ENGINE_CORE::ECS::Material ComponentSerializer::DeserializeMaterial(const rapidjson::Value& matValue)
	{
		ENGINE_CORE::ECS::Material mat{};

		mat.shadingModel = matValue["shadingModel"].GetString();
		mat.m_useTexture = matValue["useTexture"].GetBool();
		//
		mat.metallic = matValue["metallic"].GetFloat();
		mat.roughness = matValue["roughness"].GetFloat();
		mat.ao = matValue["ao"].GetFloat();
		//
		const auto& color = matValue["color"];
		mat.color = glm::vec4{ color["R"].GetFloat(),color["G"].GetFloat(),color["B"].GetFloat(),color["A"].GetFloat() };
		//
		const auto& textures = matValue["textures"];
		for (auto it = textures.MemberBegin(); it != textures.MemberEnd(); ++it)
		{
			if (it->value.IsString())
			{
				mat.AddTexture(it->name.GetString(), it->value.GetString());
			}
		}
		return mat;
	}

	ENGINE_CORE::ECS::PhysicsAttributes ComponentSerializer::DeserializePhysicsAttr(const rapidjson::Value& attrValue)
	{
		ENGINE_CORE::ECS::PhysicsAttributes physicsAttr{};

		physicsAttr.position = glm::vec3{
			attrValue["position"]["x"].GetFloat(),
			attrValue["position"]["y"].GetFloat(),
			attrValue["position"]["z"].GetFloat(),
		};
		physicsAttr.rotation = glm::vec3{
			attrValue["rotation"]["x"].GetFloat(),
			attrValue["rotation"]["y"].GetFloat(),
			attrValue["rotation"]["z"].GetFloat(),
		};
		physicsAttr.scale = glm::vec3{
			attrValue["scale"]["x"].GetFloat(),
			attrValue["scale"]["y"].GetFloat(),
			attrValue["scale"]["z"].GetFloat(),
		};

		physicsAttr.rb_type = PhysicsAttributes::StringToBodyType(attrValue["rb_type"].GetString());
		physicsAttr.rb_EnableGravity = attrValue["rb_EnableGravity"].GetBool();
		physicsAttr.rb_Mass = attrValue["rb_Mass"].GetFloat();
		physicsAttr.rb_LinearDamping = attrValue["rb_LinearDamping"].GetFloat();
		physicsAttr.rb_AngularDamping = attrValue["rb_AngularDamping"].GetFloat();
		physicsAttr.rb_LinearAxisFactor = glm::vec3{
			attrValue["rb_LinearAxisFactor"]["x"].GetFloat(),
			attrValue["rb_LinearAxisFactor"]["y"].GetFloat(),
			attrValue["rb_LinearAxisFactor"]["z"].GetFloat(),
		};
		physicsAttr.rb_AngularAxisFactor = glm::vec3{
			attrValue["rb_AngularAxisFactor"]["x"].GetFloat(),
			attrValue["rb_AngularAxisFactor"]["y"].GetFloat(),
			attrValue["rb_AngularAxisFactor"]["z"].GetFloat(),
		};

		physicsAttr.shape = attrValue["shape"].GetString();
		physicsAttr.box_halfExtents = glm::vec3{
			attrValue["box_halfExtents"]["x"].GetFloat(),
			attrValue["box_halfExtents"]["y"].GetFloat(),
			attrValue["box_halfExtents"]["z"].GetFloat(),
		};
		physicsAttr.sphere_radius = attrValue["sphere_radius"].GetFloat();
		physicsAttr.capsule_radius = attrValue["capsule_radius"].GetFloat();
		physicsAttr.capsule_halfHeight = attrValue["capsule_halfHeight"].GetFloat();

		physicsAttr.c_Trigger = attrValue["c_Trigger"].GetBool();
		physicsAttr.c_Bounciness = attrValue["c_Bounciness"].GetFloat();
		physicsAttr.c_FrictionCoefficient = attrValue["c_FrictionCoefficient"].GetFloat();
		physicsAttr.c_MassDensity = attrValue["c_MassDensity"].GetFloat();

		physicsAttr.objectData.tag = attrValue["objectData"]["tag"].GetString();
		physicsAttr.objectData.group = attrValue["objectData"]["group"].GetString();
		physicsAttr.objectData.bCollider = attrValue["objectData"]["bCollider"].GetBool();
		physicsAttr.objectData.bTrigger = attrValue["objectData"]["bTrigger"].GetBool();

		return physicsAttr;
	}

	void ComponentSerializer::SerializeComponent(ENGINE_FileSystem::JSONSerializer& serializer, const Identification& id)
	{
		serializer.StartNewObject("id")
			.AddKeyValuePair("name", id.name)
			.AddKeyValuePair("group", id.group)
			.EndObject();
	}

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

	void ComponentSerializer::SerializeComponent(ENGINE_FileSystem::JSONSerializer& serializer, const MeshFilter& meshFilter)
	{
		serializer.StartNewObject("meshFilter")
			.AddKeyValuePair("mesh", meshFilter.mesh)
			.AddKeyValuePair("changed", meshFilter.changed)
		.EndObject();
	}

	void ComponentSerializer::SerializeComponent(ENGINE_FileSystem::JSONSerializer& serializer, const MeshRender& meshRender)
	{
		serializer.StartNewObject("meshRender")
			.AddKeyValuePair("shouldRender", meshRender.shouldRender)
			.AddKeyValuePair("flipUV", meshRender.flipUV)
			.AddKeyValuePair("materialCount", static_cast<int>(meshRender.materials.size()))
			.StartNewArray("materials");

		for (const auto& mat : meshRender.materials)
		{
			SerializeMaterial(serializer, mat);
		}

		serializer.EndArray()
			.EndObject();
	}

	void ComponentSerializer::SerializeComponent(ENGINE_FileSystem::JSONSerializer& serializer, const PhysicsComponent& physics)
	{
		serializer.StartNewObject("physics");
		SerializePhysicsAttr(serializer, physics.GetAttr());
		serializer.EndObject();
	}

	void ComponentSerializer::SerializeComponent(ENGINE_FileSystem::JSONSerializer& serializer, const LightComponent& light)
	{
		serializer.StartNewObject("light")

			.StartNewObject("color")
			.AddKeyValuePair("r", light.color.r)
			.AddKeyValuePair("g", light.color.g)
			.AddKeyValuePair("b", light.color.b)
			.EndObject()

			.AddKeyValuePair("type", light.type)

			//
			.StartNewObject("position")
			.AddKeyValuePair("x", light.pos.x)
			.AddKeyValuePair("y", light.pos.y)
			.AddKeyValuePair("z", light.pos.z)
			.EndObject()
			.AddKeyValuePair("constant", light.constant)
			.AddKeyValuePair("linear", light.linear)
			.AddKeyValuePair("quadratic", light.quadratic)
			.AddKeyValuePair("render", light.render)

			//
			.StartNewObject("direction")
			.AddKeyValuePair("x", light.direction.x)
			.AddKeyValuePair("y", light.direction.y)
			.AddKeyValuePair("z", light.direction.z)
			.EndObject()

		.EndObject();
	}

	void ComponentSerializer::DeserializeComponent(const rapidjson::Value& jsonValue, Identification& id)
	{
		id.name = jsonValue["name"].GetString();
		id.group = jsonValue["group"].GetString();
	}

	void ComponentSerializer::DeserializeComponent(const rapidjson::Value& jsonValue, TransformComponent& transform)
	{
		// position
		transform.position = glm::vec3{
			jsonValue["position"]["x"].GetFloat(),
			jsonValue["position"]["y"].GetFloat(),
			jsonValue["position"]["z"].GetFloat()
		};

		// scale
		transform.scale = glm::vec3{
			jsonValue["scale"]["x"].GetFloat(),
			jsonValue["scale"]["y"].GetFloat(),
			jsonValue["scale"]["z"].GetFloat()
		};

		// euler angles
		transform.rotation_eular = glm::vec3{
			jsonValue["rotation_eular"]["x"].GetFloat(),
			jsonValue["rotation_eular"]["y"].GetFloat(),
			jsonValue["rotation_eular"]["z"].GetFloat()
		};

		// glm::quat 构造顺序是 (w, x, y, z)
		transform.rotation_quat = glm::quat{
			jsonValue["rotation_quat"]["w"].GetFloat(),
			jsonValue["rotation_quat"]["x"].GetFloat(),
			jsonValue["rotation_quat"]["y"].GetFloat(),
			jsonValue["rotation_quat"]["z"].GetFloat()
		};
	}

	void ComponentSerializer::DeserializeComponent(const rapidjson::Value& jsonValue, MeshFilter& meshFilter)
	{
		meshFilter.mesh = jsonValue["mesh"].GetString();
	}

	void ComponentSerializer::DeserializeComponent(const rapidjson::Value& jsonValue, MeshRender& meshRender)
	{
		// --- 基础属性 ---
		if (jsonValue.HasMember("shouldRender") && jsonValue["shouldRender"].IsBool())
		meshRender.shouldRender = jsonValue["shouldRender"].GetBool();

		if (jsonValue.HasMember("flipUV") && jsonValue["flipUV"].IsBool())
			meshRender.flipUV = jsonValue["flipUV"].GetBool();

		// --- 材质数组 ---
		if (jsonValue.HasMember("materials") && jsonValue["materials"].IsArray())
		{
			const auto& materialsArray = jsonValue["materials"];

			meshRender.materials.clear();
			// 预分配内存，避免 push_back 时多次 realloc
			meshRender.materials.reserve(materialsArray.Size());

			for (rapidjson::SizeType i = 0; i < materialsArray.Size(); ++i)
			{
				if (materialsArray[i].IsObject())
				{
					meshRender.materials.emplace_back(
						DeserializeMaterial(materialsArray[i])
					);
				}
			}
		}
	}

	void ComponentSerializer::DeserializeComponent(const rapidjson::Value& jsonValue, PhysicsComponent& physics)
	{
		ENGINE_CORE::ECS::PhysicsAttributes physicsAttr = DeserializePhysicsAttr(jsonValue["PhysicsAttr"]);
		physics.SetAttr(physicsAttr);
	}

	void ComponentSerializer::DeserializeComponent(const rapidjson::Value& jsonValue, LightComponent& light)
	{
		light.color = glm::vec3{
			jsonValue["color"]["r"].GetFloat(),
			jsonValue["color"]["g"].GetFloat(),
			jsonValue["color"]["b"].GetFloat()
		};

		light.type = jsonValue["type"].GetString();

		light.pos = glm::vec3{
			jsonValue["position"]["x"].GetFloat(),
			jsonValue["position"]["y"].GetFloat(),
			jsonValue["position"]["z"].GetFloat()
		};
		light.constant = jsonValue["constant"].GetFloat();
		light.linear = jsonValue["linear"].GetFloat();
		light.quadratic = jsonValue["quadratic"].GetFloat();
		light.render = jsonValue["render"].GetBool();

		light.direction = glm::vec3{
			jsonValue["direction"]["x"].GetFloat(),
			jsonValue["direction"]["y"].GetFloat(),
			jsonValue["direction"]["z"].GetFloat()
		};
	}
}
