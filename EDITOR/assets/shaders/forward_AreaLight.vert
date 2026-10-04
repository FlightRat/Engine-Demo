#version 450 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out VS_OUT {
    vec3 worldNormal;
} vs_out;

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
    vs_out.worldNormal  = normalize(normalMatrix * aNormal); 
    if(outline)
    {
        worldPos.xyz += vs_out.worldNormal  * 0.05;
    }
    
    gl_Position = projection * view * worldPos;
}