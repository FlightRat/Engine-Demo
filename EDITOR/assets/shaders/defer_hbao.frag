#version 450 core
out float FragColor;
in vec2 TexCoord;

layout(binding = 0) uniform sampler2D gPosition;
layout(binding = 2) uniform sampler2D texNoise;

layout (std140) uniform Matrices
{
    mat4 view;
    mat4 projection;
};

const float pi = 3.14159265359;
const int directionCount = 8;                   // 搜索八个方向
const int stepCount = 6;                        // 每个方向搜索六次
const float radius = 0.5;                       // 观察空间AO搜索半径
const float angleBias = 10.0 * pi / 180.0;
const float strength = 1.0;                     // AO强度

// 检查像素是否越界
bool IsInside(ivec2 pixel, ivec2 size){
    return all(greaterThanEqual(pixel, ivec2(0))) && all(lessThan(pixel, size));
}

vec3 ViewDifference(ivec2 pixel, ivec2 offset, ivec2 size, vec3 position){
    if (!IsInside(pixel + offset, size)) return vec3(0.0);
    vec4 neighbor = texelFetch(gPosition, pixel + offset, 0);
    if (neighbor.w == 0.0) return vec3(0.0);
    return (view * vec4(neighbor.xyz, 1.0)).xyz - position;
}

vec3 ContinuousDifference(vec3 forward, vec3 backward){
    float forwardLength = dot(forward, forward);
    float backwardLength = dot(backward, backward);
    if (forwardLength < 1e-12) return -backward;
    if (backwardLength < 1e-12) return forward;
    return forwardLength < backwardLength ? forward : -backward;
}

void main()
{
    FragColor = 1.0;
    ivec2 size = textureSize(gPosition, 0);
    ivec2 pixel = ivec2(gl_FragCoord.xy);

    // 着色点观察视角坐标
    vec4 worldPosition = texelFetch(gPosition, pixel, 0);
    if (worldPosition.w == 0.0) return;
    vec3 position = (view * vec4(worldPosition.xyz, 1.0)).xyz;

    // 将观察空间采样半径转换为像素空间采样半径
    float radiusPixels = radius * projection[1][1] * float(size.y) / (2.0 * -position.z);
    if (radiusPixels < 1.0) return;

    // 水平变化率dx（向右）
    vec3 dx = ContinuousDifference(
        ViewDifference(pixel, ivec2(1, 0), size, position),
        ViewDifference(pixel, ivec2(-1, 0), size, position));
    // 垂直变化率（向上）
    vec3 dy = ContinuousDifference(
        ViewDifference(pixel, ivec2(0, 1), size, position),
        ViewDifference(pixel, ivec2(0, -1), size, position));
    if (dot(dx, dx) < 1e-12 || dot(dy, dy) < 1e-12) return;

    float occlusion = 0.0;
    vec3 random = texelFetch(texNoise, pixel % 4, 0).xyz;
    float stepPixels = radiusPixels / float(stepCount + 1);
    // 遍历八个均匀采样方向
    for (int i = 0; i < directionCount; ++i)
    {
        // 这里实际是用八个方向的旋转矩阵来旋转随机向量
        float angle = 2.0 * pi * float(i) / float(directionCount);
        vec2 direction = vec2(cos(angle) * random.x - sin(angle) * random.y,
                              sin(angle) * random.x + cos(angle) * random.y);
        float horizon = 0.0;
        ivec2 previousOffset = ivec2(0);
        // 每个方向前进搜索6次
        for (int j = 0; j < stepCount; ++j)
        {
            // 每次前进都会带随机
            float distancePixels = 1.0 + (float(j) + random.z) * stepPixels;
            if (distancePixels > radiusPixels) break;
            // 采样offset
            ivec2 offset = ivec2(round(direction * distancePixels));
            if (all(equal(offset, previousOffset))) continue;
            previousOffset = offset;
            if (!IsInside(pixel + offset, size)) break;

            // 采样点世界坐标
            vec4 samplePosition = texelFetch(gPosition, pixel + offset, 0);
            if (samplePosition.w == 0.0) continue;
            // 着色点到采样点的观察空间向量
            vec3 delta = (view * vec4(samplePosition.xyz, 1.0)).xyz - position;
            float distanceSquared = dot(delta, delta);
            if (distanceSquared < 1e-12 || distanceSquared > radius * radius) continue;
            // 与观察平面的夹角sin
            float sampleSin = delta.z * inversesqrt(distanceSquared);

            // 着色点所在平面切向量
            vec3 tangent = dx * float(offset.x) + dy * float(offset.y);
            if (dot(tangent, tangent) < 1e-12) continue;
            // 切向量与观察平面夹角
            float tangentAngle = atan(tangent.z, length(tangent.xy));
            float tangentSin = sin(min(tangentAngle + angleBias, 0.5 * pi));

            // 着色点平面到采样点的仰角
            float elevation = max(sampleSin - tangentSin, 0.0);
            if (elevation > horizon)
            {
                float falloff = 1.0 - distanceSquared / (radius * radius);
                occlusion += (elevation - horizon) * falloff;
                horizon = elevation;
            }
        }
    }
    FragColor = clamp(1.0 - strength * occlusion / float(directionCount), 0.0, 1.0);
}
