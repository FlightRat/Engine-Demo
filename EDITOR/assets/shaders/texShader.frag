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
uniform sampler2D tex;
uniform DirLight dirLight;
uniform PointLight pointLights[NR_POINT_LIGHTS];

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 texColor)
{
	vec3 lightDir = normalize(light.position - fragPos);
	vec3 reflectDir = reflect(-lightDir, normal);

	float diff = max(dot(normal, lightDir), 0.0);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0),64.0f);

	float distance = length(light.position - fragPos);
	float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

	vec3 diffuse = light.diffuse * diff * texColor;
	vec3 specular = light.specular * spec * texColor;
	vec3 ambient = light.ambient * texColor;

	diffuse *= attenuation;
	specular *= attenuation;
	ambient *= attenuation;

	return (diffuse + specular + ambient);
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 texColor)
{
    vec3 lightDir = normalize(-light.direction);
    vec3 reflectDir = reflect(-lightDir, normal);

	float diff = max(dot(normal, lightDir), 0.0);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64.0f);

	vec3 diffuse = light.diffuse * diff * texColor;
	vec3 specular = light.specular * spec * texColor;
	vec3 ambient = light.ambient * texColor;

	return (diffuse + specular + ambient);
}

void main()
{
	vec2 flip_coord = vec2(TexCoord.x, 1.0-TexCoord.y);
	vec4 texColor = texture(tex, flip_coord);

	vec3 norm = normalize(Normal);
	vec3 viewDir = normalize(viewPos - FragPos);

	vec3 result = CalcDirLight(dirLight, norm, viewDir, texColor.rgb);
	for(int i = 0;i<NR_POINT_LIGHTS;i++)
		result += CalcPointLight(pointLights[i], norm, FragPos, viewDir, texColor.rgb);

    FragColor = vec4(result, 1.0);
}