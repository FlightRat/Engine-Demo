#version 450 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoord;
    mat3 TBN;
    vec4 currentClip;
    vec4 previousClip;
} vs_out;

uniform mat4 model;
uniform mat4 previousModel;
uniform mat4 previousViewProjection;
uniform mat3 normalMatrix;
layout (std140) uniform Matrices
{
    mat4 view;
    mat4 projection;
};

void main()
{
    // mat3 normalMatrix = transpose(inverse(mat3(model)));
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 B = normalize(normalMatrix * aBitangent);
    vec3 N = normalize(normalMatrix * aNormal);
    mat3 TBN = mat3(T, B, N);

    vs_out.FragPos = vec3(model * vec4(aPosition, 1.0));
    vs_out.Normal = normalMatrix * aNormal;
    vs_out.TexCoord = aTexCoord;
    vs_out.TBN = TBN;
    
    vs_out.currentClip = projection * view * model * vec4(aPosition, 1.0f);
    vs_out.previousClip = previousViewProjection * previousModel * vec4(aPosition, 1.0);
    gl_Position = vs_out.currentClip;
}