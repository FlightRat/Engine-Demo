#version 450 core
layout (triangles) in;
layout (triangle_strip, max_vertices=18) out;

uniform mat4 shadowMatrices[6];
uniform int shadowIndex;

out vec4 FragPos; // FragPos from GS (output per emitvertex)

void main()
{
    for(int face = 0; face < 6; ++face)
    {
        for(int i = 0; i < 3; ++i) // for each triangle's vertices
        {
            gl_Layer = shadowIndex * 6 + face; // 指定渲染目标层级.
            FragPos = gl_in[i].gl_Position;
            gl_Position = shadowMatrices[face] * FragPos; // 把物体世界坐标变换到6个面对应的光照空间
            EmitVertex();
        }    
        EndPrimitive();
    }
} 