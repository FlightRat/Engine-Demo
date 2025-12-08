#pragma once
#include "Core/ECS/Components/TransformComponent.h"
#include "Core/ECS/Components/PhysicsComponent.h"
#include "Core/ECS/Components/MeshFilter.h"
#include "Core/ECS/Components/MeshRender.h"

namespace ENGINE_CORE::ECS {class Entity;}

namespace ENGINE_EDITOR {
	class ComponentDrawer
	{
	public:
		ComponentDrawer() = delete;

		template <typename Tcomponent>
		static void DrawEntityComponentInfo(ENGINE_CORE::ECS::Entity& entity);

		template <typename TComponent>
		static void DrawComponentInfo(TComponent& component);

		template <typename TComponent>
		static void RegisterUIComponent();

	private:
		static void DrawImGuiComponent(ENGINE_CORE::ECS::TransformComponent& transform);
		static void DrawImGuiComponent(ENGINE_CORE::ECS::MeshFilter& meshFilter);
		static void DrawImGuiComponent(ENGINE_CORE::ECS::MeshRender& meshRender);
		static void DrawImGuiComponent(ENGINE_CORE::ECS::PhysicsComponent& physics);
	};

}
#include "ComponentDrawer.inl"