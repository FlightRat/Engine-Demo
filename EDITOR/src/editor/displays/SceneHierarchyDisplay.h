#pragma once
#include "IDisplay.h"
#include <cstdint>

namespace ENGINE_CORE::ECS { class Entity; }
namespace ENGINE_EDITOR { class SceneObject; }

namespace ENGINE_EDITOR {
	class SceneHierarchyDisplay :public IDisplay
	{
	private:
		std::shared_ptr<ENGINE_CORE::ECS::Entity> m_pSelectedEntity{ nullptr };
		bool m_bAddComponent{ false };
		bool m_bDeleteGameObject{ false };
		std::weak_ptr<SceneObject> m_SelectedScene;
		uint64_t m_SelectionResetSerial{0};

	private:
		bool OpenTreeNode(ENGINE_CORE::ECS::Entity& entity);
		void AddComponent(ENGINE_CORE::ECS::Entity& entity, bool* bAddComponent);
		void DrawGameObjectDetails();
		void DrawEntityComponents();

	public:
		SceneHierarchyDisplay();
		~SceneHierarchyDisplay();

		virtual void Update() override;
		virtual void Draw() override;
	};
}
