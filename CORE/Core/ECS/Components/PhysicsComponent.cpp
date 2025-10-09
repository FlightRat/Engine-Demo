#include "PhysicsComponent.h"
#include<Logger/Logger.h>

namespace CORE::ECS {
	PhysicsComponent::PhysicsComponent(PhysicsAttributes pAttributes, std::shared_ptr<PhysicsCommon> pPhysicsCommon, std::shared_ptr<PhysicsWorld> pPhysicsWorld):
		m_pAttribute{pAttributes}, m_pPhysicsCommon{pPhysicsCommon}, m_pPhysicsWorld{pPhysicsWorld}
	{
	}

	void PhysicsComponent::Init()
	{
		if (!m_pPhysicsWorld)
		{
			ENGINE_ERROR("Failed to initialize the physics component - Physics world is nullptr!");
			return;
		}

		// position
		rp3d::Vector3 position(m_pAttribute.position.x, m_pAttribute.position.y, m_pAttribute.position.z);
		rp3d::Quaternion orientation = rp3d::Quaternion::identity();
		rp3d::Transform transform(position, orientation);

		// rigid body
		m_pRigidBody = PHYSICS::MakeSharedRigidBody(m_pPhysicsWorld, transform);
		if (!m_pRigidBody)
		{
			ENGINE_ERROR("Failed to create the rigid body!");
			return;
		}
		m_pRigidBody->setType(m_pAttribute.Type);

		// shape
		if (m_pAttribute.Shape == "box")
		{
			m_pCollisionShape = PHYSICS::MakeSharedBoxCollisionShape(m_pPhysicsCommon, m_pAttribute.halfExtents);
			rp3d::Transform localTransform = rp3d::Transform::identity();
			m_pCollider = m_pRigidBody->addCollider(m_pCollisionShape.get(), localTransform);
		}
		else if (m_pAttribute.Shape == "sphere")
		{
			//TODO
		}
	}

	void PhysicsComponent::CreateLuaPhysicsBind(sol::state& lua, entt::registry& registry)
	{
	}
}


