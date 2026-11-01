//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSGEnum_VK.h"
#include "XUSGUltimate_VK.h"

using namespace std;
using namespace XUSG::Ultimate;

namespace
{
	void AddMeshStages(const XUSG::Device_VK* pDevice, VkPipelineStageFlags2& stage)
	{
		if (pDevice && pDevice->HasMeshShader())
		{
			stage |= VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT;
			if (pDevice->HasTaskShader()) stage |= VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_EXT;
		}
	}

	void AddRayStage(const XUSG::Device_VK* pDevice, VkPipelineStageFlags2& stage, bool required)
	{
		if (required) assert(pDevice && pDevice->HasRayTracingPipeline());
		if (required || (pDevice && pDevice->HasRayTracingPipeline()))
			stage |= VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;
	}

	void AddAccelerationStructureStage(const XUSG::Device_VK* pDevice, VkPipelineStageFlags2& stage)
	{
		assert(pDevice && pDevice->HasAccelerationStructure());
		stage |= VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
	}

	VkPipelineStageFlags2 BarrierStage(const XUSG::Device_VK* pDevice, BarrierSync sync)
	{
		VkPipelineStageFlags2 stage = VK_PIPELINE_STAGE_2_NONE;
		if ((sync & BarrierSync::ALL) == BarrierSync::ALL) return VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		if ((sync & BarrierSync::DRAW) == BarrierSync::DRAW)
		{
			stage |= VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
				VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
				VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			AddMeshStages(pDevice, stage);
		}
		if ((sync & BarrierSync::INDEX_INPUT) == BarrierSync::INDEX_INPUT) stage |= VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT;
		if ((sync & BarrierSync::VERTEX_SHADING) == BarrierSync::VERTEX_SHADING) stage |= VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
		if ((sync & BarrierSync::PIXEL_SHADING) == BarrierSync::PIXEL_SHADING) stage |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		if ((sync & BarrierSync::DEPTH_STENCIL) == BarrierSync::DEPTH_STENCIL)
			stage |= VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
		if ((sync & BarrierSync::RENDER_TARGET) == BarrierSync::RENDER_TARGET) stage |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		if ((sync & BarrierSync::COMPUTE_SHADING) == BarrierSync::COMPUTE_SHADING) stage |= VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
		if ((sync & BarrierSync::RAYTRACING) == BarrierSync::RAYTRACING)
			AddRayStage(pDevice, stage, true);
		if ((sync & BarrierSync::BUILD_RAYTRACING_ACCELERATION_STRUCTURE) == BarrierSync::BUILD_RAYTRACING_ACCELERATION_STRUCTURE ||
			(sync & BarrierSync::COPY_RAYTRACING_ACCELERATION_STRUCTURE) == BarrierSync::COPY_RAYTRACING_ACCELERATION_STRUCTURE ||
			(sync & BarrierSync::EMIT_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO) == BarrierSync::EMIT_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO)
			AddAccelerationStructureStage(pDevice, stage);
		if ((sync & BarrierSync::COPY) == BarrierSync::COPY) stage |= VK_PIPELINE_STAGE_2_COPY_BIT;
		if ((sync & BarrierSync::CONVERT_LINEAR_ALGEBRA_MATRIX) == BarrierSync::CONVERT_LINEAR_ALGEBRA_MATRIX)
			stage |= VK_PIPELINE_STAGE_2_COPY_BIT;
		if ((sync & BarrierSync::RESOLVE) == BarrierSync::RESOLVE) stage |= VK_PIPELINE_STAGE_2_RESOLVE_BIT;
		if ((sync & BarrierSync::EXECUTE_INDIRECT) == BarrierSync::EXECUTE_INDIRECT) stage |= VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
		if ((sync & BarrierSync::ALL_SHADING) == BarrierSync::ALL_SHADING)
		{
			stage |= VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
			AddMeshStages(pDevice, stage);
			AddRayStage(pDevice, stage, false);
		}
		if ((sync & BarrierSync::NON_PIXEL_SHADING) == BarrierSync::NON_PIXEL_SHADING)
		{
			stage |= VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT |
				VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT | VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT |
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
			AddMeshStages(pDevice, stage);
			AddRayStage(pDevice, stage, false);
		}
		if ((sync & BarrierSync::CLEAR_UNORDERED_ACCESS_VIEW) == BarrierSync::CLEAR_UNORDERED_ACCESS_VIEW)
			stage |= VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;

		return stage ? stage : VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	}

	VkAccessFlags2 BarrierAccessFlags(const XUSG::Device_VK* pDevice, BarrierAccess access)
	{
		VkAccessFlags2 flags = VK_ACCESS_2_NONE;
		if (access == BarrierAccess::COMMON) return VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
		if ((access & BarrierAccess::VERTEX_BUFFER) == BarrierAccess::VERTEX_BUFFER) flags |= VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
		if ((access & BarrierAccess::CONSTANT_BUFFER) == BarrierAccess::CONSTANT_BUFFER) flags |= VK_ACCESS_2_UNIFORM_READ_BIT;
		if ((access & BarrierAccess::INDEX_BUFFER) == BarrierAccess::INDEX_BUFFER) flags |= VK_ACCESS_2_INDEX_READ_BIT;
		if ((access & BarrierAccess::RENDER_TARGET) == BarrierAccess::RENDER_TARGET)
			flags |= VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		if ((access & BarrierAccess::UNORDERED_ACCESS) == BarrierAccess::UNORDERED_ACCESS)
			flags |= VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
		if ((access & BarrierAccess::DEPTH_STENCIL_WRITE) == BarrierAccess::DEPTH_STENCIL_WRITE)
			flags |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		if ((access & BarrierAccess::DEPTH_STENCIL_READ) == BarrierAccess::DEPTH_STENCIL_READ)
			flags |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		if ((access & BarrierAccess::SHADER_RESOURCE) == BarrierAccess::SHADER_RESOURCE)
			flags |= VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
		if ((access & BarrierAccess::INDIRECT_ARGUMENT) == BarrierAccess::INDIRECT_ARGUMENT) flags |= VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
		if ((access & BarrierAccess::COPY_SOURCE) == BarrierAccess::COPY_SOURCE) flags |= VK_ACCESS_2_TRANSFER_READ_BIT;
		if ((access & BarrierAccess::COPY_DEST) == BarrierAccess::COPY_DEST) flags |= VK_ACCESS_2_TRANSFER_WRITE_BIT;
		if ((access & BarrierAccess::RESOLVE_SOURCE) == BarrierAccess::RESOLVE_SOURCE) flags |= VK_ACCESS_2_TRANSFER_READ_BIT;
		if ((access & BarrierAccess::RESOLVE_DEST) == BarrierAccess::RESOLVE_DEST) flags |= VK_ACCESS_2_TRANSFER_WRITE_BIT;
		if ((access & BarrierAccess::SHADING_RATE_SOURCE) == BarrierAccess::SHADING_RATE_SOURCE)
			flags |= VK_ACCESS_2_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;
		if ((access & BarrierAccess::RAYTRACING_ACCELERATION_STRUCTURE_READ) == BarrierAccess::RAYTRACING_ACCELERATION_STRUCTURE_READ)
		{
			assert(pDevice && pDevice->HasAccelerationStructure());
			flags |= VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR;
		}
		if ((access & BarrierAccess::RAYTRACING_ACCELERATION_STRUCTURE_WRITE) == BarrierAccess::RAYTRACING_ACCELERATION_STRUCTURE_WRITE)
		{
			assert(pDevice && pDevice->HasAccelerationStructure());
			flags |= VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
		}
		if ((access & BarrierAccess::NO_ACCESS) == BarrierAccess::NO_ACCESS) return VK_ACCESS_2_NONE;

		return flags;
	}

	VkFragmentShadingRateCombinerOpKHR Combiner(ShadingRateCombiner combiner)
	{
		static const VkFragmentShadingRateCombinerOpKHR combiners[] =
		{
			VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR,
			VK_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE_KHR,
			VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MIN_KHR,
			VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MAX_KHR,
			VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MUL_KHR,
		};

		assert(combiners[static_cast<uint32_t>(ShadingRateCombiner::COMBINER_PASSTHROUGH)] == VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR);
		assert(combiners[static_cast<uint32_t>(ShadingRateCombiner::COMBINER_OVERRIDE)] == VK_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE_KHR);
		assert(combiners[static_cast<uint32_t>(ShadingRateCombiner::COMBINER_MIN)] == VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MIN_KHR);
		assert(combiners[static_cast<uint32_t>(ShadingRateCombiner::COMBINER_MAX)] == VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MAX_KHR);
		assert(combiners[static_cast<uint32_t>(ShadingRateCombiner::COMBINER_SUM)] == VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MUL_KHR);

		return combiners[static_cast<uint32_t>(combiner)];
	}

	bool IsBufferBarrier(const ResourceBarrier& barrier)
	{
		return barrier.pResource && barrier.LayoutBefore == XUSG::BarrierLayout::UNDEFINED &&
			barrier.LayoutAfter == XUSG::BarrierLayout::UNDEFINED;
	}

	void SubresourceRange(const ResourceBarrier& barrier, const XUSG::Resource_VK* pResource, VkImageSubresourceRange& range)
	{
		const auto all = barrier.IndexOrFirstMipLevel == XUSG_BARRIER_ALL_SUBRESOURCES;
		range.aspectMask = XUSG::GetVKImageAspect(XUSG::GetFormatFromVK(pResource->GetVkFormat()));
		range.baseMipLevel = all ? 0 : barrier.IndexOrFirstMipLevel;
		range.levelCount = all || !barrier.NumMipLevels ? VK_REMAINING_MIP_LEVELS : barrier.NumMipLevels;
		range.baseArrayLayer = all ? 0 : barrier.FirstArraySlice;
		range.layerCount = all || !barrier.NumArraySlices ? VK_REMAINING_ARRAY_LAYERS : barrier.NumArraySlices;
	}

	bool RangeOverlaps(uint32_t baseA, uint32_t countA, uint32_t baseB, uint32_t countB)
	{
		const auto endA = countA == VK_REMAINING_MIP_LEVELS ? UINT32_MAX : baseA + countA;
		const auto endB = countB == VK_REMAINING_MIP_LEVELS ? UINT32_MAX : baseB + countB;

		return baseA <= baseB ? endA > baseB : endB > baseA;
	}

	bool ImageBarrierOverlaps(const VkImageMemoryBarrier2& a, const VkImageMemoryBarrier2& b)
	{
		if (a.image != b.image) return false;
		if ((a.subresourceRange.aspectMask & b.subresourceRange.aspectMask) == 0) return false;

		const auto& left = a.subresourceRange;
		const auto& right = b.subresourceRange;

		return RangeOverlaps(left.baseMipLevel, left.levelCount, right.baseMipLevel, right.levelCount) &&
			RangeOverlaps(left.baseArrayLayer, left.layerCount, right.baseArrayLayer, right.layerCount);
	}

	VkImageAspectFlags DepthStencilAspect(VkFormat format)
	{
		switch (format)
		{
		case VK_FORMAT_D16_UNORM:
		case VK_FORMAT_D32_SFLOAT:
		case VK_FORMAT_X8_D24_UNORM_PACK32:
			return VK_IMAGE_ASPECT_DEPTH_BIT;
		case VK_FORMAT_S8_UINT:
			return VK_IMAGE_ASPECT_STENCIL_BIT;
		case VK_FORMAT_D16_UNORM_S8_UINT:
		case VK_FORMAT_D24_UNORM_S8_UINT:
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
			return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		default:
			return VK_IMAGE_ASPECT_COLOR_BIT;
		}
	}

	void FillBarrierDependency(VkDependencyInfo& info, const VkMemoryBarrier2& memory,
		const vector<VkBufferMemoryBarrier2>& buffers, const vector<VkImageMemoryBarrier2>& images)
	{
		info = {};
		info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		if (memory.srcStageMask || memory.dstStageMask)
		{
			info.memoryBarrierCount = 1;
			info.pMemoryBarriers = &memory;
		}
		info.bufferMemoryBarrierCount = static_cast<uint32_t>(buffers.size());
		info.pBufferMemoryBarriers = buffers.empty() ? nullptr : buffers.data();
		info.imageMemoryBarrierCount = static_cast<uint32_t>(images.size());
		info.pImageMemoryBarriers = images.empty() ? nullptr : images.data();
	}
}

//--------------------------------------------------------------------------------------
// Command list
//--------------------------------------------------------------------------------------

CommandList_VK::UltimateState::~UltimateState()
{
	const auto device = m_pDevice ? m_pDevice->GetDevice() : VK_NULL_HANDLE;
	if (m_shadingRate.View != VK_NULL_HANDLE) vkDestroyImageView(device, m_shadingRate.View, nullptr);
	if (m_resolveSrc.View != VK_NULL_HANDLE) vkDestroyImageView(device, m_resolveSrc.View, nullptr);
	if (m_resolveDst.View != VK_NULL_HANDLE) vkDestroyImageView(device, m_resolveDst.View, nullptr);
}

CommandList_VK::CommandList_VK() :
	XUSG::CommandList_VK(),
	m_ultimate(make_shared<UltimateState>())
{
}

CommandList_VK::CommandList_VK(XUSG::CommandList& commandList) :
	CommandList_VK()
{
	const auto pCommandList = AsVK(&commandList);
	assert(pCommandList);
	Attach(*pCommandList);
	if (const auto pUltimate = static_cast<CommandList_VK*>(pCommandList->GetUltimateBackend()))
		AttachUltimate(*pUltimate);
	createInterface();
}

CommandList_VK::~CommandList_VK()
{
}

bool CommandList_VK::Create(Device* pDevice, uint32_t nodeMask, CommandListType type,
	uint8_t numAllocators, CommandAllocator* const* ppAllocators, const Pipeline& pipeline, const wchar_t* name)
{
	if (m_ultimate.use_count() > 1) m_ultimate = make_shared<UltimateState>();
	XUSG_N_RETURN(XUSG::CommandList_VK::Create(pDevice, nodeMask, type, numAllocators, ppAllocators, pipeline, name), false);

	return createInterface();
}

void CommandList_VK::AttachUltimate(const CommandList_VK& source)
{
	if (this == &source) return;

	m_ultimate = source.m_ultimate;
}

void* CommandList_VK::GetUltimateBackend()
{
	return this;
}

void CommandList_VK::prepareFenceBarrierGroups(uint32_t numBarrierGroups, BarrierGroup* pBarrierGroups,
	VkMemoryBarrier2& memory) const
{
	memory = {};
	memory.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
	m_state->m_buffers.clear();
	m_state->m_images.clear();
	if (!numBarrierGroups) return;
	assert(pBarrierGroups);

	auto count = 0u;
	for (auto i = 0u; i < numBarrierGroups; ++i) count += pBarrierGroups[i].NumBarriers;
	if (!count) return;

	if (m_state->m_renderingBegun)
	{
		auto touch = false;
		for (auto i = 0u; i < numBarrierGroups && !touch; ++i)
		{
			const auto& group = pBarrierGroups[i];
			if (!group.NumBarriers) continue;
			assert(group.pBarriers);
			touch = ultimateBarriersTouchBoundAttachment(group.NumBarriers, group.pBarriers);
		}
		if (touch) endRendering();
	}
	if (!m_state->m_renderingBegun)
	{
		flushDeferredClears();
		endPredication();
	}

	if (m_state->m_buffers.capacity() < count) m_state->m_buffers.reserve(count);
	if (m_state->m_images.capacity() < count) m_state->m_images.reserve(count);
	for (auto i = 0u; i < numBarrierGroups; ++i)
	{
		const auto& group = pBarrierGroups[i];
		if (!group.NumBarriers) continue;
		assert(group.pBarriers);
		for (auto j = 0u; j < group.NumBarriers; ++j) addBarrier(group.pBarriers[j], memory);
	}
}

void CommandList_VK::Barrier(uint32_t numBarrierGroups, BarrierGroup* pBarrierGroups)
{
	VkMemoryBarrier2 memory = {};
	prepareFenceBarrierGroups(numBarrierGroups, pBarrierGroups, memory);
	flushBarriers(memory);
}

void CommandList_VK::SignalBarrier(uint32_t numBarrierGroups, BarrierGroup* pBarrierGroups,
	Fence* pFence, uint64_t fenceValue)
{
	(void)fenceValue;
	assert(pFence);
	const auto pFenceVK = AsVK(pFence);
	assert(pFenceVK);
	const auto event = pFenceVK->GetEvent();
	assert(event != VK_NULL_HANDLE && "Vulkan SignalBarrier requires ALLOW_COMMAND_LIST_BARRIERS without ALLOW_QUEUE_AND_CPU_SYNC");

	VkMemoryBarrier2 memory = {};
	prepareFenceBarrierGroups(numBarrierGroups, pBarrierGroups, memory);

	VkDependencyInfo dependency = {};
	FillBarrierDependency(dependency, memory, m_state->m_buffers, m_state->m_images);
	vkCmdResetEvent2(m_state->m_commandBuffer, event, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT);
	vkCmdSetEvent2(m_state->m_commandBuffer, event, &dependency);
	memory.srcStageMask = 0;
	memory.srcAccessMask = 0;
	memory.dstStageMask = 0;
	memory.dstAccessMask = 0;
	m_state->m_buffers.clear();
	m_state->m_images.clear();
}

void CommandList_VK::WaitBarrier(uint32_t numBarrierGroups, BarrierGroup* pBarrierGroups,
	uint32_t numFences, Fence* const* ppFences, const uint64_t* pFenceValues)
{
	(void)pFenceValues;
	assert(numFences);
	assert(ppFences);

	vector<VkEvent> events(numFences);
	for (auto i = 0u; i < numFences; ++i)
	{
		assert(ppFences[i]);
		const auto pFenceVK = AsVK(ppFences[i]);
		assert(pFenceVK);
		events[i] = pFenceVK->GetEvent();
		assert(events[i] != VK_NULL_HANDLE && "Vulkan WaitBarrier requires command-list-scoped fences (VkEvent)");
	}

	VkMemoryBarrier2 memory = {};
	prepareFenceBarrierGroups(numBarrierGroups, pBarrierGroups, memory);

	VkDependencyInfo dependency = {};
	FillBarrierDependency(dependency, memory, m_state->m_buffers, m_state->m_images);
	vector<VkDependencyInfo> dependencies(numFences, dependency);

	// Barriers apply once after all events; empty infos on earlier slots avoid repeating them.
	for (auto i = 0u; i + 1 < numFences; ++i)
	{
		dependencies[i] = {};
		dependencies[i].sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	}

	vkCmdWaitEvents2(m_state->m_commandBuffer, numFences, events.data(), dependencies.data());
	memory.srcStageMask = 0;
	memory.srcAccessMask = 0;
	memory.dstStageMask = 0;
	memory.dstAccessMask = 0;
	m_state->m_buffers.clear();
	m_state->m_images.clear();
}

void CommandList_VK::Barrier(uint32_t numBufferBarriers, ResourceBarrier* pBufferBarriers,
	uint32_t numTextureBarriers, ResourceBarrier* pTextureBarriers,
	uint32_t numGlobalBarriers, ResourceBarrier* pGlobalBarriers)
{
	if (!numBufferBarriers && !numTextureBarriers && !numGlobalBarriers) return;

	if (numBufferBarriers) assert(pBufferBarriers);
	if (numTextureBarriers) assert(pTextureBarriers);
	if (numGlobalBarriers) assert(pGlobalBarriers);

	// Only texture barriers can reference bound color/depth attachments.
	prepareUltimateBarriersOutsidePass(numTextureBarriers, pTextureBarriers);

	VkMemoryBarrier2 memory = {};
	memory.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
	m_state->m_buffers.clear();
	m_state->m_images.clear();
	if (m_state->m_buffers.capacity() < numBufferBarriers) m_state->m_buffers.reserve(numBufferBarriers);
	if (m_state->m_images.capacity() < numTextureBarriers) m_state->m_images.reserve(numTextureBarriers);
	for (auto i = 0u; i < numBufferBarriers; ++i) addBarrier(pBufferBarriers[i], memory);
	for (auto i = 0u; i < numTextureBarriers; ++i) addBarrier(pTextureBarriers[i], memory);
	for (auto i = 0u; i < numGlobalBarriers; ++i) addBarrier(pGlobalBarriers[i], memory);
	flushBarriers(memory);
}

void CommandList_VK::SetSamplePositions(uint8_t numSamplesPerPixel, uint8_t numPixels, SamplePosition* pPositions) const
{
	assert(
		numSamplesPerPixel == 0 ||
		numSamplesPerPixel == 1 ||
		numSamplesPerPixel == 2 ||
		numSamplesPerPixel == 4 ||
		numSamplesPerPixel == 8 ||
		numSamplesPerPixel == 16);
	assert(numPixels == 0 || numPixels == 1 || numPixels == 4);

	const auto numSamples = static_cast<uint32_t>(numSamplesPerPixel) * numPixels;
	assert(numSamples == 0 || pPositions);
	assert(numSamples <= 16);
	const auto sampleLocations = m_pDevice && m_pDevice->HasSampleLocations();
	assert(sampleLocations);
	if (!sampleLocations) return;

	const auto variableSampleLocations = m_pDevice->VariableSampleLocations();
	const auto restart = m_state->m_renderingBegun && !variableSampleLocations;
	if (restart) endRendering();

	const auto& ext = m_pDevice->GetExtProcs();
	if (!numSamplesPerPixel || !numPixels)
	{
		ext.SetSampleLocationsEnable(m_state->m_commandBuffer, VK_FALSE);
		if (restart) beginRendering();

		return;
	}

	uint32_t deviceWidth = 0;
	uint32_t deviceHeight = 0;
	const auto sampleLocationGrid = m_pDevice->SampleLocationGrid(numSamplesPerPixel, deviceWidth, deviceHeight);
	assert(sampleLocationGrid);
	if (!sampleLocationGrid)
	{
		if (restart) beginRendering();

		return;
	}

	const auto appWidth = numPixels == 4 ? 2u : 1u;
	const auto appHeight = numPixels == 4 ? 2u : 1u;
	auto gridWidth = appWidth;
	auto gridHeight = appHeight;
	if (!variableSampleLocations)
	{
		assert(deviceWidth % appWidth == 0 && deviceHeight % appHeight == 0);
		if (deviceWidth % appWidth != 0 || deviceHeight % appHeight != 0)
		{
			if (restart) beginRendering();

			return;
		}
		gridWidth = deviceWidth;
		gridHeight = deviceHeight;
	}
	else
	{
		assert(appWidth <= deviceWidth && appHeight <= deviceHeight);
		if (appWidth > deviceWidth || appHeight > deviceHeight)
		{
			if (restart) beginRendering();

			return;
		}
	}

	const auto locationCount = numSamplesPerPixel * gridWidth * gridHeight;
	assert(locationCount <= 64);
	if (locationCount > 64)
	{
		if (restart) beginRendering();

		return;
	}

	float rangeMin = 0.0f;
	float rangeMax = 1.0f;
	auto subPixelBits = 4u;
	m_pDevice->SampleLocationRange(rangeMin, rangeMax, subPixelBits);
	if (subPixelBits > 16) subPixelBits = 16;
	const auto scale = static_cast<float>(1u << subPixelBits);

	VkSampleLocationEXT locations[64];
	for (auto y = 0u; y < gridHeight; ++y)
	{
		for (auto x = 0u; x < gridWidth; ++x)
		{
			const auto srcPixel = (y % appHeight) * appWidth + (x % appWidth);
			for (auto sample = 0u; sample < numSamplesPerPixel; ++sample)
			{
				const auto& position = pPositions[srcPixel * numSamplesPerPixel + sample];
				auto px = static_cast<float>(position.X) / 16.0f + 0.5f;
				auto py = static_cast<float>(position.Y) / 16.0f + 0.5f;
				if (px < rangeMin) px = rangeMin;
				if (px > rangeMax) px = rangeMax;
				if (py < rangeMin) py = rangeMin;
				if (py > rangeMax) py = rangeMax;
				px = static_cast<float>(static_cast<int>(px * scale + 0.5f)) / scale;
				py = static_cast<float>(static_cast<int>(py * scale + 0.5f)) / scale;
				if (px < rangeMin) px = rangeMin;
				if (px > rangeMax) px = rangeMax;
				if (py < rangeMin) py = rangeMin;
				if (py > rangeMax) py = rangeMax;

				const auto dst = (y * gridWidth + x) * numSamplesPerPixel + sample;
				locations[dst].x = px;
				locations[dst].y = py;
			}
		}
	}

	VkSampleLocationsInfoEXT info = {};
	info.sType = VK_STRUCTURE_TYPE_SAMPLE_LOCATIONS_INFO_EXT;
	info.sampleLocationsPerPixel = static_cast<VkSampleCountFlagBits>(numSamplesPerPixel);
	info.sampleLocationGridSize = { gridWidth, gridHeight };
	info.sampleLocationsCount = locationCount;
	info.pSampleLocations = locations;
	ext.SetSampleLocationsEnable(m_state->m_commandBuffer, VK_TRUE);
	ext.SetSampleLocations(m_state->m_commandBuffer, &info);
	if (restart) beginRendering();
}

void CommandList_VK::ResolveSubresourceRegion(Resource* pDstResource, uint32_t dstSubresource,
	uint32_t dstX, uint32_t dstY, Resource* pSrcResource, uint32_t srcSubresource,
	const RectRange& srcRect, Format format, ResolveMode resolveMode) const
{
	if (resolveMode == ResolveMode::MIN || resolveMode == ResolveMode::MAX)
	{
		assert(pDstResource);
		assert(pSrcResource);
		endRendering();
		flushDeferredClears();
		resolveMinMax(pDstResource, dstSubresource, dstX, dstY,
			pSrcResource, srcSubresource, srcRect, resolveMode);

		return;
	}
	if (resolveMode != ResolveMode::AVERAGE && resolveMode != ResolveMode::DECOMPRESS)
	{
		assert(!"Resolve mode is not implemented on Vulkan");

		return;
	}
	assert(pDstResource);
	assert(pSrcResource);
	endRendering();
	flushDeferredClears();

	const auto pSrc = AsVK(pSrcResource);
	const auto pDst = AsVK(pDstResource);
	assert(pSrc);
	assert(pDst);
	const auto srcMips = (max)(pSrc->GetMipLevels(), 1u);
	const auto dstMips = (max)(pDst->GetMipLevels(), 1u);
	VkImageResolve region = {};
	region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.srcSubresource.mipLevel = srcSubresource % srcMips;
	region.srcSubresource.baseArrayLayer = pSrc->Is3D() ? 0 : srcSubresource / srcMips;
	region.srcSubresource.layerCount = 1;
	region.srcOffset = { srcRect.Left, srcRect.Top, 0 };
	region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.dstSubresource.mipLevel = dstSubresource % dstMips;
	region.dstSubresource.baseArrayLayer = pDst->Is3D() ? 0 : dstSubresource / dstMips;
	region.dstSubresource.layerCount = 1;
	region.dstOffset = { static_cast<int32_t>(dstX), static_cast<int32_t>(dstY), 0 };
	region.extent.width = static_cast<uint32_t>((max)(srcRect.Right - srcRect.Left, 0l));
	region.extent.height = static_cast<uint32_t>((max)(srcRect.Bottom - srcRect.Top, 0l));
	region.extent.depth = 1;
	if (!region.extent.width || !region.extent.height)
	{
		region.extent.width = static_cast<uint32_t>((max)(pSrc->GetWidth() >> region.srcSubresource.mipLevel, 1ull));
		region.extent.height = (max)(pSrc->GetHeight() >> region.srcSubresource.mipLevel, 1u);
	}

	vkCmdResolveImage(m_state->m_commandBuffer, pSrc->GetImage(), pSrc->ImageLayout(ResourceState::RESOLVE_SOURCE),
		pDst->GetImage(), pDst->ImageLayout(ResourceState::RESOLVE_DEST), 1, &region);
	(void)format;
}

void CommandList_VK::RSSetShadingRate(ShadingRate baseShadingRate, const ShadingRateCombiner* pCombiners) const
{
	assert(m_ultimate->m_setShadingRate);
	const auto rate = static_cast<uint32_t>(baseShadingRate);
	VkExtent2D size = { 1u << (rate >> 2), 1u << (rate & 3) };
	VkFragmentShadingRateCombinerOpKHR combiners[2] =
	{
		pCombiners ? Combiner(pCombiners[0]) : VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR,
		pCombiners ? Combiner(pCombiners[1]) : VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR
	};
	m_ultimate->m_setShadingRate(m_state->m_commandBuffer, &size, combiners);
}

void CommandList_VK::RSSetShadingRateImage(Resource* pShadingRateImage) const
{
	if (!pShadingRateImage)
	{
		if (m_ultimate->m_shadingRate.View == VK_NULL_HANDLE) return;
		endRendering();
		vkDestroyImageView(m_pDevice->GetDevice(), m_ultimate->m_shadingRate.View, nullptr);
		m_ultimate->m_shadingRate = {};

		return;
	}

	assert(m_pDevice && m_pDevice->HasFragmentShadingRate());
	const auto pResource = AsVK(pShadingRateImage);
	if (viewCached(m_ultimate->m_shadingRate, pResource, 0, 0, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D)) return;

	endRendering();
	cachedView(m_ultimate->m_shadingRate, pResource, 0, 0, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D);
}

void CommandList_VK::DispatchMesh(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) const
{
	assert(m_ultimate->m_drawMesh);
	if (!m_ultimate->m_drawMesh) return;
	if (m_state->m_type == CommandListType::DIRECT) beginRendering();
	flushPush(true);
	m_ultimate->m_drawMesh(m_state->m_commandBuffer, threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}

void CommandList_VK::SetStateObject(const Pipeline& stateObject) const
{
	assert(!"SetStateObject is not implemented on Vulkan");
	(void)stateObject;
}

void CommandList_VK::SetProgram(ProgramType type, ProgramIdentifier identifier, WorkGraphFlag flags,
	uint64_t backingMemoryAddress, uint64_t backingMemoryByteSize, uint64_t localRootArgTableAddress,
	uint64_t localRootArgTableByteSize, uint64_t localRootArgTableByteStride)
{
	assert(!"SetProgram is not implemented on Vulkan");
	(void)type;
	(void)identifier;
	(void)flags;
	(void)backingMemoryAddress;
	(void)backingMemoryByteSize;
	(void)localRootArgTableAddress;
	(void)localRootArgTableByteSize;
	(void)localRootArgTableByteStride;
}

void CommandList_VK::DispatchGraph(uint32_t numNodeInputs, const NodeCPUInput* pNodeInputs, uint64_t nodeInputByteStride)
{
	assert(!"DispatchGraph is not implemented on Vulkan");
	(void)numNodeInputs;
	(void)pNodeInputs;
	(void)nodeInputByteStride;
}

void CommandList_VK::DispatchGraph(uint64_t nodeGPUInputAddress, bool isMultiNodes)
{
	assert(!"DispatchGraph is not implemented on Vulkan");
	(void)nodeGPUInputAddress;
	(void)isMultiNodes;
}

void CommandList_VK::DispatchGraph(uint32_t numBindings, const ProgramBinding* pBindings)
{
	assert(!"DispatchGraph is not implemented on Vulkan");
	(void)numBindings;
	(void)pBindings;
}

void CommandList_VK::ConvertLinearAlgebraMatrix(const LinearAlgebraMatrixConversionInfo* pConversions, uint32_t numConversions)
{
	assert(!"ConvertLinearAlgebraMatrix is not implemented on Vulkan");
	(void)pConversions;
	(void)numConversions;
}

namespace XUSG::Ultimate
{
namespace
{
	uint32_t CopyTexelBytes(Format format)
	{
		if (IsCompressedFormat(format)) return GetVKFormatBits(format) <= 4 ? 8u : 16u;

		return (max)(GetVKFormatBits(format), 8u) / 8u;
	}

	VkImageAspectFlags TransferAspect(VkFormat format)
	{
		const auto aspect = GetVKImageAspect(GetFormatFromVK(format));
		if ((aspect & VK_IMAGE_ASPECT_DEPTH_BIT) != 0) return VK_IMAGE_ASPECT_DEPTH_BIT;

		return aspect ? aspect : static_cast<VkImageAspectFlags>(VK_IMAGE_ASPECT_COLOR_BIT);
	}

	void DecodeSubresource(const Resource_VK* pResource, uint32_t subresource, uint32_t& mip, uint32_t& layer, uint32_t& depth)
	{
		const auto mips = (max)(pResource->GetMipLevels(), 1u);
		mip = mips ? subresource % mips : 0;
		if (pResource->Is3D())
		{
			layer = 0;
			depth = (max)(pResource->GetDepthOrArray() >> mip, 1u);
		}
		else
		{
			layer = subresource / mips;
			depth = 1;
		}
	}

	uint32_t BufferRowLength(const PlacedSubresourceFootprint& footprint)
	{
		const auto texelBytes = CopyTexelBytes(footprint.Footprint.Format);
		assert(texelBytes);
		assert(footprint.Footprint.RowPitch % texelBytes == 0);

		return footprint.Footprint.RowPitch / texelBytes;
	}

	VkDeviceSize BufferRegionOffset(const PlacedSubresourceFootprint& footprint, const BoxRange* pSourceBox)
	{
		auto offset = static_cast<VkDeviceSize>(footprint.Offset);
		if (!pSourceBox) return offset;

		const auto texelBytes = CopyTexelBytes(footprint.Footprint.Format);
		const auto rowPitch = static_cast<VkDeviceSize>(footprint.Footprint.RowPitch);
		const auto slicePitch = rowPitch * footprint.Footprint.Height;
		offset += static_cast<VkDeviceSize>(pSourceBox->Front) * slicePitch;
		offset += static_cast<VkDeviceSize>(pSourceBox->Top) * rowPitch;
		offset += static_cast<VkDeviceSize>(pSourceBox->Left) * texelBytes;

		return offset;
	}
}
}

void CommandList_VK::CopyBufferRegions(uint32_t numRegions, Resource* const* ppDestBuffers, const uint64_t* pDestOffsets,
	Resource* const* ppSourceBuffers, const uint64_t* pSourceOffsets, const uint64_t* pSizes)
{
	assert(numRegions);
	assert(ppDestBuffers);
	assert(pDestOffsets);
	assert(ppSourceBuffers);
	assert(pSourceOffsets);
	assert(pSizes);

	recordOutside();
	vector<VkBufferCopy> copies(numRegions);

	// Batch contiguous runs that share the same source/dest buffers.
	auto runStart = 0u;
	while (runStart < numRegions)
	{
		assert(ppDestBuffers[runStart]);
		assert(ppSourceBuffers[runStart]);
		const auto pDst = AsVK(ppDestBuffers[runStart]);
		const auto pSrc = AsVK(ppSourceBuffers[runStart]);
		assert(pDst && pSrc);
		const auto dst = pDst->GetBuffer();
		const auto src = pSrc->GetBuffer();
		auto runCount = 0u;
		for (auto i = runStart; i < numRegions; ++i)
		{
			assert(ppDestBuffers[i]);
			assert(ppSourceBuffers[i]);
			const auto pDstI = AsVK(ppDestBuffers[i]);
			const auto pSrcI = AsVK(ppSourceBuffers[i]);
			if (!pDstI || !pSrcI || pDstI->GetBuffer() != dst || pSrcI->GetBuffer() != src) break;
			copies[runCount].srcOffset = pSourceOffsets[i];
			copies[runCount].dstOffset = pDestOffsets[i];
			copies[runCount].size = pSizes[i];
			++runCount;
		}
		vkCmdCopyBuffer(m_state->m_commandBuffer, src, dst, runCount, copies.data());
		runStart += runCount;
	}
}

void CommandList_VK::CopyTextureRegions(uint32_t numCopyDescs, const TextureCopyDesc* pCopyDescs)
{
	assert(numCopyDescs);
	assert(pCopyDescs);

	for (auto i = 0u; i < numCopyDescs; ++i)
	{
		const auto& desc = pCopyDescs[i];
		if (desc.Type == TextureCopyDescType::TEXTURE_TO_TEXTURE)
		{
			assert(desc.TextureTextureCopy.pDest);
			assert(desc.TextureTextureCopy.pSource);
			const TextureCopyLocation dst(desc.TextureTextureCopy.pDest, desc.TextureTextureCopy.DestSubresourceIndex);
			const TextureCopyLocation src(desc.TextureTextureCopy.pSource, desc.TextureTextureCopy.SourceSubresourceIndex);
			if (!desc.NumRegions)
				CopyTextureRegion(dst, 0, 0, 0, src, nullptr);
			else
			{
				assert(desc.pCopyRegions);
				for (auto j = 0u; j < desc.NumRegions; ++j)
				{
					const auto& region = desc.pCopyRegions[j];
					CopyTextureRegion(dst, region.DestX, region.DestY, region.DestZ, src, region.pSourceBox);
				}
			}

			continue;
		}

		assert(desc.BufferTextureCopy.pBuffer);
		assert(desc.BufferTextureCopy.pTexture);
		const auto pBuffer = AsVK(desc.BufferTextureCopy.pBuffer);
		const auto pTexture = AsVK(desc.BufferTextureCopy.pTexture);
		assert(pBuffer && !pBuffer->IsImage());
		assert(pTexture && pTexture->IsImage());

		auto footprint = desc.BufferTextureCopy.PlacedFootprint;
		if (footprint.Footprint.Format == Format::UNKNOWN)
			footprint.Footprint.Format = GetFormatFromVK(pTexture->GetVkFormat());
		assert(footprint.Footprint.Format != Format::UNKNOWN);

		uint32_t mip = 0, layer = 0, depth = 1;
		DecodeSubresource(pTexture, desc.BufferTextureCopy.SubresourceIndex, mip, layer, depth);

		const auto toTexture = desc.Type == TextureCopyDescType::BUFFER_TO_TEXTURE;
		const auto regionCount = desc.NumRegions ? desc.NumRegions : 1u;
		vector<VkBufferImageCopy> copies(regionCount);

		for (auto j = 0u; j < regionCount; ++j)
		{
			const TextureCopyRegion* pRegion = desc.NumRegions ? &desc.pCopyRegions[j] : nullptr;
			auto& copy = copies[j];
			copy = {};
			copy.bufferOffset = BufferRegionOffset(footprint, pRegion ? pRegion->pSourceBox : nullptr);
			copy.bufferRowLength = BufferRowLength(footprint);
			copy.bufferImageHeight = footprint.Footprint.Height;
			copy.imageSubresource.aspectMask = TransferAspect(pTexture->GetVkFormat());
			copy.imageSubresource.mipLevel = mip;
			copy.imageSubresource.baseArrayLayer = layer;
			copy.imageSubresource.layerCount = 1;
			if (pRegion)
			{
				copy.imageOffset = {
					static_cast<int32_t>(pRegion->DestX),
					static_cast<int32_t>(pRegion->DestY),
					static_cast<int32_t>(pRegion->DestZ)
				};
				if (pRegion->pSourceBox)
				{
					copy.imageExtent.width = pRegion->pSourceBox->Right - pRegion->pSourceBox->Left;
					copy.imageExtent.height = pRegion->pSourceBox->Bottom - pRegion->pSourceBox->Top;
					copy.imageExtent.depth = pRegion->pSourceBox->Back - pRegion->pSourceBox->Front;
				}
				else
				{
					copy.imageExtent.width = footprint.Footprint.Width;
					copy.imageExtent.height = footprint.Footprint.Height;
					copy.imageExtent.depth = footprint.Footprint.Depth;
				}
			}
			else
			{
				copy.imageExtent.width = footprint.Footprint.Width;
				copy.imageExtent.height = footprint.Footprint.Height;
				copy.imageExtent.depth = footprint.Footprint.Depth;
			}
		}

		recordOutside();
		if (toTexture)
		{
			vkCmdCopyBufferToImage(m_state->m_commandBuffer, pBuffer->GetBuffer(), pTexture->GetImage(),
				pTexture->ImageLayout(ResourceState::COPY_DEST), regionCount, copies.data());
		}
		else
		{
			vkCmdCopyImageToBuffer(m_state->m_commandBuffer, pTexture->GetImage(),
				pTexture->ImageLayout(ResourceState::COPY_SOURCE), pBuffer->GetBuffer(),
				regionCount, copies.data());
		}
	}
}

void CommandList_VK::CopyResources(uint32_t numSourceDestPairs, Resource* const* ppDestResources,
	Resource* const* ppSourceResources)
{
	assert(numSourceDestPairs);
	assert(ppDestResources);
	assert(ppSourceResources);

	for (auto i = 0u; i < numSourceDestPairs; ++i)
		CopyResource(ppDestResources[i], ppSourceResources[i]);
}

void CommandList_VK::CopyTilesAsync(Resource* pTiledResource, const TiledResourceCoord* pTileRegionStartCoord,
	const TileRegionSize& tileRegionSize, Resource* pBuffer, uint64_t bufferOffset, TileCopyFlag flags)
{
	assert(!"CopyTilesAsync is not implemented on Vulkan");
	(void)pTiledResource;
	(void)pTileRegionStartCoord;
	(void)tileRegionSize;
	(void)pBuffer;
	(void)bufferOffset;
	(void)flags;
}

void CommandList_VK::ResolveSubresourceRegionAsync(Resource* pDstResource, uint32_t dstSubresource,
	uint32_t dstX, uint32_t dstY, Resource* pSrcResource, uint32_t srcSubresource,
	const RectRange* pSrcRect, Format format, ResolveMode resolveMode)
{
	assert(pDstResource);
	assert(pSrcResource);

	RectRange fullRect = {};
	if (!pSrcRect)
	{
		const auto pSrc = AsVK(pSrcResource);
		assert(pSrc);
		const auto mips = (max)(pSrc->GetMipLevels(), 1u);
		const auto mip = srcSubresource % mips;
		fullRect.Left = 0;
		fullRect.Top = 0;
		fullRect.Right = static_cast<long>((max)(pSrc->GetWidth() >> mip, 1ull));
		fullRect.Bottom = static_cast<long>((max)(pSrc->GetHeight() >> mip, 1u));
		pSrcRect = &fullRect;
	}

	ResolveSubresourceRegion(pDstResource, dstSubresource, dstX, dstY,
		pSrcResource, srcSubresource, *pSrcRect, format, resolveMode);
}

void CommandList_VK::ResolveQueryDataAsync(const QueryHeap& queryHeap, QueryType type, uint32_t startIndex,
	uint32_t numQueries, Resource* pDestBuffer, uint64_t destOffset)
{
	ResolveQueryData(queryHeap, type, startIndex, numQueries, pDestBuffer, destOffset);
}

void CommandList_VK::ClearBoundDepthStencilView(ClearFlag clearFlags, float depth, uint8_t stencil,
	uint32_t numRegions, const BoxRange* pRegions)
{
	if (!m_state->m_hasDepth) return;

	if (!numRegions)
	{
		ClearDepthStencilView(clearFlags, depth, stencil, 0, nullptr);

		return;
	}

	assert(pRegions);
	if (m_state->m_type == CommandListType::DIRECT) beginRendering();
	assert(m_state->m_renderingBegun);

	VkClearAttachment clear = {};
	if ((clearFlags & ClearFlag::DEPTH) != ClearFlag::NONE) clear.aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
	if ((clearFlags & ClearFlag::STENCIL) != ClearFlag::NONE) clear.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	clear.clearValue.depthStencil.depth = depth;
	clear.clearValue.depthStencil.stencil = stencil;
	if (!clear.aspectMask) return;

	if (m_state->m_clearRects.size() < numRegions) m_state->m_clearRects.resize(numRegions);
	for (auto i = 0u; i < numRegions; ++i)
	{
		m_state->m_clearRects[i].rect.offset = { static_cast<int32_t>(pRegions[i].Left), static_cast<int32_t>(pRegions[i].Top) };
		m_state->m_clearRects[i].rect.extent.width = pRegions[i].Right - pRegions[i].Left;
		m_state->m_clearRects[i].rect.extent.height = pRegions[i].Bottom - pRegions[i].Top;
		m_state->m_clearRects[i].baseArrayLayer = m_state->m_depth.BaseLayer + pRegions[i].Front;
		m_state->m_clearRects[i].layerCount = (max)(pRegions[i].Back - pRegions[i].Front, 1u);
	}

	vkCmdClearAttachments(m_state->m_commandBuffer, 1, &clear, numRegions, m_state->m_clearRects.data());
}

void CommandList_VK::ClearBoundRenderTargetViews(uint32_t renderTargetMask, const ClearData* pClearValues,
	const uint32_t* pNumRegions, const BoxRange* const* ppRegions)
{
	assert(pClearValues);
	assert(renderTargetMask);
	assert((renderTargetMask & ~0xffu) == 0);

	if (m_state->m_type == CommandListType::DIRECT) beginRendering();
	assert(m_state->m_renderingBegun);

	for (auto slot = 0u; slot < 8; ++slot)
	{
		if ((renderTargetMask & (1u << slot)) == 0) continue;
		if (slot >= m_state->m_colorCount) continue;

		VkClearAttachment clear = {};
		clear.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		clear.colorAttachment = slot;
		memcpy(&clear.clearValue.color, &pClearValues[slot], sizeof(ClearData));

		const auto hasRegions = pNumRegions && ppRegions && pNumRegions[slot] && ppRegions[slot];
		const auto regionCount = hasRegions ? pNumRegions[slot] : 1u;
		if (m_state->m_clearRects.size() < regionCount) m_state->m_clearRects.resize(regionCount);

		if (!hasRegions)
		{
			m_state->m_clearRects[0].rect.offset = {};
			m_state->m_clearRects[0].rect.extent = { m_state->m_colors[slot].Width, m_state->m_colors[slot].Height };
			m_state->m_clearRects[0].baseArrayLayer = 0;
			m_state->m_clearRects[0].layerCount = m_state->m_colors[slot].Layers;
		}
		else
		{
			for (auto i = 0u; i < regionCount; ++i)
			{
				const auto& box = ppRegions[slot][i];
				m_state->m_clearRects[i].rect.offset = { static_cast<int32_t>(box.Left), static_cast<int32_t>(box.Top) };
				m_state->m_clearRects[i].rect.extent.width = box.Right - box.Left;
				m_state->m_clearRects[i].rect.extent.height = box.Bottom - box.Top;
				m_state->m_clearRects[i].baseArrayLayer = m_state->m_colors[slot].BaseLayer + box.Front;
				m_state->m_clearRects[i].layerCount = (max)(box.Back - box.Front, 1u);
			}
		}

		vkCmdClearAttachments(m_state->m_commandBuffer, 1, &clear, regionCount, m_state->m_clearRects.data());
	}
}

void CommandList_VK::ClearTextureSubresources(uint32_t numClearDescs, const ClearTextureDesc* pClearDescs)
{
	assert(numClearDescs);
	assert(pClearDescs);

	recordOutside();
	for (auto i = 0u; i < numClearDescs; ++i)
	{
		const auto& desc = pClearDescs[i];
		assert(desc.pTexture);
		assert(!desc.pRegion && "ClearTextureSubresources regions are not implemented on Vulkan");

		const auto pTexture = AsVK(desc.pTexture);
		assert(pTexture && pTexture->IsImage());

		auto format = desc.Format;
		if (format == Format::UNKNOWN) format = GetFormatFromVK(pTexture->GetVkFormat());

		VkImageSubresourceRange range = {};
		range.aspectMask = GetVKImageAspect(format);
		if (desc.SubresourceIndex == 0xffffffffu)
		{
			range.baseMipLevel = 0;
			range.levelCount = VK_REMAINING_MIP_LEVELS;
			range.baseArrayLayer = 0;
			range.layerCount = VK_REMAINING_ARRAY_LAYERS;
		}
		else
		{
			uint32_t mip = 0, layer = 0, depth = 1;
			DecodeSubresource(pTexture, desc.SubresourceIndex, mip, layer, depth);
			range.baseMipLevel = mip;
			range.levelCount = 1;
			range.baseArrayLayer = layer;
			range.layerCount = 1;
		}

		const auto layout = pTexture->ImageLayout(ResourceState::COPY_DEST);
		if (IsDepthStencilFormat(format))
		{
			VkClearDepthStencilValue value = {};
			value.depth = desc.ClearValue.Floats[0];
			value.stencil = static_cast<uint32_t>(desc.ClearValue.Uints[1]);
			if ((range.aspectMask & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) == 0)
				range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			vkCmdClearDepthStencilImage(m_state->m_commandBuffer, pTexture->GetImage(), layout, &value, 1, &range);
		}
		else
		{
			VkClearColorValue color = {};
			memcpy(&color, &desc.ClearValue, sizeof(color));
			if (!range.aspectMask) range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			vkCmdClearColorImage(m_state->m_commandBuffer, pTexture->GetImage(), layout, &color, 1, &range);
		}
	}
}

void CommandList_VK::FillBuffers(uint32_t numFillDescs, const FillBufferDesc* pFillDescs)
{
	assert(numFillDescs);
	assert(pFillDescs);

	recordOutside();
	for (auto i = 0u; i < numFillDescs; ++i)
	{
		const auto& desc = pFillDescs[i];
		assert(desc.pBuffer);
		if (!desc.RepeatCount) continue;

		const auto pBuffer = AsVK(desc.pBuffer);
		assert(pBuffer && !pBuffer->IsImage());

		auto patternSize = desc.RawPatternSizeInBytes;
		if (desc.Format != Format::UNKNOWN)
		{
			assert(patternSize == 0);
			patternSize = CopyTexelBytes(desc.Format);
		}
		assert(patternSize >= 1 && patternSize <= 16);

		const auto fillBytes = static_cast<VkDeviceSize>(patternSize) * desc.RepeatCount;
		assert(desc.Offset + fillBytes <= pBuffer->GetByteSize());

		if (patternSize == 4)
		{
			uint32_t value = 0;
			memcpy(&value, desc.FillValue.Bytes, sizeof(value));
			assert((desc.Offset % 4) == 0 && (fillBytes % 4) == 0);
			vkCmdFillBuffer(m_state->m_commandBuffer, pBuffer->GetBuffer(), desc.Offset, fillBytes, value);

			continue;
		}

		// vkCmdUpdateBuffer is capped at 65536 bytes; larger non-4-byte patterns need a staging path.
		assert(fillBytes <= 65536 && "FillBuffers pattern size other than 4 bytes is limited to 64KB on Vulkan");
		vector<uint8_t> data(static_cast<size_t>(fillBytes));
		for (auto n = 0u; n < desc.RepeatCount; ++n)
			memcpy(data.data() + static_cast<size_t>(n) * patternSize, desc.FillValue.Bytes, patternSize);
		vkCmdUpdateBuffer(m_state->m_commandBuffer, pBuffer->GetBuffer(), desc.Offset, fillBytes, data.data());
	}
}

bool CommandList_VK::createInterface()
{
	if (m_state->m_commandBuffer == VK_NULL_HANDLE || !m_pDevice || m_pDevice->GetDevice() == VK_NULL_HANDLE) return false;
	loadProcs();

	return true;
}

const void* CommandList_VK::renderingPNext() const
{
	if (m_ultimate->m_shadingRate.View == VK_NULL_HANDLE) return nullptr;

	m_ultimate->m_shadingRateInfo.sType = VK_STRUCTURE_TYPE_RENDERING_FRAGMENT_SHADING_RATE_ATTACHMENT_INFO_KHR;
	m_ultimate->m_shadingRateInfo.pNext = nullptr;
	m_ultimate->m_shadingRateInfo.imageView = m_ultimate->m_shadingRate.View;
	m_ultimate->m_shadingRateInfo.imageLayout = m_pDevice->PreferredImageLayout(VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR);

	return &m_ultimate->m_shadingRateInfo;
}

void CommandList_VK::loadProcs()
{
	if (!m_pDevice) return;
	m_ultimate->m_pDevice = m_pDevice;
	const auto device = m_pDevice->GetDevice();
	if (device == VK_NULL_HANDLE) return;
	m_ultimate->m_setShadingRate = reinterpret_cast<PFN_vkCmdSetFragmentShadingRateKHR>(
		vkGetDeviceProcAddr(device, "vkCmdSetFragmentShadingRateKHR"));
	m_ultimate->m_drawMesh = reinterpret_cast<PFN_vkCmdDrawMeshTasksEXT>(vkGetDeviceProcAddr(device, "vkCmdDrawMeshTasksEXT"));
}

void CommandList_VK::flushBarriers(VkMemoryBarrier2& memory) const
{
	if (!memory.srcStageMask && !memory.dstStageMask && m_state->m_buffers.empty() && m_state->m_images.empty()) return;

	VkDependencyInfo info = {};
	FillBarrierDependency(info, memory, m_state->m_buffers, m_state->m_images);
	pipelineBarrier(info);
	memory.srcStageMask = 0;
	memory.srcAccessMask = 0;
	memory.dstStageMask = 0;
	memory.dstAccessMask = 0;
	m_state->m_buffers.clear();
	m_state->m_images.clear();
}

void CommandList_VK::addBarrier(const ResourceBarrier& barrier, VkMemoryBarrier2& memory) const
{
	if (!barrier.pResource)
	{
		memory.srcStageMask |= BarrierStage(m_pDevice, barrier.SyncBefore);
		memory.srcAccessMask |= BarrierAccessFlags(m_pDevice, barrier.AccessBefore);
		memory.dstStageMask |= BarrierStage(m_pDevice, barrier.SyncAfter);
		memory.dstAccessMask |= BarrierAccessFlags(m_pDevice, barrier.AccessAfter);

		return;
	}

	const auto pResource = AsVK(barrier.pResource);
	assert(pResource);
	if (IsBufferBarrier(barrier) || !pResource->IsImage())
	{
		VkBufferMemoryBarrier2 buffer = {};
		buffer.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
		buffer.srcStageMask = BarrierStage(m_pDevice, barrier.SyncBefore);
		buffer.srcAccessMask = BarrierAccessFlags(m_pDevice, barrier.AccessBefore);
		buffer.dstStageMask = BarrierStage(m_pDevice, barrier.SyncAfter);
		buffer.dstAccessMask = BarrierAccessFlags(m_pDevice, barrier.AccessAfter);
		buffer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		buffer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		buffer.buffer = pResource->GetBuffer();
		buffer.offset = barrier.Offset;
		buffer.size = barrier.Size ? barrier.Size : VK_WHOLE_SIZE;
		m_state->m_buffers.push_back(buffer);

		return;
	}

	VkImageMemoryBarrier2 image = {};
	image.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	image.srcStageMask = BarrierStage(m_pDevice, barrier.SyncBefore);
	image.srcAccessMask = BarrierAccessFlags(m_pDevice, barrier.AccessBefore);
	image.dstStageMask = BarrierStage(m_pDevice, barrier.SyncAfter);
	image.dstAccessMask = BarrierAccessFlags(m_pDevice, barrier.AccessAfter);
	image.oldLayout = XUSG::GetVKBarrierLayout(barrier.LayoutBefore);
	image.newLayout = XUSG::GetVKBarrierLayout(barrier.LayoutAfter);

	// BarrierLayout::PRESENT aliases COMMON and maps to GENERAL in GetVKBarrierLayout;
	// resolve to PRESENT_SRC / per-resource common layout like Resource_VK::ImageLayout.
	if (barrier.LayoutBefore == BarrierLayout::COMMON)
	{
		image.oldLayout = pResource->IsPresentable() ?
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : pResource->GetCommonLayout();
	}
	if (barrier.LayoutAfter == BarrierLayout::COMMON)
	{
		image.newLayout = pResource->IsPresentable() ?
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : pResource->GetCommonLayout();
	}
	image.oldLayout = m_pDevice->PreferredImageLayout(image.oldLayout);
	image.newLayout = m_pDevice->PreferredImageLayout(image.newLayout);
	image.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	image.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	image.image = pResource->GetImage();
	SubresourceRange(barrier, pResource, image.subresourceRange);
	if (image.oldLayout == image.newLayout && barrier.LayoutBefore == BarrierLayout::COMMON)
		return;

	// Vulkan applies every barrier in one call at once, so an overlapping layout
	// change has to be submitted first.
	if (image.oldLayout != image.newLayout)
	{
		for (const auto& queued : m_state->m_images)
		{
			if (ImageBarrierOverlaps(image, queued) && queued.oldLayout != queued.newLayout)
			{
				flushBarriers(memory);
				break;
			}
		}
	}
	pResource->AppendImageBarrier(image, m_state->m_images);
}

bool CommandList_VK::ultimateBarriersTouchBoundAttachment(uint32_t numBarriers, const ResourceBarrier* pBarriers) const
{
	if (!m_state->m_renderingBegun || !numBarriers || !pBarriers) return false;

	for (auto i = 0u; i < numBarriers; ++i)
	{
		const auto& barrier = pBarriers[i];
		if (!barrier.pResource || IsBufferBarrier(barrier)) continue;

		const auto pResource = AsVK(barrier.pResource);
		if (pResource->IsImage() && imageBound(pResource->GetImage())) return true;
	}

	return false;
}

void CommandList_VK::prepareUltimateBarriersOutsidePass(uint32_t numBarriers, const ResourceBarrier* pBarriers) const
{
	if (m_state->m_renderingBegun)
	{
		if (!ultimateBarriersTouchBoundAttachment(numBarriers, pBarriers)) return;
		endRendering();
	}
	flushDeferredClears();
	endPredication();
}

void CommandList_VK::resolveMinMax(Resource* pDstResource, uint32_t dstSubresource, uint32_t dstX, uint32_t dstY,
	Resource* pSrcResource, uint32_t srcSubresource, const RectRange& srcRect, ResolveMode resolveMode) const
{
	const auto pSrc = AsVK(pSrcResource);
	const auto pDst = AsVK(pDstResource);
	assert(pSrc);
	assert(pDst);
	const auto srcAspect = DepthStencilAspect(pSrc->GetVkFormat());
	const auto dstAspect = DepthStencilAspect(pDst->GetVkFormat());
	if (srcAspect == VK_IMAGE_ASPECT_COLOR_BIT || dstAspect == VK_IMAGE_ASPECT_COLOR_BIT)
	{
		assert(!"Color min/max resolve is not implemented on Vulkan");

		return;
	}
	const auto depthStencilResolve = m_pDevice && m_pDevice->HasDepthStencilResolve();
	assert(depthStencilResolve);
	if (!depthStencilResolve) return;
	assert(pSrc->GetSampleCount() > 1 && pDst->GetSampleCount() <= 1);
	if (pSrc->GetSampleCount() <= 1 || pDst->GetSampleCount() > 1) return;

	const auto full = srcRect.Right <= srcRect.Left || srcRect.Bottom <= srcRect.Top;
	if (full)
	{
		assert(dstX == 0 && dstY == 0);
		if (dstX || dstY) return;
	}
	else
	{
		assert(dstX == static_cast<uint32_t>(srcRect.Left) && dstY == static_cast<uint32_t>(srcRect.Top));
		if (dstX != static_cast<uint32_t>(srcRect.Left) || dstY != static_cast<uint32_t>(srcRect.Top)) return;
	}

	const auto mode = resolveMode == ResolveMode::MIN ? VK_RESOLVE_MODE_MIN_BIT : VK_RESOLVE_MODE_MAX_BIT;
	const auto independentNone = m_pDevice->IndependentDepthStencilResolveNone();
	auto depthMode = (srcAspect & VK_IMAGE_ASPECT_DEPTH_BIT) != 0 &&
		(m_pDevice->SupportedDepthResolveModes() & mode) != 0 ? mode : VK_RESOLVE_MODE_NONE;
	auto stencilMode = (srcAspect & VK_IMAGE_ASPECT_STENCIL_BIT) != 0 &&
		(m_pDevice->SupportedStencilResolveModes() & mode) != 0 ? mode : VK_RESOLVE_MODE_NONE;
	if ((dstAspect & VK_IMAGE_ASPECT_DEPTH_BIT) == 0) depthMode = VK_RESOLVE_MODE_NONE;
	if ((dstAspect & VK_IMAGE_ASPECT_STENCIL_BIT) == 0) stencilMode = VK_RESOLVE_MODE_NONE;
	const auto wantDepth = (srcAspect & dstAspect & VK_IMAGE_ASPECT_DEPTH_BIT) != 0;
	const auto wantStencil = (srcAspect & dstAspect & VK_IMAGE_ASPECT_STENCIL_BIT) != 0;
	if (wantDepth && depthMode == VK_RESOLVE_MODE_NONE &&
		(!wantStencil || stencilMode == VK_RESOLVE_MODE_NONE || !independentNone))
	{
		assert(!"Depth resolve mode is not supported");

		return;
	}
	if (wantStencil && stencilMode == VK_RESOLVE_MODE_NONE &&
		(!wantDepth || depthMode == VK_RESOLVE_MODE_NONE || !independentNone))
	{
		assert(!"Stencil resolve mode is not supported");

		return;
	}
	if (depthMode != VK_RESOLVE_MODE_NONE && stencilMode != VK_RESOLVE_MODE_NONE &&
		depthMode != stencilMode && !m_pDevice->IndependentDepthStencilResolve())
	{
		assert(!"Independent depth and stencil resolve is not supported");

		return;
	}
	if (depthMode == VK_RESOLVE_MODE_NONE && stencilMode == VK_RESOLVE_MODE_NONE)
	{
		assert(!"Depth resolve mode is not supported");

		return;
	}

	VkImageAspectFlags viewAspect = 0;
	if (depthMode != VK_RESOLVE_MODE_NONE) viewAspect |= VK_IMAGE_ASPECT_DEPTH_BIT;
	if (stencilMode != VK_RESOLVE_MODE_NONE) viewAspect |= VK_IMAGE_ASPECT_STENCIL_BIT;

	const auto srcMips = (max)(pSrc->GetMipLevels(), 1u);
	const auto dstMips = (max)(pDst->GetMipLevels(), 1u);
	const auto srcMip = srcSubresource % srcMips;
	const auto dstMip = dstSubresource % dstMips;
	const auto srcLayer = pSrc->Is3D() ? 0u : srcSubresource / srcMips;
	const auto dstLayer = pDst->Is3D() ? 0u : dstSubresource / dstMips;
	auto width = static_cast<uint32_t>((max)(srcRect.Right - srcRect.Left, 0l));
	auto height = static_cast<uint32_t>((max)(srcRect.Bottom - srcRect.Top, 0l));
	VkOffset2D offset = { srcRect.Left, srcRect.Top };
	if (!width || !height)
	{
		width = static_cast<uint32_t>((max)(pSrc->GetWidth() >> srcMip, 1ull));
		height = (max)(pSrc->GetHeight() >> srcMip, 1u);
		offset = { 0, 0 };
	}

	const auto srcType = !pSrc->Is3D() && pSrc->GetDepthOrArray() > 1 ?
		VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
	const auto dstType = !pDst->Is3D() && pDst->GetDepthOrArray() > 1 ?
		VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
	if (!cachedView(m_ultimate->m_resolveSrc, pSrc, srcMip, srcLayer, viewAspect, srcType) ||
		!cachedView(m_ultimate->m_resolveDst, pDst, dstMip, dstLayer, viewAspect, dstType))
		return;

	struct LayoutPass
	{
		const XUSG::Resource_VK* Resource;
		uint32_t Mip;
		uint32_t Layer;
		VkImageAspectFlags Aspect;
		VkImageLayout OldLayout;
		VkImageLayout NewLayout;
		VkPipelineStageFlags2 SrcStage;
		VkAccessFlags2 SrcAccess;
		VkPipelineStageFlags2 DstStage;
		VkAccessFlags2 DstAccess;
	};

	const auto passLayout = m_pDevice->PreferredImageLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	const auto dsStage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
	const auto dsAccess = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	const auto srcState = pSrc->GetResourceState(srcSubresource);
	const auto dstState = pDst->GetResourceState(dstSubresource);
	LayoutPass passes[2] =
	{
		{ pSrc, srcMip, srcLayer, srcAspect, pSrc->ImageLayout(srcState), passLayout,
			GetVKStage(m_pDevice, srcState), GetVKAccess(m_pDevice, srcState), dsStage, dsAccess },
		{ pDst, dstMip, dstLayer, dstAspect, pDst->ImageLayout(dstState), passLayout,
			GetVKStage(m_pDevice, dstState), GetVKAccess(m_pDevice, dstState), dsStage, dsAccess }
	};

	auto transition = [&]()
	{
		VkImageMemoryBarrier2 images[2] = {};
		auto count = 0u;
		for (auto i = 0u; i < 2u; ++i)
		{
			if (passes[i].OldLayout == passes[i].NewLayout) continue;
			images[count].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
			images[count].srcStageMask = passes[i].SrcStage;
			images[count].srcAccessMask = passes[i].SrcAccess;
			images[count].dstStageMask = passes[i].DstStage;
			images[count].dstAccessMask = passes[i].DstAccess;
			images[count].oldLayout = passes[i].OldLayout;
			images[count].newLayout = passes[i].NewLayout;
			images[count].image = passes[i].Resource->GetImage();
			images[count].subresourceRange.aspectMask = passes[i].Aspect;
			images[count].subresourceRange.baseMipLevel = passes[i].Mip;
			images[count].subresourceRange.levelCount = 1;
			images[count].subresourceRange.baseArrayLayer = passes[i].Layer;
			images[count].subresourceRange.layerCount = 1;
			++count;
		}
		if (!count) return;

		VkDependencyInfo dependency = {};
		dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		dependency.imageMemoryBarrierCount = count;
		dependency.pImageMemoryBarriers = images;
		pipelineBarrier(dependency);
	};

	const auto restart = m_state->m_predicationActive;
	endPredication();
	transition();

	VkRenderingAttachmentInfo depth = {};
	depth.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	depth.imageView = m_ultimate->m_resolveSrc.View;
	depth.imageLayout = passLayout;
	depth.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	depth.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depth.resolveMode = depthMode;
	depth.resolveImageView = m_ultimate->m_resolveDst.View;
	depth.resolveImageLayout = passLayout;
	VkRenderingAttachmentInfo stencil = depth;
	stencil.resolveMode = stencilMode;

	VkRenderingInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	info.renderArea.offset = offset;
	info.renderArea.extent = { width, height };
	info.layerCount = 1;
	info.pDepthAttachment = depthMode != VK_RESOLVE_MODE_NONE ? &depth : nullptr;
	info.pStencilAttachment = stencilMode != VK_RESOLVE_MODE_NONE ? &stencil : nullptr;
	assert(m_state->m_beginRendering && m_state->m_endRendering);
	m_state->m_beginRendering(m_state->m_commandBuffer, &info);
	m_state->m_endRendering(m_state->m_commandBuffer);

	for (auto i = 0u; i < 2u; ++i)
	{
		passes[i].SrcStage = dsStage;
		passes[i].SrcAccess = dsAccess;
		passes[i].DstStage = i == 0 ? GetVKStage(m_pDevice, srcState) : GetVKStage(m_pDevice, dstState);
		passes[i].DstAccess = i == 0 ? GetVKAccess(m_pDevice, srcState) : GetVKAccess(m_pDevice, dstState);
		passes[i].OldLayout = passLayout;
		passes[i].NewLayout = i == 0 ? pSrc->ImageLayout(srcState) : pDst->ImageLayout(dstState);
	}
	transition();
	if (restart) beginPredication();
}

bool CommandList_VK::viewCached(const CachedView& cached, const XUSG::Resource_VK* pResource, uint32_t mip,
	uint32_t layer, VkImageAspectFlags aspect, VkImageViewType type) const
{
	return cached.View != VK_NULL_HANDLE && cached.Image == pResource->GetImage() && cached.Format == pResource->GetVkFormat() &&
		cached.Type == type && cached.Mip == mip && cached.Layer == layer && cached.Aspect == aspect;
}

bool CommandList_VK::cachedView(CachedView& cached, const XUSG::Resource_VK* pResource, uint32_t mip,
	uint32_t layer, VkImageAspectFlags aspect, VkImageViewType type) const
{
	if (viewCached(cached, pResource, mip, layer, aspect, type)) return true;

	const auto device = m_pDevice->GetDevice();
	if (cached.View != VK_NULL_HANDLE) vkDestroyImageView(device, cached.View, nullptr);
	cached.View = VK_NULL_HANDLE;

	VkImageViewCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	info.image = pResource->GetImage();
	info.viewType = type;
	info.format = pResource->GetVkFormat();
	info.subresourceRange.aspectMask = aspect;
	info.subresourceRange.baseMipLevel = mip;
	info.subresourceRange.levelCount = 1;
	info.subresourceRange.baseArrayLayer = layer;
	info.subresourceRange.layerCount = 1;
	const auto result = vkCreateImageView(device, &info, nullptr, &cached.View);
	if (result < 0)
	{
		cerr << "VkResult " << result << endl;
		assert(result >= 0);
		cached.View = VK_NULL_HANDLE;

		return false;
	}

	cached.Image = info.image;
	cached.Format = info.format;
	cached.Type = type;
	cached.Mip = mip;
	cached.Layer = layer;
	cached.Aspect = aspect;

	return true;
}

//--------------------------------------------------------------------------------------
// Pipeline layout
//--------------------------------------------------------------------------------------

PipelineLayoutLib_VK::PipelineLayoutLib_VK() :
	XUSG::PipelineLayoutLib_VK()
{
}

PipelineLayoutLib_VK::PipelineLayoutLib_VK(Device* pDevice) :
	XUSG::PipelineLayoutLib_VK(pDevice)
{
}

PipelineLayoutLib_VK::~PipelineLayoutLib_VK()
{
}

XUSG::PipelineLayout PipelineLayoutLib_VK::CreateRootSignatureFromLibSubobject(
	const Blob& blobLib, const wchar_t* name, uint32_t nodeMask)
{
	string key(sizeof(blobLib), 0);
	memcpy(&key[0], blobLib, sizeof(blobLib));

	return createRootSignatureFromLibSubobject(key, blobLib, name, nodeMask);
}

XUSG::PipelineLayout PipelineLayoutLib_VK::GetRootSignatureFromLibSubobject(const Blob& blobLib,
	const wchar_t* name, bool create, uint32_t nodeMask)
{
	string key(sizeof(blobLib), 0);
	memcpy(&key[0], blobLib, sizeof(blobLib));

	return getRootSignatureFromLibSubobject(key, blobLib, name, create, nodeMask);
}

XUSG::PipelineLayout PipelineLayoutLib_VK::createRootSignatureFromLibSubobject(const string& key,
	const Blob& blobLib, const wchar_t* name, uint32_t nodeMask)
{
	const void* pData = nullptr;
	size_t size = 0;
	XUSG::GetVKBlobData(blobLib, &pData, &size);

	const void* pPayload = nullptr;
	size_t payloadSize = 0;
	XUSG_N_RETURN(XUSG::FindVKRootSignaturePayload(pData, size, name, &pPayload, &payloadSize), nullptr);

	return createRootSignatureFromPayload(key, pPayload, payloadSize, name, nodeMask);
}

XUSG::PipelineLayout PipelineLayoutLib_VK::getRootSignatureFromLibSubobject(const string& key,
	const Blob& blobLib, const wchar_t* name, bool create, uint32_t nodeMask)
{
	const auto layoutIter = m_pipelineLayouts.find(key);

	// Create one, if it does not exist
	if (layoutIter == m_pipelineLayouts.cend())
	{
		if (create) return createRootSignatureFromLibSubobject(key, blobLib, name, nodeMask);
		else return nullptr;
	}

	return layoutIter->second.Object;
}

//--------------------------------------------------------------------------------------
// Sampler feedback
//--------------------------------------------------------------------------------------

SamplerFeedBack_VK::SamplerFeedBack_VK()
{
}

SamplerFeedBack_VK::~SamplerFeedBack_VK()
{
}

bool SamplerFeedBack_VK::Create(Device* pDevice, Texture* pTarget, Format format,
	uint32_t mipRegionWidth, uint32_t mipRegionHeight, uint32_t mipRegionDepth,
	ResourceFlag resourceFlags, bool isCubeMap, MemoryFlag memoryFlags, const wchar_t* name,
	uint16_t srvComponentMapping, const TextureLayoutDesc& textureLayout, uint32_t maxThreads)
{
	assert(!"Sampler feedback is not implemented on Vulkan");
	(void)pDevice;
	(void)pTarget;
	(void)format;
	(void)mipRegionWidth;
	(void)mipRegionHeight;
	(void)mipRegionDepth;
	(void)resourceFlags;
	(void)isCubeMap;
	(void)memoryFlags;
	(void)name;
	(void)srvComponentMapping;
	(void)textureLayout;
	(void)maxThreads;

	return false;
}

bool SamplerFeedBack_VK::Create(Device* pDevice, Heap* pHeap, uint64_t heapOffset,
	Texture* pTarget, Format format, uint32_t mipRegionWidth, uint32_t mipRegionHeight,
	uint32_t mipRegionDepth, ResourceFlag resourceFlags, bool isCubeMap, const wchar_t* name,
	uint16_t srvComponentMapping, const TextureLayoutDesc& textureLayout, uint32_t maxThreads)
{
	assert(!"Sampler feedback is not implemented on Vulkan");
	(void)pDevice;
	(void)pHeap;
	(void)heapOffset;
	(void)pTarget;
	(void)format;
	(void)mipRegionWidth;
	(void)mipRegionHeight;
	(void)mipRegionDepth;
	(void)resourceFlags;
	(void)isCubeMap;
	(void)name;
	(void)srvComponentMapping;
	(void)textureLayout;
	(void)maxThreads;

	return false;
}

bool SamplerFeedBack_VK::CreateResource(Device* pDevice, Texture* pTarget, Format format, uint32_t mipRegionWidth,
	uint32_t mipRegionHeight, uint32_t mipRegionDepth, ResourceFlag resourceFlags, bool isCubeMap, MemoryFlag memoryFlags,
	ResourceState initialResourceState, const TextureLayoutDesc& textureLayout, uint32_t maxThreads)
{
	assert(!"Sampler feedback is not implemented on Vulkan");
	(void)pDevice;
	(void)pTarget;
	(void)format;
	(void)mipRegionWidth;
	(void)mipRegionHeight;
	(void)mipRegionDepth;
	(void)resourceFlags;
	(void)isCubeMap;
	(void)memoryFlags;
	(void)initialResourceState;
	(void)textureLayout;
	(void)maxThreads;

	return false;
}

bool SamplerFeedBack_VK::CreateResource(Device* pDevice, Heap* pHeap, uint64_t heapOffset, Texture* pTarget,
	Format format, uint32_t mipRegionWidth, uint32_t mipRegionHeight, uint32_t mipRegionDepth, ResourceFlag resourceFlags,
	bool isCubeMap, ResourceState initialResourceState, const TextureLayoutDesc& textureLayout, uint32_t maxThreads)
{
	assert(!"Sampler feedback is not implemented on Vulkan");
	(void)pDevice;
	(void)pHeap;
	(void)heapOffset;
	(void)pTarget;
	(void)format;
	(void)mipRegionWidth;
	(void)mipRegionHeight;
	(void)mipRegionDepth;
	(void)resourceFlags;
	(void)isCubeMap;
	(void)initialResourceState;
	(void)textureLayout;
	(void)maxThreads;

	return false;
}

XUSG::Descriptor SamplerFeedBack_VK::CreateUAV(XUSG::Resource* pTarget)
{
	assert(!"Sampler feedback is not implemented on Vulkan");
	(void)pTarget;

	return nullptr;
}
