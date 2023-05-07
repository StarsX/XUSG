//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSG_DX12.h"
#include "Core/XUSGEnum_DX12.h"
#include "Core/XUSGPipelineLayout_DX12.h"
#include "XUSGRayTracingPipelineLayout_DX12.h"

using namespace std;
using namespace XUSG;
using namespace XUSG::RayTracing;

RayTracing::PipelineLayout_DX12::PipelineLayout_DX12() :
	Util::PipelineLayout_DX12()
{
}

RayTracing::PipelineLayout_DX12::~PipelineLayout_DX12()
{
}

XUSG::PipelineLayout RayTracing::PipelineLayout_DX12::CreatePipelineLayout(const Device* pDevice,
	PipelineLayoutLib* pPipelineLayoutLib, PipelineLayoutFlag flags, const wchar_t* name)
{
	const auto pLayoutLib = dynamic_cast<PipelineLayoutLib_DX12*>(pPipelineLayoutLib);
	static auto highestVersion = pLayoutLib->GetRootSignatureHighestVersion();
	reinterpret_cast<uint16_t&>(m_pipelineLayoutKey[0]) = static_cast<uint16_t>(flags);
	const auto& key = m_pipelineLayoutKey;

	const auto numRootParams = reinterpret_cast<const uint16_t&>(key[Util::PipelineLayout_DX12::DescriptorTableLayoutCountOffset]);
	const auto pDescriptorTableLayouts = reinterpret_cast<const DescriptorTableLayout*>(&key[Util::PipelineLayout_DX12::DescriptorTableLayoutOffset]);

	vector<CD3DX12_ROOT_PARAMETER1> rootParams(numRootParams);
	vector<vector<CD3DX12_DESCRIPTOR_RANGE1>> descriptorRanges(numRootParams);
	for (auto i = 0u; i < numRootParams; ++i)
		pLayoutLib->GetRootParameter(rootParams[i], descriptorRanges[i], pDescriptorTableLayouts[i]);

	const auto staticSamplerKeyOffset = Util::PipelineLayout_DX12::DescriptorTableLayoutOffset + sizeof(DescriptorTableLayout) * numRootParams;
	const auto numSamplers = static_cast<uint32_t>((key.size() - staticSamplerKeyOffset) / sizeof(StaticSampler));
	const auto pStaticSamplers = reinterpret_cast<const StaticSampler*>(&key[staticSamplerKeyOffset]);

	vector<CD3DX12_STATIC_SAMPLER_DESC1> samplerDescs(numSamplers);
	for (auto i = 0u; i < numSamplers; ++i)
		pLayoutLib->GetStaticSampler(samplerDescs[i], pStaticSamplers[i]);

	vector<D3D12_ROOT_PARAMETER> rootParams0;
	vector<vector<D3D12_DESCRIPTOR_RANGE>> descriptorRanges0;
	vector<D3D12_STATIC_SAMPLER_DESC> samplerDescs0;
	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	if (highestVersion == D3D_ROOT_SIGNATURE_VERSION_1_2)
		rootSignatureDesc.Init_1_2(rootSignatureDesc, numRootParams, numRootParams ? rootParams.data() : nullptr,
			numSamplers, numSamplers ? samplerDescs.data() : nullptr, GetDX12RootSignatureFlags(flags));
	else
	{
		samplerDescs0.resize(numSamplers);
		for (auto i = 0u; i < numSamplers; ++i)
			samplerDescs0[i] = reinterpret_cast<const D3D12_STATIC_SAMPLER_DESC&>(samplerDescs[i]);

		if (highestVersion == D3D_ROOT_SIGNATURE_VERSION_1_1)
			rootSignatureDesc.Init_1_1(numRootParams, numRootParams ? rootParams.data() : nullptr,
				numSamplers, numSamplers ? samplerDescs0.data() : nullptr, GetDX12RootSignatureFlags(flags));
		else
		{
			rootParams0.resize(numRootParams);
			descriptorRanges0.resize(numRootParams);
			for (auto i = 0u; i < numRootParams; ++i)
				convertToRootParam1_0(rootParams0[i], descriptorRanges0[i], rootParams[i]);

			descriptorRanges.clear();
			rootParams.clear();

			rootSignatureDesc.Init_1_0(numRootParams, numRootParams ? rootParams0.data() : nullptr,
				numSamplers, numSamplers ? samplerDescs0.data() : nullptr, GetDX12RootSignatureFlags(flags));
		}
	}

	com_ptr<ID3D12RootSignature> rootSignature;
	com_ptr<ID3DBlob> signature, error;
	const auto pDxDevice = static_cast<ID3D12RaytracingFallbackDevice*>(pDevice->GetRTHandle());
	H_RETURN(pDxDevice->D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &signature, &error,
		AccelerationStructure::GetUAVCount()), cerr, reinterpret_cast<char*>(error->GetBufferPointer()), nullptr);

	V_RETURN(pDxDevice->CreateRootSignature(1, signature->GetBufferPointer(), signature->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature)), cerr, nullptr);
	if (name) rootSignature->SetName(name);
	pPipelineLayoutLib->SetPipelineLayout(key, rootSignature.get());

	return rootSignature.get();
}

XUSG::PipelineLayout RayTracing::PipelineLayout_DX12::GetPipelineLayout(const Device* pDevice,
	PipelineLayoutLib* pPipelineLayoutLib, PipelineLayoutFlag flags, const wchar_t* name)
{
	auto layout = pPipelineLayoutLib->GetPipelineLayout(this, flags, name, false);

	if (!layout) return CreatePipelineLayout(pDevice, pPipelineLayoutLib, flags, name);

	return layout;
}

void RayTracing::PipelineLayout_DX12::convertToRootParam1_0(D3D12_ROOT_PARAMETER& rootParam,
	vector<D3D12_DESCRIPTOR_RANGE>& descriptorRanges, const D3D12_ROOT_PARAMETER1& rootParam1)
{
	rootParam.ParameterType = rootParam1.ParameterType;

	switch (rootParam1.ParameterType)
	{
	case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
	{
		descriptorRanges.resize(rootParam1.DescriptorTable.NumDescriptorRanges);
		for (auto i = 0u; i < rootParam1.DescriptorTable.NumDescriptorRanges; ++i)
		{
			const auto& descriptorRange1 = rootParam1.DescriptorTable.pDescriptorRanges[i];
			auto& descriptorRange = descriptorRanges[i];

			descriptorRange.RangeType = descriptorRange1.RangeType;
			descriptorRange.NumDescriptors = descriptorRange1.NumDescriptors;
			descriptorRange.BaseShaderRegister = descriptorRange1.BaseShaderRegister;
			descriptorRange.RegisterSpace = descriptorRange1.RegisterSpace;
			descriptorRange.OffsetInDescriptorsFromTableStart = descriptorRange1.OffsetInDescriptorsFromTableStart;
		}

		rootParam.DescriptorTable.NumDescriptorRanges = rootParam1.DescriptorTable.NumDescriptorRanges;
		rootParam.DescriptorTable.pDescriptorRanges = descriptorRanges.data();

		break;
	}
	case D3D12_ROOT_PARAMETER_TYPE_CBV:
	case D3D12_ROOT_PARAMETER_TYPE_SRV:
	case D3D12_ROOT_PARAMETER_TYPE_UAV:
		rootParam.Descriptor.ShaderRegister = rootParam1.Descriptor.ShaderRegister;
		rootParam.Descriptor.RegisterSpace = rootParam1.Descriptor.RegisterSpace;
		break;
	default:
		assert(rootParam1.ParameterType == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS);
		rootParam.Constants = rootParam1.Constants;
	}
}
