#version 450 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

uniform mat4 model;
layout (std140) uniform Matrices
{
    mat4 view;
    mat4 projection;
};

void main()
{
    FragPos = vec3(model * vec4(aPosition, 1.0));
    // 建议：transpose(inverse(model)) 最好在 CPU 端计算好传入，不要在 Shader 里算，性能开销大
    Normal = normalize(mat3(transpose(inverse(model))) * aNormal); 
    TexCoord = aTexCoord;
    gl_Position = projection * view * model * vec4(aPosition, 1.0f);
}