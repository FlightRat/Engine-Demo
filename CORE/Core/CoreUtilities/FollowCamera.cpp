#include "FollowCamera.h"
#include "../ECS/Components/TransformComponent.h"
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>

namespace ENGINE_CORE {
	FollowCamera::FollowCamera(ENGINE_RENDERING::Camera3D& camera, const ECS::Entity& entity, const glm::vec3& offset)
		:m_Camera{ camera }, m_Entity{ entity }, m_Offset{ offset }
	{
	}

    void FollowCamera::Update()
    {
        // 1. 获取玩家的 Transform 组件
        const auto& transform = m_Entity.GetComponent<ENGINE_CORE::ECS::TransformComponent>();

        // 2. 获取玩家的位置和旋转
        glm::vec3 targetPosition = transform.position;
        glm::quat targetRotation = transform.rotation_quat;

        // 3. 将本地偏移量 (m_Offset) 通过玩家的旋转转换到世界坐标系
        //    targetRotation * m_Offset 的意思是“将 m_Offset 这个向量按照 targetRotation 进行旋转”
        glm::vec3 worldOffset = targetRotation * m_Offset;

        // 4. 计算出相机最终应该在的位置
        glm::vec3 desiredPosition = targetPosition + worldOffset;

        // 5. 设置相机的位置
        m_Camera.SetPosition(desiredPosition);

        // 6. 确保相机始终朝向玩家
        //    为了获得更好的视角，我们可以让相机稍微朝向玩家的上方一点，而不是脚下
        glm::vec3 lookAtPoint = targetPosition + glm::vec3(0.0f, 1.0f, 0.0f); // 例如，向上偏移1个单位
        m_Camera.LookAt(lookAtPoint);
    }

	void FollowCamera::CreateLuaFollowCameraBind(sol::state& lua, ECS::Registry& registry)
	{
		auto& camera = registry.GetContext<std::shared_ptr<ENGINE_RENDERING::Camera3D>>();
		lua.new_usertype<FollowCamera>(
			"FollowCamera",
			sol::call_constructor,
			sol::factories([&](const ECS::Entity& entity, const glm::vec3& offset) {
				return FollowCamera(*camera, entity, offset);
				}),
			"update", &FollowCamera::Update,
			"set_entity",&FollowCamera::SetEntity
		);
	}
}