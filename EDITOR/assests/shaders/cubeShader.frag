#version 450 core
out vec4 color;
in vec2 TexCoord;
uniform sampler2D texture0;
void main()
{
	vec4 texColor = texture(texture0, TexCoord);
	color = texColor;
	//color = vec4(1.0f,0.0f,0.0f,1.0f);
}