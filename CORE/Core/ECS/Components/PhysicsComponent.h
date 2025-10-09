#pragma once
#include<Physics/RP3D_Wrappers.h>
#include<sol/sol.hpp>
#include<entt.hpp>
#include<glm/glm.hpp>

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

		std::shared_ptr<PhysicsCommon> m_pPhysicsCommon;
		std::shared_ptr<PhysicsWorld> m_pPhysicsWorld;
		std::shared_ptr<rp3d::RigidBody> m_pRigidBody;
		//std::shared_ptr<rp3d::Collider> m_pCollider;
		rp3d::Collider* m_pCollider;
		std::shared_ptr<rp3d::CollisionShape> m_pCollisionShape;

	public:
		PhysicsComponent(PhysicsAttributes pAttributes, std::shared_ptr<PhysicsCommon> pPhysicsCommon, std::shared_ptr<PhysicsWorld> pPhysicsWorld);
		~PhysicsComponent() = default;

		void Init();
		rp3d::RigidBody* GetRigidBody() { return m_pRigidBody.get(); }

		static void CreateLuaPhysicsBind(sol::state& lua, entt::registry& registry);
	};
}