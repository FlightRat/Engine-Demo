#include "UserDataLuaBindings.h"
#include <entt.hpp>

using namespace ENGINE_PHYSICS;

namespace ENGINE_CORE::Scripting {

	template <typename DATA>
	void UserDataBindings::register_meta_user_data()
	{
		using namespace entt::literals;
		entt::meta_factory<DATA>()
			.type(entt::type_hash<DATA>::value())
			.template func<&create_user_data<DATA>>("create_user_data"_hs)
			.template func<&set_user_data<DATA>>("set_user_data"_hs)
			.template func<&get_user_data<DATA>>("get_user_data"_hs);
	}

	template <typename DATA>
	auto create_user_data(const sol::table& data, sol::this_state s)
	{
		auto newData = data.valid() ? data.as<DATA>() : DATA{};
		UserData userData{};
		userData.userData = newData;
		userData.type_id = entt::type_hash<DATA>::value();
		return sol::make_reference(s, userData);
	}

	template <typename DATA>
	auto set_user_data(ENGINE_PHYSICS::UserData* pUserData, const sol::table& data, sol::this_state s)
	{
		auto newData = data.valid() ? data.as<DATA>() : DATA{};
		pUserData->userData.reset();
		pUserData->userData = newData;
		pUserData->type_id = entt::type_hash<DATA>::value();
		return sol::make_reference(s, std::any_cast<DATA>(pUserData->userData));
	}

	template <typename DATA>
	auto get_user_data(ENGINE_PHYSICS::UserData& userData, sol::this_state s)
	{
		auto& data = userData.userData;
		return sol::make_reference(s, std::any_cast<DATA>(data));
	}
}