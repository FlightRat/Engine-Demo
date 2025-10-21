#include "PhysicsSystem.h"
#include "../ECS/Components/TransformComponent.h"
#include "../ECS/Components/PhysicsComponent.h"
#include <Logger/Logger.h>

using namespace ENGINE_CORE::ECS;
using namespace reactphysics3d;

namespace ENGINE_CORE::Systems {
	PhysicsSystem::PhysicsSystem(ENGINE_CORE::ECS::Registry& registry):m_Registry{registry}
	{
	}

	void PhysicsSystem::Update(entt::registry& registry)
	{
		auto view = m_Registry.GetRegistry().view<TransformComponent, PhysicsComponent>();
		for (auto [entity, transform, physics] : view.each())
		{
			auto pRigidBody = physics.GetRigidBody();
			if (!pRigidBody)
				continue;
			const Transform& rb_transform = pRigidBody->getTransform();

			//position
			const Vector3& rb_pos = rb_transform.getPosition();
			transform.position = glm::vec3(rb_pos.x, rb_pos.y, rb_pos.z);
			//rotation
			const Quaternion& rp3d_quat = rb_transform.getOrientation();
			glm::quat quat(
				static_cast<float>(rp3d_quat.w),
				static_cast<float>(rp3d_quat.x),
				static_cast<float>(rp3d_quat.y),
				static_cast<float>(rp3d_quat.z)
			);
			transform.rotation = glm::degrees(glm::eulerAngles(quat));
		}
	}

}