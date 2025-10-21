#include "PhysicsComponent.h"
#include<Logger/Logger.h>

namespace ENGINE_CORE::ECS {
	PhysicsComponent::PhysicsComponent() :m_pAttribute{ PhysicsAttributes {} }
	{
	}

	PhysicsComponent::PhysicsComponent(PhysicsAttributes pAttributes):m_pAttribute{pAttributes}
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
		m_pRigidBody = ENGINE_PHYSICS::MakeSharedRigidBody(world, transform);
		if (!m_pRigidBody)
		{
			ENGINE_ERROR("Failed to create the rigid body!");
			return;
		}
		m_pRigidBody->setType(m_pAttribute.Type);
		m_pRigidBody->enableGravity(m_pAttribute.rb_EnableGravity);
		m_pRigidBody->setMass(m_pAttribute.rb_Mass);
		m_pRigidBody->setLinearDamping(m_pAttribute.rb_LinearDamping);
		m_pRigidBody->setAngularDamping(m_pAttribute.rb_AngularDamping);
		m_pRigidBody->setLinearLockAxisFactor(rp3d::Vector3(m_pAttribute.rb_LinearAxisFactor.x, m_pAttribute.rb_LinearAxisFactor.y, m_pAttribute.rb_LinearAxisFactor.z));
		m_pRigidBody->setAngularLockAxisFactor(rp3d::Vector3(m_pAttribute.rb_AngularAxisFactor.x, m_pAttribute.rb_AngularAxisFactor.y, m_pAttribute.rb_AngularAxisFactor.z));

		// shape
		if (m_pAttribute.Shape == "box")
		{
			m_pCollisionShape = ENGINE_PHYSICS::MakeSharedBoxCollisionShape(common, rp3d::Vector3(m_pAttribute.box_halfExtents.x, m_pAttribute.box_halfExtents.y, m_pAttribute.box_halfExtents.z));
		}
		else if (m_pAttribute.Shape == "sphere")
		{
			m_pCollisionShape = ENGINE_PHYSICS::MakeSharedSphereCollisionShape(common, m_pAttribute.sphere_radius);
		}
		else if (m_pAttribute.Shape == "capsule")
		{
			m_pCollisionShape = ENGINE_PHYSICS::MakeSharedCapsuleCollisionShape(common, m_pAttribute.capsule_radius, m_pAttribute.capsule_halfHeight);
		}

		// collider
		m_pCollider = ENGINE_PHYSICS::MakeSharedCollider(m_pRigidBody, m_pCollisionShape);
		Material& c_material = m_pCollider->getMaterial();
		c_material.setBounciness(m_pAttribute.c_Bounciness);
		c_material.setFrictionCoefficient(m_pAttribute.c_FrictionCoefficient);
		c_material.setMassDensity(m_pAttribute.c_MassDensity);
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

		lua.new_usertype<PhysicsAttributes>(
			"PhysicsAttributes",
			sol::call_constructor,
			sol::factories(
				[] {return PhysicsAttributes{}; }
			),
			"position", &PhysicsAttributes::position,
			"rotation", &PhysicsAttributes::rotation,
			"enable_gravity", &PhysicsAttributes::rb_EnableGravity,
			"type", &PhysicsAttributes::Type,
			"mass", &PhysicsAttributes::rb_Mass,
			"linear_damping", &PhysicsAttributes::rb_LinearDamping,
			"linear_axis_factor", &PhysicsAttributes::rb_LinearAxisFactor,
			"angular_damping", &PhysicsAttributes::rb_AngularDamping,
			"angular_axis_factor", &PhysicsAttributes::rb_AngularAxisFactor,
			"shape", &PhysicsAttributes::Shape,
			"box_halfExtents", &PhysicsAttributes::box_halfExtents,
			"sphere_radius", &PhysicsAttributes::sphere_radius,
			"capsule_radius", &PhysicsAttributes::capsule_radius,
			"capsule_halfHeight", &PhysicsAttributes::capsule_halfHeight,
			"bounciness", &PhysicsAttributes::c_Bounciness,
			"friction", &PhysicsAttributes::c_FrictionCoefficient,
			"mass_density", &PhysicsAttributes::c_MassDensity
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
			"linear_impulse",[](PhysicsComponent& pc){},
			"angular_impulse", [](PhysicsComponent& pc) {},
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
			}
		);
	}
}


