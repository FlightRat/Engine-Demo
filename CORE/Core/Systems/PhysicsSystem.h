#pragma once
#include"../ECS/Registry.h"
#include <glm/gtx/quaternion.hpp> 
#include <Physics/RP3D_Wrappers.h>

namespace ENGINE_CORE::Systems {
	class PhysicsSystem
	{
	public:
		PhysicsSystem();
		~PhysicsSystem() = default;
		void Update(ENGINE_CORE::ECS::Registry& runtimeRegistry, rp3d::decimal factor);
	};
}