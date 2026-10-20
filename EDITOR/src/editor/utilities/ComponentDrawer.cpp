#include "../automation/MaterialRules.h"
#include "../automation/PhysicsEditing.h"
#include "ComponentDrawer.h"
#include "../automation/StateRules.h"
#include "Core/ECS/MainRegistry.h"
#include "Core/Resources/AssetManager.h"
#include "../scene/SceneManager.h"
#include "../scene/SceneObject.h"
#include "Physics/PhysicsUtilities.h"
#include "UTILITIES/EngineUtilities.h"
#include "Logger/Logger.h"
#include "ImGuiUtils.h"
#include "Rendering/Essentials/TextureCommon.h"

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
            auto selected = TOOL_CTX().selectedEntity;
            const bool physical = selected && selected->HasComponent<PhysicsComponent>();
            const auto before = transform;
            const bool child = physical && selected->HasComponent<Identification>() && selected->GetComponent<Identification>().parent_id != -1;
            ImGui::BeginDisabled(child);
            ImGui::DrawVec3Control("Position", transform.position);
			
			glm::vec3 euler = transform.rotation_eular;
			glm::vec3 oldEuler = euler;
			ImGui::DrawVec3Control("Rotation", euler);
			if (euler != oldEuler) {
				
				transform.rotation_eular = euler;
				ENGINE_EDITOR::Automation::SynchronizeRotation(transform); // 欧拉角→弧度→四元数
			}

			ImGui::BeginDisabled(physical);
            ImGui::DrawVec3Control("Scale", transform.scale, 1.0f);
            ImGui::EndDisabled(); ImGui::EndDisabled();
            if (physical && (before.position != transform.position || before.rotation_eular != transform.rotation_eular))
                Automation::SyncPhysicsPose(selected->GetRegistry(), selected->GetEntity());

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
				for (const auto& sMeshName : assetManager.GetAssetKeyName(ENGINE_UTIL::AssetType::MODEL, true))
				{
					if (ImGui::Selectable(sMeshName.c_str(), sMeshName == sSelectedMesh))
					{
						sSelectedMesh = sMeshName;
						meshFilter.mesh = sSelectedMesh;
						meshFilter.changed = true;
                ENGINE_EDITOR::Automation::NotifyManualEdit();
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
				auto& original = materials[i];
                auto m = original;

				bool open = ImGui::TreeNodeEx("##mat",
					ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth,
					"Material %d: %s", i, m.shadingModel.c_str());

				if (open)
				{
					ImGui::ColorEdit4("Base Color", &m.color.x);

					ImGui::Columns(2, "PhysicsCols", false);
					ImGui::SetColumnWidth(0, 100.0f); // 标签宽度固定

					ImGui::Text("Metallic");
					ImGui::NextColumn();
					ImGui::DragFloat("##Metallic", &m.metallic, 0.005f, 0.0f, 1.0f);
					ImGui::NextColumn();

					ImGui::Text("Roughness");
					ImGui::NextColumn();
					ImGui::DragFloat("##Roughness", &m.roughness, 0.005f, 0.0f, 1.0f);
					ImGui::NextColumn();

					ImGui::Text("Ao");
					ImGui::NextColumn();
					ImGui::DragFloat("##Ao", &m.ao, 0.005f, 0.0f, 1.0f);
					ImGui::NextColumn();

					ImGui::Columns(1);

					ImGui::Checkbox("Use Texture", &m.m_useTexture);

					if (ImGui::BeginTable("TexTable", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp))
					{
						ImGui::TableSetupColumn("Slot", ImGuiTableColumnFlags_WidthFixed, 80.0f);
						ImGui::TableSetupColumn("Texture");

						const auto& textureSlots = ENGINE_RENDERING::TextureRegistry::GetSlots();
						for (const auto& slot : textureSlots)
						{
							ImGui::TableNextRow();
							ImGui::TableSetColumnIndex(0);
							ImGui::Text("%s", slot.displayName.c_str());

							ImGui::TableSetColumnIndex(1);
							ImGui::PushID(slot.key.c_str());

							const auto existing = m.m_textures.find(slot.key);
                            std::string currentTextureName = existing == m.m_textures.end() ? "" : existing->second;
							const char* previewValue = currentTextureName.empty() ? "None" : currentTextureName.c_str();

							if (ImGui::BeginCombo("##tex", previewValue))
							{
								if (ImGui::Selectable("None", currentTextureName.empty())) m.m_textures[slot.key] = "";
								for (const auto& sTextureName : assetManager.GetAssetKeyName(ENGINE_UTIL::AssetType::TEXTURE)) {
									if (ImGui::Selectable(sTextureName.c_str(), sTextureName == currentTextureName)) m.m_textures[slot.key] = sTextureName;
								}
								ImGui::EndCombo();
							}
							ImGui::PopID();
						}
						ImGui::EndTable();
					}
					ImGui::TreePop();
				}
                try { ENGINE_EDITOR::Automation::CommitMaterial(original, m, assetManager, true); }
                catch (const ENGINE_EDITOR::Automation::Error& error) { ENGINE_ERROR("Material edit rejected: {}", error.what()); }
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
            auto scene = SCENE_MANAGER().GetCurrentScene();
            auto selected = TOOL_CTX().selectedEntity;
            if (!scene || !selected) { ImGui::TreePop(); return; }
            auto& registry = scene->GetRegistry().GetRegistry();
            const auto entity = selected->GetEntity();
            static std::weak_ptr<SceneObject> draftScene;
            static entt::entity draftEntity = entt::null;
            static PhysicsAttributes draft;
            static std::string applied, error;
            if (!physics.initialized) {
                try { Automation::ApplyPhysics(registry, entity, physics.GetAttr()); }
                catch (const std::exception& e) { error = e.what(); }
            }
            Automation::Json state; auto& allocator = state.GetAllocator();
            const auto fingerprint = Automation::Stringify(Automation::PhysicsJson(physics.GetAttr(), allocator));
            if (draftScene.lock() != scene || draftEntity != entity || applied != fingerprint) {
                draft = physics.GetAttr(); applied = fingerprint; draftScene = scene; draftEntity = entity; error.clear();
            }
            PhysicsAttributes& physicsAttr = draft;
            ImGui::TextUnformatted("Draft settings - use Apply to commit");

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
				ImGui::DragFloat3("Half Extents", &physicsAttr.box_halfExtents.x, .05f);
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
				try {
                    if (auto* id = registry.try_get<Identification>(entity); id && id->parent_id != -1)
                        throw Automation::Error("unsupported_target", "Physics editing requires a root entity");
                    Automation::ApplyPhysics(registry, entity, draft);
                    Automation::NotifyManualEdit(); error.clear();
                } catch (const std::exception& e) { error = e.what(); }
			}

            if (!error.empty()) ImGui::TextWrapped("%s", error.c_str());
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
                ENGINE_EDITOR::Automation::NotifyManualEdit();
			}
			ImGui::NextColumn();

			ImGui::Text("Group"); ImGui::NextColumn();
			char groupBuf[256];
			memset(groupBuf, 0, sizeof(groupBuf));
			strcpy_s(groupBuf, identity.group.c_str());
			if (ImGui::InputText("##Group", groupBuf, sizeof(groupBuf))) {
				identity.group = groupBuf;
                ENGINE_EDITOR::Automation::NotifyManualEdit();
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
			const char* lightTypes[] = { "point_light", "direction_light", "area_light"};
			if (ImGui::BeginCombo("##Type", light.type.c_str()))
			{
				for (auto type : lightTypes) {
					if (ImGui::Selectable(type, light.type == type)) { Automation::SetLightType(light, type); Automation::NotifyManualEdit(); }
				}
				ImGui::EndCombo();
			}
			ImGui::Columns(1);

			ImGui::Separator();

			// 2. Colors 
			if (ImGui::ColorEdit3(
				"Base Color",
				&light.color.x,
                ImGuiColorEditFlags_DisplayRGB |
				ImGuiColorEditFlags_Uint8 |
				ImGuiColorEditFlags_InputRGB)) ENGINE_EDITOR::Automation::NotifyManualEdit();
			if (ImGui::DragFloat("Intensity", &light.intensity, 0.1f, 0.0f, 10000.0f, "%.2f")) ENGINE_EDITOR::Automation::NotifyManualEdit();

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
				if (ImGui::DragFloat("##Const", &light.constant, 0.01f, 0.0f, 10.0f)) ENGINE_EDITOR::Automation::NotifyManualEdit(); ImGui::NextColumn();

				ImGui::Text("Linear"); ImGui::NextColumn();
				if (ImGui::DragFloat("##Lin", &light.linear, 0.001f, 0.0f, 1.0f)) ENGINE_EDITOR::Automation::NotifyManualEdit(); ImGui::NextColumn();

				ImGui::Text("Quadratic"); ImGui::NextColumn();
				if (ImGui::DragFloat("##Quad", &light.quadratic, 0.001f, 0.0f, 1.0f)) ENGINE_EDITOR::Automation::NotifyManualEdit(); ImGui::NextColumn();

				ImGui::Text("Render"); ImGui::NextColumn();
				if (ImGui::Checkbox("##Render", &light.render)) ENGINE_EDITOR::Automation::NotifyManualEdit();

				ImGui::Columns(1);

				if (ImGui::DragFloat("Range", &light.range, 0.1f, 0.0f, 10000.0f,"%.2f", ImGuiSliderFlags_AlwaysClamp)) ENGINE_EDITOR::Automation::NotifyManualEdit();
			}
			else if (light.type == "direction_light")
			{
				const auto oldDirection = light.direction;
                ImGui::DrawVec3Control("Direction", light.direction);
                if (!Automation::SetLightDirection(light, light.direction)) light.direction = oldDirection;
			}
			else if (light.type == "area_light") {
				ImGui::DrawVec3Control("Center_Pos", light.pos);
				glm::vec3 euler = light.rotation_eular;
				glm::vec3 oldEuler = euler;
				ImGui::DrawVec3Control("Rotation", euler);
				if (euler != oldEuler) {
					light.rotation_eular = euler;
					ENGINE_EDITOR::Automation::SynchronizeRotation(light);
				}

				// 使用 Columns 简单对齐参数
				ImGui::Columns(2, nullptr, false);
				ImGui::SetColumnWidth(0, 100.0f);

				ImGui::Text("Half_Width"); ImGui::NextColumn();
				if (ImGui::DragFloat("##Width", &light.half_width, 0.01f, 0.001f, 4.999f, "%.3f", ImGuiSliderFlags_AlwaysClamp)) ENGINE_EDITOR::Automation::NotifyManualEdit();
				ImGui::NextColumn();

				ImGui::Text("Half_Height"); ImGui::NextColumn();
				if (ImGui::DragFloat("##Height", &light.half_height, 0.01f, 0.001f, 4.999f, "%.3f", ImGuiSliderFlags_AlwaysClamp)) ENGINE_EDITOR::Automation::NotifyManualEdit();
				ImGui::NextColumn();
			}

			ImGui::TreePop();
		}
	}

}