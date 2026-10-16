#version 450 core
out float FragColor;
in vec2 TexCoord;

layout(binding = 0) uniform sampler2D aoInput;
layout(binding = 1) uniform sampler2D gPosition;
uniform vec2 direction;

layout (std140) uniform Matrices
{
    mat4 view;
    mat4 projection;
};

const int blurRadius = 3;
const float spatialSigma = 1.5;
const float depthSigma = 0.1;

void main()
{
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    ivec2 size = textureSize(aoInput, 0);
    vec4 center = texelFetch(gPosition, pixel, 0);
    FragColor = 1.0;
    if (center.w == 0.0) return;

    float centerDepth = -(view * vec4(center.xyz, 1.0)).z;
    float result = texelFetch(aoInput, pixel, 0).r;
    float weightSum = 1.0;
    for (int i = -blurRadius; i <= blurRadius; ++i)
    {
        if (i == 0) continue;
        ivec2 neighbor = pixel + ivec2(direction) * i;
        if (any(lessThan(neighbor, ivec2(0))) || any(greaterThanEqual(neighbor, size))) continue;
        vec4 samplePosition = texelFetch(gPosition, neighbor, 0);
        if (samplePosition.w == 0.0) continue;
        float sampleDepth = -(view * vec4(samplePosition.xyz, 1.0)).z;
        float depthDifference = (sampleDepth - centerDepth) / depthSigma;
        float weight = exp(-0.5 * (float(i * i) / (spatialSigma * spatialSigma)
                                   + depthDifference * depthDifference));
        result += texelFetch(aoInput, neighbor, 0).r * weight;
        weightSum += weight;
    }
    FragColor = result / weightSum;
}
