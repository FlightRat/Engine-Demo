#pragma once
#include<sol/sol.hpp>

namespace ENGINE_CORE::Scripting {
	struct GLMBindings
	{
		static void CreateGLMBindings(sol::state& lua);
	};
}