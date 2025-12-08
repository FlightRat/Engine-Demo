#include "SceneHierarchyDisplay.h"
#include "../scene/SceneManager.h"
#include "../scene/SceneObject.h"
#include "../utilities/ComponentDrawer.h"
#include "Core/ECS/MainRegistry.h"
#include "Core/ECS/Entity.h"
#include "Core/ECS/Components/ScriptComponent.h"
#include "Core/ECS/Components/TransformComponent.h"
#include "Core/ECS/Components/PhysicsComponent.h"
#include "Core/ECS/Components/MeshFilter.h"
#include "Core/ECS/Components/MeshRender.h"
#include "Core/ECS/MetaUtilities.h"
#include <imgui.h>

using namespace entt::literals;

namespace ENGINE_EDITOR {

	auto create_entity = [&](ENGINE_EDITOR::SceneObject& currentScene) {
		ENGINE_CORE::ECS::Entity newEntity{ currentScene.GetRegistry(),"GameObject","" };
		newEntity.AddComponent<ENGINE_CORE::ECS::TransformComponent>();
		};

	bool SceneHierarchyDisplay::OpenTreeNode(ENGINE_CORE::ECS::Entity& entity)
	{

		ImGui::PushID(static_cast<int32_t>(entity.GetEntity()));

		ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
		if (m_pSelectedEntity && m_pSelectedEntity->GetEntity() == entity.GetEntity())	// highlight if selected
		{
			nodeFlags |= ImGuiTreeNodeFlags_Selected;
		}

		bool bTreeNodeOpen{ false };
		const auto& name = entity.GetName();
		bTreeNodeOpen = ImGui::TreeNodeEx(name.c_str(), nodeFlags);
		if (ImGui::IsItemClicked())
		{
			m_pSelectedEntity = std::make_shared<ENGINE_CORE::ECS::Entity>(SCENE_MANAGER().GetCurrentScene()->GetRegistry(), entity.GetEntity());
		}

		ImGui::PopID();
		return bTreeNodeOpen;
	}

	void SceneHierarchyDisplay::AddComponent(ENGINE_CORE::ECS::Entity& entity, bool* bAddComponent)
	{
		if (!bAddComponent)
			return;

		if (*bAddComponent)
			ImGui::OpenPopup("Add Component");

		ImGui::SetNextWindowSize(ImVec2(400, 0));
		if (ImGui::BeginPopupModal("Add Component"))
		{
			auto& registry = entity.GetRegistry();
			std::map<entt::id_type, std::string> componentMap;

			// 获取所有meta注册组件，存在map
			for (auto&& [id, type] : entt::resolve())
			{
				const auto& info = type.info();
				auto pos = info.name().find_last_of(':') + 1;
				auto name = info.name().substr(pos);
				componentMap[id] = std::string{ name };
			}

			static std::string componentStr{ "" };	//选中组件名
			static entt::id_type id_type{ 0 };		//选中组件id

			// 组件下拉选择
			if (ImGui::BeginCombo("Choose Component", componentStr.c_str()))
			{
				for (const auto& [id, name] : componentMap)
				{
					const bool isSelected = (componentStr == name);
					if (ImGui::Selectable(name.c_str(), isSelected))
					{
						componentStr = name;
						id_type = id;
					}
					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			// 检查组件是否重复
			static bool bError{ false };
			if (id_type != 0 && !componentStr.empty())
			{
				if (auto* storage = registry.storage(id_type))
				{
					if (storage->contains(entity.GetEntity()))
					{
						bError = true;
					}
				}
			}

			// 组件若重复，提示
			if (bError)
			{
				ImGui::Spacing();
				ImGui::TextColored(
					ImVec4{ 1.f, 0.f, 0.f, 1.f },
					"Game Object already has [%s].\nPlease make another selection.",
					componentStr.c_str()
				);
				ImGui::Spacing();
			}

			// ok按钮
			bool canAdd = (id_type != 0) && !bError;
			ImGui::BeginDisabled(!canAdd);
			if (ImGui::Button("Ok") && !bError)
			{
				auto&& storage = registry.storage(id_type);
				if (!storage)
				{
					const auto addComponent = ENGINE_CORE::Utils::InvokeMetaFunction(id_type, "add_component_default"_hs, entity);
					if (addComponent)
					{
						*bAddComponent = false;
						ImGui::CloseCurrentPopup();
					}
					else
					{
						assert(addComponent && "Failed to add component!");
						*bAddComponent = false;
						bError = true;
						ImGui::CloseCurrentPopup();
					}
				}
				else
				{
					storage->push(entity.GetEntity());
					*bAddComponent = false;
					ImGui::CloseCurrentPopup();
				}
				// 重置
				componentStr = "";
				id_type = 0;
			}
			ImGui::EndDisabled();

			ImGui::SameLine();

			// cancel按钮
			if (ImGui::Button("Cancel"))
			{
				bError = false;
				*bAddComponent = false;
				componentStr = "";
				id_type = 0;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	}

	void SceneHierarchyDisplay::DrawGameObjectDetails()
	{
		if (!ImGui::Begin("GO Details"))
		{
			ImGui::End();
			return;
		}

		if (ImGui::BeginPopupContextWindow())
		{
			if (ImGui::Selectable("Add Component"))
				m_bAddComponent = true;
			ImGui::EndPopup();
		}

		if (m_pSelectedEntity && m_bAddComponent)
			AddComponent(*m_pSelectedEntity, &m_bAddComponent);

		if (m_pSelectedEntity)
			DrawEntityComponents();

		ImGui::End();
	}

	void SceneHierarchyDisplay::DrawEntityComponents()
	{
		if (!m_pSelectedEntity)
			return;

		auto& registry = m_pSelectedEntity->GetRegistry();

		/* for -- 找到registry中所有已注册的组件类型存储池
			id: 组件类型的唯一标识符（Type ID）
			storage: 该类型组件的数据存储容器。
		*/
		for (const auto&& [id, storage] : registry.storage())
		{
			if(!storage.contains(m_pSelectedEntity->GetEntity())) // 检查选中entity是否包含当前遍历组件
				continue;
			
			// 调用id对应组件的绘制函数
			const auto drawInfo = ENGINE_CORE::Utils::InvokeMetaFunction(id, "DrawEntityComponentInfo"_hs, *m_pSelectedEntity);

			if (drawInfo)
			{
				ImGui::Spacing();
				ImGui::PushID(id);
				if (ImGui::Button("remove"))
				{
					storage.remove(m_pSelectedEntity->GetEntity());
				}
				ImGui::PopID();
			}
			ImGui::Spacing();
			ImGui::Separator();
		}
	}

	SceneHierarchyDisplay::SceneHierarchyDisplay()
	{
	}

	SceneHierarchyDisplay::~SceneHierarchyDisplay()
	{
	}

	void SceneHierarchyDisplay::Update()
	{
	}

	void SceneHierarchyDisplay::Draw()
	{
		auto pCurrentScene = SCENE_MANAGER().GetCurrentScene();
		if (!ImGui::Begin("Scene Hierarchy") || !pCurrentScene)
		{
			ImGui::End();
			return;
		}

		if (ImGui::BeginPopupContextWindow())
		{
			if (ImGui::Selectable("Add New Game Object"))
			{
				create_entity(*pCurrentScene);
			}
			ImGui::EndPopup();
		}

		auto& registry = pCurrentScene->GetRegistry();
		auto sceneEntities = registry.GetRegistry().view<entt::entity>(entt::exclude<ENGINE_CORE::ECS::ScriptComponent>);
		for (auto entity : sceneEntities)
		{
			ENGINE_CORE::ECS::Entity ent{ registry, entity };
			if (OpenTreeNode(ent))
				ImGui::TreePop();
		}

		ImGui::End();

		DrawGameObjectDetails();
	}
}


