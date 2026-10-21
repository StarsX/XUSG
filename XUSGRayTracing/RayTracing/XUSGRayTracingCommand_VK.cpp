//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSGCommand_VK.h"
#include "Ultimate/XUSGUltimate_VK.h"
#include "XUSGRayTracingCommand_VK.h"
#include "XUSGAccelerationStructure_VK.h"
#include "XUSGRayTracingState_VK.h"

using namespace std;
using namespace XUSG;
using namespace XUSG::RayTracing;

RayTracing::CommandList_VK::RTState::~RTState()
{
	const auto device = m_pDevice ? m_pDevice->GetDevice() : VK_NULL_HANDLE;
	for (auto& pool : m_queryPools)
		if (pool.Pool != VK_NULL_HANDLE) vkDestroyQueryPool(device, pool.Pool, nullptr);
}

RayTracing::CommandList_VK::CommandList_VK() :
	Ultimate::CommandList_VK(),
	m_rt(make_shared<RTState>()),
	m_pDeviceRT(nullptr)
{
}

RayTracing::CommandList_VK::~CommandList_VK()
{
}

RayTracing::CommandList_VK::CommandList_VK(XUSG::CommandList* pCommandList, RayTracing::Device* pDevice) :
	CommandList_VK()
{
	const auto p = XUSG::AsVK(pCommandList);
	assert(p);

	Attach(*p);
	if (const auto pUltimate = dynamic_cast<Ultimate::CommandList_VK*>(pCommandList))
		AttachUltimate(*pUltimate);
	if (const auto pRT = dynamic_cast<CommandList_VK*>(pCommandList))
		AttachRayTracing(*pRT);
	m_pDevice = XUSG::AsVK(pDevice);
	m_pDeviceRT = pDevice;
	createInterface();
}

bool RayTracing::CommandList_VK::Create(XUSG::Device* pDevice, uint32_t nodeMask, CommandListType type,
	uint8_t numAllocators, CommandAllocator* const* ppAllocators, const Pipeline& pipeline, const wchar_t* name)
{
	const auto pDeviceRT = dynamic_cast<Device*>(pDevice);
	assert(pDeviceRT);

	return Create(pDeviceRT, nodeMask, type, numAllocators, ppAllocators, pipeline, name);
}

bool RayTracing::CommandList_VK::Create(Device* pDevice, uint32_t nodeMask, CommandListType type,
	uint8_t numAllocators, CommandAllocator* const* ppAllocators, const Pipeline& pipeline, const wchar_t* name)
{
	XUSG_N_RETURN(Ultimate::CommandList_VK::Create(pDevice, nodeMask, type, numAllocators, ppAllocators, pipeline, name), false);
	m_pDeviceRT = pDevice;

	return createInterface();
}

void RayTracing::CommandList_VK::Create(void* pNativeHandle, Device* pDevice)
{
	XUSG::CommandList_VK::Create(pNativeHandle, pDevice);
	Ultimate::CommandList_VK::createInterface();
	m_pDeviceRT = pDevice;
	createInterface();
}

void RayTracing::CommandList_VK::AttachRayTracing(const CommandList_VK& source)
{
	if (this == &source) return;

	m_rt = source.m_rt;
	m_pDeviceRT = source.m_pDeviceRT;
}

bool RayTracing::CommandList_VK::Reset(uint8_t allocatorIndex, const Pipeline& initialState) const
{
	for (auto& pool : m_rt->m_queryPools) pool.Used = 0;

	return XUSG::CommandList_VK::Reset(allocatorIndex, initialState);
}

void RayTracing::CommandList_VK::BuildRaytracingAccelerationStructure(const BuildDesc* pDesc, uint32_t numPostbuildInfoDescs,
	const PostbuildInfo* pPostbuildInfoDescs, const DescriptorHeap* pDescriptorHeap)
{
	(void)pDescriptorHeap;
	assert(pDesc);
	assert(numPostbuildInfoDescs == 0 || pPostbuildInfoDescs);
	assert(m_rt->m_cmdBuildAccelerationStructures);

	recordOutside();
	const auto pBuild = static_cast<const BuildDesc_VK*>(pDesc);
	if (pBuild->Micromap != VK_NULL_HANDLE)
	{
		assert(m_rt->m_cmdBuildMicromaps);
		if (m_rt->m_cmdBuildMicromaps) m_rt->m_cmdBuildMicromaps(m_state->m_commandBuffer, 1, &pBuild->MicromapInfo);
	}
	else
	{
		const VkAccelerationStructureBuildRangeInfoKHR* pRanges = pBuild->Ranges.empty() ? nullptr : pBuild->Ranges.data();
		if (pBuild->Type == VK_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_KHR)
		{
			pRanges = nullptr;
			m_rt->m_cmdBuildAccelerationStructures(m_state->m_commandBuffer, 1, &pBuild->Info, &pRanges);
		}
		else m_rt->m_cmdBuildAccelerationStructures(m_state->m_commandBuffer, 1, &pBuild->Info, pRanges ? &pRanges : nullptr);
	}

	if (!numPostbuildInfoDescs) return;

	const uint64_t accelerationStructure = pBuild->Micromap != VK_NULL_HANDLE ?
		reinterpret_cast<uint64_t>(pBuild->Micromap) : reinterpret_cast<uint64_t>(pBuild->AccelerationStructure);
	for (auto i = 0u; i < numPostbuildInfoDescs; ++i)
		EmitRaytracingAccelerationStructurePostbuildInfo(&pPostbuildInfoDescs[i], 1, &accelerationStructure);
}

void RayTracing::CommandList_VK::EmitRaytracingAccelerationStructurePostbuildInfo(const PostbuildInfo* pDesc,
	uint32_t numAccelerationStructures, const uint64_t* pAccelerationStructureData) const
{
	assert(pDesc);
	assert(numAccelerationStructures == 0 || pAccelerationStructureData);
	if (!numAccelerationStructures) return;

	if (pDesc->InfoType == PostbuildInfoType::TOOLS_VISUALIZATION)
	{
		assert(!"Visualization postbuild info is not supported");

		return;
	}

	vector<VkAccelerationStructureKHR> accelerationStructures;
	vector<VkMicromapEXT> micromaps;
	accelerationStructures.reserve(numAccelerationStructures);
	for (auto i = 0u; i < numAccelerationStructures; ++i)
	{
		const auto accelerationStructure = FindVKAccelerationStructure(pAccelerationStructureData[i]);
		if (accelerationStructure != VK_NULL_HANDLE)
		{
			accelerationStructures.push_back(accelerationStructure);
			continue;
		}

		const auto micromap = FindVKMicromap(pAccelerationStructureData[i]);
		assert(micromap != VK_NULL_HANDLE);
		micromaps.push_back(micromap);
	}
	assert(accelerationStructures.empty() || micromaps.empty());

	VkQueryType queryType = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
	VkPipelineStageFlags2 srcStage = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
	VkAccessFlags2 srcAccess = VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR;
	if (!micromaps.empty())
	{
		if (pDesc->InfoType == PostbuildInfoType::CURRENT_SIZE)
		{
			assert(!"EXT opacity micromap has no current-size query");

			return;
		}

		queryType = pDesc->InfoType == PostbuildInfoType::SERIALIZATION ?
			VK_QUERY_TYPE_MICROMAP_SERIALIZATION_SIZE_EXT : VK_QUERY_TYPE_MICROMAP_COMPACTED_SIZE_EXT;
		srcStage = VK_PIPELINE_STAGE_2_MICROMAP_BUILD_BIT_EXT;
		srcAccess = VK_ACCESS_2_MICROMAP_READ_BIT_EXT;
		assert(m_rt->m_cmdWriteMicromapsProperties);
	}
	else
	{
		queryType = GetVKAccelerationStructurePostbuildQueryType(pDesc->InfoType);
		assert(m_rt->m_cmdWriteAccelerationStructuresProperties);
	}

	const auto allocation = allocateQueries(queryType, numAccelerationStructures);
	if (allocation.Pool == VK_NULL_HANDLE) return;

	recordOutside();
	vkCmdResetQueryPool(m_state->m_commandBuffer, allocation.Pool, allocation.First, numAccelerationStructures);
	if (!micromaps.empty())
	{
		if (m_rt->m_cmdWriteMicromapsProperties)
			m_rt->m_cmdWriteMicromapsProperties(m_state->m_commandBuffer, static_cast<uint32_t>(micromaps.size()),
				micromaps.data(), queryType, allocation.Pool, allocation.First);
	}
	else if (m_rt->m_cmdWriteAccelerationStructuresProperties)
		m_rt->m_cmdWriteAccelerationStructuresProperties(m_state->m_commandBuffer, numAccelerationStructures,
			accelerationStructures.data(), queryType, allocation.Pool, allocation.First);

	VkMemoryBarrier2 barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
	barrier.srcStageMask = srcStage;
	barrier.srcAccessMask = srcAccess;
	barrier.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
	barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
	VkDependencyInfo dependency = {};
	dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependency.memoryBarrierCount = 1;
	dependency.pMemoryBarriers = &barrier;
	pipelineBarrier(dependency);

	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceSize offset = 0;
	const auto found = FindVKBuffer(pDesc->DestBuffer, buffer, offset);
	assert(found);
	if (!found) return;

	vkCmdCopyQueryPoolResults(m_state->m_commandBuffer, allocation.Pool, allocation.First, numAccelerationStructures,
		buffer, offset, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
}

void RayTracing::CommandList_VK::CopyRaytracingAccelerationStructure(AccelerationStructure* pDst,
	AccelerationStructure* pSrc, CopyMode mode, const DescriptorHeap* pDescriptorHeap)
{
	(void)pDescriptorHeap;
	assert(pDst);
	assert(pSrc);
	if (mode == CopyMode::VISUALIZATION_DECODE_FOR_TOOLS ||
		mode == CopyMode::SERIALIZE ||
		mode == CopyMode::DESERIALIZE)
	{
		assert(!"Acceleration-structure copy mode is not supported");

		return;
	}

	const auto pDstVK = AsVK(pDst);
	const auto pSrcVK = AsVK(pSrc);
	assert(pDstVK);
	assert(pSrcVK);
	if (pDstVK->GetMicromap() != VK_NULL_HANDLE)
	{
		assert(pSrcVK->GetMicromap() != VK_NULL_HANDLE);
		assert(m_rt->m_cmdCopyMicromap);
		VkCopyMicromapInfoEXT info = {};
		info.sType = VK_STRUCTURE_TYPE_COPY_MICROMAP_INFO_EXT;
		info.src = pSrcVK->GetMicromap();
		info.dst = pDstVK->GetMicromap();
		info.mode = mode == CopyMode::COMPACT ? VK_COPY_MICROMAP_MODE_COMPACT_EXT : VK_COPY_MICROMAP_MODE_CLONE_EXT;
		recordOutside();
		if (m_rt->m_cmdCopyMicromap) m_rt->m_cmdCopyMicromap(m_state->m_commandBuffer, &info);

		return;
	}

	assert(m_rt->m_cmdCopyAccelerationStructure);

	VkCopyAccelerationStructureInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR;
	info.src = pSrcVK->GetNativeHandle();
	info.dst = pDstVK->GetNativeHandle();
	info.mode = GetVKAccelerationStructureCopyMode(mode);

	recordOutside();
	m_rt->m_cmdCopyAccelerationStructure(m_state->m_commandBuffer, &info);
}

void RayTracing::CommandList_VK::SetDescriptorHeaps(uint32_t numDescriptorHeaps, const DescriptorHeap* pDescriptorHeaps)
{
	XUSG::CommandList_VK::SetDescriptorHeaps(numDescriptorHeaps, pDescriptorHeaps);
}

void RayTracing::CommandList_VK::SetTopLevelAccelerationStructure(uint32_t index, const TopLevelAS* pTopLevelAS) const
{
	SetTopLevelAccelerationStructure(index, pTopLevelAS ? pTopLevelAS->GetResourcePointer() : 0);
}

void RayTracing::CommandList_VK::SetTopLevelAccelerationStructure(uint32_t index, uint64_t topLevelASPtr) const
{
	assert(m_state->m_computeLayout);
	setRootAddress(m_state->m_computeLayout, index, topLevelASPtr);
}

void RayTracing::CommandList_VK::SetRayTracingPipeline(const Pipeline& pipeline) const
{
	const auto pPipeline = static_cast<const Pipeline_VK*>(pipeline);
	recordOutside();
	m_state->m_pipeline = pPipeline;
	if (pPipeline && pPipeline->Handle != VK_NULL_HANDLE)
		vkCmdBindPipeline(m_state->m_commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pPipeline->Handle);
}

void RayTracing::CommandList_VK::DispatchRays(uint32_t width, uint32_t height, uint32_t depth,
	const ShaderTable* pRayGen,
	const ShaderTable* pHitGroup,
	const ShaderTable* pMiss,
	const ShaderTable* pCallable) const
{
	assert(m_rt->m_cmdTraceRays);
	auto region = [](const ShaderTable* pTable, bool rayGen)
	{
		VkStridedDeviceAddressRegionKHR shader = {};
		if (!pTable) return shader;

		shader.deviceAddress = pTable->GetVirtualAddress();
		shader.size = pTable->GetByteSize();
		shader.stride = rayGen ? pTable->GetByteSize() : pTable->GetByteStride();

		return shader;
	};

	const auto rayGen = region(pRayGen, true);
	const auto miss = region(pMiss, false);
	const auto hit = region(pHitGroup, false);
	const auto callable = region(pCallable, false);

	recordOutside();
	flushPush(false);
	m_rt->m_cmdTraceRays(m_state->m_commandBuffer, &rayGen, &miss, &hit, &callable, width, height, depth);
}

RayTracing::Device* RayTracing::CommandList_VK::GetRTDevice() const
{
	return m_pDeviceRT;
}

void* RayTracing::CommandList_VK::GetRTBackend()
{
	return this;
}

bool RayTracing::CommandList_VK::createInterface()
{
	Ultimate::CommandList_VK::createInterface();

	assert(m_pDeviceRT);

	const auto pDevice = m_pDevice;
	m_rt->m_pDevice = pDevice;
	assert(pDevice);
	assert(pDevice->HasRayTracingPipeline());
	XUSG_N_RETURN(pDevice && pDevice->HasRayTracingPipeline(), false);

	const auto& ext = pDevice->GetExtProcs();
	m_rt->m_cmdBuildAccelerationStructures = ext.CmdBuildAccelerationStructures;
	m_rt->m_cmdCopyAccelerationStructure = ext.CmdCopyAccelerationStructure;
	m_rt->m_cmdWriteAccelerationStructuresProperties = ext.CmdWriteAccelerationStructuresProperties;
	m_rt->m_cmdBuildMicromaps = ext.CmdBuildMicromaps;
	m_rt->m_cmdCopyMicromap = ext.CmdCopyMicromap;
	m_rt->m_cmdWriteMicromapsProperties = ext.CmdWriteMicromapsProperties;
	m_rt->m_cmdTraceRays = ext.CmdTraceRays;
	XUSG_N_RETURN(m_rt->m_cmdBuildAccelerationStructures && m_rt->m_cmdCopyAccelerationStructure &&
		m_rt->m_cmdWriteAccelerationStructuresProperties && m_rt->m_cmdTraceRays, false);

	return true;
}

RayTracing::CommandList_VK::QueryAllocation RayTracing::CommandList_VK::allocateQueries(VkQueryType type, uint32_t count) const
{
	for (auto& pool : m_rt->m_queryPools)
	{
		if (pool.Type != type || pool.Pool == VK_NULL_HANDLE) continue;
		if (pool.Count - pool.Used < count) continue;

		QueryAllocation allocation = { pool.Pool, pool.Used };
		pool.Used += count;

		return allocation;
	}

	const auto pDevice = AsVK(m_pDevice);
	assert(pDevice);
	QueryPool created = {};
	created.Type = type;
	created.Count = count;
	created.Used = count;
	created.Pool = VK_NULL_HANDLE;

	VkQueryPoolCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	info.queryType = type;
	info.queryCount = count;
	const auto result = vkCreateQueryPool(pDevice->GetDevice(), &info, nullptr, &created.Pool);
	if (result < 0)
	{
		cerr << "VkResult " << result << endl;
		assert(!"Failed to create an acceleration-structure query pool");

		return { VK_NULL_HANDLE, 0 };
	}

	m_rt->m_queryPools.push_back(created);

	return { created.Pool, 0 };
}

VkQueryType XUSG::RayTracing::GetVKAccelerationStructurePostbuildQueryType(PostbuildInfoType type)
{
	static const VkQueryType queryTypes[] =
	{
		VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR,
		VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR,
		VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SERIALIZATION_SIZE_KHR,
		VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SIZE_KHR
	};

	return queryTypes[static_cast<uint32_t>(type)];
}

VkCopyAccelerationStructureModeKHR XUSG::RayTracing::GetVKAccelerationStructureCopyMode(CopyMode mode)
{
	static const VkCopyAccelerationStructureModeKHR copyModes[] =
	{
		VK_COPY_ACCELERATION_STRUCTURE_MODE_CLONE_KHR,
		VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR,
		VK_COPY_ACCELERATION_STRUCTURE_MODE_CLONE_KHR,
		VK_COPY_ACCELERATION_STRUCTURE_MODE_SERIALIZE_KHR,
		VK_COPY_ACCELERATION_STRUCTURE_MODE_DESERIALIZE_KHR
	};

	return copyModes[static_cast<uint32_t>(mode)];
}
