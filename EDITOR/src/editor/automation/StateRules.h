#pragma once
#include <Core/ECS/Entity.h>
#include <Core/ECS/Components/TransformComponent.h>
#include <Core/ECS/Components/LightComponent.h>
#include "../tools/ToolContext.h"
#include <cstdint>
#include <cmath>

namespace ENGINE_EDITOR::Automation {
// Main-thread-only notifications. They do not expose bridge state to engine libraries.
inline uint64_t ManualEditSerial = 0;
inline uint64_t SelectionResetSerial = 0;
inline void NotifyManualEdit() { ++ManualEditSerial; }
inline void ResetSelection() { TOOL_CTX().selectedEntity.reset(); ++SelectionResetSerial; }
inline void SynchronizeRotation(ENGINE_CORE::ECS::TransformComponent& t) {
    t.rotation_quat = glm::quat(glm::radians(t.rotation_eular));
}
inline void SynchronizeRotation(ENGINE_CORE::ECS::LightComponent& l) {
    l.rotation_quat = glm::quat(glm::radians(l.rotation_eular));
}
inline void SetLightType(ENGINE_CORE::ECS::LightComponent& l, const std::string& type) {
    if (l.type == type) return;
    ENGINE_CORE::ECS::LightComponent next;
    next.type = type; next.half_width = next.half_height = 1;
    next.color = l.color; next.intensity = l.intensity; next.render = l.render;
    l = next;
}
inline bool SetLightDirection(ENGINE_CORE::ECS::LightComponent& l, const glm::vec3& direction) {
    const double length = std::hypot(double(direction.x), double(direction.y), double(direction.z));
    if (!std::isfinite(length) || length < 1e-12) return false;
    l.direction = {float(direction.x / length), float(direction.y / length), float(direction.z / length)};
    return true;
}
inline ENGINE_CORE::ECS::Entity CreateEditableEntity(ENGINE_CORE::ECS::Registry& registry, const std::string& name) {
    ENGINE_CORE::ECS::Entity e{registry, name, ""};
    e.AddComponent<ENGINE_CORE::ECS::TransformComponent>();
    return e;
}
}
