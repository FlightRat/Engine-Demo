#include "../src/editor/automation/EditorOperations.h"
#include "../src/editor/automation/StateRules.h"
#include "../src/editor/scene/SceneManager.h"
#include "../src/editor/scene/SceneObject.h"
#include <Core/Resources/AssetManager.h>
#include <Core/ECS/Components/Identification.h>
#include <Core/ECS/Components/PhysicsComponent.h>
#include <Logger/Logger.h>
#include <iostream>
using namespace ENGINE_EDITOR;
using namespace ENGINE_EDITOR::Automation;
namespace {
void Check(bool b, const char* msg) { if (!b) throw std::runtime_error(msg); }
void Success(const Json& d) { if (Str(d, "status") != "succeeded") throw std::runtime_error(Stringify(d)); }
Json Params(EditorOperations& ops, const std::string& extra, const std::string& scene = {}) {
    static unsigned next = 0;
    Json d = Parse(extra); auto& a = d.GetAllocator();
    d.AddMember("session_id", Text(ops.Session(), a), a); d.AddMember("request_id", Text("test-" + std::to_string(++next), a), a);
    d.AddMember("expected_revision", ops.Revision(), a);
    if (!scene.empty()) d.AddMember("scene_id", Text(scene, a), a);
    return d;
}
Json Run(EditorOperations& ops, const char* name, const std::string& extra, const std::string& scene = {}) {
    auto p = Params(ops, extra, scene); return ops.Execute(Request(name, p));
}
}
void OperationsTests(Contract& c) {
    ENGINE_INIT_LOGS(false, true);
    auto& scenes = SCENE_MANAGER(); scenes.CleanUp();
    ENGINE_CORE::RESOURCES::AssetManager assets;
    {
        EditorOperations ops(scenes, assets, c, "test");
        auto empty = ops.Execute(Request("get_scene_state", Parse("{}"))); Success(empty);
        Check(empty["data"]["entities"].Empty(), "No active scene should be empty");
        auto create = Run(ops, "create_scene", R"({"name":"Display"})"); Success(create);
        const auto scene = Str(create["data"], "scene_id");
        Check(scenes.ChangeSceneName("Display", "Renamed"), "Rename failed"); ops.ObserveExternalChanges();
        auto renamed = ops.Execute(Request("get_editor_status", Parse("{}")));
        Check(Str(renamed["data"], "current_scene_id") == scene, "Rename changed scene identity");
        Check(scenes.GetCurrentScene()->GetName() == "Renamed", "Active scene name not synchronized");
        Check(scenes.ChangeSceneName("Renamed", "Display"), "Rename back failed"); ops.ObserveExternalChanges();
        auto bad = Run(ops, "create_scene", R"({"name":"Display"})");
        Check(Str(bad["error"], "code") == "scene_name_conflict", "Scene conflict must preserve activation");
        auto missing = Run(ops, "create_object", R"({"name":"missing","kind":"mesh","model":"not-loaded"})", scene);
        Check(Str(missing["error"], "code") == "asset_not_found", "Missing asset must fail");
        Check(scenes.GetCurrentScene()->GetRegistry().GetRegistry().view<entt::entity>().size() == 0, "Missing asset left entity");
        auto batch = Run(ops, "apply_operations", R"({"operations":[{"method":"create_object","params":{"name":"warm","kind":"light","client_ref":"lamp"}},{"method":"set_light","params":{"target_ref":"lamp","position":[2,3,4],"color":[1,0.5,0.2],"intensity":10}}]})", scene); Success(batch);
        auto lampId = Str(batch["data"]["references"], "lamp");
        auto query = ops.Execute(Request("get_scene_state", Parse("{}"))); Success(query);
        Check(query["data"]["entities"].Size() == 1, "Batch should create one lamp");
        Check(query["data"]["entities"][0]["light"]["position"][0].GetFloat() == 2, "Light position must be independent");
        auto undo = Run(ops, "undo_operation", "{\"operation_id\":\"" + Str(batch, "operation_id") + "\"}", scene); Success(undo);
        auto stale = Run(ops, "set_light", "{\"entity_id\":\"" + lampId + "\",\"intensity\":1}", scene);
        Check(Str(stale["error"], "code") == "stale_target", "Deleted entity handle accepted");
        auto entity = CreateEditableEntity(scenes.GetCurrentScene()->GetRegistry(), "manual"); NotifyManualEdit(); ops.ObserveExternalChanges();
        query = ops.Execute(Request("get_scene_state", Parse("{}")));
        auto entityId = Str(query["data"]["entities"][0], "entity_id");
        auto extra = CreateEditableEntity(scenes.GetCurrentScene()->GetRegistry(), "second"); NotifyManualEdit(); ops.ObserveExternalChanges();
        auto firstPage = ops.Execute(Request("get_scene_state", Parse(R"({"page_size":1})"))); Success(firstPage);
        auto pageArgs = Parse("{}"); pageArgs.AddMember("cursor", Copy(firstPage["data"]["next_cursor"], pageArgs.GetAllocator()), pageArgs.GetAllocator());
        auto page2 = ops.Execute(Request("get_scene_state", pageArgs)); Success(page2);
        Check(page2["data"]["entities"].Size() == 1, "Second page missing");
        NotifyManualEdit(); ops.ObserveExternalChanges();
        auto oldPage = ops.Execute(Request("get_scene_state", pageArgs));
        Check(Str(oldPage["error"], "code") == "revision_conflict", "Old cursor accepted");
        extra.Kill(); NotifyManualEdit(); ops.ObserveExternalChanges();
        auto transform = Run(ops, "set_transform", "{\"entity_id\":\"" + entityId + "\",\"rotation_degrees\":[0,90,0]}", scene); Success(transform);
        auto& t = entity.GetComponent<ENGINE_CORE::ECS::TransformComponent>();
        Check(std::abs(t.rotation_quat.y - std::sqrt(.5f)) < 1e-5f, "Quaternion not synchronized");
        Check(t.position == glm::vec3(0) && t.scale == glm::vec3(1), "Partial rotation changed position or scale");
        t.position.x = 12; NotifyManualEdit(); ops.ObserveExternalChanges();
        auto conflict = Run(ops, "undo_operation", "{\"operation_id\":\"" + Str(transform, "operation_id") + "\"}", scene);
        Check(Str(conflict["error"], "code") == "history_conflict" && t.position.x == 12, "Undo overwrote manual edit");
        auto edited = Run(ops, "set_transform", "{\"entity_id\":\"" + entityId + "\",\"position\":[1,2,3]}", scene); Success(edited);
        entity.AddComponent<ENGINE_CORE::ECS::PhysicsComponent>(); ops.ObserveExternalChanges();
        auto dependency = Run(ops, "undo_operation", "{\"operation_id\":\"" + Str(edited, "operation_id") + "\"}", scene);
        Check(Str(dependency["error"], "code") == "history_conflict", "Dependency change did not invalidate undo");
        auto physics = Run(ops, "set_transform", "{\"entity_id\":\"" + entityId + "\",\"position\":[0,0,0]}", scene);
        Check(Str(physics["error"], "code") == "unsupported_target", "Physics transform was accepted");
        auto lamp = Run(ops, "create_object", R"({"name":"types","kind":"light"})", scene); Success(lamp);
        const auto typesId = Str(lamp["data"]["entity"], "entity_id");
        auto area = Run(ops, "set_light", "{\"entity_id\":\"" + typesId + "\",\"type\":\"area_light\",\"rotation_degrees\":[0,45,0]}", scene); Success(area);
        Check(area["data"]["entity"]["light"]["half_width"].GetFloat() == 1, "Area type default is invalid");
        auto badField = Run(ops, "set_light", "{\"entity_id\":\"" + typesId + "\",\"constant\":1}", scene);
        Check(Str(badField["error"], "code") == "invalid_arguments", "Type-incompatible field accepted");
        auto direction = Run(ops, "set_light", "{\"entity_id\":\"" + typesId + "\",\"type\":\"direction_light\",\"direction\":[0,-2,0]}", scene); Success(direction);
        Check(direction["data"]["entity"]["light"]["direction"][1].GetFloat() == -1, "Direction was not normalized");
        Check(direction["data"]["entity"]["light"]["intensity"] == area["data"]["entity"]["light"]["intensity"], "Type change lost common fields");
        auto invalidated = Params(ops, "{\"entity_id\":\"" + entityId + "\",\"position\":[1,2,3]}", scene);
        scenes.GetCurrentScene()->SetPlay(true);
        auto play = ops.Execute(Request("set_transform", invalidated));
        Check(Str(play["error"], "code") == "play_mode", "Play writes accepted");
        scenes.GetCurrentScene()->SetPlay(false); ops.ObserveExternalChanges();
        auto revision = ops.Execute(Request("set_transform", invalidated));
        Check(Str(revision["error"], "code") == "revision_conflict", "Old revision accepted");
        auto fresh = ops.Session();
        EditorOperations other(scenes, assets, c, "other"); Check(fresh != other.Session(), "Sessions reused");
    }
    scenes.CleanUp();
    {
        EditorOperations ops(scenes, assets, c, "scene-undo");
        auto create = Run(ops, "create_scene", R"({"name":"Empty"})"); Success(create);
        const auto oldScene = Str(create["data"], "scene_id");
        auto undo = Run(ops, "undo_operation", "{\"operation_id\":\"" + Str(create, "operation_id") + "\"}", oldScene); Success(undo);
        Check(!scenes.GetCurrentScene() && scenes.GetAllSceneNames().empty(), "Empty scene undo did not restore no active scene");
        auto recreated = Run(ops, "create_scene", R"({"name":"Empty"})"); Success(recreated);
        Check(Str(recreated["data"], "scene_id") != oldScene, "Recreated scene reused identity");
        auto second = Run(ops, "create_scene", R"({"name":"Second"})"); Success(second);
        auto jump = Run(ops, "undo_operation", "{\"operation_id\":\"" + Str(recreated, "operation_id") + "\"}", Str(second["data"], "scene_id"));
        Check(Str(jump["error"], "code") == "history_conflict", "Undo skipped latest record");
        auto restore = Run(ops, "undo_operation", "{\"operation_id\":\"" + Str(second, "operation_id") + "\"}", Str(second["data"], "scene_id")); Success(restore);
        Check(scenes.GetCurrentSceneName() == "Empty" && scenes.GetAllSceneNames().size() == 1, "Scene undo did not restore previous activation");
        auto twice = Run(ops, "undo_operation", "{\"operation_id\":\"" + Str(recreated, "operation_id") + "\"}", Str(recreated["data"], "scene_id"));
        Check(Str(twice["error"], "code") == "history_conflict", "Undo restored an older history slot");
    }
    scenes.CleanUp();
    {
        bool failApply = true, failRollback = false;
        OperationHooks hooks;
        hooks.beforeApply = [&](size_t i) { if (failApply && i == 1) throw std::runtime_error("injected apply failure"); };
        hooks.beforeRollback = [&] { if (failRollback) throw std::runtime_error("injected recovery failure"); };
        EditorOperations ops(scenes, assets, c, "faults", hooks);
        auto sceneResult = Run(ops, "create_scene", R"({"name":"Faults"})"); Success(sceneResult);
        auto scene = Str(sceneResult["data"], "scene_id");
        auto batch = Run(ops, "apply_operations", R"({"operations":[{"method":"create_object","params":{"kind":"light","name":"a"}},{"method":"create_object","params":{"kind":"light","name":"b"}}]})", scene);
        Check(Str(batch["error"], "code") == "execution_failed", "Injected failure not reported");
        Check(batch["error"]["details"]["failed_action_index"].GetUint64() == 1, "Failed action index missing");
        auto q = ops.Execute(Request("get_scene_state", Parse("{}"))); Check(q["data"]["entities"].Empty(), "Partial batch survived rollback");
        failApply = false;
        auto a = Run(ops, "create_object", R"({"kind":"light","name":"a"})", scene); Success(a);
        auto id = Str(a["data"]["entity"], "entity_id");
        failApply = true;
        auto original = a["data"]["entity"]["light"]["intensity"].GetFloat();
        auto failedEdit = Run(ops, "apply_operations", "{\"operations\":[{\"method\":\"set_light\",\"params\":{\"entity_id\":\"" + id + "\",\"intensity\":123}},{\"method\":\"create_object\",\"params\":{\"kind\":\"light\",\"name\":\"b\"}}]}", scene);
        Check(Str(failedEdit["error"], "code") == "execution_failed", "Second-stage failure was accepted");
        auto restored = ops.Execute(Request("get_scene_state", Parse("{}"))); Success(restored);
        Check(restored["data"]["entities"].Size() == 1 && restored["data"]["entities"][0]["light"]["intensity"].GetFloat() == original, "Rollback failed to restore existing entity");
        failRollback = true;
        auto undo = Run(ops, "undo_operation", "{\"operation_id\":\"" + Str(a, "operation_id") + "\"}", scene);
        Check(Str(undo["error"], "code") == "recovery_required", "Recovery failure not reported");
        auto blocked = Run(ops, "create_object", R"({"kind":"light","name":"blocked"})", scene);
        Check(Str(blocked["error"], "code") == "recovery_required", "Writes enabled after recovery failure");
        auto status = ops.Execute(Request("get_editor_status", Parse("{}"))); Success(status);
        Check(!status["data"]["write_enabled"].GetBool(), "Recovery status still advertises writes");
        auto readable = ops.Execute(Request("get_scene_state", Parse("{}"))); Success(readable);
    }
    scenes.CleanUp();
    std::cout << "Editor operation tests passed\n";
}
