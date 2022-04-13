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
	PipelineLayoutCache* pPipelineLayoutCache, PipelineLayoutFlag flags, const wchar_t* name)
{
	const auto pLayoutCache = dynamic_cast<PipelineLayoutCache_DX12*>(pPipelineLayoutCache);
	const auto highestVersion = pLayoutCache->GetRootSignatureHighestVersion();
	reinterpret_cast<uint16_t&>(m_pipelineLayoutKey[0]) = static_cast<uint16_t>(flags);
	const auto& key = m_pipelineLayoutKey;

	const auto numRootParams = reinterpret_cast<const uint16_t&>(key[Util::PipelineLayout_DX12::DescriptorTableLayoutCountOffset]);
	const auto pDescriptorTableLayouts = reinterpret_cast<const DescriptorTableLayout*>(&key[Util::PipelineLayout_DX12::DescriptorTableLayoutOffset]);

	vector<CD3DX12_ROOT_PARAMETER1> rootParams(numRootParams);
	vector<vector<CD3DX12_DESCRIPTOR_RANGE1>> descriptorRanges(numRootParams);
	for (auto i = 0u; i < numRootParams; ++i)
		pLayoutCache->GetRootParameter(rootParams[i], descriptorRanges[i], pDescriptorTableLayouts[i]);

	const auto staticSamplerKeyOffset = Util::PipelineLayout_DX12::DescriptorTableLayoutOffset + sizeof(DescriptorTableLayout) * numRootParams;
	const auto numSamplers = static_cast<uint32_t>((key.size() - staticSamplerKeyOffset) / sizeof(StaticSampler));
	const auto pStaticSamplers = reinterpret_cast<const StaticSampler*>(&key[staticSamplerKeyOffset]);

	vector<CD3DX12_STATIC_SAMPLER_DESC> samplerDescs(numSamplers);
	for (auto i = 0u; i < numSamplers; ++i)
		pLayoutCache->GetStaticSampler(samplerDescs[i], pStaticSamplers[i]);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init_1_1(numRootParams, numRootParams ? rootParams.data() : nullptr,
		numSamplers, numSamplers ? samplerDescs.data() : nullptr, GetDX12RootSignatureFlags(flags));

	com_ptr<ID3D12RootSignature> rootSignature;
	com_ptr<ID3DBlob> signature, error;
	rootSignatureDesc.Version = highestVersion;
	const auto pDxDevice = static_cast<ID3D12RaytracingFallbackDevice*>(pDevice->GetRTHandle());
	H_RETURN(pDxDevice->D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &signature, &error,
		AccelerationStructure::GetUAVCount()), cerr, reinterpret_cast<wchar_t*>(error->GetBufferPointer()), nullptr);

	V_RETURN(pDxDevice->CreateRootSignature(1, signature->GetBufferPointer(), signature->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature)), cerr, nullptr);
	if (name) rootSignature->SetName(name);
	pPipelineLayoutCache->SetPipelineLayout(key, rootSignature.get());

	return rootSignature.get();
}

XUSG::PipelineLayout RayTracing::PipelineLayout_DX12::GetPipelineLayout(const Device* pDevice,
	PipelineLayoutCache* pPipelineLayoutCache, PipelineLayoutFlag flags, const wchar_t* name)
{
	auto layout = pPipelineLayoutCache->GetPipelineLayout(this, flags, name, false);

	if (!layout) return CreatePipelineLayout(pDevice, pPipelineLayoutCache, flags, name);

	return layout;
}
