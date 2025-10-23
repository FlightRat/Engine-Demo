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
				glm::quat quaternion = glm::quat(glm::radians(rotation));
				return TransformComponent{
					.position = position,
					.scale = scale,
					.rotation_eular = rotation,
					.rotation_quat = quaternion
				};
			},
			[](float x,float y,float z, float scale_x, float scale_y, float scale_z, float rotation_x, float rotation_y, float rotation_z) {
				glm::vec3 eulerAngle(rotation_x, rotation_y, rotation_z);
				glm::quat quaternion = glm::quat(glm::radians(eulerAngle));
				return TransformComponent{
					.position = glm::vec3(x,y,z),
					.scale = glm::vec3(scale_x,scale_y,scale_z),
					.rotation_eular = glm::vec3(rotation_x, rotation_y, rotation_z),
					.rotation_quat = quaternion
				};
			}
		),
		"position", &TransformComponent::position,
		"scale", &TransformComponent::scale,
		"rotation_eular",&TransformComponent::rotation_eular,
		"rotation_quat", &TransformComponent::rotation_quat
	);
}
