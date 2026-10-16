#include "RenderSystem.h"
#include "../ECS/MainRegistry.h"
#include "../Buffers/BufferManager.h"
#include "../Resources/AssetManager.h"
#include <Rendering/Essentials/Shader.h>

namespace ENGINE_CORE::Systems {
    void RenderSystem::SSRSpatial_Pass(std::shared_ptr<ENGINE_RENDERING::Framebuffer> intermediateGB, SSRViewState& reflection)
    {
        auto& assets = MAIN_REGISTRY().GetAssetManager();
        auto shader = assets.GetShader("defer_ssrFilter");
        shader->Enable();
        glBindTextureUnit(1, intermediateGB->GetTextureID());
        glBindTextureUnit(2, intermediateGB->GetTextureID(1));
        glBindTextureUnit(3, intermediateGB->GetTextureID(3));
        glBindTextureUnit(4, intermediateGB->GetTextureID(5));
        glBindTextureUnit(5, reflection.history[1 - reflection.readIndex]->GetTextureID(1));
        glViewport(0, 0, reflection.width, reflection.height);
        for (int pass = 0; pass < 2; ++pass) {
            shader->SetUniformInt("sampleStride", 1 << pass);
            auto source = pass == 0 ? reflection.history[1 - reflection.readIndex] : reflection.scratch;
            auto target = pass == 0 ? reflection.scratch : reflection.raw;
            glBindTextureUnit(0, source->GetTextureID());
            // The raw hit attachment must survive the color-only spatial write.
            glNamedFramebufferDrawBuffer(target->GetFboID(), GL_COLOR_ATTACHMENT0);
            target->Bind();
            assets.GetModel("quad")->GetMeshes()[0].Draw();
        }
        const GLenum attachments[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        glNamedFramebufferDrawBuffers(reflection.raw->GetFboID(), 2, attachments);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void RenderSystem::SSRTemporal_Pass(std::shared_ptr<ENGINE_RENDERING::Framebuffer> intermediateGB, SSRViewState& reflection)
    {
        auto& assets = MAIN_REGISTRY().GetAssetManager();
        auto shader = assets.GetShader("defer_ssrTemporal");
        shader->Enable();
        shader->SetUniformBool("historyValid", reflection.valid);
        shader->SetUniformMat4("previousViewProjection", reflection.previousProjection * reflection.previousView);
        shader->SetUniformMat4("inversePreviousView", glm::inverse(reflection.previousView));
        shader->SetUniformMat4("inversePreviousProjection", glm::inverse(reflection.previousProjection));
        glBindTextureUnit(0, intermediateGB->GetTextureID());
        glBindTextureUnit(1, intermediateGB->GetTextureID(1));
        glBindTextureUnit(2, intermediateGB->GetTextureID(3));
        glBindTextureUnit(3, intermediateGB->GetTextureID(4));
        glBindTextureUnit(4, intermediateGB->GetTextureID(5));
        glBindTextureUnit(5, reflection.raw->GetTextureID());
        glBindTextureUnit(6, reflection.raw->GetTextureID(1));
        for (int attachment = 0; attachment < 4; ++attachment)
            glBindTextureUnit(7 + attachment, reflection.previous->GetTextureID(attachment));
        glBindTextureUnit(11, reflection.history[reflection.readIndex]->GetTextureID());
        glBindTextureUnit(12, reflection.history[reflection.readIndex]->GetTextureID(1));
        reflection.history[1 - reflection.readIndex]->Bind();
        glViewport(0, 0, reflection.width, reflection.height);
        assets.GetModel("quad")->GetMeshes()[0].Draw();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void RenderSystem::SSRSnapshot_Pass(std::shared_ptr<ENGINE_RENDERING::Framebuffer> intermediateGB, SSRViewState& reflection)
    {
        auto& assets = MAIN_REGISTRY().GetAssetManager();
        auto shader = assets.GetShader("defer_ssrSnapshot");
        shader->Enable();
        glBindTextureUnit(0, intermediateGB->GetTextureID());
        glBindTextureUnit(1, intermediateGB->GetTextureID(1));
        glBindTextureUnit(2, intermediateGB->GetTextureID(3));
        glBindTextureUnit(3, intermediateGB->GetTextureID(5));
        glBindTextureUnit(4, reflection.raw->GetTextureID(1));
        reflection.previous->Bind();
        glViewport(0, 0, reflection.width, reflection.height);
        assets.GetModel("quad")->GetMeshes()[0].Draw();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}
