#include "ContactListenerLuaBindings.h"
#include "../ECS/MetaUtilities.h"
#include "../ECS/Registry.h"
#include <Physics/UserData.h>
#include <Logger/Logger.h>

using namespace ENGINE_CORE::Utils;

namespace ENGINE_CORE::Scripting{
	std::vector<std::tuple<sol::reference, sol::reference>> ContactListenerBindings::GetContactPairs(ENGINE_PHYSICS::ContactListener& contactListener, sol::this_state s)
	{
		std::vector<std::tuple<sol::reference, sol::reference>> ObjectDataPairs;
		auto UserDataPairs = contactListener.GetContactPairs();
		for (const auto userDataPairs : UserDataPairs)
		{
			auto dataA = userDataPairs.first;
			auto dataB = userDataPairs.second;
			if (!dataA || !dataB)
				continue;
			assert(dataA->type_id != 0 && dataB->type_id != 0 && "User Data Type ID must be set!");

			using namespace entt::literals;
			const auto maybe_any_a = InvokeMetaFunction(
				static_cast<entt::id_type>(dataA->type_id),
				"get_user_data"_hs,
				*dataA, s
			);
			const auto maybe_any_b = InvokeMetaFunction(
				static_cast<entt::id_type>(dataB->type_id),
				"get_user_data"_hs,
				*dataB, s
			);

			if (maybe_any_a && maybe_any_b)
			{
				ObjectDataPairs.emplace_back(std::make_tuple(maybe_any_a.cast<sol::reference>(), maybe_any_b.cast<sol::reference>()));
			}
		}
		return ObjectDataPairs;
	}

	void ContactListenerBindings::CreateLuaContactListenerBind(sol::state& lua, ENGINE_CORE::ECS::Registry& registry)
	{
		auto& contactListener = registry.GetContext<std::shared_ptr<ENGINE_PHYSICS::ContactListener>>();
		if (!contactListener)
		{
			ENGINE_ERROR("Failed to bind the contact listener to Lua -- Not in the registry!");
			return;
		}

		lua.new_usertype<ENGINE_PHYSICS::ContactListener>(
			"ContactListener",
			sol::no_constructor,
			"GetUserDataPairs", [&](sol::this_state s) {
				return GetContactPairs(*contactListener, s);
			}
		);
	}
}


