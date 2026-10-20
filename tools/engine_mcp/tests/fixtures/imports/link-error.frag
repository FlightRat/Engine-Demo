#version 330 core
in vec3 missingVarying;
out vec4 color;
void main(){color=vec4(missingVarying,1.0);}
