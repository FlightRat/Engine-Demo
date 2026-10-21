#include "../src/editor/automation/EditorOperations.h"
#include "../src/editor/automation/OperationHistory.h"
#include "../src/editor/automation/StateRules.h"
#include "../src/editor/scene/SceneManager.h"
#include "../src/editor/scene/SceneObject.h"
#include <fstream>
#include <Core/Resources/AssetManager.h>
#include <iostream>
using namespace ENGINE_EDITOR;
using namespace ENGINE_EDITOR::Automation;
namespace {
void Check(bool b, const char* message) { if (!b) throw std::runtime_error(message); }
void Success(const Json& r) { if (Str(r, "status") != "succeeded") throw std::runtime_error(Stringify(r)); }
Json Write(EditorOperations& ops, const char* method, const std::string& extra, const std::string& scene = {}) {
    static size_t next = 0;
    auto p = Parse(extra); auto& a = p.GetAllocator();
    p.AddMember("session_id", Text(ops.Session(), a), a); p.AddMember("request_id", Text("history-test-" + std::to_string(++next), a), a);
    p.AddMember("expected_revision", ops.Revision(), a);
    if (!scene.empty()) p.AddMember("scene_id", Text(scene, a), a);
    return ops.Execute(Request(method, p));
}
std::string Target(const std::string& id) { return "{\"operation_id\":\"" + id + "\"}"; }
}
void HistoryTests(Contract& contract) {
    {
        OperationHistory history(2, 4096);
        OperationRecord a; a.id = "first"; history.Commit(std::move(a));
        OperationRecord b; b.id = "second"; history.Commit(std::move(b));
        history.Traversed(false);
        OperationRecord huge; huge.id = "large"; huge.sceneName.assign(5000, 'x');
        bool rejected = false;
        try { history.Admit(huge); } catch (const Error& e) { rejected = e.code == "history_capacity_exceeded"; }
        Check(rejected && history.Top()->id == "first" && history.Top(true)->id == "second", "Failed admission destroyed the history branch");
        OperationRecord c; c.id = "third"; history.Commit(std::move(c));
        Check(!history.Top(true) && history.Undo().size() == 2, "New branch did not clear redo");
        OperationRecord d; d.id = "fourth"; history.Commit(std::move(d));
        Check(history.Undo().size() == 2 && history.Undo().front().id == "third", "History did not evict oldest entry");
    }
    auto& scenes = SCENE_MANAGER(); ResetSelection(); scenes.CleanUp();
    ENGINE_CORE::RESOURCES::AssetManager assets;
    bool fail = false;
    OperationHooks hooks; hooks.beforeApply = [&](size_t i) { if (fail && i == 1) throw std::runtime_error("history failure"); };
    EditorOperations ops(scenes, assets, contract, "history", hooks);
    auto created = Write(ops, "create_scene", R"({"name":"History"})"); Success(created);
    auto scene = Str(created["data"], "scene_id"); const auto sceneOperation = Str(created, "operation_id");
    auto batch = Write(ops, "apply_operations", R"({"operations":[{"method":"create_object","params":{"kind":"light","name":"warm","client_ref":"lamp"}},{"method":"set_light","params":{"target_ref":"lamp","intensity":42}}]})", scene); Success(batch);
    auto oldId = Str(batch["data"]["references"], "lamp"); const auto batchOperation = Str(batch, "operation_id");
    auto edit = Write(ops, "set_light", "{\"entity_id\":\"" + oldId + "\",\"intensity\":84}", scene); Success(edit);
    auto undoEdit = Write(ops, "undo_operation", Target(Str(edit, "operation_id")), scene); Success(undoEdit);
    auto invalid = Write(ops, "set_light", "{\"entity_id\":\"" + oldId + "\",\"direction\":[0,0,0]}", scene);
    Check(Str(invalid, "status") == "failed", "Invalid edit accepted");
    auto history = ops.Execute(Request("get_history", Parse(R"({"stack":"redo"})"))); Success(history);
    Check(history["data"]["entries"].Size() == 1, "Failed write discarded redo");
    auto undoBatch = Write(ops, "undo_operation", Target(batchOperation), scene); Success(undoBatch);
    auto undoScene = Write(ops, "undo_operation", Target(sceneOperation), scene); Success(undoScene);
    Check(!scenes.GetCurrentScene(), "Full history traversal did not return to no scene");
    auto redoScene = Write(ops, "redo_operation", Target(sceneOperation)); Success(redoScene);
    auto newScene = Str(redoScene["data"], "scene_id"); Check(newScene != scene, "Scene redo reused public identity");
    Check(Str(redoScene["data"]["scene_id_remap"], scene.c_str()) == newScene, "Scene identity mapping missing");
    fail = true;
    const auto revision = ops.Revision();
    auto failed = Write(ops, "redo_operation", Target(batchOperation), newScene);
    Check(Str(failed, "status") == "failed" && ops.Revision() == revision, "Failed redo committed revision");
    auto empty = ops.Execute(Request("get_scene_state", Parse("{}"))); Success(empty);
    Check(empty["data"]["entities"].Empty(), "Failed redo left a partial entity");
    fail = false;
    auto redoBatch = Write(ops, "redo_operation", Target(batchOperation), newScene); Success(redoBatch);
    auto newId = Str(redoBatch["data"]["entity_id_remap"], oldId.c_str());
    Check(!newId.empty() && newId != oldId, "Entity redo reused old public identity");
    auto stale = Write(ops, "set_light", "{\"entity_id\":\"" + oldId + "\",\"intensity\":1}", newScene);
    Check(Str(stale["error"], "code") == "stale_target", "Old public ID resolved after redo");
    auto redoEdit = Write(ops, "redo_operation", Target(Str(edit, "operation_id")), newScene); Success(redoEdit);
    auto state = ops.Execute(Request("get_scene_state", Parse("{}"))); Success(state);
    Check(state["data"]["entities"][0]["light"]["intensity"].GetFloat() == 84, "Historical edit missed restored logical entity");
    auto firstPage = ops.Execute(Request("get_history", Parse(R"({"page_size":1})"))); Success(firstPage);
    Check(firstPage["data"]["next_cursor"].IsString(), "History pagination missing");
    auto cursor = Parse("{}"); cursor.AddMember("cursor", Copy(firstPage["data"]["next_cursor"], cursor.GetAllocator()), cursor.GetAllocator());
    NotifyManualEdit(); ops.ObserveExternalChanges();
    auto expired = ops.Execute(Request("get_history", cursor));
    Check(Str(expired["error"], "code") == "revision_conflict", "History cursor survived manual edit");
    auto cleared = ops.Execute(Request("get_history", Parse("{}"))); Success(cleared);
    Check(cleared["data"]["undo_count"].GetUint64() == 0 && cleared["data"]["redo_count"].GetUint64() == 0, "Manual edit did not clear both history stacks");
    scenes.CleanUp();
    {
        const auto script = scenes.Scripts().ScriptPath("ConstructedHistory");
        { std::ofstream file(script); file << "return {on_build=function() local e=Entity('authored','');e:add_component(Transform(3,4,5,1,1,1,0,0,0)) end,on_start=function() end,on_update=function() end,on_stop=function() end}"; }
        EditorOperations constructed(scenes, assets, contract, "constructed-history");
        auto creation = Write(constructed, "create_scene", R"({"name":"ConstructedHistory"})"); Success(creation);
        const auto creationId = Str(creation["data"], "operation_id"); auto sceneId = Str(creation["data"], "scene_id");
        auto snapshot = constructed.Execute(Request("get_scene_state", Parse("{}"))); Success(snapshot);
        const auto entityId = Str(snapshot["data"]["entities"][0], "entity_id");
        auto mutation = Write(constructed, "set_transform", "{\"entity_id\":\"" + entityId + "\",\"position\":[8,9,10]}", sceneId); Success(mutation);
        const auto mutationId = Str(mutation["data"], "operation_id");
        Success(Write(constructed, "undo_operation", Target(mutationId), sceneId));
        Success(Write(constructed, "undo_operation", Target(creationId), sceneId));
        Check(!scenes.GetCurrentScene() && std::filesystem::is_regular_file(script), "Constructed Scene undo did not retain its script");
        const auto beforeFailure = constructed.Revision();
        { std::ofstream file(script); file << "return {on_build=function() Entity('partial','');error('redo construction failure') end,on_start=function() end,on_update=function() end,on_stop=function() end}"; }
        auto failedConstruction = Write(constructed, "redo_operation", Target(creationId));
        Check(Str(failedConstruction["error"], "code") == "execution_failed" && constructed.Revision() == beforeFailure && !scenes.GetCurrentScene(), "Failed construction redo changed history or current Scene");
        { std::ofstream file(script); file << "return {on_build=function() local e=Entity('authored','');e:add_component(Transform(6,4,5,1,1,1,0,0,0)) end,on_start=function() end,on_update=function() end,on_stop=function() end}"; }
        auto recreate = Write(constructed, "redo_operation", Target(creationId)); Success(recreate); sceneId = Str(recreate["data"], "scene_id");
        snapshot = constructed.Execute(Request("get_scene_state", Parse("{}"))); Success(snapshot);
        Check(snapshot["data"]["entities"][0]["transform"]["position"][0].GetFloat() == 6, "Scene redo ignored the current on_build contents");
        auto staleConstructed = Write(constructed, "set_transform", "{\"entity_id\":\"" + entityId + "\",\"position\":[1,2,3]}", sceneId);
        Check(Str(staleConstructed["error"], "code") == "stale_target", "Constructed entity public identity survived scene recreation");
        Success(Write(constructed, "redo_operation", Target(mutationId), sceneId));
        snapshot = constructed.Execute(Request("get_scene_state", Parse("{}"))); Success(snapshot);
        Check(snapshot["data"]["entities"].Size() == 1 && snapshot["data"]["entities"][0]["transform"]["position"][0].GetFloat() == 8, "History failed to rebind a constructed entity after scene redo");
        scenes.CleanUp();
    }
    std::cout << "Multi-level history tests passed\n";
}
