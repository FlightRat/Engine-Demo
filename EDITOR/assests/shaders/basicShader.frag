#version 450 core
out vec4 color;
in vec2 TexCoord;
//uniform sampler2D texture0;
void main()
{
	//vec2 flip_coord = vec2(TexCoord.x, 1.0-TexCoord.y);
	//vec4 texColor = texture(texture0, flip_coord);
	//color = texColor;
	color = vec4(1.0f,0.0f,0.0f,1.0f);
}