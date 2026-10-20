#pragma once
#include "../ECS/Registry.h"
#include "../ECS/Components/PhysicsComponent.h"
#include "../ECS/Components/TransformComponent.h"
#include "../ECS/Components/Identification.h"
#include <cmath>
#include <stdexcept>

namespace ENGINE_CORE::Systems {
inline void ValidatePhysicsAttributes(const ECS::PhysicsAttributes& v) {
    auto positive = [](float x) { return std::isfinite(x) && x > 0; };
    auto unit = [](float x) { return std::isfinite(x) && x >= 0 && x <= 1; };
    if (v.rb_type != rp3d::BodyType::STATIC && v.rb_type != rp3d::BodyType::DYNAMIC && v.rb_type != rp3d::BodyType::KINEMATIC)
        throw std::invalid_argument("Unknown rigid body type");
    if (!positive(v.rb_Mass) || !positive(v.c_MassDensity) || !std::isfinite(v.c_FrictionCoefficient) || v.c_FrictionCoefficient < 0 ||
        !unit(v.c_Bounciness) || !unit(v.rb_LinearDamping) || !unit(v.rb_AngularDamping)) throw std::invalid_argument("Invalid physics material or mass");
    for (int i = 0; i < 3; ++i) if (!unit(v.rb_LinearAxisFactor[i]) || !unit(v.rb_AngularAxisFactor[i])) throw std::invalid_argument("Invalid physics axis factor");
    if (v.shape == "box") { for (int i = 0; i < 3; ++i) if (!positive(v.box_halfExtents[i])) throw std::invalid_argument("Invalid box size"); }
    else if (v.shape == "sphere") { if (!positive(v.sphere_radius)) throw std::invalid_argument("Invalid sphere radius"); }
    else if (v.shape == "capsule") { if (!positive(v.capsule_radius) || !positive(v.capsule_halfHeight)) throw std::invalid_argument("Invalid capsule size"); }
    else throw std::invalid_argument("Unknown collision shape");
}
inline void RefreshDebugGeometry(rp3d::PhysicsWorld& world) {
    auto& debug = world.getDebugRenderer(); debug.reset(); debug.computeDebugRenderingPrimitives(world);
}
inline void EnsurePhysicsResources(entt::registry& registry) {
    auto& ctx = registry.ctx();
    auto* common = ctx.find<std::shared_ptr<rp3d::PhysicsCommon>>();
    if (!common) common = &ctx.emplace<std::shared_ptr<rp3d::PhysicsCommon>>(std::make_shared<rp3d::PhysicsCommon>());
    if (!*common) *common = std::make_shared<rp3d::PhysicsCommon>();
    auto* world = ctx.find<std::shared_ptr<rp3d::PhysicsWorld>>();
    if (!world) ctx.emplace<std::shared_ptr<rp3d::PhysicsWorld>>(ENGINE_PHYSICS::MakeSharedPhysicsWorld(*common));
    else if (!*world) *world = ENGINE_PHYSICS::MakeSharedPhysicsWorld(*common);
}
inline void BuildPhysicsBody(entt::registry& registry, entt::entity entity, ECS::PhysicsAttributes value) {
    ValidatePhysicsAttributes(value);
    auto* t = registry.try_get<ECS::TransformComponent>(entity);
    if (!t) throw std::invalid_argument("Physics needs a Transform");
    for (int i = 0; i < 3; ++i) if (!std::isfinite(t->position[i]) || !std::isfinite(t->rotation_eular[i])) throw std::invalid_argument("Invalid physics pose");
    EnsurePhysicsResources(registry);
    value.position = t->position; value.rotation = t->rotation_eular;
    value.objectData.entityID = entt::to_integral(entity); value.objectData.bTrigger = value.c_Trigger;
    value.objectData.bCollider = true; value.objectData.contactEntities.clear();
    if (auto* id = registry.try_get<ECS::Identification>(entity)) { value.objectData.tag = id->name; value.objectData.group = id->group; }
    ECS::PhysicsComponent next(value);
    next.Init(registry.ctx().get<std::shared_ptr<rp3d::PhysicsCommon>>(), registry.ctx().get<std::shared_ptr<rp3d::PhysicsWorld>>());
    if (!next.GetRigidBody()) throw std::runtime_error("Could not create physics body");
    next.initialized = true;
    if (auto* current = registry.try_get<ECS::PhysicsComponent>(entity)) current->Swap(next);
    else registry.emplace<ECS::PhysicsComponent>(entity).Swap(next);
}
}
