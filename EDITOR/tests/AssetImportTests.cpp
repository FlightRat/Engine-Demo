#include "../src/editor/automation/AssetImportCoordinator.h"
#include "../src/editor/automation/StateRules.h"
#include "../src/editor/scene/SceneManager.h"
#include "../src/editor/scene/ProjectAssets.h"
#include <Core/ECS/Components/MeshFilter.h>
#include <Rendering/Essentials/TextureLoader.h>
#include <fstream>
#include <iostream>
using namespace ENGINE_EDITOR;
using namespace ENGINE_EDITOR::Automation;
using namespace ENGINE_CORE::RESOURCES;
using namespace ENGINE_RENDERING;
using ENGINE_UTIL::AssetType;
namespace {
void Check(bool b, const char* message) { if (!b) throw std::runtime_error(message); }
void Success(const Json& r) { if (Str(r, "status") != "succeeded") throw std::runtime_error(Stringify(r)); }
std::string Fixture(const char* name) {
    auto path = std::filesystem::absolute(std::filesystem::path("tools/engine_mcp/tests/fixtures/imports") / name).generic_u8string();
    return {reinterpret_cast<const char*>(path.data()), path.size()};
}
Json Parameters(EditorOperations& ops, const std::string& type, const std::string& key, const char* file) {
    static size_t next = 0; Json out(rapidjson::kObjectType); auto& a = out.GetAllocator();
    out.AddMember("session_id", Text(ops.Session(), a), a); out.AddMember("request_id", Text("import-test-" + std::to_string(++next), a), a);
    out.AddMember("expected_revision", ops.Revision(), a); out.AddMember("asset_type", Text(type, a), a); out.AddMember("asset_key", Text(key, a), a);
    if (type == "shader") { out.AddMember("vertex_path", Text(Fixture("vertex.vert"), a), a); out.AddMember("fragment_path", Text(Fixture(file), a), a); }
    else out.AddMember("path", Text(Fixture(file), a), a);
    return out;
}
Json Traverse(EditorOperations& ops, const char* method, const std::string& operation) {
    static size_t next = 0; Json args(rapidjson::kObjectType); auto& a = args.GetAllocator();
    args.AddMember("session_id", Text(ops.Session(), a), a); args.AddMember("request_id", Text("asset-history-" + std::to_string(++next), a), a);
    args.AddMember("expected_revision", ops.Revision(), a); args.AddMember("operation_id", Text(operation, a), a);
    return ops.Execute(Request(method, args));
}
Json Import(EditorOperations& ops, Json args) {
    Request request("import_asset", args); Success(ops.CheckImport(request));
    auto prepared = PrepareImport(args, std::make_shared<std::atomic_bool>(false));
    ImportUpload upload(std::move(prepared), ops.Assets());
    for (size_t i = 0; i < 10000; ++i) if (upload.Tick()) return ops.CommitImport(request, upload.Result());
    throw std::runtime_error("Import upload did not finish");
}
}
void AssetImportTests(Contract& contract, bool gpu) {
    auto& scenes = SCENE_MANAGER(); ResetSelection(); scenes.CleanUp();
    AssetManager assets; EditorOperations ops(scenes, assets, contract, "assets");
    {
        auto args = Parameters(ops, "model", "cpu", "triangle.obj");
        auto prepared = PrepareImport(args, std::make_shared<std::atomic_bool>(false));
        Check(prepared.model && !prepared.model->meshes.empty() && prepared.images.empty(), "CPU model parser required GL or fabricated dependencies");
        auto textured = PrepareImport(Parameters(ops, "model", "textured", "textured.obj"), std::make_shared<std::atomic_bool>(false));
        Check(textured.images.size() == 1 && textured.images.begin()->second.width == 2 && textured.images.begin()->second.height == 2, "CPU dependency/image preparation failed");
        for (const auto& input : {std::pair{"texture", "broken.png"}, {"model", "embedded.gltf"}, {"model", "missing.obj"}}) {
            bool rejected = false;
            try { PrepareImport(Parameters(ops, input.first, "invalid", input.second), std::make_shared<std::atomic_bool>(false)); } catch (const std::exception&) { rejected = true; }
            Check(rejected && !assets.CheckHasAsset("invalid", ImportType(input.first)), "Invalid dependency was silently published");
        }
        auto cancelled = std::make_shared<std::atomic_bool>(true); bool rejected = false;
        try { PrepareImport(args, cancelled); } catch (...) { rejected = true; }
        Check(rejected, "Preparation ignored cancellation");
    }
    {
        // A default Model owns no GL objects: exercise registry atomicity without a GL stub.
        LoadedAsset entry; entry.type = AssetType::MODEL; entry.key = "registry-test"; entry.payload = std::make_shared<Model>();
        std::vector<LoadedAsset> values; values.push_back(std::move(entry)); assets.Publish(values);
        auto collision = values; collision.push_back(values.front()); bool rejected = false;
        try { assets.Publish(collision); } catch (...) { rejected = true; }
        Check(rejected && assets.CheckHasAsset("registry-test", AssetType::MODEL), "Resource publication lost an existing registration");
        LoadedAsset fresh; fresh.type = AssetType::MODEL; fresh.key = "must-not-publish"; fresh.payload = std::make_shared<Model>();
        std::vector<LoadedAsset> conflicting; conflicting.push_back(std::move(fresh)); conflicting.push_back(values.front());
        rejected = false; try { assets.Publish(conflicting); } catch (...) { rejected = true; }
        Check(rejected && !assets.CheckHasAsset("must-not-publish", AssetType::MODEL), "A later collision left a partial resource publication");
        auto invalidWithdrawal = values; invalidWithdrawal.push_back(conflicting.front());
        rejected = false; try { assets.Withdraw(invalidWithdrawal); } catch (...) { rejected = true; }
        Check(rejected && assets.CheckHasAsset("registry-test", AssetType::MODEL), "A later withdrawal failure removed an earlier resource");
        assets.Withdraw(values); Check(!assets.CheckHasAsset("registry-test", AssetType::MODEL), "Resource withdrawal failed");
    }
    {
        auto args = Parameters(ops, "model", "global-history", "triangle.obj");
        Request request("import_asset", args);
        auto bundle = std::make_shared<ImportBundle>(); bundle->type = AssetType::MODEL; bundle->key = "global-history";
        LoadedAsset value; value.type = AssetType::MODEL; value.key = bundle->key; value.payload = std::make_shared<Model>();
        bundle->entries.push_back(std::move(value)); bundle->estimatedBytes = 1024;
        auto result = ops.CommitImport(request, bundle); Success(result); const auto operation = Str(result, "operation_id");
        Check(!scenes.GetCurrentScene(), "Global import created a scene");
        auto undo = Traverse(ops, "undo_operation", operation); Success(undo);
        Check(!assets.CheckHasAsset(bundle->key, AssetType::MODEL), "Import undo left its registration visible");
        auto redo = Traverse(ops, "redo_operation", operation); Success(redo);
        auto held = assets.GetModel(bundle->key);
        auto used = Traverse(ops, "undo_operation", operation);
        Check(Str(used["error"], "code") == "asset_in_use", "Context-held asset was withdrawn");
        held.reset(); Success(Traverse(ops, "undo_operation", operation));
    }
    {
        auto heldWorker = [](const Value& args, const std::shared_ptr<std::atomic_bool>& cancelled) {
            while (!cancelled->load()) std::this_thread::sleep_for(std::chrono::milliseconds(1));
            throw std::runtime_error("cancelled test preparation"); return PrepareImport(args, cancelled);
        };
        ImportHooks hooks; hooks.prepare = heldWorker;
        CommandQueue queue(contract, ops.Session(), [&](const Request& r) { return ops.Execute(r); });
        AssetImportCoordinator coordinator(queue, ops, hooks);
        auto args = Parameters(ops, "model", "pending", "triangle.obj");
        const auto requestId = Str(args, "request_id");
        auto ticket = queue.Submit(Request("import_asset", args)); queue.Process();
        Check(Str(queue.Status(ops.Session(), requestId), "status") == "running", "Deferred import did not stay running");
        Check(ticket == queue.Submit(Request("import_asset", args)), "Import retry did not deduplicate");
        auto busyArgs = Parameters(ops, "model", "second-pending", "triangle.obj");
        auto busy = queue.Submit(Request("import_asset", busyArgs));
        Check(Str(Parse(busy->future.get())["error"], "code") == "import_busy", "Concurrent import was admitted");
        auto query = queue.Submit(Request("get_history", Parse("{}"))); queue.Process();
        Success(Parse(query->future.get()));
        Check(Str(queue.Cancel(ops.Session(), requestId), "status") == "cancelled", "Preparing import could not be cancelled");
        coordinator.Tick(); coordinator.Stop(); queue.Stop();
        Check(!assets.CheckHasAsset("pending", AssetType::MODEL), "Cancelled import published a resource");
        for (const bool timeout : {false, true}) {
            ImportHooks test; test.prepare = heldWorker; if (timeout) test.timeout = std::chrono::milliseconds(1);
            CommandQueue q(contract, ops.Session(), [&](const Request& r) { return ops.Execute(r); });
            AssetImportCoordinator importer(q, ops, test);
            auto parameters = Parameters(ops, "model", timeout ? "expired" : "conflicted", "triangle.obj");
            auto pending = q.Submit(Request("import_asset", parameters)); q.Process();
            if (timeout) std::this_thread::sleep_for(std::chrono::milliseconds(3));
            else { NotifyManualEdit(); ops.ObserveExternalChanges(); }
            importer.Tick();
            auto result = Parse(pending->future.get());
            Check(Str(result["error"], "code") == (timeout ? "import_timeout" : "revision_conflict"), "Import did not revalidate timeout/revision");
            importer.Stop(); q.Stop();
        }
        {
            CommandQueue q(contract, ops.Session(), [&](const Request& r) { return ops.Execute(r); });
            q.SetImportHandler([](const auto&) {});
            auto parameters = Parameters(ops, "model", "commit-race", "triangle.obj");
            auto pending = q.Submit(Request("import_asset", parameters)); q.Process();
            Check(q.BeginImportCommit(pending), "Import commit could not claim its ticket");
            Check(Str(q.Cancel(ops.Session(), Str(parameters, "request_id")), "status") == "running", "Commit was falsely reported cancelled");
            q.Stop(); Check(Str(Parse(pending->future.get()), "status") == "cancelled", "Shutdown left a deferred import promise unresolved");
        }
    }
    if (gpu) {
        for (const auto& input : {std::pair{"model", "textured.obj"}, {"texture", "color.png"}, {"shader", "fragment.frag"}, {"music", "tone.wav"}, {"soundfx", "tone.wav"}}) {
            const auto key = std::string("gpu-") + input.first;
            auto result = Import(ops, Parameters(ops, input.first, key, input.second)); Success(result);
            Check(assets.CheckHasAsset(key, ImportType(input.first)), "Backend import did not publish the requested type");
            if (std::string(input.first) == "texture") {
                const auto texture = assets.GetTexture(key); GLint filter = 0;
                glGetTextureParameteriv(texture->GetID(), GL_TEXTURE_MAG_FILTER, &filter);
                Check(filter == GL_LINEAR && texture->GetType() == Texture::TextureType::BLENDED, "Blended texture sampling option was lost");
            }
            const auto operation = Str(result, "operation_id");
            Success(Traverse(ops, "undo_operation", operation));
            Check(!assets.CheckHasAsset(key, ImportType(input.first)), "Backend import undo did not hide the resource");
            Success(Traverse(ops, "redo_operation", operation));
        }
        auto pixelArgs = Parameters(ops, "texture", "pixel-texture", "color.png");
        pixelArgs.AddMember("pixel_art", true, pixelArgs.GetAllocator());
        auto pixel = Import(ops, std::move(pixelArgs)); Success(pixel);
        { auto texture = assets.GetTexture("pixel-texture"); GLint filter = 0; glGetTextureParameteriv(texture->GetID(), GL_TEXTURE_MAG_FILTER, &filter);
          Check(filter == GL_NEAREST && texture->GetType() == Texture::TextureType::PIXEL, "Pixel texture sampling option was lost"); }
        Success(Traverse(ops, "undo_operation", Str(pixel, "operation_id")));
        bool shaderRejected = false;
        try { Import(ops, Parameters(ops, "shader", "broken-shader", "broken.frag")); }
        catch (const std::exception& e) { shaderRejected = std::string(e.what()).find("compilation failed") != std::string::npos; }
        Check(shaderRejected && !assets.CheckHasAsset("broken-shader", AssetType::SHADER), "Invalid Shader was published without a compile diagnostic");
        shaderRejected = false;
        try { Import(ops, Parameters(ops, "shader", "link-failure", "link-error.frag")); }
        catch (const std::exception& e) { shaderRejected = std::string(e.what()).find("linking failed") != std::string::npos; }
        Check(shaderRejected && !assets.CheckHasAsset("link-failure", AssetType::SHADER), "Shader link error did not leave a clean registry");
        auto sound = Import(ops, Parameters(ops, "soundfx", "playing-sound", "tone.wav")); Success(sound);
        int channel = -1;
        { auto effect = assets.GetSoundFx("playing-sound"); channel = Mix_PlayChannel(-1, effect->GetSoundFxPtr(), -1); }
        Check(channel >= 0, "Test effect could not start playing");
        auto playingSound = Traverse(ops, "undo_operation", Str(sound, "operation_id"));
        Check(Str(playingSound["error"], "code") == "asset_in_use", "A playing sound effect was withdrawn");
        Mix_HaltChannel(channel); Success(Traverse(ops, "undo_operation", Str(sound, "operation_id")));

        const auto fixturePath = Fixture("triangle.obj");
        const auto root = std::filesystem::path(reinterpret_cast<const char8_t*>(fixturePath.c_str())).parent_path();
        const auto name = "retained-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".obj";
        const auto file = root / name; std::filesystem::copy_file(root / "triangle.obj", file);
        try {
            auto args = Parameters(ops, "model", "retained-model", name.c_str());
            auto result = Import(ops, std::move(args)); Success(result);
            std::filesystem::remove(file);
            const auto operation = Str(result, "operation_id"); Success(Traverse(ops, "undo_operation", operation)); Success(Traverse(ops, "redo_operation", operation));
            Check(assets.GetModel("retained-model")->GetMeshCount() > 0, "Redo reopened a removed external source");
        } catch (...) { std::filesystem::remove(file); throw; }
        const auto audioName = "retained-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".wav";
        const auto audioFile = root / audioName; std::filesystem::copy_file(root / "tone.wav", audioFile);
        try {
            auto result = Import(ops, Parameters(ops, "music", "retained-music", audioName.c_str())); Success(result);
            const auto operation = Str(result, "operation_id"); std::filesystem::remove(audioFile);
            Success(Traverse(ops, "undo_operation", operation)); Success(Traverse(ops, "redo_operation", operation));
            auto music = assets.GetMusic("retained-music");
            Check(Mix_PlayMusic(music->GetMusicPtr(), -1) == 0, "Retained music source was not valid after redo");
            ENGINE_SOUNDS::Music::ActiveMusic = music->GetMusicPtr(); music.reset();
            auto playing = Traverse(ops, "undo_operation", operation);
            Check(Str(playing["error"], "code") == "asset_in_use", "Playing music was withdrawn");
            Mix_HaltMusic(); ENGINE_SOUNDS::Music::ActiveMusic = nullptr;
            Success(Traverse(ops, "undo_operation", operation));
        } catch (...) { std::filesystem::remove(audioFile); throw; }
        // A preloaded matching dependency is reused and survives model undo.
        auto preparation = PrepareImport(Parameters(ops, "model", "reuse", "textured.obj"), std::make_shared<std::atomic_bool>(false));
        const auto dependency = preparation.images.begin()->first;
        Success(Import(ops, Parameters(ops, "texture", dependency, "color.png")));
        auto reused = Import(ops, Parameters(ops, "model", "reuse", "textured.obj")); Success(reused);
        Check(reused["data"]["reused"].Size() == 1, "Matching verified dependency was not reused");
        Success(Traverse(ops, "undo_operation", Str(reused, "operation_id")));
        Check(assets.CheckHasAsset(dependency, AssetType::TEXTURE), "Model undo removed a reused texture");
        auto conflictPreparation = PrepareImport(Parameters(ops, "model", "conflict", "textured.obj"), std::make_shared<std::atomic_bool>(false));
        const auto conflictKey = conflictPreparation.images.begin()->first;
        Success(Import(ops, Parameters(ops, "texture", conflictKey, "other.png")));
        bool conflictRejected = false;
        try { Import(ops, Parameters(ops, "model", "conflict", "textured.obj")); }
        catch (const Error& e) { conflictRejected = e.code == "asset_name_conflict"; }
        Check(conflictRejected && !assets.CheckHasAsset("conflict", AssetType::MODEL) && assets.CheckHasAsset(conflictKey, AssetType::TEXTURE), "Conflicting texture provenance was reused or partially published");
        std::weak_ptr<const std::vector<unsigned char>> retainedSource = assets.Provenance(AssetType::MUSIC, "gpu-music")->source;
        Check(assets.ChangeAssetName("gpu-music", "manual-music", AssetType::MUSIC), "Manual resource rename failed");
        ops.ObserveExternalChanges();
        auto history = ops.Execute(Request("get_history", Parse("{}"))); Success(history);
        Check(history["data"]["undo_count"].GetUint64() == 0 && history["data"]["redo_count"].GetUint64() == 0, "Manual resource change left history valid");
        Check(assets.Provenance(AssetType::MUSIC, "manual-music") != nullptr, "Resource rename lost provenance ownership");
        Check(assets.DeleteAsset("manual-music", AssetType::MUSIC) && retainedSource.expired(), "Removed resource leaked provenance/source data");
        ops.ObserveExternalChanges();
        for (const bool shutdown : {false, true}) {
            CommandQueue queue(contract, ops.Session(), [&](const Request& r) { return ops.Execute(r); });
            GLuint textureId = 0; std::string requestId;
            ImportHooks hooks;
            hooks.afterUploadStep = [&](const ImportBundle& bundle) {
                if (textureId || bundle.entries.empty()) return;
                const auto& payload = bundle.entries.front().payload;
                if (!std::holds_alternative<std::shared_ptr<Texture>>(payload)) return;
                textureId = std::get<std::shared_ptr<Texture>>(payload)->GetID();
                if (shutdown) queue.Stop(); else queue.Cancel(ops.Session(), requestId);
            };
            AssetImportCoordinator coordinator(queue, ops, hooks);
            auto args = Parameters(ops, "model", shutdown ? "shutdown-upload" : "cancel-upload", "textured.obj");
            requestId = Str(args, "request_id"); auto ticket = queue.Submit(Request("import_asset", args)); queue.Process();
            for (size_t i = 0; i < 10000 && !textureId; ++i) {
                coordinator.Tick();
                if (!textureId && ticket->future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
                    throw std::runtime_error("Staged upload ended before texture creation: " + ticket->future.get());
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            Check(textureId != 0, "No real staged texture was observed before cancellation");
            coordinator.Tick();
            Check(glIsTexture(textureId) == GL_FALSE && Str(Parse(ticket->future.get()), "status") == "cancelled", "Cancellation/shutdown leaked a staged GPU resource");
            Check(!assets.CheckHasAsset(shutdown ? "shutdown-upload" : "cancel-upload", AssetType::MODEL), "Cancelled upload was published");
            coordinator.Stop(); queue.Stop();
        }



    }
    if (gpu) {
        const auto root = std::filesystem::absolute("EDITOR/assets");
        LoadProjectAssets(root, assets);
        auto manifest = ReadProjectAssets(root);
        std::vector<AssetPayload> identities;
        for (const auto& request : manifest) {
            auto payload = assets.FindPayload(ImportType(Str(request, "asset_type")), Str(request, "asset_key"));
            Check(payload.has_value(), "Startup preload did not publish a demo resource");
            identities.push_back(*payload);
        }
        LoadProjectAssets(root, assets);
        size_t index = 0;
        for (const auto& request : manifest)
            Check(*assets.FindPayload(ImportType(Str(request, "asset_type")), Str(request, "asset_key")) == identities[index++], "Repeated startup preload replaced a resource");
        Check(glGetError() == GL_NO_ERROR, "Default project preload caused a GL error");
        std::cout << "Default project preload passed with real GL uploads and audio decoding\n";
    }
    scenes.CleanUp();
    std::cout << (gpu ? "Real GL/audio asset import tests passed\n" : "CPU preparation, resource registry and global history tests passed\n");
}
