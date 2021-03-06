//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGDescriptor_DX12.h"

using namespace std;
using namespace XUSG;

Sampler DescriptorTableCache_DX12::SamplerPointWrap()
{
	const auto sampler = make_shared<SamplerDesc>();
	sampler->Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	sampler->AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler->AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler->AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler->ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler->MinLOD = 0.0f;
	sampler->MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerPointMirror()
{
	const auto sampler = make_shared<SamplerDesc>();
	sampler->Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	sampler->AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	sampler->AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	sampler->AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	sampler->ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler->MinLOD = 0.0f;
	sampler->MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerPointClamp()
{
	const auto sampler = make_shared<SamplerDesc>();
	sampler->Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	sampler->AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler->AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler->AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler->ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler->MinLOD = 0.0f;
	sampler->MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerPointBorder()
{
	const auto sampler = make_shared<SamplerDesc>();
	sampler->Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	sampler->AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler->AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler->AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler->ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler->MinLOD = 0.0f;
	sampler->MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerPointLessEqual()
{
	const auto sampler = make_shared<SamplerDesc>();
	sampler->Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
	sampler->AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler->AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler->AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler->BorderColor[0] = 1.0f;
	sampler->BorderColor[1] = 1.0f;
	sampler->BorderColor[2] = 1.0f;
	sampler->BorderColor[3] = 1.0f;
	sampler->ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	sampler->MinLOD = 0.0f;
	sampler->MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerLinearWrap()
{
	const auto sampler = make_shared<SamplerDesc>();
	sampler->Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler->AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler->AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler->AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler->ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler->MinLOD = 0.0f;
	sampler->MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerLinearMirror()
{
	const auto sampler = make_shared<SamplerDesc>();
	sampler->Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler->AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	sampler->AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	sampler->AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	sampler->ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler->MinLOD = 0.0f;
	sampler->MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerLinearClamp()
{
	const auto sampler = make_shared<SamplerDesc>();
	sampler->Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler->AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler->AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler->AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler->ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler->MinLOD = 0.0f;
	sampler->MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerLinearBorder()
{
	const auto sampler = make_shared<SamplerDesc>();
	sampler->Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler->AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler->AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler->AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler->ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler->MinLOD = 0.0f;
	sampler->MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerLinearLessEqual()
{
	const auto sampler = make_shared<SamplerDesc>();
	sampler->Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	sampler->AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler->AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler->AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler->BorderColor[0] = 1.0f;
	sampler->BorderColor[1] = 1.0f;
	sampler->BorderColor[2] = 1.0f;
	sampler->BorderColor[3] = 1.0f;
	sampler->ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	sampler->MinLOD = 0.0f;
	sampler->MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerAnisotropicWrap()
{
	const auto sampler = make_shared<SamplerDesc>();
	sampler->Filter = D3D12_FILTER_ANISOTROPIC;
	sampler->AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler->AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler->AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler->MaxAnisotropy = 16;
	sampler->ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler->MinLOD = 0.0f;
	sampler->MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerAnisotropicMirror()
{
	const auto sampler = make_shared<SamplerDesc>();
	sampler->Filter = D3D12_FILTER_ANISOTROPIC;
	sampler->AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	sampler->AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	sampler->AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	sampler->MaxAnisotropy = 16;
	sampler->ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler->MinLOD = 0.0f;
	sampler->MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerAnisotropicClamp()
{
	const auto sampler = make_shared<SamplerDesc>();
	sampler->Filter = D3D12_FILTER_ANISOTROPIC;
	sampler->AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler->AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler->AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler->MaxAnisotropy = 16;
	sampler->ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler->MinLOD = 0.0f;
	sampler->MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerAnisotropicBorder()
{
	const auto sampler = make_shared<SamplerDesc>();
	sampler->Filter = D3D12_FILTER_ANISOTROPIC;
	sampler->AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler->AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler->AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler->MaxAnisotropy = 16;
	sampler->ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler->MinLOD = 0.0f;
	sampler->MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerAnisotropicLessEqual()
{
	const auto sampler = make_shared<SamplerDesc>();
	sampler->Filter = D3D12_FILTER_COMPARISON_ANISOTROPIC;
	sampler->AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler->AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler->AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler->BorderColor[0] = 1.0f;
	sampler->BorderColor[1] = 1.0f;
	sampler->BorderColor[2] = 1.0f;
	sampler->BorderColor[3] = 1.0f;
	sampler->MaxAnisotropy = 16;
	sampler->ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	sampler->MinLOD = 0.0f;
	sampler->MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}
