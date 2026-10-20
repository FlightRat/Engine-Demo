#include "GameDisplay.h"
#include "../scene/PlaybackController.h"
#include "../scene/SceneManager.h"
#include "../scene/SceneObject.h"
#include "../utilities/ImGuiUtils.h"
#include <Core/ECS/MainRegistry.h>
#include <Core/Resources/AssetManager.h>
#include <Core/Buffers/BufferManager.h>
#include <Core/Systems/RenderSystem.h>
#include <Rendering/Core/Camera3D.h>
#include <imgui.h>
namespace ENGINE_EDITOR {
	GLuint GameDisplay::RenderGame(int width, int height)
    {
        auto scene = SCENE_MANAGER().GetCurrentScene();
        if (!scene) return 0;
        auto& runtimeRegistry = *PlaybackController::Get().VisibleRegistry();
        if (!runtimeRegistry.GetRegistry().ctx().contains<std::shared_ptr<ENGINE_RENDERING::Camera3D>>()) return 0;
        const auto& camera = runtimeRegistry.GetContext<std::shared_ptr<ENGINE_RENDERING::Camera3D>>();
        auto& mainRegistry = MAIN_REGISTRY();
        const auto& renderSystem = mainRegistry.GetContext<std::shared_ptr<ENGINE_CORE::Systems::RenderSystem>>();
        auto& buffers = mainRegistry.GetBufferManager();
        const auto fb = buffers.GetFrameBuffer("GAME_FB");
        const auto gb = buffers.GetFrameBuffer("GAME_GB");
        const auto ao = buffers.GetFrameBuffer("GAME_AO");
        const auto blur = buffers.GetFrameBuffer("GAME_AO_Blur");
        const auto fxaa = buffers.GetFrameBuffer("GAME_FXAA");
        // Finish all allocation changes before camera setup or any draw command.
        for (const auto& target : {fb, gb, ao, blur, fxaa}) {
            target->Resize(width, height);
            target->CheckResize();
        }
        camera->SetWidth(width);
        camera->SetHeight(height);
        renderSystem->DeferredRenderPipeline(camera, runtimeRegistry, gb, ao, blur, fb, fxaa);
		return fxaa->GetTextureID();
    }


void GameDisplay::DrawToolbar() {
    auto& playback = PlaybackController::Get();
    const auto controls = PlaybackControls(playback.State());
    auto& assets = MAIN_REGISTRY().GetAssetManager();
    auto button = [&](const char* id, const char* asset, bool enabled, bool active, const char* tooltip) {
        auto texture = assets.GetTexture(asset); if (!texture) return false;
        if (active) ImGui::PushStyleColor(ImGuiCol_Button, BUTTON_HELD);
        ImGui::BeginDisabled(!enabled);
        const bool pressed = ImGui::ImageButton(id, (ImTextureID)(intptr_t)texture->GetID(), TOOL_BUTTON_SIZE);
        ImGui::EndDisabled(); if (active) ImGui::PopStyleColor();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled | ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("%s", tooltip);
        return pressed && enabled;
    };
    ImGui::Separator();
    if (button("##playButton", "play_button", controls.playEnabled, controls.playActive, controls.playTooltip)) playback.Play();
    ImGui::SameLine();
    if (button("##pauseButton", "pause_button", controls.pauseEnabled, controls.pauseActive, "Pause Game")) playback.Pause();
    ImGui::SameLine();
    if (button("##stopButton", "stop_button", controls.stopEnabled, false, "Stop Game")) playback.Stop();
    ImGui::Separator();
}
void GameDisplay::Draw() {
    auto& playback = PlaybackController::Get();
    if (!ImGui::Begin("Game")) { playback.SetGameInputAllowed(false); ImGui::End(); return; }
    DrawToolbar();
    if (ImGui::BeginChild("##GameChild", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollWithMouse)) {
        const auto available = ImGui::GetContentRegionAvail(); const auto origin = ImGui::GetCursorScreenPos();
        const auto texture = RenderGame(int(available.x), int(available.y));
        if (texture) ImGui::Image((ImTextureID)(intptr_t)texture, available, ImVec2(0, 1), ImVec2(1, 0));
        const bool focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::IsAnyItemActive();
        const auto window = uint32_t(reinterpret_cast<intptr_t>(ImGui::GetWindowViewport()->PlatformHandle));
        playback.SetGameViewport(origin.x, origin.y, origin.x + available.x, origin.y + available.y, window, focused, ImGui::GetIO().WantTextInput);
    } else playback.SetGameInputAllowed(false);
    ImGui::EndChild(); ImGui::End();
}
}
