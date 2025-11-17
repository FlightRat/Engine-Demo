#include "AssetDisplay.h"
#include "Utilities/EngineUtilities.h"
#include "Core/ECS/MainRegistry.h"
#include "Core/Scripting/InputManager.h"
#include "Core/Resources/AssetManager.h"
#include "../utilities/editor_utilities.h"
#include "../scene/SceneManager.h"
#include "Logger/Logger.h"
#include <imgui.h>

constexpr float DEFAULT_ASSET_SIZE = 128.f;
constexpr ImVec2 DRAG_ASSET_SIZE = ImVec2{ 32.f,32.f };

namespace ENGINE_EDIOTR {
	void AssetDisplay::SetAssetType()
	{
		if (!m_bAssetTypeChanged)
			return;
		if (m_sSelectedType == "TEXTURE")
		{
			m_eSelectedType = ENGINE_UTIL::AssetType::TEXTURE;
			m_sDragSource = std::string{ DROP_TEXTURE_SRC };
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

		float windowWidth = ImGui::GetWindowWidth();
		int numCols = static_cast<int>((windowWidth - m_AssetSize) / m_AssetSize);
		int numRows = static_cast<int>(assetNames.size() / (numCols <= 1 ? 1 : numCols) + 1);
		if (!numCols || !numRows)
			return;

		ImGuiTableFlags tableFlags{ 0 };
		tableFlags |= ImGuiTableFlags_SizingFixedFit;
		int k{ 0 }, id{ 0 };
		auto assetItr = assetNames.begin();
		
		if (ImGui::BeginTable("Assets", numCols, tableFlags))
		{
			for (int row = 0; row < numRows; row++)
			{
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

					// image button
					GLuint textureID{ GetTextureID(*assetItr) };
					if (textureID == 0)
						break;
					ImGui::ImageButton("asset image button",(ImTextureID)textureID, ImVec2{m_AssetSize,m_AssetSize});

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

					// show asset name
					if (!m_bRename || !bSelectedAsset)
						ImGui::Text(sAssetName);

					// do the rename
					std::string sCheckName{ m_sRenameBuf.data() };
					if (m_bRename && bSelectedAsset)
					{
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
						else if (m_bRename && ImGui::IsKeyPressed(ImGuiKey_Escape)) //TODO: add check here
						{
							m_sRenameBuf.clear();
							m_bRename = false;
						}
					}

					// enter rename if double click the text
					if (!m_bRename && bSelectedAsset && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
					{
						m_sRenameBuf.clear();
						m_sRenameBuf = *assetItr;
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
			//TODO: change scene name
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
			//TODO: check if the scene already exists!
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
		if (ImGui::BeginCombo("##AssetType", m_sSelectedType.c_str()))
		{
			/*
			* for -- go through all asset types
			* ImGui::Selectable -- set each asset type as selectable item
			* if -- if a item is selected, do the work
			*/
			for (const auto& sAssetType : m_SelectableTypes)
			{
				bool bIsSelected = m_sSelectedType == sAssetType;
				if (ImGui::Selectable(sAssetType.c_str(), bIsSelected))
				{
					// if a type is selected
					m_bAssetTypeChanged = true;
					m_sSelectedType = sAssetType;
					m_SelectedID = -1;
					SetAssetType();
				}
				if (bIsSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

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


