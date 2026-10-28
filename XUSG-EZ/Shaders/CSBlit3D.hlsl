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
Texture3D g_txSrc : register(t0);
XUSG_VK_BINDING(0, XUSG_VK_SPACE_UAV)
RWTexture3D<float4> g_rwDst : register(u0);

//--------------------------------------------------------------------------------------
// Texture sampler
//--------------------------------------------------------------------------------------
XUSG_VK_BINDING(0, XUSG_VK_SPACE_SAMPLER)
SamplerState g_sampler : register(s0);

[numthreads(4, 4, 4)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float3 texSize;
	g_rwDst.GetDimensions(texSize.x, texSize.y, texSize.z);

	const float3 uvw = (DTid + 0.5) / texSize;

	g_rwDst[DTid] = g_txSrc.SampleLevel(g_sampler, uvw, 0.0);
}
