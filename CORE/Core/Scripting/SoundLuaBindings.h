#pragma once
#include<sol/sol.hpp>
namespace ENGINE_CORE { namespace ECS{ class Registry; }}

namespace ENGINE_CORE::Scripting {
	struct SoundBindings
	{
		static void CreateSoundBindings(sol::state& lua, ENGINE_CORE::ECS::Registry& registry);
	};
}