#version 450 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

uniform bool outline;
uniform mat4 model;
uniform mat3 normalMatrix;
layout (std140) uniform Matrices
{
    mat4 view;
    mat4 projection;
};

void main()
{
    vec4 worldPos = model * vec4(aPosition, 1.0);
    vec3 worldNormal  = normalize(normalMatrix * aNormal); 
    if(outline)
    {
        worldPos.xyz += worldNormal  * 0.05;
    }
    
    gl_Position = projection * view * worldPos;
}