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
        glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
        std::vector<float> data;
        unsigned short vertexNum;
        GLuint m_VAO, m_VBO;

        void load_plane();
        void load_cube();
        void load_mesh(const std::string& mesh_type);
        
        inline void set_color(const glm::vec3 mesh_color) { color = mesh_color; }

        void Render();
        static void CreateMeshLuaBind(sol::state& lua);
    };
}