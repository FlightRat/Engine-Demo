#include "TileSystem.h"
#include "LightSystem.h"
#include "../ECS/MainRegistry.h"
#include "../Resources/AssetManager.h"

namespace ENGINE_CORE::Systems {
    void TileSystem::Bind(const std::shared_ptr<ENGINE_RENDERING::Framebuffer>& intermediateGB)
    {
        const auto& buffers = m_TileBuffers.at(ViewportKey(intermediateGB));
        buffers.headers.Bind(4);
        buffers.indices.Bind(5);
    }

    void TileSystem::Prepare(const std::shared_ptr<ENGINE_RENDERING::Framebuffer>& intermediateGB)
    {
        std::erase_if(m_TileBuffers, [](const auto& entry) {return entry.first.expired(); });

        auto& buffers = m_TileBuffers[ViewportKey(intermediateGB)];
        buffers.tileCountX = (intermediateGB->Width() + TILE_SIZE - 1) / TILE_SIZE;
        buffers.tileCountY = (intermediateGB->Height() + TILE_SIZE - 1) / TILE_SIZE;
        const GLsizeiptr tileCount = static_cast<GLsizeiptr>(buffers.tileCountX) * buffers.tileCountY;
        buffers.headers.Reserve(tileCount * sizeof(TileHeader));
        buffers.indices.Reserve(tileCount * MAX_TILE_LIGHTS * sizeof(GLuint));
        buffers.headers.Bind(4);
        buffers.indices.Bind(5);
    }

    void TileSystem::Update(const std::shared_ptr<ENGINE_RENDERING::Framebuffer>& intermediateGB)
    {
        Prepare(intermediateGB);
        auto& buffers = m_TileBuffers[ViewportKey(intermediateGB)];
        auto& lightSystem = MAIN_REGISTRY().GetContext<std::shared_ptr<ENGINE_CORE::Systems::LightSystem>>();
        auto& assetManager = MAIN_REGISTRY().GetAssetManager();
        auto shader = assetManager.GetShader("light_Culling");
        shader->Enable();
        shader->SetUniformInt("pointLightCount", lightSystem->GetActivatedPointLights());
        glBindTextureUnit(0, intermediateGB->GetTextureID(0));

        // 向GPU提交compute shader任务，参数为xyz三个方向工作组数量，一个工作组这里语义上对应了一个tile
        glDispatchCompute(buffers.tileCountX, buffers.tileCountY, 1);
        //
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }


}
