#pragma once
#include <memory>
#include <unordered_map>
#include <cstdint>
#include <glm/glm.hpp>
#include <Rendering/Buffers/DepthPyramid.h>
#include <Rendering/Buffers/Framebuffer.h>
#include <Rendering/Core/Camera3D.h>

namespace ENGINE_CORE::Systems {
    struct SSRObjectState
    {
        glm::mat4 model{1.0f};
        size_t signature = 0;
    };

    struct SSRViewState
    {
        std::shared_ptr<ENGINE_RENDERING::Framebuffer> raw;
        ENGINE_RENDERING::DepthPyramid depth;
        std::shared_ptr<ENGINE_RENDERING::Framebuffer> history[2], previous, scratch;
        std::unordered_map<uint32_t, SSRObjectState> models, currentModels;
        glm::mat4 view{1.0f}, projection{1.0f}, previousView{1.0f}, previousProjection{1.0f};
        int width = 0, height = 0, readIndex = 0;
        unsigned int sampleIndex = 0;
        size_t lightingSignature = 0;
        bool valid = false;

        void SetScene(const std::shared_ptr<void>& scene, bool playing);
        void Begin(const std::shared_ptr<ENGINE_RENDERING::Camera3D>& camera, int aoMode);
        void Finish(bool keepHistory);
        void Invalidate();
    private:
        std::weak_ptr<void> m_Scene;
        std::weak_ptr<ENGINE_RENDERING::Camera3D> m_Camera;
        bool m_Playing = false;
        int m_AOMode = -1;
    };
}
