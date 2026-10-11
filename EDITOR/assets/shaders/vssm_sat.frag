#version 450 core

layout(binding = 0) uniform usampler2D momentSums;
layout(binding = 1) uniform usampler2D coverage;

layout(location = 0) out uvec4 MomentSums;
layout(location = 1) out uint Coverage;

uniform int axis;		// 累加轴：0横向，1纵向
uniform int stride;		// 累加步长：1、2、4、8……

// 使用两个 uint 表示一个 64 位无符号整数，x：低 32 位，y：高 32 位。
uvec2 AddMomentSum(uvec2 a, uvec2 b)
{
	uint low = a.x + b.x;
	uint carry = uint(low < a.x);
	return uvec2(low, a.y+b.y+carry);
}

void main()
{
	ivec2 pixel = ivec2(gl_FragCoord.xy);
	MomentSums = texelFetch(momentSums, pixel, 0);
	Coverage = texelFetch(coverage, pixel, 0).r;
	ivec2 offset = axis==0 ? ivec2(stride,0) : ivec2(0,stride);
	ivec2 previous = pixel - offset;
	if(any(lessThan(previous, ivec2(0)))) return;
	uvec4 value = texelFetch(momentSums, previous, 0);
	MomentSums = uvec4(
		AddMomentSum(MomentSums.xy, value.xy),
		AddMomentSum(MomentSums.zw, value.zw)
		);
	Coverage += texelFetch(coverage, previous, 0).r;
}