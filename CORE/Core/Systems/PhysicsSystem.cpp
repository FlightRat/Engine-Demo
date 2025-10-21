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

	void PhysicsSystem::Update(entt::registry& registry, rp3d::decimal factor)
	{
		auto view = m_Registry.GetRegistry().view<TransformComponent, PhysicsComponent>();
		for (auto [entity, transform, physics] : view.each())
		{
			// previous transform
			glm::vec3 pre_pos = transform.position;
			glm::vec3 pre_rot = transform.rotation;
			rp3d::Vector3 pre_rb_position(pre_pos.x, pre_pos.y, pre_pos.z);
			glm::vec3 pre_eulerDegree(pre_rot.x, pre_rot.y, pre_rot.z);
			glm::quat pre_gl_quat = glm::quat(glm::radians(pre_eulerDegree));
			rp3d::Quaternion pre_rb_rotation = rp3d::Quaternion(pre_gl_quat.x, pre_gl_quat.y, pre_gl_quat.z, pre_gl_quat.w);
			rp3d::Transform prevTransform(pre_rb_position, pre_rb_rotation);

			auto pRigidBody = physics.GetRigidBody();
			if (!pRigidBody)
				continue;
			const Transform& currTransform = pRigidBody->getTransform();

			Transform rb_transform = Transform::interpolateTransforms(prevTransform, currTransform, factor);

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