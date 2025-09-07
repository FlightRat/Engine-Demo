#include "TransformComponent.h"
#include<entt.hpp>

void CORE::ECS::TransformComponent::CreateLuaTransformBind(sol::state& lua)
{
	lua.new_usertype<TransformComponent>(
		"Transform",
		"type_id", &entt::type_hash<TransformComponent>::value,
		sol::call_constructor,
		sol::factories(
			[](glm::vec3 position, glm::vec3 scale) {
				return TransformComponent{
					.position = position,
					.scale = scale
				};
			},
			[](float x,float y,float z, float scale_x, float scale_y, float scale_z) {
				return TransformComponent{
					.position = glm::vec3(x,y,z),
					.scale = glm::vec3(scale_x,scale_y,scale_z)
				};
			}
		),
		"position", [](TransformComponent& transform) {return std::make_tuple(transform.position.x, transform.position.y, transform.position.z); },
		"scale",[](TransformComponent& transform){return std::make_tuple(transform.scale.x, transform.scale.y, transform.scale.z); },
		"set_pos", [](TransformComponent& transform, float x, float y, float z) {transform.position = glm::vec3{ x,y,z }; },
		"set_scale", [](TransformComponent& transform, float x, float y, float z) {transform.scale = glm::vec3{ x,y,z }; }
	);
}
