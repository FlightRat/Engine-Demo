#include "../src/editor/automation/EditorOperations.h"
#include "../src/editor/automation/PhysicsEditing.h"
#include "../src/editor/scene/SceneManager.h"
#include <Core/Resources/AssetManager.h>
#include <iostream>
using namespace ENGINE_EDITOR;
using namespace ENGINE_EDITOR::Automation;
using namespace ENGINE_CORE::ECS;
namespace {
void Check(bool b, const char* text) { if (!b) throw std::runtime_error(text); }
void Success(const Json& r) { if (Str(r, "status") != "succeeded") throw std::runtime_error(Stringify(r)); }
Json Write(EditorOperations& ops, const char* method, const std::string& extra, const std::string& scene) {
    static size_t next = 0; auto p = Parse(extra); auto& a = p.GetAllocator();
    p.AddMember("session_id", Text(ops.Session(), a), a); p.AddMember("request_id", Text("physics-test-" + std::to_string(++next), a), a);
    p.AddMember("expected_revision", ops.Revision(), a); if (!scene.empty()) p.AddMember("scene_id", Text(scene, a), a);
    return ops.Execute(Request(method, p));
}
}
void PhysicsEditingTests(Contract& contract) {
    auto& scenes = SCENE_MANAGER(); ResetSelection(); scenes.CleanUp();
    ENGINE_CORE::RESOURCES::AssetManager assets;
    EditorOperations ops(scenes, assets, contract, "physics");
    auto created = Write(ops, "create_scene", R"({"name":"Physics"})", ""); Success(created);
    auto scene = Str(created["data"], "scene_id");
    auto entity = CreateEditableEntity(scenes.GetCurrentScene()->GetRegistry(), "body");
    ops.ObserveExternalChanges();
    auto state = ops.Execute(Request("get_scene_state", Parse("{}"))); Success(state);
    auto id = Str(state["data"]["entities"][0], "entity_id");
    const auto prefix = "{\"entity_id\":\"" + id + "\"";
    auto& reg = entity.GetRegistry();
    for (const auto& type : {"static", "dynamic", "kinematic"}) {
        auto add = Write(ops, "add_physics", prefix + ",\"body_type\":\"" + type + "\"}", scene); Success(add);
        auto world = reg.ctx().get<std::shared_ptr<rp3d::PhysicsWorld>>();
        Check(world->getNbRigidBodies() == 1, "Physics creation leaked a body");
        auto duplicate = Write(ops, "add_physics", prefix + "}", scene);
        Check(Str(duplicate["error"], "code") == "component_exists", "Duplicate physics accepted");
        for (const auto& shape : {"box", "sphere", "capsule"}) {
            auto set = Write(ops, "set_physics", prefix + ",\"shape\":\"" + shape + "\",\"mass\":3,\"enable_gravity\":false}", scene); Success(set);
            Check(world->getNbRigidBodies() == 1 && Str(set["data"]["entity"]["physics"], "shape") == shape, "Shape replacement leaked resources or returned wrong shape");
        }
        const auto revision = ops.Revision();
        auto invalid = Write(ops, "set_physics", prefix + ",\"shape\":\"sphere\",\"half_extents\":[1,1,1],\"mass\":5}", scene);
        Check(Str(invalid, "status") == "failed" && ops.Revision() == revision && entity.GetComponent<PhysicsComponent>().GetAttr().rb_Mass == 3, "Invalid physics partially applied");
        auto move = Write(ops, "set_transform", prefix + ",\"position\":[1,4,2],\"rotation_degrees\":[0,45,0]}", scene); Success(move);
        Check(entity.GetComponent<PhysicsComponent>().GetRigidBody()->getTransform().getPosition().y == 4, "Rigid body pose not updated");
        auto scale = Write(ops, "set_transform", prefix + ",\"position\":[7,8,9],\"scale\":[2,2,2]}", scene);
        Check(Str(scale["error"], "code") == "unsupported_target" && entity.GetComponent<TransformComponent>().position.y == 4, "Physical scale request partially applied");
        auto remove = Write(ops, "remove_physics", prefix + "}", scene); Success(remove);
        Check(world->getNbRigidBodies() == 0 && !reg.all_of<PhysicsComponent>(entity.GetEntity()), "Physics removal left a live body");
        const auto target = "{\"operation_id\":\"" + Str(remove, "operation_id") + "\"}";
        auto undo = Write(ops, "undo_operation", target, scene); Success(undo);
        Check(world->getNbRigidBodies() == 1 && entity.GetComponent<PhysicsComponent>().GetAttr().rb_Mass == 3, "Physics undo did not restore applied configuration");
        auto redo = Write(ops, "redo_operation", target, scene); Success(redo);
        Check(world->getNbRigidBodies() == 0, "Physics redo left a body");
    }
    auto missing = Write(ops, "remove_physics", prefix + "}", scene);
    Check(Str(missing["error"], "code") == "component_not_found", "Missing Physics removal accepted");
    auto batch = Write(ops, "apply_operations", "{\"operations\":[{\"method\":\"add_physics\",\"params\":{\"entity_id\":\"" + id + "\"}},{\"method\":\"set_physics\",\"params\":{\"entity_id\":\"" + id + "\",\"mass\":7}}]}", scene); Success(batch);
    auto undoBatch = Write(ops, "undo_operation", "{\"operation_id\":\"" + Str(batch, "operation_id") + "\"}", scene); Success(undoBatch);
    Check(!entity.HasComponent<PhysicsComponent>(), "Physics batch undo retained a component");
    scenes.CleanUp();
    std::cout << "Physics editing tests passed\n";
}
