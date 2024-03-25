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
	const PostbuildInfo* pPostbuildInfoDescs, const DescriptorHeap* pDescriptorHeap)
{
	assert(numPostbuildInfoDescs == 0 || pPostbuildInfoDescs);
	vector<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC> postbuildInfoDescs(numPostbuildInfoDescs);
	for (auto i = 0u; i < numPostbuildInfoDescs; ++i)
	{
		postbuildInfoDescs[i].DestBuffer = pPostbuildInfoDescs[i].DestBuffer;
		postbuildInfoDescs[i].InfoType = GetDXRAccelerationStructurePostbuildInfoType(pPostbuildInfoDescs[i].InfoType);
	}

	// Set the descriptor heaps to be used during acceleration structure build for the Fallback Layer.
	const auto pDxDevice = static_cast<ID3D12RaytracingFallbackDevice*>(m_pDeviceRT->GetRTHandle());
	if (!pDxDevice->UsingRaytracingDriver() && pDescriptorHeap) SetDescriptorHeaps(1, pDescriptorHeap);

	assert(pDesc);
	const auto pBuildDesc = static_cast<const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC*>(pDesc);
	m_commandListRT->BuildRaytracingAccelerationStructure(pBuildDesc, numPostbuildInfoDescs,
		pPostbuildInfoDescs ? postbuildInfoDescs.data() : nullptr, AccelerationStructure::GetUAVCount());
}

void RayTracing::CommandList_DX12::EmitRaytracingAccelerationStructurePostbuildInfo(const PostbuildInfo* pDesc,
	uint32_t numAccelerationStructures, const uint64_t* pAccelerationStructureData) const
{
	assert(pDesc);
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC desc;
	desc.DestBuffer = pDesc->DestBuffer;
	desc.InfoType = GetDXRAccelerationStructurePostbuildInfoType(pDesc->InfoType);

	m_commandListRT->EmitRaytracingAccelerationStructurePostbuildInfo(&desc, numAccelerationStructures,
		pAccelerationStructureData, AccelerationStructure::GetUAVCount());
}

void RayTracing::CommandList_DX12::CopyRaytracingAccelerationStructure(const AccelerationStructure* pDst,
	const AccelerationStructure* pSrc, CopyMode mode, const DescriptorHeap* pDescriptorHeap)
{
	// Set the descriptor heaps to be used during acceleration structure build for the Fallback Layer.
	const auto pDxDevice = static_cast<ID3D12RaytracingFallbackDevice*>(m_pDeviceRT->GetRTHandle());
	if (!pDxDevice->UsingRaytracingDriver() && pDescriptorHeap) SetDescriptorHeaps(1, pDescriptorHeap);

	m_commandListRT->CopyRaytracingAccelerationStructure(pDst->GetResource()->GetVirtualAddress(),
		pSrc->GetResource()->GetVirtualAddress(), GetDXRAccelerationStructureCopyMode(mode),
		AccelerationStructure::GetUAVCount());
}

void RayTracing::CommandList_DX12::SetDescriptorHeaps(uint32_t numDescriptorHeaps, const DescriptorHeap* pDescriptorHeaps)
{
	assert(numDescriptorHeaps == 0 || pDescriptorHeaps);
	assert(numDescriptorHeaps <= size(m_descriptorHeapStarts));
	for (auto i = 0u; i < numDescriptorHeaps; ++i)
	{
		const auto pDescriptorHeap = static_cast<ID3D12DescriptorHeap*>(pDescriptorHeaps[i]);
		const auto desc = pDescriptorHeap->GetDesc();
		const auto isCbvSrvUavHeap = desc.Type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		assert(isCbvSrvUavHeap || desc.Type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		assert(desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

		const auto heapType = isCbvSrvUavHeap ? CBV_SRV_UAV_HEAP : SAMPLER_HEAP;
		m_descriptorHeapStarts[heapType] = pDescriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr;
	}

	m_commandListRT->SetDescriptorHeaps(numDescriptorHeaps, pDescriptorHeaps ?
		reinterpret_cast<ID3D12DescriptorHeap* const*>(pDescriptorHeaps) : nullptr);
}

void RayTracing::CommandList_DX12::SetTopLevelAccelerationStructure(uint32_t index, const TopLevelAS* pTopLevelAS) const
{
	SetTopLevelAccelerationStructure(index, pTopLevelAS ? pTopLevelAS->GetResourcePointer() : 0);
}

void RayTracing::CommandList_DX12::SetTopLevelAccelerationStructure(uint32_t index, uint64_t topLevelASPtr) const
{
	m_commandListRT->SetTopLevelAccelerationStructure(index,
		reinterpret_cast<const WRAPPED_GPU_POINTER&>(topLevelASPtr));
}

void RayTracing::CommandList_DX12::SetRayTracingPipeline(const Pipeline& pipeline) const
{
	m_commandListRT->SetPipelineState1(static_cast<ID3D12RaytracingFallbackStateObject*>(pipeline));
}

void RayTracing::CommandList_DX12::DispatchRays(uint32_t width, uint32_t height, uint32_t depth,
	const ShaderTable* pRayGen,
	const ShaderTable* pHitGroup,
	const ShaderTable* pMiss,
	const ShaderTable* pCallable) const
{
	D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};

	// Since each shader table has only one shader record, the stride is same as the size.
	const auto dxRayGen = pRayGen ? static_cast<ID3D12Resource*>(pRayGen->GetResource()->GetHandle()) : nullptr;
	const auto dxHitGroup = pHitGroup ? static_cast<ID3D12Resource*>(pHitGroup->GetResource()->GetHandle()) : nullptr;
	const auto dxMiss = pMiss ? static_cast<ID3D12Resource*>(pMiss->GetResource()->GetHandle()) : nullptr;
	const auto dxCallable = pCallable ? static_cast<ID3D12Resource*>(pCallable->GetResource()->GetHandle()) : nullptr;
	dispatchDesc.RayGenerationShaderRecord.StartAddress = dxRayGen ? dxRayGen->GetGPUVirtualAddress() : 0;
	dispatchDesc.RayGenerationShaderRecord.SizeInBytes = pRayGen ? pRayGen->GetResource()->GetWidth() : 0;
	dispatchDesc.HitGroupTable.StartAddress = dxHitGroup ? dxHitGroup->GetGPUVirtualAddress() : 0;
	dispatchDesc.HitGroupTable.SizeInBytes = pHitGroup ? pHitGroup->GetResource()->GetWidth() : 0;
	dispatchDesc.HitGroupTable.StrideInBytes = pHitGroup ? pHitGroup->GetShaderRecordSize() : 0;
	dispatchDesc.MissShaderTable.StartAddress = dxMiss ? dxMiss->GetGPUVirtualAddress() : 0;
	dispatchDesc.MissShaderTable.SizeInBytes = pMiss ? pMiss->GetResource()->GetWidth() : 0;
	dispatchDesc.MissShaderTable.StrideInBytes = pMiss ? pMiss->GetShaderRecordSize() : 0;
	dispatchDesc.CallableShaderTable.StartAddress = pCallable ? dxCallable->GetGPUVirtualAddress() : 0;
	dispatchDesc.CallableShaderTable.SizeInBytes = pCallable ? pCallable->GetResource()->GetWidth() : 0;
	dispatchDesc.CallableShaderTable.StrideInBytes = pCallable ? pCallable->GetShaderRecordSize() : 0;
	dispatchDesc.Width = width;
	dispatchDesc.Height = height;
	dispatchDesc.Depth = depth;

	m_commandListRT->DispatchRays(&dispatchDesc);
}

void RayTracing::CommandList_DX12::DispatchRays(const Pipeline& pipeline,
	uint32_t width, uint32_t height, uint32_t depth,
	const ShaderTable* pRayGen,
	const ShaderTable* pHitGroup,
	const ShaderTable* pMiss,
	const ShaderTable* pCallable) const
{
	SetRayTracingPipeline(pipeline);
	DispatchRays(width, height, depth, pRayGen, pHitGroup, pMiss, pCallable);
}

const RayTracing::Device* RayTracing::CommandList_DX12::GetRTDevice() const
{
	return m_pDeviceRT;
}

D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_TYPE XUSG::RayTracing::GetDXRAccelerationStructurePostbuildInfoType(
	PostbuildInfoType type)
{
	static const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_TYPE postbuildInfoTypes[] =
	{
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE,
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_TOOLS_VISUALIZATION,
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_SERIALIZATION,
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE
	};

	return postbuildInfoTypes[static_cast<uint32_t>(type)];
}

D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE XUSG::RayTracing::GetDXRAccelerationStructureCopyMode(
	CopyMode mode)
{
	static const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE copyModes[] =
	{
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_CLONE,
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_COMPACT,
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_VISUALIZATION_DECODE_FOR_TOOLS,
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_SERIALIZE,
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_DESERIALIZE
	};

	return copyModes[static_cast<uint32_t>(mode)];
}
