#version 450 core
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 HitData;
in vec2 TexCoord;

layout(binding = 0) uniform sampler2D gPosition;
layout(binding = 1) uniform sampler2D gNormal;
layout(binding = 2) uniform sampler2D gMRA;
layout(binding = 3) uniform sampler2D sceneColor;
layout(binding = 4) uniform sampler2D depthPyramid;
layout(binding = 5) uniform sampler2D gAlbedo;
layout(binding = 6) uniform sampler2D iblSpecular;
layout(binding = 7) uniform sampler2D brdfLUT;
layout(binding = 8) uniform samplerCube environmentMap;
uniform vec3 viewPos;
uniform uint sampleIndex;

layout (std140) uniform Matrices
{
    mat4 view;
    mat4 projection;
};

uniform vec2 nearFar;
uniform bool debug_AO;

const int refinementSteps = 5;
const float maxDistance = 50.0;
const float normalBias = 0.02;
const float thickness = 0.1;
const float roughnessStart = 0.50;
const float roughnessEnd = 0.65;
const float edgeFade = 0.05;

bool IsInside(ivec2 pixel, ivec2 size)
{
    return all(greaterThanEqual(pixel, ivec2(0))) && all(lessThan(pixel, size));
}

vec3 ViewDifference(ivec2 pixel, ivec2 offset, ivec2 size, vec3 position)
{
    if (!IsInside(pixel + offset, size)) return vec3(0.0);
    vec4 neighbor = texelFetch(gPosition, pixel + offset, 0);
    if (neighbor.w == 0.0) return vec3(0.0);
    return (view * vec4(neighbor.xyz, 1.0)).xyz - position;
}

vec3 ContinuousDifference(vec3 forward, vec3 backward)
{
    float forwardLength = dot(forward, forward);
    float backwardLength = dot(backward, backward);
    if (forwardLength < 1e-12) return -backward;
    if (backwardLength < 1e-12) return forward;
    return forwardLength < backwardLength ? forward : -backward;
}

vec3 GeometryNormal(ivec2 pixel, ivec2 size, vec3 position, vec3 normal)
{
    vec3 dx = ContinuousDifference(ViewDifference(pixel, ivec2(1, 0), size, position),
                                   ViewDifference(pixel, ivec2(-1, 0), size, position));
    vec3 dy = ContinuousDifference(ViewDifference(pixel, ivec2(0, 1), size, position),
                                   ViewDifference(pixel, ivec2(0, -1), size, position));
    vec3 geometric = cross(dx, dy);
    float lengthSquared = dot(geometric, geometric);
    if (lengthSquared < 1e-12) return normal;
    geometric *= inversesqrt(lengthSquared);
    return dot(geometric, normal) < 0.0 ? -geometric : geometric;
}

// Perspective-correct screen-space DDA: McGuire/Mara, JCGT 3(4), 2014.
// https://jcgt.org/published/0003/04/04/paper.pdf
bool Trace(vec3 origin, vec3 direction, ivec2 sourcePixel, ivec2 size,
           out ivec2 hitPixel, out float hitDistance, out float facing)
{
    float rayLength = maxDistance;
    if (direction.z > 1e-6) rayLength = min(rayLength, (-nearFar.x - origin.z) / direction.z);
    if (direction.z < -1e-6) rayLength = min(rayLength, (-nearFar.y - origin.z) / direction.z);
    if (rayLength <= 0.0 || origin.z >= -nearFar.x) return false;

    vec4 h0 = projection * vec4(origin, 1.0);
    vec4 h1 = projection * vec4(origin + direction * rayLength, 1.0);
    float k0 = 1.0 / h0.w, k1 = 1.0 / h1.w;
    vec3 q0 = origin * k0;
    vec3 q1 = (origin + direction * rayLength) * k1;
    vec2 p0 = (h0.xy * k0 * 0.5 + 0.5) * vec2(size);
    vec2 p1 = (h1.xy * k1 * 0.5 + 0.5) * vec2(size);
    vec2 delta = p1 - p0;
    float extent = max(abs(delta.x), abs(delta.y));
    if (extent < 1e-4) return false;
    float inverseExtent = 1.0 / extent;

    // A one-pixel DDA crosses at most the viewport's major dimension.
    int maxSteps = max(size.x, size.y);
    int step = 0;
    int lastLevel = textureQueryLevels(depthPyramid) - 1;
    int level = min(4, lastLevel);
    vec2 pixelStep = delta * inverseExtent;
    int visitLimit = 2 * (lastLevel + 1) * (maxSteps + 1);
    for (int visit = 0; visit < visitLimit && step < maxSteps; ++visit) {
        float start = float(step) * inverseExtent;
        if (start >= 1.0) break;
        float end = min(float(step + 1) * inverseExtent, 1.0);
        vec2 samplePoint = p0 + delta * (0.5 * (start + end));
        ivec2 pixel = ivec2(floor(samplePoint));
        if (!IsInside(pixel, size)) break;
        ivec2 levelSize = textureSize(depthPyramid, level);
        ivec2 cell = ivec2(floor(samplePoint * vec2(levelSize) / vec2(size)));
        vec2 boundary = (vec2(cell) + vec2(greaterThanEqual(pixelStep, vec2(0.0)))) * vec2(size) / vec2(levelSize);
        vec2 exits = vec2(1e30);
        if (abs(pixelStep.x) > 1e-6) exits.x = (boundary.x - p0.x) / pixelStep.x;
        if (abs(pixelStep.y) > 1e-6) exits.y = (boundary.y - p0.y) / pixelStep.y;
        int after = min(maxSteps, max(step + 1, int(floor(min(exits.x, exits.y) - 0.5))));
        float rangeEnd = min(float(after) * inverseExtent, 1.0);
        float rayDepth0 = -mix(q0.z, q1.z, start) / mix(k0, k1, start);
        float rayDepth1 = -mix(q0.z, q1.z, rangeEnd) / mix(k0, k1, rangeEnd);
        vec2 range = texelFetch(depthPyramid, cell, level).rg;
        if (range.x > range.y || max(rayDepth0, rayDepth1) < range.x || min(rayDepth0, rayDepth1) > range.y + thickness) {
            step = after;
            level = min(level + 1, lastLevel);
            continue;
        }
        if (level > 0) { --level; continue; }
        ++step;
        level = min(1, lastLevel);
        if (all(equal(pixel, sourcePixel))) continue;

        vec4 worldPosition = texelFetch(gPosition, pixel, 0);
        if (worldPosition.w == 0.0) continue;
        float surfaceDepth = -(view * vec4(worldPosition.xyz, 1.0)).z;
        float depth0 = -mix(q0.z, q1.z, start) / mix(k0, k1, start);
        float depth1 = -mix(q0.z, q1.z, end) / mix(k0, k1, end);
        if (max(depth0, depth1) < surfaceDepth || min(depth0, depth1) > surfaceDepth + thickness) continue;

        // Refine only this candidate interval; revalidate its actual texel below.
        float low = start, high = end;
        float targetDepth = clamp(surfaceDepth, min(depth0, depth1), max(depth0, depth1));
        for (int refinement = 0; refinement < refinementSteps; ++refinement) {
            float middle = 0.5 * (low + high);
            float depth = -mix(q0.z, q1.z, middle) / mix(k0, k1, middle);
            if ((depth < targetDepth) == (depth1 > depth0)) low = middle;
            else high = middle;
        }
        float fraction = 0.5 * (low + high);
        hitPixel = ivec2(floor(p0 + delta * fraction));
        if (!IsInside(hitPixel, size) || all(equal(hitPixel, sourcePixel))) continue;
        vec4 hitWorld = texelFetch(gPosition, hitPixel, 0);
        if (hitWorld.w == 0.0) continue;
        vec3 hitPosition = (view * vec4(hitWorld.xyz, 1.0)).xyz;
        vec3 rayPosition = mix(q0, q1, fraction) / mix(k0, k1, fraction);
        float depthGap = hitPosition.z - rayPosition.z;
        if (depthGap < -0.001 || depthGap > thickness) continue;
        vec3 hitNormal = normalize(mat3(view) * texelFetch(gNormal, hitPixel, 0).xyz);
        hitNormal = GeometryNormal(hitPixel, size, hitPosition, hitNormal);
        facing = dot(hitNormal, -direction);
        if (facing <= 0.0) continue;
        hitDistance = length(rayPosition - origin);
        return true;
    }
    return false;
}

uint Hash(uint value)
{
    value ^= value >> 16; value *= 0x7feb352du;
    value ^= value >> 15; value *= 0x846ca68bu;
    return value ^ (value >> 16);
}

vec2 SampleSequence(ivec2 pixel)
{
    uint seed = Hash(uint(pixel.x) + 65537u * uint(pixel.y));
    vec2 rotation = vec2(seed & 0xffffu, seed >> 16) / 65536.0;
    float index = float(sampleIndex & 4095u) + 0.5;
    return fract(rotation + index * vec2(0.7548776662, 0.5698402910));
}

// GGX visible-normal sampling: Heitz, JCGT 7(4), 2018.
// https://www.jcgt.org/published/0007/04/01/paper.pdf
vec3 SampleDirection(vec3 normal, vec3 viewDir, float alpha, vec2 samplePoint)
{
    vec3 tangent = normalize(cross(abs(normal.z) < 0.999 ? vec3(0, 0, 1) : vec3(0, 1, 0), normal));
    vec3 bitangent = cross(normal, tangent);
    vec3 localView = vec3(dot(viewDir, tangent), dot(viewDir, bitangent), dot(viewDir, normal));
    vec3 stretched = normalize(vec3(alpha * localView.xy, localView.z));
    float radiusSquared = dot(stretched.xy, stretched.xy);
    vec3 axis1 = radiusSquared > 1e-12 ? vec3(-stretched.y, stretched.x, 0) * inversesqrt(radiusSquared) : vec3(1, 0, 0);
    vec3 axis2 = cross(stretched, axis1);
    float radius = sqrt(samplePoint.x), angle = 6.28318530718 * samplePoint.y;
    float x = radius * cos(angle), y = radius * sin(angle);
    float blend = 0.5 * (1.0 + stretched.z);
    y = mix(sqrt(max(0.0, 1.0 - x * x)), y, blend);
    vec3 disk = x * axis1 + y * axis2 + sqrt(max(0.0, 1.0 - x * x - y * y)) * stretched;
    vec3 halfNormal = normalize(vec3(alpha * disk.xy, max(0.0, disk.z)));
    vec3 worldHalf = tangent * halfNormal.x + bitangent * halfNormal.y + normal * halfNormal.z;
    return reflect(-viewDir, worldHalf);
}

vec3 SampleWeight(vec3 normal, vec3 viewDir, vec3 lightDir, vec3 F0, float alpha)
{
    float NdotV = max(dot(normal, viewDir), 1e-5), NdotL = dot(normal, lightDir);
    if (NdotL <= 0.0) return vec3(0);
    vec3 halfVector = normalize(viewDir + lightDir);
    vec3 F = F0 + (1.0 - F0) * pow(1.0 - max(dot(viewDir, halfVector), 0.0), 5.0);
    float lambdaV = 0.5 * (sqrt(1.0 + alpha * alpha * (1.0 - NdotV * NdotV) / (NdotV * NdotV)) - 1.0);
    float lambdaL = 0.5 * (sqrt(1.0 + alpha * alpha * (1.0 - NdotL * NdotL) / (NdotL * NdotL)) - 1.0);
    // BRDF * NdotL / p(L) for the matching visible-normal reflection PDF.
    return F * (1.0 + lambdaV) / (1.0 + lambdaV + lambdaL);
}

void main()
{
    FragColor = vec4(0.0);
    HitData = vec4(0.0);
    if (debug_AO) return;
    ivec2 size = textureSize(gPosition, 0);
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    vec4 worldPosition = texelFetch(gPosition, pixel, 0);
    if (worldPosition.w == 0.0) return;
    float roughness = texelFetch(gMRA, pixel, 0).y;
    if (roughness >= roughnessEnd) return;
    FragColor.a = 1.0;
    vec3 position = (view * vec4(worldPosition.xyz, 1.0)).xyz;
    vec3 normal = normalize(mat3(view) * texelFetch(gNormal, pixel, 0).xyz);
    vec3 geometric = GeometryNormal(pixel, size, position, normal);
    vec3 direction = reflect(normalize(position), normal);
    vec3 worldNormal = normalize(texelFetch(gNormal, pixel, 0).xyz);
    vec3 viewDir = normalize(viewPos - worldPosition.xyz);
    float transition = smoothstep(0.05, 0.10, roughness);
    float alpha = max(1e-4, roughness * roughness * transition);
    vec3 worldDirection = reflect(-viewDir, worldNormal);
    if (dot(worldNormal, viewDir) <= 0.0) return;
    if (roughness > 0.05) {
        worldDirection = SampleDirection(worldNormal, viewDir, alpha, SampleSequence(pixel));
        direction = mat3(view) * worldDirection;
    }
    if (dot(worldNormal, worldDirection) <= 0.0) return;
    if (dot(direction, geometric) <= 0.0) return;

    ivec2 hitPixel;
    float hitDistance, facing;
    if (!Trace(position + geometric * normalBias, direction, pixel, size, hitPixel, hitDistance, facing)) return;
    vec2 hitUV = (vec2(hitPixel) + 0.5) / vec2(size);
    float edge = min(min(hitUV.x, hitUV.y), min(1.0 - hitUV.x, 1.0 - hitUV.y));
    float confidence = smoothstep(0.0, edgeFade, edge);
    confidence *= 1.0 - smoothstep(0.8 * maxDistance, maxDistance, hitDistance);
    confidence *= smoothstep(0.0, 0.2, facing);
    confidence *= 1.0 - smoothstep(roughnessStart, roughnessEnd, roughness);
    HitData = vec4(hitUV, hitDistance, clamp(confidence, 0.0, 1.0));
    vec3 albedo = texelFetch(gAlbedo, pixel, 0).rgb;
    vec3 mra = texelFetch(gMRA, pixel, 0).rgb;
    float NdotV = max(dot(worldNormal, viewDir), 0.0);
    vec3 F0 = mix(vec3(0.04), albedo, mra.x);
    vec3 F = F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - NdotV, 5.0);
    vec2 brdf = texture(brdfLUT, vec2(NdotV, roughness)).rg;
    vec3 mirrorWeight = F * brdf.x + brdf.y;
    vec3 sampledWeight = SampleWeight(worldNormal, viewDir, worldDirection, F0, alpha);
    vec3 weight = mix(mirrorWeight, sampledWeight, transition);
    vec3 environment = textureLod(environmentMap, worldDirection, 0.0).rgb;
    vec3 baseline = mix(texelFetch(iblSpecular, pixel, 0).rgb, environment * sampledWeight, transition);
    FragColor.rgb = confidence * (texelFetch(sceneColor, hitPixel, 0).rgb * weight - baseline);
}
