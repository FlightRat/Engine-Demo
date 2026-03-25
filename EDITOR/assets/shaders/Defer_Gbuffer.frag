#version 450 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;
layout (location = 3) out vec3 gRefl;

struct Material{
    vec4 color;
    float shininess;
    sampler2D diffuse;
    sampler2D specular;
    sampler2D normal;
    sampler2D reflection;

    bool useDiffuse;
    bool useSpecular;
    bool useNormal;
    bool useReflect;
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
        gNormal = texture(material.normal, uv).rgb;
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
        vec4 checkerColor = (checker > 0.5) ? vec4(1.0) : vec4(0.1);
        gAlbedoSpec = checkerColor;
    } 
    else{
        gAlbedoSpec.rgb = material.color.rgb;
        if(useTexture && material.useDiffuse){
            gAlbedoSpec.rgb = texture(material.diffuse, uv).rgb;
        }
        gAlbedoSpec.a = 1.0;
        if(useTexture&& material.useSpecular){
            gAlbedoSpec.a = texture(material.specular, uv).r;
        }
    }

    // gRefl
    gRefl = vec3(0.0);
    if(useTexture&& material.useReflect){
        gRefl = vec3(texture(material.reflection, uv));
    }
}