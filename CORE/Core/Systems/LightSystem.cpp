#include "LightSystem.h"
#include "Logger/Logger.h"
#include "../ECS/Components/LightComponent.h"
#include<glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace ENGINE_CORE::Systems {
    LightSystem::LightSystem(int max_dir_light, int max_point_light) :
        MAX_POINT_LIGHTS{ max_point_light }, MAX_DIR_LIGHTS{ max_dir_light }
    {
        // 构造时就分配好固定大小，后续 assign() 不会重新分配堆内存
        m_DirLightData.resize(MAX_DIR_LIGHTS);
        m_PointLightData.resize(MAX_POINT_LIGHTS);
    }

    //LightSystem::Update() 修复版
    void LightSystem::Update(ENGINE_CORE::ECS::Registry& runtimeRegistry)
    {
        ACTIVATED_DIR_LIGHTS = 0;
        ACTIVATED_POINT_LIGHTS = 0;

        // 关键：先用"空/未激活"数据填满整个数组，再按实际灯光覆盖
        // 这样 Shader 里未使用的槽位 direction.w == 0.0，会被 continue 跳过
        m_DirLightData.assign(MAX_DIR_LIGHTS, ENGINE_RENDERING::DirLight{});   // w默认0
        m_PointLightData.assign(MAX_POINT_LIGHTS, ENGINE_RENDERING::PointLight{});

        auto lightView = runtimeRegistry.GetRegistry().view<ENGINE_CORE::ECS::LightComponent>();
        for (auto [_, light] : lightView.each())
        {
            if (light.type == "direction_light")
            {
                if (ACTIVATED_DIR_LIGHTS < MAX_DIR_LIGHTS)
                {
                    // calculate lightSpaceMatrix
                    glm::vec3 dirLightPos = glm::vec3(0.0f) - (light.direction * 10.0f);
                    float near_plane = 0.1f, far_plane = 50.0f;
                    glm::vec3 upVector = glm::abs(light.direction.y) > 0.99f
                        ? glm::vec3(0.0f, 0.0f, 1.0f)
                        : glm::vec3(0.0f, 1.0f, 0.0f);
                    glm::mat4 lightViewMatrix = glm::lookAt(dirLightPos, glm::vec3(0.0f), upVector);
                    glm::mat4 lightProjectionMatrix = glm::ortho(-40.0f, 40.0f, -40.0f, 40.0f, near_plane, far_plane);
                    glm::mat4 lightSpaceMatrix = lightProjectionMatrix * lightViewMatrix;

                    // 直接写入对应槽位，而不是 push_back
                    m_DirLightData[ACTIVATED_DIR_LIGHTS] = ENGINE_RENDERING::DirLight{
                        .direction = glm::vec4(light.direction, 1.0f), // w=1.0 → 激活标志
                        .diffuse = glm::vec4(light.diffuse,   1.0f),
                        .specular = glm::vec4(light.specular,  1.0f),
                        .ambient = glm::vec4(light.ambient,   1.0f),
                        .lightSpaceMatrix = lightSpaceMatrix
                    };
                    ACTIVATED_DIR_LIGHTS++;
                }
                else
                {
                    ENGINE_WARN("Too many direction lights! The limit is [{0}].", MAX_DIR_LIGHTS);
                }
            }
            else if (light.type == "point_light")
            {
                if (ACTIVATED_POINT_LIGHTS < MAX_POINT_LIGHTS)
                {
                    m_PointLightData[ACTIVATED_POINT_LIGHTS] = ENGINE_RENDERING::PointLight{

                        .position = glm::vec4(light.pos,      1.0f),
                        .diffuse = glm::vec4(light.diffuse,  1.0f),
                        .specular = glm::vec4(light.specular, 1.0f),
                        .ambient = glm::vec4(light.ambient,  1.0f),
                        .attenuation = glm::vec4(light.constant, light.linear, light.quadratic, 0.0f),
                        //.render = light.render
                    };
                    ACTIVATED_POINT_LIGHTS++;
                }
                else
                {
                    ENGINE_WARN("Too many point lights! The limit is [{0}].", MAX_POINT_LIGHTS);
                }
            }
        }
    }
}


