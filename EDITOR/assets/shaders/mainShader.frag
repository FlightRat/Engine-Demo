#version 450 core
#define NR_POINT_LIGHTS 4

struct Material{
    vec4 color;
    float shininess;
    samplerCube skybox;
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

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

// --- Uniforms ---
uniform bool bug;
uniform bool flipUV;
uniform bool useTexture;
uniform vec3 viewPos;
uniform Material material;
layout (std140) uniform Lighting
{
    DirLight dirLight;
    PointLight pointLights[NR_POINT_LIGHTS];
};

// 优化：使用 Blinn-Phong 模型 (Halfway Vector)
// 比 reflect() 计算更快，且高光过渡更自然
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 albedo, vec3 specMap)
{
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
    vec3 ambient  = vec3(light.ambient)  * albedo;
    vec3 diffuse  = vec3(light.diffuse)  * diff * albedo;
    vec3 specular = vec3(light.specular) * spec * specMap;

    return (ambient + diffuse + specular) * attenuation;
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 albedo, vec3 specMap)
{
    vec3 lightDir = normalize(-vec3(light.direction));
    vec3 halfwayDir = normalize(lightDir + viewDir);

    float diff = max(dot(normal, lightDir), 0.0);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);

    vec3 ambient  = vec3(light.ambient)  * albedo;
    vec3 diffuse  = vec3(light.diffuse)  * diff * albedo;
    vec3 specular = vec3(light.specular) * spec * specMap;

    return (ambient + diffuse + specular);
}

void main()
{
    // 1. 几何数据准备
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 I = normalize(FragPos - viewPos);
    vec3 R = reflect(I,normalize(Normal));
    
    // UV 处理
    vec2 uv = TexCoord;
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
    result += CalcDirLight(dirLight, norm, viewDir, albedo, specMap);

    // 点光源循环
    // 编译器通常会自动展开这个固定次数的循环
    for(int i = 0; i < NR_POINT_LIGHTS; i++) {
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir, albedo, specMap);
    }

    // reflect map
    if (material.useReflect && useTexture) {
        result += vec3(texture(material.reflection, TexCoord)) * texture(material.skybox, R).rgb;
    }
    
    // 4. 输出
    result = pow(result, vec3(1.0/2.2));    // gamma correction
    FragColor = vec4(result, 1.0);
}