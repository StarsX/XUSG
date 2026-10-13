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

RayTracing::CommandList_VK::CommandList_VK() :
	Ultimate::CommandList_VK(),
	m_cmdBuildAccelerationStructures(nullptr),
	m_cmdCopyAccelerationStructure(nullptr),
	m_cmdWriteAccelerationStructuresProperties(nullptr),
	m_cmdTraceRays(nullptr),
	m_pDeviceRT(nullptr)
{
}

RayTracing::CommandList_VK::~CommandList_VK()
{
	if (!m_pDevice) return;

	const auto pDevice = AsVK(m_pDevice);
	if (!pDevice) return;

	const auto device = pDevice->GetDevice();
	for (auto& pool : m_queryPools)
		if (pool.Pool != VK_NULL_HANDLE) vkDestroyQueryPool(device, pool.Pool, nullptr);
}

RayTracing::CommandList_VK::CommandList_VK(XUSG::CommandList* pCommandList, RayTracing::Device* pDevice) :
	CommandList_VK()
{
	const auto p = AsVK(pCommandList);
	assert(p);

	Attach(*p);
	m_pDevice = AsVK(pDevice);
	CreateInterface();
}

bool RayTracing::CommandList_VK::CreateInterface()
{
	Ultimate::CommandList_VK::CreateInterface();

	m_pDeviceRT = dynamic_cast<RayTracing::Device*>(m_pDevice);
	assert(m_pDeviceRT);

	const auto pDevice = AsVK(m_pDevice);
	assert(pDevice);
	assert(pDevice->HasRayTracingPipeline());
	XUSG_N_RETURN(pDevice && pDevice->HasRayTracingPipeline(), false);

	const auto& ext = pDevice->GetExtProcs();
	m_cmdBuildAccelerationStructures = ext.CmdBuildAccelerationStructures;
	m_cmdCopyAccelerationStructure = ext.CmdCopyAccelerationStructure;
	m_cmdWriteAccelerationStructuresProperties = ext.CmdWriteAccelerationStructuresProperties;
	m_cmdTraceRays = ext.CmdTraceRays;
	XUSG_N_RETURN(m_cmdBuildAccelerationStructures && m_cmdCopyAccelerationStructure &&
		m_cmdWriteAccelerationStructuresProperties && m_cmdTraceRays, false);

	return true;
}

bool RayTracing::CommandList_VK::Reset(CommandAllocator* pAllocator, const Pipeline& initialState) const
{
	for (auto& pool : m_queryPools) pool.Used = 0;

	return XUSG::CommandList_VK::Reset(pAllocator, initialState);
}

RayTracing::CommandList_VK::QueryAllocation RayTracing::CommandList_VK::allocateQueries(VkQueryType type, uint32_t count) const
{
	for (auto& pool : m_queryPools)
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

	m_queryPools.push_back(created);

	return { created.Pool, 0 };
}

void RayTracing::CommandList_VK::BuildRaytracingAccelerationStructure(const BuildDesc* pDesc, uint32_t numPostbuildInfoDescs,
	const PostbuildInfo* pPostbuildInfoDescs, const DescriptorHeap* pDescriptorHeap)
{
	(void)pDescriptorHeap;
	assert(pDesc);
	assert(numPostbuildInfoDescs == 0 || pPostbuildInfoDescs);
	assert(m_cmdBuildAccelerationStructures);

	recordOutside();
	const auto pBuild = static_cast<const BuildDesc_VK*>(pDesc);
	const VkAccelerationStructureBuildRangeInfoKHR* pRanges = pBuild->Ranges.empty() ? nullptr : pBuild->Ranges.data();
	if (pBuild->Type == VK_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_KHR)
	{
		pRanges = nullptr;
		m_cmdBuildAccelerationStructures(m_commandBuffer, 1, &pBuild->Info, &pRanges);
	}
	else m_cmdBuildAccelerationStructures(m_commandBuffer, 1, &pBuild->Info, pRanges ? &pRanges : nullptr);

	if (!numPostbuildInfoDescs) return;

	const uint64_t accelerationStructure = reinterpret_cast<uint64_t>(pBuild->AccelerationStructure);
	for (auto i = 0u; i < numPostbuildInfoDescs; ++i)
		EmitRaytracingAccelerationStructurePostbuildInfo(&pPostbuildInfoDescs[i], 1, &accelerationStructure);
}

void RayTracing::CommandList_VK::EmitRaytracingAccelerationStructurePostbuildInfo(const PostbuildInfo* pDesc,
	uint32_t numAccelerationStructures, const uint64_t* pAccelerationStructureData) const
{
	assert(pDesc);
	assert(numAccelerationStructures == 0 || pAccelerationStructureData);
	assert(m_cmdWriteAccelerationStructuresProperties);
	if (!numAccelerationStructures) return;

	if (pDesc->InfoType == PostbuildInfoType::TOOLS_VISUALIZATION)
	{
		assert(!"Visualization postbuild info is not implemented");

		return;
	}

	const auto queryType = GetVKAccelerationStructurePostbuildQueryType(pDesc->InfoType);
	const auto allocation = allocateQueries(queryType, numAccelerationStructures);
	if (allocation.Pool == VK_NULL_HANDLE) return;

	vector<VkAccelerationStructureKHR> accelerationStructures(numAccelerationStructures);
	for (auto i = 0u; i < numAccelerationStructures; ++i)
	{
		accelerationStructures[i] = FindVKAccelerationStructure(pAccelerationStructureData[i]);
		assert(accelerationStructures[i] != VK_NULL_HANDLE);
	}

	recordOutside();
	vkCmdResetQueryPool(m_commandBuffer, allocation.Pool, allocation.First, numAccelerationStructures);
	m_cmdWriteAccelerationStructuresProperties(m_commandBuffer, numAccelerationStructures,
		accelerationStructures.data(), queryType, allocation.Pool, allocation.First);

	VkMemoryBarrier2 barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
	barrier.srcStageMask = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
	barrier.srcAccessMask = VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR;
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

	vkCmdCopyQueryPoolResults(m_commandBuffer, allocation.Pool, allocation.First, numAccelerationStructures,
		buffer, offset, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
}

void RayTracing::CommandList_VK::CopyRaytracingAccelerationStructure(const AccelerationStructure* pDst,
	const AccelerationStructure* pSrc, CopyMode mode, const DescriptorHeap* pDescriptorHeap)
{
	(void)pDescriptorHeap;
	assert(pDst);
	assert(pSrc);
	assert(m_cmdCopyAccelerationStructure);
	// SERIALIZE/DESERIALIZE need vkCmdCopyAccelerationStructureToMemoryKHR /
	// vkCmdCopyMemoryToAccelerationStructureKHR, not vkCmdCopyAccelerationStructureKHR.
	if (mode == CopyMode::VISUALIZATION_DECODE_FOR_TOOLS ||
		mode == CopyMode::SERIALIZE ||
		mode == CopyMode::DESERIALIZE)
	{
		assert(!"Acceleration-structure copy mode is not implemented");

		return;
	}

	const auto pDstVK = dynamic_cast<const AccelerationStructure_VK*>(pDst);
	const auto pSrcVK = dynamic_cast<const AccelerationStructure_VK*>(pSrc);
	assert(pDstVK);
	assert(pSrcVK);

	VkCopyAccelerationStructureInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR;
	info.src = pSrcVK->GetHandle();
	info.dst = pDstVK->GetHandle();
	info.mode = GetVKAccelerationStructureCopyMode(mode);

	recordOutside();
	m_cmdCopyAccelerationStructure(m_commandBuffer, &info);
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
	assert(m_computeLayout);
	setRootAddress(m_computeLayout, index, topLevelASPtr);
}

void RayTracing::CommandList_VK::SetRayTracingPipeline(const Pipeline& pipeline) const
{
	const auto pPipeline = static_cast<const Pipeline_VK*>(pipeline);
	recordOutside();
	m_pipeline = pPipeline;
	if (pPipeline && pPipeline->Handle != VK_NULL_HANDLE)
		vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pPipeline->Handle);
}

void RayTracing::CommandList_VK::DispatchRays(uint32_t width, uint32_t height, uint32_t depth,
	const ShaderTable* pRayGen,
	const ShaderTable* pHitGroup,
	const ShaderTable* pMiss,
	const ShaderTable* pCallable) const
{
	assert(m_cmdTraceRays);
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
	m_cmdTraceRays(m_commandBuffer, &rayGen, &miss, &hit, &callable, width, height, depth);
}

RayTracing::Device* RayTracing::CommandList_VK::GetRTDevice() const
{
	return m_pDeviceRT;
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
