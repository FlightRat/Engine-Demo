#version 450 core

out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D tex;

void main()
{
	vec2 flip_coord = vec2(TexCoord.x, 1.0-TexCoord.y);
	vec4 texColor = texture(tex, flip_coord);
	FragColor = texColor;
}