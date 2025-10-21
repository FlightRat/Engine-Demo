#include "FollowCamera.h"
#include "../ECS/Components/TransformComponent.h"

namespace ENGINE_CORE {
	FollowCamera::FollowCamera(ENGINE_RENDERING::Camera3D& camera, const ECS::Entity& entity)
		:m_Camera{camera},m_Entity{entity}
	{
	}

	void FollowCamera::Update()
	{
		const auto& transform = m_Entity.GetComponent<ENGINE_CORE::ECS::TransformComponent>();
		glm::vec3 newPosition = transform.position;
		glm::vec3 oldPosition = m_Camera.GetPosition();
		m_Camera.SetPosition(glm::vec3(newPosition.x, oldPosition.y, newPosition.z));
	}

	void FollowCamera::CreateLuaFollowCameraBind(sol::state& lua, ECS::Registry& registry)
	{
		auto& camera = registry.GetContext<std::shared_ptr<ENGINE_RENDERING::Camera3D>>();
		lua.new_usertype<FollowCamera>(
			"FollowCamera",
			sol::call_constructor,
			sol::factories([&](const ECS::Entity& entity) {return FollowCamera(*camera, entity); }),
			"update", &FollowCamera::Update,
			"set_eneity",&FollowCamera::SetEntity
		);
	}
}