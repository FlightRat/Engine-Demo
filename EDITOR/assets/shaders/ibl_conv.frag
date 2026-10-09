#version 450 core
out vec4 FragColor;
in vec3 WorldPos;

layout(binding = 0) uniform samplerCube environmentMap;

const float PI = 3.14159265359;

void main()
{
    vec3 N = normalize(WorldPos);
    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, N));
    up = normalize(cross(N, right));

    float sampleDelta = 0.025;
    float nrSamples = 0.0;
    vec3 irradiance = vec3(0.0);

    for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta) {
        for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta) {
            vec3 tangle_coord = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            vec3 world_coord =tangle_coord.x * right + tangle_coord.y * up + tangle_coord.z * N;
            irradiance += texture(environmentMap, world_coord).rgb* cos(theta) * sin(theta);
            nrSamples++;
        }
    }
    irradiance = PI * irradiance / nrSamples;
    FragColor = vec4(irradiance, 1.0);
}