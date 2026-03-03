#version 450 core
#define NR_POINT_LIGHTS 1

struct Material
{
    // 材质属性
    vec4 color;         // 基础颜色 (Tint)
    float shininess;    // 高光反光度 (2, 4, 8, 16, 32, 64, ...)

    // 纹理采样器 (对应 C++ 绑定的纹理单元)
    sampler2D diffuse;  // 漫反射贴图 (纹理单元 0)
    sampler2D specular; // 高光贴图   (纹理单元 1)

    // 状态标志 (由 C++ 传入，判断是否有对应的纹理)
    bool useDiffuse;
    bool useSpecular;
};

struct DirLight {
    vec3 direction;
    vec3 diffuse;
    vec3 specular;
    vec3 ambient;
};

struct PointLight {
    vec3 position;
    vec3 diffuse;
    vec3 specular;
    vec3 ambient;
    float constant;
    float linear;
    float quadratic;
};

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

// --- Uniforms ---
uniform bool useTexture;
uniform vec3 viewPos;
uniform Material material;
uniform DirLight dirLight;
uniform PointLight pointLights[NR_POINT_LIGHTS];

// 计算点光源
// 参数说明: 
// diffuseColor: 材质的漫反射颜色 (纹理 * 颜色)
// specularStrength: 材质的高光强度 (来自高光贴图)
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffuseColor, vec3 specularStrength)
{
    vec3 lightDir = normalize(light.position - fragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    
    float diff = max(dot(normal, lightDir), 0.0);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    // 衰减
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    // 合并结果
    // 注意：diffuseColor 已经包含了 material.color * texture
    vec3 ambient  = light.ambient  * diffuseColor;
    vec3 diffuse  = light.diffuse  * diff * diffuseColor;
    // 注意：高光通常不受漫反射颜色影响，而是受高光贴图(specularStrength)和光源颜色影响
    vec3 specular = light.specular * spec * specularStrength; 

    return (ambient + diffuse + specular) * attenuation;
}

// 计算定向光
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 diffuseColor, vec3 specularStrength)
{
    vec3 lightDir = normalize(-light.direction);
    
    // 漫反射
    float diff = max(dot(normal, lightDir), 0.0);
    
    // 高光
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    // 合并
    vec3 ambient  = light.ambient * diffuseColor;
    vec3 diffuse  = light.diffuse * diffuseColor * diff;
    vec3 specular = light.specular * specularStrength * spec;

    return (ambient + diffuse + specular);
}

void main()
{
    // 0. 准备数据
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    // 处理纹理坐标翻转
    vec2 flip_coord = vec2(TexCoord.x, 1.0 - TexCoord.y);

    // 1. 获取漫反射颜色 (Albedo)
    // 默认为材质的颜色
    vec4 baseColor = material.color; 
    if (material.useDiffuse && useTexture) {
        // 如果有漫反射贴图，颜色 = 材质颜色 * 纹理颜色
        baseColor *= texture(material.diffuse, flip_coord);
    }
    
    // 如果 alpha 通道太低，可以丢弃 (可选)
    // if(baseColor.a < 0.1) discard;

    // 2. 获取高光强度 (Specular Map)
    // 默认为灰色 (0.5) 或者白色 (1.0)，表示全物体都有高光
    vec3 specMap = vec3(1.0); 
    if (material.useSpecular && useTexture) {
        // 采样高光贴图 (通常是黑白图，越白越亮)
        specMap = vec3(texture(material.specular, flip_coord).r);
    }

    // 3. 计算光照
    vec3 result = CalcDirLight(dirLight, norm, viewDir, baseColor.rgb, specMap);
    
    for(int i = 0; i < NR_POINT_LIGHTS; i++)
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir, baseColor.rgb, specMap);

    FragColor = vec4(result, baseColor.a);
}