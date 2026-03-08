#include "ComponentDrawer.h"
#include "Core/ECS/MainRegistry.h"
#include "Core/Resources/AssetManager.h"
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

			ImGui::Columns(2, nullptr, false);
			ImGui::SetColumnWidth(0, 100.0f);

			ImGui::Text("Mesh Asset");
			ImGui::NextColumn();

			std::string sSelectedMesh{ meshFilter.mesh };
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
			ImGui::Columns(1);
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

		if (ImGui::TreeNodeEx((void*)typeid(ENGINE_CORE::ECS::MeshRender).hash_code(), flags, "Mesh Renderer"))
		{
			auto& assetManager = MAIN_REGISTRY().GetAssetManager();

			// --- General Settings ---
			ImGui::Columns(2, nullptr, false);
			ImGui::SetColumnWidth(0, 100.0f);

			ImGui::Text("Render"); ImGui::NextColumn();
			ImGui::Checkbox("##render", &meshRender.shouldRender); ImGui::NextColumn();

			ImGui::Text("Flip UV"); ImGui::NextColumn();
			ImGui::Checkbox("##flipUV", &meshRender.flipUV); ImGui::NextColumn();

			ImGui::Columns(1);
			ImGui::Separator();

			// --- Materials List ---
			auto& materials = meshRender.materials;
			for (size_t i = 0; i < materials.size(); ++i)
			{
				ImGui::PushID((int)i);
				ENGINE_CORE::ECS::Material& m = meshRender.GetMaterial(i);

				bool open = ImGui::TreeNodeEx("##mat",
					ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth,
					"Material %d: %s", i, m.shaderName.c_str());

				if (open)
				{
					ImGui::ColorEdit4("Base Color", &m.color.x);
					ImGui::Checkbox("Use Texture", &m.m_useTexture);

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
			// Context retrieval
			auto pCurrentScene = SCENE_MANAGER().GetCurrentScene();
			auto& runtimeRegistry = pCurrentScene->GetRegistry();
			auto& common = runtimeRegistry.GetRegistry().ctx().get<std::shared_ptr<PhysicsCommon>>();
			auto& world = runtimeRegistry.GetRegistry().ctx().get<std::shared_ptr<PhysicsWorld>>();
			if (!physics.initialized)
			{
				physics.Init(common, world);
				physics.initialized = true;
			}

			PhysicsAttributes& physicsAttr = physics.GetAttr();

			// --- 1. Physical Properties ---
			ImGui::Columns(2, "PhysicsCols", false);
			ImGui::SetColumnWidth(0, 100.0f); // 标签宽度固定

			ImGui::Text("Mass");
			ImGui::NextColumn();
			ImGui::DragFloat("##Mass", &physicsAttr.rb_Mass, 0.1f, 0.0f, 0.0f, "%.1f");
			ImGui::NextColumn();

			ImGui::Text("Density");
			ImGui::NextColumn();
			ImGui::DragFloat("##MassDensity", &physicsAttr.c_MassDensity, 0.01f, 0.0f, 1.0f, "%.2f");
			ImGui::NextColumn();

			ImGui::Text("Friction");
			ImGui::NextColumn();
			ImGui::DragFloat("##FC", &physicsAttr.c_FrictionCoefficient, 0.01f, 0.0f, 1.0f, "%.2f");
			ImGui::NextColumn();

			ImGui::Text("Bounciness");
			ImGui::NextColumn();
			ImGui::DragFloat("##Bounciness", &physicsAttr.c_Bounciness, 0.01f, 0.0f, 1.0f, "%.2f");
			ImGui::NextColumn();

			// --- 2. RigidBody Settings ---
			ImGui::Columns(1); // 暂时退出列布局以绘制分隔线
			ImGui::Separator();
			ImGui::Columns(2, "PhysicsCols2", false); // 重新进入列布局
			ImGui::SetColumnWidth(0, 100.0f);

			ImGui::Text("Body Type");
			ImGui::NextColumn();
			std::string sSelectedBodyType{ RigidBody_type2string(physicsAttr.rb_type) };
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
			ImGui::NextColumn();

			ImGui::Text("Is Trigger");
			ImGui::NextColumn();
			ImGui::Checkbox("##bTrigger", &physicsAttr.c_Trigger);
			ImGui::NextColumn();

			// --- 3. Collider Settings ---
			ImGui::Columns(1);
			ImGui::Separator();
			ImGui::Columns(2, "PhysicsCols3", false);
			ImGui::SetColumnWidth(0, 100.0f);

			ImGui::Text("Collider Shape");
			ImGui::NextColumn();
			std::string sSelectedColliderType{ physicsAttr.shape };
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
			ImGui::NextColumn();

			// 结束基础列布局，因为 Box 需要 Vec3Control (它自己有列)
			ImGui::Columns(1);

			// --- Shape Specific UI ---
			if (physicsAttr.shape == "box")
			{
				// 使用统一的 Vec3 控件
				ImGui::DrawVec3Control("Half Extents", physicsAttr.box_halfExtents, 0.5f);
			}
			else
			{
				// 对于 Sphere 和 Capsule，重新使用两列布局
				ImGui::Columns(2, "ShapeParams", false);
				ImGui::SetColumnWidth(0, 100.0f);

				if (physicsAttr.shape == "sphere")
				{
					ImGui::Text("Radius"); ImGui::NextColumn();
					ImGui::DragFloat("##sphere_radius", &physicsAttr.sphere_radius, 0.05f, 0.01f, 1000.f, "%.2f");
					ImGui::NextColumn();
				}
				else if (physicsAttr.shape == "capsule")
				{
					ImGui::Text("Radius"); ImGui::NextColumn();
					ImGui::DragFloat("##capsule_radius", &physicsAttr.capsule_radius, 0.05f, 0.01f, 1000.f, "%.2f");
					ImGui::NextColumn();

					ImGui::Text("Half Height"); ImGui::NextColumn();
					ImGui::DragFloat("##capsule_height", &physicsAttr.capsule_halfHeight, 0.05f, 0.01f, 1000.f, "%.2f");
					ImGui::NextColumn();
				}
				ImGui::Columns(1);
			}

			// --- Apply Button ---
			ImGui::Spacing();
			// 使用 ImVec2(-1, 0) 让按钮横跨整个可用宽度，更加明显
			if (ImGui::Button("Apply Physics Changes", ImVec2(-1, 0))) {
				physics.Update(common);
			}

			ImGui::TreePop();
		}
	}

	void ComponentDrawer::DrawImGuiComponent(ENGINE_CORE::ECS::Identification& identity)
	{
		if (ImGui::TreeNodeEx((void*)typeid(Identification).hash_code(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed, "Identification"))
		{
			ImGui::Columns(2, nullptr, false);
			ImGui::SetColumnWidth(0, 80.0f);

			ImGui::Text("Name"); ImGui::NextColumn();
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
			// 1. Light Type (Full Width)
			ImGui::Columns(2, nullptr, false);
			ImGui::SetColumnWidth(0, 100.0f);

			ImGui::Text("Type"); ImGui::NextColumn();
			const char* lightTypes[] = { "point_light", "direction_light" };
			if (ImGui::BeginCombo("##Type", light.type.c_str()))
			{
				for (auto type : lightTypes) {
					if (ImGui::Selectable(type, light.type == type)) light.type = type;
				}
				ImGui::EndCombo();
			}
			ImGui::Columns(1);

			ImGui::Separator();

			// 2. Colors 
			ImGui::ColorEdit3("Diffuse", &light.diffuse.r);
			ImGui::ColorEdit3("Specular", &light.specular.r);
			ImGui::ColorEdit3("Ambient", &light.ambient.r);

			ImGui::Separator();

			// 3. Type specific properties
			if (light.type == "point_light")
			{
				ImGui::DrawVec3Control("Position", light.pos);

				ImGui::Text("Attenuation");
				// 使用 Columns 简单对齐参数
				ImGui::Columns(2, nullptr, false);
				ImGui::SetColumnWidth(0, 100.0f);

				ImGui::Text("Constant"); ImGui::NextColumn();
				ImGui::DragFloat("##Const", &light.constant, 0.01f, 0.0f, 10.0f); ImGui::NextColumn();

				ImGui::Text("Linear"); ImGui::NextColumn();
				ImGui::DragFloat("##Lin", &light.linear, 0.001f, 0.0f, 1.0f); ImGui::NextColumn();

				ImGui::Text("Quadratic"); ImGui::NextColumn();
				ImGui::DragFloat("##Quad", &light.quadratic, 0.001f, 0.0f, 1.0f); ImGui::NextColumn();

				ImGui::Text("Render"); ImGui::NextColumn();
				ImGui::Checkbox("##Render", &light.render);

				ImGui::Columns(1);
			}
			else if (light.type == "direction_light")
			{
				ImGui::DrawVec3Control("Direction", light.direction);
			}

			ImGui::TreePop();
		}
	}

}