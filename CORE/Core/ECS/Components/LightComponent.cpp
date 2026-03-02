#include "LightComponent.h"
#include <entt.hpp>

namespace ENGINE_CORE::ECS {
	void LightComponent::CreateLuaLightBind(sol::state& lua)
	{
		lua.new_usertype<LightComponent>(
			"Light",
			"type_id", &entt::type_hash<LightComponent>::value,
			sol::call_constructor,
			sol::factories(
				[](
					glm::vec3 diffuse, glm::vec3 specular, glm::vec3 ambient, 
					std::string type,
					glm::vec3 pos, float constant, float linear, float quadratic, glm::vec3 direction)
				{
					return LightComponent{
						.diffuse = diffuse,
						.specular = specular,
						.ambient = ambient,
						.type = type,
						.pos = pos,
						.constant = constant,
						.linear = linear,
						.quadratic = quadratic,
						.direction = direction
					};
				}
			),
			"diffuse", &LightComponent::diffuse,
			"specular", &LightComponent::specular,
			"ambient", &LightComponent::ambient,
			"direction", &LightComponent::direction,
			"position", &LightComponent::pos,
			"constant", &LightComponent::constant,
			"linear", &LightComponent::linear,
			"quadratic", &LightComponent::quadratic
		);
	}
}


