//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef T
#define T StructuredBuffer<float3>
#endif

#ifndef SH_COEFFS
#define SH_COEFFS shCoeffs
#endif

float4 EvaluateSHIrradiance(T SH_COEFFS, float3 norm)
{
	const float c1 = 0.42904276540489171563379376569857;	// 4 * A2.Y22 = 1/16 * sqrt(15.PI)
	const float c2 = 0.51166335397324424423977581244463;	// 0.5 * A1.Y10 = 1/2 * sqrt(PI/3)
	const float c3 = 0.24770795610037568833406429782001;	// A2.Y20 = 1/16 * sqrt(5.PI)
	const float c4 = 0.88622692545275801364908374167057;	// A0.Y00 = 1/2 * sqrt(PI)

	const float x = -norm.x;
	const float y = -norm.y;
	const float z = norm.z;

	const float3 irradiance = max(0.0,
		(c1 * (x * x - y * y)) * shCoeffs[8]											// c1.L22.(x^2 - y^2)
		+ (c3 * (3.0 * z * z - 1.0)) * shCoeffs[6]										// c3.L20.(3.z^2 - 1)
		+ c4 * shCoeffs[0]																// c4.L00 
		+ 2.0 * c1 * (shCoeffs[4] * x * y + shCoeffs[7] * x * z + shCoeffs[5] * y * z)	// 2.c1.(L2-2.xy + L21.xz + L2-1.yz)
		+ 2.0 * c2 * (shCoeffs[3] * x + shCoeffs[1] * y + shCoeffs[2] * z));			// 2.c2.(L11.x + L1-1.y + L10.z)

	const float avgLum = dot(shCoeffs[0], float3(0.25, 0.5, 0.25));

	return float4(irradiance, avgLum);
}
