#pragma once
#include "EditorOperations.h"
#include "../bridge/CommandQueue.h"
#include <optional>
#include <vector>

namespace ENGINE_EDITOR::Automation {
inline constexpr size_t MaxCapturePixels = 230400;
struct CaptureFrame {
    int width = 0, height = 0;
    Json camera;
    std::vector<unsigned char> pixels;
};
class CaptureSource {
public:
    virtual ~CaptureSource() = default;
    virtual void Begin(int width, int height) = 0;
    virtual std::optional<CaptureFrame> Poll() = 0;
    virtual void Cancel() = 0;
};
std::pair<int, int> CaptureSize(int width, int height);
Json CaptureResponse(Json context, CaptureFrame frame, uint64_t frameId);
class CaptureCoordinator {
    CommandQueue& m_Queue;
    EditorOperations& m_Operations;
    std::unique_ptr<CaptureSource> m_Source;
    CommandQueue::Handle m_Pending;
    std::future<Json> m_Encoded;
    bool m_Started = false;
    uint64_t m_FrameId = 0;
    Json ErrorResult(const Error&);
    void Begin(const CommandQueue::Handle&);
    void Discard();
public:
    CaptureCoordinator(CommandQueue&, EditorOperations&, std::unique_ptr<CaptureSource>);
    ~CaptureCoordinator();
    void Tick();
    void Stop();
};
}
