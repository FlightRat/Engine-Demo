#pragma once
#include<glm/glm.hpp>
#include<Rendering/Core/Camera3D.h>
#include"../ECS/Entity.h"

namespace ENGINE_CORE {
	class FollowCamera
	{
	private:
		ENGINE_RENDERING::Camera3D& m_Camera;
		ECS::Entity m_Entity;
		glm::vec3 m_Offset;
	public:
		FollowCamera(ENGINE_RENDERING::Camera3D& camera, const ECS::Entity& entity, const glm::vec3& offset);
		~FollowCamera() = default;

		void Update();

		inline void SetEntity(const ECS::Entity& entity) { m_Entity = entity; }
	
		static void CreateLuaFollowCameraBind(sol::state& lua, ECS::Registry& registry);
	};
}