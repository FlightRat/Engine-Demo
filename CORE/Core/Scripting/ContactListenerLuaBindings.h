#pragma once
#include <sol/sol.hpp>
#include <Physics/ContactListener.h>

namespace ENGINE_CORE { namespace ECS { class Registry; } }

namespace ENGINE_CORE::Scripting {
	class ContactListenerBindings
	{
	private:
		static std::vector<std::tuple<sol::reference, sol::reference>> GetContactPairs(ENGINE_PHYSICS::ContactListener& contactListener, sol::this_state s);

	public:
		static void CreateLuaContactListenerBind(sol::state& lua, ENGINE_CORE::ECS::Registry& registry);
	};
}