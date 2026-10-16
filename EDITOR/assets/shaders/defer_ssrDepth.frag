#version 450 core
layout(location = 0) out vec2 DepthRange;
layout(binding = 0) uniform sampler2D sourceTexture;
layout(std140, binding = 0) uniform Matrices
{
    mat4 view;
    mat4 projection;
};
uniform bool baseLevel;
uniform vec2 outputSize;

void main()
{
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    DepthRange = vec2(1e30, -1e30);
    if (baseLevel) {
        vec4 position = texelFetch(sourceTexture, pixel, 0);
        if (position.w != 0.0) DepthRange = vec2(-(view * vec4(position.xyz, 1.0)).z);
        return;
    }
    ivec2 sourceSize = textureSize(sourceTexture, 0);
    // Include every source texel intersecting the destination cell, including odd edges.
    ivec2 targetSize = ivec2(outputSize);
    ivec2 first = pixel * sourceSize / targetSize;
    ivec2 last = min(sourceSize, ((pixel + 1) * sourceSize + targetSize - 1) / targetSize);
    for (int y = first.y; y < last.y; ++y)
        for (int x = first.x; x < last.x; ++x) {
            vec2 range = texelFetch(sourceTexture, ivec2(x, y), 0).rg;
            DepthRange.x = min(DepthRange.x, range.x);
            DepthRange.y = max(DepthRange.y, range.y);
        }
}
