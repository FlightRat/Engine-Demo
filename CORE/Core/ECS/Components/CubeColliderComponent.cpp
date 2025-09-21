#include "CubeColliderComponent.h"
#include<entt.hpp>

void CORE::ECS::CubeColliderComponent::CreateLuaCubeColliderBind(sol::state& lua)
{
	lua.new_usertype<CubeColliderComponent>(
		"CubeCollider",
		"type_id", &entt::type_hash<CubeColliderComponent>::value,
		sol::call_constructor,
		sol::factories(
			[](float width, float height) {return CubeColliderComponent{ .width = width,.height = height}; },
			[](float width, float height, glm::vec3 offset) {return CubeColliderComponent{.width = width,.height = height,.offset = offset};}
		),
		"width", &CubeColliderComponent::width,
		"height", &CubeColliderComponent::height,
		"offset", &CubeColliderComponent::offset,
		"bColliding", &CubeColliderComponent::bColliding
	);
}
