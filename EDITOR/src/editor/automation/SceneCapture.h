#pragma once
#include "CaptureCoordinator.h"
namespace ENGINE_RENDERING { class Camera3D; }
namespace ENGINE_EDITOR::Automation {
std::unique_ptr<CaptureSource> MakeSceneCapture(std::shared_ptr<ENGINE_RENDERING::Camera3D>);
}
