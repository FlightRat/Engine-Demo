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
					glm::vec3 color, float intensity, std::string type,
					glm::vec3 pos, float constant, float linear, float quadratic, bool render,
					glm::vec3 direction, float half_width, float half_height)
				{
					return LightComponent{
						.color = color,
						.intensity=intensity,
						.type = type,
						.pos = pos,
						.constant = constant,
						.linear = linear,
						.quadratic = quadratic,
						.render = render,
						.direction = direction,
						.half_width = half_width,
						.half_height = half_height,
					};
				}
			),
			"color", &LightComponent::color,
			"intensity",&LightComponent::intensity,
			"direction", &LightComponent::direction,
			"position", &LightComponent::pos,
			"constant", &LightComponent::constant,
			"linear", &LightComponent::linear,
			"quadratic", &LightComponent::quadratic,
			"half_width", &LightComponent::half_width,
			"half_height", &LightComponent::half_height
		);
	}
}


