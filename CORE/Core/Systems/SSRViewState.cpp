#include "SSRViewState.h"
#include <glm/gtc/matrix_inverse.hpp>

namespace ENGINE_CORE::Systems {
    void SSRViewState::Invalidate()
    {
        valid = false;
        sampleIndex = 0;
        models.clear();
        currentModels.clear();
    }

    void SSRViewState::SetScene(const std::shared_ptr<void>& scene, bool playing)
    {
        if (m_Scene.owner_before(scene) || scene.owner_before(m_Scene) || playing != m_Playing) Invalidate();
        m_Scene = scene;
        m_Playing = playing;
    }

    void SSRViewState::Begin(const std::shared_ptr<ENGINE_RENDERING::Camera3D>& camera, int aoMode)
    {
        using namespace ENGINE_RENDERING;
        if (width != raw->Width() || height != raw->Height()) {
            width = raw->Width(); height = raw->Height();
            for (auto& target : history) target = std::make_shared<Framebuffer>(BufferType::SSR_HISTORY, width, height, false);
            previous = std::make_shared<Framebuffer>(BufferType::SSR_PREVIOUS, width, height, false);
            scratch = std::make_shared<Framebuffer>(BufferType::SSR_FILTER, width, height, false);
            Invalidate();
        }
        view = camera->GetViewMatrix();
        projection = camera->GetProjectionMatrix();
        bool cameraChanged = m_Camera.owner_before(camera) || camera.owner_before(m_Camera);
        bool projectionChanged = projection != previousProjection;
        bool cut = false;
        if (valid) {
            auto inverseView = glm::inverse(view), inversePrevious = glm::inverse(previousView);
            float translation = glm::length(glm::vec3(inverseView[3] - inversePrevious[3]));
            float orientation = glm::dot(glm::vec3(inverseView[2]), glm::vec3(inversePrevious[2]));
            cut = translation > std::max(1.0f, camera->GetFarPlane() * 0.1f) || orientation < 0.70710678f;
        }
        if (cameraChanged || projectionChanged || cut || aoMode != m_AOMode) Invalidate();
        m_Camera = camera;
        m_AOMode = aoMode;
        currentModels.clear();
    }

    void SSRViewState::Finish(bool keepHistory)
    {
        previousView = view;
        previousProjection = projection;
        models.swap(currentModels);
        currentModels.clear();
        readIndex = 1 - readIndex;
        valid = keepHistory;
        if (keepHistory) ++sampleIndex;
        else sampleIndex = 0;
    }
}
