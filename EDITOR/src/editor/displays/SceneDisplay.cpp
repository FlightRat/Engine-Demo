#include "SceneDisplay.h"
#include <imgui.h>
#include <Rendering/Buffers/Framebuffer.h>

namespace ENGINE_EDIOTR
{
	SceneDisplay::SceneDisplay(ENGINE_CORE::ECS::Registry& registry):m_Registry{ registry }
	{
	}

	void SceneDisplay::Draw()
	{
		static bool pOpen{ true };
		if (!ImGui::Begin("Scene", &pOpen))
		{
			ImGui::End();
			return;
		}

		if (ImGui::BeginChild("##SceneChild", ImVec2{ 0.f,0.f }, NULL, ImGuiWindowFlags_NoScrollWithMouse))
		{
			const auto& fb = m_Registry.GetContext<std::shared_ptr<ENGINE_RENDERING::Framebuffer>>();
			ImGui::SetCursorPos(ImVec2{ 0.f,0.f });
			ImGui::Image(
				(ImTextureID)fb->GetTextureID(),
				ImVec2{
					static_cast<float>(fb->Width()),
					static_cast<float>(fb->Height())
				},
				ImVec2{ 0.f,1.f }, ImVec2{ 1.f,0.f }
			);
			ImGui::EndChild();

			ImVec2 windowSize{ ImGui::GetWindowSize() };
			if (fb->Width() != static_cast<int>(windowSize.x) || fb->Height() != static_cast<int>(windowSize.y))
				fb->Resize(static_cast<int>(windowSize.x), static_cast<int>(windowSize.y));
		}
		ImGui::End();
	}
}
