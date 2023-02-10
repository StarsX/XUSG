//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSG.h"

using namespace std;
using namespace XUSG;

Sampler DescriptorTableLib::SamplerPointWrap()
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

Sampler DescriptorTableLib::SamplerPointMirror()
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

Sampler DescriptorTableLib::SamplerPointClamp()
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

Sampler DescriptorTableLib::SamplerPointBorder()
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

Sampler DescriptorTableLib::SamplerPointLessEqual()
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

Sampler DescriptorTableLib::SamplerLinearWrap()
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

Sampler DescriptorTableLib::SamplerLinearMirror()
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

Sampler DescriptorTableLib::SamplerLinearClamp()
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

Sampler DescriptorTableLib::SamplerLinearBorder()
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

Sampler DescriptorTableLib::SamplerLinearLessEqual()
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

Sampler DescriptorTableLib::SamplerAnisotropicWrap()
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

Sampler DescriptorTableLib::SamplerAnisotropicMirror()
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

Sampler DescriptorTableLib::SamplerAnisotropicClamp()
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

Sampler DescriptorTableLib::SamplerAnisotropicBorder()
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

Sampler DescriptorTableLib::SamplerAnisotropicLessEqual()
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

Sampler DescriptorTableLib::SamplerMinLinearWrap()
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

Sampler DescriptorTableLib::SamplerMinLinearMirror()
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

Sampler DescriptorTableLib::SamplerMinLinearClamp()
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

Sampler DescriptorTableLib::SamplerMinLinearBorder()
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

Sampler DescriptorTableLib::SamplerMaxLinearWrap()
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

Sampler DescriptorTableLib::SamplerMaxLinearMirror()
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

Sampler DescriptorTableLib::SamplerMaxLinearClamp()
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

Sampler DescriptorTableLib::SamplerMaxLinearBorder()
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

Sampler DescriptorTableLib::SamplerMinAnisotropicWrap()
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

Sampler DescriptorTableLib::SamplerMinAnisotropicMirror()
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

Sampler DescriptorTableLib::SamplerMinAnisotropicClamp()
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

Sampler DescriptorTableLib::SamplerMinAnisotropicBorder()
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

Sampler DescriptorTableLib::SamplerMaxAnisotropicWrap()
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

Sampler DescriptorTableLib::SamplerMaxAnisotropicMirror()
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

Sampler DescriptorTableLib::SamplerMaxAnisotropicClamp()
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

Sampler DescriptorTableLib::SamplerMaxAnisotropicBorder()
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
