#version 450 core
#define NR_DIR_LIGHTS   4
#define NR_POINT_LIGHTS 4

struct DirLight {
    vec4 color;
    vec4 direction;
    mat4 lightSpaceMatrices;
};

struct PointLight {
    vec4 color;
    vec4 position;
    vec4 attenuation;
};

out vec4 FragColor;
in vec2 TexCoord;

uniform vec3 viewPos;
uniform float far_plane;
uniform bool use_SSAO;
uniform bool debug_SSAO;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D gMRA;
uniform sampler2D ssao;

uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;
uniform samplerCube shadowCubeMap[NR_POINT_LIGHTS];
uniform sampler2D shadowMaps[NR_DIR_LIGHTS];

layout (std140) uniform DirLights {
    DirLight dir_lights[NR_DIR_LIGHTS];
};
layout (std140) uniform PointLights {
    PointLight point_lights[NR_POINT_LIGHTS];
};

const float PI = 3.14159265359;

// ==================== 函数前向声明 ====================
vec3 fresnelSchlick(float cosTheta, vec3 F0);
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness);
float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);

// 直接光照计算（不含 ambient，只返回 Lo）
vec3 CalcPointLight(vec3 fragPos, PointLight pointLight, vec3 viewDir, vec3 normal, vec3 albedo, vec3 mra, float shadow);
vec3 CalcDirLight(vec3 fragPos, DirLight dirLight, vec3 viewDir, vec3 normal, vec3 albedo, vec3 mra, float shadow);

float ShadowCalculation_dir(sampler2D shadowMap, vec4 fragPosLightSpace, vec3 normal, vec3 lightDir);
float ShadowCalculation_point(samplerCube shadowCubeMap, vec3 lightPos, vec3 fragPos, vec3 normal);

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

    // SSAO
    float ssaoFactor = 1.0;
    if (use_SSAO){
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

    // ============================================================
    // 累加所有直接光照（不含 ambient）
    // ============================================================
    vec3 Lo = vec3(0.0);

    // ============================================================
    // ---------- 方向光 ----------
    // ============================================================
    for (int i = 0; i < NR_DIR_LIGHTS; i++) {
        if (dir_lights[i].direction.w < 0.5) continue;
        vec4 fragPosLightSpace = dir_lights[i].lightSpaceMatrices * vec4(FragPos, 1.0);             // 计算该片元在光源空间中的位置
        vec3 lightDir = normalize(-vec3(dir_lights[i].direction));                                  // 方向光的光照方向（注意 direction 存储的是"从光源出发的方向"，需要取反）
        float shadow = ShadowCalculation_dir(shadowMaps[i], fragPosLightSpace, Normal, lightDir);   // 阴影计算          
        Lo += CalcDirLight(FragPos, dir_lights[i], ViewDir, Normal, Albedo, MRA, shadow);           // 累加直接光照
    }

    // ============================================================
    // ---------- 点光源 ----------
    // ============================================================
    for (int i = 0; i < NR_POINT_LIGHTS; i++) {
        if (point_lights[i].attenuation.x > 0.0 || point_lights[i].attenuation.y > 0.0 || point_lights[i].attenuation.z > 0.0){
            vec3 lightPos = vec3(point_lights[i].position);
            float shadow = ShadowCalculation_point(shadowCubeMap[i], lightPos, FragPos, Normal);
            Lo += CalcPointLight(FragPos, point_lights[i], ViewDir, Normal, Albedo, MRA, shadow);
        }
    }

    // ============================================================
    // ---------- 环境光 ----------
    // ============================================================
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

    // ============================================================
    // 最终合成
    // ============================================================
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

// 点光源阴影（Omnidirectional Shadow Map，PCF软阴影）
float ShadowCalculation_point(samplerCube shadowCubeMap, vec3 lightPos, vec3 fragPos, vec3 normal) {
    vec3 fragToLight = fragPos - lightPos;
    float currentDepth = length(fragToLight);
    
    vec3 lightDir = normalize(-fragToLight);
    float bias = max(0.15 * (1.0 - dot(normal, lightDir)), 0.05);
    
    float shadow = 0.0;
    int samples = 20;
    float viewDistance = length(viewPos - fragPos);
    float diskRadius = (1.0 + (viewDistance / far_plane)) / 25.0;  

    for (int i = 0; i < samples; ++i) {
        float closestDepth = texture(shadowCubeMap, fragToLight + sampleOffsetDirections[i] * diskRadius).r;
        closestDepth *= far_plane;
        if (currentDepth - bias > closestDepth)
            shadow += 1.0;
    }
    shadow /= float(samples);  
    
    return shadow;
}

// 方向光阴影（标准 Shadow Map，PCF 3x3 软阴影）
float ShadowCalculation_dir(sampler2D shadowMap, vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    // 透视除法（方向光用正交投影，w通常为1.0，但保险起见仍做除法）
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // 从 [-1,1] NDC 变换到 [0,1] 纹理坐标
    projCoords = projCoords * 0.5 + 0.5;

    // 超出阴影贴图范围，不产生阴影
    if (projCoords.z > 1.0) return 0.0;

    float currentDepth = projCoords.z;
    
    // 自适应偏移：表面越接近与光线平行，偏移越大，防止阴影痤疮(Shadow Acne)
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);

    // PCF 3x3 软阴影
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (currentDepth - bias > pcfDepth) ? 1.0 : 0.0;
        }
    }
    return shadow / 9.0;
}


//  方向光 PBR 直接光照
vec3 CalcDirLight(vec3 fragPos, DirLight dirLight, vec3 viewDir, vec3 normal, vec3 albedo, vec3 mra, float shadow){
    vec3 lightColor = vec3(dirLight.color);
    float metallic  = mra.x;
    float roughness = mra.y;
    // mra.z (ao) 在 ambient 中处理，直接光照不用

    // ---------- 光照方向 ----------
    // direction 存储的是光源照射方向（从光源指向场景），取反得到片元到光源的方向
    vec3 lightDir   = normalize(-vec3(dirLight.direction));
    vec3 halfVector = normalize(viewDir + lightDir);

    // ---------- 入射辐射度（方向光无衰减） ----------
    vec3 radiance = lightColor;

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
vec3 CalcPointLight(vec3 fragPos, PointLight pointLight, vec3 viewDir, vec3 normal, vec3 albedo, vec3 mra, float shadow) { 
    vec3 lightPos   = vec3(pointLight.position); 
    vec3 lightColor = vec3(pointLight.color); 
    float metallic  = mra.x; 
    float roughness = mra.y; 

    // ---------- 光照方向与半角向量 ----------
    vec3 lightDir   = normalize(lightPos - fragPos); 
    vec3 halfVector = normalize(viewDir + lightDir); 

    // ---------- 距离衰减 ----------
    float distance    = length(lightPos - fragPos); 
    float attenuation = 1.0 / (pointLight.attenuation.x 
                              + pointLight.attenuation.y * distance 
                              + pointLight.attenuation.z * distance * distance); 

    // 入射辐射度
    vec3 radiance = lightColor * attenuation; 

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