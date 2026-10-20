#define SDL_MAIN_HANDLED
#include "../src/editor/automation/Protocol.h"
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <SDL.h>
#include <SDL_mixer.h>
#include <glad/glad.h>
#include <Logger/Logger.h>
#include "../src/editor/scene/SceneManager.h"
#include <filesystem>
#include <Windows.h>
#ifdef _MSC_VER
#include <crtdbg.h>
#endif
using namespace ENGINE_EDITOR::Automation;
void OperationsTests(Contract&);
void HistoryTests(Contract&);
void PhysicsEditingTests(Contract&);
void DeletionTests(Contract&);
void AssetImportTests(Contract&, bool);
void MaterialSelectionTests(Contract&);
void CaptureTests(Contract&);
void QueueTests(Contract&);
void RuntimeBasicsTests();
void SceneScriptTests();
void RuntimeLifecycleTests();
void RuntimeAudioBackendTests();
void PlaybackAutomationTests(Contract&);
int BridgeHarness(Contract&, const std::string&, int);
struct TestAssetScope {
    std::filesystem::path root = std::filesystem::temp_directory_path() /
        ("engine-scene-tests-" + std::to_string(GetCurrentProcessId()) + "-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    ~TestAssetScope() {
        if (root.parent_path() == std::filesystem::temp_directory_path() && root.filename().string().starts_with("engine-scene-tests-")) {
            std::error_code error; std::filesystem::remove_all(root, error);
        }
    }
};
static void Check(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
static void Invalid(Contract& c, const char* method, const std::string& text) {
    bool rejected = false;
    try { auto d = Parse(text); c.Validate(method, d); } catch (const Error&) { rejected = true; }
    Check(rejected, "Invalid request accepted");
}
int main(int argc, char** argv) {
#ifdef _MSC_VER
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
    try {
        const TestAssetScope testAssetScope; const auto testAssets = testAssetScope.root;
        std::filesystem::create_directories(testAssets / "scripts");
        SCENE_MANAGER().Scripts().SetAssetRoot(testAssets);
        const bool glTesting = argc > 1 && std::string(argv[1]) == "--gl-tests";
        const bool serving = argc > 2 && std::string(argv[1]) == "--serve";
        Contract c(!serving && !glTesting && argc > 1 ? argv[1] : "tools/engine_mcp/contracts/tools.json");
        if (glTesting) {
            ENGINE_INIT_LOGS(false, true);
            SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
            if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) throw std::runtime_error(SDL_GetError());
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4); SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
            auto* window = SDL_CreateWindow("Asset import backend QA", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 64, 64, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
            if (!window) throw std::runtime_error(SDL_GetError());
            auto context = SDL_GL_CreateContext(window);
            if (!context || !gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) throw std::runtime_error("Could not initialize real GL backend");
            if (Mix_OpenAudio(22050, AUDIO_S16SYS, 2, 1024) != 0) throw std::runtime_error(Mix_GetError());
            AssetImportTests(c, true);
            Mix_CloseAudio(); SDL_GL_DeleteContext(context); SDL_DestroyWindow(window); SDL_Quit(); return 0;
        }
        if (serving) return BridgeHarness(c, argv[2], argc > 3 ? std::stoi(argv[3]) : 120);
        Check(c.Tools().Size() == 21, "Twenty-one tools required");
        Check(Request("set_material", Parse("{}")).IsWrite() && Request("set_selection", Parse("{}")).IsWrite() &&
            !Request("get_selection", Parse("{}")).IsWrite() && !Request("capture_viewport", Parse("{}")).IsWrite(), "Extended write classification mismatch");
        auto valid = Parse(R"({"session_id":"s","request_id":"r","expected_revision":0,"scene_id":"scene","entity_id":"entity","space":"local","rotation_degrees":[0,90,0]})");
        c.Validate("set_transform", valid);
        Invalid(c, "set_transform", R"({"session_id":"s","request_id":"r","expected_revision":0,"scene_id":"scene","entity_id":"e","scale":[1,0,1]})");
        Invalid(c, "get_scene_state", R"({"unexpected":true})");
        Invalid(c, "get_scene_state", R"({"page_size":201})");
        Invalid(c, "create_scene", R"({"name":"test"})");
        Invalid(c, "set_transform", R"({"session_id":"s","request_id":"r","expected_revision":0,"scene_id":"scene","entity_id":"entity","position":[1e100,0,0]})");
        Invalid(c, "get_scene_state", R"({"page_size":1,"page_size":2})");
        Invalid(c, "apply_operations", R"({"session_id":"s","request_id":"r","expected_revision":0,"scene_id":"s1","operations":[{"method":"set_transform","params":{"target_ref":"later","position":[0,0,0]}}]})");
        Invalid(c, "apply_operations", R"({"session_id":"s","request_id":"r","expected_revision":0,"scene_id":"s1","operations":[{"method":"create_object","params":{"name":"a","kind":"light","client_ref":"same"}},{"method":"create_object","params":{"name":"b","kind":"light","client_ref":"same"}}]})");
        Invalid(c, "set_light", R"({"session_id":"s","request_id":"r","expected_revision":0,"scene_id":"s1","entity_id":"e","intensity":10001})");
        Invalid(c, "create_object", R"({"session_id":"s","request_id":"r","expected_revision":0,"scene_id":"s1","name":"a","kind":"mesh"})");
        auto first = Parse(R"({"b":1.0,"a":[0,-0.0]})");
        auto second = Parse(R"({"a":[0,0],"b":1})");
        Check(Request("test", first).Fingerprint() == Request("test", second).Fingerprint(), "Semantic fingerprints differ");
        ENGINE_INIT_LOGS(false, true);
        RuntimeBasicsTests();
        SCENE_MANAGER().Scripts().SetAssetRoot(testAssets / "SceneScriptTests");
        SceneScriptTests();
        SCENE_MANAGER().Scripts().SetAssetRoot(testAssets / "RuntimeLifecycleTests");
        RuntimeLifecycleTests();
        RuntimeAudioBackendTests();
        SCENE_MANAGER().Scripts().SetAssetRoot(testAssets / "PlaybackAutomationTests");
        PlaybackAutomationTests(c);
        SCENE_MANAGER().Scripts().SetAssetRoot(testAssets / "OperationsTests");
        OperationsTests(c);
        SCENE_MANAGER().Scripts().SetAssetRoot(testAssets / "HistoryTests");
        HistoryTests(c);
        SCENE_MANAGER().Scripts().SetAssetRoot(testAssets / "PhysicsEditingTests");
        PhysicsEditingTests(c);
        SCENE_MANAGER().Scripts().SetAssetRoot(testAssets / "DeletionTests");
        DeletionTests(c);
        AssetImportTests(c, false);
        SCENE_MANAGER().Scripts().SetAssetRoot(testAssets / "MaterialSelectionTests");
        MaterialSelectionTests(c);
        SCENE_MANAGER().Scripts().SetAssetRoot(testAssets / "CaptureTests");
        CaptureTests(c);
        QueueTests(c);
        std::cout << "Automation contract tests passed\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
