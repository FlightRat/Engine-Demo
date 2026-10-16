#version 450 core
out vec4 Filtered;
layout(binding = 0) uniform sampler2D sourceCorrection;
layout(binding = 1) uniform sampler2D gPosition;
layout(binding = 2) uniform sampler2D gNormal;
layout(binding = 3) uniform sampler2D gMRA;
layout(binding = 4) uniform usampler2D gObjectID;
layout(binding = 5) uniform sampler2D moments;
uniform int sampleStride;

void main()
{
    ivec2 pixel = ivec2(gl_FragCoord.xy), size = textureSize(sourceCorrection, 0);
    vec4 center = texelFetch(sourceCorrection, pixel, 0);
    Filtered = center;
    float roughness = texelFetch(gMRA, pixel, 0).y;
    if (center.a <= 0.0 || roughness <= 0.05 || roughness >= 0.65) return;
    vec2 stats = texelFetch(moments, pixel, 0).rg;
    float deviation = sqrt(max(0.0, stats.y - stats.x * stats.x));
    float strength = smoothstep(0.05, 0.25, roughness);
    strength *= clamp(deviation / (0.05 + abs(stats.x)) + 1.0 / max(center.a, 1.0), 0.0, 1.0);
    if (strength < 0.001) return;
    vec3 position = texelFetch(gPosition, pixel, 0).xyz;
    vec3 normal = normalize(texelFetch(gNormal, pixel, 0).xyz);
    uint id = texelFetch(gObjectID, pixel, 0).r;
    vec3 sum = vec3(0);
    float total = 0.0;
    for (int y = -1; y <= 1; ++y) for (int x = -1; x <= 1; ++x) {
        ivec2 neighbor = pixel + ivec2(x, y) * sampleStride;
        if (any(lessThan(neighbor, ivec2(0))) || any(greaterThanEqual(neighbor, size))) continue;
        if (texelFetch(gObjectID, neighbor, 0).r != id) continue;
        vec4 value = texelFetch(sourceCorrection, neighbor, 0);
        if (value.a <= 0.0) continue;
        float neighborRoughness = texelFetch(gMRA, neighbor, 0).y;
        if (neighborRoughness <= 0.05 || abs(neighborRoughness - roughness) > 0.03) continue;
        float facing = max(dot(texelFetch(gNormal, neighbor, 0).xyz, normal), 0.0);
        vec3 delta = texelFetch(gPosition, neighbor, 0).xyz - position;
        float plane = abs(dot(delta, normal));
        float weight = float((x == 0 ? 2 : 1) * (y == 0 ? 2 : 1));
        weight *= pow(facing, 64.0) * exp(-plane / max(0.01, 0.02 * length(delta)));
        sum += value.rgb * weight;
        total += weight;
    }
    if (total > 0.0) Filtered.rgb = mix(center.rgb, sum / total, strength);
}
