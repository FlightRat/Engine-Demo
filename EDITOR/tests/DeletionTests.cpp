#include "../src/editor/automation/EditorOperations.h"
#include "../src/editor/automation/PhysicsEditing.h"
#include "../src/editor/scene/SceneManager.h"
#include <Core/Resources/AssetManager.h>
#include <Core/ECS/Components/ScriptComponent.h>
#include <Core/ECS/Components/CubeColliderComponent.h>
#include <iostream>
using namespace ENGINE_EDITOR;
using namespace ENGINE_EDITOR::Automation;
using namespace ENGINE_CORE::ECS;
namespace {
void Check(bool b, const char* message) { if (!b) throw std::runtime_error(message); }
void Success(const Json& r) { if (Str(r, "status") != "succeeded") throw std::runtime_error(Stringify(r)); }
Json Write(EditorOperations& ops, const char* method, const std::string& extra, const std::string& scene) {
    static size_t next = 0; auto p = Parse(extra); auto& a = p.GetAllocator();
    p.AddMember("session_id", Text(ops.Session(), a), a); p.AddMember("request_id", Text("deletion-" + std::to_string(++next), a), a);
    p.AddMember("expected_revision", ops.Revision(), a); if (!scene.empty()) p.AddMember("scene_id", Text(scene, a), a);
    return ops.Execute(Request(method, p));
}
std::string Target(const std::string& id) { return "{\"entity_id\":\"" + id + "\"}"; }
std::string Operation(const Json& r) { return "{\"operation_id\":\"" + Str(r, "operation_id") + "\"}"; }
}
void DeletionTests(Contract& contract) {
    auto& scenes = SCENE_MANAGER(); ResetSelection(); scenes.CleanUp();
    ENGINE_CORE::RESOURCES::AssetManager assets;
    bool fail = false; OperationHooks hooks;
    hooks.beforeApply = [&](size_t i) { if (fail && i == 1) throw std::runtime_error("after deferred delete"); };
    EditorOperations ops(scenes, assets, contract, "deletion", hooks);
    auto created = Write(ops, "create_scene", R"({"name":"Deletion"})", ""); Success(created);
    auto sceneId = Str(created["data"], "scene_id"); auto scene = scenes.GetCurrentScene();
    auto root = CreateEditableEntity(scene->GetRegistry(), "root");
    auto child = CreateEditableEntity(scene->GetRegistry(), "child");
    child.GetComponent<Identification>().parent_id = static_cast<int32_t>(root.GetEntity());
    child.AddComponent<CubeColliderComponent>().width = 3;
    auto other = CreateEditableEntity(scene->GetRegistry(), "other");
    ApplyPhysics(root.GetRegistry(), root.GetEntity(), PhysicsAttributes{});
    ops.ObserveExternalChanges();
    auto state = ops.Execute(Request("get_scene_state", Parse("{}"))); Success(state);
    std::string rootId, childId;
    for (const auto& e : state["data"]["entities"].GetArray()) {
        if (Str(e, "name") == "root") rootId = Str(e, "entity_id");
        if (Str(e, "name") == "child") childId = Str(e, "entity_id");
    }
    auto refused = Write(ops, "delete_entity", Target(rootId), sceneId);
    Check(Str(refused["error"], "code") == "entity_has_children", "Deletion cascaded without consent");
    auto moved = Write(ops, "set_transform", "{\"entity_id\":\"" + rootId + "\",\"position\":[1,2,3]}", sceneId); Success(moved);
    SelectEntity(scene, root.GetEntity());
    auto remove = Write(ops, "delete_entity", "{\"entity_id\":\"" + rootId + "\",\"cascade\":true}", sceneId); Success(remove);
    auto& reg = scene->GetRegistry().GetRegistry(); auto world = reg.ctx().get<std::shared_ptr<rp3d::PhysicsWorld>>();
    Check(!reg.valid(root.GetEntity()) && !reg.valid(child.GetEntity()) && !TOOL_CTX().selectedEntity && world->getNbRigidBodies() == 0, "Deletion retained tree, selection or body");
    SelectEntity(scene, other.GetEntity());
    auto undo = Write(ops, "undo_operation", Operation(remove), sceneId); Success(undo);
    auto newRootId = Str(undo["data"]["entity_id_remap"], rootId.c_str());
    auto newChildId = Str(undo["data"]["entity_id_remap"], childId.c_str());
    Check(!newRootId.empty() && !newChildId.empty() && world->getNbRigidBodies() == 1, "Restoration identity/body missing");
    Check(TOOL_CTX().selectedEntity && TOOL_CTX().selectedEntity->GetEntity() == other.GetEntity(), "Undo overwrote unrelated selection");
    entt::entity restoredRoot = entt::null, restoredChild = entt::null;
    for (auto e : reg.view<Identification>()) {
        if (reg.get<Identification>(e).name == "root") restoredRoot = e;
        if (reg.get<Identification>(e).name == "child") restoredChild = e;
    }
    Check(reg.get<Identification>(restoredChild).parent_id == static_cast<int32_t>(restoredRoot) && reg.get<CubeColliderComponent>(restoredChild).width == 3, "Hierarchy or collider snapshot lost");
    Check(reg.get<PhysicsComponent>(restoredRoot).GetAttr().objectData.entityID == entt::to_integral(restoredRoot), "Physics user data contains old ID");
    auto undoMove = Write(ops, "undo_operation", Operation(moved), sceneId); Success(undoMove);
    Check(reg.get<TransformComponent>(restoredRoot).position == glm::vec3(0), "Earlier history missed restored target");
    auto redoMove = Write(ops, "redo_operation", Operation(moved), sceneId); Success(redoMove);
    const auto revision = ops.Revision();
    SelectEntity(scene, restoredRoot); fail = true;
    auto failed = Write(ops, "apply_operations", "{\"operations\":[{\"method\":\"delete_entity\",\"params\":{\"entity_id\":\"" + newRootId + "\",\"cascade\":true}},{\"method\":\"create_object\",\"params\":{\"kind\":\"light\",\"name\":\"fail\"}}]}", sceneId);
    Check(Str(failed, "status") == "failed" && ops.Revision() == revision && reg.valid(restoredRoot) && reg.valid(restoredChild) && TOOL_CTX().selectedEntity->GetEntity() == restoredRoot && world->getNbRigidBodies() == 1, "Failed batch changed original identity, selection or resources");
    fail = false;
    auto stillValid = Write(ops, "set_transform", "{\"entity_id\":\"" + newRootId + "\",\"position\":[4,5,6]}", sceneId); Success(stillValid);
    other.AddComponent<ScriptComponent>(); ops.ObserveExternalChanges();
    state = ops.Execute(Request("get_scene_state", Parse(R"({"name_filter":"other"})"))); Success(state);
    auto unsupported = Write(ops, "delete_entity", Target(Str(state["data"]["entities"][0], "entity_id")), sceneId);
    Check(Str(unsupported["error"], "code") == "unsupported_component" && unsupported["error"]["details"]["unsupported_components"].Size() == 1 && reg.valid(other.GetEntity()), "Unsupported script component was silently lost");
    ResetSelection(); scenes.CleanUp();
    std::cout << "Reversible deletion tests passed\n";
}
