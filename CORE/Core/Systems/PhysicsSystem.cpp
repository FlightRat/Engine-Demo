#include "PhysicsSystem.h"
#include "../ECS/Components/TransformComponent.h"
#include "../ECS/Components/PhysicsComponent.h"
#include <Logger/Logger.h>

using namespace CORE::ECS;
using namespace reactphysics3d;

namespace CORE::Systems {
	PhysicsSystem::PhysicsSystem(CORE::ECS::Registry& registry):m_Registry{registry}
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
			const Vector3& rb_pos = rb_transform.getPosition();
			transform.position.x = rb_pos.x;
			transform.position.y = rb_pos.y;
			transform.position.z = rb_pos.z;
		}
	}

}