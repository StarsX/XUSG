//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef PI
#define PI 3.1415926535897
#endif
#define SH_NUM_COEFF (SH_ORDER * SH_ORDER)

//--------------------------------------------------------------------------------------
// Load spherical harmonics
//--------------------------------------------------------------------------------------
void LoadSH(out float3 shCoeffs[SH_NUM_COEFF], StructuredBuffer<float3> roSHCoeffs)
{
	[unroll]
	for (uint i = 0; i < SH_NUM_COEFF; ++i) shCoeffs[i] = roSHCoeffs[i];
}

//--------------------------------------------------------------------------------------
// Evaluate irradiance using spherical harmonics
//--------------------------------------------------------------------------------------
#include "SHIrradianceTypeless.hlsli"
#undef SH_COEFFS
#undef T

#define T			float3
#define SH_COEFFS	shCoeffs[SH_NUM_COEFF]
#include "SHIrradianceTypeless.hlsli"
#undef SH_COEFFS
#undef T
