#pragma once
#include "Protocol.h"
#include "StateRules.h"
#include <Core/ECS/Components/PhysicsComponent.h>
#include <cmath>
#include <Core/Systems/PhysicsResources.h>

namespace ENGINE_EDITOR::Automation {
using ENGINE_CORE::ECS::PhysicsAttributes;
using ENGINE_CORE::ECS::PhysicsComponent;
inline void ValidatePhysics(const PhysicsAttributes& v) {
    auto positive = [](float x) { return std::isfinite(x) && x > 0; };
    auto nonnegative = [](float x) { return std::isfinite(x) && x >= 0; };
    auto unit = [](float x) { return std::isfinite(x) && x >= 0 && x <= 1; };
    if (v.rb_type != rp3d::BodyType::STATIC && v.rb_type != rp3d::BodyType::DYNAMIC && v.rb_type != rp3d::BodyType::KINEMATIC)
        throw Error("invalid_arguments", "Unknown body_type");
    if (!positive(v.rb_Mass) || !positive(v.c_MassDensity) || !std::isfinite(v.c_FrictionCoefficient) || v.c_FrictionCoefficient < 0 ||
        !unit(v.c_Bounciness) || !nonnegative(v.rb_LinearDamping) || !nonnegative(v.rb_AngularDamping)) throw Error("invalid_arguments", "Invalid physics material or rigid body values");
    for (int i = 0; i < 3; ++i) if (!unit(v.rb_LinearAxisFactor[i]) || !unit(v.rb_AngularAxisFactor[i])) throw Error("invalid_arguments", "Physics axis factors must be in [0,1]");
    if (v.shape == "box") {
        for (int i = 0; i < 3; ++i) if (!positive(v.box_halfExtents[i])) throw Error("invalid_arguments", "Box half_extents must be positive");
    } else if (v.shape == "sphere") {
        if (!positive(v.sphere_radius)) throw Error("invalid_arguments", "Sphere radius must be positive");
    } else if (v.shape == "capsule") {
        if (!positive(v.capsule_radius) || !positive(v.capsule_halfHeight)) throw Error("invalid_arguments", "Capsule sizes must be positive");
    } else throw Error("invalid_arguments", "Unknown collider shape");
}
inline PhysicsAttributes UpdatedPhysics(PhysicsAttributes v, const Value& p) {
    auto vector = [](const Value& x) { return glm::vec3(x[0].GetFloat(), x[1].GetFloat(), x[2].GetFloat()); };
    const auto shape = Str(p, "shape", v.shape);
    if (shape != v.shape) {
        v.box_halfExtents = glm::vec3(1); v.sphere_radius = 1; v.capsule_radius = 1; v.capsule_halfHeight = 1; v.shape = shape;
    }
    for (auto i = p.MemberBegin(); i != p.MemberEnd(); ++i) {
        const std::string key = i->name.GetString();
        if ((key == "half_extents" && shape != "box") || (key == "radius" && shape == "box") || (key == "half_height" && shape != "capsule"))
            throw Error("invalid_arguments", "Field is not applicable to collider shape: " + key);
    }
    if (p.HasMember("body_type")) {
        auto t = Str(p, "body_type");
        if (t != "static" && t != "dynamic" && t != "kinematic") throw Error("invalid_arguments", "Unknown body_type");
        v.rb_type = PhysicsAttributes::StringToBodyType(t);
    }
    for (auto [name, value] : {std::pair{"mass", &v.rb_Mass}, {"mass_density", &v.c_MassDensity}, {"friction", &v.c_FrictionCoefficient},
         {"bounciness", &v.c_Bounciness}, {"linear_damping", &v.rb_LinearDamping}, {"angular_damping", &v.rb_AngularDamping}})
        if (p.HasMember(name)) *value = p[name].GetFloat();
    if (p.HasMember("enable_gravity")) v.rb_EnableGravity = p["enable_gravity"].GetBool();
    if (p.HasMember("trigger")) v.c_Trigger = p["trigger"].GetBool();
    if (p.HasMember("linear_axis_factor")) v.rb_LinearAxisFactor = vector(p["linear_axis_factor"]);
    if (p.HasMember("angular_axis_factor")) v.rb_AngularAxisFactor = vector(p["angular_axis_factor"]);
    if (p.HasMember("half_extents")) v.box_halfExtents = vector(p["half_extents"]);
    if (p.HasMember("radius")) {
        if (shape == "sphere") v.sphere_radius = p["radius"].GetFloat(); else v.capsule_radius = p["radius"].GetFloat();
    }
    if (p.HasMember("half_height")) v.capsule_halfHeight = p["half_height"].GetFloat();
    ValidatePhysics(v); return v;
}
inline void EnsurePhysicsContext(entt::registry& reg) { ENGINE_CORE::Systems::EnsurePhysicsResources(reg); }
inline void ApplyPhysics(entt::registry& reg, entt::entity e, PhysicsAttributes value);
inline void SyncPhysicsPose(entt::registry& reg, entt::entity e) {
    if (!PlaybackController::Get().CanEdit()) throw Error("play_mode", "Stop playback before editing physics");
    auto* physics = reg.try_get<PhysicsComponent>(e); auto* t = reg.try_get<ENGINE_CORE::ECS::TransformComponent>(e);
    if (!physics || !t) return;
    if (!physics->GetRigidBody()) { ApplyPhysics(reg, e, physics->GetAttr()); return; }
    auto& attr = physics->GetAttr(); attr.position = t->position; attr.rotation = t->rotation_eular;
    if (auto* body = physics->GetRigidBody()) {
        const auto& q = t->rotation_quat;
        rp3d::Transform pose(rp3d::Vector3(t->position.x, t->position.y, t->position.z), rp3d::Quaternion(q.x, q.y, q.z, q.w));
        body->setTransform(pose); physics->SetPreviousTransform(pose); physics->SetCurrentTransform(pose);
    }
}
inline void ApplyPhysics(entt::registry& reg, entt::entity e, PhysicsAttributes value) {
    if (!PlaybackController::Get().CanEdit()) throw Error("play_mode", "Stop playback before editing physics");
    ValidatePhysics(value);
    if (!reg.try_get<ENGINE_CORE::ECS::TransformComponent>(e)) throw Error("unsupported_target", "Physics needs a Transform");
    ENGINE_CORE::Systems::BuildPhysicsBody(reg, e, std::move(value));
    SyncPhysicsPose(reg, e);
}
inline Value PhysicsJson(const PhysicsAttributes& p, Allocator& a) {
    Value out(rapidjson::kObjectType);
    auto vec = [&](const glm::vec3& v) { Value x(rapidjson::kArrayType); for (int i = 0; i < 3; ++i) x.PushBack(v[i], a); return x; };
    out.AddMember("body_type", Text(PhysicsAttributes::BodyTypeToString(p.rb_type), a), a);
    out.AddMember("enable_gravity", p.rb_EnableGravity, a); out.AddMember("trigger", p.c_Trigger, a);
    for (auto [key, value] : {std::pair{"mass", p.rb_Mass}, {"mass_density", p.c_MassDensity}, {"friction", p.c_FrictionCoefficient},
         {"bounciness", p.c_Bounciness}, {"linear_damping", p.rb_LinearDamping}, {"angular_damping", p.rb_AngularDamping}}) out.AddMember(Text(key, a), Value(value), a);
    out.AddMember("linear_axis_factor", vec(p.rb_LinearAxisFactor), a); out.AddMember("angular_axis_factor", vec(p.rb_AngularAxisFactor), a);
    out.AddMember("shape", Text(p.shape, a), a);
    if (p.shape == "box") out.AddMember("half_extents", vec(p.box_halfExtents), a);
    else out.AddMember("radius", p.shape == "sphere" ? p.sphere_radius : p.capsule_radius, a);
    if (p.shape == "capsule") out.AddMember("half_height", p.capsule_halfHeight, a);
    return out;
}
}
