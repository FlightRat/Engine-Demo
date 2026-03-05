#include "MeshFilter.h"
#include <entt.hpp>

void ENGINE_CORE::ECS::MeshFilter::CreateLuaMeshFilterBind(sol::state& lua)
{
    lua.new_usertype<MeshFilter>(
        "MeshFilter",
        "type_id", &entt::type_hash<MeshFilter>::value,
        sol::call_constructor,
        sol::factories(
            [&](const std::string& mesh) {
                MeshFilter MF{
                    .mesh = mesh,
                };
                return MF;
            }
        )
    );
}
