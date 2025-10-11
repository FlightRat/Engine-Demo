#include "PhysicsComponent.h"
#include<Logger/Logger.h>

namespace CORE::ECS {
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
		m_pRigidBody = PHYSICS::MakeSharedRigidBody(world, transform);
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
		m_pRigidBody->setLinearLockAxisFactor(m_pAttribute.rb_LinearAxisFactor);
		m_pRigidBody->setAngularLockAxisFactor(m_pAttribute.rb_AngularAxisFactor);

		// shape
		if (m_pAttribute.Shape == "box")
		{
			m_pCollisionShape = PHYSICS::MakeSharedBoxCollisionShape(common, m_pAttribute.halfExtents);
		}
		else if (m_pAttribute.Shape == "sphere")
		{
			m_pCollisionShape = PHYSICS::MakeSharedSphereCollisionShape(common, m_pAttribute.radius);
		}

		// collider
		m_pCollider = PHYSICS::MakeSharedCollider(m_pRigidBody, m_pCollisionShape);
		Material& c_material = m_pCollider->getMaterial();
		c_material.setBounciness(m_pAttribute.c_Bounciness);
		c_material.setFrictionCoefficient(m_pAttribute.c_FrictionCoefficient);
		c_material.setMassDensity(m_pAttribute.c_MassDensity);
	}

	void PhysicsComponent::CreateLuaPhysicsBind(sol::state& lua, entt::registry& registry)
	{
	}
}


