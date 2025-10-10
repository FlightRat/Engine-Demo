#include "PhysicsComponent.h"
#include<Logger/Logger.h>

namespace CORE::ECS {
	PhysicsComponent::PhysicsComponent(PhysicsAttributes pAttributes, std::shared_ptr<PhysicsCommon> pPhysicsCommon, std::shared_ptr<PhysicsWorld> pPhysicsWorld)
		:m_pAttribute{pAttributes},m_pPhysicsCommon{pPhysicsCommon},m_pPhysicsWorld{pPhysicsWorld}
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
		rp3d::Vector3 rb_position(m_pAttribute.position.x, m_pAttribute.position.y, m_pAttribute.position.z);
		// rotation
		glm::vec3 eulerDegree(m_pAttribute.rotation.x, m_pAttribute.rotation.y, m_pAttribute.rotation.z);
		glm::quat gl_quat = glm::quat(glm::radians(eulerDegree));
		rp3d::Quaternion rb_rotation = rp3d::Quaternion(gl_quat.x, gl_quat.y, gl_quat.z, gl_quat.w);
		rp3d::Transform transform(rb_position, rb_rotation);

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
			m_pCollider = PHYSICS::MakeSharedCollider(m_pRigidBody, m_pCollisionShape);
		}
		else if (m_pAttribute.Shape == "sphere")
		{
			m_pCollisionShape = PHYSICS::MakeSharedSphereCollisionShape(m_pPhysicsCommon, m_pAttribute.radius);
			m_pCollider = PHYSICS::MakeSharedCollider(m_pRigidBody, m_pCollisionShape);
		}
	}

	void PhysicsComponent::CreateLuaPhysicsBind(sol::state& lua, entt::registry& registry)
	{
	}
}


