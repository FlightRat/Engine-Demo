#include "ComponentDrawer.h"
#include "Core/ECS/MainRegistry.h"
#include "Core/Resources/AssetManager.h"
//#include "Core/CoreUtilities/CoreUtilities.h"
#include "UTILITIES/EngineUtilities.h"
#include "Logger/Logger.h"
#include "ImGuiUtils.h"

using namespace ENGINE_CORE::ECS;

namespace ENGINE_EDITOR {
	void ComponentDrawer::DrawImGuiComponent(ENGINE_CORE::ECS::TransformComponent& transform)
	{
		ImGui::SeparatorText("Transform");
		ImGui::PushID(entt::type_hash<TransformComponents>::value());
		if (ImGui::TreeNodeEx("##TransformTree", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushItemWidth(120.f);

			// position
			ImGui::InlineLabel("position");
			// position-x 
			ImGui::ColoredLabel("x##pos_x", LABEL_SINGLE_SIZE, LABEL_RED);
			ImGui::SameLine();
			ImGui::InputFloat("##position_x", &transform.position.x, 1.f, 10.f, "%.1f");
			// position-y 
			ImGui::ColoredLabel("y##pos_y", LABEL_SINGLE_SIZE, LABEL_GREEN);
			ImGui::SameLine();
			ImGui::InputFloat("##position_y", &transform.position.y, 1.f, 10.f, "%.1f");
			// position-z
			ImGui::ColoredLabel("z##pos_z", LABEL_SINGLE_SIZE, LABEL_BLUE);
			ImGui::SameLine();
			ImGui::InputFloat("##position_z", &transform.position.z, 1.f, 10.f, "%.1f");

			// scale
			ImGui::InlineLabel("scale");
			// scale-x 
			ImGui::ColoredLabel("x##scale_x", LABEL_SINGLE_SIZE, LABEL_RED);
			ImGui::SameLine();
			ImGui::InputFloat("##scale_x", &transform.scale.x, 1.f, 1.f, "%.1f");
			// scale-y 
			ImGui::ColoredLabel("y##scale_y", LABEL_SINGLE_SIZE, LABEL_GREEN);
			ImGui::SameLine();
			ImGui::InputFloat("##scale_y", &transform.scale.y, 1.f, 1.f, "%.1f");
			// scale-z
			ImGui::ColoredLabel("z##scale_z", LABEL_SINGLE_SIZE, LABEL_BLUE);
			ImGui::SameLine();
			ImGui::InputFloat("##scale_z", &transform.scale.z, 1.f, 1.f, "%.1f");

			// rotation
			ImGui::InlineLabel("rotation");

			ImGui::PopItemWidth();
			ImGui::TreePop();
		}
		ImGui::PopID();
	}

	void ComponentDrawer::DrawImGuiComponent(ENGINE_CORE::ECS::MeshFilter& meshFilter)
	{
		bool bChanged{ false };
		ImGui::SeparatorText("MeshFilter");
		ImGui::PushID(entt::type_hash<MeshFilter>::value());
		if (ImGui::TreeNodeEx("##MeshFilterTree", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushItemWidth(120.f);
			auto& assetManager = MAIN_REGISTRY().GetAssetManager();

			std::string sSelectedMesh{ meshFilter.mesh };
			ImGui::InlineLabel("mesh");
			if (ImGui::BeginCombo("##mesh", sSelectedMesh.c_str()))
			{
				for (const auto& sMeshName : assetManager.GetSelectableMesh())
				{
					if (ImGui::Selectable(sMeshName.c_str(), sMeshName == sSelectedMesh))
					{
						sSelectedMesh = sMeshName;
						meshFilter.mesh = sSelectedMesh;
						bChanged = true;
					}
				}
				ImGui::EndCombo();
			}
			ImGui::PopItemWidth();
			ImGui::TreePop();
		}
		ImGui::PopID();

		if (bChanged)
		{
			meshFilter.load_mesh();
		}
	}

	void ComponentDrawer::DrawImGuiComponent(ENGINE_CORE::ECS::MeshRender& meshRender)
	{
		ImGui::SeparatorText("MeshRender");
		ImGui::PushID(entt::type_hash<MeshRender>::value());
		if (ImGui::TreeNodeEx("##MeshRenderTree", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushItemWidth(120.f);
			auto& assetManager = MAIN_REGISTRY().GetAssetManager();

			// color
			ImVec4 col = { meshRender.color.x, meshRender.color.y,meshRender.color.z, meshRender.color.w };
			ImGui::InlineLabel("color");
			if (ImGui::ColorEdit4("##color", &col.x, IMGUI_COLOR_PICKER_FLAGS))
			{
				meshRender.color.x = static_cast<GLubyte>(col.x);
				meshRender.color.y = static_cast<GLubyte>(col.y);
				meshRender.color.z = static_cast<GLubyte>(col.z);
				meshRender.color.w = static_cast<GLubyte>(col.w);
			}

			// texture
			ImGui::InlineLabel("texture");
			std::string sSelectedTexture{ meshRender.textureName };
			if (ImGui::BeginCombo("##texture", sSelectedTexture.c_str()))
			{
				for (const auto& sTextureName : assetManager.GetAssetKeyName(ENGINE_UTIL::AssetType::TEXTURE))
				{
					if (ImGui::Selectable(sTextureName.c_str(), sTextureName == sSelectedTexture))
					{
						sSelectedTexture = sTextureName;
						meshRender.textureName = sSelectedTexture;
					}
				}
				ImGui::EndCombo();
			}

			// shader
			ImGui::InlineLabel("shader");
			std::string sSelectedShader{ meshRender.shaderName };
			if (ImGui::BeginCombo("##shader", sSelectedShader.c_str()))
			{
				for (const auto& sShaderName : assetManager.GetAssetKeyName(ENGINE_UTIL::AssetType::SHADER))
				{
					if (ImGui::Selectable(sShaderName.c_str(), sShaderName == sSelectedShader))
					{
						sSelectedShader = sShaderName;
						meshRender.shaderName = sSelectedShader;
					}
				}
				ImGui::EndCombo();
			}

			// TODO: add should render

			ImGui::PopItemWidth();
			ImGui::TreePop();
		}
		ImGui::PopID();
	}

	void ComponentDrawer::DrawImGuiComponent(ENGINE_CORE::ECS::PhysicsComponent& physics)
	{
		ImGui::SeparatorText("Physics");
		ImGui::PushID(entt::type_hash<PhysicsComponent>::value());
		if (ImGui::TreeNodeEx("##PhysicsTree", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushItemWidth(120.f);

			// stuff
			
			ImGui::PopItemWidth();
			ImGui::TreePop();
		}
		ImGui::PopID();
	}

	void ComponentDrawer::DrawImGuiComponent(ENGINE_CORE::ECS::Identification& identity)
	{
		ImGui::SeparatorText("Identity");
		ImGui::PushID(entt::type_hash<Identification>::value());
		if (ImGui::TreeNodeEx("##IdentityTree", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushItemWidth(120.f);

			// stuff

			ImGui::PopItemWidth();
			ImGui::TreePop();
		}
		ImGui::PopID();
	}
}


