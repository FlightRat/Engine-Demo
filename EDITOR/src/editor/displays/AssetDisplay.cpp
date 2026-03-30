#include "AssetDisplay.h"
#include "Utilities/EngineUtilities.h"
#include "Core/ECS/MainRegistry.h"
#include "Core/Inputs/InputManager.h"
#include "Core/Resources/AssetManager.h"
#include "../utilities/editor_utilities.h"
#include "../scene/SceneManager.h"
#include "Logger/Logger.h"
#include <imgui.h>
#include <SDL.h>
#include "FileSystem/Dialogs/FileDialog.h"

constexpr float DEFAULT_ASSET_SIZE = 128.f;
constexpr ImVec2 DRAG_ASSET_SIZE = ImVec2{ 32.f,32.f };

namespace ENGINE_EDITOR {
	void AssetDisplay::SetAssetType()
	{
		if (!m_bAssetTypeChanged)
			return;
		if (m_sSelectedType == "TEXTURE")
		{
			m_eSelectedType = ENGINE_UTIL::AssetType::TEXTURE;
			m_sDragSource = std::string{ DROP_TEXTURE_SRC };
		}
		else if (m_sSelectedType == "MODEL")
		{
			m_eSelectedType = ENGINE_UTIL::AssetType::MODEL;
			m_sDragSource = std::string{ DROP_MODEL_SRC };
		}
		else if (m_sSelectedType == "SHADER")
		{
			m_eSelectedType = ENGINE_UTIL::AssetType::SHADER;
			m_sDragSource = std::string{ DROP_SHADER_SRC };
		}
		else if (m_sSelectedType == "MUSIC")
		{
			m_eSelectedType = ENGINE_UTIL::AssetType::MUSIC;
			m_sDragSource = std::string{ DROP_MUSIC_SRC };
		}
		else if (m_sSelectedType == "SOUNDFX")
		{
			m_eSelectedType = ENGINE_UTIL::AssetType::SOUNDFX;
			m_sDragSource = std::string{ DROP_SOUNDFX_SRC };
		}
		else if (m_sSelectedType == "SCENE")
		{
			m_eSelectedType = ENGINE_UTIL::AssetType::SCENE;
			m_sDragSource = std::string{ DROP_SCENE_SRC };
		}
		else
		{
			m_eSelectedType = ENGINE_UTIL::AssetType::NO_TYPE;
			m_sDragSource = "NO_ASSET_TYPE";
		}
		m_bAssetTypeChanged = false;
	}

	void AssetDisplay::DrawSelectedAssets()
	{
		auto& mainRegistry = MAIN_REGISTRY();
		auto& assetManager = mainRegistry.GetAssetManager();

		std::vector<std::string> assetNames;
		if (m_eSelectedType == ENGINE_UTIL::AssetType::SCENE)
		{
			assetNames = SCENE_MANAGER().GetAllSceneNames();
		}
		else
		{
			assetNames = assetManager.GetAssetKeyName(m_eSelectedType);
		}
		if (assetNames.empty())
			return;

		// 1. [核心修复] 获取当前可用的真实内容区域宽度（剔除 padding 和滚动条）
		float availableWidth = ImGui::GetContentRegionAvail().x;

		// 评估每个 Cell 需要占用的宽度 (Image 大小 + ImGui 各自默认的 padding)
		ImGuiStyle& style = ImGui::GetStyle();
		float cellPadding = style.CellPadding.x * 2.0f;
		float cellSize = m_AssetSize + cellPadding;

		// 2. [核心修复] 动态计算列数，至少保证有 1 列，防止除 0 和显示异常
		int numCols = (std::max)(1, static_cast<int>(availableWidth / cellSize));

		// 3. [核心修复] 更改 Table Flag 为 SizingFixedSame 强制所有列宽一致，不受内部长文本影响
		ImGuiTableFlags tableFlags = ImGuiTableFlags_SizingFixedSame | ImGuiTableFlags_PadOuterX;

		int k{ 0 }, id{ 0 };
		auto assetItr = assetNames.begin();

		if (ImGui::BeginTable("Assets", numCols, tableFlags))
		{
			for (int row = 0; row < assetNames.size(); row++) // 使用资源数量做防护
			{
				if (assetItr == assetNames.end()) break;

				ImGui::TableNextRow();
				for (int col = 0; col < numCols; col++)
				{
					if (assetItr == assetNames.end())
						break;

					ImGui::PushID(k++);
					ImGui::TableSetColumnIndex(col);

					// set bg color if is selected
					bool bSelectedAsset{ m_SelectedID == id };
					if (bSelectedAsset)
						ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::GetColorU32(ImVec4{ 0.0f,0.9f,0.f,0.3f }));

					GLuint textureID{ GetTextureID(*assetItr) };
					if (textureID == 0)
						break;

					// 为了让图片在列中居中或排版好看，可以稍微控制一下 X 偏移，这里暂时保持你的设定
					ImGui::ImageButton("asset image button", (ImTextureID)textureID, ImVec2{ m_AssetSize, m_AssetSize });

					// select if mouse click the button
					if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0) && !m_bRename)
						m_SelectedID = id;

					// right click context
					auto sAssetName = (*assetItr).c_str();
					if (bSelectedAsset && ImGui::BeginPopupContextItem())
					{
						OpenAssetContext(*assetItr);
						ImGui::EndPopup();
					}

					// drag & drop with a image
					if (ImGui::BeginDragDropSource())
					{
						ImGui::SetDragDropPayload(m_sDragSource.c_str(), sAssetName, (strlen(sAssetName) + 1) * sizeof(char), ImGuiCond_Once);
						ImGui::Image((ImTextureID)textureID, DRAG_ASSET_SIZE);
						ImGui::EndDragDropSource();
					}

					// 4. [核心修复] 长文本处理：包裹和裁剪，而不是撑大元素
					if (!m_bRename || !bSelectedAsset)
					{
						// 使用 TextWrapped 让文本到达列宽边界时自动换行，保持布局整洁
						ImGui::TextWrapped("%s", sAssetName);
					}

					// do the rename
					std::string sCheckName{ m_sRenameBuf.data() };
					if (m_bRename && bSelectedAsset)
					{
						// 5. [核心修复] 限制 InputText 的宽度，防止输入超长字符串时撑大 UI 列宽 (-FLT_MIN 表示占据该列全部剩余宽度)
						ImGui::SetNextItemWidth(-FLT_MIN);
						ImGui::SetKeyboardFocusHere();
						if (ImGui::InputText("##rename", m_sRenameBuf.data(), 255, ImGuiInputTextFlags_EnterReturnsTrue))
						{
							if (!DoRenameAsset(*assetItr, sCheckName))
							{
								ENGINE_ERROR("Failed to change asset name!");
							}
							m_sRenameBuf.clear();
							m_bRename = false;
						}
						else if (m_bRename && ImGui::IsKeyPressed(ImGuiKey_Escape))
						{
							m_sRenameBuf.clear();
							m_bRename = false;
						}
					}

					// enter rename if double click the text
					if (!m_bRename && bSelectedAsset && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
					{
						m_sRenameBuf.clear();
						// 注意：考虑到 string 预分配 buffer, 虽然你直接赋值可能可以工作，但为了配合 InputText(data, 255)
						m_sRenameBuf.resize(255, '\0');
						std::copy((*assetItr).begin(), (*assetItr).end(), m_sRenameBuf.begin());
						m_bRename = true;
					}

					// check new name
					if (m_bRename && bSelectedAsset)
					{
						if (sAssetName != sCheckName)
							CheckRename(sCheckName);
					}

					++id;
					++assetItr;
					ImGui::PopID();
				}
			}
			
			// draw right click menu
			if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
			{
				switch (m_eSelectedType)
				{
				case ENGINE_UTIL::AssetType::TEXTURE:
					if (ImGui::Selectable("Add Texture"))
					{
						ENGINE_FileSystem::FileDialog fd{};
						auto file = fd.OpenFileDialog("Open texture", SDL_GetBasePath(), { "*.jpg", "*.png" });
						if (!file.empty())
						{
							assetManager.AddTexture("new_texture", file, false);
						}
					}
					break;
				case ENGINE_UTIL::AssetType::MODEL:
					if (ImGui::Selectable("Add Model"))
					{
						ENGINE_FileSystem::FileDialog fd{};
						auto file = fd.OpenFileDialog("Open model", SDL_GetBasePath(), { "*.obj" });
						if (!file.empty())
						{
							std::map<std::string, std::string> textures;
							assetManager.AddModel("new_model", file, textures);
							for (const auto& [texName, texPath] : textures) {
								if (!assetManager.CheckHasAsset(texName, ENGINE_UTIL::AssetType::TEXTURE)) {
									assetManager.AddTexture(texName, texPath, false);
								}
							}
						}
					}
					break;
				case ENGINE_UTIL::AssetType::MUSIC:
					if (ImGui::Selectable("Add Music"))
					{
						ENGINE_FileSystem::FileDialog fd{};
						auto file = fd.OpenFileDialog("Open music", SDL_GetBasePath(), { "*.wav" });
						if (!file.empty())
						{
							assetManager.AddMusic("new_music", file);
						}
					}
					break;
				case ENGINE_UTIL::AssetType::SOUNDFX:
					if (ImGui::Selectable("Add SoundFX"))
					{
						ENGINE_FileSystem::FileDialog fd{};
						auto file = fd.OpenFileDialog("Open soundFX", SDL_GetBasePath(), { "*.wav" });
						if (!file.empty())
						{
							assetManager.AddSoundFx("new_soundfx", file);
						}
					}
					break;
				case ENGINE_UTIL::AssetType::SCENE:
					if (ImGui::Selectable("Add Scene"))
					{
						SCENE_MANAGER().AddScene("new_scene");
					}
					break;
				case ENGINE_UTIL::AssetType::SHADER:
					if (ImGui::Selectable("Add Shader"))
					{
						ENGINE_FileSystem::FileDialog fd{};
						auto vsPath = fd.OpenFileDialog("Open vertex shader", SDL_GetBasePath(), { "*.vert" });
						auto fsPath = fd.OpenFileDialog("Open fragment shader", SDL_GetBasePath(), { "*.frag" });
						if (!vsPath.empty() && !fsPath.empty()) {
							assetManager.AddShader("new_shader", vsPath, fsPath);
						}
					}
					break;
				case ENGINE_UTIL::AssetType::NO_TYPE:
					break;
				default:
					break;
				}
				ImGui::EndPopup();
			}

			ImGui::EndTable();
		}
	}
	
	unsigned int AssetDisplay::GetTextureID(const std::string& sAssetName)
	{
		auto& assetManager = MAIN_REGISTRY().GetAssetManager();
		switch (m_eSelectedType)
		{
		case ENGINE_UTIL::AssetType::TEXTURE:{
			auto pTexture = assetManager.GetTexture(sAssetName);
			if (pTexture)
				return pTexture->GetID();
			break;
		}
		case ENGINE_UTIL::AssetType::MODEL: {
			auto pTexture = assetManager.GetTexture("model_icon");
			if (pTexture)
				return pTexture->GetID();
			break;
		}
		case ENGINE_UTIL::AssetType::SOUNDFX: {
			auto pTexture = assetManager.GetTexture("music_icon");
			if (pTexture)
				return pTexture->GetID();

			break;
		}
		case ENGINE_UTIL::AssetType::MUSIC:{
			auto pTexture = assetManager.GetTexture("music_icon");
			if (pTexture)
				return pTexture->GetID();
			break;
		}
		case ENGINE_UTIL::AssetType::SHADER: {
			auto pTexture = assetManager.GetTexture("shader_icon");
			if (pTexture)
				return pTexture->GetID();
			break;
		}
		case ENGINE_UTIL::AssetType::SCENE: {
			auto pTexture = assetManager.GetTexture("scene_icon");
			if (pTexture)
				return pTexture->GetID();
			break;
		}
		}
		return 0;
	}

	bool AssetDisplay::DoRenameAsset(const std::string& sOldName, const std::string& sNewName)
	{
		if (sNewName.empty())
			return false;
		if (m_eSelectedType == ENGINE_UTIL::AssetType::SCENE)
		{
			return SCENE_MANAGER().ChangeSceneName(sOldName, sNewName);
		}
		else
		{
			auto& assetManager = MAIN_REGISTRY().GetAssetManager();
			return assetManager.ChangeAssetName(sOldName, sNewName, m_eSelectedType);
		}
		assert(false && "How did it get here?");
		return false;
	}

	void AssetDisplay::CheckRename(const std::string& sCheckName)
	{
		if (sCheckName.empty())
		{
			ImGui::TextColored(ImVec4{ 1.f,0.f,0.f,1.f }, "Rename text cannot be blank!");
		}
		bool bHasAsset{ false };
		if (m_eSelectedType == ENGINE_UTIL::AssetType::SCENE)
		{
			if (SCENE_MANAGER().CheckHasScene(sCheckName))
				bHasAsset = true;
		}
		else
		{
			auto& assetManager = MAIN_REGISTRY().GetAssetManager();
			if (assetManager.CheckHasAsset(sCheckName,m_eSelectedType))
				bHasAsset = true;
		}
		if (bHasAsset)
			ImGui::TextColored(ImVec4{ 1.f,0.f,0.f,1.f }, std::format("Asset name [{}] already exists!", sCheckName).c_str());
	}

	void AssetDisplay::OpenAssetContext(const std::string& sAssetName)
	{
		if (ImGui::Selectable("rename"))
		{
			m_bRename = true;
		}

		if(ImGui::Selectable("delete"))
		{
			if (m_eSelectedType == ENGINE_UTIL::AssetType::SCENE)
			{
				if(!SCENE_MANAGER().DeleteScene(sAssetName))
					ENGINE_ERROR("Failed to delete scene [{}].", sAssetName);
			}
			else
			{
				auto& assetManager = MAIN_REGISTRY().GetAssetManager();
				if (!assetManager.DeleteAsset(sAssetName, m_eSelectedType))
				{
					ENGINE_ERROR("Failed to delete asset [{}].", sAssetName);
				}
			}
		}
	}

	AssetDisplay::AssetDisplay() :
		m_bItemHovered{ false },
		m_bAssetTypeChanged{ true },
		m_bRename{ false },
		m_bWindowSelected{ false },
		m_bWindowHovered{ false },
		m_sSelectedAssetName{ "" },
		m_sSelectedType{ "TEXTURE" },
		m_sDragSource{ "" },
		m_sRenameBuf{ "" },
		m_eSelectedType{ENGINE_UTIL::AssetType::TEXTURE},
		m_AssetSize{DEFAULT_ASSET_SIZE},
		m_SelectedID{-1}
	{
		SetAssetType();
	}

	void AssetDisplay::Draw()
	{
		if (!ImGui::Begin("Asset"))
		{
			ImGui::End();
			return;
		}
		auto& mainRegistry = MAIN_REGISTRY();
		auto& assetManager = mainRegistry.GetAssetManager(); // TODO:use???

		ImGui::Text("Asset Type");
		ImGui::SameLine(0.f, 10.f);
		if (ImGui::BeginCombo("##AssetType", m_sSelectedType.c_str()))		// draw the asset selecting dropdown
		{
			for (const auto& sAssetType : m_SelectableTypes)				//go through all asset types
			{
				bool bIsSelected = m_sSelectedType == sAssetType;
				if (ImGui::Selectable(sAssetType.c_str(), bIsSelected))		// set each asset type as selectable item
				{
					m_bAssetTypeChanged = true;								// if a type is selected, do some update
					m_sSelectedType = sAssetType;
					m_SelectedID = -1;
					SetAssetType();
				}
				if (bIsSelected)
					ImGui::SetItemDefaultFocus();							// highlight the selected asset type
			}
			ImGui::EndCombo();
		}

		// draw the asset item display
		if (ImGui::BeginChild("##AssetTable", ImVec2{ 0.f,0.f }, NULL, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_ChildWindow))
		{
			m_bWindowHovered = ImGui::IsWindowHovered();
			m_bWindowSelected = ImGui::IsWindowFocused();
			DrawSelectedAssets();
			ImGui::EndChild();
		}
		
		ImGui::End();
	}

	void AssetDisplay::Update()
	{
	}
}


