#pragma once
#include<glm/glm.hpp>
#include<sol/sol.hpp>
#include<string>

namespace ENGINE_CORE::ECS {

	struct MeshFilter
	{
        std::string mesh="cube";
		bool changed = { false };

        static void CreateLuaMeshFilterBind(sol::state& lua);
	};
}