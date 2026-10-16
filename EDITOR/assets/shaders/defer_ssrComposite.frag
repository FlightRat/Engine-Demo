#version 450 core
out vec4 FragColor;
in vec2 TexCoord;
layout(binding = 0) uniform sampler2D sceneColor;
layout(binding = 2) uniform sampler2D screenReflection;
layout(binding = 3) uniform sampler2D gPosition;
layout(binding = 6) uniform sampler2D gMRA;
layout(binding = 7) uniform sampler2D screenAO;
uniform bool disable_AO;
uniform bool debug_AO;

void main()
{
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    vec3 result = texelFetch(sceneColor, pixel, 0).rgb;
    if (debug_AO) { FragColor = vec4(result, 1.0); return; }
    if (texelFetch(gPosition, pixel, 0).w != 0.0) {
        float ao = texelFetch(gMRA, pixel, 0).z;
        if (!disable_AO) ao *= texelFetch(screenAO, pixel, 0).r;
        result += ao * texelFetch(screenReflection, pixel, 0).rgb;
    }
    result = max(result, vec3(0));
    result = result / (result + vec3(1));
    result = pow(result, vec3(1.0 / 2.2));
    FragColor = vec4(result, 1.0);
}
