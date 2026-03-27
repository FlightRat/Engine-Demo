#version 450 core
out float FragColor;
in vec2 TexCoord;

uniform sampler2D gPosition;   // 世界空间位置（从 GBuffer）
uniform sampler2D gNormal;     // 世界空间法线（从 GBuffer）
uniform sampler2D texNoise;
uniform vec2 screenSize;
uniform vec3 samples[64];

int kernelSize = 64;
float radius = 0.5;
float bias = 0.025;

layout (std140) uniform Matrices
{
    mat4 view;
    mat4 projection;
};

void main()
{
    vec2 noiseScale = screenSize / 4.0;

    // === 修复1：将位置和法线统一转换到【观察空间】 ===
    vec3 worldPos = texture(gPosition, TexCoord).xyz;
    vec3 fragPos = vec3(view * vec4(worldPos, 1.0));  // 世界 -> 观察空间

    vec3 worldNormal = normalize(texture(gNormal, TexCoord).rgb);
    vec3 normal = normalize(mat3(view) * worldNormal);  // 法线也转到观察空间

    vec3 randomVec = normalize(texture(texNoise, TexCoord * noiseScale).xyz);

    // 构建 TBN（现在在观察空间中）
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    for(int i = 0; i < kernelSize; ++i)
    {
        // 采样点位置（观察空间）
        vec3 samplePos = TBN * samples[i];
        samplePos = fragPos + samplePos * radius;

        // 投影到屏幕空间获取 UV
        vec4 offset = vec4(samplePos, 1.0);
        offset = projection * offset;
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        // === 修复2：采样到的世界空间位置也必须转换到观察空间再取 z ===
        vec3 sampledWorldPos = texture(gPosition, offset.xy).xyz;
        float sampleDepth = (view * vec4(sampledWorldPos, 1.0)).z;  // 转到观察空间

        // === 修复3：rangeCheck 也要用观察空间的深度 ===
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
    }
    occlusion = 1.0 - (occlusion / kernelSize);

    FragColor = occlusion;
}