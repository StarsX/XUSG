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
RayTracing::CommandList_DX12::CommandList_DX12(XUSG::CommandList& commandList, const RayTracing::Device& device)
{
	m_commandList = dynamic_cast<XUSG::CommandList_DX12&>(commandList).GetGraphicsCommandList();
	CreateInterface(device);
}
#else
RayTracing::CommandList_DX12::CommandList_DX12(XUSG::CommandList& commandList)
{
	m_commandList = dynamic_cast<XUSG::CommandList_DX12&>(commandList).GetGraphicsCommandList();
	CreateInterface();
}
#endif

#if ENABLE_DXR_FALLBACK
bool RayTracing::CommandList_DX12::CreateInterface(const RayTracing::Device& device)
{
	device.Derived->QueryRaytracingCommandList(m_commandList.get(), IID_PPV_ARGS(&m_commandListR));

	return true;
}
#else
bool RayTracing::CommandList_DX12::CreateInterface()
{
	const auto hr = m_commandList->QueryInterface(IID_PPV_ARGS(&m_commandListR));
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
#if ENABLE_DXR_FALLBACK
	// Set the descriptor heaps to be used during acceleration structure build for the Fallback Layer.
	m_commandListR->SetDescriptorHeaps(1, descriptorPool.GetAddressOf());
	m_commandListR->BuildRaytracingAccelerationStructure(pDesc, numPostbuildInfoDescs,
		pPostbuildInfoDescs, AccelerationStructure::GetUAVCount());
#else // DirectX Raytracing
	m_commandListR->BuildRaytracingAccelerationStructure(pDesc, numPostbuildInfoDescs, pPostbuildInfoDescs);
#endif
}

void RayTracing::CommandList_DX12::SetDescriptorPools(uint32_t numDescriptorPools, const DescriptorPool* pDescriptorPools) const
{
	vector<DescriptorPool::element_type*> ppDescriptorPools(numDescriptorPools);
	for (auto i = 0u; i < numDescriptorPools; ++i)
		ppDescriptorPools[i] = pDescriptorPools[i].get();

	m_commandListR->SetDescriptorHeaps(numDescriptorPools, ppDescriptorPools.data());
}

void RayTracing::CommandList_DX12::SetTopLevelAccelerationStructure(uint32_t index, const TopLevelAS& topLevelAS) const
{
#if ENABLE_DXR_FALLBACK
	m_commandListR->SetTopLevelAccelerationStructure(index, topLevelAS.GetResultPointer());
#else // DirectX Raytracing
	XUSG::CommandList_DX12::SetComputeRootShaderResourceView(index, const_cast<TopLevelAS&>(topLevelAS).GetResult()->GetResource());
#endif
}

void RayTracing::CommandList_DX12::DispatchRays(const RayTracing::Pipeline& pipeline,
	uint32_t width, uint32_t height, uint32_t depth,
	const ShaderTable& hitGroup,
	const ShaderTable& miss,
	const ShaderTable& rayGen) const
{
	auto dispatchRays = [&](auto* commandList, auto* stateObject, auto* dispatchDesc)
	{
		// Since each shader table has only one shader record, the stride is same as the size.
		dispatchDesc->HitGroupTable.StartAddress = hitGroup.GetResource()->GetGPUVirtualAddress();
		dispatchDesc->HitGroupTable.SizeInBytes = hitGroup.GetResource()->GetDesc().Width;
		dispatchDesc->HitGroupTable.StrideInBytes = hitGroup.GetShaderRecordSize();
		dispatchDesc->MissShaderTable.StartAddress = miss.GetResource()->GetGPUVirtualAddress();
		dispatchDesc->MissShaderTable.SizeInBytes = miss.GetResource()->GetDesc().Width;
		dispatchDesc->MissShaderTable.StrideInBytes = miss.GetShaderRecordSize();
		dispatchDesc->RayGenerationShaderRecord.StartAddress = rayGen.GetResource()->GetGPUVirtualAddress();
		dispatchDesc->RayGenerationShaderRecord.SizeInBytes = rayGen.GetResource()->GetDesc().Width;
		dispatchDesc->Width = width;
		dispatchDesc->Height = height;
		dispatchDesc->Depth = depth;
		commandList->SetPipelineState1(stateObject);
		commandList->DispatchRays(dispatchDesc);
	};

	D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
	dispatchRays(m_commandListR.get(), pipeline.get(), &dispatchDesc);
}
