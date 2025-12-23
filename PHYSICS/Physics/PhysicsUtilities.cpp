#include "PhysicsUtilities.h"


namespace ENGINE_PHYSICS {
	static const std::map<std::string, rp3d::BodyType> String_Type_Map
	{
		{"STATIC",rp3d::BodyType::STATIC},
		{"DYNAMIC",rp3d::BodyType::DYNAMIC},
		{"KINEMATIC",rp3d::BodyType::KINEMATIC}
	};

	static const std::map<rp3d::BodyType, std::string> Type_String_Map
	{
		{rp3d::BodyType::STATIC,"STATIC"},
		{rp3d::BodyType::DYNAMIC,"DYNAMIC"},
		{rp3d::BodyType::KINEMATIC,"KINEMATIC"}
	};

	static const std::vector<std::string> Usable_collider
	{
		"box","sphere","capsule"
	};

	std::string RigidBody_type2string(rp3d::BodyType type)
	{
		auto rigidItr = Type_String_Map.find(type);
		if (rigidItr == Type_String_Map.end())
			return {};
		return rigidItr->second;
	}

	rp3d::BodyType RigidBody_string2type(const std::string type)
	{
		auto Itr = String_Type_Map.find(type);
		if (Itr == String_Type_Map.end())
			return BodyType::STATIC;
		return Itr->second;
	}

	const std::map<rp3d::BodyType, std::string>& GetRigidBodyStringMap()
	{
		return Type_String_Map;
	}

	std::vector<std::string> GetUsableCollider()
	{
		return Usable_collider;
	}

}

