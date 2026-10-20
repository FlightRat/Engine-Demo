#include "SceneCapture.h"
#include "../scene/SceneManager.h"
#include "../scene/SceneObject.h"
#include <Core/ECS/MainRegistry.h>
#include <Core/Systems/RenderSystem.h>
#include <Rendering/Core/Camera3D.h>
#include <Rendering/Buffers/Framebuffer.h>
#include <array>
#include <cstring>

namespace ENGINE_EDITOR::Automation {
namespace {
// Captures run after ImGui. Preserve bindings and fixed-function state so the
// next normal Scene/Game frame starts with the same caller-visible GL state.
struct GLState {
    GLint readFB, drawFB, viewport[4], scissor[4], program, vao, activeTexture, packBuffer, packAlignment, packRowLength, packSkipRows, packSkipPixels, renderbuffer, arrayBuffer;
    GLint polygon[2], depthFunc, cullMode, frontFace, blendSrcRGB, blendDstRGB, blendSrcAlpha, blendDstAlpha, blendEqRGB, blendEqAlpha;
    GLboolean blend, depth, stencil, cull, scissorEnabled, depthMask, colorMask[4];
    GLfloat clearColor[4];
    std::array<std::array<GLint, 4>, 32> textures{};
    struct Stencil { GLint function, ref, valueMask, writeMask, fail, depthFail, pass; } front, back;
    static Stencil GetStencil(bool back) {
        Stencil s;
        glGetIntegerv(back ? GL_STENCIL_BACK_FUNC : GL_STENCIL_FUNC, &s.function);
        glGetIntegerv(back ? GL_STENCIL_BACK_REF : GL_STENCIL_REF, &s.ref);
        glGetIntegerv(back ? GL_STENCIL_BACK_VALUE_MASK : GL_STENCIL_VALUE_MASK, &s.valueMask);
        glGetIntegerv(back ? GL_STENCIL_BACK_WRITEMASK : GL_STENCIL_WRITEMASK, &s.writeMask);
        glGetIntegerv(back ? GL_STENCIL_BACK_FAIL : GL_STENCIL_FAIL, &s.fail);
        glGetIntegerv(back ? GL_STENCIL_BACK_PASS_DEPTH_FAIL : GL_STENCIL_PASS_DEPTH_FAIL, &s.depthFail);
        glGetIntegerv(back ? GL_STENCIL_BACK_PASS_DEPTH_PASS : GL_STENCIL_PASS_DEPTH_PASS, &s.pass);
        return s;
    }
    GLState() {
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFB); glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFB);
        glGetIntegerv(GL_VIEWPORT, viewport); glGetIntegerv(GL_SCISSOR_BOX, scissor); glGetIntegerv(GL_CURRENT_PROGRAM, &program);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao); glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);
        glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &packBuffer); glGetIntegerv(GL_PACK_ALIGNMENT, &packAlignment);
        glGetIntegerv(GL_PACK_ROW_LENGTH, &packRowLength); glGetIntegerv(GL_PACK_SKIP_ROWS, &packSkipRows); glGetIntegerv(GL_PACK_SKIP_PIXELS, &packSkipPixels);
        glGetIntegerv(GL_RENDERBUFFER_BINDING, &renderbuffer); glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &arrayBuffer);
        glGetIntegerv(GL_POLYGON_MODE, polygon); glGetIntegerv(GL_DEPTH_FUNC, &depthFunc); glGetIntegerv(GL_CULL_FACE_MODE, &cullMode); glGetIntegerv(GL_FRONT_FACE, &frontFace);
        glGetIntegerv(GL_BLEND_SRC_RGB, &blendSrcRGB); glGetIntegerv(GL_BLEND_DST_RGB, &blendDstRGB); glGetIntegerv(GL_BLEND_SRC_ALPHA, &blendSrcAlpha); glGetIntegerv(GL_BLEND_DST_ALPHA, &blendDstAlpha);
        glGetIntegerv(GL_BLEND_EQUATION_RGB, &blendEqRGB); glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &blendEqAlpha);
        blend = glIsEnabled(GL_BLEND); depth = glIsEnabled(GL_DEPTH_TEST); stencil = glIsEnabled(GL_STENCIL_TEST);
        cull = glIsEnabled(GL_CULL_FACE); scissorEnabled = glIsEnabled(GL_SCISSOR_TEST);
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask); glGetBooleanv(GL_COLOR_WRITEMASK, colorMask); glGetFloatv(GL_COLOR_CLEAR_VALUE, clearColor);
        front = GetStencil(false); back = GetStencil(true);
        for (unsigned i = 0; i < textures.size(); ++i) {
            glActiveTexture(GL_TEXTURE0 + i);
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &textures[i][0]); glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &textures[i][1]);
            glGetIntegerv(GL_TEXTURE_BINDING_2D_ARRAY, &textures[i][2]); glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP_ARRAY, &textures[i][3]);
        }
        glActiveTexture(activeTexture);
    }
    ~GLState() {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, readFB); glBindFramebuffer(GL_DRAW_FRAMEBUFFER, drawFB);
        glViewport(viewport[0], viewport[1], viewport[2], viewport[3]); glScissor(scissor[0], scissor[1], scissor[2], scissor[3]);
        glUseProgram(program); glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER, arrayBuffer); glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, packBuffer); glPixelStorei(GL_PACK_ALIGNMENT, packAlignment);
        glPixelStorei(GL_PACK_ROW_LENGTH, packRowLength); glPixelStorei(GL_PACK_SKIP_ROWS, packSkipRows); glPixelStorei(GL_PACK_SKIP_PIXELS, packSkipPixels);
        glPolygonMode(GL_FRONT_AND_BACK, polygon[0]); glDepthFunc(depthFunc); glDepthMask(depthMask); glCullFace(cullMode); glFrontFace(frontFace);
        glColorMask(colorMask[0], colorMask[1], colorMask[2], colorMask[3]); glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
        glBlendEquationSeparate(blendEqRGB, blendEqAlpha); glBlendFuncSeparate(blendSrcRGB, blendDstRGB, blendSrcAlpha, blendDstAlpha);
        for (auto [cap, enabled] : {std::pair{GL_BLEND, blend}, {GL_DEPTH_TEST, depth}, {GL_STENCIL_TEST, stencil}, {GL_CULL_FACE, cull}, {GL_SCISSOR_TEST, scissorEnabled}})
            if (enabled) glEnable(cap); else glDisable(cap);
        for (auto [face, s] : {std::pair{GL_FRONT, front}, {GL_BACK, back}}) {
            glStencilFuncSeparate(face, s.function, s.ref, s.valueMask); glStencilMaskSeparate(face, s.writeMask);
            glStencilOpSeparate(face, s.fail, s.depthFail, s.pass);
        }
        for (unsigned i = 0; i < textures.size(); ++i) {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, textures[i][0]); glBindTexture(GL_TEXTURE_CUBE_MAP, textures[i][1]);
            glBindTexture(GL_TEXTURE_2D_ARRAY, textures[i][2]); glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, textures[i][3]);
        }
        glActiveTexture(activeTexture);
    }
};
Value Vector(const glm::vec3& v, Allocator& a) { Value out(rapidjson::kArrayType); for (int i = 0; i < 3; ++i) out.PushBack(v[i], a); return out; }
class SceneCapture final : public CaptureSource {
    std::shared_ptr<ENGINE_RENDERING::Camera3D> m_Camera;
    std::array<std::shared_ptr<ENGINE_RENDERING::Framebuffer>, 5> m_Targets;
    GLuint m_PBO = 0;
    GLsync m_Fence = nullptr;
    int m_Width = 0, m_Height = 0;
    Json m_CameraData;
public:
    explicit SceneCapture(std::shared_ptr<ENGINE_RENDERING::Camera3D> camera) : m_Camera(std::move(camera)) {}
    ~SceneCapture() override { Cancel(); }
    void Begin(int width, int height) override {
        Cancel();
        if (!glReadPixels || !glGetString(GL_VERSION) || !m_Camera) throw Error("capture_unavailable", "No valid Scene GL context", true);
        auto scene = SCENE_MANAGER().GetCurrentScene();
        auto render = MAIN_REGISTRY().GetContext<std::shared_ptr<ENGINE_CORE::Systems::RenderSystem>>();
        if (!scene || !render) throw Error("capture_unavailable", "No Scene render state", true);
        GLState state;
        glDisable(GL_SCISSOR_TEST); glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); glDepthMask(GL_TRUE); glStencilMask(0xff);
        if (!m_Targets[0] || m_Width != width || m_Height != height) {
            using B = ENGINE_RENDERING::BufferType; using F = ENGINE_RENDERING::Framebuffer;
            m_Targets = {std::make_shared<F>(B::GBUFFER, width, height, true), std::make_shared<F>(B::AO, width, height, false),
                std::make_shared<F>(B::AO, width, height, false), std::make_shared<F>(B::FRAMEBUFFER, width, height, true),
                std::make_shared<F>(B::FRAMEBUFFER, width, height, false)};
        }
        m_Width = width; m_Height = height;
        for (const auto& target : m_Targets) if (glCheckNamedFramebufferStatus(target->GetFboID(), GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            throw Error("capture_unavailable", "Capture framebuffer is incomplete", true);
        auto camera = std::make_shared<ENGINE_RENDERING::Camera3D>(*m_Camera); camera->SetWidth(width); camera->SetHeight(height);
        // Poll moves this Document into the frame; recreate its allocator before
        // the next capture rather than calling SetObject on a moved-from pool.
        m_CameraData = Json(rapidjson::kObjectType); auto& a = m_CameraData.GetAllocator();
        m_CameraData.AddMember("position", Vector(camera->Position, a), a); m_CameraData.AddMember("front", Vector(camera->Front, a), a);
        m_CameraData.AddMember("up", Vector(camera->Up, a), a); m_CameraData.AddMember("fov_degrees", camera->Zoom, a);
        m_CameraData.AddMember("near", camera->NearPlane, a); m_CameraData.AddMember("far", camera->FarPlane, a);
        render->DeferredRenderPipeline(camera, scene->GetRegistry(), m_Targets[0], m_Targets[1], m_Targets[2], m_Targets[3], m_Targets[4]);
        glCreateBuffers(1, &m_PBO); glNamedBufferData(m_PBO, size_t(width) * height * 3, nullptr, GL_STREAM_READ);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_Targets[4]->GetFboID()); glReadBuffer(GL_COLOR_ATTACHMENT0);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, m_PBO); glPixelStorei(GL_PACK_ALIGNMENT, 1); glPixelStorei(GL_PACK_ROW_LENGTH, 0);
        glPixelStorei(GL_PACK_SKIP_ROWS, 0); glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
        glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        m_Fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0); glFlush();
        if (!m_Fence) throw Error("capture_unavailable", "Could not create capture fence", true);
    }
    std::optional<CaptureFrame> Poll() override {
        if (!m_Fence) return {};
        const auto wait = glClientWaitSync(m_Fence, 0, 0);
        if (wait == GL_TIMEOUT_EXPIRED) return {};
        if (wait == GL_WAIT_FAILED) throw Error("capture_unavailable", "Capture GPU readback failed", true);
        CaptureFrame frame; frame.width = m_Width; frame.height = m_Height; frame.camera = std::move(m_CameraData);
        frame.pixels.resize(size_t(m_Width) * m_Height * 3);
        const auto* data = glMapNamedBufferRange(m_PBO, 0, frame.pixels.size(), GL_MAP_READ_BIT);
        if (!data) throw Error("capture_unavailable", "Capture pixel buffer cannot be mapped", true);
        std::memcpy(frame.pixels.data(), data, frame.pixels.size());
        const auto valid = glUnmapNamedBuffer(m_PBO); Cancel();
        if (!valid) throw Error("capture_unavailable", "Capture pixel buffer was invalidated", true);
        return frame;
    }
    void Cancel() override {
        if (m_Fence) { glDeleteSync(m_Fence); m_Fence = nullptr; }
        if (m_PBO) { glDeleteBuffers(1, &m_PBO); m_PBO = 0; }
    }
};
}
std::unique_ptr<CaptureSource> MakeSceneCapture(std::shared_ptr<ENGINE_RENDERING::Camera3D> camera) { return std::make_unique<SceneCapture>(std::move(camera)); }
}
