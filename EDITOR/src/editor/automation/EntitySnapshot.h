#pragma once
#include "OperationTransaction.h"
#include <Core/ECS/Components/ScriptComponent.h>
#include <set>

namespace ENGINE_EDITOR::Automation {
inline EntityValues CaptureEntity(entt::registry& reg, entt::entity entity) {
    const std::set<entt::id_type> supported = {
        entt::type_hash<entt::entity>::value(), entt::type_hash<ENGINE_CORE::ECS::Identification>::value(),
        entt::type_hash<ENGINE_CORE::ECS::TransformComponent>::value(), entt::type_hash<ENGINE_CORE::ECS::MeshFilter>::value(),
        entt::type_hash<ENGINE_CORE::ECS::MeshRender>::value(), entt::type_hash<ENGINE_CORE::ECS::LightComponent>::value(),
        entt::type_hash<ENGINE_CORE::ECS::PhysicsComponent>::value(), entt::type_hash<ENGINE_CORE::ECS::CubeColliderComponent>::value(),
        entt::type_hash<ENGINE_CORE::ECS::SphereColliderComponent>::value()
    };
    Error unsupported("unsupported_component", "Entity contains components without a complete snapshot adapter");
    for (auto&& [id, storage] : reg.storage()) if (storage.contains(entity) && !supported.contains(id))
        unsupported.unsupportedComponents.emplace_back(storage.type().name());
    if (!unsupported.unsupportedComponents.empty()) throw unsupported;
    EntityValues out;
    if (auto* v = reg.try_get<ENGINE_CORE::ECS::Identification>(entity)) { out.identity = *v; out.identity->selected = false; }
    if (auto* v = reg.try_get<ENGINE_CORE::ECS::TransformComponent>(entity)) out.transform = *v;
    if (auto* v = reg.try_get<ENGINE_CORE::ECS::MeshFilter>(entity)) out.mesh = *v;
    if (auto* v = reg.try_get<ENGINE_CORE::ECS::MeshRender>(entity)) out.render = *v;
    if (auto* v = reg.try_get<ENGINE_CORE::ECS::LightComponent>(entity)) out.light = *v;
    out.physicsTouched = true;
    if (auto* v = reg.try_get<ENGINE_CORE::ECS::PhysicsComponent>(entity)) {
        out.physics = v->GetAttr(); out.physics->objectData.contactEntities.clear();
    }
    if (auto* v = reg.try_get<ENGINE_CORE::ECS::CubeColliderComponent>(entity)) out.cubeCollider = *v;
    if (auto* v = reg.try_get<ENGINE_CORE::ECS::SphereColliderComponent>(entity)) out.sphereCollider = *v;
    return out;
}
}
