#include "TransformComponent.h"
#include<entt.hpp>

/*register a "Transform" type into lua*/
void ENGINE_CORE::ECS::TransformComponent::CreateLuaTransformBind(sol::state& lua)
{
	lua.new_usertype<TransformComponent>(
		"Transform",
		"type_id", &entt::type_hash<TransformComponent>::value,
		sol::call_constructor,
		sol::factories(
			[](glm::vec3 position, glm::vec3 scale, glm::vec3 rotation) {
				return TransformComponent{
					.position = position,
					.scale = scale,
					.rotation = rotation
				};
			},
			[](float x,float y,float z, float scale_x, float scale_y, float scale_z, float rotation_x, float rotation_y, float rotation_z) {
				return TransformComponent{
					.position = glm::vec3(x,y,z),
					.scale = glm::vec3(scale_x,scale_y,scale_z),
					.rotation = glm::vec3(rotation_x, rotation_y, rotation_z)
				};
			}
		),
		"position", &TransformComponent::position,
		"scale", &TransformComponent::scale,
		"rotation",&TransformComponent::rotation
	);
}
