//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Group-shared memory
//--------------------------------------------------------------------------------------
#if !defined(__SHADER_TARGET_MAJOR) || __SHADER_TARGET_MAJOR <= 5 || SH_GROUP_SIZE > SH_WAVE_SIZE
groupshared float4 g_smem[SH_WAVE_SIZE];
#endif

#if defined(__SHADER_TARGET_MAJOR) && __SHADER_TARGET_MAJOR > 5
//--------------------------------------------------------------------------------------
// Native WaveActiveSum for SM6+
//--------------------------------------------------------------------------------------
#define WaveLanesSumBroadCast WaveLanesSum

float WaveLanesSum(uint laneId, float expr)
{
	return WaveActiveSum(expr);
}

float2 WaveLanesSum(uint laneId, float2 expr)
{
	return WaveActiveSum(expr);
}

float3 WaveLanesSum(uint laneId, float3 expr)
{
	return WaveActiveSum(expr);
}

float4 WaveLanesSum(uint laneId, float4 expr)
{
	return WaveActiveSum(expr);
}
#else
//--------------------------------------------------------------------------------------
// WaveActiveSum emulation for SM6-
//--------------------------------------------------------------------------------------
// No broad-cast, only the first lane needs the result
#define T	float
#define SWZ	x
#include "WaveOpTypeless.hlsli"
#undef SWZ
#undef T

#define T	float2
#define SWZ	xy
#include "WaveOpTypeless.hlsli"
#undef SWZ
#undef T

#define T	float3
#define SWZ	xyz
#include "WaveOpTypeless.hlsli"
#undef SWZ
#undef T

#define T	float4
#define SWZ	xyzw
#include "WaveOpTypeless.hlsli"
#undef SWZ
#undef T

#undef _BROAD_CAST_

// Broad-cast, all lanes need the result
#define _BROAD_CAST_ 1

#define T	float
#define SWZ	x
#include "WaveOpTypeless.hlsli"
#undef SWZ
#undef T

#define T	float2
#define SWZ	xy
#include "WaveOpTypeless.hlsli"
#undef SWZ
#undef T

#define T	float3
#define SWZ	xyz
#include "WaveOpTypeless.hlsli"
#undef SWZ
#undef T

#define T	float4
#define SWZ	xyzw
#include "WaveOpTypeless.hlsli"
#undef SWZ
#undef T

#undef _BROAD_CAST_

uint WaveGetLaneCount()
{
	return SH_WAVE_SIZE;
}
#endif
