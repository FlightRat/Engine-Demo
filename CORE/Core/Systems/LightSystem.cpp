#include "LightSystem.h"
#include "Logger/Logger.h"
#include "../ECS/Components/LightComponent.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <initializer_list>

namespace ENGINE_CORE::Systems {
    LightSystem::LightSystem(int max_dir_light, int max_point_light, int max_area_light) :
        MAX_POINT_LIGHTS{ max_point_light }, MAX_DIR_LIGHTS{ max_dir_light }, MAX_AREA_LIGHTS{ max_area_light }
    {
        // 构造时就分配好固定大小，后续 assign() 不会重新分配堆内存
        m_DirLightData.resize(MAX_DIR_LIGHTS);
        m_PointLightData.resize(MAX_POINT_LIGHTS);
        m_AreaLightData.resize(MAX_AREA_LIGHTS);
    }

    //LightSystem::Update()
    void LightSystem::Update(ENGINE_CORE::ECS::Registry& runtimeRegistry, const ENGINE_RENDERING::Camera3D& camera)
    {
        ACTIVATED_DIR_LIGHTS = 0;
        ACTIVATED_POINT_LIGHTS = 0;
        ACTIVATED_AREA_LIGHTS = 0;

        // 关键：先用"空/未激活"数据填满整个数组，再按实际灯光覆盖
        // 这样 Shader 里未使用的槽位 direction.w == 0.0，会被 continue 跳过
        m_DirLightData.assign(MAX_DIR_LIGHTS, ENGINE_RENDERING::DirLight{});   // w默认0
        m_PointLightData.assign(MAX_POINT_LIGHTS, ENGINE_RENDERING::PointLight{});
        m_AreaLightData.assign(MAX_AREA_LIGHTS, ENGINE_RENDERING::AreaLight{});

        auto lightView = runtimeRegistry.GetRegistry().view<ENGINE_CORE::ECS::LightComponent>();
        for (auto [_, light] : lightView.each())
        {
            if (light.type == "direction_light")
            {
                if (ACTIVATED_DIR_LIGHTS < MAX_DIR_LIGHTS)
                {
                    m_DirLightData[ACTIVATED_DIR_LIGHTS] = ENGINE_RENDERING::DirLight{
                        .color = glm::vec4(light.color, light.intensity),
                        .direction = glm::vec4(light.direction, 1.0f)
                    };
                    PrepareDirLightCSM(camera, m_DirLightData[ACTIVATED_DIR_LIGHTS]);
                    if (m_DirLightData[ACTIVATED_DIR_LIGHTS].params.z > 0.5f)
                        ACTIVATED_DIR_LIGHTS++;
                }
                else
                {
                    ENGINE_WARN("Too many direction lights! The limit is [{0}].", MAX_DIR_LIGHTS);
                }
            }
            else if (light.type == "area_light") {
                if (ACTIVATED_AREA_LIGHTS < MAX_AREA_LIGHTS)
                {
                    light.rotation_quat = glm::quat(glm::radians(light.rotation_eular));
                    glm::vec3 normalizedDirection = light.rotation_quat * glm::vec3(0.0f, -1.0f, 0.0f);
                    glm::vec3 rightVector = light.rotation_quat * glm::vec3(-1.0f, 0.0f, 0.0f);
                    glm::vec3 upVector = light.rotation_quat * glm::vec3(0.0f, 0.0f, 1.0f);
                    float near_plane = 2.0f, far_plane = 100.0f, fovY = glm::radians(90.0f), aspect = 1.0f;
                    float nearHalfHeight = near_plane * glm::tan(fovY * 0.5f);  // 透视投影近平面半高
                    float nearHalfWidth = nearHalfHeight * aspect;              // 透视投影近平面半宽
                    glm::mat4 lightViewMatrix = glm::lookAtRH(light.pos, light.pos+normalizedDirection, upVector);
                    glm::mat4 lightProjectionMatrix = glm::perspectiveRH_NO(fovY, aspect, near_plane, far_plane);
                    glm::mat4 lightSpaceMatrix = lightProjectionMatrix * lightViewMatrix;

                    // 直接写入对应槽位，而不是 push_back
                    m_AreaLightData[ACTIVATED_AREA_LIGHTS] = ENGINE_RENDERING::AreaLight{
                        .color = glm::vec4(light.color, light.intensity),
                        .center_pos = glm::vec4(light.pos, 1.0f),
                        .direction = glm::vec4(normalizedDirection, 1.0f),
                        .rightVector_halfWidth = glm::vec4(rightVector,light.half_width),
                        .upVector_halfHeight = glm::vec4(upVector,light.half_height),
                        .lightSpaceMatrix = lightSpaceMatrix,
                        .shadowParams = glm::vec4(near_plane,far_plane,nearHalfWidth,nearHalfHeight)
                    };
                    ACTIVATED_AREA_LIGHTS++;
                }
                else
                {
                    ENGINE_WARN("Too many area lights! The limit is [{0}].", MAX_AREA_LIGHTS);
                }
            }
            else if (light.type == "point_light")
            {
                if (ACTIVATED_POINT_LIGHTS < MAX_POINT_LIGHTS)
                {
                    m_PointLightData[ACTIVATED_POINT_LIGHTS] = ENGINE_RENDERING::PointLight{

                        .color = glm::vec4(light.color, light.intensity),
                        .position = glm::vec4(light.pos, 1.0f),
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
    void LightSystem::PrepareDirLightCSM(const ENGINE_RENDERING::Camera3D& camera, ENGINE_RENDERING::DirLight& dirLight)
    {
        const glm::vec3& direction = glm::vec3(dirLight.direction);
        const float lengthSquared = glm::dot(direction, direction);
        const float nearPlane = camera.GetNearPlane();
        const float farPlane = camera.GetFarPlane();
        if (!std::isfinite(lengthSquared) || lengthSquared <= 1e-12f ||
            camera.GetWidth() <= 0 || camera.GetHeight() <= 0 ||
            !std::isfinite(nearPlane) || !std::isfinite(farPlane) || nearPlane <= 0.0f || farPlane <= nearPlane ||
            !std::isfinite(camera.Zoom) || camera.Zoom <= 0.0f || camera.Zoom >= 179.0f)
            return;

        const float aspect = (float)camera.GetWidth() / (float)camera.GetHeight();
        const float tanHalfFov = glm::tan(glm::radians(camera.Zoom) * 0.5f);
        const glm::mat4 inverseView = glm::inverse(camera.GetViewMatrix());
        for (int column = 0; column < 4; column++)
            for (int row = 0; row < 4; row++)
                if (!std::isfinite(inverseView[column][row])) return;

        glm::vec3 lightDirection = glm::normalize(direction);
        glm::vec3 upVector = glm::abs(lightDirection.y) > 0.99f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
        // 仅使用旋转建立光源视图，使 texel 网格固定在世界空间。
        glm::mat4 lightView = glm::lookAtRH(glm::vec3(0.0f), lightDirection, upVector);
        float previousSplit = nearPlane;
        for (int index = 0; index < ENGINE_RENDERING::DIR_CASCADE_COUNT; index++)
        {
            float fraction = (float)(index + 1) / (float)ENGINE_RENDERING::DIR_CASCADE_COUNT;
            float logarithmic = nearPlane * glm::pow(farPlane / nearPlane, fraction);
            float uniform = nearPlane + (farPlane - nearPlane) * fraction;
            float split = index == ENGINE_RENDERING::DIR_CASCADE_COUNT - 1 ? farPlane : glm::mix(uniform, logarithmic, 0.5f);
            dirLight.splitDepths[index] = split;
            dirLight.blendStarts[index] = split - (split - previousSplit) * 0.1f;

            // 下一级级联包含上一级的完整混合区域。
            float sliceNear = index == 0 ? nearPlane : dirLight.blendStarts[index - 1];
            float centerDepth = (sliceNear + split) * 0.5f;
            glm::vec3 centerView(0.0f, 0.0f, -centerDepth);
            float radius = 0.0f;
            for (float depth : { sliceNear, split })
            {
                glm::vec3 corner(depth * tanHalfFov * aspect, depth * tanHalfFov, -depth);
                radius = (glm::max)(radius, glm::length(corner - centerView));
            }
            // 在相机空间计算半径，避免相机旋转改变级联范围。
            radius = glm::ceil(radius * 16.0f) / 16.0f;
            // 三个 texel 为 PCF、中心对齐和接收面法线偏移留出空间。
            float halfExtent = radius / (1.0f - 6.0f / (float)2048);
            float texelSize = 2.0f * halfExtent / (float)2048;
            glm::vec3 centerLight = glm::vec3(lightView * inverseView * glm::vec4(centerView, 1.0f));
            float centerX = glm::round(centerLight.x / texelSize) * texelSize;
            float centerY = glm::round(centerLight.y / texelSize) * texelSize;

            // 右手系光源视图沿 -Z 观察；上游投射物沿 +Z 延伸。
            float minZ = centerLight.z - radius - 1.0f;
            float maxZ = centerLight.z + radius + ENGINE_RENDERING::DIR_CASTER_DISTANCE + 1.0f;
            glm::mat4 lightProjection = glm::orthoRH_NO(
                centerX - halfExtent, centerX + halfExtent,
                centerY - halfExtent, centerY + halfExtent, 
                -maxZ, -minZ);
            dirLight.lightSpaceMatrices[index] = lightProjection * lightView;
            dirLight.texelSizes[index] = texelSize;
            dirLight.depthRanges[index] = maxZ - minZ;
            previousSplit = split;
        }
        dirLight.params = glm::vec4(nearPlane, farPlane, 1.0f, 0.0f);
    }
}


