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

bool RayTracing::CommandList_DX12::CreateRaytracingInterfaces(const RayTracing::Device& device)
{
#if ENABLE_DXR_FALLBACK
	m_raytracingAPI = device.RaytracingAPI;

	if (m_raytracingAPI == API::FallbackLayer)
		device.Fallback->QueryRaytracingCommandList(m_commandList.get(), IID_PPV_ARGS(&m_fallback));
	else // DirectX Raytracing
#endif
	{
		const auto hr = m_commandList->QueryInterface(IID_PPV_ARGS(&m_native));
		if (FAILED(hr))
		{
			OutputDebugString(L"Couldn't get DirectX Raytracing interface for the command list.\n");

			return false;
		}
	}

	return true;
}

void RayTracing::CommandList_DX12::BuildRaytracingAccelerationStructure(const BuildDesc* pDesc, uint32_t numPostbuildInfoDescs,
	const PostbuildInfo* pPostbuildInfoDescs, const DescriptorPool& descriptorPool) const
{
#if ENABLE_DXR_FALLBACK
	if (m_raytracingAPI == API::FallbackLayer)
	{
		// Set the descriptor heaps to be used during acceleration structure build for the Fallback Layer.
		m_fallback->SetDescriptorHeaps(1, descriptorPool.GetAddressOf());
		m_fallback->BuildRaytracingAccelerationStructure(pDesc, numPostbuildInfoDescs,
			pPostbuildInfoDescs, AccelerationStructure::GetUAVCount());
	}
	else // DirectX Raytracing
#endif
		m_native->BuildRaytracingAccelerationStructure(pDesc, numPostbuildInfoDescs, pPostbuildInfoDescs);
}

void RayTracing::CommandList_DX12::SetDescriptorPools(uint32_t numDescriptorPools, const DescriptorPool* pDescriptorPools) const
{
	vector<DescriptorPool::element_type*> ppDescriptorPools(numDescriptorPools);
	for (auto i = 0u; i < numDescriptorPools; ++i)
		ppDescriptorPools[i] = pDescriptorPools[i].get();

#if ENABLE_DXR_FALLBACK
	if (m_raytracingAPI == API::FallbackLayer)
		m_fallback->SetDescriptorHeaps(numDescriptorPools, ppDescriptorPools.data());
	else // DirectX Raytracing
#endif
		m_commandList->SetDescriptorHeaps(numDescriptorPools, ppDescriptorPools.data());
}

void RayTracing::CommandList_DX12::SetTopLevelAccelerationStructure(uint32_t index, const TopLevelAS& topLevelAS) const
{
#if ENABLE_DXR_FALLBACK
	if (m_raytracingAPI == API::FallbackLayer)
		m_fallback->SetTopLevelAccelerationStructure(index, topLevelAS.GetResultPointer());
	else // DirectX Raytracing
#endif
		XUSG::CommandList_DX12::SetComputeRootShaderResourceView(index, const_cast<TopLevelAS&>(topLevelAS).GetResult()->GetResource());
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
#if ENABLE_DXR_FALLBACK
	if (m_raytracingAPI == API::FallbackLayer)
		dispatchRays(m_fallback.get(), pipeline.Fallback.get(), &dispatchDesc);
	else // DirectX Raytracing
#endif
		dispatchRays(m_native.get(), pipeline.Native.get(), &dispatchDesc);
}
