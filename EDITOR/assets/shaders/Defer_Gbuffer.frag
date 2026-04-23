#version 450 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec3 gAlbedo;
layout (location = 3) out vec3 gMRA;

layout(binding = 0) uniform sampler2D albedoMap; 
layout(binding = 1) uniform sampler2D normalMap; 
layout(binding = 2) uniform sampler2D metallicMap; 
layout(binding = 3) uniform sampler2D roughnessMap; 
layout(binding = 4) uniform sampler2D aoMap; 

struct Material{
    vec4 color;
    float metallic;
    float roughness;
    float ao;

    bool useAlbedo;
    bool useNormal;
    bool useMetallic;
    bool useRoughness;
    bool useAo;
};

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoord;
    mat3 TBN;
} fs_in;

// --- Uniforms ---
uniform bool bug;
uniform bool flipUV;
uniform bool useTexture;
uniform Material material;

void main()
{    
    vec2 uv = fs_in.TexCoord;
    if (flipUV) uv.y = 1.0 - uv.y;

    // gPosition
    gPosition = fs_in.FragPos;

    // gNormal
    gNormal = normalize(fs_in.Normal);
    if(useTexture && material.useNormal){
        gNormal = texture(normalMap, uv).rgb;
        gNormal = normalize(gNormal * 2.0 - 1.0);
        gNormal = normalize(fs_in.TBN * gNormal);
    }

    // gAlbedo && gSpec
    if(bug) {
        // --- Bug 调试模式 ---
        float scale = 10.0;
        vec2 bugUV = uv * scale;
        float checker = mod(floor(bugUV.x) + floor(bugUV.y), 2.0);
        
        // 黑白格子作为基础色和高光图
        vec3 checkerColor = (checker > 0.5) ? vec3(1.0) : vec3(0.1);
        gAlbedo = checkerColor;
    } 
    else{
        gAlbedo = material.color.rgb;
        if(useTexture && material.useAlbedo){
            gAlbedo = pow(texture(albedoMap, uv).rgb, vec3(2.2));
            }
    }

    // metallic + roughness + ao
    gMRA = vec3(material.metallic, material.roughness, material.ao);
    if(useTexture&& material.useMetallic){
        gMRA.x = texture(metallicMap, uv).r;
    }
    if(useTexture&& material.useRoughness){
        gMRA.y = texture(roughnessMap, uv).r;
    }
    if(useTexture&& material.useAo){
        gMRA.z = texture(aoMap, uv).r;
    }
}