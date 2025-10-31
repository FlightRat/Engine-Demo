#include "PhysicsComponent.h"
#include<Logger/Logger.h>

namespace ENGINE_CORE::ECS {
	PhysicsComponent::PhysicsComponent() :m_pAttribute{ PhysicsAttributes {} }
	{
	}

	PhysicsComponent::PhysicsComponent(const PhysicsAttributes& pAttributes):
		m_pRigidBody{ nullptr }, m_pCollisionShape{ nullptr }, m_pCollider{nullptr}, 
		m_pAttribute{ pAttributes }, m_pUserData{ nullptr }
	{
	}

	void PhysicsComponent::Init(std::shared_ptr<PhysicsCommon> common, std::shared_ptr<PhysicsWorld> world)
	{
		if (!common)
		{
			ENGINE_ERROR("Failed to initialize the physics component - Physics common is nullptr!");
			return;
		}
		if (!world)
		{
			ENGINE_ERROR("Failed to initialize the physics component - Physics world is nullptr!");
			return;
		}

		// position
		rp3d::Vector3 rb_position(m_pAttribute.position.x, m_pAttribute.position.y, m_pAttribute.position.z);
		// rotation
		glm::vec3 eulerDegree(m_pAttribute.rotation.x, m_pAttribute.rotation.y, m_pAttribute.rotation.z);
		glm::quat gl_quat = glm::quat(glm::radians(eulerDegree));
		rp3d::Quaternion rb_rotation = rp3d::Quaternion(gl_quat.x, gl_quat.y, gl_quat.z, gl_quat.w);
		rp3d::Transform transform(rb_position, rb_rotation);

		// rigid body
		m_previousTransform = transform;
		m_currentTransform = transform;
		m_pRigidBody = ENGINE_PHYSICS::MakeSharedRigidBody(world, transform);
		if (!m_pRigidBody)
		{
			ENGINE_ERROR("Failed to create the rigid body!");
			return;
		}
		m_pRigidBody->setType(m_pAttribute.rb_type);
		m_pRigidBody->enableGravity(m_pAttribute.rb_EnableGravity);
		m_pRigidBody->setMass(m_pAttribute.rb_Mass);
		m_pRigidBody->setLinearDamping(m_pAttribute.rb_LinearDamping);
		m_pRigidBody->setAngularDamping(m_pAttribute.rb_AngularDamping);
		m_pRigidBody->setLinearLockAxisFactor(rp3d::Vector3(m_pAttribute.rb_LinearAxisFactor.x, m_pAttribute.rb_LinearAxisFactor.y, m_pAttribute.rb_LinearAxisFactor.z));
		m_pRigidBody->setAngularLockAxisFactor(rp3d::Vector3(m_pAttribute.rb_AngularAxisFactor.x, m_pAttribute.rb_AngularAxisFactor.y, m_pAttribute.rb_AngularAxisFactor.z));

		// shape
		if (m_pAttribute.shape == "box")
		{
			m_pCollisionShape = ENGINE_PHYSICS::MakeSharedBoxCollisionShape(common, rp3d::Vector3(m_pAttribute.box_halfExtents.x, m_pAttribute.box_halfExtents.y, m_pAttribute.box_halfExtents.z));
		}
		else if (m_pAttribute.shape == "sphere")
		{
			m_pCollisionShape = ENGINE_PHYSICS::MakeSharedSphereCollisionShape(common, m_pAttribute.sphere_radius);
		}
		else if (m_pAttribute.shape == "capsule")
		{
			m_pCollisionShape = ENGINE_PHYSICS::MakeSharedCapsuleCollisionShape(common, m_pAttribute.capsule_radius, m_pAttribute.capsule_halfHeight);
		}

		// collider
		m_pCollider = ENGINE_PHYSICS::MakeSharedCollider(m_pRigidBody, m_pCollisionShape);
		Material& c_material = m_pCollider->getMaterial();
		c_material.setBounciness(m_pAttribute.c_Bounciness);
		c_material.setFrictionCoefficient(m_pAttribute.c_FrictionCoefficient);
		c_material.setMassDensity(m_pAttribute.c_MassDensity);

		// user data
		m_pUserData = std::make_shared<ENGINE_PHYSICS::UserData>();
		m_pUserData->userData = m_pAttribute.objectData;
		m_pUserData->type_id = entt::type_hash<ENGINE_PHYSICS::ObjectData>::value();
		m_pRigidBody->setUserData(m_pUserData.get());
	}

	void PhysicsComponent::CreateLuaPhysicsBind(sol::state& lua, entt::registry& registry)
	{
		lua.new_enum<BodyType>(
			"BodyType", {
				{"Static",BodyType::STATIC},
				{"Dynamic",BodyType::DYNAMIC},
				{"Kinematic",BodyType::KINEMATIC}
			}
		);

		lua.new_usertype<ENGINE_PHYSICS::ObjectData>(
			"ObjectData",
			"type_id", entt::type_hash<ENGINE_PHYSICS::ObjectData>::value,
			sol::call_constructor,
			sol::factories(
				[](const std::string& tag, const std::string& group, bool bCollider, bool bTrigger, std::uint32_t entityID)
				{
					return ENGINE_PHYSICS::ObjectData{
						.tag = tag,
						.group = group,
						.bCollider = bCollider,
						.bTrigger = bTrigger,
						.entityID = entityID
					};
				},
				[](const sol::table& objectData)
				{
					return ENGINE_PHYSICS::ObjectData{
						.tag = objectData["tag"].get_or(std::string{""}),
						.group = objectData["group"].get_or(std::string{""}),
						.bCollider = objectData["bCollider"].get_or(false),
						.bTrigger = objectData["bTrigger"].get_or(false),
						.entityID = objectData["tag"].get_or((std::uint32_t)entt::null)
					};
				}
			),
			"tag", &ENGINE_PHYSICS::ObjectData::tag,
			"group", &ENGINE_PHYSICS::ObjectData::group,
			"entityID", &ENGINE_PHYSICS::ObjectData::entityID,
			"bCollider", &ENGINE_PHYSICS::ObjectData::bCollider,
			"bTrigger", &ENGINE_PHYSICS::ObjectData::bTrigger,
			"contactEntities", &ENGINE_PHYSICS::ObjectData::contactEntities,
			"to_string", &ENGINE_PHYSICS::ObjectData::to_string
		);

		lua.new_usertype<PhysicsAttributes>(
			"PhysicsAttributes",
			sol::call_constructor,
			sol::factories(
				[] {return PhysicsAttributes{}; },
				[](const sol::table& physAttr)
				{
					return PhysicsAttributes{
						.position = glm::vec3{physAttr["position"]["x"].get_or(0.0f), physAttr["position"]["y"].get_or(0.0f), physAttr["position"]["z"].get_or(0.0f)},
						.rotation = glm::vec3{physAttr["rotation"]["x"].get_or(0.0f), physAttr["rotation"]["y"].get_or(0.0f), physAttr["rotation"]["z"].get_or(0.0f)},
						.rb_type = physAttr["type"].get_or(BodyType::STATIC),
						.rb_EnableGravity = physAttr["enable_gravity"].get_or(true),
						.rb_Mass = physAttr["mass"].get_or(1.0f),
						.rb_LinearDamping = physAttr["linear_damping"].get_or(0.0f),
						.rb_AngularDamping = physAttr["angular_damping"].get_or(0.0f),
						.rb_LinearAxisFactor = glm::vec3{physAttr["linear_axis_factor"]["x"].get_or(1.0f), physAttr["linear_axis_factor"]["y"].get_or(1.0f), physAttr["linear_axis_factor"]["z"].get_or(1.0f)},
						.rb_AngularAxisFactor = glm::vec3{physAttr["angular_axis_factor"]["x"].get_or(1.0f), physAttr["angular_axis_factor"]["y"].get_or(1.0f), physAttr["angular_axis_factor"]["z"].get_or(1.0f)},
						//
						.shape = physAttr["shape"].get_or(std::string{"box"}),
						.box_halfExtents = glm::vec3{physAttr["box_halfExtents"]["x"].get_or(1.0f), physAttr["box_halfExtents"]["y"].get_or(1.0f), physAttr["box_halfExtents"]["z"].get_or(1.0f)},
						.sphere_radius = physAttr["sphere_radius"].get_or(1.0f),
						.capsule_radius = physAttr["capsule_radius"].get_or(1.0f),
						.capsule_halfHeight = physAttr["capsule_halfHeight"].get_or(1.0f),
						//
						.c_Bounciness = physAttr["bounciness"].get_or(0.5f),
						.c_FrictionCoefficient = physAttr["friction"].get_or(0.3f),
						.c_MassDensity = physAttr["mass_density"].get_or(1.0f),
						.objectData = ENGINE_PHYSICS::ObjectData{
							.tag = physAttr["objectData"]["tag"].get_or(std::string{""}),
							.group = physAttr["objectData"]["group"].get_or(std::string{""}),
							.bCollider = physAttr["objectData"]["bCollider"].get_or(false),
							.bTrigger = physAttr["objectData"]["bTrigger"].get_or(false),
							.entityID = physAttr["objectData"]["tag"].get_or((std::uint32_t)entt::null)
						}
					};
				}
			),
			"position", &PhysicsAttributes::position,
			"rotation", &PhysicsAttributes::rotation,
			"enable_gravity", &PhysicsAttributes::rb_EnableGravity,
			"type", &PhysicsAttributes::rb_type,
			"mass", &PhysicsAttributes::rb_Mass,
			"linear_damping", &PhysicsAttributes::rb_LinearDamping,
			"linear_axis_factor", &PhysicsAttributes::rb_LinearAxisFactor,
			"angular_damping", &PhysicsAttributes::rb_AngularDamping,
			"angular_axis_factor", &PhysicsAttributes::rb_AngularAxisFactor,
			"shape", &PhysicsAttributes::shape,
			"box_halfExtents", &PhysicsAttributes::box_halfExtents,
			"sphere_radius", &PhysicsAttributes::sphere_radius,
			"capsule_radius", &PhysicsAttributes::capsule_radius,
			"capsule_halfHeight", &PhysicsAttributes::capsule_halfHeight,
			"bounciness", &PhysicsAttributes::c_Bounciness,
			"friction", &PhysicsAttributes::c_FrictionCoefficient,
			"mass_density", &PhysicsAttributes::c_MassDensity,
			"objectData",&PhysicsAttributes::objectData
		);

		auto& common = registry.ctx().get<std::shared_ptr<PhysicsCommon>>();
		auto& world = registry.ctx().get<std::shared_ptr<PhysicsWorld>>();
		if (!common || ! world)
			return;

		lua.new_usertype<PhysicsComponent>(
			"Physics",
			"type_id",&entt::type_hash<PhysicsComponent>::value,
			sol::call_constructor,
			sol::factories(
				[&](const PhysicsAttributes& attr) {
					PhysicsComponent pc{ attr };
					pc.Init(common, world);
					return pc;
				}
			),
			"attributes", &PhysicsComponent::m_pAttribute,
			"linear_impulse",[](PhysicsComponent& pc, const glm::vec3 impulse){
				auto body = pc.GetRigidBody();
				body->applyLocalForceAtCenterOfMass(Vector3(impulse.x, impulse.y, impulse.z));
			},
			"angular_impulse", [](PhysicsComponent& pc, const glm::vec3 impulse) {
				auto body = pc.GetRigidBody();
				body->applyLocalTorque(Vector3(impulse.x, impulse.y, impulse.z));
			},
			"set_linear_velocity", [](PhysicsComponent& pc, const glm::vec3 velocity) {
				auto body = pc.GetRigidBody();
				body->setLinearVelocity(Vector3(velocity.x, velocity.y, velocity.z));
			},
			"set_angular_velocity", [](PhysicsComponent& pc, const glm::vec3 velocity) {
				auto body = pc.GetRigidBody();
				body->setAngularVelocity(Vector3(velocity.x, velocity.y, velocity.z));
			},
			"get_linear_velocity", [](PhysicsComponent& pc) {
				auto body = pc.GetRigidBody();
				const Vector3 velocity = body->getLinearVelocity();
				return glm::vec3(velocity.x, velocity.y, velocity.z);
			},
			"get_angular_velocity", [](PhysicsComponent& pc) {
				auto body = pc.GetRigidBody();
				const Vector3 velocity = body->getAngularVelocity();
				return glm::vec3(velocity.x, velocity.y, velocity.z);
			},
			"user_data",&PhysicsComponent::GetUserData
		);
	}
}


