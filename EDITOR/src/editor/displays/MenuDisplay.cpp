#include "MenuDisplay.h"
#include "Logger/Logger.h"
#include <imgui.h>
#include "SDL.h"
#include "FileSystem/Dialogs/FileDialog.h"
#include "Core/Loaders/SceneLoader.h"
#include "../scene/SceneManager.h"
#include "../scene/SceneObject.h"

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
					auto file = fd.OpenFileDialog("Open test", SDL_GetBasePath(), { "*.json" });
					if (!file.empty())
					{
						auto pCurrentScene = SCENE_MANAGER().GetCurrentScene();
						if (pCurrentScene)
						{
							ENGINE_CORE::Loaders::SceneLoader sl{};
							if (!sl.LoadScene(pCurrentScene->GetRegistry(), file, true))
							{
								ENGINE_ERROR("Failed to load scene.");
							}
						}
						else
						{
							ENGINE_ERROR("Failed to save scene.No scene activated!");
						}
					}
				}
				if (ImGui::MenuItem("Save", "Ctrl + S"))
				{
					ENGINE_FileSystem::FileDialog fd{};
					auto file = fd.SaveFileDialog("Save scene test", SDL_GetBasePath(), { "*.json"});
					if (!file.empty())
					{
						auto pCurrentScene = SCENE_MANAGER().GetCurrentScene();
						if (pCurrentScene)
						{
							ENGINE_CORE::Loaders::SceneLoader sl{};
							if (!sl.SaveScene(pCurrentScene->GetRegistry(), file, true))
							{
								ENGINE_ERROR("Failed to save scene!");
							}
						}
						else
						{
							ENGINE_ERROR("Failed to save scene.No scene activated!");
						}
					}
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


