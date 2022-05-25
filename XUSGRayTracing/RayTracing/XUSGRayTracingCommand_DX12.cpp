//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSGCommand_DX12.h"
#include "Core/XUSGResource_DX12.h"
#include "Ultimate/XUSGUltimate_DX12.h"
#include "XUSGRayTracingCommand_DX12.h"
#include "XUSGAccelerationStructure_DX12.h"

using namespace std;
using namespace XUSG;
using namespace XUSG::RayTracing;

RayTracing::CommandList_DX12::CommandList_DX12() :
	Ultimate::CommandList_DX12()
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
	if (!Ultimate::CommandList_DX12::CreateInterface()) m_commandListU = nullptr;

	m_pDeviceRT = dynamic_cast<const RayTracing::Device*>(m_pDevice);
	const auto pDxDevice = static_cast<ID3D12RaytracingFallbackDevice*>(m_pDeviceRT->GetRTHandle());
	XUSG_N_RETURN(pDxDevice, false);

	pDxDevice->QueryRaytracingCommandList(m_commandList.get(), IID_PPV_ARGS(&m_commandListRT));

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

	assert(numPostbuildInfoDescs == 0 || pPostbuildInfoDescs);
	vector<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC> postbuildInfoDescs(numPostbuildInfoDescs);
	for (auto i = 0u; i < numPostbuildInfoDescs; ++i)
	{
		postbuildInfoDescs[i].DestBuffer = pPostbuildInfoDescs[i].DestBuffer;
		postbuildInfoDescs[i].InfoType = infoTypes[pPostbuildInfoDescs[i].InfoType];
	}

	const auto pDxDevice = static_cast<ID3D12RaytracingFallbackDevice*>(m_pDeviceRT->GetRTHandle());
	if (!pDxDevice->UsingRaytracingDriver())
	{
		// Set the descriptor heaps to be used during acceleration structure build for the Fallback Layer.
		const auto pDescriptorHeap = reinterpret_cast<ID3D12DescriptorHeap*>(descriptorPool);
		m_commandListRT->SetDescriptorHeaps(1, &pDescriptorHeap);
	}
	
	const auto pBuildDesc = static_cast<const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC*>(pDesc);
	m_commandListRT->BuildRaytracingAccelerationStructure(pBuildDesc, numPostbuildInfoDescs,
		pPostbuildInfoDescs ? postbuildInfoDescs.data() : nullptr, AccelerationStructure::GetUAVCount());
}

void RayTracing::CommandList_DX12::SetDescriptorPools(uint32_t numDescriptorPools, const DescriptorPool* pDescriptorPools) const
{
	assert(numDescriptorPools == 0 || pDescriptorPools);
	m_commandListRT->SetDescriptorHeaps(numDescriptorPools, pDescriptorPools ?
		reinterpret_cast<ID3D12DescriptorHeap* const*>(pDescriptorPools) : nullptr);
}

void RayTracing::CommandList_DX12::SetTopLevelAccelerationStructure(uint32_t index, const TopLevelAS* pTopLevelAS) const
{
	SetTopLevelAccelerationStructure(index, pTopLevelAS ? pTopLevelAS->GetResultPointer() : 0);
}

void XUSG::RayTracing::CommandList_DX12::SetTopLevelAccelerationStructure(uint32_t index, uint64_t topLevelASPtr) const
{
	m_commandListRT->SetTopLevelAccelerationStructure(index,
		reinterpret_cast<const WRAPPED_GPU_POINTER&>(topLevelASPtr));
}

void RayTracing::CommandList_DX12::SetRayTracingPipeline(const Pipeline& pipeline) const
{
	m_commandListRT->SetPipelineState1(static_cast<ID3D12RaytracingFallbackStateObject*>(pipeline));
}

void RayTracing::CommandList_DX12::DispatchRays(uint32_t width, uint32_t height, uint32_t depth,
	const ShaderTable* pHitGroup, const ShaderTable* pMiss, const ShaderTable* pRayGen) const
{
	D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};

	// Since each shader table has only one shader record, the stride is same as the size.
	const auto dxHitGroup = pHitGroup ? static_cast<ID3D12Resource*>(pHitGroup->GetResource()->GetHandle()) : nullptr;
	const auto dxMiss = pMiss ? static_cast<ID3D12Resource*>(pMiss->GetResource()->GetHandle()) : nullptr;
	const auto dxRayGen = pRayGen ? static_cast<ID3D12Resource*>(pRayGen->GetResource()->GetHandle()) : nullptr;
	dispatchDesc.HitGroupTable.StartAddress = dxHitGroup ? dxHitGroup->GetGPUVirtualAddress() : 0;
	dispatchDesc.HitGroupTable.SizeInBytes = pHitGroup ? pHitGroup->GetResource()->GetWidth() : 0;
	dispatchDesc.HitGroupTable.StrideInBytes = pHitGroup ? pHitGroup->GetShaderRecordSize() : 0;
	dispatchDesc.MissShaderTable.StartAddress = dxMiss ? dxMiss->GetGPUVirtualAddress() : 0;
	dispatchDesc.MissShaderTable.SizeInBytes = pMiss ? pMiss->GetResource()->GetWidth() : 0;
	dispatchDesc.MissShaderTable.StrideInBytes = pMiss ? pMiss->GetShaderRecordSize() : 0;
	dispatchDesc.RayGenerationShaderRecord.StartAddress = dxRayGen ? dxRayGen->GetGPUVirtualAddress() : 0;
	dispatchDesc.RayGenerationShaderRecord.SizeInBytes = pRayGen ? pRayGen->GetResource()->GetWidth() : 0;
	dispatchDesc.Width = width;
	dispatchDesc.Height = height;
	dispatchDesc.Depth = depth;

	m_commandListRT->DispatchRays(&dispatchDesc);
}

void RayTracing::CommandList_DX12::DispatchRays(const Pipeline& pipeline,
	uint32_t width, uint32_t height, uint32_t depth,
	const ShaderTable* pHitGroup,
	const ShaderTable* pMiss,
	const ShaderTable* pRayGen) const
{
	SetRayTracingPipeline(pipeline);
	DispatchRays(width, height, depth, pHitGroup, pMiss, pRayGen);
}

const RayTracing::Device* RayTracing::CommandList_DX12::GetRTDevice() const
{
	return m_pDeviceRT;
}
