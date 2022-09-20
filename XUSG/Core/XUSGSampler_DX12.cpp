//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGDescriptor_DX12.h"

using namespace std;
using namespace XUSG;

Sampler DescriptorTableCache_DX12::SamplerPointWrap()
{
	Sampler sampler = {};
	sampler.Filter = SamplerFilter::MIN_MAG_MIP_POINT;
	sampler.AddressU = TextureAddressMode::WRAP;
	sampler.AddressV = TextureAddressMode::WRAP;
	sampler.AddressW = TextureAddressMode::WRAP;
	sampler.Comparison = ComparisonFunc::NEVER;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerPointMirror()
{
	Sampler sampler = {};
	sampler.Filter = SamplerFilter::MIN_MAG_MIP_POINT;
	sampler.AddressU = TextureAddressMode::MIRROR;
	sampler.AddressV = TextureAddressMode::MIRROR;
	sampler.AddressW = TextureAddressMode::MIRROR;
	sampler.Comparison = ComparisonFunc::NEVER;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerPointClamp()
{
	Sampler sampler = {};
	sampler.Filter = SamplerFilter::MIN_MAG_MIP_POINT;
	sampler.AddressU = TextureAddressMode::CLAMP;
	sampler.AddressV = TextureAddressMode::CLAMP;
	sampler.AddressW = TextureAddressMode::CLAMP;
	sampler.Comparison = ComparisonFunc::NEVER;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerPointBorder()
{
	Sampler sampler = {};
	sampler.Filter = SamplerFilter::MIN_MAG_MIP_POINT;
	sampler.AddressU = TextureAddressMode::BORDER;
	sampler.AddressV = TextureAddressMode::BORDER;
	sampler.AddressW = TextureAddressMode::BORDER;
	sampler.Comparison = ComparisonFunc::NEVER;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerPointLessEqual()
{
	Sampler sampler = {};
	sampler.Filter = SamplerFilter::COMPARISON_MIN_MAG_MIP_POINT;
	sampler.AddressU = TextureAddressMode::BORDER;
	sampler.AddressV = TextureAddressMode::BORDER;
	sampler.AddressW = TextureAddressMode::BORDER;
	sampler.BorderColor[0] = 1.0f;
	sampler.BorderColor[1] = 1.0f;
	sampler.BorderColor[2] = 1.0f;
	sampler.BorderColor[3] = 1.0f;
	sampler.Comparison = ComparisonFunc::LESS_EQUAL;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerLinearWrap()
{
	Sampler sampler = {};
	sampler.Filter = SamplerFilter::MIN_MAG_MIP_LINEAR;
	sampler.AddressU = TextureAddressMode::WRAP;
	sampler.AddressV = TextureAddressMode::WRAP;
	sampler.AddressW = TextureAddressMode::WRAP;
	sampler.Comparison = ComparisonFunc::NEVER;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerLinearMirror()
{
	Sampler sampler = {};
	sampler.Filter = SamplerFilter::MIN_MAG_MIP_LINEAR;
	sampler.AddressU = TextureAddressMode::MIRROR;
	sampler.AddressV = TextureAddressMode::MIRROR;
	sampler.AddressW = TextureAddressMode::MIRROR;
	sampler.Comparison = ComparisonFunc::NEVER;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerLinearClamp()
{
	Sampler sampler = {};
	sampler.Filter = SamplerFilter::MIN_MAG_MIP_LINEAR;
	sampler.AddressU = TextureAddressMode::CLAMP;
	sampler.AddressV = TextureAddressMode::CLAMP;
	sampler.AddressW = TextureAddressMode::CLAMP;
	sampler.Comparison = ComparisonFunc::NEVER;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerLinearBorder()
{
	Sampler sampler = {};
	sampler.Filter = SamplerFilter::MIN_MAG_MIP_LINEAR;
	sampler.AddressU = TextureAddressMode::BORDER;
	sampler.AddressV = TextureAddressMode::BORDER;
	sampler.AddressW = TextureAddressMode::BORDER;
	sampler.Comparison = ComparisonFunc::NEVER;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerLinearLessEqual()
{
	Sampler sampler = {};
	sampler.Filter = SamplerFilter::COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	sampler.AddressU = TextureAddressMode::BORDER;
	sampler.AddressV = TextureAddressMode::BORDER;
	sampler.AddressW = TextureAddressMode::BORDER;
	sampler.BorderColor[0] = 1.0f;
	sampler.BorderColor[1] = 1.0f;
	sampler.BorderColor[2] = 1.0f;
	sampler.BorderColor[3] = 1.0f;
	sampler.Comparison = ComparisonFunc::LESS_EQUAL;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerAnisotropicWrap()
{
	Sampler sampler = {};
	sampler.Filter = SamplerFilter::ANISOTROPIC;
	sampler.AddressU = TextureAddressMode::WRAP;
	sampler.AddressV = TextureAddressMode::WRAP;
	sampler.AddressW = TextureAddressMode::WRAP;
	sampler.MaxAnisotropy = 16;
	sampler.Comparison = ComparisonFunc::NEVER;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerAnisotropicMirror()
{
	Sampler sampler = {};
	sampler.Filter = SamplerFilter::ANISOTROPIC;
	sampler.AddressU = TextureAddressMode::MIRROR;
	sampler.AddressV = TextureAddressMode::MIRROR;
	sampler.AddressW = TextureAddressMode::MIRROR;
	sampler.MaxAnisotropy = 16;
	sampler.Comparison = ComparisonFunc::NEVER;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerAnisotropicClamp()
{
	Sampler sampler = {};
	sampler.Filter = SamplerFilter::ANISOTROPIC;
	sampler.AddressU = TextureAddressMode::CLAMP;
	sampler.AddressV = TextureAddressMode::CLAMP;
	sampler.AddressW = TextureAddressMode::CLAMP;
	sampler.MaxAnisotropy = 16;
	sampler.Comparison = ComparisonFunc::NEVER;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerAnisotropicBorder()
{
	Sampler sampler = {};
	sampler.Filter = SamplerFilter::ANISOTROPIC;
	sampler.AddressU = TextureAddressMode::BORDER;
	sampler.AddressV = TextureAddressMode::BORDER;
	sampler.AddressW = TextureAddressMode::BORDER;
	sampler.MaxAnisotropy = 16;
	sampler.Comparison = ComparisonFunc::NEVER;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerAnisotropicLessEqual()
{
	Sampler sampler = {};
	sampler.Filter = SamplerFilter::COMPARISON_ANISOTROPIC;
	sampler.AddressU = TextureAddressMode::BORDER;
	sampler.AddressV = TextureAddressMode::BORDER;
	sampler.AddressW = TextureAddressMode::BORDER;
	sampler.BorderColor[0] = 1.0f;
	sampler.BorderColor[1] = 1.0f;
	sampler.BorderColor[2] = 1.0f;
	sampler.BorderColor[3] = 1.0f;
	sampler.MaxAnisotropy = 16;
	sampler.Comparison = ComparisonFunc::LESS_EQUAL;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerMinLinearWrap()
{
	Sampler sampler = {};
	sampler.Filter = SamplerFilter::MINIMUM_MIN_MAG_MIP_LINEAR;
	sampler.AddressU = TextureAddressMode::WRAP;
	sampler.AddressV = TextureAddressMode::WRAP;
	sampler.AddressW = TextureAddressMode::WRAP;
	sampler.Comparison = ComparisonFunc::NEVER;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerMinLinearMirror()
{
	Sampler sampler = {};
	sampler.Filter = SamplerFilter::MINIMUM_MIN_MAG_MIP_LINEAR;
	sampler.AddressU = TextureAddressMode::MIRROR;
	sampler.AddressV = TextureAddressMode::MIRROR;
	sampler.AddressW = TextureAddressMode::MIRROR;
	sampler.Comparison = ComparisonFunc::NEVER;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerMinLinearClamp()
{
	Sampler sampler = {};
	sampler.Filter = SamplerFilter::MINIMUM_MIN_MAG_MIP_LINEAR;
	sampler.AddressU = TextureAddressMode::CLAMP;
	sampler.AddressV = TextureAddressMode::CLAMP;
	sampler.AddressW = TextureAddressMode::CLAMP;
	sampler.Comparison = ComparisonFunc::NEVER;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerMinLinearBorder()
{
	Sampler sampler = {};
	sampler.Filter = SamplerFilter::MINIMUM_MIN_MAG_MIP_LINEAR;
	sampler.AddressU = TextureAddressMode::BORDER;
	sampler.AddressV = TextureAddressMode::BORDER;
	sampler.AddressW = TextureAddressMode::BORDER;
	sampler.Comparison = ComparisonFunc::NEVER;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerMaxLinearWrap()
{
	Sampler sampler = {};
	sampler.Filter = SamplerFilter::MAXIMUM_MIN_MAG_MIP_LINEAR;
	sampler.AddressU = TextureAddressMode::WRAP;
	sampler.AddressV = TextureAddressMode::WRAP;
	sampler.AddressW = TextureAddressMode::WRAP;
	sampler.Comparison = ComparisonFunc::NEVER;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerMaxLinearMirror()
{
	Sampler sampler = {};
	sampler.Filter = SamplerFilter::MAXIMUM_MIN_MAG_MIP_LINEAR;
	sampler.AddressU = TextureAddressMode::MIRROR;
	sampler.AddressV = TextureAddressMode::MIRROR;
	sampler.AddressW = TextureAddressMode::MIRROR;
	sampler.Comparison = ComparisonFunc::NEVER;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerMaxLinearClamp()
{
	Sampler sampler = {};
	sampler.Filter = SamplerFilter::MAXIMUM_MIN_MAG_MIP_LINEAR;
	sampler.AddressU = TextureAddressMode::CLAMP;
	sampler.AddressV = TextureAddressMode::CLAMP;
	sampler.AddressW = TextureAddressMode::CLAMP;
	sampler.Comparison = ComparisonFunc::NEVER;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerMaxLinearBorder()
{
	Sampler sampler = {};
	sampler.Filter = SamplerFilter::MAXIMUM_MIN_MAG_MIP_LINEAR;
	sampler.AddressU = TextureAddressMode::BORDER;
	sampler.AddressV = TextureAddressMode::BORDER;
	sampler.AddressW = TextureAddressMode::BORDER;
	sampler.Comparison = ComparisonFunc::NEVER;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerMinAnisotropicWrap()
{
	Sampler sampler = {};
	sampler.Filter = SamplerFilter::MINIMUM_ANISOTROPIC;
	sampler.AddressU = TextureAddressMode::WRAP;
	sampler.AddressV = TextureAddressMode::WRAP;
	sampler.AddressW = TextureAddressMode::WRAP;
	sampler.MaxAnisotropy = 16;
	sampler.Comparison = ComparisonFunc::NEVER;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerMinAnisotropicMirror()
{
	Sampler sampler = {};
	sampler.Filter = SamplerFilter::MINIMUM_ANISOTROPIC;
	sampler.AddressU = TextureAddressMode::MIRROR;
	sampler.AddressV = TextureAddressMode::MIRROR;
	sampler.AddressW = TextureAddressMode::MIRROR;
	sampler.MaxAnisotropy = 16;
	sampler.Comparison = ComparisonFunc::NEVER;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerMinAnisotropicClamp()
{
	Sampler sampler = {};
	sampler.Filter = SamplerFilter::MINIMUM_ANISOTROPIC;
	sampler.AddressU = TextureAddressMode::CLAMP;
	sampler.AddressV = TextureAddressMode::CLAMP;
	sampler.AddressW = TextureAddressMode::CLAMP;
	sampler.MaxAnisotropy = 16;
	sampler.Comparison = ComparisonFunc::NEVER;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerMinAnisotropicBorder()
{
	Sampler sampler = {};
	sampler.Filter = SamplerFilter::MINIMUM_ANISOTROPIC;
	sampler.AddressU = TextureAddressMode::BORDER;
	sampler.AddressV = TextureAddressMode::BORDER;
	sampler.AddressW = TextureAddressMode::BORDER;
	sampler.MaxAnisotropy = 16;
	sampler.Comparison = ComparisonFunc::NEVER;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerMaxAnisotropicWrap()
{
	Sampler sampler = {};
	sampler.Filter = SamplerFilter::MAXIMUM_ANISOTROPIC;
	sampler.AddressU = TextureAddressMode::WRAP;
	sampler.AddressV = TextureAddressMode::WRAP;
	sampler.AddressW = TextureAddressMode::WRAP;
	sampler.MaxAnisotropy = 16;
	sampler.Comparison = ComparisonFunc::NEVER;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerMaxAnisotropicMirror()
{
	Sampler sampler = {};
	sampler.Filter = SamplerFilter::MAXIMUM_ANISOTROPIC;
	sampler.AddressU = TextureAddressMode::MIRROR;
	sampler.AddressV = TextureAddressMode::MIRROR;
	sampler.AddressW = TextureAddressMode::MIRROR;
	sampler.MaxAnisotropy = 16;
	sampler.Comparison = ComparisonFunc::NEVER;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerMaxAnisotropicClamp()
{
	Sampler sampler = {};
	sampler.Filter = SamplerFilter::MAXIMUM_ANISOTROPIC;
	sampler.AddressU = TextureAddressMode::CLAMP;
	sampler.AddressV = TextureAddressMode::CLAMP;
	sampler.AddressW = TextureAddressMode::CLAMP;
	sampler.MaxAnisotropy = 16;
	sampler.Comparison = ComparisonFunc::NEVER;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}

Sampler DescriptorTableCache_DX12::SamplerMaxAnisotropicBorder()
{
	Sampler sampler = {};
	sampler.Filter = SamplerFilter::MAXIMUM_ANISOTROPIC;
	sampler.AddressU = TextureAddressMode::BORDER;
	sampler.AddressV = TextureAddressMode::BORDER;
	sampler.AddressW = TextureAddressMode::BORDER;
	sampler.MaxAnisotropy = 16;
	sampler.Comparison = ComparisonFunc::NEVER;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;

	return sampler;
}
