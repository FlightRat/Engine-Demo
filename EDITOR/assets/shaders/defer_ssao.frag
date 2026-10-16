#version 450 core
out float FragColor;
in vec2 TexCoord;

layout(binding = 0) uniform sampler2D gPosition;
layout(binding = 1) uniform sampler2D gNormal;
layout(binding = 2) uniform sampler2D texNoise;

layout(std140) uniform Matrices
{
    mat4 view;
    mat4 projection;
};
layout(std140) uniform SSAO_samples
{
    vec4 samples[64];
};

const int kernelSize = 64;
const float radius = 0.5;
const float bias = 0.025;

void main()
{
    FragColor = 1.0;
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    vec4 worldPosition = texelFetch(gPosition, pixel, 0);
    if (worldPosition.w == 0.0) return;
    vec3 fragPos = (view * vec4(worldPosition.xyz, 1.0)).xyz;
    vec3 normal = mat3(view) * texelFetch(gNormal, pixel, 0).xyz;
    if (dot(normal, normal) < 1e-12) return;
    normal = normalize(normal);

    // HBAO stores jitter in noise.z; SSAO uses only the planar rotation.
    vec3 randomVec = vec3(texelFetch(texNoise, pixel % 4, 0).xy, 0.0);
    vec3 tangent = randomVec - normal * dot(randomVec, normal);
    if (dot(tangent, tangent) < 1e-12)
    {
        vec3 axis = abs(normal.z) < 0.9 ? vec3(0.0, 0.0, 1.0) : vec3(0.0, 1.0, 0.0);
        tangent = cross(axis, normal);
    }
    tangent = normalize(tangent);
    mat3 TBN = mat3(tangent, cross(normal, tangent), normal);

    float occlusion = 0.0;
    for (int i = 0; i < kernelSize; ++i)
    {
        vec3 samplePos = fragPos + TBN * samples[i].xyz * radius;
        vec4 offset = projection * vec4(samplePos, 1.0);
        if (offset.w <= 0.0) continue;
        vec3 coords = offset.xyz / offset.w * 0.5 + 0.5;
        if (any(lessThan(coords, vec3(0.0))) || any(greaterThanEqual(coords, vec3(1.0)))) continue;

        vec4 sampledWorldPos = texture(gPosition, coords.xy);
        if (sampledWorldPos.w == 0.0) continue;
        float sampleDepth = (view * vec4(sampledWorldPos.xyz, 1.0)).z;
        float rangeCheck = smoothstep(0.0, 1.0, radius / max(abs(fragPos.z - sampleDepth), 1e-6));
        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
    }
    FragColor = clamp(1.0 - occlusion / float(kernelSize), 0.0, 1.0);
}
