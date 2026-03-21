#version 450 core

out vec4 FragColor;

uniform vec3 color;

void main()
{
    vec3 result = color;
    result = pow(result, vec3(1.0/2.2));    // gamma correction
    FragColor = vec4(result, 1.0);
}