#include "SceneObject.h"
#include "PlaybackController.h"
#include <Core/ECS/Components/Identification.h>
#include <Core/ECS/Components/ScriptComponent.h>
#include <Core/ECS/Components/TransformComponent.h>
#include <Core/ECS/Components/PhysicsComponent.h>
#include <Core/ECS/Components/MeshFilter.h>
#include <Core/ECS/Components/MeshRender.h>
#include <Core/ECS/Components/LightComponent.h>
#include <Core/ECS/Components/CubeColliderComponent.h>
#include <Core/ECS/Components/SphereColliderComponent.h>
#include <Core/Systems/PhysicsResources.h>
#include <Rendering/Core/Camera3D.h>
#include <unordered_map>
#include <unordered_set>
#include <stdexcept>
using namespace ENGINE_CORE::ECS;
namespace ENGINE_EDITOR {
SceneObject::SceneObject(const std::string& name) : m_sSceneName(name) {
    m_Registry.AddToContext<std::shared_ptr<ENGINE_RENDERING::Camera3D>>(std::make_shared<ENGINE_RENDERING::Camera3D>(
        glm::vec3(0, 10, 10), glm::vec3(0, 1, 0), -90.f, -45.f));
}
namespace {
template<typename T> void Copy(entt::registry& source, entt::registry& target, entt::entity from, entt::entity to) {
    if (auto* value = source.try_get<T>(from)) target.emplace<T>(to, *value);
}
}
void SceneObject::CopySceneToRuntime() {
    auto& source = m_Registry.GetRegistry();
    const std::unordered_set<entt::id_type> supported{
        entt::type_hash<entt::entity>::value(), entt::type_hash<Identification>::value(),
        entt::type_hash<TransformComponent>::value(), entt::type_hash<PhysicsComponent>::value(),
        entt::type_hash<MeshFilter>::value(), entt::type_hash<MeshRender>::value(), entt::type_hash<LightComponent>::value(),
        entt::type_hash<CubeColliderComponent>::value(), entt::type_hash<SphereColliderComponent>::value()};
    for (auto e : source.view<entt::entity>()) {
        const auto* identity = source.try_get<Identification>(e);
        const std::string label = identity ? identity->name : std::to_string(entt::to_integral(e));
        for (auto&& [id, storage] : source.storage()) if (storage.contains(e) && !supported.contains(id))
            throw std::runtime_error("Cannot clone entity " + label + ": unsupported component " + std::string(storage.type().name()));
        std::unordered_set<entt::entity> parents;
        auto parent = e;
        while (const auto* value = source.try_get<Identification>(parent)) {
            if (value->parent_id == -1) break;
            parent = static_cast<entt::entity>(value->parent_id);
            if (!source.valid(parent) || !parents.insert(parent).second) throw std::runtime_error("Cannot clone entity " + label + ": invalid hierarchy");
        }
        if (const auto* physics = source.try_get<PhysicsComponent>(e)) {
            if (!source.all_of<TransformComponent>(e) || (identity && identity->parent_id != -1)) throw std::runtime_error("Cannot clone physics entity " + label + ": unsupported pose");
            ENGINE_CORE::Systems::ValidatePhysicsAttributes(physics->GetAttr());
        }
    }
    m_RuntimeRegistry.Reset();
    try {
        auto& target = m_RuntimeRegistry.GetRegistry();
        std::unordered_map<entt::entity, entt::entity> ids;
        for (auto e : source.view<entt::entity>()) ids.emplace(e, target.create());
        ENGINE_CORE::Systems::EnsurePhysicsResources(target);
        const auto& camera = m_Registry.GetContext<std::shared_ptr<ENGINE_RENDERING::Camera3D>>();
        m_RuntimeRegistry.AddToContext<std::shared_ptr<ENGINE_RENDERING::Camera3D>>(std::make_shared<ENGINE_RENDERING::Camera3D>(*camera));
        for (const auto [from, to] : ids) {
            Copy<Identification>(source, target, from, to); Copy<TransformComponent>(source, target, from, to);
            Copy<MeshFilter>(source, target, from, to); Copy<MeshRender>(source, target, from, to); Copy<LightComponent>(source, target, from, to);
            Copy<CubeColliderComponent>(source, target, from, to); Copy<SphereColliderComponent>(source, target, from, to);
            if (auto* id = target.try_get<Identification>(to)) {
                id->entity_id = static_cast<int32_t>(entt::to_integral(to)); id->selected = false;
                if (id->parent_id != -1) id->parent_id = static_cast<int32_t>(entt::to_integral(ids.at(static_cast<entt::entity>(id->parent_id))));
            }
            if (const auto* physics = source.try_get<PhysicsComponent>(from)) ENGINE_CORE::Systems::BuildPhysicsBody(target, to, physics->GetAttr());
        }
    } catch (...) { m_RuntimeRegistry.Reset(); throw; }
}
bool SceneObject::CheckPlay() const { return PlaybackController::Get().IsSceneRunning(this); }
void SceneObject::ClearRuntimeScene() { m_RuntimeRegistry.Reset(); }
}
