#version 450 core

out vec4 FragColor;

in VS_OUT {
    vec3 worldNormal;
} fs_in;

uniform vec3 color;
uniform vec3 direction;

void main()
{
	// This visualization shader receives base color only, not light intensity.
    vec3 N=normalize(fs_in.worldNormal);
    vec3 D=normalize(direction);
    vec3 result = dot(N, D) > 0.5 ? clamp(color, vec3(0.0), vec3(1.0)) : vec3(0.0);
    result = pow(result, vec3(1.0/2.2));    // gamma correction
    FragColor = vec4(result, 1.0);
}
