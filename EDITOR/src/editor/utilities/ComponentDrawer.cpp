#include "ComponentDrawer.h"
#include "Core/ECS/MainRegistry.h"
#include "Core/Resources/AssetManager.h"
//#include "Core/CoreUtilities/CoreUtilities.h"
#include "../scene/SceneManager.h"
#include "../scene/SceneObject.h"
#include "Physics/PhysicsUtilities.h"
#include "UTILITIES/EngineUtilities.h"
#include "Logger/Logger.h"
#include "ImGuiUtils.h"

using namespace ENGINE_CORE::ECS;
using namespace ENGINE_PHYSICS;
using namespace ENGINE_EDITOR;

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
			ImGui::NewLine();
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
			ImGui::NewLine();
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
			ImGui::NewLine();
			glm::vec3 euler = glm::degrees(glm::eulerAngles(transform.rotation_quat));
			// rotation-x
			ImGui::ColoredLabel("x##rotation_x", LABEL_SINGLE_SIZE, LABEL_RED);
			ImGui::SameLine();
			bool changedX = ImGui::DragFloat("##rot_x", &euler.x, 0.5f); // 使用 DragFloat 体验更好
			// rotation-y
			ImGui::ColoredLabel("y##rotation_y", LABEL_SINGLE_SIZE, LABEL_GREEN);
			ImGui::SameLine();
			bool changedY = ImGui::DragFloat("##rot_y", &euler.y, 0.5f);
			// rotation-z
			ImGui::ColoredLabel("z##rotation_z", LABEL_SINGLE_SIZE, LABEL_BLUE);
			ImGui::SameLine();
			bool changedZ = ImGui::DragFloat("##rot_z", &euler.z, 0.5f);
			if (changedX || changedY || changedZ) {
				transform.rotation_quat = glm::quat(glm::radians(euler));
				transform.rotation_eular = glm::radians(euler);
			}

			ImGui::PopItemWidth();
			ImGui::TreePop();
		}
		ImGui::PopID();
	}

	void ComponentDrawer::DrawImGuiComponent(ENGINE_CORE::ECS::MeshFilter& meshFilter)
	{
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
					}
				}
				ImGui::EndCombo();
			}
			ImGui::PopItemWidth();
			ImGui::TreePop();
		}
		ImGui::PopID();

		if (ImGui::Button("Apply")) {
			meshFilter.load_mesh();
			meshFilter.m_bChanged = true;
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
			auto pCurrentScene = SCENE_MANAGER().GetCurrentScene();
			auto& runtimeRegistry = pCurrentScene->GetRegistry();
			auto& common = runtimeRegistry.GetRegistry().ctx().get<std::shared_ptr<PhysicsCommon>>();
			//auto& world = runtimeRegistry.GetRegistry().ctx().get<std::shared_ptr<PhysicsWorld>>();


			PhysicsAttributes& physicsAttr = physics.GetAttr();

			//mass
			ImGui::InlineLabel("Mass");
			ImGui::InputFloat("##Mass", &physicsAttr.rb_Mass, 1.f, 1.f, "%.1f");
			ImGui::InlineLabel("MassDensity");
			ImGui::InputFloat("##MassDensity", &physicsAttr.c_MassDensity, 0.1f, 1.0f, "%.1f");
			ImGui::InlineLabel("Bounciness");
			ImGui::InputFloat("##Bounciness", &physicsAttr.c_Bounciness, 0.1f, 1.0f, "%.1f");
			ImGui::InlineLabel("FC");
			ImGui::InputFloat("##FC", &physicsAttr.c_FrictionCoefficient, 0.1f, 1.0f, "%.1f");
			ImGui::Checkbox("bTrigger", &physicsAttr.c_Trigger);

			// rigidbody type
			std::string sSelectedBodyType{ RigidBody_type2string(physicsAttr.rb_type) };
			ImGui::InlineLabel("BodyType");
			if (ImGui::BeginCombo("##BodyType", sSelectedBodyType.c_str()))
			{
				for (const auto& [bodyType, bodyStr] : GetRigidBodyStringMap())
				{
					if (ImGui::Selectable(bodyStr.c_str(), bodyStr == sSelectedBodyType))
					{
						sSelectedBodyType = bodyStr;
						physicsAttr.rb_type = bodyType;
					}
				}
				ImGui::EndCombo();
			}

			//ImGui::Separator();

			// collider type & shape
			std::string sSelectedColliderType{ physicsAttr.shape };
			ImGui::InlineLabel("ColliderType");
			if (ImGui::BeginCombo("##ColliderType", sSelectedColliderType.c_str()))
			{
				for (const auto& colliderType : GetUsableCollider())
				{
					if (ImGui::Selectable(colliderType.c_str(), colliderType == sSelectedColliderType))
					{
						sSelectedColliderType = colliderType;
						physicsAttr.shape = colliderType;
					}
				}
				ImGui::EndCombo();
			}
			if (physicsAttr.shape == "box")
			{
				ImGui::InlineLabel("Half extents");
				ImGui::DragFloat3("##box_extents", &physicsAttr.box_halfExtents.x, 0.1, 0.01f, 100.f, "%.2f");
			}
			else if (physicsAttr.shape == "sphere")
			{
				ImGui::InlineLabel("Radius");
				ImGui::DragFloat("##sphere_radius", &physicsAttr.sphere_radius, 0.1f, 0.01f, 100.f, "%.2f");
			}
			else if (physicsAttr.shape == "capsule")
			{
				ImGui::InlineLabel("Radius");
				ImGui::DragFloat("##capsule_radius", &physicsAttr.capsule_radius, 0.1f, 0.01f, 100.f, "%.2f");
				ImGui::InlineLabel("Half Height");
				ImGui::DragFloat("##capsule_height", &physicsAttr.capsule_halfHeight, 0.1f, 0.01f, 100.f, "%.2f");
			}
			
			if (ImGui::Button("Apply")) {
				physics.Update(common);
			}

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

			std::string sNameBuffer{ identity.name};
			ImGui::InlineLabel("name");
			if (ImGui::InputText(
				"##_name", sNameBuffer.data(), sizeof(char) * 255, ImGuiInputTextFlags_EnterReturnsTrue))
			{
				identity.name = std::string{ sNameBuffer.data() };
			}

			std::string sGroupBuffer{ identity.group };
			ImGui::InlineLabel("group");
			if (ImGui::InputText(
				"##_group", sGroupBuffer.data(), sizeof(char) * 255, ImGuiInputTextFlags_EnterReturnsTrue))
			{
				identity.group = std::string{ sGroupBuffer.data() };
			}

			ImGui::PopItemWidth();
			ImGui::TreePop();
		}
		ImGui::PopID();
	}
}


