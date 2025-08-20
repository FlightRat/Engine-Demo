#version 450 core
out vec4 color;
in vec2 TexCoord;
uniform sampler2D texture0;
void main()
{
	//color = vec4(1.0f,1.0f,1.0f,1.0f);
	vec2 flip_coord = vec2(TexCoord.x, 1.0 - TexCoord.y);
	color = texture(texture0, flip_coord);
}