#version 450 core
#define NR_DIR_LIGHTS   4
#define NR_POINT_LIGHTS 4
#define NR_AREA_LIGHTS  4

struct DirLight {
    vec4 color;
    vec4 direction;
    mat4 lightSpaceMatrix;
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
	vec4 half_width;
	vec4 half_height;
	mat4 lightSpaceMatrix;
    vec4 shadowParams;
};

out vec4 FragColor;
in vec2 TexCoord;

uniform vec3 viewPos;
uniform float far_plane;
uniform bool diable_SSAO;
uniform bool debug_SSAO;

layout(binding = 0) uniform sampler2D gPosition;
layout(binding = 1) uniform sampler2D gNormal;
layout(binding = 2) uniform sampler2D gAlbedo;
layout(binding = 3) uniform sampler2D gMRA;
layout(binding = 4) uniform sampler2D ssao; 
layout(binding = 5) uniform samplerCube irradianceMap; 
layout(binding = 6) uniform samplerCube prefilterMap; 
layout(binding = 7) uniform sampler2D brdfLUT; 
layout(binding = 11) uniform sampler2D shadowMaps[NR_DIR_LIGHTS];
layout(binding = 15) uniform samplerCube shadowCubeMap[NR_POINT_LIGHTS];
layout(binding = 19) uniform sampler2D areaShadowMap[NR_AREA_LIGHTS];

layout (std140) uniform DirLights {
    DirLight dir_lights[NR_DIR_LIGHTS];
};
layout (std140) uniform PointLights {
    PointLight point_lights[NR_POINT_LIGHTS];
};
layout (std140) uniform AreaLights {
    AreaLight area_lights[NR_AREA_LIGHTS];
};

const float PI = 3.14159265359;
const int PCSS_POISSON_SAMPLE_COUNT = 32;
const vec2 PCSS_POISSON_SAMPLES[PCSS_POISSON_SAMPLE_COUNT] = vec2[](
    vec2(-0.149526,  0.359701),
    vec2( 0.986664, -0.996068),
    vec2(-0.922278, -0.990225),
    vec2( 0.997021,  0.982214),
    vec2( 0.027819, -0.649155),
    vec2(-0.998254,  0.938730),
    vec2( 0.802187, -0.014030),
    vec2(-0.995388, -0.041308),
    vec2( 0.228153,  0.998907),
    vec2(-0.568586, -0.490378),
    vec2(-0.383064,  0.925000),
    vec2( 0.444705,  0.464125),
    vec2( 0.604381, -0.540442),
    vec2( 0.213788, -0.111529),
    vec2(-0.720852,  0.461645),
    vec2(-0.383883, -0.971204),
    vec2( 0.925918,  0.502835),
    vec2( 0.462445, -0.998728),
    vec2(-0.493441,  0.007651),
    vec2(-0.999795, -0.484943),
    vec2( 0.993740, -0.429842),
    vec2(-0.168694, -0.286180),
    vec2( 0.606570,  0.878681),
    vec2( 0.101144,  0.633689),
    vec2( 0.010122, -0.994586),
    vec2( 0.484304,  0.112859),
    vec2(-0.092252,  0.037104),
    vec2(-0.407757,  0.588829),
    vec2( 0.287081, -0.427186),
    vec2( 0.540960, -0.212704),
    vec2( 0.173981,  0.237580),
    vec2(-0.707220,  0.792787)
);
// ==================== 函数前向声明 ====================
vec3 fresnelSchlick(float cosTheta, vec3 F0);
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness);
float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
// PCSS
float DepthToDistance(float depth, float nearPlane, float farPlane);
bool FindAverageBlockerDepth(sampler2D shadowMap,vec2 uv, float receiverDepth, vec2 depthGradient, float nearPlane, float farPlane, float depthBias, vec2 searchRadiusUV, out float avgBlockerZ);
vec2 ComputeReceiverDepthGradient(AreaLight areaLight, vec3 receiverPos, vec3 receiverNormal);
vec2 ComputeBlockerSearchRadiusUV(AreaLight areaLight, float receiverZ);
vec2 ComputePenumbraRadiusUV(AreaLight areaLight, float receiverZ, float avgBlockerZ);
float FilterAreaShadowPCF(sampler2D shadowMap, vec2 uv, float receiverDepth, vec2 depthGradient, float depthBias, vec2 filterRadiusUV);
// 直接光照计算（不含 ambient，只返回 Lo）
vec3 CalcDirLight(DirLight dirLight, vec3 fragPos, vec3 viewDir, vec3 normal, vec3 albedo, vec3 mra, float shadow);
vec3 CalcPointLight(PointLight pointLight, vec3 fragPos, vec3 viewDir, vec3 normal, vec3 albedo, vec3 mra, float shadow);
vec3 CalcAreaLight(AreaLight areaLight, vec3 fragPos, vec3 viewDir, vec3 normal, vec3 albedo, vec3 mra, float shadow);
// 阴影系数计算
float ShadowCalculation_dir(DirLight dirLight, sampler2D shadowMap, vec3 fragPos, vec3 normal);
float ShadowCalculation_point(PointLight pointLight, samplerCube shadowCubeMap, vec3 fragPos, vec3 normal);
float ShadowCalculation_area(AreaLight areaLight, sampler2D shadowMap, vec3 fragPos, vec3 normal);

// ==================== 主函数 ====================
void main()
{   
    vec3 FragPos = texture(gPosition, TexCoord).rgb;
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
    receiverNormal=Normal;
    if (dot(receiverNormal, Normal) < 0.0) receiverNormal = -receiverNormal;

    // SSAO
    float ssaoFactor = 1.0;
    if (!diable_SSAO){
        ssaoFactor = texture(ssao, TexCoord).r;
        ssaoFactor = pow(ssaoFactor, 2.0);  // 让暗部更暗
    }
    if(debug_SSAO){
        ssaoFactor = texture(ssao, TexCoord).r;
        ssaoFactor = pow(ssaoFactor, 2.0);  // 让暗部更暗
        FragColor = vec4(vec3(ssaoFactor), 1.0);
        return;
    }

    vec3 ViewDir = normalize(viewPos - FragPos);
    vec3 ReflectDir = reflect(-ViewDir, Normal); 

    // 累加所有直接光照（不含 ambient）
    vec3 Lo = vec3(0.0);

    // ---------- 方向光 ----------
    for (int i = 0; i < NR_DIR_LIGHTS; i++) {
        if (dir_lights[i].direction.w < 0.5) continue;
        float shadow = ShadowCalculation_dir(dir_lights[i], shadowMaps[i], FragPos, Normal);       
        Lo += CalcDirLight(dir_lights[i], FragPos, ViewDir, Normal, Albedo, MRA, shadow);
    }

    // ---------- 点光源 ----------
    for (int i = 0; i < NR_POINT_LIGHTS; i++) {
        if (point_lights[i].attenuation.x > 0.0 || point_lights[i].attenuation.y > 0.0 || point_lights[i].attenuation.z > 0.0){
            float shadow = ShadowCalculation_point(point_lights[i], shadowCubeMap[i], FragPos, Normal);
            Lo += CalcPointLight(point_lights[i], FragPos, ViewDir, Normal, Albedo, MRA, shadow);
        }
    }

    // ---------- 面光源 ----------
    for (int i = 0; i < NR_AREA_LIGHTS; i++) {
        if (area_lights[i].direction.w < 0.5) continue;
        float shadow = ShadowCalculation_area(area_lights[i], areaShadowMap[i], FragPos, receiverNormal);
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

    // 最终环境光 = (漫反射 + 高光) * AO * SSAO
    vec3 ambient = (kD_ambient * ambientDiffuse + ambientSpecular) * ao * ssaoFactor;

    // 最终合成
    vec3 result = ambient + Lo;

    // Reinhard Tone Mapping
    result = result / (result + vec3(1.0));
    // Gamma Correction
    result = pow(result, vec3(1.0 / 2.2));

    FragColor = vec4(result, 1.0);
}

// ==================== BRDF 工具函数 ====================

// 菲涅尔（标准）
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// 菲涅尔（带粗糙度，用于环境光/IBL）
// 原理：粗糙表面在掠射角时菲涅尔效果应该被抑制，否则粗糙金属边缘会过亮
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

float DepthToDistance(float depth, float nearPlane, float farPlane){
    return nearPlane*farPlane/(farPlane-depth*(farPlane-nearPlane));
}

vec2 ComputeBlockerSearchRadiusUV(AreaLight areaLight, float receiverZ)
{
    float nearPlane = areaLight.shadowParams.x;
    vec2 nearHalfSize = areaLight.shadowParams.zw;
    vec2 lightHalfSize = vec2(areaLight.half_width.w, areaLight.half_height.w);
    float depthFactor = max(receiverZ - nearPlane, 0.0) / max(receiverZ, 1e-4);
    vec2 nearPlaneSearchOffset = lightHalfSize * depthFactor;
    vec2 searchRadiusUV = nearPlaneSearchOffset / max(2.0 * nearHalfSize, vec2(1e-4));
    return searchRadiusUV;
}

vec2 ComputeReceiverDepthGradient(AreaLight areaLight, vec3 receiverPos, vec3 receiverNormal)
{
    float nearPlane = areaLight.shadowParams.x;
    float farPlane = areaLight.shadowParams.y;
    vec3 N = receiverNormal;
    float planeDistance = dot(N, receiverPos-areaLight.center_pos.xyz);
    if(abs(planeDistance) < 1e-5) return vec2(0.0);
    vec2 normalXY=vec2(dot(N,areaLight.half_width.xyz), dot(N,areaLight.half_height.xyz));
    vec2 nearHalfSize=areaLight.shadowParams.zw;
    return -2.0 * farPlane * nearHalfSize * normalXY / ((farPlane - nearPlane) * planeDistance);
}

bool FindAverageBlockerDepth(sampler2D shadowMap, vec2 uv, float receiverDepth, vec2 depthGradient, float nearPlane, float farPlane, float depthBias, vec2 searchRadiusUV, out float avgBlockerZ)
{
    float blockerZSum = 0.0;
    int blockerCount = 0;

    ivec2 texSize = textureSize(shadowMap, 0);

    for (int i = 0; i < PCSS_POISSON_SAMPLE_COUNT; ++i) {
        // 采样点UV
        vec2 sampleUV = uv + PCSS_POISSON_SAMPLES[i] * searchRadiusUV;
        // 采样点UV越界检查
        if (any(lessThan(sampleUV, vec2(0.0))) ||any(greaterThanEqual(sampleUV, vec2(1.0)))) {continue;}
        // 采样点深度
        float sampleDepth = textureLod(shadowMap, sampleUV, 0.0).r;
        // 采样点中心UV
        vec2 sampleTexelCenterUV = (vec2(ivec2(sampleUV * vec2(texSize))) + 0.5) / vec2(texSize);
        // 根据uv偏移着色点深度
        float receiverDepthAtSample = receiverDepth + dot(depthGradient, sampleTexelCenterUV - uv);
        // 比较并累计blocker距离
        if (sampleDepth < 1.0 && receiverDepthAtSample > sampleDepth + depthBias) {
            // 确认是遮挡物后，再转换为线性深度用于平均
            float sampleZ = DepthToDistance(sampleDepth, nearPlane, farPlane);
            blockerZSum += sampleZ;
            blockerCount++;
        }
    }
    if (blockerCount == 0) {
        avgBlockerZ = 0.0;
        return false;
    }
    avgBlockerZ = blockerZSum / float(blockerCount);
    return true;
}

vec2 ComputePenumbraRadiusUV(AreaLight areaLight, float receiverZ, float avgBlockerZ)
{
    float nearPlane = areaLight.shadowParams.x;
    vec2 nearHalfSize = areaLight.shadowParams.zw;
    vec2 lightHalfSize = vec2(areaLight.half_width.w, areaLight.half_height.w);
    vec2 penumbra = lightHalfSize * max(receiverZ - avgBlockerZ, 0.0) / max(avgBlockerZ, 1e-4);
    vec2 receiverPlaneSize = 2.0 * nearHalfSize * receiverZ / max(nearPlane, 1e-4);
    return penumbra / receiverPlaneSize;
}

float FilterAreaShadowPCF( sampler2D shadowMap, vec2 uv, float receiverDepth, vec2 depthGradient, float depthBias, vec2 filterRadiusUV)
{
    float shadow = 0.0;
    int validFilterCount = 0;
    ivec2 texSize = textureSize(shadowMap, 0);
    for (int i = 0; i < PCSS_POISSON_SAMPLE_COUNT; ++i) {
        // 采样点UV
        vec2 sampleUV = uv + PCSS_POISSON_SAMPLES[i] * filterRadiusUV;
        // 采样点UV越界检查
        if (any(lessThan(sampleUV, vec2(0.0))) || any(greaterThanEqual(sampleUV, vec2(1.0)))) { continue; }
        // 采样点深度
        float sampleDepth = textureLod(shadowMap, sampleUV, 0.0).r;
        // 采样点中心UV
        vec2 sampleTexelCenterUV = (vec2(ivec2(sampleUV * vec2(texSize))) + 0.5) / vec2(texSize);
        // 根据uv偏移着色点深度
        float receiverDepthAtSample = receiverDepth + dot(depthGradient, sampleTexelCenterUV - uv);
        // 比较并累计阴影
        if (sampleDepth < 1.0 &&receiverDepthAtSample > sampleDepth + depthBias) {
            shadow += 1.0;
        }
        validFilterCount++;
    }
    return validFilterCount > 0 ? shadow / float(validFilterCount) : 0.0;
}

// 方向光阴影（标准 Shadow Map，PCF 3x3 软阴影）
float ShadowCalculation_dir(DirLight dirLight, sampler2D shadowMap, vec3 fragPos, vec3 normal) {
    vec3 N = normalize(normal);
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    // 光源方向
    vec3 lightDir = normalize(-vec3(dirLight.direction));
    // 光源矩阵
    mat4 lightSpaceMatrix = dirLight.lightSpaceMatrix;

    // Slope-scaled bias————光线越与表面垂直，偏移越低
    float NdotL = clamp(dot(N, lightDir), 0.0, 1.0);
    float bias = max(0.01 * (1.0 - NdotL), 0.001);

    // normal bias————把片段世界坐标朝着法线移动，再转光源坐标系
    vec3 normalOffset = normalize(normal) * 0.02;
    vec4 fragPosLightSpace = lightSpaceMatrix * vec4(fragPos+normalOffset, 1.0);

    // 拿到真实深度
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    if (any(lessThan(projCoords, vec3(0.0))) || any(greaterThan(projCoords, vec3(1.0)))) return 0.0;
    float currentDepth = projCoords.z;

    // PCF 9x9
    float shadow = 0.0;
    for (int x = -4; x <= 4; ++x) {
        for (int y = -4; y <= 4; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (currentDepth > pcfDepth + bias) ? 1.0 : 0.0;
        }
    }
    return shadow/81.0;
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

    // 4. PCF
    float shadow = 0.0;
    int samples = 20;
    float viewDistance = length(viewPos - fragPos);
    float diskRadius = (1.0 + (viewDistance / far_plane)) / 50.0;   // 调整半径缩放
    for (int i = 0; i < samples; ++i) {
        float closestDepth = texture(shadowCubeMap, samplingVector + sampleOffsetDirections[i] * diskRadius).r; // 0~1
        closestDepth *= far_plane; // 还原到世界空间距离
        if (currentDepth > closestDepth + bias)
            shadow += 1.0;
    }
    return shadow / float(samples);
}

// 面光源阴影
float ShadowCalculation_area(AreaLight areaLight, sampler2D shadowMap, vec3 fragPos, vec3 normal)
{
    vec3 F=normalize(areaLight.direction.xyz);

    // 光源背面为阴影
    vec3 lightToFrag=fragPos - areaLight.center_pos.xyz;
    if(dot(lightToFrag, F)<=0.0) return 1.0;
    // 归一化法线
    vec3 N = normalize(normal);
    // 沿法线偏移着色点
    vec3 offsetPos = fragPos+N*0.02;
    // 世界坐标系→光源裁剪坐标系
    vec4 clipPos = areaLight.lightSpaceMatrix * vec4(offsetPos,1.0);
    if (clipPos.w <= 0.000001) return 0.0;
    // 透视除法到NDC，然后转为0~1
    vec3 coords=clipPos.xyz/clipPos.w*0.5+0.5;
    if (any(lessThan(coords, vec3(0.0))) || any(greaterThanEqual(coords, vec3(1.0)))) return 0.0;
    // 透视投影远近平面
    float nearPlane=areaLight.shadowParams.x;
    float farPlane=areaLight.shadowParams.y;
    // 线性深度：用于搜索半径、半影半径和 bias 换算
    float receiverZ = dot(offsetPos - areaLight.center_pos.xyz, F);
    // 投影深度：用于阴影比较
    float receiverDepth = coords.z;
    // 线性深度偏移
    vec3 fragToLight = normalize(areaLight.center_pos.xyz - offsetPos);
    float NdotL = clamp(dot(fragToLight, N), 0.0, 1.0);
    float bias = max(0.1 * (1.0 - NdotL), 0.001);
    // 接收面的投影深度梯度
    vec2 depthGradient = ComputeReceiverDepthGradient(areaLight, offsetPos, N);
    // 将线性深度偏移转换为投影深度偏移
    float biasedReceiverZ = max(receiverZ - bias, nearPlane);
    float depthBias = nearPlane * farPlane / (farPlane - nearPlane) * (1.0 / biasedReceiverZ - 1.0 / receiverZ);
    depthBias = max(depthBias, 2.0 / 16777216.0);
    // PCSS————计算Blocker平均距离
    float avgBlockerZ = 0.0;
    vec2 searchRadiusUV = ComputeBlockerSearchRadiusUV(areaLight, receiverZ);           // 搜索blocker用的偏移半径
    if (!FindAverageBlockerDepth(shadowMap,coords.xy,receiverDepth,depthGradient,nearPlane,farPlane,depthBias,searchRadiusUV,avgBlockerZ)) {
        return 0.0;
    }
    // PCSS————根据Blocker平均距离计算可变 PCF 半径
    vec2 filterRadiusUV = ComputePenumbraRadiusUV(areaLight, receiverZ, avgBlockerZ);  // 根据半影算出来的PCF半径
    return FilterAreaShadowPCF(shadowMap, coords.xy, receiverDepth, depthGradient, depthBias, filterRadiusUV);
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
    float metallic  = mra.x; 
    float roughness = mra.y; 
    vec3 lightPos   = vec3(pointLight.position); 
    vec3 lightColor = clamp(pointLight.color.rgb, vec3(0.0), vec3(1.0));
    float lightIntensity = max(pointLight.color.a, 0.0);
    vec3 lightDir   = normalize(lightPos - fragPos); 
    vec3 halfVector = normalize(viewDir + lightDir); 
    float distance    = length(lightPos - fragPos); 
    float attenuationDenominator = pointLight.attenuation.x
                                 + pointLight.attenuation.y * distance
                                 + pointLight.attenuation.z * distance * distance;
    float attenuation = 1.0 / max(attenuationDenominator, 0.0001);

    // 入射辐射度
    vec3 radiance = lightColor * lightIntensity * attenuation;

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
