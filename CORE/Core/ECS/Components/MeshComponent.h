#pragma once
#include<glm/glm.hpp>
#include<glad/glad.h>
#include<vector>
#include<string>
#include<sol/sol.hpp>
#include"../Registry.h"

namespace CORE::ECS {
    struct MeshComponent
    {
        std::string mesh;
        std::string shader;
        glm::vec4 color;
        int texture;
        
        bool bHidden{ false };

        std::vector<float> data;
        unsigned short vertexNum;
        GLuint m_VAO, m_VBO;

        void load_hud_quad();
        void load_plane();
        void load_cube();
        void load_mesh();
        
        inline void set_shader(const std::string mesh_shader) { shader = mesh_shader; }
        inline void set_color(const glm::vec4 mesh_color) { color = mesh_color; }
        inline void set_texture(const int mesh_texture) { texture = mesh_texture; }

        void Render();
        static void CreateLuaMeshBind(sol::state& lua);
    };
}