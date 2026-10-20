#include "CaptureCoordinator.h"
#include <cmath>

namespace ENGINE_EDITOR::Automation {
std::pair<int, int> CaptureSize(int width, int height) {
    if (width < 64 || height < 64 || width > 1024 || height > 1024) throw Error("invalid_arguments", "Capture dimensions must be 64..1024");
    const double factor = std::min(1.0, std::sqrt(double(MaxCapturePixels) / (double(width) * height)));
    return {std::max(1, int(std::floor(width * factor))), std::max(1, int(std::floor(height * factor)))};
}
namespace {
std::string Base64(const std::vector<unsigned char>& bytes) {
    constexpr char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out; out.reserve((bytes.size() + 2) / 3 * 4);
    for (size_t i = 0; i < bytes.size(); i += 3) {
        const uint32_t n = uint32_t(bytes[i]) << 16 | (i + 1 < bytes.size() ? uint32_t(bytes[i+1]) << 8 : 0) | (i + 2 < bytes.size() ? bytes[i+2] : 0);
        out += chars[n >> 18]; out += chars[(n >> 12) & 63];
        out += i + 1 < bytes.size() ? chars[(n >> 6) & 63] : '=';
        out += i + 2 < bytes.size() ? chars[n & 63] : '=';
    }
    return out;
}
}
Json CaptureResponse(Json out, CaptureFrame frame, uint64_t frameId) {
    if (frame.width <= 0 || frame.height <= 0 || frame.width > 1024 || frame.height > 1024 ||
        size_t(frame.width) * frame.height > MaxCapturePixels || frame.pixels.size() != size_t(frame.width) * frame.height * 3)
        throw Error("invalid_response", "Capture pixel buffer does not match dimensions");
    auto& a = out.GetAllocator(); auto& data = out["data"];
    data.AddMember("frame_id", frameId, a); data.AddMember("width", frame.width, a); data.AddMember("height", frame.height, a);
    data.AddMember("camera", Copy(frame.camera, a), a);
    data.AddMember("encoding", "rgb8", a); data.AddMember("origin", "bottom_left", a);
    if (Stringify(out).size() > 32768) throw Error("capture_too_large", "Capture metadata exceeds budget");
    data.AddMember("pixels_base64", Text(Base64(frame.pixels), a), a);
    if (Stringify(out).size() > MaxMessageBytes) throw Error("capture_too_large", "Capture response exceeds bridge limit");
    return out;
}
CaptureCoordinator::CaptureCoordinator(CommandQueue& q, EditorOperations& ops, std::unique_ptr<CaptureSource> source)
    : m_Queue(q), m_Operations(ops), m_Source(std::move(source)) {
    m_Queue.SetCaptureHandler([this](const auto& ticket) { Begin(ticket); });
}
CaptureCoordinator::~CaptureCoordinator() { Stop(); }
Json CaptureCoordinator::ErrorResult(const Error& error) {
    auto out = Failure("", error); auto& a = out.GetAllocator();
    out.AddMember("session_id", Text(m_Operations.Session(), a), a); out.AddMember("editor_revision", m_Operations.Revision(), a);
    return out;
}
void CaptureCoordinator::Begin(const CommandQueue::Handle& ticket) {
    auto context = m_Operations.CheckCapture(ticket->request);
    if (Str(context, "status") != "succeeded") { m_Queue.CompleteDeferred(ticket, std::move(context)); return; }
    if (!m_Source) { m_Queue.CompleteDeferred(ticket, ErrorResult(Error("capture_unavailable", "No scene renderer is attached", true))); return; }
    if (m_Pending || m_Encoded.valid()) { m_Queue.CompleteDeferred(ticket, ErrorResult(Error("capture_busy", "Previous capture is still finishing", true))); return; }
    m_Pending = ticket; m_Started = false;
}
void CaptureCoordinator::Discard() { if (m_Source) m_Source->Cancel(); m_Pending.reset(); m_Started = false; }
void CaptureCoordinator::Tick() {
    if (m_Encoded.valid()) {
        if (m_Encoded.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return;
        try { auto result = m_Encoded.get(); if (m_Pending) m_Queue.CompleteDeferred(m_Pending, std::move(result)); }
        catch (const Error& e) { if (m_Pending) m_Queue.CompleteDeferred(m_Pending, ErrorResult(e)); }
        catch (const std::exception&) { if (m_Pending) m_Queue.CompleteDeferred(m_Pending, ErrorResult(Error("capture_unavailable", "Capture encoding failed", true))); }
        Discard(); return;
    }
    if (!m_Pending) return;
    if (!m_Queue.Active(m_Pending)) { Discard(); return; }
    try {
        if (std::chrono::steady_clock::now() >= m_Pending->request.deadline) throw Error("capture_timeout", "Capture deadline expired", true);
        auto context = m_Operations.CheckCapture(m_Pending->request);
        if (Str(context, "status") != "succeeded") { m_Queue.CompleteDeferred(m_Pending, std::move(context)); Discard(); return; }
        if (!m_Started) {
            auto& p = m_Pending->request.params;
            const auto [width, height] = CaptureSize(p.HasMember("width") ? p["width"].GetInt() : 640, p.HasMember("height") ? p["height"].GetInt() : 360);
            m_Source->Begin(width, height); m_Started = true; return;
        }
        if (auto frame = m_Source->Poll()) {
            auto id = ++m_FrameId;
            m_Encoded = std::async(std::launch::async, [c = std::move(context), f = std::move(*frame), id]() mutable { return CaptureResponse(std::move(c), std::move(f), id); });
        }
    } catch (const Error& e) { m_Queue.CompleteDeferred(m_Pending, ErrorResult(e)); Discard(); }
    catch (const std::exception&) { m_Queue.CompleteDeferred(m_Pending, ErrorResult(Error("capture_unavailable", "Scene rendering failed", true))); Discard(); }
}
void CaptureCoordinator::Stop() {
    if (m_Pending) m_Queue.Cancel(m_Pending);
    Discard();
    if (m_Encoded.valid()) { try { m_Encoded.get(); } catch (...) {} }
    m_Queue.SetCaptureHandler({});
}
}
