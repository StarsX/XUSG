//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSGCommand_DX12.h"
#include "XUSGRayTracingCommand_DX12.h"

using namespace std;
using namespace XUSG;
using namespace XUSG::RayTracing;

RayTracing::CommandList_DX12::CommandList_DX12() :
	XUSG::CommandList_DX12()
{
}

RayTracing::CommandList_DX12::~CommandList_DX12()
{
}

#if ENABLE_DXR_FALLBACK
RayTracing::CommandList_DX12::CommandList_DX12(XUSG::CommandList* pCommandList, const RayTracing::Device* pDevice)
{
	m_commandList = dynamic_cast<XUSG::CommandList_DX12*>(pCommandList)->GetGraphicsCommandList();
	CreateInterface(pDevice);
}
#else
RayTracing::CommandList_DX12::CommandList_DX12(XUSG::CommandList* pCommandList)
{
	m_commandList = dynamic_cast<XUSG::CommandList_DX12*>(pCommandList)->GetGraphicsCommandList();
	CreateInterface();
}
#endif

#if ENABLE_DXR_FALLBACK
bool RayTracing::CommandList_DX12::CreateInterface(const RayTracing::Device* pDevice)
{
	const auto pDxDevice = static_cast<ID3D12RaytracingFallbackDevice*>(pDevice->GetRTHandle());

	assert(pDxDevice);
	pDxDevice->QueryRaytracingCommandList(m_commandList.get(), IID_PPV_ARGS(&m_commandListRT));

	return true;
}
#else
bool RayTracing::CommandList_DX12::CreateInterface()
{
	const auto hr = m_commandList->QueryInterface(IID_PPV_ARGS(&m_commandListRT));
	if (FAILED(hr))
	{
		OutputDebugString(L"Couldn't get DirectX Raytracing interface for the command list.\n");

		return false;
	}

	return true;
}
#endif

void RayTracing::CommandList_DX12::BuildRaytracingAccelerationStructure(const BuildDesc* pDesc, uint32_t numPostbuildInfoDescs,
	const PostbuildInfo* pPostbuildInfoDescs, const DescriptorPool& descriptorPool) const
{
	const auto pPostbuildInfo = reinterpret_cast<const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC*>(pPostbuildInfoDescs);
#if ENABLE_DXR_FALLBACK
	// Set the descriptor heaps to be used during acceleration structure build for the Fallback Layer.
	const auto pDescriptorHeap = reinterpret_cast<ID3D12DescriptorHeap*>(descriptorPool);
	m_commandListRT->SetDescriptorHeaps(1, &pDescriptorHeap);
	m_commandListRT->BuildRaytracingAccelerationStructure(pDesc, numPostbuildInfoDescs,
		pPostbuildInfo, AccelerationStructure::GetUAVCount());
#else // DirectX Raytracing
	m_commandListRT->BuildRaytracingAccelerationStructure(pDesc, numPostbuildInfoDescs, pPostbuildInfo);
#endif
}

void RayTracing::CommandList_DX12::SetDescriptorPools(uint32_t numDescriptorPools, const DescriptorPool* pDescriptorPools) const
{
	m_commandListRT->SetDescriptorHeaps(numDescriptorPools, reinterpret_cast<ID3D12DescriptorHeap* const*>(pDescriptorPools));
}

void RayTracing::CommandList_DX12::SetTopLevelAccelerationStructure(uint32_t index, const TopLevelAS* pTopLevelAS) const
{
#if ENABLE_DXR_FALLBACK
	m_commandListRT->SetTopLevelAccelerationStructure(index, pTopLevelAS->GetResultPointer());
#else // DirectX Raytracing
	XUSG::CommandList_DX12::SetComputeRootShaderResourceView(index, pTopLevelAS->GetResult().get());
#endif
}

void RayTracing::CommandList_DX12::DispatchRays(const Pipeline& pipeline,
	uint32_t width, uint32_t height, uint32_t depth,
	const ShaderTable* pHitGroup,
	const ShaderTable* pMiss,
	const ShaderTable* pRayGen) const
{
	auto dispatchRays = [&](auto* commandList, auto* stateObject, auto* dispatchDesc)
	{
		// Since each shader table has only one shader record, the stride is same as the size.
		const auto dxHitGroup = static_cast<ID3D12Resource*>(pHitGroup->GetResource()->GetHandle());
		const auto dxMiss = static_cast<ID3D12Resource*>(pMiss->GetResource()->GetHandle());
		const auto dxRayGen = static_cast<ID3D12Resource*>(pRayGen->GetResource()->GetHandle());
		dispatchDesc->HitGroupTable.StartAddress = dxHitGroup->GetGPUVirtualAddress();
		dispatchDesc->HitGroupTable.SizeInBytes = pHitGroup->GetResource()->GetWidth();
		dispatchDesc->HitGroupTable.StrideInBytes = pHitGroup->GetShaderRecordSize();
		dispatchDesc->MissShaderTable.StartAddress = dxMiss->GetGPUVirtualAddress();
		dispatchDesc->MissShaderTable.SizeInBytes = pMiss->GetResource()->GetWidth();
		dispatchDesc->MissShaderTable.StrideInBytes = pMiss->GetShaderRecordSize();
		dispatchDesc->RayGenerationShaderRecord.StartAddress = dxRayGen->GetGPUVirtualAddress();
		dispatchDesc->RayGenerationShaderRecord.SizeInBytes = pRayGen->GetResource()->GetWidth();
		dispatchDesc->Width = width;
		dispatchDesc->Height = height;
		dispatchDesc->Depth = depth;
#if ENABLE_DXR_FALLBACK
		commandList->SetPipelineState1(static_cast<ID3D12RaytracingFallbackStateObject*>(stateObject));
#else // DirectX Raytracing
		commandList->SetPipelineState1(static_cast<ID3D12StateObject*>(stateObject));
#endif
		commandList->DispatchRays(dispatchDesc);
	};

	D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
	dispatchRays(m_commandListRT.get(), pipeline, &dispatchDesc);
}
