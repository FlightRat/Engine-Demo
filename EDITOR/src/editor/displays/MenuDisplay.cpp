#include "MenuDisplay.h"
#include "Logger/Logger.h"
#include <imgui.h>
#include "SDL.h"
#include "FileSystem/Dialogs/FileDialog.h"

namespace ENGINE_EDITOR {
	void MenuDisplay::Draw()
	{
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("New", "Ctrl + N"))
				{
					ENGINE_LOG("New is pressed!")
				}
				if (ImGui::MenuItem("Open", "Ctrl + O"))
				{
					ENGINE_FileSystem::FileDialog fd{};
					auto file = fd.OpenFileDialog("Open test", SDL_GetBasePath(), { "*.png","*.jpg" });
					if (!file.empty())
					{
						ENGINE_LOG("File Opened: {}", file);
					}
				}
				if (ImGui::MenuItem("Save", "Ctrl + S"))
				{
					ENGINE_LOG("Save is pressed!")
				}
				if (ImGui::MenuItem("Exit"))
				{
					ENGINE_LOG("There is no exit func now!")
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Edit"))
			{
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Tools"))
			{
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Settings"))
			{
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Help"))
			{
				ImGui::EndMenu();
			}
			
			ImGui::EndMainMenuBar();
		}
	}
}


