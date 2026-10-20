#pragma once
#include<entt.hpp>
#include<sol/sol.hpp>

namespace ENGINE_CORE::ECS {
	class Registry
	{
	private:
		std::unique_ptr<entt::registry> m_pRegistry;
	public:
		Registry();
		~Registry() = default;

		inline entt::registry& GetRegistry() { return *m_pRegistry; }
		inline entt::entity CreateEntity() { return m_pRegistry->create(); }
		inline void ClearRegistry() { m_pRegistry->clear(); }
        void Reset() {
            auto next = std::make_unique<entt::registry>();
            m_pRegistry->clear(); // Release component-owned resources before contexts.
            m_pRegistry.swap(next);
        }

		template <typename TContext>
		TContext AddToContext(TContext context);

		template <typename TContext>
		TContext& GetContext();

		template <typename TContext>
		bool RemoveContext();

		static void CreateLuaRegistryBind(sol::state& lua, Registry& state);

		template <typename TComponent>
		static void RegisterMetaComponent();
	};

	template <typename TComponent>
	entt::runtime_view& add_component_to_view(Registry* registry, entt::runtime_view& view);

	template <typename TComponent>
	entt::runtime_view& exclude_component_from_view(Registry* registry, entt::runtime_view& view);

}

#include "Registry.inl"
