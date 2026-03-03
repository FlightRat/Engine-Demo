#version 450 core
#define NR_POINT_LIGHTS 1

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

uniform vec3 viewPos;
uniform DirLight dirLight;
uniform PointLight pointLights[NR_POINT_LIGHTS];

// 修改点 1: 增加 objectColor 参数，不再使用全局 color
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 objectColor)
{
    vec3 lightDir = normalize(light.position - fragPos);
    vec3 reflectDir = reflect(-lightDir, normal);

    float diff = max(dot(normal, lightDir), 0.0);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64.0f);

    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    // 使用传入的 objectColor
    vec3 diffuse = light.diffuse * diff * objectColor;
    vec3 specular = light.specular * spec * objectColor; // 高光通常也可以是白色的，看需求，这里随物体颜色
    vec3 ambient = light.ambient * objectColor;

    diffuse *= attenuation;
    specular *= attenuation;
    ambient *= attenuation;

    return (diffuse + specular + ambient);
}

// 修改点 2: 增加 objectColor 参数
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 objectColor)
{
    vec3 lightDir = normalize(-light.direction);
    vec3 reflectDir = reflect(-lightDir, normal);

    float diff = max(dot(normal, lightDir), 0.0);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64.0f);

    // 使用传入的 objectColor
    vec3 diffuse = light.diffuse * diff * objectColor;
    vec3 specular = light.specular * spec * objectColor;
    vec3 ambient = light.ambient * objectColor;

    return (diffuse + specular + ambient);
}

void main()
{
    // --- 改进版：基于 UV 的棋盘格 ---
    float scale = 10.0; // 控制格子密度
    
    // 使用 UV 坐标而不是世界坐标
    // 这样纹理会“贴”在模型表面，随模型移动/旋转
    vec2 uv = TexCoord * scale;
    
    // 基础算法：判断 x 和 y 的奇偶性
    // mod(..., 2.0) 结果非 0 即 1
    float checker = mod(floor(uv.x) + floor(uv.y), 2.0);

    // 颜色混合
    vec3 objectColor = (checker > 0.5) ? vec3(1.0) : vec3(0.1); 
    // ----------------------------------

    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 result = CalcDirLight(dirLight, norm, viewDir, objectColor);
    for(int i = 0; i < NR_POINT_LIGHTS; i++)
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir, objectColor);

    FragColor = vec4(result, 1.0);
}