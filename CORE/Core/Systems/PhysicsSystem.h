#pragma once
#include"../ECS/Registry.h"
#include <glm/gtx/quaternion.hpp> 
#include <Physics/RP3D_Wrappers.h>

namespace ENGINE_CORE::Systems {
	class PhysicsSystem
	{
	private:
		ENGINE_CORE::ECS::Registry& m_Registry;
	public:
		PhysicsSystem(ENGINE_CORE::ECS::Registry& registry);
		~PhysicsSystem() = default;
		void Update(entt::registry& registry, rp3d::decimal factor);
	};
}