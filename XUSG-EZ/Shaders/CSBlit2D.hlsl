//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Textures
//--------------------------------------------------------------------------------------
Texture2D g_txSrc;
RWTexture2D<float4> g_rwDst;

//--------------------------------------------------------------------------------------
// Texture sampler
//--------------------------------------------------------------------------------------
SamplerState g_sampler;

[numthreads(8, 8, 1)]
void main(uint2 DTid : SV_DispatchThreadID)
{
	float2 texSize;
	g_rwDst.GetDimensions(texSize.x, texSize.y);

	const float2 uv = (DTid + 0.5) / texSize;

	g_rwDst[DTid] = g_txSrc.SampleLevel(g_sampler, uv, 0.0);
}
