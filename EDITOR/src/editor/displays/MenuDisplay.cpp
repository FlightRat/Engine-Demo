#include "MenuDisplay.h"
#include "Logger/Logger.h"
#include <imgui.h>

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
					ENGINE_LOG("Open is pressed!")
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


