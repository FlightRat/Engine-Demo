#pragma once
#include<sol/sol.hpp>
namespace CORE { namespace ECS{ class Registry; }}

namespace CORE::Scripting {
	struct SoundBindings
	{
		static void CreateSoundBindings(sol::state& lua, CORE::ECS::Registry& registry);
	};
}