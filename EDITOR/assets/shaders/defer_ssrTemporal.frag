#version 450 core
layout(location = 0) out vec4 Resolved;
layout(location = 1) out vec4 Moments;
layout(binding = 0) uniform sampler2D gPosition;
layout(binding = 1) uniform sampler2D gNormal;
layout(binding = 2) uniform sampler2D gMRA;
layout(binding = 3) uniform sampler2D gMotion;
layout(binding = 4) uniform usampler2D gObjectID;
layout(binding = 5) uniform sampler2D rawCorrection;
layout(binding = 6) uniform sampler2D currentHit;
layout(binding = 7) uniform sampler2D previousDepth;
layout(binding = 8) uniform sampler2D previousNormalRoughness;
layout(binding = 9) uniform usampler2D previousID;
layout(binding = 10) uniform sampler2D previousHit;
layout(binding = 11) uniform sampler2D historyCorrection;
layout(binding = 12) uniform sampler2D historyMoments;
layout(std140, binding = 0) uniform Matrices { mat4 view; mat4 projection; };
uniform mat4 previousViewProjection;
uniform mat4 inversePreviousView;
uniform mat4 inversePreviousProjection;
uniform bool historyValid;

float Luminance(vec3 color) { return dot(color, vec3(0.2126, 0.7152, 0.0722)); }
bool Inside(vec2 uv) { return all(greaterThanEqual(uv, vec2(0))) && all(lessThan(uv, vec2(1))); }
ivec2 Pixel(vec2 uv) { return ivec2(floor(uv * vec2(textureSize(gPosition, 0)))); }

vec3 PreviousWorld(vec2 uv, float depth)
{
    vec4 ray = inversePreviousProjection * vec4(uv * 2.0 - 1.0, 1, 1);
    vec3 position = ray.xyz * (depth / -ray.z);
    return (inversePreviousView * vec4(position, 1)).xyz;
}

float DepthTolerance(float depth, vec3 normal, vec3 world)
{
    vec3 direction = normalize((view * vec4(world, 1)).xyz);
    float grazing = 1.0 - abs(dot(mat3(view) * normal, direction));
    return max(0.025, 0.01 * depth) * (1.0 + 2.0 * grazing);
}

bool SurfaceCandidate(vec2 uv, uint id, vec3 normal, float roughness, float expectedDepth)
{
    if (!Inside(uv)) return false;
    ivec2 pixel = Pixel(uv);
    vec4 nr = texelFetch(previousNormalRoughness, pixel, 0);
    float depth = texelFetch(previousDepth, pixel, 0).r;
    if (depth <= 0.0 || texelFetch(previousID, pixel, 0).r != id) return false;
    if (dot(normal, nr.xyz) < 0.9 || abs(roughness - nr.w) > 0.03) return false;
    vec3 world = PreviousWorld(uv, depth);
    return abs(depth - expectedDepth) < DepthTolerance(expectedDepth, normal, world);
}

bool HitCandidate(vec2 uv, vec4 hit, vec3 movedHit, uint hitID, bool roughSample)
{
    vec4 oldHit = texelFetch(previousHit, Pixel(uv), 0);
    if (roughSample) return true;
    if ((oldHit.w > 0.0) != (hit.w > 0.0)) return false;
    if (hit.w <= 0.0) return true;
    if (!Inside(oldHit.xy)) return false;
    ivec2 source = Pixel(oldHit.xy);
    if (texelFetch(previousID, source, 0).r != hitID) return false;
    float depth = texelFetch(previousDepth, source, 0).r;
    if (depth <= 0.0) return false;
    vec3 oldWorld = PreviousWorld(oldHit.xy, depth);
    return distance(oldWorld, movedHit) < max(0.08, 0.015 * hit.z);
}

bool SourceUnchanged(vec2 oldUV, out vec2 currentUV)
{
    ivec2 oldPixel = Pixel(oldUV);
    uint id = texelFetch(previousID, oldPixel, 0).r;
    float oldDepth = texelFetch(previousDepth, oldPixel, 0).r;
    if (oldDepth <= 0.0 || id == 0xffffffffu) return false;
    vec3 oldWorld = PreviousWorld(oldUV, oldDepth);
    vec4 clip = projection * view * vec4(oldWorld, 1);
    if (clip.w <= 0.0) return false;
    currentUV = clip.xy / clip.w * 0.5 + 0.5;
    if (!Inside(currentUV)) return false;
    ivec2 pixel = Pixel(currentUV);
    if (texelFetch(gObjectID, pixel, 0).r != id) return false;
    vec4 motion = texelFetch(gMotion, pixel, 0);
    if (motion.w == 0.0) return false;
    vec3 world = texelFetch(gPosition, pixel, 0).xyz;
    vec4 previousClip = previousViewProjection * vec4(world, 1);
    if (previousClip.w <= 0.0) return false;
    vec2 staticPreviousUV = previousClip.xy / previousClip.w * 0.5 + 0.5;
    vec2 observedPreviousUV = (vec2(pixel) + 0.5) / vec2(textureSize(gPosition, 0)) + motion.xy;
    float objectMotion = length((observedPreviousUV - staticPreviousUV) * vec2(textureSize(gPosition, 0)));
    float currentDepth = -(view * vec4(world, 1)).z;
    return objectMotion < 0.25 && abs(currentDepth - clip.w) < max(0.05, 0.01 * clip.w);
}

void main()
{
    ivec2 pixel = ivec2(gl_FragCoord.xy), size = textureSize(gPosition, 0);
    vec4 raw = texelFetch(rawCorrection, pixel, 0);
    Resolved = vec4(raw.rgb, raw.a > 0.0 ? 1.0 : 0.0);
    float currentLuma = Luminance(raw.rgb);
    vec4 hit = texelFetch(currentHit, pixel, 0);
    Moments = vec4(currentLuma, currentLuma * currentLuma, hit.w > 0.0 ? hit.xy : vec2(-1));
    if (raw.a == 0.0 || !historyValid) return;

    vec4 motion = texelFetch(gMotion, pixel, 0);
    if (motion.w == 0.0) return;
    vec2 uv = (vec2(pixel) + 0.5) / vec2(size);
    vec2 surfaceUV = uv + motion.xy;
    vec3 normal = normalize(texelFetch(gNormal, pixel, 0).xyz);
    float roughness = texelFetch(gMRA, pixel, 0).y;
    uint id = texelFetch(gObjectID, pixel, 0).r;
    if (!SurfaceCandidate(surfaceUV, id, normal, roughness, motion.z)) return;

    bool roughSample = roughness > 0.05;
    vec3 movedHit = vec3(0);
    uint hitID = 0xffffffffu;
    bool sourceValid = hit.w <= 0.0;
    if (hit.w > 0.0 && Inside(hit.xy)) {
        ivec2 source = Pixel(hit.xy);
        vec4 sourceMotion = texelFetch(gMotion, source, 0);
        vec2 oldUV = hit.xy + sourceMotion.xy;
        hitID = texelFetch(gObjectID, source, 0).r;
        sourceValid = sourceMotion.w > 0.0 && Inside(oldUV);
        if (sourceValid) {
            movedHit = PreviousWorld(oldUV, sourceMotion.z);
            sourceValid = texelFetch(previousID, Pixel(oldUV), 0).r == hitID;
        }
    }
    if (!sourceValid) return;

    vec2 candidate = surfaceUV;
    // A local planar mirror offers a reflection-parallax correspondence, distinct from surface motion.
    if (!roughSample && hit.w > 0.0) {
        vec3 receiver = PreviousWorld(surfaceUV, motion.z);
        vec3 previousNormal = normalize(texelFetch(previousNormalRoughness, Pixel(surfaceUV), 0).xyz);
        ivec2 right = min(pixel + ivec2(1, 0), size - 1), up = min(pixel + ivec2(0, 1), size - 1);
        vec3 position = texelFetch(gPosition, pixel, 0).xyz;
        vec3 dx = texelFetch(gPosition, right, 0).xyz - position;
        vec3 dy = texelFetch(gPosition, up, 0).xyz - position;
        vec3 geometric = cross(dx, dy);
        bool plane = dot(geometric, geometric) > 1e-12 &&
                     abs(dot(normalize(geometric), normal)) > 0.98 &&
                     texelFetch(gObjectID, right, 0).r == id && texelFetch(gObjectID, up, 0).r == id;
        if (plane) {
            vec3 virtualHit = movedHit - 2.0 * dot(movedHit - receiver, previousNormal) * previousNormal;
            vec4 clip = previousViewProjection * vec4(virtualHit, 1);
            vec2 mirrorUV = clip.xy / clip.w * 0.5 + 0.5;
            if (clip.w > 0.0 && Inside(mirrorUV)) {
                ivec2 oldPixel = Pixel(mirrorUV);
                vec4 oldNR = texelFetch(previousNormalRoughness, oldPixel, 0);
                float oldDepth = texelFetch(previousDepth, oldPixel, 0).r;
                vec3 oldPosition = PreviousWorld(mirrorUV, oldDepth);
                if (oldDepth > 0.0 && texelFetch(previousID, oldPixel, 0).r == id &&
                    dot(oldNR.xyz, previousNormal) > 0.98 && abs(oldNR.w - roughness) < 0.03 &&
                    abs(dot(oldPosition - receiver, previousNormal)) < max(0.03, motion.z * 0.005) &&
                    HitCandidate(mirrorUV, hit, movedHit, hitID, false)) candidate = mirrorUV;
            }
        }
    }
    if (!HitCandidate(candidate, hit, movedHit, hitID, roughSample)) return;
    vec4 oldStats = texelFetch(historyMoments, Pixel(candidate), 0);
    if (roughSample && Inside(oldStats.zw)) {
        vec2 sourceUV;
        if (!SourceUnchanged(oldStats.zw, sourceUV)) return;
        // Keep a representative source across valid zero samples, without keeping stale moving sources.
        if (hit.w <= 0.0) Moments.zw = sourceUV;
    }

    vec3 minimum = raw.rgb, maximum = raw.rgb, mean = vec3(0), square = vec3(0);
    float count = 0.0;
    for (int y = -1; y <= 1; ++y) for (int x = -1; x <= 1; ++x) {
        ivec2 neighbor = pixel + ivec2(x, y);
        if (any(lessThan(neighbor, ivec2(0))) || any(greaterThanEqual(neighbor, size))) continue;
        if (texelFetch(gObjectID, neighbor, 0).r != id) continue;
        if (dot(texelFetch(gNormal, neighbor, 0).xyz, normal) < 0.9) continue;
        if (abs(texelFetch(gMRA, neighbor, 0).y - roughness) > 0.03) continue;
        vec3 value = texelFetch(rawCorrection, neighbor, 0).rgb;
        minimum = min(minimum, value); maximum = max(maximum, value);
        mean += value; square += value * value; count += 1.0;
    }
    mean /= max(count, 1.0);
    vec3 sigma = sqrt(max(square / max(count, 1.0) - mean * mean, vec3(0)));
    vec4 history = texelFetch(historyCorrection, Pixel(candidate), 0);
    if (history.a <= 0.0) return;
    vec2 oldMoments = oldStats.xy;
    float variance = max(0.0, oldMoments.y - oldMoments.x * oldMoments.x);
    if (roughSample) {
        vec3 radius = 3.0 * max(sigma, vec3(sqrt(variance)));
        minimum = min(minimum, mean - radius);
        maximum = max(maximum, mean + radius);
    }
    vec3 clipped = clamp(history.rgb, minimum, maximum);
    float deviation = abs(Luminance(clipped) - currentLuma);
    float responsiveness = 1.0 / (1.0 + deviation / (0.05 + sqrt(variance) + Luminance(sigma)));
    float weight = min(history.a / (history.a + 1.0), roughSample ? 0.97 : 0.94) * (roughSample ? 1.0 : responsiveness);
    weight *= 1.0 / (1.0 + 0.02 * length(motion.xy * vec2(size)));
    Resolved = vec4(mix(raw.rgb, clipped, weight), min(history.a + 1.0, 32.0));
    Moments.xy = mix(Moments.xy, oldMoments, weight);
}
