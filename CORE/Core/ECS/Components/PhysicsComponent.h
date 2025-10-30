#pragma once
#include<Physics/RP3D_Wrappers.h>
#include<Physics/UserData.h>
#include<sol/sol.hpp>
#include<entt.hpp>
#include<glm/glm.hpp>
#include<glm/gtx/quaternion.hpp>
#include<glm/gtc/quaternion.hpp>

using namespace reactphysics3d;

namespace ENGINE_CORE::ECS {

	struct PhysicsAttributes
	{
		glm::vec3 position{ 0.0f };
		glm::vec3 rotation{ 0.0f };
		glm::vec3 scale{ 1.0f };

		// rigidbody
		BodyType rb_type{ BodyType::STATIC };
		bool rb_EnableGravity{ true };
		float rb_Mass{ 1.0f };
		float rb_LinearDamping{ 0.0f };
		float rb_AngularDamping{ 0.0f };
		glm::vec3 rb_LinearAxisFactor{ 1.0f, 1.0f, 1.0f };
		glm::vec3 rb_AngularAxisFactor{ 1.0f, 1.0f, 1.0f };

		// shape
		std::string shape{"box"};
		glm::vec3 box_halfExtents{ 1.0, 1.0, 1.0 };
		decimal sphere_radius{ 1.0 };
		decimal capsule_radius{ 1.0 };
		decimal capsule_halfHeight{ 1.0 };

		// collider
		float c_Bounciness{ 0.5f };
		float c_FrictionCoefficient{ 0.3f };
		float c_MassDensity{ 1.0f };

		ENGINE_PHYSICS::ObjectData objectData{};
	};

	class PhysicsComponent
	{
		rp3d::Transform m_previousTransform;
		rp3d::Transform m_currentTransform;

		PhysicsAttributes m_pAttribute;
		std::shared_ptr<RigidBody> m_pRigidBody;
		std::shared_ptr<CollisionShape> m_pCollisionShape;
		std::shared_ptr<Collider> m_pCollider;
		std::shared_ptr<ENGINE_PHYSICS::UserData> m_pUserData;

	public:
		PhysicsComponent();
		PhysicsComponent(const PhysicsAttributes& pAttributes);
		~PhysicsComponent() = default;

		void Init(std::shared_ptr<PhysicsCommon> common, std::shared_ptr<PhysicsWorld> world);

		void SetPreviousTransform(const rp3d::Transform& transform) { m_previousTransform = transform; }
		const rp3d::Transform& GetPreviousTransform() const { return m_previousTransform; }

		const rp3d::Transform& GetCurrentTransform() const { return m_currentTransform; }
		void SetCurrentTransform(const rp3d::Transform& transform) { m_currentTransform = transform; }

		void SetDebug(const bool debug) { m_pRigidBody->setIsDebugEnabled(debug); }

		rp3d::RigidBody* GetRigidBody() { return m_pRigidBody.get(); }
		PhysicsAttributes GetAttr() { return m_pAttribute; }

		ENGINE_PHYSICS::UserData* GetUserData(){ return m_pUserData.get(); }

		static void CreateLuaPhysicsBind(sol::state& lua, entt::registry& registry);
	};
}