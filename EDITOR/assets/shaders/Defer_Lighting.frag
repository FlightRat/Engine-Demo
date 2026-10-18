#version 450 core
#define MAX_DIR_SHADOWS   1
#define MAX_POINT_SHADOWS 5
#define MAX_AREA_SHADOWS  1
#define DIR_CASCADE_COUNT 4

const uint TILE_SIZE = 16u;
const uint MAX_TILE_LIGHTS = 128u;

struct DirLight {
    vec4 color;
    vec4 direction;
    mat4 lightSpaceMatrices[DIR_CASCADE_COUNT];
    vec4 splitDepths;
    vec4 blendStarts;
    vec4 texelSizes;
    vec4 depthRanges;
    vec4 params;
};
struct PointLight {
    vec4 color;
    vec4 position;
    vec4 attenuation;
};
struct AreaLight {
	vec4 color;
	vec4 center_pos;
	vec4 direction;
	vec4 rightVector_halfWidth;
	vec4 upVector_halfHeight;
	mat4 lightSpaceMatrix;
    vec4 shadowParams;
};
struct TileHeader {
    uint count;
    float minDepth;
    float maxDepth;
    uint validPixelCount;
};

out vec4 FragColor;
in vec2 TexCoord;

uniform vec3 viewPos;
uniform float far_plane;
uniform int dirLightCount;
uniform int pointLightCount;
uniform int areaLightCount;

layout(binding = 0) uniform sampler2D gPosition;
layout(binding = 1) uniform sampler2D gNormal;
layout(binding = 2) uniform sampler2D gAlbedo;
layout(binding = 3) uniform sampler2D gMRA;
layout(binding = 4) uniform sampler2D screenAO;
layout(binding = 5) uniform samplerCube irradianceMap; 
layout(binding = 6) uniform samplerCube prefilterMap; 
layout(binding = 7) uniform sampler2D brdfLUT; 
layout(binding = 8) uniform sampler2DArray dirLight_CSM[MAX_DIR_SHADOWS];
layout(binding = 9) uniform usampler2D areaMomentSAT[MAX_AREA_SHADOWS];
layout(binding = 10) uniform usampler2D areaCoverageSAT[MAX_AREA_SHADOWS];
layout(binding = 11) uniform samplerCube pointLight_ShadowMap[MAX_POINT_SHADOWS];

layout (std140) uniform Matrices {
    mat4 view;
    mat4 projection;
};

layout(std430, binding=1) readonly buffer DirLights{ DirLight dir_lights[]; };
layout(std430, binding=2) readonly buffer PointLights{ PointLight point_lights[]; };
layout(std430, binding=3) readonly buffer AreaLights{ AreaLight area_lights[]; };
layout(std430, binding=4) readonly buffer TileHeaders{ TileHeader tileHeaders[];};
layout(std430, binding=5) readonly buffer TileLightIndices { uint tileLightIndices[]; };

const float PI = 3.14159265359;
const float VSSM_MOMENT_SCALE = 16777215.0;
const float VSSM_MIN_VARIANCE = 1e-7;
const float VSSM_BLEED_REDUCTION = 0.2;

// Cook-Torrance BRDF
vec3 fresnelSchlick(float cosTheta, vec3 F0);
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness);
float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
// VSSM
float DepthToDistance(float depth, float nearPlane, float farPlane);
vec2 ComputeReceiverDepthGradient(AreaLight areaLight, vec3 receiverPos, vec3 receiverNormal);
vec2 ComputeBlockerSearchRadiusUV(AreaLight areaLight, float receiverZ);
vec2 ComputePenumbraRadiusUV(AreaLight areaLight, float receiverZ, float avgBlockerZ);
uvec2 SubtractMomentSum(uvec2 a, uvec2 b);
uvec4 ReadVSSMPrefix(usampler2D table, ivec2 pixel);
ivec4 VSSMRectangle(vec2 uv, vec2 radius, ivec2 size);
vec3 VSSMRegionMoments(usampler2D momentSAT, usampler2D coverageSAT, ivec4 rect);
float VSSMReceiverDepth(AreaLight areaLight, vec3 coords, vec2 depthGradient, float bias, ivec4 rect, ivec2 size);
float VSSMVisibility(vec3 moments, float receiverQ, int area);
// CSM
int SelectCascade(DirLight dirLight, float viewDepth);
float CascadeBlend(DirLight dirLight, int layer, float viewDepth);
float SampleCascade(DirLight dirLight, sampler2DArray shadowMapCascaded, int layer, vec3 fragPos, vec3 normal);
// 阴影系数计算
float ShadowCalculation_DIR(DirLight dirLight, sampler2DArray shadowMapCascaded, vec3 fragPos, vec3 normal);
float ShadowCalculation_point(PointLight pointLight, samplerCube shadowCubeMap, vec3 fragPos, vec3 normal);
float ShadowCalculation_area(AreaLight areaLight, usampler2D momentSAT, usampler2D coverageSAT, vec3 fragPos, vec3 normal);
// 直接光照计算（不含 ambient，只返回 Lo）
vec3 CalcDirLight(DirLight dirLight, vec3 fragPos, vec3 viewDir, vec3 normal, vec3 albedo, vec3 mra, float shadow);
vec3 CalcPointLight(PointLight pointLight, vec3 fragPos, vec3 viewDir, vec3 normal, vec3 albedo, vec3 mra, float shadow);
vec3 CalcAreaLight(AreaLight areaLight, vec3 fragPos, vec3 viewDir, vec3 normal, vec3 albedo, vec3 mra, float shadow);

vec3 EvaluatePointLight(int index, vec3 fragPos, vec3 viewDir, vec3 normal, vec3 albedo, vec3 mra){
    PointLight light = point_lights[index];
    float range = light.attenuation.w;
    vec3 offset = light.position.xyz - fragPos;

    if (range <= 0.0 || dot(offset, offset) >= range * range)
        return vec3(0.0);

    if (!any(greaterThan(light.attenuation.xyz, vec3(0.0))))
        return vec3(0.0);

    float shadow = 0.0;

    switch (index)
    {
        case 0:
            shadow = ShadowCalculation_point(
                light, pointLight_ShadowMap[0], fragPos, normal);
            break;
        case 1:
            shadow = ShadowCalculation_point(
                light, pointLight_ShadowMap[1], fragPos, normal);
            break;
        case 2:
            shadow = ShadowCalculation_point(
                light, pointLight_ShadowMap[2], fragPos, normal);
            break;
        case 3:
            shadow = ShadowCalculation_point(
                light, pointLight_ShadowMap[3], fragPos, normal);
            break;
        case 4:
            shadow = ShadowCalculation_point(
                light, pointLight_ShadowMap[4], fragPos, normal);
            break;
    }

    return CalcPointLight(
        light, fragPos, viewDir, normal, albedo, mra, shadow);
}

// ==================== 主函数 ====================
void main()
{   
    vec4 positionData = texture(gPosition, TexCoord);
    if (positionData.w < 0.5){
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec3 FragPos = positionData.xyz;
    vec3 Normal  = texture(gNormal, TexCoord).rgb;
    vec3 Albedo  = texture(gAlbedo, TexCoord).rgb;
    vec3 MRA     = texture(gMRA, TexCoord).rgb;

    float metallic  = MRA.x;
    float roughness = MRA.y;
    float ao        = MRA.z;  // 材质贴图中的AO

    // 重建原始法线
    vec3 dPdx = dFdx(FragPos);
    vec3 dPdy = dFdy(FragPos);
    vec3 receiverNormal = cross(dPdx, dPdy);
    float receiverNormalLength = dot(receiverNormal, receiverNormal);
    if(receiverNormalLength > 1e-12)receiverNormal *= inversesqrt(receiverNormalLength);
    else receiverNormal=Normal; 
    if (dot(receiverNormal, Normal) < 0.0) receiverNormal = -receiverNormal;

    // Screen-space AO: use the same final visibility for lighting and debug.
    const float aoPower = 1.0;
    float aoFactor = pow(texture(screenAO, TexCoord).r, aoPower);

    vec3 ViewDir = normalize(viewPos - FragPos);
    vec3 ReflectDir = reflect(-ViewDir, Normal); 

    // 累加所有直接光照（不含 ambient）
    vec3 Lo = vec3(0.0);

    // ---------- 方向光 ----------
    for (int i = 0; i < dirLightCount; i++) {
        if (dir_lights[i].params.z < 0.5) continue;
        float shadow = i < MAX_DIR_SHADOWS ? ShadowCalculation_DIR(dir_lights[i], dirLight_CSM[i], FragPos, receiverNormal) : 0.0;
        Lo += CalcDirLight(dir_lights[i], FragPos, ViewDir, Normal, Albedo, MRA, shadow);
    }

    // ---------- 点光源 ----------
    uvec2 tile = uvec2(gl_FragCoord.xy) / TILE_SIZE;
    uint width = uint(textureSize(gPosition, 0).x);
    uint tileCountX = (width + TILE_SIZE - 1u) / TILE_SIZE;
    uint tileIndex = tile.y * tileCountX + tile.x;
    uint candidateCount = tileHeaders[tileIndex].count;
    for (uint slot = 0u; slot < candidateCount; slot++)
    {
        uint lightIndex = tileLightIndices[tileIndex * MAX_TILE_LIGHTS + slot];
        Lo += EvaluatePointLight(int(lightIndex), FragPos, ViewDir, Normal, Albedo, MRA);
    }

    // ---------- 面光源 ----------
    for (int i = 0; i < areaLightCount; i++) {
        if (area_lights[i].direction.w < 0.5) continue;
        float shadow = i < MAX_AREA_SHADOWS ? ShadowCalculation_area(area_lights[i], areaMomentSAT[i], areaCoverageSAT[i], FragPos,receiverNormal) : 0.0;
        Lo += CalcAreaLight(area_lights[i], FragPos, ViewDir, Normal, Albedo, MRA, shadow);
    }

    // ---------- 环境光 ----------
    vec3 F0 = mix(vec3(0.04), Albedo, metallic);
    // 使用带粗糙度的菲涅尔近似，使粗糙表面环境高光更柔和
    vec3 F = fresnelSchlickRoughness(max(dot(Normal, ViewDir), 0.0), F0, roughness);
    vec3 kS_ambient = F;
    vec3 kD_ambient = (1.0 - kS_ambient) * (1.0 - metallic);

    // IBL irradiance map diffuse
    vec3 irradiance = texture(irradianceMap, Normal).rgb;
    vec3 ambientDiffuse = irradiance * Albedo;
    
    // IBL speculuar
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(prefilterMap, ReflectDir,  roughness * MAX_REFLECTION_LOD).rgb;    
    vec2 brdf = texture(brdfLUT, vec2(max(dot(Normal, ViewDir), 0.0), roughness)).rg;
    vec3 ambientSpecular = prefilteredColor * (F * brdf.x + brdf.y);

    // 最终环境光 = (漫反射 + 高光) * AO * HBAO
    vec3 ambient = (kD_ambient * ambientDiffuse + ambientSpecular) * ao * aoFactor;

    // 最终合成
    vec3 result = ambient + Lo;

    // Reinhard Tone Mapping
    result = result / (result + vec3(1.0));
    // Gamma Correction
    result = pow(result, vec3(1.0 / 2.2));

    FragColor = vec4(result, 1.0);
}


// 菲涅尔（标准）
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// 菲涅尔（带粗糙度，用于环境光/IBL），粗糙表面在掠射角时菲涅尔效果应该被抑制，否则粗糙金属边缘会过亮
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
} 

// 法线分布函数 (GGX/Trowbridge-Reitz)
float DistributionGGX(vec3 N, vec3 H, float roughness) {    
    float a  = roughness * roughness;
    float a2 = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom  = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return a2 / denom;
}

// 单方向几何遮蔽 (Schlick-GGX)
float GeometrySchlickGGX(float NdotV, float roughness) {    
    float k    = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    float denom = NdotV * (1.0 - k) + k;
    return NdotV / denom;
}

// Smith's 几何函数：同时考虑视线遮蔽和光线遮蔽
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1  = GeometrySchlickGGX(NdotV, roughness);
    float ggx2  = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

// ==================== 阴影计算 ====================

vec3 sampleOffsetDirections[20] = vec3[](
   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);

// 把[0,1]非线性深度转为线性深度
float DepthToDistance(float depth, float nearPlane, float farPlane){
    return nearPlane*farPlane/(farPlane-depth*(farPlane-nearPlane));
}

// 利用相似三角形计算搜索blocker的uv半径
vec2 ComputeBlockerSearchRadiusUV(AreaLight areaLight, float receiverZ){
    float nearPlane = areaLight.shadowParams.x;
    vec2 nearHalfSize = areaLight.shadowParams.zw;
    vec2 lightHalfSize = vec2(areaLight.rightVector_halfWidth.w, areaLight.upVector_halfHeight.w);
    float depthFactor = max(receiverZ - nearPlane, 0.0) / max(receiverZ, 1e-4);
    vec2 nearPlaneSearchOffset = lightHalfSize * depthFactor;
    vec2 searchRadiusUV = nearPlaneSearchOffset / max(2.0 * nearHalfSize, vec2(1e-4));
    return searchRadiusUV;
}

// 计算深度梯度，表示单位UV变化对应多少深度变化
vec2 ComputeReceiverDepthGradient(AreaLight areaLight, vec3 receiverPos, vec3 receiverNormal){
    float nearPlane = areaLight.shadowParams.x;
    float farPlane = areaLight.shadowParams.y;
    vec3 N = receiverNormal;
    float planeDistance = dot(N, receiverPos-areaLight.center_pos.xyz);
    if(abs(planeDistance) < 1e-5) return vec2(0.0);
    vec2 normalXY=vec2(dot(N,areaLight.rightVector_halfWidth.xyz), dot(N,areaLight.upVector_halfHeight.xyz));
    vec2 nearHalfSize=areaLight.shadowParams.zw;
    return -2.0 * farPlane * nearHalfSize * normalXY / ((farPlane - nearPlane) * planeDistance);
}

// 利用相似三角形计算用于PCF的半径
vec2 ComputePenumbraRadiusUV(AreaLight areaLight, float receiverZ, float avgBlockerZ){
    float nearPlane = areaLight.shadowParams.x;
    vec2 nearHalfSize = areaLight.shadowParams.zw;
    vec2 lightHalfSize = vec2(areaLight.rightVector_halfWidth.w, areaLight.upVector_halfHeight.w);
    vec2 penumbra = lightHalfSize * max(receiverZ - avgBlockerZ, 0.0) / max(avgBlockerZ, 1e-4);
    vec2 receiverPlaneSize = 2.0 * nearHalfSize * receiverZ / max(nearPlane, 1e-4);
    return penumbra / receiverPlaneSize;
}

// 两个 uint 表示一个 64 位整数：x 为低位，y 为高位。
uvec2 SubtractMomentSum(uvec2 a, uvec2 b){
    uint low = a.x - b.x;
    uint borrow = uint(a.x < b.x);
    return uvec2(low, a.y - b.y - borrow);
}

// 左边界、下边界之外的前缀和为零。
uvec4 ReadVSSMPrefix(usampler2D table, ivec2 pixel){
    if (any(lessThan(pixel, ivec2(0)))) return uvec4(0u);
    return texelFetch(table, pixel, 0);
}

// 返回半开区间：[lower.x, upper.x) × [lower.y, upper.y)
ivec4 VSSMRectangle(vec2 uv, vec2 radius, ivec2 size){
    ivec2 lower = ivec2(floor(clamp(
        (uv - radius) * vec2(size),
        vec2(0.0), vec2(size-1))));
    ivec2 upper = ivec2(ceil(clamp(
        (uv + radius) * vec2(size),
        vec2(0.0), vec2(size))));
    return ivec4(lower, max(upper,lower+1));
}

// 返回：有效纹素的 E[q]、E[q²]、有效纹素数量。
vec3 VSSMRegionMoments(usampler2D momentSAT, usampler2D coverageSAT, ivec4 rect){
    ivec2 a = rect.xy - 1;                      // 左下
    ivec2 b = ivec2(rect.z - 1, rect.y - 1);    // 右下
    ivec2 c = ivec2(rect.x - 1, rect.w - 1);    // 左上
    ivec2 d = rect.zw - 1;                      // 右上
    uvec4 sa = ReadVSSMPrefix(momentSAT, a);    // 左下深度前缀
    uvec4 sb = ReadVSSMPrefix(momentSAT, b);    // 右下深度前缀
    uvec4 sc = ReadVSSMPrefix(momentSAT, c);    // 左上深度前缀
    uvec4 sd = ReadVSSMPrefix(momentSAT, d);    // 右上深度前缀
    // 矩形区域深度sum
    uvec2 firstMomentSum = SubtractMomentSum(SubtractMomentSum(sd.xy,sb.xy), SubtractMomentSum(sc.xy,sa.xy));   // 一阶深度sum
    uvec2 secondMomentSum = SubtractMomentSum(SubtractMomentSum(sd.zw,sb.zw), SubtractMomentSum(sc.zw,sa.zw));  // 二阶深度sum
    // 区域有效深度count
    uint count =
        (ReadVSSMPrefix(coverageSAT, d).r - ReadVSSMPrefix(coverageSAT, b).r) - 
        (ReadVSSMPrefix(coverageSAT, c).r - ReadVSSMPrefix(coverageSAT, a).r);
    if(count == 0u) return vec3(0.0);
    // 完成区域差分之后，再把高低位合并为浮点数。
    vec2 sums = vec2(
        float(firstMomentSum.x) + float(firstMomentSum.y) * 4294967296.0,
        float(secondMomentSum.x) + float(secondMomentSum.y) * 4294967296.0
    );
    return vec3(sums/float(count)/VSSM_MOMENT_SCALE, float(count));
}

// 在分区中心估计接收面深度，转换为与矩统计一致的 q
float VSSMReceiverDepth(AreaLight areaLight, vec3 coords, vec2 depthGradient, float bias, ivec4 rect, ivec2 size){
    vec2 centerUV = (vec2(rect.xy) + vec2(rect.zw)) * 0.5 / vec2(size);
    float depth = clamp(coords.z + dot(depthGradient, centerUV-coords.xy), 0.0, 1.0);
    float nearPlane = areaLight.shadowParams.x;
    float farPlane = areaLight.shadowParams.y;
    float z = max(DepthToDistance(depth, nearPlane, farPlane) - bias, nearPlane);
    return (z - nearPlane) / (farPlane - nearPlane);
}

// 切比雪夫估计
float VSSMVisibility(vec3 moments, float receiverQ, int area){
    if(moments.z==0.0 || receiverQ<=moments.x) return 1.0;
    if(area==1) return 0.0;
    float variance = max(moments.y-moments.x*moments.x, VSSM_MIN_VARIANCE);
    float delta = receiverQ - moments.x;
    return variance/(variance+delta*delta);
}

// 选择级联阴影层级
int SelectCascade(DirLight dirLight, float viewDepth) {
    if (isnan(viewDepth) || isinf(viewDepth) || viewDepth < dirLight.params.x || viewDepth > dirLight.params.y)
        return -1;
    for (int layer = 0; layer < DIR_CASCADE_COUNT; layer++)
        if (viewDepth <= dirLight.splitDepths[layer]) return layer;
    return -1;
}

// 级联边界混合系数
float CascadeBlend(DirLight dirLight, int layer, float viewDepth) {
    if (layer >= DIR_CASCADE_COUNT - 1) return 0.0;
    return smoothstep(dirLight.blendStarts[layer], dirLight.splitDepths[layer], viewDepth);
}

// 采样级联阴影
float SampleCascade(DirLight dirLight, sampler2DArray shadowMapCascaded, int layer, vec3 fragPos, vec3 normal) {
    vec3 N = normalize(normal);
    vec3 lightDir = normalize(-vec3(dirLight.direction));
    float NdotL = clamp(dot(N, lightDir), 0.0, 1.0);
    float worldTexel = dirLight.texelSizes[layer];
    // Slope bias
    float slope = sqrt(max(1.0 - NdotL * NdotL, 0.0)) / max(NdotL, 0.2);
    float biasTexels = clamp(1.0 + 1.5 * slope, 1.0, 8.0);
    float biasWorld = worldTexel * biasTexels;
    float bias = biasWorld / dirLight.depthRanges[layer];
    // Normal offset
    vec3 normalOffset =N * worldTexel * 1.25 *(1.0 - NdotL);
    vec4 clipPos = dirLight.lightSpaceMatrices[layer] * vec4(fragPos + normalOffset, 1.0);
    vec3 coords = clipPos.xyz / clipPos.w * 0.5 + 0.5;
    if (any(isnan(coords)) || any(isinf(coords)) || any(lessThan(coords, vec3(0.0))) || any(greaterThan(coords, vec3(1.0)))) return 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMapCascaded, 0).xy);
    float shadow = 0.0;
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            float depth = texture(shadowMapCascaded, vec3(coords.xy + vec2(x, y) * texelSize, float(layer))).r;
            shadow += coords.z > depth + bias ? 1.0 : 0.0;
        }
    }
    return shadow / 9.0;
}

// 方向光阴影
float ShadowCalculation_DIR(DirLight dirLight, sampler2DArray shadowMapCascaded, vec3 fragPos, vec3 normal) {
    if (dirLight.params.z < 0.5) return 0.0;
    float viewDepth = -(view * vec4(fragPos, 1.0)).z;
    int layer = SelectCascade(dirLight, viewDepth);
    if (layer < 0) return 0.0;
    float shadow = SampleCascade(dirLight, shadowMapCascaded, layer, fragPos, normal);
    float blend = CascadeBlend(dirLight, layer, viewDepth);
    if (blend > 0.0 && layer < DIR_CASCADE_COUNT - 1)
        shadow = mix(shadow, SampleCascade(dirLight, shadowMapCascaded, layer + 1, fragPos, normal), blend);
    return shadow;
}

// 点光源阴影（Omnidirectional Shadow Map，PCF软阴影）
float ShadowCalculation_point(PointLight pointLight, samplerCube shadowCubeMap, vec3 fragPos, vec3 normal) {
    vec3 N = normalize(normal);
    vec3 lightPos = vec3(pointLight.position);
    vec3 fragToLight = fragPos - lightPos;
    
    // 1. 光照方向向量
    vec3 lightDir = normalize(lightPos - fragPos);

    // 2. Slope-scaled bias————光线越与表面垂直，偏移越低
    float NdotL = clamp(dot(N, lightDir), 0.0, 1.0);
    float bias = max(0.1 * (1.0 - NdotL), 0.01); 

    // 3. Normal Offset————根据法线偏移片段世界坐标
    float normalOffsetScale = 0.02; 
    vec3 offsetPos = fragPos + N * normalOffsetScale;  // 偏移后坐标
    vec3 samplingVector = offsetPos - lightPos;        // 光源指向片段的采样向量
    float currentDepth = length(samplingVector);
    if (currentDepth >= far_plane)
        return 0.0;

    // 4. PCF
    float shadow = 0.0;
    int samples = 20;
    float viewDistance = length(viewPos - fragPos);
    float diskRadius = (1.0 + (viewDistance / far_plane)) / 50.0;   // 调整半径缩放
    for (int i = 0; i < samples; ++i) {
        float closestDepth = texture(shadowCubeMap, samplingVector + sampleOffsetDirections[i] * diskRadius, 0.0).r; // 0~1
        closestDepth *= far_plane; // 还原到世界空间距离
        if (currentDepth > closestDepth + bias)
            shadow += 1.0;
    }
    return shadow / float(samples);
}

// 面光源阴影
float ShadowCalculation_area(AreaLight areaLight, usampler2D momentSAT, usampler2D coverageSAT, vec3 fragPos, vec3 normal){
    vec3 F = normalize(areaLight.direction.xyz);
    ivec2 size = textureSize(momentSAT, 0);

    // 光源背面为阴影
    vec3 lightToFrag = fragPos - areaLight.center_pos.xyz;
    if (dot(lightToFrag, F) <= 0.0) return 1.0;

    // 法线偏移
    vec3 N = normalize(normal);
    vec3 offsetPos = fragPos + N * 0.02;
    // 坡度偏移
    vec3 fragToLight = normalize(areaLight.center_pos.xyz - offsetPos);
    float NdotL = clamp(dot(fragToLight, N), 0.0, 1.0);
    float bias = max(0.1 * (1.0 - NdotL), 0.001);
    // 光源视角→透视投影w→转为uv[0,1]
    vec4 clipPos = areaLight.lightSpaceMatrix * vec4(offsetPos, 1.0);
    if (clipPos.w <= 0.000001) return 0.0;
    vec3 coords = clipPos.xyz / clipPos.w * 0.5 + 0.5;
    if (any(lessThan(coords, vec3(0.0))) || any(greaterThanEqual(coords, vec3(1.0)))) return 0.0;
    // 着色点在光源视角线性距离
    float nearPlane = areaLight.shadowParams.x;
    float farPlane = areaLight.shadowParams.y;
    float receiverZ = dot(offsetPos - areaLight.center_pos.xyz, F);
    // 接收面的投影深度梯度——单位UV变化对应多少深度偏移
    vec2 depthGradient = ComputeReceiverDepthGradient(areaLight, offsetPos, N);
    // 搜索blocker的UV半径
    vec2 searchRadiusUV = ComputeBlockerSearchRadiusUV(areaLight, receiverZ);
    // 以着色点为中心的blocker搜索矩形像素范围
    ivec4 searchRect = VSSMRectangle(coords.xy, searchRadiusUV, size);
    // blocker搜索矩形的像素宽高
    ivec2 extent = searchRect.zw - searchRect.xy;
    // 把blocker搜索矩形分为4最多份，分别求blocker距离，最后平均
    float blockerSum = 0.0;
    float blockerWeight = 0.0;
    ivec2 divisions = min(extent, ivec2(4));
    for (int y = 0; y < divisions.y; y++)
    {
        for (int x = 0; x < divisions.x; x++)
        {
            ivec2 cell = ivec2(x, y);
            ivec4 rect = ivec4(searchRect.xy + extent * cell / divisions, searchRect.xy + extent * (cell + 1) / divisions);
            ivec2 cellSize = rect.zw - rect.xy;
            // 分块区域内的E[q], E[q²], 有效数量
            vec3 moments = VSSMRegionMoments(momentSAT, coverageSAT, rect);
            // 接收面在分块中心处的投影深度（用于处理平面倾斜的情况）
            float receiverQ = VSSMReceiverDepth(areaLight, coords, depthGradient, bias, rect, size);
            // 切比雪夫估计区域内的可见比例
            float p = VSSMVisibility(moments, receiverQ, cellSize.x * cellSize.y);
            float blocked = 1.0 - p;
            if (moments.z == 0.0 || blocked <= 1e-5) continue;
            // 用接收面深度近似非遮挡部分的平均深度  moments.x = p*receiverQ + (1-p)*blockerQ
            float blockerQ = clamp((moments.x - p * receiverQ) / blocked, 0.0, receiverQ);
            // 按估计的遮挡纹素数量加权
            float weight = moments.z * blocked;
            blockerSum += blockerQ * weight;
            blockerWeight += weight;
        }
    }
    if (blockerWeight == 0.0) return 0.0;
    // blocker平均深度，由 q 还原到线性深度
    float blockerZ = nearPlane + blockerSum / blockerWeight * (farPlane - nearPlane);
    // 根据blocker平均深度算出PCF半径
    vec2 filterRadiusUV = ComputePenumbraRadiusUV(areaLight, receiverZ, blockerZ);
    // PCF搜索矩形
    ivec4 filterRect = VSSMRectangle(coords.xy, filterRadiusUV, size);
    // 矩形像素范围
    extent = filterRect.zw - filterRect.xy;
    // 对PCF矩形分块遍历，求得平均可见性
    float visibleArea = 0.0;
    divisions = min(extent, ivec2(4));
    for (int y = 0; y < divisions.y; y++)
    {
        for (int x = 0; x < divisions.x; x++)
        {
            ivec2 cell = ivec2(x, y);
            ivec4 rect = ivec4(filterRect.xy + extent * cell / divisions, filterRect.xy + extent * (cell + 1) / divisions);
            ivec2 cellSize = rect.zw - rect.xy;
            int area = cellSize.x * cellSize.y;
            // 分块区域内的E[q], E[q²], 有效数量
            vec3 moments = VSSMRegionMoments(momentSAT, coverageSAT, rect);
            // 接收面在分块中心处的投影深度（用于处理平面倾斜的情况）
            float receiverQ = VSSMReceiverDepth(areaLight, coords, depthGradient, bias, rect, size);
            // 切比雪夫估计区域内的可见比例
            float p = VSSMVisibility(moments, receiverQ, area);
            // 仅在最终滤波阶段抑制漏光
            p = clamp((p - VSSM_BLEED_REDUCTION) / (1.0 - VSSM_BLEED_REDUCTION), 0.0, 1.0);
            // 空白纹素全部可见；有效纹素按 p 估计可见数量
            visibleArea += float(area) - moments.z + moments.z * p;
        }
    }
    // 返回阴影系数：0 为无阴影，1 为完全遮挡
    return clamp(
        1.0 - visibleArea / float(extent.x * extent.y),
        0.0, 1.0
    );
}

//  方向光 PBR 直接光照
vec3 CalcDirLight(DirLight dirLight, vec3 fragPos, vec3 viewDir, vec3 normal, vec3 albedo, vec3 mra, float shadow){
    vec3 lightColor = clamp(dirLight.color.rgb, vec3(0.0), vec3(1.0));
    float lightIntensity = max(dirLight.color.a, 0.0);
    float metallic  = mra.x;
    float roughness = mra.y;
    // mra.z (ao) 在 ambient 中处理，直接光照不用

    // ---------- 光照方向 ----------
    // direction 存储的是光源照射方向（从光源指向场景），取反得到片元到光源的方向
    vec3 lightDir   = normalize(-vec3(dirLight.direction));
    vec3 halfVector = normalize(viewDir + lightDir);

    // ---------- 入射辐射度（方向光无衰减） ----------
    vec3 radiance = lightColor * lightIntensity;

    // ---------- 菲涅尔项 ----------
    vec3 F0 = vec3(0.04);
    F0      = mix(F0, albedo, metallic);
    vec3 F  = fresnelSchlick(max(dot(halfVector, viewDir), 0.0), F0);

    // ---------- 法线分布函数 ----------
    float NDF = DistributionGGX(normal, halfVector, roughness);

    // ---------- 几何遮蔽函数 ----------
    float G = GeometrySmith(normal, viewDir, lightDir, roughness);

    // ---------- Cook-Torrance BRDF 高光项 ----------
    vec3  numerator   = NDF * G * F;
    float denominator = 4.0 * max(dot(normal, viewDir), 0.0)
                            * max(dot(normal, lightDir), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    // ---------- 能量守恒 ----------
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;  // 金属无漫反射

    // ---------- 出射辐射度 ----------
    float NdotL = max(dot(normal, lightDir), 0.0);
    vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;

    // ---------- 阴影衰减 ----------
    Lo *= (1.0 - shadow);

    return Lo;
}

// 点光源 PBR 直接光照
vec3 CalcPointLight(PointLight pointLight, vec3 fragPos, vec3 viewDir, vec3 normal, vec3 albedo, vec3 mra, float shadow) { 
    float range = pointLight.attenuation.w;
    if (range <= 0.0) return vec3(0.0);
    
    float metallic  = mra.x; 
    float roughness = mra.y; 
    vec3 lightPos   = vec3(pointLight.position); 
    vec3 lightColor = clamp(pointLight.color.rgb, vec3(0.0), vec3(1.0));
    float lightIntensity = max(pointLight.color.a, 0.0);
    vec3 lightDir   = normalize(lightPos - fragPos); 
    vec3 halfVector = normalize(viewDir + lightDir); 
    float distance    = length(lightPos - fragPos); 
    float rangeWeight = 1.0 - smoothstep(range * 0.8, range, distance);
    float attenuationDenominator = pointLight.attenuation.x
                                 + pointLight.attenuation.y * distance
                                 + pointLight.attenuation.z * distance * distance;
    float attenuation = 1.0 / max(attenuationDenominator, 0.0001);

    // 入射辐射度
    vec3 radiance = lightColor * lightIntensity * attenuation * rangeWeight;

    // ---------- 菲涅尔项 ----------
    vec3 F0 = vec3(0.04); 
    F0      = mix(F0, albedo, metallic); 
    vec3 F  = fresnelSchlick(max(dot(halfVector, viewDir), 0.0), F0); 

    // ---------- 法线分布函数 ----------
    float NDF = DistributionGGX(normal, halfVector, roughness);       

    // ---------- 几何遮蔽函数 ----------
    float G = GeometrySmith(normal, viewDir, lightDir, roughness);  

    // ---------- Cook-Torrance BRDF 高光项 ----------
    vec3  numerator   = NDF * G * F; 
    float denominator = 4.0 * max(dot(normal, viewDir), 0.0) 
                            * max(dot(normal, lightDir), 0.0) + 0.0001; 
    vec3 specular = numerator / denominator;  

    // ---------- 能量守恒 ----------
    vec3 kS = F; 
    vec3 kD = vec3(1.0) - kS; 
    kD *= 1.0 - metallic;

    // ---------- 出射辐射度 ----------
    float NdotL = max(dot(normal, lightDir), 0.0);        
    vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL; 

    // ---------- 阴影衰减 ----------
    Lo *= (1.0 - shadow);

    return Lo; 
}

// 面光源 PBR 直接光照
vec3 CalcAreaLight(AreaLight areaLight, vec3 fragPos, vec3 viewDir, vec3 normal, vec3 albedo, vec3 mra, float shadow){
    vec3 lightColor = clamp(areaLight.color.rgb, vec3(0.0), vec3(1.0));
    float lightIntensity = max(areaLight.color.a, 0.0);
    float metallic  = mra.x;
    float roughness = mra.y;
    // mra.z (ao) 在 ambient 中处理，直接光照不用

    // ---------- 光照方向 ----------
    // direction 存储的是光源照射方向（从光源指向场景），取反得到片元到光源的方向
    vec3 lightDir   = normalize(-vec3(areaLight.direction));
    vec3 halfVector = normalize(viewDir + lightDir);

    // ---------- 入射辐射度（方向光无衰减） ----------
    vec3 radiance = lightColor * lightIntensity;

    // ---------- 菲涅尔项 ----------
    vec3 F0 = vec3(0.04);
    F0      = mix(F0, albedo, metallic);
    vec3 F  = fresnelSchlick(max(dot(halfVector, viewDir), 0.0), F0);

    // ---------- 法线分布函数 ----------
    float NDF = DistributionGGX(normal, halfVector, roughness);

    // ---------- 几何遮蔽函数 ----------
    float G = GeometrySmith(normal, viewDir, lightDir, roughness);

    // ---------- Cook-Torrance BRDF 高光项 ----------
    vec3  numerator   = NDF * G * F;
    float denominator = 4.0 * max(dot(normal, viewDir), 0.0)
                            * max(dot(normal, lightDir), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    // ---------- 能量守恒 ----------
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;  // 金属无漫反射

    // ---------- 出射辐射度 ----------
    float NdotL = max(dot(normal, lightDir), 0.0);
    vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;

    // ---------- 阴影衰减 ----------
    Lo *= (1.0 - shadow);

    return Lo;
}
