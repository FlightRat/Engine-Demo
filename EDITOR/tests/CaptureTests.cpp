#include "../src/editor/automation/CaptureCoordinator.h"
#include "../src/editor/scene/SceneManager.h"
#include <Core/Resources/AssetManager.h>
#include <iostream>
using namespace ENGINE_EDITOR;
using namespace ENGINE_EDITOR::Automation;
namespace {
void Check(bool b, const char* text) { if (!b) throw std::runtime_error(text); }
class Pixels final : public CaptureSource {
public:
    bool ready = false, began = false;
    int width = 0, height = 0, cancelled = 0;
    void Begin(int w, int h) override { began = true; width = w; height = h; }
    std::optional<CaptureFrame> Poll() override {
        if (!ready) return {};
        CaptureFrame frame; frame.width = width; frame.height = height;
        frame.camera = Parse(R"({"position":[0,1,2],"front":[0,0,-1],"up":[0,1,0],"fov_degrees":43,"near":0.1,"far":200})");
        frame.pixels.resize(size_t(width) * height * 3, 123); return frame;
    }
    void Cancel() override { ++cancelled; }
};
}
void CaptureTests(Contract& contract) {
    auto [w, h] = CaptureSize(1024,1024);
    Check(w == h && w * h <= MaxCapturePixels, "Capture size exceeds pixel bound");
    auto& scenes = SCENE_MANAGER(); scenes.CleanUp();
    ENGINE_CORE::RESOURCES::AssetManager assets;
    EditorOperations ops(scenes, assets, contract, "capture");
    auto p = Parse("{\"session_id\":\"" + ops.Session() + "\",\"request_id\":\"new\",\"expected_revision\":0,\"name\":\"Capture\"}");
    auto created = ops.Execute(Request("create_scene", p)); Check(Str(created,"status") == "succeeded", "Scene creation failed");
    auto requestParams = Parse("{\"session_id\":\"" + ops.Session() + "\",\"scene_id\":\"" + Str(created["data"],"scene_id") + "\",\"expected_revision\":1,\"width\":64,\"height\":64}");
    CommandQueue queue(contract, ops.Session(), [&](const Request& r) { return ops.Execute(r); });
    auto source = std::make_unique<Pixels>(); auto* pixels = source.get();
    CaptureCoordinator captures(queue, ops, std::move(source));
    auto ticket = queue.Submit(Request("capture_viewport", requestParams)); queue.Process();
    Check(ticket->future.wait_for(std::chrono::seconds(0)) != std::future_status::ready, "Queue blocked on capture");
    auto busy = queue.Submit(Request("capture_viewport", requestParams));
    Check(Str(Parse(busy->future.get())["error"],"code") == "capture_busy", "Concurrent capture accepted");
    auto status = queue.Submit(Request("get_editor_status", Parse("{}"))); queue.Process();
    Check(Str(Parse(status->future.get()),"status") == "succeeded", "Capture prevented another query");
    captures.Tick(); Check(pixels->began, "Capture never began"); pixels->ready = true;
    for (int i=0; i<100 && ticket->future.wait_for(std::chrono::seconds(0)) != std::future_status::ready; ++i) {
        captures.Tick(); std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    auto result = Parse(ticket->future.get()); Check(Str(result,"status") == "succeeded" && result["data"]["frame_id"] == 1, "Capture did not complete");
    Check(result["data"]["pixels_base64"].GetStringLength() == 64 * 64 * 4 && ops.Revision() == 1, "Capture byte length or revision incorrect");
    auto duplicate = Parse(Stringify(result)); queue.CompleteDeferred(ticket, std::move(duplicate));
    queue.ReleaseCapture(ticket);
    auto expired = Request("capture_viewport", requestParams); expired.deadline = std::chrono::steady_clock::now() - std::chrono::seconds(1);
    auto timeout = queue.Submit(std::move(expired)); queue.Process();
    Check(Str(Parse(timeout->future.get())["error"],"code") == "capture_timeout", "Expired capture was not rejected");
    auto cancelled = queue.Submit(Request("capture_viewport", requestParams)); queue.Process(); captures.Tick(); queue.Cancel(cancelled); captures.Tick();
    Check(Str(Parse(cancelled->future.get()),"status") == "cancelled" && pixels->cancelled > 0, "Capture cancellation leaked work");
    auto late = queue.Submit(Request("capture_viewport", requestParams)); queue.Process(); captures.Tick(); captures.Tick(); queue.Cancel(late);
    for (int i = 0; i < 20; ++i) { captures.Tick(); std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
    Check(Str(Parse(late->future.get()), "status") == "cancelled", "Late encoder overwrote cancelled capture");
    auto changed = queue.Submit(Request("capture_viewport", requestParams)); queue.Process();
    scenes.AddScene("Other"); scenes.SetCurrentScene("Other"); ops.ObserveExternalChanges(); captures.Tick();
    Check(Str(Parse(changed->future.get())["error"],"code") == "stale_target", "Capture crossed scene context");
    CaptureFrame bad; bad.width = 64; bad.height = 64; bad.camera = Parse("{}");
    bool rejected = false; try { CaptureResponse(Parse(R"({"data":{}})"), std::move(bad), 1); } catch (const Error& e) { rejected = e.code == "invalid_response"; }
    Check(rejected, "Invalid pixel buffer accepted");
    CaptureFrame maximum; maximum.width = 640; maximum.height = 360; maximum.pixels.resize(640 * 360 * 3); maximum.camera = Parse("{}");
    Check(Stringify(CaptureResponse(Parse(R"({"data":{}})"), std::move(maximum), 1)).size() < MaxMessageBytes, "Maximum capture exceeds message limit");
    CaptureFrame oversized; oversized.width = 64; oversized.height = 64; oversized.pixels.resize(64 * 64 * 3);
    oversized.camera.SetObject(); oversized.camera.AddMember("oversized", Text(std::string(33000, 'x'), oversized.camera.GetAllocator()), oversized.camera.GetAllocator());
    rejected = false; try { CaptureResponse(Parse(R"({"data":{}})"), std::move(oversized), 1); } catch (const Error& e) { rejected = e.code == "capture_too_large"; }
    Check(rejected, "Oversized metadata accepted");
    auto current = ops.Execute(Request("get_editor_status", Parse("{}")));
    auto lastParams = Parse("{\"session_id\":\"" + ops.Session() + "\",\"scene_id\":\"" + Str(current["data"], "current_scene_id") + "\",\"expected_revision\":" + std::to_string(ops.Revision()) + "}");
    auto shutdown = queue.Submit(Request("capture_viewport", lastParams)); queue.Process(); captures.Tick();
    queue.Stop(); captures.Stop();
    Check(Str(Parse(shutdown->future.get()), "status") == "cancelled", "Shutdown left a capture unresolved");
    scenes.CleanUp();
    std::cout << "Capture coordinator tests passed (synthetic pixels, no GL window)\n";
}
