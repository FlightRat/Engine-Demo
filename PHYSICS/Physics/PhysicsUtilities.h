#pragma once
#include <cstdint>
#include <string>
#include <map>
#include <vector>
#include<reactphysics3d/reactphysics3d.h>

using namespace rp3d;

namespace ENGINE_PHYSICS
{
	std::string RigidBody_type2string(rp3d::BodyType eType);
	rp3d::BodyType RigidBody_string2type(const std::string sRigidType);
	const std::map<rp3d::BodyType, std::string>& GetRigidBodyStringMap();

	std::vector<std::string> GetUsableCollider();
}