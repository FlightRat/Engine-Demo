#version 450 core

layout(binding = 0) uniform sampler2D shadowMap;
layout(location = 0) out uvec4 Moment;
layout(location = 1) out uint Coverage;

const float MOMENT_SCALE = 16777215.0; // 2^24 - 1  24为float的有效精度

uniform vec2 nearFar;

void main()
{
	Moment = uvec4(0u);
	Coverage = 0u;

	ivec2 pixel = ivec2(gl_FragCoord.xy);
	float depth = texelFetch(shadowMap, pixel, 0).r;
	if(depth >= 1.0) return;
	// 把[0~1]的非线性深度转为线性距离，再归一化到线性[0,1]深度
	float nearPlane = nearFar.x;
	float farPlane = nearFar.y;
	float z = nearPlane * farPlane / (farPlane - depth * (farPlane - nearPlane));
	float q = clamp((z - nearPlane) / (farPlane - nearPlane), 0.0, 1.0);
	// 深度前缀
	Moment = uvec4(
		uint(round(q*MOMENT_SCALE)), 0u,	// 一阶深度，
		uint(round(q*q*MOMENT_SCALE)), 0u	// 二阶深度
		);
	Coverage=1u;
}

