#include "CameraLuaBindings.h"
#include "../ECS/Registry.h"
#include "InputManager.h"
#include <Rendering/Core/Camera3D.h>
#include <Logger/Logger.h>

using namespace ENGINE_RENDERING;

void ENGINE_CORE::Scripting::CameraBindings::CreateCameraBindings(sol::state& lua, ENGINE_CORE::ECS::Registry& registry)
{
	auto& camera = registry.GetContext<std::shared_ptr<Camera3D>>();
	if (!camera)
	{
		ENGINE_ERROR("Failed to bind the camera to Lua -- Not in the registry!");
		return;
	}

	lua.new_enum<Camera_Movement>(
		"Camera_Movement", {
			{"Cam_Forward",Camera_Movement::FORWARD},
			{"Cam_Backward",Camera_Movement::BACKWARD},
			{"Cam_Left",Camera_Movement::LEFT},
			{"Cam_Right",Camera_Movement::RIGHT}
		}
	);

	// register "Camera" into lua
	lua.new_usertype<Camera3D>(
		"Camera",
		sol::no_constructor,
		"get", [&] {return *camera; },
		"position", [&] {return camera->GetPosition(); },
		"set_position", [&](const glm::vec3 newPosition) { camera->SetPosition(newPosition); },
		"process_mouse", [&](const glm::vec2 offset) {camera->ProcessMouseMovement(offset.x, -offset.y, TRUE); },
		"process_scroll", [&](const float wheelY) {camera->ProcessMouseScroll(wheelY); },
		"process_key", [&](Camera_Movement direction) {camera->ProcessKeyboard(direction); }//todo
	);
}
