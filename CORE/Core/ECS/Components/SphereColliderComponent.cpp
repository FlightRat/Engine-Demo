#include "SphereColliderComponent.h"
#include<entt.hpp>

void CORE::ECS::SphereColliderComponent::CreateLuaSphereColliderBind(sol::state& lua)
{
	lua.new_usertype<SphereColliderComponent>(
		"SphereCollider",
		"type_id",&entt::type_hash<SphereColliderComponent>::value,
		sol::call_constructor,
		sol::factories(
			[](float radius) {return SphereColliderComponent{ .radius = radius }; },
			[](float radius, glm::vec3 offset) {return SphereColliderComponent{ .radius = radius,.offset = offset }; }
		),
		"radius",&SphereColliderComponent::radius,
		"offset",&SphereColliderComponent::offset,
		"bColliding", &SphereColliderComponent::bColliding
	);
}
