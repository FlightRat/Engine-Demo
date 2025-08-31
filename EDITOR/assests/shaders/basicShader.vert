#version 450 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec2 aTexCoord;
out vec2 TexCoord;
uniform mat4 Model;
uniform mat4 Projection;
void main()
{
	gl_Position = Projection * Model * vec4(aPosition, 1.0);
	TexCoord = aTexCoord;
}