#version 450 core
#define NR_DIR_LIGHTS   4
#define NR_POINT_LIGHTS 4

struct Material{
    vec4 color;
    float shininess;
    sampler2D diffuse;
    sampler2D specular; 
    sampler2D reflection;
    sampler2D normal;

    bool useDiffuse;
    bool useSpecular;
    bool useReflect;
    bool useNormal;
};

struct DirLight {
    vec4 direction;
    vec4 diffuse;
    vec4 specular;
    vec4 ambient;
};

struct PointLight {
    vec4 position;
    vec4 diffuse;
    vec4 specular;
    vec4 ambient;
    vec4 attenuation;
};

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoord;
    vec4 FragPosLightSpace[NR_DIR_LIGHTS]; // 接收数组
} fs_in;

out vec4 FragColor;

// --- Uniforms ---
uniform bool bug;
uniform bool flipUV;
uniform bool useTexture;

uniform vec3 viewPos;
uniform float far_plane;

uniform Material material;

uniform samplerCube skybox;
uniform samplerCube shadowCubeMap[NR_POINT_LIGHTS];
uniform sampler2D shadowMaps[NR_DIR_LIGHTS];

layout (std140, binding = 1) uniform DirLights {
    DirLight dir_lights[NR_DIR_LIGHTS];
};
layout (std140, binding = 2) uniform PointLights {
    PointLight point_lights[NR_POINT_LIGHTS];
};

// 优化：使用 Blinn-Phong 模型 (Halfway Vector)
// 比 reflect() 计算更快，且高光过渡更自然
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 albedo, vec3 specMap, float shadow){
    vec3 lightDir = normalize(vec3(light.position) - fragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir); // Blinn-Phong 核心

    // 漫反射
    float diff = max(dot(normal, lightDir), 0.0);
    
    // 高光 (Blinn-Phong)
    // 注意：Blinn-Phong 的 shininess 通常需要是 Phong 的 2-4 倍才能达到类似的视觉锐度
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);

    // 衰减
    float distance = length(vec3(light.position) - fragPos);
    float attenuation = 1.0 / (light.attenuation.x + light.attenuation.y * distance + light.attenuation.z * (distance * distance));

    // 合并
    vec3 ambient  = vec3(light.ambient)  * albedo * attenuation;
    vec3 diffuse  = vec3(light.diffuse)  * diff * albedo * attenuation;
    vec3 specular = vec3(light.specular) * spec * specMap * attenuation;

    // return ambient + diffuse + specular;
    return ambient + (1 - shadow) * (diffuse + specular);
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 albedo, vec3 specMap, float shadow){
    vec3 lightDir = normalize(-vec3(light.direction));
    vec3 halfwayDir = normalize(lightDir + viewDir);

    float diff = max(dot(normal, lightDir), 0.0);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);

    vec3 ambient  = vec3(light.ambient)  * albedo;
    vec3 diffuse  = vec3(light.diffuse)  * diff * albedo;
    vec3 specular = vec3(light.specular) * spec * specMap;

    // return ambient + diffuse + specular;
    return ambient + (1 - shadow) * (diffuse + specular);
}

// 参数改为传入对应的 sampler 和光方向，而不依赖全局变量
float ShadowCalculation_dir(sampler2D shadowMap, vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    // 超出光锥范围直接不产生阴影
    if (projCoords.z > 1.0) return 0.0;

    float currentDepth = projCoords.z;
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);

    // PCF 3x3
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

vec3 sampleOffsetDirections[20] = vec3[]
(
   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);
float ShadowCalculation_point(samplerCube shadowCubeMap, vec3 lightPos, vec3 fragPos, vec3 normal){
    vec3 fragToLight = fragPos - lightPos;
    float currentDepth = length(fragToLight);
    
    vec3 lightDir = normalize(-fragToLight);
    float bias = max(0.15 * (1.0 - dot(normal, lightDir)), 0.05);
    
    float shadow = 0.0;
    int samples = 20;
    float viewDistance = length(viewPos - fragPos);
    // 距离摄像机越远，采样的散布半径越小，或者根据需要写死为 0.05
    float diskRadius = (1.0 + (viewDistance / far_plane)) / 25.0;  

    for(int i = 0; i < samples; ++i)
    {
        float closestDepth = texture(shadowCubeMap, fragToLight + sampleOffsetDirections[i] * diskRadius).r;
        closestDepth *= far_plane;   // 恢复到线性距离
        if(currentDepth - bias > closestDepth)
            shadow += 1.0;
    }
    shadow /= float(samples);  
    
    return shadow;
}

void main(){
    // 1. 几何数据准备
    vec3 norm = normalize(fs_in.Normal);
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    vec3 I = normalize(fs_in.FragPos - viewPos);
    vec3 R = reflect(I,normalize(fs_in.Normal));
    
    // UV 处理
    vec2 uv = fs_in.TexCoord;
    if (flipUV) uv.y = 1.0 - uv.y;

    // 2. 材质属性获取 (Albedo 和 SpecularMap)
    vec3 albedo;
    vec3 specMap;

    if(bug) {
        // --- Bug 调试模式 ---
        float scale = 10.0;
        vec2 bugUV = uv * scale;
        float checker = mod(floor(bugUV.x) + floor(bugUV.y), 2.0);
        
        // 黑白格子作为基础色和高光图
        vec3 checkerColor = (checker > 0.5) ? vec3(1.0) : vec3(0.1);
        albedo = checkerColor;
        specMap = checkerColor; 
    } 
    else {
        // --- 正常材质模式 ---
        // 漫反射基础色
        vec4 baseColor = material.color;
        if (material.useDiffuse && useTexture) {
            baseColor *= texture(material.diffuse, uv);
        }
        albedo = baseColor.rgb;

        // 高光采样
        specMap = vec3(1.0); // 默认全白高光
        if (material.useSpecular && useTexture) {
            specMap = vec3(texture(material.specular, uv).r); // 通常高光图是灰度的，取 r 即可
        }
    }

    // 3. 统一光照计算 (避免代码重复)
    vec3 result = vec3(0.0);

    // 定向光
    for (int i = 0; i < NR_DIR_LIGHTS; i++) {
        if (dir_lights[i].direction.w < 0.5) continue;

        vec3 lightDir = normalize(-vec3(dir_lights[i].direction));

        float shadow = ShadowCalculation_dir(shadowMaps[i], fs_in.FragPosLightSpace[i], norm, lightDir);
        result += CalcDirLight(dir_lights[i], norm, viewDir, albedo, specMap, shadow);
    }

    // 点光源
    for(int i = 0; i < NR_POINT_LIGHTS; i++) {
        // 通过 position.w 或 attenuation 判断点光源是否有效
        if (point_lights[i].attenuation.x > 0.0 || point_lights[i].attenuation.y > 0.0 || point_lights[i].attenuation.z > 0.0)
        {
            vec3 lightPos = vec3(point_lights[i].position);
            float shadow = ShadowCalculation_point(shadowCubeMap[i], lightPos, fs_in.FragPos, norm);
            result += CalcPointLight(point_lights[i], norm, fs_in.FragPos, viewDir, albedo, specMap, shadow);
        }
    }

    // reflect map
    if (material.useReflect && useTexture) {
        result += vec3(texture(material.reflection, fs_in.TexCoord)) * texture(skybox, R).rgb;
    }
    
    // 4. 输出
    result = pow(result, vec3(1.0/2.2));    // gamma correction
    FragColor = vec4(result, 1.0);
}