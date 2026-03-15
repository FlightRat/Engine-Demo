#version 450 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 TexCoords;

uniform mat4 model;
layout (std140) uniform Matrices
{
    mat4 view;
    mat4 projection;
};

void main()
{
    TexCoords = aPosition;
    vec4 pos = projection * view * model * vec4(aPosition, 1.0);
    gl_Position = pos.xyww;
}