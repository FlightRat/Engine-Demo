#version 450 core
out vec4 color;
in vec2 TexCoord;
in vec4 FragColor;
uniform sampler2D texture0;
void main()
{
	vec2 flip_coord = vec2(TexCoord.x, 1.0 - TexCoord.y);
	vec4 texColor = texture(texture0, flip_coord);
	color = texColor * FragColor;
}