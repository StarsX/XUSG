//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGAdvanced.h"
#include "SHMath.hlsli"
#include "CubeMap.hlsli"
#include "CSWaveOP.hlsli"

//--------------------------------------------------------------------------------------
// Constant buffer
//--------------------------------------------------------------------------------------
XUSG_VK_BINDING(0, XUSG_VK_SPACE_CBV)
cbuffer cb : register(b0)
{
	uint g_order;
	uint g_mapSize;
};

//--------------------------------------------------------------------------------------
// Buffers and texture
//--------------------------------------------------------------------------------------
XUSG_VK_BINDING(0, XUSG_VK_SPACE_UAV)
RWStructuredBuffer<float4> g_rwSHBuff : register(u0);
XUSG_VK_BINDING(1, XUSG_VK_SPACE_UAV)
RWStructuredBuffer<float> g_rwWeight : register(u1);
XUSG_VK_BINDING(0, XUSG_VK_SPACE_SRV)
TextureCube<float3> g_txCubeMap : register(t0);

//--------------------------------------------------------------------------------------
// Texture sampler
//--------------------------------------------------------------------------------------
XUSG_VK_BINDING(0, XUSG_VK_SPACE_SAMPLER)
SamplerState g_sampler : register(s0);

//--------------------------------------------------------------------------------------
// Compute shader that performs spherical-harmonics transform from a cube map
//--------------------------------------------------------------------------------------
[numthreads(SH_GROUP_SIZE, 1, 1)]
void main(uint DTid : SV_DispatchThreadID, uint GTid : SV_GroupThreadID, uint Gid : SV_GroupID)
{
	uint3 idx;
	const uint sliceSize = g_mapSize * g_mapSize;
	const uint xy = DTid % sliceSize;
	idx.x = xy % g_mapSize;
	idx.y = xy / g_mapSize;
	idx.z = DTid / sliceSize;

	const float size = g_mapSize;
	float3 dir = GetCubeTexcoord(idx, float3(size.xx, 6));

	const float3 color = g_txCubeMap.SampleLevel(g_sampler, dir, 0.0);
	dir = normalize(dir);

	// index from [0, w - 1], f(0) maps to -1 + 1/w, f(w - 1) maps to 1 - 1/w
	// linear function x * s + b, 1st constraint means B is (-1 + 1/w), plug into
	// second and solve for s: s = 2 * (1 - 1/w) / (w - 1). The old code that did 
	// this was incorrect - but only for computing the differential solid
	// angle, where the final value was 1.0 instead of 1 - 1/w...
	const float b = 1.0 / size - 1.0;
	const float s = g_mapSize > 1 ? 2.0 * (1.0 - 1.0 / size) / (size - 1.0) : 0.0;
	const float2 uv = idx.xy * s + b;
	const float diff = 1.0 + dot(uv, uv);
	const float diffSolid = 4.0 / (diff * sqrt(diff));
	float wt = WaveLanesSum(GTid, diffSolid);
#if SH_GROUP_SIZE > SH_WAVE_SIZE
	if (GTid % WaveGetLaneCount() == 0) g_smem[GTid / WaveGetLaneCount()].w = wt;

	GroupMemoryBarrierWithGroupSync();

	if (GTid < WaveGetLaneCount())
	{
		wt = g_smem[GTid].w;
		wt = WaveLanesSum(GTid, wt);
	}
#endif
	if (GTid == 0) g_rwWeight[Gid] = wt;

	float shBuff[SH_MAX_COEFF];
	float3 shBuffB[SH_MAX_COEFF];
	SHEvalDirection(shBuff, g_order, dir);

	const uint n = g_order * g_order;
	SHScale(shBuffB, g_order, shBuff, color * diffSolid);

	for (uint i = 0; i < n; ++i)
	{
		float3 sh = WaveLanesSum(GTid, shBuffB[i]);
#if SH_GROUP_SIZE > SH_WAVE_SIZE
		if (GTid % WaveGetLaneCount() == 0) g_smem[GTid / WaveGetLaneCount()].xyz = sh;
			
		GroupMemoryBarrierWithGroupSync();

		if (GTid < WaveGetLaneCount())
		{
			sh = g_smem[GTid].xyz;
			sh = WaveLanesSum(GTid, sh);
		}
#endif
		if (GTid == 0) g_rwSHBuff[GetLocation(n, uint2(Gid, i))] = float4(sh, 0.0);
	}
}
