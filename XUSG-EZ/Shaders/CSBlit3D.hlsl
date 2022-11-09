//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Textures
//--------------------------------------------------------------------------------------
Texture3D g_txSrc;
RWTexture3D<float4> g_rwDst;

//--------------------------------------------------------------------------------------
// Texture sampler
//--------------------------------------------------------------------------------------
SamplerState g_sampler;

[numthreads(4, 4, 4)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float3 texSize;
	g_rwDst.GetDimensions(texSize.x, texSize.y, texSize.z);

	const float3 uvw = (DTid + 0.5) / texSize;

	g_rwDst[DTid] = g_txSrc.SampleLevel(g_sampler, uvw, 0.0);
}
