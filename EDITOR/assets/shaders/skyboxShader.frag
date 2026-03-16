#version 450 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;

void main()
{    
    vec3 result = vec3(texture(skybox, TexCoords));
    result = pow(result, vec3(1.0/2.2));    // gamma correction
    FragColor = vec4(result, 1.0);
}