//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#define PI 3.1415926535897

#include "XUSGAdvanced.h"

XUSG_VK_BINDING(0, XUSG_VK_SPACE_UAV)
RWStructuredBuffer<float4> g_rwSHResult : register(u0);
XUSG_VK_BINDING(0, XUSG_VK_SPACE_SRV)
StructuredBuffer<float4> g_roSHBuff : register(t0);
XUSG_VK_BINDING(1, XUSG_VK_SPACE_SRV)
StructuredBuffer<float> g_roWeight : register(t1);

[numthreads(32, 1, 1)]
void main(uint DTid : SV_DispatchThreadID)
{
	const float wt = g_roWeight[0];
	const float normProj = wt > 0.0 ? 4.0 * PI / wt : 0.0;

	g_rwSHResult[DTid] = float4(g_roSHBuff[DTid].xyz * normProj, 0.0);
}
