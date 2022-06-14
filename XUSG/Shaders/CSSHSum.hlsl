//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "SHMath.hlsli"
#include "CSWaveOP.hlsli"

//--------------------------------------------------------------------------------------
// Constant buffer
//--------------------------------------------------------------------------------------
cbuffer cb
{
	uint g_order;
	uint g_pixelCount;
};

//--------------------------------------------------------------------------------------
// Buffers
//--------------------------------------------------------------------------------------
RWStructuredBuffer<float3> g_rwSHBuff;
RWStructuredBuffer<float> g_rwWeight;
StructuredBuffer<float3> g_roSHBuff;
StructuredBuffer<float> g_roWeight;

//--------------------------------------------------------------------------------------
// Compute shader that performs group reduction of sum
//--------------------------------------------------------------------------------------
[numthreads(SH_GROUP_SIZE, 1, 1)]
void main(uint2 DTid : SV_DispatchThreadID, uint GTid : SV_GroupThreadID, uint2 Gid : SV_GroupID)
{
	const uint n = g_order * g_order;
	float4 sh = 0.0;

	if (DTid.x < g_pixelCount)
	{
		sh.xyz = g_roSHBuff[GetLocation(n, DTid)];
		if (Gid.y == 0) sh.w = g_roWeight[DTid.x];
	}

	sh = WaveLanesSum(GTid, sh);

#if SH_GROUP_SIZE > SH_WAVE_SIZE
	if (GTid % WaveGetLaneCount() == 0) g_smem[GTid / WaveGetLaneCount()] = sh;

	GroupMemoryBarrierWithGroupSync();

	if (GTid < WaveGetLaneCount())
	{
		sh = g_smem[GTid];
		sh = WaveLanesSum(GTid, sh);
	}
#endif

	if (GTid == 0)
	{
		g_rwSHBuff[GetLocation(n, Gid)] = sh.xyz;
		if (Gid.y == 0) g_rwWeight[Gid.x] = sh.w;
	}
}
