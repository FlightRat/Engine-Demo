#version 330 core
layout(location=0) in vec3 position;
out vec2 missingVarying;
void main(){missingVarying=position.xy;gl_Position=vec4(position,1.0);}
