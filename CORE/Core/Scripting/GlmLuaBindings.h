#pragma once
#include<sol/sol.hpp>

namespace ENGINE_CORE::Scripting {
	struct GLMBindings
	{
		static void CreateLuaGlmBind(sol::state& lua);
	};
}