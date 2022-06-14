//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#define PI 3.1415926535897

RWStructuredBuffer<float3> g_rwSHResult;
StructuredBuffer<float3> g_roSHBuff;
StructuredBuffer<float> g_roWeight;

[numthreads(32, 1, 1)]
void main(uint DTid : SV_DispatchThreadID)
{
	const float wt = g_roWeight[0];
	const float normProj = wt > 0.0 ? 4.0 * PI / wt : 0.0;

	g_rwSHResult[DTid] = g_roSHBuff[DTid] * normProj;
}
