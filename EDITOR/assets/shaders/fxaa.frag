#version 450 core
// SPDX-License-Identifier: BSD-3-Clause
// FXAA Quality algorithm: Timothy Lottes, NVIDIA FXAA 3.11.
// Copyright (c) 2014, NVIDIA CORPORATION. All rights reserved.
// License: fxaa-license.txt, distributed alongside this source.
// Reference: https://github.com/GameTechDev/CMAA2/blob/master/Projects/CMAA2/FXAA/Fxaa3_11.h
// Adaptation: standalone GLSL 450, preset 12 search, RGB-derived luma at
// every sample (no RGBL/green-only/gather path), opaque display output.

layout(binding = 0) uniform sampler2D sceneColor;
uniform vec2 inverseInputSize;
in vec2 TexCoord;
out vec4 FragColor;

const float EDGE_THRESHOLD = 0.166;
const float EDGE_THRESHOLD_MIN = 0.0833;
const float SUBPIXEL_STRENGTH = 0.75;
const float SEARCH_STEPS[5] = float[](1.0, 1.5, 2.0, 4.0, 12.0);

// 颜色转亮度 L = 0.299*R + 0.587*G + 0.114*B
float luminance(vec3 color)
{
    return dot(color, vec3(0.299, 0.587, 0.114));
}

float sampleLuminance(vec2 uv)
{
    return luminance(textureLod(sceneColor, uv, 0.0).rgb);
}

void main()
{
    // 当前像素亮度
    vec3 centerColor = textureLod(sceneColor, TexCoord, 0.0).rgb;
    float center = luminance(centerColor);
    // 上下左右像素亮度
    vec2 pixel = inverseInputSize; // 分辨率倒数
    float north = sampleLuminance(TexCoord + vec2(0.0, pixel.y));
    float south = sampleLuminance(TexCoord - vec2(0.0, pixel.y));
    float east = sampleLuminance(TexCoord + vec2(pixel.x, 0.0));
    float west = sampleLuminance(TexCoord - vec2(pixel.x, 0.0));
    // 根据亮度范围判断是否需要抗锯齿
    float darkest = min(center, min(min(north, south), min(east, west)));
    float brightest = max(center, max(max(north, south), max(east, west)));
    float contrast = brightest - darkest;
    if (contrast < max(EDGE_THRESHOLD_MIN, brightest * EDGE_THRESHOLD)) {
        FragColor = vec4(centerColor, 1.0);
        return;
    }
    // 四个对角像素亮度
    float ne = sampleLuminance(TexCoord + pixel);
    float nw = sampleLuminance(TexCoord + vec2(-pixel.x, pixel.y));
    float se = sampleLuminance(TexCoord + vec2(pixel.x, -pixel.y));
    float sw = sampleLuminance(TexCoord - pixel);
    // 比较亮度在垂直方向和水平方向的变化, 决定边缘方向
    float vertical_delta = 2.0 * abs(north + south - 2.0 * center) + abs(ne + se - 2.0 * east) + abs(nw + sw - 2.0 * west);
    float horizontal_delta = 2.0 * abs(east + west - 2.0 * center) + abs(ne + nw - 2.0 * north) + abs(se + sw - 2.0 * south);
    bool horizontalEdge = vertical_delta >= horizontal_delta;

    // 用position表示水平边缘的上，垂直边缘的右
    float positiveNeighbor = horizontalEdge ? north : east;
    float negativeNeighbor = horizontalEdge ? south : west;
    // 边缘垂直两侧的亮度梯度
    float positiveGradient = abs(positiveNeighbor - center);
    float negativeGradient = abs(negativeNeighbor - center);
    // 边缘垂直两侧，哪边差更大
    bool positivePair = positiveGradient >= negativeGradient;
    // 当前像素与亮度梯度较大一侧的均值
    float pairMean = 0.5 * (center + (positivePair ? positiveNeighbor : negativeNeighbor));
    // endThreshold 则用于后续判断：沿边缘方向搜索时，采样亮度偏离这个平均值多少，才认为可能遇到了边缘端点。
    float endThreshold = 0.25 * max(positiveGradient, negativeGradient);
    // 沿着边缘法线亮的一侧偏移半个像素
    float signedPixel = (horizontalEdge ? pixel.y : pixel.x) * (positivePair ? 1.0 : -1.0);
    vec2 normalOffset = horizontalEdge ? vec2(0.0, signedPixel) : vec2(signedPixel, 0.0);
    vec2 edgeCenter = TexCoord + normalOffset * 0.5;
    // 沿着边缘的方向
    vec2 alongEdge = horizontalEdge ? vec2(pixel.x, 0.0) : vec2(0.0, pixel.y);

    // 沿着边缘两侧搜索
    float positiveDelta = 0.0, negativeDelta = 0.0;
    bool positiveDone = false, negativeDone = false;
    vec2 positiveEnd = edgeCenter + alongEdge * SEARCH_STEPS[0];
    vec2 negativeEnd = edgeCenter - alongEdge * SEARCH_STEPS[0];
    // 当搜索的像素与pairMean的差大于endThreshold时，说明到了边缘边界
    for (int step = 1; step < 5; ++step) {
        if (!negativeDone) {
            negativeDelta = sampleLuminance(negativeEnd) - pairMean;
            negativeDone = abs(negativeDelta) >= endThreshold;
        }
        if (!positiveDone) {
            positiveDelta = sampleLuminance(positiveEnd) - pairMean;
            positiveDone = abs(positiveDelta) >= endThreshold;
        }
        if (negativeDone && positiveDone) break;
        if (!negativeDone) negativeEnd -= alongEdge * SEARCH_STEPS[step];
        if (!positiveDone) positiveEnd += alongEdge * SEARCH_STEPS[step];
    }
    // 根据像素的与边缘两端的距离，推算出沿着法线的偏移比例
    float negativeDistance = horizontalEdge ? TexCoord.x - negativeEnd.x : TexCoord.y - negativeEnd.y;
    float positiveDistance = horizontalEdge ? positiveEnd.x - TexCoord.x : positiveEnd.y - TexCoord.y;
    float edgeBlend = 0.5 - min(negativeDistance, positiveDistance) / (negativeDistance + positiveDistance);

    // 判断最近端点的亮度与当前像素是否属于边缘的不同侧，以确定当前像素是否处于应该被平滑的台阶一侧。
    bool nearestNegative = negativeDistance < positiveDistance;
    float nearestDelta = nearestNegative ? negativeDelta : positiveDelta;
    bool validSpan = (nearestDelta < 0.0) != (center - pairMean < 0.0);
    if (!validSpan) edgeBlend = 0.0;

    // 亚像素抗锯齿
    float average = (2.0 * (north + south + east + west) + ne + nw + se + sw) / 12.0;
    float subpixel = clamp(abs(average - center) / contrast, 0.0, 1.0);
    subpixel = subpixel * subpixel * (3.0 - 2.0 * subpixel);
    subpixel = subpixel * subpixel * SUBPIXEL_STRENGTH;

    //FXAA 的核心不是直接把邻域颜色平均，而是找到合适的 UV 偏移，让纹理硬件完成颜色插值
    vec2 filteredUV = TexCoord + normalOffset * max(edgeBlend, subpixel);
    FragColor = vec4(textureLod(sceneColor, filteredUV, 0.0).rgb, 1.0);
}
