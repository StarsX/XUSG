//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

float3 GetCubeTexcoord(uint slice, float3 pos)
{
	switch (slice)
	{
	case 0:
		return float3(pos.z, pos.y, -pos.x);
	case 1:
		return float3(-pos.z, pos.y, pos.x);
	case 2:
		return float3(pos.x, pos.z, -pos.y);
	case 3:
		return float3(pos.x, -pos.z, pos.y);
	case 4:
		return float3(pos.x, pos.y, pos.z);
	case 5:
		return float3(-pos.x, pos.y, -pos.z);
	default:
		return pos;
	}
}

float3 GetCubeTexcoord(uint3 index, float3 texSize)
{
	const float2 radii = texSize.xy * 0.5;
	float2 xy = index.xy - radii + 0.5;
	xy.y = -xy.y;

	const float3 pos = { xy, radii.x };

	return GetCubeTexcoord(index.z, pos);
}

float3 GetCubeTexcoord(uint3 index, RWTexture2DArray<float3> cubeMap)
{
	float3 texSize;
	cubeMap.GetDimensions(texSize.x, texSize.y, texSize.z);

	return GetCubeTexcoord(index, texSize);
}

float3 GetCubeTexcoord(uint slice, float2 uv)
{
	float2 xy = uv * 2.0 - 1.0;
	xy.y = -xy.y;

	const float3 pos = { xy, 1.0 };

	return GetCubeTexcoord(slice, pos);
}
