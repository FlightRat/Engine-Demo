#version 450 core
layout(location = 0) out float PreviousDepth;
layout(location = 1) out vec4 PreviousNormalRoughness;
layout(location = 2) out uint PreviousID;
layout(location = 3) out vec4 PreviousHit;
layout(binding = 0) uniform sampler2D gPosition;
layout(binding = 1) uniform sampler2D gNormal;
layout(binding = 2) uniform sampler2D gMRA;
layout(binding = 3) uniform usampler2D gObjectID;
layout(binding = 4) uniform sampler2D hitData;
layout(std140, binding = 0) uniform Matrices { mat4 view; mat4 projection; };

void main()
{
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    vec4 position = texelFetch(gPosition, pixel, 0);
    PreviousDepth = position.w == 0.0 ? 0.0 : -(view * vec4(position.xyz, 1.0)).z;
    PreviousNormalRoughness = vec4(texelFetch(gNormal, pixel, 0).xyz, texelFetch(gMRA, pixel, 0).y);
    PreviousID = texelFetch(gObjectID, pixel, 0).r;
    PreviousHit = texelFetch(hitData, pixel, 0);
}
