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

RayTracing::CommandList_DX12::CommandList_DX12(XUSG::CommandList* pCommandList, const RayTracing::Device* pDevice)
{
	m_commandList = dynamic_cast<XUSG::CommandList_DX12*>(pCommandList)->GetGraphicsCommandList();
	m_pDevice = pDevice;
	CreateInterface();
}

bool RayTracing::CommandList_DX12::CreateInterface()
{
	m_pDeviceRT = dynamic_cast<const RayTracing::Device*>(m_pDevice);
#if ENABLE_DXR_FALLBACK
	const auto pDxDevice = static_cast<ID3D12RaytracingFallbackDevice*>(m_pDeviceRT->GetRTHandle());

	assert(pDxDevice);
	pDxDevice->QueryRaytracingCommandList(m_commandList.get(), IID_PPV_ARGS(&m_commandListRT));
#else
	const auto hr = m_commandList->QueryInterface(IID_PPV_ARGS(&m_commandListRT));
	if (FAILED(hr))
	{
		OutputDebugString(L"Couldn't get DirectX Raytracing interface for the command list.\n");

		return false;
	}
#endif

	return true;
}

void RayTracing::CommandList_DX12::BuildRaytracingAccelerationStructure(const BuildDesc* pDesc, uint32_t numPostbuildInfoDescs,
	const PostbuildInfo* pPostbuildInfoDescs, const DescriptorPool& descriptorPool) const
{
	const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_TYPE infoTypes[] =
	{
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE,
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_TOOLS_VISUALIZATION,
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_SERIALIZATION,
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE
	};

	vector<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC> postbuildInfoDescs(numPostbuildInfoDescs);
	for (auto i = 0u; i < numPostbuildInfoDescs; ++i)
	{
		postbuildInfoDescs[i].DestBuffer = pPostbuildInfoDescs[i].DestBuffer;
		postbuildInfoDescs[i].InfoType = infoTypes[pPostbuildInfoDescs[i].InfoType];
	}

	const auto pBuildDesc = static_cast<const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC*>(pDesc);
#if ENABLE_DXR_FALLBACK
	// Set the descriptor heaps to be used during acceleration structure build for the Fallback Layer.
	const auto pDescriptorHeap = reinterpret_cast<ID3D12DescriptorHeap*>(descriptorPool);
	m_commandListRT->SetDescriptorHeaps(1, &pDescriptorHeap);
	m_commandListRT->BuildRaytracingAccelerationStructure(pBuildDesc, numPostbuildInfoDescs,
		postbuildInfoDescs.data(), AccelerationStructure::GetUAVCount());
#else // DirectX Raytracing
	m_commandListRT->BuildRaytracingAccelerationStructure(pBuildDesc, numPostbuildInfoDescs, postbuildInfoDescs.data());
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

const RayTracing::Device* RayTracing::CommandList_DX12::GetRTDevice() const
{
	return m_pDeviceRT;
}
