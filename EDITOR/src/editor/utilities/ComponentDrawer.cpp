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
		ImGuiTreeNodeFlags flags =
			ImGuiTreeNodeFlags_DefaultOpen |
			ImGuiTreeNodeFlags_Framed |
			ImGuiTreeNodeFlags_SpanAvailWidth |
			ImGuiTreeNodeFlags_AllowOverlap;

		if (ImGui::TreeNodeEx((void*)typeid(TransformComponents).hash_code(), flags, "Transform"))
		{
			ImGui::DrawVec3Control("Position", transform.position);

			// 欧拉角转换逻辑保持不变，但UI使用 Vec3Control
			glm::vec3 euler = glm::degrees(glm::eulerAngles(transform.rotation_quat));
			glm::vec3 oldEuler = euler;

			ImGui::DrawVec3Control("Rotation", euler);

			if (euler != oldEuler) {
				transform.rotation_quat = glm::quat(glm::radians(euler));
				transform.rotation_eular = glm::radians(euler);
			}

			ImGui::DrawVec3Control("Scale", transform.scale, 1.0f);

			ImGui::TreePop();
		}
	}

	void ComponentDrawer::DrawImGuiComponent(ENGINE_CORE::ECS::MeshFilter& meshFilter)
	{
		ImGuiTreeNodeFlags flags =
			ImGuiTreeNodeFlags_DefaultOpen |
			ImGuiTreeNodeFlags_Framed |
			ImGuiTreeNodeFlags_SpanAvailWidth |
			ImGuiTreeNodeFlags_AllowOverlap;

		if (ImGui::TreeNodeEx((void*)typeid(MeshFilter).hash_code(), flags, "MeshFilter"))
		{
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
						meshFilter.changed = true;
					}
				}
				ImGui::EndCombo();
			}
			ImGui::TreePop();
		}
	}

	void ComponentDrawer::DrawImGuiComponent(ENGINE_CORE::ECS::MeshRender& meshRender)
	{
		ImGuiTreeNodeFlags flags =
			ImGuiTreeNodeFlags_DefaultOpen |
			ImGuiTreeNodeFlags_Framed |
			ImGuiTreeNodeFlags_SpanAvailWidth |
			ImGuiTreeNodeFlags_AllowOverlap;

		// 注意：这里的 hash_code 只是为了生成唯一 ID，没问题
		if (ImGui::TreeNodeEx((void*)typeid(ENGINE_CORE::ECS::MeshRender).hash_code(), flags, "Mesh Renderer"))
		{
			auto& assetManager = MAIN_REGISTRY().GetAssetManager();

			// --- 顶部控制区 ---
			ImGui::Columns(2, nullptr, false);
			ImGui::SetColumnWidth(0, 100.0f);

			ImGui::Text("Render"); ImGui::NextColumn();
			ImGui::Checkbox("##render", &meshRender.shouldRender); ImGui::NextColumn();

			ImGui::Text("Flip UV"); ImGui::NextColumn();
			ImGui::Checkbox("##flipUV", &meshRender.flipUV); ImGui::NextColumn();

			ImGui::Columns(1); // 结束列布局
			ImGui::Separator();

			// --- 材质列表 ---
			auto& materials = meshRender.materials;
			for (size_t i = 0; i < materials.size(); ++i)
			{
				ImGui::PushID((int)i);
				ENGINE_CORE::ECS::Material& m = meshRender.GetMaterial(i);

				// 材质折叠头
				bool open = ImGui::TreeNodeEx("##mat",
					ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth,
					"Material %d: %s", i, m.shaderName.c_str());

				if (open)
				{
					// =========================================================
					// 1. Color (修复点)
					// =========================================================
					// m.color 是 glm::vec4，内存布局等同于 float[4]，且范围是 0.0-1.0
					// 直接取第一个分量的地址 (&m.color.x) 传给 ImGui 即可。
					ImGui::ColorEdit4("Base Color", &m.color.x);

					// 2. Use Texture
					ImGui::Checkbox("Use Texture", &m.m_useTexture);

					// 3. Texture Slots (使用 Table 对齐)
					if (ImGui::BeginTable("TexTable", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp))
					{
						ImGui::TableSetupColumn("Slot", ImGuiTableColumnFlags_WidthFixed, 80.0f);
						ImGui::TableSetupColumn("Texture");

						const char* textureSlots[] = { "diffuse", "specular" };
						for (const char* slot : textureSlots)
						{
							ImGui::TableNextRow();
							ImGui::TableSetColumnIndex(0);
							ImGui::Text("%s", slot);

							ImGui::TableSetColumnIndex(1);
							ImGui::PushID(slot);

							std::string& currentTextureName = m.m_textures[slot];
							const char* previewValue = currentTextureName.empty() ? "None" : currentTextureName.c_str();

							if (ImGui::BeginCombo("##tex", previewValue))
							{
								if (ImGui::Selectable("None", currentTextureName.empty())) currentTextureName = "";
								for (const auto& sTextureName : assetManager.GetAssetKeyName(ENGINE_UTIL::AssetType::TEXTURE)) {
									if (ImGui::Selectable(sTextureName.c_str(), sTextureName == currentTextureName)) currentTextureName = sTextureName;
								}
								ImGui::EndCombo();
							}
							ImGui::PopID();
						}
						ImGui::EndTable();
					}
					ImGui::TreePop();
				}
				ImGui::PopID();
			}
			ImGui::TreePop();
		}
	}

	void ComponentDrawer::DrawImGuiComponent(ENGINE_CORE::ECS::PhysicsComponent& physics)
	{
		ImGuiTreeNodeFlags flags =
			ImGuiTreeNodeFlags_DefaultOpen |
			ImGuiTreeNodeFlags_Framed |
			ImGuiTreeNodeFlags_SpanAvailWidth |
			ImGuiTreeNodeFlags_AllowOverlap;

		if (ImGui::TreeNodeEx((void*)typeid(PhysicsComponent).hash_code(), flags, "Physics"))
		{
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

			ImGui::TreePop();
		}
	}

	void ComponentDrawer::DrawImGuiComponent(ENGINE_CORE::ECS::Identification& identity)
	{
		if (ImGui::TreeNodeEx((void*)typeid(Identification).hash_code(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed, "Identification"))
		{
			ImGui::Columns(2);
			ImGui::SetColumnWidth(0, 80.0f); // Label 宽度

			ImGui::Text("Name"); ImGui::NextColumn();
			// 简单的 buffer 处理，实际项目中建议封装一个 InputTextString
			char nameBuf[256];
			memset(nameBuf, 0, sizeof(nameBuf));
			strcpy_s(nameBuf, identity.name.c_str());
			if (ImGui::InputText("##Name", nameBuf, sizeof(nameBuf))) {
				identity.name = nameBuf;
			}
			ImGui::NextColumn();

			ImGui::Text("Group"); ImGui::NextColumn();
			char groupBuf[256];
			memset(groupBuf, 0, sizeof(groupBuf));
			strcpy_s(groupBuf, identity.group.c_str());
			if (ImGui::InputText("##Group", groupBuf, sizeof(groupBuf))) {
				identity.group = groupBuf;
			}

			ImGui::Columns(1);
			ImGui::TreePop();
		}
	}

	void ComponentDrawer::DrawImGuiComponent(ENGINE_CORE::ECS::LightComponent& light)
	{
		if (ImGui::TreeNodeEx((void*)typeid(LightComponent).hash_code(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed, "Light"))
		{
			// 1. Light Type (放在最上面)
			const char* lightTypes[] = { "point_light", "direction_light" };
			if (ImGui::BeginCombo("Type", light.type.c_str()))
			{
				for (auto type : lightTypes) {
					if (ImGui::Selectable(type, light.type == type)) light.type = type;
				}
				ImGui::EndCombo();
			}

			ImGui::Separator();

			// 2. Colors - 使用 ColorEdit3 代替三个独立的 DragFloat，体验好太多了
			ImGui::ColorEdit3("Diffuse", &light.diffuse.r);
			ImGui::ColorEdit3("Specular", &light.specular.r);
			ImGui::ColorEdit3("Ambient", &light.ambient.r);

			ImGui::Separator();

			// 3. Type specific properties
			if (light.type == "point_light")
			{
				ImGui::DrawVec3Control("Position", light.pos); // 复用之前的 Helper

				// 衰减参数
				ImGui::Text("Attenuation");
				ImGui::DragFloat("Constant", &light.constant, 0.01f, 0.0f, 10.0f);
				ImGui::DragFloat("Linear", &light.linear, 0.001f, 0.0f, 1.0f);
				ImGui::DragFloat("Quadratic", &light.quadratic, 0.001f, 0.0f, 1.0f);
			}
			else if (light.type == "direction_light")
			{
				ImGui::DrawVec3Control("Direction", light.direction);
			}

			ImGui::TreePop();
		}
	}

}