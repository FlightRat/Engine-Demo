#version 450 core
out float FragColor;
in vec2 TexCoord;

uniform sampler2D gPosition;    // 世界空间位置（从 GBuffer）
uniform sampler2D gNormal;      // 世界空间法线（从 GBuffer）
uniform sampler2D texNoise;     // 噪声纹理
uniform vec2 screenSize;        // 屏幕大小

int kernelSize = 64;
float radius = 0.5;
float bias = 0.025;

layout (std140) uniform Matrices
{
    mat4 view;
    mat4 projection;
};
layout (std140) uniform SSAO_samples    // 在切线空间的64个半球偏移采样点
{
    vec4 samples[64];
};

void main()
{
    vec2 noiseScale = screenSize / 4.0;

    // === 修复1：将位置和法线统一转换到【观察空间】 ===
    vec3 worldPos = texture(gPosition, TexCoord).xyz;
    vec3 fragPos = vec3(view * vec4(worldPos, 1.0));  // 世界 -> 观察空间

    vec3 worldNormal = normalize(texture(gNormal, TexCoord).rgb);
    vec3 normal = normalize(mat3(view) * worldNormal);  // 法线也转到观察空间

    // 由于噪声纹理4x4，纹理坐标是0~1,直接采样会重复在4x4中采样，需要根据屏幕缩放UV，然后自动在repeat的4x4纹理中采样
    vec3 randomVec = normalize(texture(texNoise, TexCoord * noiseScale).xyz);  

    // 构建 TBN（切线空间 → 观察空间）
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    for(int i = 0; i < kernelSize; ++i)
    {
        vec3 samplePos = TBN * samples[i].xyz;      // 观察空间的半球偏移量
        samplePos = fragPos + samplePos * radius;   // 观察空间的半球采样点

        // 投影到屏幕空间获取 UV
        vec4 offset = vec4(samplePos, 1.0);
        offset = projection * offset;
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        // 半球采样点在观察空间上的表面深度
        vec3 sampledWorldPos = texture(gPosition, offset.xy).xyz;
        float sampleDepth = (view * vec4(sampledWorldPos, 1.0)).z;  // 转到观察空间

        // 根据距离对遮蔽贡献进行衰减，防止远处的物体对近处的点产生影响
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));

        // 半球采样点在观察空间上的表面深度 VS 半球采样点在观察空间上的实际深度（类似shadowmap）
        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
    }

    // 平均采用点贡献并反转
    occlusion = 1.0 - (occlusion / kernelSize); 

    FragColor = occlusion;
}

// 原理简述：相机看向一个墙角，对于一个点，采样周围，用这些采样点的被遮蔽情况得出一个遮蔽系数，决定环境光的作用大小

// Q：怎么计算遮蔽情况？
// A：采样点在view space下的实际深度 VS 相机指向采样到达的表面深度