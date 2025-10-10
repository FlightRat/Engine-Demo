#pragma once
#include<Physics/RP3D_Wrappers.h>
#include<sol/sol.hpp>
#include<entt.hpp>
#include<glm/glm.hpp>
#include<glm/gtx/quaternion.hpp>
#include<glm/gtc/quaternion.hpp>

using namespace reactphysics3d;

namespace CORE::ECS {

	struct PhysicsAttributes
	{
		BodyType Type{ BodyType::STATIC };
		std::string Shape{"box"};
		const Vector3 halfExtents{ 1.0, 1.0, 1.0 };
		const decimal radius{ 1.0 };

		glm::vec3 position{ 0.0f };
		glm::vec3 scale{ 1.0f };
		glm::vec3 rotation{ 0.0f };
	};

	class PhysicsComponent
	{
		PhysicsAttributes m_pAttribute;
		std::shared_ptr<RigidBody> m_pRigidBody;
		std::shared_ptr<CollisionShape> m_pCollisionShape;
		std::shared_ptr<Collider> m_pCollider;

	public:
		PhysicsComponent(PhysicsAttributes pAttributes);
		~PhysicsComponent() = default;

		void Init(std::shared_ptr<PhysicsCommon> common, std::shared_ptr<PhysicsWorld> world);
		rp3d::RigidBody* GetRigidBody() { return m_pRigidBody.get(); }

		static void CreateLuaPhysicsBind(sol::state& lua, entt::registry& registry);
	};
}