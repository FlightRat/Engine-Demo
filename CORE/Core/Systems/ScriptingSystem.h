#pragma once
#include <sol/sol.hpp>

namespace ENGINE_CORE::ECS
{
	class Registry;
}

namespace ENGINE_CORE::Systems {
	class ScriptingSystem
	{
	private:
		bool m_bMainLoaded;
	public:
		ScriptingSystem();
		~ScriptingSystem() = default;

		bool LoadMainScript(ENGINE_CORE::ECS::Registry& registry, sol::state& lua);
		void Update(ENGINE_CORE::ECS::Registry& registry);
		void Render(ENGINE_CORE::ECS::Registry& registry);

		static void RegisterLuaBindings(sol::state& lua, ENGINE_CORE::ECS::Registry& registry);
		static void RegisterLuaFunctions(sol::state& lua);
	};
}