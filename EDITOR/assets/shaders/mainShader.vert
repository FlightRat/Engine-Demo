#version 450 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

#define NR_DIR_LIGHTS 4

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoord;
    vec4 FragPosLightSpace[NR_DIR_LIGHTS];
} vs_out;

uniform mat4 model;
layout (std140) uniform Matrices
{
    mat4 view;
    mat4 projection;
};
uniform mat4 lightSpaceMatrices[NR_DIR_LIGHTS];

void main()
{
    vs_out.FragPos   = vec3(model * vec4(aPosition, 1.0));
    // 注意：法线矩阵建议 CPU 端预计算，避免 GPU 上 inverse() 的高开销
    vs_out.Normal    = normalize(mat3(transpose(inverse(model))) * aNormal);
    vs_out.TexCoord  = aTexCoord;

    // 为每个方向光计算其裁剪空间坐标
    for (int i = 0; i < NR_DIR_LIGHTS; i++) {
        vs_out.FragPosLightSpace[i] = lightSpaceMatrices[i] * vec4(vs_out.FragPos, 1.0);
    }

    gl_Position = projection * view * model * vec4(aPosition, 1.0);
}