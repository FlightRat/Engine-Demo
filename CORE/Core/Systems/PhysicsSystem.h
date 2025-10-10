#pragma once
#include"../ECS/Registry.h"
#include <glm/gtx/quaternion.hpp> 

namespace CORE::Systems {
	class PhysicsSystem
	{
	private:
		CORE::ECS::Registry& m_Registry;
	public:
		PhysicsSystem(CORE::ECS::Registry& registry);
		~PhysicsSystem() = default;
		void Update(entt::registry& registry);
	};
}