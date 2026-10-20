#pragma once
#include <Core/ECS/Entity.h>
#include <Core/ECS/Components/TransformComponent.h>
#include <Core/ECS/Components/LightComponent.h>
#include "../tools/ToolContext.h"
#include "../scene/SceneObject.h"
#include <Core/ECS/Components/Identification.h>
#include <cstdint>
#include <cmath>

namespace ENGINE_EDITOR::Automation {
// Main-thread-only notifications. They do not expose bridge state to engine libraries.
inline uint64_t ManualEditSerial = 0;
inline uint64_t SelectionResetSerial = 0;
inline std::weak_ptr<ENGINE_EDITOR::SceneObject> SelectionScene;
inline void NotifyManualEdit() { ++ManualEditSerial; }
inline void ResetSelection() {
    if (!TOOL_CTX().selectedEntity) { SelectionScene.reset(); return; }
    if (auto scene = SelectionScene.lock()) {
        auto& reg = scene->GetRegistry().GetRegistry();
        const auto e = TOOL_CTX().selectedEntity->GetEntity();
        if (reg.valid(e)) if (auto* id = reg.try_get<ENGINE_CORE::ECS::Identification>(e)) id->selected = false;
    }
    TOOL_CTX().selectedEntity.reset(); SelectionScene.reset(); ++SelectionResetSerial;
}
inline void SynchronizeSelection(const std::shared_ptr<ENGINE_EDITOR::SceneObject>& scene) {
    if (!TOOL_CTX().selectedEntity) return;
    if (!scene || SelectionScene.lock() != scene || !scene->GetRegistry().GetRegistry().valid(TOOL_CTX().selectedEntity->GetEntity())) ResetSelection();
}
inline void SelectEntity(const std::shared_ptr<ENGINE_EDITOR::SceneObject>& scene, entt::entity e) {
    SynchronizeSelection(scene);
    if (e == entt::null) { ResetSelection(); return; }
    if (!scene || !scene->GetRegistry().GetRegistry().valid(e)) throw std::invalid_argument("Invalid selection");
    if (SelectionScene.lock() == scene && TOOL_CTX().selectedEntity && TOOL_CTX().selectedEntity->GetEntity() == e) return;
    const auto before = SelectionResetSerial;
    ResetSelection();
    SelectionScene = scene;
    TOOL_CTX().selectedEntity = std::make_shared<ENGINE_CORE::ECS::Entity>(scene->GetRegistry(), e);
    if (auto* id = scene->GetRegistry().GetRegistry().try_get<ENGINE_CORE::ECS::Identification>(e)) id->selected = true;
    SelectionResetSerial = before + 1;
}
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
