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
	vec4 rightVector_halfWidth;
	vec4 upVector_halfHeight;
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
// Adjust the two stages independently (1..128 samples per stage).
// The tables stay at 128 entries; each stage uses the requested prefix.
const int PCSS_BLOCKER_SAMPLE_COUNT = 128;
const int PCSS_PCF_SAMPLE_COUNT = 128;

// Progressive best-candidate samples over [-1, 1]^2 for the rectangular light.
// Each new point is the best of 1024 candidates; seeds: 202610061 / 202610062.
// A center tap is included in each table, and prefixes also cover the square.
const vec2 PCSS_BLOCKER_SAMPLES[128] = vec2[](
    vec2( 0.000000,  0.000000),
    vec2( 0.956601,  0.976718),
    vec2(-0.976216,  0.958497),
    vec2(-0.991327, -0.878558),
    vec2( 0.951031, -0.976367),
    vec2( 0.988993, -0.009971),
    vec2(-0.031489, -0.964514),
    vec2(-0.002669,  0.935440),
    vec2(-0.908105,  0.049667),
    vec2( 0.489469,  0.449797),
    vec2(-0.475165, -0.446970),
    vec2( 0.532991, -0.463621),
    vec2(-0.424884,  0.456733),
    vec2(-0.486371,  0.997854),
    vec2( 0.452776, -0.964316),
    vec2(-0.519682, -0.982387),
    vec2( 0.035889, -0.470657),
    vec2(-0.940656, -0.409041),
    vec2( 0.469041,  0.936904),
    vec2( 0.999382, -0.474572),
    vec2( 0.953619,  0.458090),
    vec2( 0.483536, -0.005248),
    vec2(-0.999099,  0.503900),
    vec2( 0.025481,  0.487054),
    vec2(-0.412188, -0.005354),
    vec2(-0.719408,  0.722863),
    vec2(-0.239513, -0.686354),
    vec2(-0.724242, -0.675785),
    vec2( 0.725192,  0.719636),
    vec2( 0.752129, -0.234338),
    vec2( 0.244826,  0.211591),
    vec2( 0.710563, -0.752291),
    vec2( 0.212494, -0.746885),
    vec2( 0.257356, -0.239996),
    vec2( 0.731829,  0.196170),
    vec2(-0.681088, -0.200062),
    vec2(-0.646143,  0.244459),
    vec2( 0.263013,  0.688587),
    vec2(-0.306238,  0.744311),
    vec2(-0.187504, -0.235115),
    vec2(-0.194032,  0.249743),
    vec2( 0.965037, -0.721641),
    vec2(-0.758128,  0.462368),
    vec2(-0.772060, -0.992983),
    vec2( 0.994994,  0.713895),
    vec2(-0.246210,  0.962221),
    vec2(-0.723807,  0.984005),
    vec2(-0.956704,  0.275112),
    vec2(-0.486310, -0.710360),
    vec2( 0.457526, -0.710308),
    vec2(-0.282112, -0.992943),
    vec2( 0.295291, -0.514160),
    vec2( 0.710782,  0.980039),
    vec2( 0.226793,  0.998855),
    vec2(-0.201017,  0.484369),
    vec2(-0.046750,  0.714074),
    vec2(-0.972835, -0.643429),
    vec2(-0.978151, -0.178443),
    vec2( 0.996893,  0.224548),
    vec2( 0.720398,  0.444404),
    vec2( 0.261807,  0.438398),
    vec2( 0.220079, -0.977413),
    vec2( 0.722226, -0.987480),
    vec2(-0.013181, -0.717267),
    vec2( 0.505677,  0.676647),
    vec2( 0.516782, -0.242185),
    vec2( 0.754181, -0.522362),
    vec2(-0.217188, -0.468116),
    vec2(-0.961574,  0.739999),
    vec2(-0.628764,  0.024866),
    vec2( 0.990765, -0.247512),
    vec2(-0.423299,  0.226834),
    vec2( 0.500612,  0.209459),
    vec2(-0.431909, -0.225753),
    vec2(-0.695346, -0.449835),
    vec2( 0.725266, -0.018760),
    vec2( 0.025038,  0.264728),
    vec2( 0.282059, -0.020923),
    vec2(-0.206744,  0.038148),
    vec2(-0.516025,  0.674610),
    vec2( 0.043250, -0.181413),
    vec2(-0.649874, -0.840982),
    vec2( 0.143402,  0.831449),
    vec2( 0.320628,  0.851423),
    vec2(-0.589255,  0.838083),
    vec2(-0.855729, -0.778299),
    vec2(-0.852082,  0.614212),
    vec2(-0.361646, -0.830580),
    vec2( 0.855979,  0.835868),
    vec2( 0.832173,  0.583244),
    vec2( 0.585277, -0.858893),
    vec2(-0.584186,  0.520477),
    vec2(-0.821998, -0.096579),
    vec2( 0.379092, -0.352095),
    vec2( 0.863171, -0.363773),
    vec2( 0.107531,  0.631342),
    vec2(-0.174732, -0.863461),
    vec2(-0.809089,  0.183563),
    vec2( 0.183901, -0.390505),
    vec2(-0.363365, -0.570793),
    vec2( 0.842674,  0.320929),
    vec2(-0.837879,  0.857689),
    vec2(-0.823117, -0.290371),
    vec2(-0.320593, -0.344147),
    vec2( 0.597322,  0.829427),
    vec2( 0.345881, -0.842229),
    vec2( 0.862065,  0.093642),
    vec2( 0.152963,  0.079205),
    vec2( 0.681161, -0.381325),
    vec2(-0.090109, -0.361578),
    vec2(-0.833870, -0.554747),
    vec2(-0.104335, -0.588888),
    vec2( 0.618905, -0.614282),
    vec2( 0.390102, -0.145821),
    vec2( 0.872450, -0.113537),
    vec2( 0.847215, -0.837116),
    vec2(-0.309506, -0.125079),
    vec2( 0.153243, -0.597383),
    vec2(-0.437639,  0.834608),
    vec2(-0.540926, -0.101547),
    vec2( 0.627023,  0.315180),
    vec2( 0.080225, -0.849945),
    vec2( 0.633539,  0.569322),
    vec2(-0.091158,  0.371349),
    vec2( 0.600866,  0.090656),
    vec2(-0.585325, -0.578735),
    vec2(-0.367994,  0.602783),
    vec2(-0.533179,  0.350425)
);

const vec2 PCSS_PCF_SAMPLES[128] = vec2[](
    vec2( 0.000000,  0.000000),
    vec2(-0.997774,  0.991896),
    vec2( 0.964062, -0.973045),
    vec2( 0.993988,  0.920226),
    vec2(-0.991900, -0.825764),
    vec2(-0.009252, -0.992052),
    vec2(-0.018534,  0.969326),
    vec2(-0.863276,  0.129006),
    vec2( 0.986581,  0.013661),
    vec2( 0.480985,  0.478865),
    vec2( 0.493613, -0.491334),
    vec2(-0.487662, -0.359990),
    vec2(-0.508305,  0.636924),
    vec2(-0.507460, -0.982782),
    vec2( 0.481634,  0.994667),
    vec2( 0.007916, -0.505493),
    vec2(-0.017469,  0.506761),
    vec2( 0.991862, -0.490194),
    vec2( 0.491671, -0.012208),
    vec2( 0.485796, -0.997906),
    vec2(-0.998414, -0.332369),
    vec2(-0.971045,  0.526537),
    vec2(-0.405404,  0.202314),
    vec2( 0.922770,  0.463119),
    vec2(-0.298919, -0.669412),
    vec2(-0.446436,  0.975152),
    vec2(-0.660044, -0.657266),
    vec2( 0.744841, -0.239558),
    vec2( 0.691560,  0.741106),
    vec2( 0.246705, -0.758701),
    vec2( 0.224323, -0.243971),
    vec2( 0.224778,  0.736382),
    vec2( 0.252236,  0.226778),
    vec2( 0.709220, -0.759734),
    vec2( 0.723383,  0.211612),
    vec2(-0.742103, -0.157209),
    vec2(-0.680432,  0.369528),
    vec2(-0.189827, -0.263832),
    vec2(-0.731754,  0.899398),
    vec2(-0.213502,  0.718402),
    vec2(-0.294733,  0.457098),
    vec2(-0.135447,  0.239317),
    vec2(-0.460232, -0.075460),
    vec2(-0.771064, -0.986501),
    vec2( 0.734525,  0.998473),
    vec2(-0.765024, -0.420241),
    vec2(-0.976630, -0.579241),
    vec2( 0.980248, -0.734965),
    vec2( 0.005613, -0.749260),
    vec2( 0.250898, -0.488351),
    vec2(-0.263055, -0.993972),
    vec2(-0.627041,  0.119043),
    vec2( 0.757641, -0.528727),
    vec2( 0.226747,  0.493724),
    vec2(-0.995434, -0.081860),
    vec2(-0.927879,  0.763919),
    vec2( 0.491040,  0.224498),
    vec2( 0.233168, -0.010528),
    vec2(-0.741437,  0.614902),
    vec2( 0.212555,  0.995082),
    vec2(-0.229779,  0.010716),
    vec2( 0.697255,  0.491566),
    vec2( 0.722626, -0.993698),
    vec2( 0.259044, -0.993314),
    vec2( 0.967632,  0.691075),
    vec2( 0.981032, -0.225855),
    vec2( 0.471502, -0.244372),
    vec2( 0.728142, -0.009025),
    vec2( 0.965913,  0.223947),
    vec2(-0.231091,  0.957039),
    vec2( 0.480755, -0.758652),
    vec2( 0.459014,  0.766296),
    vec2( 0.020215, -0.298464),
    vec2(-0.205655, -0.477500),
    vec2(-0.007305,  0.757152),
    vec2(-0.897268,  0.334284),
    vec2(-0.496513, -0.770506),
    vec2(-0.171818, -0.820356),
    vec2( 0.066340,  0.317261),
    vec2(-0.496637,  0.423875),
    vec2(-0.838082, -0.719658),
    vec2(-0.377480,  0.809945),
    vec2( 0.362910,  0.606107),
    vec2(-0.458364, -0.561371),
    vec2( 0.636219, -0.382376),
    vec2(-0.358810, -0.233990),
    vec2(-0.639965, -0.869876),
    vec2( 0.143122, -0.628582),
    vec2( 0.125831, -0.881304),
    vec2(-0.629677,  0.758081),
    vec2( 0.837941, -0.373685),
    vec2(-0.984403, -0.997382),
    vec2( 0.370122,  0.108035),
    vec2(-0.344542,  0.618027),
    vec2( 0.836953,  0.854349),
    vec2( 0.102492,  0.141352),
    vec2( 0.371645,  0.347240),
    vec2( 0.337562,  0.873650),
    vec2(-0.608888, -0.482344),
    vec2(-0.065538, -0.156862),
    vec2(-0.134319, -0.632672),
    vec2( 0.851512, -0.845985),
    vec2( 0.595368, -0.633434),
    vec2(-0.565502, -0.198297),
    vec2( 0.603688,  0.878775),
    vec2( 0.111159,  0.612045),
    vec2( 0.607409, -0.150080),
    vec2( 0.381021, -0.616714),
    vec2(-0.358205, -0.857673),
    vec2( 0.363122, -0.371062),
    vec2( 0.808019,  0.616949),
    vec2( 0.550629,  0.633843),
    vec2( 0.109479,  0.874479),
    vec2(-0.894985, -0.201681),
    vec2( 0.100198, -0.147784),
    vec2( 0.606725,  0.323154),
    vec2(-0.813416,  0.476284),
    vec2(-0.552401,  0.265451),
    vec2( 0.872602, -0.095667),
    vec2(-0.768255, -0.007862),
    vec2(-0.163878,  0.572029),
    vec2( 0.361361, -0.125722),
    vec2(-0.163269,  0.391015),
    vec2( 0.825686,  0.330295),
    vec2( 0.840911, -0.652938),
    vec2( 0.591210,  0.106423),
    vec2( 0.850853,  0.090375),
    vec2( 0.607736, -0.864476)
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
    vec2 lightHalfSize = vec2(areaLight.rightVector_halfWidth.w, areaLight.upVector_halfHeight.w);
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
    vec2 normalXY=vec2(dot(N,areaLight.rightVector_halfWidth.xyz), dot(N,areaLight.upVector_halfHeight.xyz));
    vec2 nearHalfSize=areaLight.shadowParams.zw;
    return -2.0 * farPlane * nearHalfSize * normalXY / ((farPlane - nearPlane) * planeDistance);
}

bool FindAverageBlockerDepth(sampler2D shadowMap, vec2 uv, float receiverDepth, vec2 depthGradient, float nearPlane, float farPlane, float depthBias, vec2 searchRadiusUV, out float avgBlockerZ)
{
    float blockerZSum = 0.0;
    int blockerCount = 0;

    ivec2 texSize = textureSize(shadowMap, 0);

    int sampleCount = clamp(PCSS_BLOCKER_SAMPLE_COUNT, 1, PCSS_BLOCKER_SAMPLES.length());
    for (int i = 0; i < sampleCount; ++i) {
        // 采样点UV
        vec2 sampleUV = uv + PCSS_BLOCKER_SAMPLES[i] * searchRadiusUV;
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
    vec2 lightHalfSize = vec2(areaLight.rightVector_halfWidth.w, areaLight.upVector_halfHeight.w);
    vec2 penumbra = lightHalfSize * max(receiverZ - avgBlockerZ, 0.0) / max(avgBlockerZ, 1e-4);
    vec2 receiverPlaneSize = 2.0 * nearHalfSize * receiverZ / max(nearPlane, 1e-4);
    return penumbra / receiverPlaneSize;
}

float FilterAreaShadowPCF( sampler2D shadowMap, vec2 uv, float receiverDepth, vec2 depthGradient, float depthBias, vec2 filterRadiusUV)
{
    float shadow = 0.0;
    int validFilterCount = 0;
    ivec2 texSize = textureSize(shadowMap, 0);
    int sampleCount = clamp(PCSS_PCF_SAMPLE_COUNT, 1, PCSS_PCF_SAMPLES.length());
    for (int i = 0; i < sampleCount; ++i) {
        // 采样点UV
        vec2 sampleUV = uv + PCSS_PCF_SAMPLES[i] * filterRadiusUV;
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
    // 把线性偏移作用在线性深度上，即世界空间
    float biasedReceiverZ = max(receiverZ - bias, nearPlane);
    // 把偏移前后的线性深度转为光源视角投影深度，相减，获得光源投影空间偏移
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
