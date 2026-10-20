#include "CameraLuaBindings.h"
#include "../ECS/Registry.h"
#include <Rendering/Core/Camera3D.h>
#include <Logger/Logger.h>
#include "../CoreUtilities/GameplayContext.h"

using namespace ENGINE_RENDERING;

void ENGINE_CORE::Scripting::CameraBindings::CreateLuaCameraBind(sol::state& lua, ENGINE_CORE::ECS::Registry& registry)
{
	auto camera = registry.GetContext<std::shared_ptr<Camera3D>>();
    auto* context = registry.GetRegistry().ctx().find<std::shared_ptr<GameplayContext>>();
    auto runtime = context ? *context : nullptr;
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
		"get", [camera] {return *camera; },
		"position", [camera] {return camera->GetPosition(); },
		"set_position", [camera](const glm::vec3 newPosition) { camera->SetPosition(newPosition); },
		"process_mouse", [camera](const glm::vec2 offset) {camera->ProcessMouseMovement(offset.x, -offset.y, TRUE); },
		"process_scroll", [camera](const float wheelY) {camera->ProcessMouseScroll(wheelY); },
		"process_key", [camera, runtime](Camera_Movement direction) { if (runtime) camera->ProcessKeyboard(direction, runtime->clock.Delta()); else camera->ProcessKeyboard(direction); }//todo
	);
}
