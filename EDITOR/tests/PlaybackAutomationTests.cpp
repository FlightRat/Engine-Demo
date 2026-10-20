#include "../src/editor/automation/EditorOperations.h"
#include "../src/editor/automation/StateRules.h"
#include "../src/editor/automation/PhysicsEditing.h"
#include "../src/editor/automation/MaterialRules.h"
#include "../src/editor/automation/AssetImport.h"
#include "../src/editor/bridge/CommandQueue.h"
#include "../src/editor/scene/PlaybackController.h"
#include "../src/editor/scene/SceneManager.h"
#include "../src/editor/scene/SceneObject.h"
#include <Core/Resources/AssetManager.h>
#include <fstream>
#include <iostream>
using namespace ENGINE_EDITOR;
using namespace ENGINE_EDITOR::Automation;
static void Check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
void PlaybackAutomationTests(Contract& contract) {
    auto& scenes = SCENE_MANAGER(); auto& playback = PlaybackController::Get(); scenes.CleanUp();
    ENGINE_CORE::RESOURCES::AssetManager assets;
    EditorOperations ops(scenes, assets, contract, "playback-protocol");
    size_t requestId = 0;
    auto params = [&](const char* method, const std::string& text) {
        auto status = ops.Execute(Request("get_editor_status", Parse("{}")));
        auto args = Parse(text); auto& a = args.GetAllocator();
        args.AddMember("session_id", Text(ops.Session(), a), a);
        args.AddMember("request_id", Text("playback-" + std::to_string(++requestId), a), a);
        args.AddMember("expected_revision", ops.Revision(), a);
        if (std::string(method) != "create_scene" && std::string(method) != "import_asset" && !Str(status["data"], "current_scene_id").empty()) args.AddMember("scene_id", Copy(status["data"]["current_scene_id"], a), a);
        return args;
    };
    auto run = [&](const char* method, const std::string& text) { return ops.Execute(Request(method, params(method, text))); };
    auto success = [&](const Json& value) { Check(Str(value, "status") == "succeeded", Stringify(value).c_str()); };
    auto created = run("create_scene", R"({"name":"ProtocolScene"})"); success(created);
    const auto sceneId = Str(created["data"], "scene_id"), operationId = Str(created, "operation_id");
    const auto revisionBeforeIOFailure = ops.Revision();
    scenes.Scripts().SetHook([](const char*) { throw std::runtime_error("injected file failure"); });
    auto ioFailure = run("create_scene", R"({"name":"file-failure"})");
    Check(Str(ioFailure, "status") == "failed" && ops.Revision() == revisionBeforeIOFailure && scenes.GetCurrentScene()->GetName() == "ProtocolScene", "File failure changed scene/revision");
    scenes.Scripts().SetHook({});
    Check(!std::filesystem::exists(scenes.Scripts().ScriptPath("file-failure")), "File failure left a template");
    const auto path = scenes.Scripts().ScriptPath("ProtocolScene");
    const std::string userScript = std::string(SceneScriptFiles::Template()) + "-- user edit retained through history\n";
    { std::ofstream file(path, std::ios::binary); file << userScript; }
    success(run("undo_operation", "{\"operation_id\":\"" + operationId + "\"}"));
    Check(SceneScriptFiles::Read(path) == userScript && !scenes.GetCurrentScene(), "Undo removed script/current Scene not restored");
    success(run("redo_operation", "{\"operation_id\":\"" + operationId + "\"}"));
    Check(SceneScriptFiles::Read(path) == userScript, "Redo overwrote user script");
    auto status = ops.Execute(Request("get_editor_status", Parse("{}")));
    Check(Str(status["data"], "current_scene_id") != sceneId, "Redo reused public Scene identity");
    Check(Str(status["data"], "playback_state") == "editing" && !status["data"]["playing"].GetBool(), "Editing status contract");
    auto invalid = run("create_scene", R"({"name":"../invalid"})"); Check(Str(invalid["error"], "code") == "invalid_arguments", "Unsafe Scene name contract");
    CommandQueue queue(contract, ops.Session(), [&](const Request& r) { return ops.Execute(r); });
    auto original = params("create_object", R"({"name":"lamp","kind":"light"})");
    auto ticket = queue.Submit(Request("create_object", original)); queue.Process(); success(Parse(ticket->future.get()));
    const auto beforeFailedPlay = ops.Execute(Request("get_editor_status", Parse("{}")));
    { std::ofstream file(path, std::ios::binary); file << "return {on_start=function() error('start') end,on_update=function() end,on_stop=function() end}"; }
    Check(!playback.Play(), "Failed start unexpectedly succeeded"); ops.ObserveExternalChanges();
    const auto afterFailedPlay = ops.Execute(Request("get_editor_status", Parse("{}")));
    Check(afterFailedPlay["editor_revision"] == beforeFailedPlay["editor_revision"] && afterFailedPlay["data"]["undo_count"] == beforeFailedPlay["data"]["undo_count"], "Failed start invalidated edit history");
    { std::ofstream file(path, std::ios::binary); file << userScript; }
    auto editEntity = CreateEditableEntity(scenes.GetCurrentScene()->GetRegistry(), "physical");
    ApplyPhysics(editEntity.GetRegistry(), editEntity.GetEntity(), {}); NotifyManualEdit(); ops.ObserveExternalChanges();
    auto captureArgs = params("capture_viewport", "{}"); captureArgs.RemoveMember("request_id");
    auto importArgs = params("import_asset", "{}"); auto& ia = importArgs.GetAllocator();
    importArgs.AddMember("asset_type", "model", ia); importArgs.AddMember("asset_key", "pending-test", ia);
    const auto source = std::filesystem::absolute("tools/engine_mcp/tests/fixtures/imports/triangle.obj").u8string();
    importArgs.AddMember("path", Text(std::string(source.begin(), source.end()), ia), ia);
    Request pendingImport("import_asset", importArgs); success(ops.CheckImport(pendingImport));
    const auto oldRevision = ops.Revision();
    Check(playback.Play(), playback.LastError().c_str());
    ops.ObserveExternalChanges(); Check(ops.Revision() == oldRevision + 1, "Play revision missing");
    for (auto state : {PlaybackState::Playing, PlaybackState::Paused}) {
        if (state == PlaybackState::Paused) playback.Pause();
        status = ops.Execute(Request("get_editor_status", Parse("{}")));
        Check(Str(status["data"], "playback_state") == PlaybackStateName(state) && status["data"]["playing"].GetBool() && !status["data"]["write_enabled"].GetBool(), "Runtime status contract");
        auto snapshot = ops.Execute(Request("get_scene_state", Parse("{}")));
        Check(Str(snapshot["data"], "playback_state") == PlaybackStateName(state) && snapshot["data"]["total"].GetUint() == 2, "Scene query changed scope");
        for (const auto& item : {std::pair{"create_scene", R"({"name":"blocked"})"}, std::pair{"set_selection", R"({"entity_id":null})"}, std::pair{"undo_operation", R"({"operation_id":"old"})"}, std::pair{"redo_operation", R"({"operation_id":"old"})"}}) {
            const auto denied = run(item.first, item.second); Check(Str(denied["error"], "code") == "play_mode", "Runtime write accepted");
        }
        auto freshCapture = params("capture_viewport", "{}"); freshCapture.RemoveMember("request_id");
        auto captured = ops.CheckCapture(Request("capture_viewport", freshCapture)); Check(Str(captured["error"], "code") == "play_mode", "Runtime edit capture accepted");
        bool physicsDenied = false; try { ApplyPhysics(editEntity.GetRegistry(), editEntity.GetEntity(), {}); } catch (const Error& e) { physicsDenied = e.code == "play_mode"; }
        Check(physicsDenied, "Manual physics entry bypassed runtime guard");
        ENGINE_CORE::ECS::Material material, next; next.metallic = .2f;
        Check(!CommitMaterial(material, next, assets, true) && material.metallic == 1, "Manual material entry bypassed guard");
        Check(queue.Submit(Request("create_object", original)) == ticket, "Paused retry lost original receipt");
    }
    auto selection = ops.Execute(Request("get_selection", Parse("{}"))); Check(selection["data"]["entity_id"].IsNull(), "Runtime identity leaked as edit selection");
    auto failedImport = ops.CheckImport(pendingImport);
    Check(Str(failedImport["error"], "code") == "revision_conflict" || Str(failedImport["error"], "code") == "play_mode", "Pending import survived Play");
    const auto pausedRevision = ops.Revision(); playback.Play(); ops.ObserveExternalChanges();
    Check(ops.Revision() == pausedRevision + 1, "Resume revision missing");
    const auto runningRevision = ops.Revision(); playback.Tick(.1); ops.ObserveExternalChanges();
    Check(ops.Revision() == runningRevision, "Runtime frame advanced edit revision");
    playback.Stop(); ops.ObserveExternalChanges(); Check(ops.Revision() == runningRevision + 1, "Stop revision missing");
    status = ops.Execute(Request("get_editor_status", Parse("{}")));
    Check(status["data"]["undo_count"].GetUint64() == 0 && status["data"]["redo_count"].GetUint64() == 0, "Playback retained edit history");
    auto staleCapture = ops.CheckCapture(Request("capture_viewport", captureArgs)); Check(Str(staleCapture["error"], "code") == "revision_conflict", "Pending capture context remained valid");
    const auto beforeBatchTransitions = ops.Revision();
    Check(playback.Play(), "Cannot start multiple transition test"); playback.Pause(); playback.Play(); playback.Stop(); ops.ObserveExternalChanges();
    Check(ops.Revision() == beforeBatchTransitions + 4, "Multiple transitions were collapsed into one revision");
    queue.Stop(); scenes.CleanUp();
    std::cout << "Playback MCP state, edit guards, history, receipts and pending operation tests passed\n";
}
