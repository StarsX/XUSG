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
	const auto layoutCache = dynamic_cast<PipelineLayoutCache_DX12*>(pPipelineLayoutCache);
	m_pipelineLayoutKey[0] = static_cast<uint8_t>(flags);
	const auto& key = m_pipelineLayoutKey;

	const auto numRootParams = static_cast<uint32_t>((key.size() - 1) / sizeof(void*));
	const auto pDescriptorTableLayouts = reinterpret_cast<const DescriptorTableLayout*>(&key[1]);

	vector<CD3DX12_ROOT_PARAMETER1> rootParams(numRootParams);
	vector<vector<CD3DX12_DESCRIPTOR_RANGE1>> descriptorRanges(numRootParams);
	for (auto i = 0u; i < numRootParams; ++i)
		layoutCache->GetRootParameter(rootParams[i], descriptorRanges[i], pDescriptorTableLayouts[i]);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init_1_1(numRootParams, rootParams.data(), 0, nullptr, GetDX12RootSignatureFlags(flags));

	com_ptr<ID3D12RootSignature> rootSignature;
	com_ptr<ID3DBlob> signature, error;
#if ENABLE_DXR_FALLBACK
	const auto pDxDevice = static_cast<ID3D12RaytracingFallbackDevice*>(pDevice->GetRTHandle());
	H_RETURN(pDxDevice->D3D12SerializeRootSignature(&rootSignatureDesc.Desc_1_0, D3D_ROOT_SIGNATURE_VERSION_1, &signature,
		&error, AccelerationStructure::GetUAVCount()), cerr, reinterpret_cast<wchar_t*>(error->GetBufferPointer()), nullptr);
#else // DirectX Raytracing
	const auto pDxDevice = static_cast<ID3D12Device5*>(pDevice->GetRTHandle());
	H_RETURN(D3D12SerializeRootSignature(&rootSignatureDesc.Desc_1_0, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error),
		cerr, reinterpret_cast<wchar_t*>(error->GetBufferPointer()), nullptr);
#endif

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

	if (!layout)
	{
		layout = CreatePipelineLayout(pDevice, pPipelineLayoutCache, flags, name);
		
	}

	return layout;
}
