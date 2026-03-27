#version 450 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

#define NR_DIR_LIGHTS 4

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoord;
    mat3 TBN;
} vs_out;

uniform mat4 model;
layout (std140) uniform Matrices
{
    mat4 view;
    mat4 projection;
};

void main()
{
    vs_out.FragPos   = vec3(model * vec4(aPosition, 1.0));
    // 注意：法线矩阵建议 CPU 端预计算，避免 GPU 上 inverse() 的高开销
    vs_out.Normal    = normalize(mat3(transpose(inverse(model))) * aNormal);
    vs_out.TexCoord  = aTexCoord;

    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 B = normalize(normalMatrix * aBitangent);
    vec3 N = normalize(normalMatrix * aNormal);
    mat3 TBN = mat3(T, B, N);
    vs_out.TBN = TBN;

    gl_Position = projection * view * model * vec4(aPosition, 1.0);
}