#pragma once
#include <sol/sol.hpp>
#include <Physics/UserData.h>

namespace ENGINE_CORE::Scripting {
	struct UserDataBindings
	{
		static void CreateLuaUserDataBind(sol::state& lua);

		template <typename DATA>
		static void register_meta_user_data();
	};

	template <typename DATA>
	auto create_user_data(const sol::table& data, sol::this_state s);

	template <typename DATA>
	auto set_user_data(ENGINE_PHYSICS::UserData* pUserData, const sol::table& data, sol::this_state s);

	template <typename DATA>
	auto get_user_data(ENGINE_PHYSICS::UserData& userData, sol::this_state s);

}
#include "UserDataLuaBindings.inl"