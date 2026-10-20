#pragma once
#include "Protocol.h"
#include <Core/ECS/Components/Identification.h>
#include <Core/ECS/Components/TransformComponent.h>
#include <Core/ECS/Components/MeshFilter.h>
#include <Core/ECS/Components/MeshRender.h>
#include <Core/ECS/Components/LightComponent.h>
#include <optional>
#include <Core/ECS/Components/PhysicsComponent.h>
#include <Core/ECS/Components/CubeColliderComponent.h>
#include <Core/ECS/Components/SphereColliderComponent.h>

namespace ENGINE_EDITOR::Automation {
struct ImportBundle;
// Value-only before/after images. Never retain an ECS handle in durable history.
struct EntityValues {
    std::optional<ENGINE_CORE::ECS::Identification> identity;
    std::optional<ENGINE_CORE::ECS::TransformComponent> transform;
    std::optional<ENGINE_CORE::ECS::MeshFilter> mesh;
    std::optional<ENGINE_CORE::ECS::MeshRender> render;
    std::optional<ENGINE_CORE::ECS::LightComponent> light;
    bool physicsTouched = false;
    std::optional<ENGINE_CORE::ECS::PhysicsAttributes> physics;
    std::optional<ENGINE_CORE::ECS::CubeColliderComponent> cubeCollider;
    std::optional<ENGINE_CORE::ECS::SphereColliderComponent> sphereCollider;
    std::string logicalParent;
    size_t Bytes() const {
        size_t bytes = sizeof(*this) + logicalParent.size();
        if (physics) bytes += physics->shape.size() + physics->objectData.tag.size() + physics->objectData.group.size();
        if (identity) bytes += identity->name.size() + identity->group.size();
        if (mesh) bytes += mesh->mesh.size();
        if (light) bytes += light->type.size();
        if (render) for (const auto& m : render->materials) {
            bytes += sizeof(m) + m.shadingModel.size();
            for (const auto& [slot, key] : m.m_textures) bytes += sizeof(std::pair<std::string, std::string>) + slot.size() + key.size();
        }
        return bytes;
    }
};
enum class EntityAction { Create, Transform, Light, Material, Physics, Delete };
struct EntityChange {
    EntityAction kind = EntityAction::Create;
    std::string logicalEntity;
    EntityValues before, after;
};
struct OperationRecord {
    std::string id, logicalScene, sceneName, previousScene;
    bool createsScene = false;
    std::shared_ptr<ImportBundle> assets;
    size_t retainedAssetBytes = 0;
    std::vector<EntityChange> changes;
    size_t Bytes() const {
        size_t bytes = sizeof(*this) + retainedAssetBytes + id.size() + logicalScene.size() + sceneName.size() + previousScene.size();
        for (const auto& c : changes) bytes += sizeof(c) + c.logicalEntity.size() + c.before.Bytes() + c.after.Bytes();
        return bytes;
    }
};
}
