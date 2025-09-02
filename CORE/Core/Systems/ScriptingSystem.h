#pragma once
#include "../ECS/Registry.h"
#include <sol/sol.hpp>

namespace CORE::Systems {
	class ScriptingSystem
	{
	private:
		CORE::ECS::Registry& m_Registry;
		bool m_bMainLoaded;
	public:
		ScriptingSystem(CORE::ECS::Registry& registry);
		~ScriptingSystem() = default;

		bool LoadMainScript(sol::state& lua);
		void Update();
		void Render();
	};
}