//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#ifdef XUSG_SPIRV
#include "XUSGAdvanced.h"
#endif

#ifndef XUSG_VK_BINDING
#define XUSG_VK_BINDING(b, s)
#endif

//--------------------------------------------------------------------------------------
// Textures
//--------------------------------------------------------------------------------------
XUSG_VK_BINDING(0, XUSG_VK_SPACE_SRV)
Texture2D g_txSrc : register(t0);
XUSG_VK_BINDING(0, XUSG_VK_SPACE_UAV)
RWTexture2D<float4> g_rwDst : register(u0);

//--------------------------------------------------------------------------------------
// Texture sampler
//--------------------------------------------------------------------------------------
XUSG_VK_BINDING(0, XUSG_VK_SPACE_SAMPLER)
SamplerState g_sampler : register(s0);

[numthreads(8, 8, 1)]
void main(uint2 DTid : SV_DispatchThreadID)
{
	float2 texSize;
	g_rwDst.GetDimensions(texSize.x, texSize.y);

	const float2 uv = (DTid + 0.5) / texSize;

	g_rwDst[DTid] = g_txSrc.SampleLevel(g_sampler, uv, 0.0);
}
