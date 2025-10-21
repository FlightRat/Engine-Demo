#pragma once
#include<glm/glm.hpp>
#include<sol/sol.hpp>
#include<vector>
#include<string>

namespace ENGINE_CORE::ECS {
    struct Vertex
    {
        glm::vec3 pos_;
        glm::vec3 normal_;
        glm::vec2 uv_;
    };

	struct MeshFilter
	{
        std::string mesh;
        std::vector<Vertex> vertex_data;
        std::vector<unsigned int> index_data;

        void load_mesh();
        void load_hud_quad();
        void load_plane();
        void load_cube();
        void load_sphere();
        void load_capsule();
        static void CreateLuaMeshFilterBind(sol::state& lua);
	};
}