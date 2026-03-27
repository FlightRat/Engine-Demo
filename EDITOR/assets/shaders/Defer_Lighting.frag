#version 450 core
#define NR_DIR_LIGHTS   4
#define NR_POINT_LIGHTS 4
#define shininess 64.0

struct DirLight {
    vec4 direction;
    vec4 diffuse;
    vec4 specular;
    vec4 ambient;
    mat4 lightSpaceMatrices;
};

struct PointLight {
    vec4 position;
    vec4 diffuse;
    vec4 specular;
    vec4 ambient;
    vec4 attenuation;
};

out vec4 FragColor;
in vec2 TexCoord;

uniform vec3 viewPos;
uniform float far_plane;
uniform bool use_SSAO;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform sampler2D gRefl;
uniform sampler2D ssao;

uniform samplerCube skybox;
uniform samplerCube shadowCubeMap[NR_POINT_LIGHTS];
uniform sampler2D shadowMaps[NR_DIR_LIGHTS];

layout (std140) uniform DirLights {
    DirLight dir_lights[NR_DIR_LIGHTS];
};
layout (std140) uniform PointLights {
    PointLight point_lights[NR_POINT_LIGHTS];
};

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 albedo, float specMap, float ambientOcclusion, float shadow){
    vec3 lightDir = normalize(vec3(light.position) - fragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir); // Blinn-Phong 核心

    // 漫反射
    float diff = max(dot(normal, lightDir), 0.0);
    
    // 高光 (Blinn-Phong)
    // 注意：Blinn-Phong 的 shininess 通常需要是 Phong 的 2-4 倍才能达到类似的视觉锐度
    float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);

    // 衰减
    float distance = length(vec3(light.position) - fragPos);
    float attenuation = 1.0 / (light.attenuation.x + light.attenuation.y * distance + light.attenuation.z * (distance * distance));

    // 合并
    vec3 ambient  = vec3(light.ambient)  * ambientOcclusion * albedo * attenuation;
    vec3 diffuse  = vec3(light.diffuse)  * diff * albedo * attenuation;
    vec3 specular = vec3(light.specular) * spec * specMap * attenuation;

    // return ambient + diffuse + specular;
    return ambient + (1.0 - shadow) * (diffuse + specular);
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 albedo, float specMap, float ambientOcclusion, float shadow){
    vec3 lightDir = normalize(-vec3(light.direction));
    vec3 halfwayDir = normalize(lightDir + viewDir);

    float diff = max(dot(normal, lightDir), 0.0);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);

    vec3 ambient  = vec3(light.ambient)  * ambientOcclusion * albedo;
    vec3 diffuse  = vec3(light.diffuse)  * diff * albedo;
    vec3 specular = vec3(light.specular) * spec * specMap;

    // return ambient + diffuse + specular;
    return ambient + (1.0 - shadow) * (diffuse + specular);
}

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

vec3 sampleOffsetDirections[20] = vec3[](
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


void main()
{             
    vec3 FragPos = texture(gPosition, TexCoord).rgb;
    vec3 Normal = texture(gNormal, TexCoord).rgb;
    vec3 Diffuse = texture(gAlbedoSpec, TexCoord).rgb;
    float Specular = texture(gAlbedoSpec, TexCoord).a;
    vec3 Reflection = texture(gRefl, TexCoord).rgb;
    float AmbientOcclusion = 1.0;
    if (use_SSAO)
        AmbientOcclusion = texture(ssao, TexCoord).r;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 I = normalize(FragPos - viewPos);
    vec3 R = reflect(I,normalize(Normal));

    vec3 result = vec3(0.0);

    // 定向光
    for (int i = 0; i < NR_DIR_LIGHTS; i++) {
        if (dir_lights[i].direction.w < 0.5) continue;

        vec3 lightDir = normalize(-vec3(dir_lights[i].direction));
        vec4 FragPosLightSpace = dir_lights[i].lightSpaceMatrices * vec4(FragPos, 1.0);

        float shadow = ShadowCalculation_dir(shadowMaps[i], FragPosLightSpace, Normal, lightDir);
        result += CalcDirLight(dir_lights[i], Normal, viewDir, Diffuse, Specular, AmbientOcclusion, shadow);
    }

    // 点光源
    for(int i = 0; i < NR_POINT_LIGHTS; i++) {
        // 通过 position.w 或 attenuation 判断点光源是否有效
        if (point_lights[i].attenuation.x > 0.0 || point_lights[i].attenuation.y > 0.0 || point_lights[i].attenuation.z > 0.0)
        {
            vec3 lightPos = vec3(point_lights[i].position);
            float shadow = ShadowCalculation_point(shadowCubeMap[i], lightPos, FragPos, Normal);
            result += CalcPointLight(point_lights[i], Normal, FragPos, viewDir, Diffuse, Specular, AmbientOcclusion, shadow);
        }
    }

    // reflect map
    result += Reflection * texture(skybox, R).rgb;
    
    // 4. 输出
    result = result / (result + vec3(1.0));     // Reinhard Tone Mapping
    result = pow(result, vec3(1.0/2.2));        // gamma correction
    FragColor = vec4(result, 1.0);

    // for test
    //if(length(FragPos) < 0.01) discard; 
    //vec3 viewCol = abs(FragPos) * 0.05; 
    //FragColor = vec4(viewCol, 1.0); 
    //FragColor = vec4(Normal, 1.0);
    //FragColor = vec4(Diffuse, 1.0);
    //FragColor = vec4(Specular,Specular,Specular, 1.0);
}