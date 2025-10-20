#include "MetaUtilities.h"

/*get the id_type of comp, which is used to link between lua and meta*/
entt::id_type ENGINE_CORE::Utils::GetIdType(const sol::table& comp)
{
    // check if comp is registered to lua
    if (!comp.valid())
    {
        ENGINE_ERROR("Failed to get the type id -- Component has not been exposed to lua!");
        assert(comp.valid() && "Failed to get the type id -- Component has not been exposed to lua!");
        return -1;
    }

    // check if comp has a "type_id" function, which is used to link between lua and meta
    const auto func = comp["type_id"].get<sol::function>();
    assert(func.valid() &&
        "[type_id()] - function has not been exposed to lua!"
        "\nPlease ensure all components and types have a type_id function"
        "\nwhen creating the new usertype"
    );

    // return the id type
    return func.valid() ? func().get<entt::id_type>() : -1;
}
