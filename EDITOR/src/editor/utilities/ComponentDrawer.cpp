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
				for (const auto& sMeshName : assetManager.GetAssetKeyName(ENGINE_UTIL::AssetType::MODEL))
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
	}

	void ComponentDrawer::DrawImGuiComponent(ENGINE_CORE::ECS::MeshRender& meshRender)
	{
		ImGui::SeparatorText("MeshRender");
		// 使用组件的 type_hash 作为一级 ID
		ImGui::PushID(entt::type_hash<ENGINE_CORE::ECS::MeshRender>::value());

		if (ImGui::TreeNodeEx("##MeshRenderTree", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushItemWidth(120.f);
			auto& assetManager = MAIN_REGISTRY().GetAssetManager();
			auto& materials = meshRender.materials;

			for (size_t i = 0; i < materials.size(); ++i)
			{
				// --- 关键修复：为每个材质分配独立 ID 域 ---
				ImGui::PushID(static_cast<int>(i));

				ImGui::Text("Material [%zu]", i);

				ENGINE_CORE::ECS::Material& m = meshRender.GetMaterial(i);

				// 1. color
				ImVec4 col = { m.color.x, m.color.y,m.color.z, m.color.w };
				ImGui::InlineLabel("color");
				if (ImGui::ColorEdit4("##color", &col.x, IMGUI_COLOR_PICKER_FLAGS))
				{
					m.color.x = static_cast<GLubyte>(col.x);
					m.color.y = static_cast<GLubyte>(col.y);
					m.color.z = static_cast<GLubyte>(col.z);
					m.color.w = static_cast<GLubyte>(col.w);
				}

				// 2. useTex
				ImGui::InlineLabel("useTex");
				ImGui::Checkbox("##useTexCheck", &m.m_useTexture); // 修复了 label 的 # 号用法

				// 3. Textures
				const char* textureSlots[] = { "diffuse", "specular" };
				for (const char* slot : textureSlots)
				{
					ImGui::InlineLabel(slot);

					// 使用 PushID 避免字符串拼接导致的内存分配
					ImGui::PushID(slot);
					std::string& currentTextureName = m.m_textures[slot];

					if (ImGui::BeginCombo("##texCombo", currentTextureName.c_str()))
					{
						if (ImGui::Selectable("None", currentTextureName.empty()))
						{
							currentTextureName = "";
						}

						for (const auto& sTextureName : assetManager.GetAssetKeyName(ENGINE_UTIL::AssetType::TEXTURE))
						{
							bool isSelected = (sTextureName == currentTextureName);
							if (ImGui::Selectable(sTextureName.c_str(), isSelected))
							{
								currentTextureName = sTextureName;
							}
							if (isSelected) ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}
					ImGui::PopID(); // Pop slot ID
				}

				// 4. Shader (只读展示)
				ImGui::InlineLabel("shader");
				ImGui::TextDisabled("%s", m.shaderName.c_str());

				ImGui::Separator();
				ImGui::PopID(); // Pop material index ID
			}

			ImGui::PopItemWidth();
			ImGui::TreePop();
		}
		ImGui::PopID(); // Pop MeshRender type ID
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

	void ComponentDrawer::DrawImGuiComponent(ENGINE_CORE::ECS::LightComponent& light)
	{
		ImGui::SeparatorText("Light");
		ImGui::PushID(entt::type_hash<Identification>::value());
		if (ImGui::TreeNodeEx("##LightTree", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushItemWidth(120.f);

			// diffuse
			ImGui::InlineLabel("diffuse");
			ImGui::NewLine();
			// diffuse r
			ImGui::ColoredLabel("r##diffuse_r", LABEL_SINGLE_SIZE, LABEL_RED);
			ImGui::SameLine();
			ImGui::DragFloat("##diffuse_r", &light.diffuse.r, 0.01f, 0.0f, 1.0f, "%.2f");
			// diffuse g
			ImGui::ColoredLabel("g##diffuse_g", LABEL_SINGLE_SIZE, LABEL_GREEN);
			ImGui::SameLine();
			ImGui::DragFloat("##diffuse_g", &light.diffuse.g, 0.01f, 0.0f, 1.0f, "%.2f");
			// diffuse b
			ImGui::ColoredLabel("b##diffuse_b", LABEL_SINGLE_SIZE, LABEL_BLUE);
			ImGui::SameLine();
			ImGui::DragFloat("##diffuse_b", &light.diffuse.b, 0.01f, 0.0f, 1.0f, "%.2f");

			// specular
			ImGui::InlineLabel("specular");
			ImGui::NewLine();
			// specular r
			ImGui::ColoredLabel("r##specular_r", LABEL_SINGLE_SIZE, LABEL_RED);
			ImGui::SameLine();
			ImGui::DragFloat("##specular_r", &light.specular.r, 0.01f, 0.0f, 1.0f, "%.2f");
			// specular g
			ImGui::ColoredLabel("g##specular_g", LABEL_SINGLE_SIZE, LABEL_GREEN);
			ImGui::SameLine();
			ImGui::DragFloat("##specular_g", &light.specular.g, 0.01f, 0.0f, 1.0f, "%.2f");
			// specular b
			ImGui::ColoredLabel("b##specular_b", LABEL_SINGLE_SIZE, LABEL_BLUE);
			ImGui::SameLine();
			ImGui::DragFloat("##specular_b", &light.specular.b, 0.01f, 0.0f, 1.0f, "%.2f");

			// ambient
			ImGui::InlineLabel("ambient");
			ImGui::NewLine();
			// ambient r
			ImGui::ColoredLabel("r##ambient_r", LABEL_SINGLE_SIZE, LABEL_RED);
			ImGui::SameLine();
			ImGui::DragFloat("##ambient_r", &light.ambient.r, 0.01f, 0.0f, 1.0f, "%.2f");
			// ambient g
			ImGui::ColoredLabel("g##ambient_g", LABEL_SINGLE_SIZE, LABEL_GREEN);
			ImGui::SameLine();
			ImGui::DragFloat("##ambient_g", &light.ambient.g, 0.01f, 0.0f, 1.0f, "%.2f");
			// ambient b
			ImGui::ColoredLabel("b##ambient_b", LABEL_SINGLE_SIZE, LABEL_BLUE);
			ImGui::SameLine();
			ImGui::DragFloat("##ambient_b", &light.ambient.b, 0.01f, 0.0f, 1.0f, "%.2f");

			std::vector<std::string> UseableLight = {"point_light", "direction_light"}; // TODO: Move this some where else

			// light type
			std::string sSelectedLightType{ light.type };
			ImGui::InlineLabel("LightType");
			if (ImGui::BeginCombo("##LightType", sSelectedLightType.c_str()))
			{
				for (const auto& lightType : UseableLight)
				{
					if (ImGui::Selectable(lightType.c_str(), lightType == sSelectedLightType))
					{
						sSelectedLightType = lightType;
						light.type = lightType;
					}
				}
				ImGui::EndCombo();
			}

			if (light.type == "point_light")
			{
				// position
				ImGui::InlineLabel("light_position");
				ImGui::NewLine();
				// position x
				ImGui::ColoredLabel("x##pos_x", LABEL_SINGLE_SIZE, LABEL_RED);
				ImGui::SameLine();
				ImGui::InputFloat("##pos_x", &light.pos.x, 1.f, 10.f, "%.1f");
				// position y
				ImGui::ColoredLabel("y##pos_y", LABEL_SINGLE_SIZE, LABEL_GREEN);
				ImGui::SameLine();
				ImGui::InputFloat("##pos_y", &light.pos.y, 1.f, 10.f, "%.1f");
				// position z
				ImGui::ColoredLabel("z##pos_z", LABEL_SINGLE_SIZE, LABEL_BLUE);
				ImGui::SameLine();
				ImGui::InputFloat("##pos_z", &light.pos.z, 1.f, 10.f, "%.1f");

				ImGui::InlineLabel("constant");
				ImGui::InputFloat("##light_constant", &light.constant, 1.f, 10.f, "%.3f");

				ImGui::InlineLabel("linear");
				ImGui::InputFloat("##light_linear", &light.linear, 1.f, 10.f, "%.3f");

				ImGui::InlineLabel("quadratic");
				ImGui::InputFloat("##light_quadratic", &light.quadratic, 1.f, 10.f, "%.3f");
			}
			else if (light.type == "direction_light")
			{
				// direction
				ImGui::InlineLabel("light_direction");
				ImGui::NewLine();
				// direction x
				ImGui::ColoredLabel("x##direction_x", LABEL_SINGLE_SIZE, LABEL_RED);
				ImGui::SameLine();
				ImGui::DragFloat("##direction_x", &light.direction.x, 0.01f, 0.0f, 1.0f, "%.2f");
				// direction y
				ImGui::ColoredLabel("y##direction_y", LABEL_SINGLE_SIZE, LABEL_GREEN);
				ImGui::SameLine();
				ImGui::DragFloat("##direction_y", &light.direction.y, 0.01f, 0.0f, 1.0f, "%.2f");
				// direction z
				ImGui::ColoredLabel("z##direction_z", LABEL_SINGLE_SIZE, LABEL_BLUE);
				ImGui::SameLine();
				ImGui::DragFloat("##direction_z", &light.direction.z, 0.01f, 0.0f, 1.0f, "%.2f");
			}

			ImGui::PopItemWidth();
			ImGui::TreePop();
		}
		ImGui::PopID();
	}

}


