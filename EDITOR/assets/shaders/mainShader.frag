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

// --- 统一的 Uniforms ---
uniform vec3 viewPos;
uniform vec4 objectColor; // 对应原来的 color
uniform sampler2D objectTexture; // 对应原来的 tex
uniform bool useTexture; // 新增：控制开关
uniform DirLight dirLight;
uniform PointLight pointLights[NR_POINT_LIGHTS];

// 注意：这里删除了 texColor 参数，只传一个通用的 albedo
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 albedo)
{
    vec3 lightDir = normalize(light.position - fragPos);
    vec3 reflectDir = reflect(-lightDir, normal);

    float diff = max(dot(normal, lightDir), 0.0);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64.0f);

    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    // 使用传入的 albedo
    vec3 diffuse = light.diffuse * diff * albedo;
    vec3 specular = light.specular * spec * albedo; // Specular 通常也可以受 albedo 影响，或者单独用 specular map
    vec3 ambient = light.ambient * albedo;

    return (diffuse + specular + ambient) * attenuation;
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 albedo)
{
    vec3 lightDir = normalize(-light.direction);
    vec3 reflectDir = reflect(-lightDir, normal);

    float diff = max(dot(normal, lightDir), 0.0);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64.0f);

    vec3 diffuse = light.diffuse * diff * albedo;
    vec3 specular = light.specular * spec * albedo;
    vec3 ambient = light.ambient * albedo;

    return (diffuse + specular + ambient);
}

void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    // --- 核心合并逻辑 ---
    vec4 finalAlbedo = objectColor;
    
    if (useTexture) {
        vec2 flip_coord = vec2(TexCoord.x, 1.0 - TexCoord.y);
        vec4 texColor = texture(objectTexture, flip_coord);
        // 这里的做法是：如果有纹理，就让纹理乘以颜色。
        // 如果你只想显示纹理，C++端把 objectColor 设为 vec4(1.0) 即可（白色）
        finalAlbedo *= texColor; 
    }

    vec3 result = CalcDirLight(dirLight, norm, viewDir, finalAlbedo.rgb);
    
    for(int i = 0; i < NR_POINT_LIGHTS; i++)
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir, finalAlbedo.rgb);

    FragColor = vec4(result, finalAlbedo.a);
}