#include "../src/editor/automation/EditorOperations.h"
#include "../src/editor/automation/StateRules.h"
#include "../src/editor/automation/MaterialRules.h"
#include "../src/editor/bridge/CommandQueue.h"
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
        Success(physics); Check(entity.GetComponent<ENGINE_CORE::ECS::PhysicsComponent>().GetRigidBody() != nullptr, "Physical pose edit did not initialize the body");
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
        Success(twice); Check(!scenes.GetCurrentScene(), "Multi-level undo did not restore the original empty context");
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

void MaterialSelectionTests(Contract& c) {
    auto& scenes = SCENE_MANAGER(); ResetSelection(); scenes.CleanUp();
    struct HeadlessTextureLifetime {
        PFNGLDELETETEXTURESPROC previous = glad_glDeleteTextures;
        HeadlessTextureLifetime() { glad_glDeleteTextures = +[](GLsizei, const GLuint*) {}; }
        ~HeadlessTextureLifetime() { glad_glDeleteTextures = previous; }
    } textureLifetime;
    ENGINE_CORE::RESOURCES::AssetManager assets;
    // CPU-only texture descriptors test type validation; no GPU image is claimed.
    auto& textureMap = const_cast<std::map<std::string, std::shared_ptr<ENGINE_RENDERING::Texture>>&>(assets.GetAllTextures());
    using Texture = ENGINE_RENDERING::Texture;
    textureMap.emplace("test2d", std::make_shared<Texture>(0, 1, 1, Texture::TextureType::PIXEL));
    textureMap.emplace("testCube", std::make_shared<Texture>(0, 1, 1, Texture::TextureType::ENVCUBEMAP));
    bool fail = false, failRollback = false;
    OperationHooks hooks; hooks.beforeApply = [&](size_t i) { if (fail && i == 1) throw std::runtime_error("material failure"); };
    hooks.beforeRollback = [&] { if (failRollback) throw std::runtime_error("material recovery failure"); };
    EditorOperations ops(scenes, assets, c, "materials", hooks);
    auto none = ops.Execute(Request("get_selection", Parse("{}"))); Success(none);
    Check(none["data"]["entity_id"].IsNull(), "No-scene selection must be null");
    auto created = Run(ops, "create_scene", R"({"name":"Materials"})"); Success(created);
    auto scene = Str(created["data"], "scene_id");
    auto a = CreateEditableEntity(scenes.GetCurrentScene()->GetRegistry(), "A");
    auto b = CreateEditableEntity(scenes.GetCurrentScene()->GetRegistry(), "B");
    auto& render = a.AddComponent<ENGINE_CORE::ECS::MeshRender>(); render.materials.resize(2);
    ops.ObserveExternalChanges();
    auto state = ops.Execute(Request("get_scene_state", Parse("{}"))); Success(state);
    std::string aid, bid;
    for (const auto& e : state["data"]["entities"].GetArray()) {
        if (Str(e, "name") == "A") { aid = Str(e, "entity_id"); Check(e["materials"][1]["material_index"] == 1, "Material index missing"); }
        else bid = Str(e, "entity_id");
    }
    SelectEntity(scenes.GetCurrentScene(), a.GetEntity());
    a.GetComponent<ENGINE_CORE::ECS::Identification>().name = "Renamed A";
    auto renamedSelection = ops.Execute(Request("get_selection", Parse("{}"))); Success(renamedSelection);
    Check(Str(renamedSelection["data"], "name") == "Renamed A", "Selection returned a cached name");
    a.GetComponent<ENGINE_CORE::ECS::Identification>().name = "A"; ResetSelection(); ops.ObserveExternalChanges();
    const auto prefix = "{\"entity_id\":\"" + aid + "\",\"material_index\":";
    const auto original = render.materials[0];
    auto changed = Run(ops, "set_material", prefix + "0,\"color\":[1,0,0],\"metallic\":0.7,\"roughness\":0.2,\"textures\":{\"normal\":null,\"albedo\":\"test2d\"}}", scene); Success(changed);
    Check(render.materials.size() == 2 && render.materials[1].color == glm::vec4(1), "Other material modified");
    Check(render.materials[0].color == glm::vec4(1,0,0,1) && !render.materials[0].m_useTexture, "Partial fields changed");
    const auto revision = ops.Revision();
    auto badIndex = Run(ops, "set_material", prefix + "2,\"metallic\":0}", scene);
    Check(Str(badIndex["error"], "code") == "invalid_arguments" && render.materials.size() == 2, "Material index expanded vector");
    auto badTexture = Run(ops, "set_material", prefix + "0,\"metallic\":0,\"textures\":{\"albedo\":\"missing\"}}", scene);
    Check(Str(badTexture["error"], "code") == "asset_not_found" && ops.Revision() == revision, "Missing texture changed state");
    auto cubeTexture = Run(ops, "set_material", prefix + "0,\"textures\":{\"albedo\":\"testCube\"}}", scene);
    Check(Str(cubeTexture["error"], "code") == "unsupported_target" && render.materials[0].m_textures.at("albedo") == "test2d", "Cube texture accepted or partial binding survived");
    auto unsupported = Run(ops, "set_material", "{\"entity_id\":\"" + bid + "\",\"material_index\":0,\"ao\":0}", scene);
    Check(Str(unsupported["error"], "code") == "unsupported_target", "Material accepted on empty object");
    CommandQueue queue(c, ops.Session(), [&](const Request& r) { return ops.Execute(r); });
    auto selectionParams = Params(ops, "{\"entity_id\":\"" + aid + "\"}", scene);
    auto selected = queue.Submit(Request("set_selection", selectionParams)); queue.Process();
    auto receipt = Parse(selected->future.get()); Success(receipt);
    Check(ops.Revision() == revision && a.GetComponent<ENGINE_CORE::ECS::Identification>().selected, "Selection modified content revision");
    const auto serial = SelectionResetSerial;
    auto same = Run(ops, "set_selection", "{\"entity_id\":\"" + aid + "\"}", scene); Success(same);
    Check(SelectionResetSerial == serial, "Same selection advanced serial");
    SelectEntity(scenes.GetCurrentScene(), b.GetEntity());
    auto retry = queue.Submit(Request("set_selection", selectionParams));
    Check(retry == selected && TOOL_CTX().selectedEntity->GetEntity() == b.GetEntity(), "Retry overwrote manual selection");
    auto clashParams = Parse(Stringify(selectionParams)); ENGINE_EDITOR::Automation::Set(clashParams, "entity_id", Text(bid, clashParams.GetAllocator()), clashParams.GetAllocator());
    auto clash = queue.Submit(Request("set_selection", clashParams));
    Check(Str(Parse(clash->future.get())["error"], "code") == "request_id_conflict", "Selection ID reuse accepted");
    auto badUndo = Run(ops, "undo_operation", "{\"operation_id\":\"" + Str(receipt, "operation_id") + "\"}", scene);
    Check(Str(badUndo["error"], "code") == "history_conflict", "Selection operation was undoable");
    auto undo = Run(ops, "undo_operation", "{\"operation_id\":\"" + Str(changed, "operation_id") + "\"}", scene); Success(undo);
    Check(MaterialEqual(render.materials[0], original), "Material undo did not restore all fields");
    Check(TOOL_CTX().selectedEntity && TOOL_CTX().selectedEntity->GetEntity() == b.GetEntity(), "Material undo cleared unrelated selection");
    auto cleared = Run(ops, "set_selection", R"({"entity_id":null})", scene); Success(cleared);
    Check(!TOOL_CTX().selectedEntity && !b.GetComponent<ENGINE_CORE::ECS::Identification>().selected, "Clear left selected flag");
    auto queuedParams = Params(ops, "{\"entity_id\":\"" + bid + "\"}", scene);
    auto cancelledSelection = queue.Submit(Request("set_selection", queuedParams));
    queue.Cancel(ops.Session(), Str(queuedParams, "request_id")); queue.Process();
    Check(Str(Parse(cancelledSelection->future.get()), "status") == "cancelled" && !TOOL_CTX().selectedEntity, "Cancelled selection executed");
    SelectEntity(scenes.GetCurrentScene(), b.GetEntity()); scenes.AddScene("Switched"); scenes.SetCurrentScene("Switched"); ops.ObserveExternalChanges();
    Check(!TOOL_CTX().selectedEntity && !b.GetComponent<ENGINE_CORE::ECS::Identification>().selected, "Scene switch left old selection flag");
    scenes.SetCurrentScene("Materials"); ops.ObserveExternalChanges();
    const auto noEditSerial = ManualEditSerial;
    Check(!CommitMaterial(render.materials[0], render.materials[0], assets, true) && ManualEditSerial == noEditSerial, "Browsing material caused edit");
    auto next = render.materials[0]; next.ao = .4f;
    Check(CommitMaterial(render.materials[0], next, assets, true) && ManualEditSerial == noEditSerial + 1, "Manual material change was not notified");
    ops.ObserveExternalChanges();
    auto beforeFailure = render.materials;
    fail = true;
    auto batch = Run(ops, "apply_operations", "{\"operations\":[{\"method\":\"set_material\",\"params\":{\"entity_id\":\"" + aid + "\",\"material_index\":0,\"metallic\":0}},{\"method\":\"set_material\",\"params\":{\"entity_id\":\"" + aid + "\",\"material_index\":1,\"roughness\":0}}]}", scene);
    Check(Str(batch["error"], "code") == "execution_failed" && MaterialEqual(render.materials[0], beforeFailure[0]) && MaterialEqual(render.materials[1], beforeFailure[1]), "Material rollback left partial edit");
    fail = false;
    auto staleSelect = Params(ops, "{\"entity_id\":\"" + aid + "\"}", scene);
    scenes.GetCurrentScene()->SetPlay(true);
    auto playing = ops.Execute(Request("set_selection", staleSelect));
    Check(Str(playing["error"], "code") == "play_mode", "Selection accepted during play");
    scenes.GetCurrentScene()->SetPlay(false); ops.ObserveExternalChanges();
    auto staleRevision = ops.Execute(Request("set_selection", staleSelect));
    Check(Str(staleRevision["error"], "code") == "revision_conflict", "Selection accepted old revision");
    auto staleSession = Params(ops, R"({"entity_id":null})", scene);
    ENGINE_EDITOR::Automation::Set(staleSession, "session_id", Text("old", staleSession.GetAllocator()), staleSession.GetAllocator());
    Check(Str(ops.Execute(Request("set_selection", staleSession))["error"], "code") == "stale_session", "Selection accepted old session");
    auto materialWrite = Run(ops, "set_material", prefix + "0,\"roughness\":0.9}", scene); Success(materialWrite);
    failRollback = true;
    auto recovery = Run(ops, "undo_operation", "{\"operation_id\":\"" + Str(materialWrite, "operation_id") + "\"}", scene);
    Check(Str(recovery["error"], "code") == "recovery_required", "Recovery failure not reported");
    auto blockedSelect = Run(ops, "set_selection", R"({"entity_id":null})", scene);
    Check(Str(blockedSelect["error"], "code") == "recovery_required", "Selection bypassed recovery write guard");
    SelectEntity(scenes.GetCurrentScene(), a.GetEntity()); a.Kill(); ops.ObserveExternalChanges();
    Check(!TOOL_CTX().selectedEntity, "Deleted selection was not cleared");
    auto captureParams = Parse("{\"session_id\":\"" + ops.Session() + "\",\"scene_id\":\"" + scene + "\",\"expected_revision\":" + std::to_string(ops.Revision()) + "}");
    auto unavailable = ops.Execute(Request("capture_viewport", captureParams));
    Check(Str(unavailable["error"], "code") == "capture_unavailable", "Headless host claimed successful capture");
    queue.Stop(); ResetSelection(); scenes.CleanUp();
    std::cout << "Material and selection tests passed\n";
}
