#pragma once
#include<sol/sol.hpp>

namespace CORE::Scripting {
	struct GLMBindings
	{
		static void CreateGLMBindings(sol::state& lua);
	};
}