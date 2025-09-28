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
        glm::vec4 color{ glm::vec4{1.0f} };
        std::string type;
        std::vector<float> data;
        unsigned short vertexNum;
        GLuint m_VAO, m_VBO;

        void load_plane();
        void load_cube();
        void load_mesh();
        
        inline void set_color(const glm::vec4 mesh_color) { color = mesh_color; }

        void Render();
        static void CreateLuaMeshBind(sol::state& lua);
    };
}