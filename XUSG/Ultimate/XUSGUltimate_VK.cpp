//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSGEnum_VK.h"
#include "XUSGUltimate_VK.h"

using namespace std;
using namespace XUSG::Ultimate;

namespace
{
	VkPipelineStageFlags2 GetBarrierSync(BarrierSync sync)
	{
		VkPipelineStageFlags2 stage = VK_PIPELINE_STAGE_2_NONE;
		if ((sync & BarrierSync::ALL) == BarrierSync::ALL) return VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		if ((sync & BarrierSync::DRAW) == BarrierSync::DRAW)
			stage |= VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
			VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
			VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		if ((sync & BarrierSync::INDEX_INPUT) == BarrierSync::INDEX_INPUT) stage |= VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT;
		if ((sync & BarrierSync::VERTEX_SHADING) == BarrierSync::VERTEX_SHADING) stage |= VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
		if ((sync & BarrierSync::PIXEL_SHADING) == BarrierSync::PIXEL_SHADING) stage |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		if ((sync & BarrierSync::DEPTH_STENCIL) == BarrierSync::DEPTH_STENCIL)
			stage |= VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
		if ((sync & BarrierSync::RENDER_TARGET) == BarrierSync::RENDER_TARGET) stage |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		if ((sync & BarrierSync::COMPUTE_SHADING) == BarrierSync::COMPUTE_SHADING) stage |= VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
		if ((sync & BarrierSync::COPY) == BarrierSync::COPY) stage |= VK_PIPELINE_STAGE_2_COPY_BIT;
		if ((sync & BarrierSync::RESOLVE) == BarrierSync::RESOLVE) stage |= VK_PIPELINE_STAGE_2_RESOLVE_BIT;
		if ((sync & BarrierSync::EXECUTE_INDIRECT) == BarrierSync::EXECUTE_INDIRECT) stage |= VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
		if ((sync & BarrierSync::ALL_SHADING) == BarrierSync::ALL_SHADING)
			stage |= VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
		if ((sync & BarrierSync::NON_PIXEL_SHADING) == BarrierSync::NON_PIXEL_SHADING)
			stage |= VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT |
			VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT | VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT |
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
		if ((sync & BarrierSync::CLEAR_UNORDERED_ACCESS_VIEW) == BarrierSync::CLEAR_UNORDERED_ACCESS_VIEW)
			stage |= VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;

		return stage ? stage : VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	}

	VkAccessFlags2 GetBarrierAccess(BarrierAccess access)
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

	void AddBarrier(const ResourceBarrier& barrier, vector<VkMemoryBarrier2>& globals,
		vector<VkBufferMemoryBarrier2>& buffers, vector<VkImageMemoryBarrier2>& images)
	{
		if (!barrier.pResource)
		{
			VkMemoryBarrier2 global = {};
			global.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
			global.srcStageMask = GetBarrierSync(barrier.SyncBefore);
			global.srcAccessMask = GetBarrierAccess(barrier.AccessBefore);
			global.dstStageMask = GetBarrierSync(barrier.SyncAfter);
			global.dstAccessMask = GetBarrierAccess(barrier.AccessAfter);
			globals.push_back(global);

			return;
		}

		const auto pResource = dynamic_cast<XUSG::Resource_VK*>(barrier.pResource);
		if (IsBufferBarrier(barrier) || !pResource->IsImage())
		{
			VkBufferMemoryBarrier2 buffer = {};
			buffer.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
			buffer.srcStageMask = GetBarrierSync(barrier.SyncBefore);
			buffer.srcAccessMask = GetBarrierAccess(barrier.AccessBefore);
			buffer.dstStageMask = GetBarrierSync(barrier.SyncAfter);
			buffer.dstAccessMask = GetBarrierAccess(barrier.AccessAfter);
			buffer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			buffer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			buffer.buffer = pResource->GetBuffer();
			buffer.offset = barrier.Offset;
			buffer.size = barrier.Size ? barrier.Size : VK_WHOLE_SIZE;
			buffers.push_back(buffer);

			return;
		}

		VkImageMemoryBarrier2 image = {};
		image.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		image.srcStageMask = GetBarrierSync(barrier.SyncBefore);
		image.srcAccessMask = GetBarrierAccess(barrier.AccessBefore);
		image.dstStageMask = GetBarrierSync(barrier.SyncAfter);
		image.dstAccessMask = GetBarrierAccess(barrier.AccessAfter);
		image.oldLayout = XUSG::GetVKBarrierLayout(barrier.LayoutBefore);
		image.newLayout = XUSG::GetVKBarrierLayout(barrier.LayoutAfter);
		image.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		image.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		image.image = pResource->GetImage();
		SubresourceRange(barrier, pResource, image.subresourceRange);
		pResource->AppendImageBarrier(image, images);
	}

	void SubmitBarriers(VkCommandBuffer commandBuffer, const vector<VkMemoryBarrier2>& globals,
		const vector<VkBufferMemoryBarrier2>& buffers, const vector<VkImageMemoryBarrier2>& images)
	{
		if (globals.empty() && buffers.empty() && images.empty()) return;

		VkDependencyInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		info.memoryBarrierCount = static_cast<uint32_t>(globals.size());
		info.pMemoryBarriers = globals.empty() ? nullptr : globals.data();
		info.bufferMemoryBarrierCount = static_cast<uint32_t>(buffers.size());
		info.pBufferMemoryBarriers = buffers.empty() ? nullptr : buffers.data();
		info.imageMemoryBarrierCount = static_cast<uint32_t>(images.size());
		info.pImageMemoryBarriers = images.empty() ? nullptr : images.data();
		vkCmdPipelineBarrier2(commandBuffer, &info);
	}
}

//--------------------------------------------------------------------------------------

CommandList_VK::CommandList_VK() :
	XUSG::CommandList_VK(),
	m_setShadingRate(nullptr),
	m_drawMesh(nullptr),
	m_shadingRateView(VK_NULL_HANDLE)
{
	m_shadingRateInfo = {};
}

CommandList_VK::CommandList_VK(XUSG::CommandList& commandList) :
	CommandList_VK()
{
	const auto pCommandList = dynamic_cast<XUSG::CommandList_VK*>(&commandList);
	assert(pCommandList);
	Attach(*pCommandList);
	CreateInterface();
}

CommandList_VK::~CommandList_VK()
{
	if (m_shadingRateView) vkDestroyImageView(m_device, m_shadingRateView, nullptr);
}

bool CommandList_VK::Create(const Device* pDevice, uint32_t nodeMask, CommandListType type,
	const CommandAllocator* pAllocator, const Pipeline& pipeline, const wchar_t* name)
{
	XUSG_N_RETURN(XUSG::CommandList_VK::Create(pDevice, nodeMask, type, pAllocator, pipeline, name), false);
	loadProcs();

	return true;
}

bool CommandList_VK::CreateInterface()
{
	if (!m_commandBuffer || !m_device) return false;
	loadProcs();

	return true;
}

void CommandList_VK::loadProcs()
{
	if (!m_device) return;
	m_setShadingRate = reinterpret_cast<PFN_vkCmdSetFragmentShadingRateKHR>(
		vkGetDeviceProcAddr(m_device, "vkCmdSetFragmentShadingRateKHR"));
	m_drawMesh = reinterpret_cast<PFN_vkCmdDrawMeshTasksEXT>(vkGetDeviceProcAddr(m_device, "vkCmdDrawMeshTasksEXT"));
}

void CommandList_VK::Barrier(uint32_t numBarrierGroups, BarrierGroup* pBarrierGroups)
{
	if (!numBarrierGroups) return;
	assert(pBarrierGroups);
	endRendering();

	m_globals.clear();
	m_buffers.clear();
	m_images.clear();
	for (auto i = 0u; i < numBarrierGroups; ++i)
	{
		const auto& group = pBarrierGroups[i];
		if (!group.NumBarriers) continue;
		assert(group.pBarriers);
		for (auto j = 0u; j < group.NumBarriers; ++j) AddBarrier(group.pBarriers[j], m_globals, m_buffers, m_images);
	}
	SubmitBarriers(m_commandBuffer, m_globals, m_buffers, m_images);
}

void CommandList_VK::Barrier(uint32_t numBufferBarriers, ResourceBarrier* pBufferBarriers,
	uint32_t numTextureBarriers, ResourceBarrier* pTextureBarriers,
	uint32_t numGlobalBarriers, ResourceBarrier* pGlobalBarriers)
{
	endRendering();
	m_globals.clear();
	m_buffers.clear();
	m_images.clear();
	for (auto i = 0u; i < numBufferBarriers; ++i) AddBarrier(pBufferBarriers[i], m_globals, m_buffers, m_images);
	for (auto i = 0u; i < numTextureBarriers; ++i) AddBarrier(pTextureBarriers[i], m_globals, m_buffers, m_images);
	for (auto i = 0u; i < numGlobalBarriers; ++i) AddBarrier(pGlobalBarriers[i], m_globals, m_buffers, m_images);
	SubmitBarriers(m_commandBuffer, m_globals, m_buffers, m_images);
}

void CommandList_VK::SetSamplePositions(uint8_t numSamplesPerPixel, uint8_t numPixels, SamplePosition* pPositions) const
{
	assert(!"SetSamplePositions is not implemented on Vulkan");
	(void)numSamplesPerPixel;
	(void)numPixels;
	(void)pPositions;
}

void CommandList_VK::ResolveSubresourceRegion(const Resource* pDstResource, uint32_t dstSubresource,
	uint32_t dstX, uint32_t dstY, const Resource* pSrcResource, uint32_t srcSubresource,
	const RectRange& srcRect, Format format, ResolveMode resolveMode) const
{
	if (resolveMode != ResolveMode::AVERAGE && resolveMode != ResolveMode::DECOMPRESS)
	{
		assert(!"Resolve mode is not implemented on Vulkan");

		return;
	}
	assert(pDstResource);
	assert(pSrcResource);
	endRendering();

	const auto pSrc = dynamic_cast<const XUSG::Resource_VK*>(pSrcResource);
	const auto pDst = dynamic_cast<const XUSG::Resource_VK*>(pDstResource);
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

	vkCmdResolveImage(m_commandBuffer, pSrc->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		pDst->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	(void)format;
}

void CommandList_VK::RSSetShadingRate(ShadingRate baseShadingRate, const ShadingRateCombiner* pCombiners) const
{
	assert(m_setShadingRate);
	const auto rate = static_cast<uint32_t>(baseShadingRate);
	VkExtent2D size = { 1u << (rate >> 2), 1u << (rate & 3) };
	VkFragmentShadingRateCombinerOpKHR combiners[2] =
	{
		pCombiners ? Combiner(pCombiners[0]) : VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR,
		pCombiners ? Combiner(pCombiners[1]) : VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR
	};
	m_setShadingRate(m_commandBuffer, &size, combiners);
}

void CommandList_VK::RSSetShadingRateImage(const Resource* pShadingRateImage) const
{
	endRendering();
	if (m_shadingRateView) vkDestroyImageView(m_device, m_shadingRateView, nullptr);
	m_shadingRateView = VK_NULL_HANDLE;
	if (!pShadingRateImage) return;

	assert(m_pDeviceVK && m_pDeviceVK->HasFragmentShadingRate());
	const auto pResource = dynamic_cast<const XUSG::Resource_VK*>(pShadingRateImage);
	VkImageViewCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	info.image = pResource->GetImage();
	info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	info.format = pResource->GetVkFormat();
	info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	info.subresourceRange.levelCount = 1;
	info.subresourceRange.layerCount = 1;
	const auto result = vkCreateImageView(m_device, &info, nullptr, &m_shadingRateView);
	if (result < 0)
	{
		cerr << "VkResult " << result << endl;
		assert(result >= 0);
	}
}

void CommandList_VK::DispatchMesh(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) const
{
	assert(m_drawMesh);
	endRendering();
	beginPredication();
	m_drawMesh(m_commandBuffer, threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}

void CommandList_VK::SetStateObject(const Pipeline& stateObject)
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

void CommandList_VK::ConvertLinearAlgebraMatrix(const LinearAlgebraMatrixConversionInfo* pConversions, uint32_t numConversions)
{
	assert(!"ConvertLinearAlgebraMatrix is not implemented on Vulkan");
	(void)pConversions;
	(void)numConversions;
}

const void* CommandList_VK::renderingPNext() const
{
	if (!m_shadingRateView) return nullptr;

	m_shadingRateInfo.sType = VK_STRUCTURE_TYPE_RENDERING_FRAGMENT_SHADING_RATE_ATTACHMENT_INFO_KHR;
	m_shadingRateInfo.pNext = nullptr;
	m_shadingRateInfo.imageView = m_shadingRateView;
	m_shadingRateInfo.imageLayout = VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;

	return &m_shadingRateInfo;
}

//--------------------------------------------------------------------------------------

PipelineLayoutLib_VK::PipelineLayoutLib_VK() :
	XUSG::PipelineLayoutLib_VK()
{
}

PipelineLayoutLib_VK::PipelineLayoutLib_VK(const Device* pDevice) :
	XUSG::PipelineLayoutLib_VK(pDevice)
{
}

PipelineLayoutLib_VK::~PipelineLayoutLib_VK()
{
}

XUSG::PipelineLayout PipelineLayoutLib_VK::CreateRootSignatureFromLibSubobject(const XUSG::Blob& blobLib,
	const wchar_t* name, 	uint32_t nodeMask)
{
	assert(!"CreateRootSignatureFromLibSubobject is not implemented on Vulkan");
	(void)blobLib;
	(void)name;
	(void)nodeMask;

	return nullptr;
}

XUSG::PipelineLayout PipelineLayoutLib_VK::GetRootSignatureFromLibSubobject(const XUSG::Blob& blobLib,
	const wchar_t* name, bool create, uint32_t nodeMask)
{
	assert(!"GetRootSignatureFromLibSubobject is not implemented on Vulkan");
	(void)blobLib;
	(void)name;
	(void)create;
	(void)nodeMask;

	return nullptr;
}

//--------------------------------------------------------------------------------------

SamplerFeedBack_VK::SamplerFeedBack_VK()
{
}

SamplerFeedBack_VK::~SamplerFeedBack_VK()
{
}

bool SamplerFeedBack_VK::Create(const Device* pDevice, Texture* pTarget, Format format, uint32_t mipRegionWidth,
	uint32_t mipRegionHeight, uint32_t mipRegionDepth, ResourceFlag resourceFlags, bool isCubeMap, MemoryFlag memoryFlags,
	const wchar_t* name, uint16_t srvComponentMapping, TextureLayout textureLayout, uint32_t maxThreads)
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

bool SamplerFeedBack_VK::Create(const Device* pDevice, const Heap* pHeap, uint64_t heapOffset, Texture* pTarget,
	Format format, uint32_t mipRegionWidth, uint32_t mipRegionHeight, uint32_t mipRegionDepth, ResourceFlag resourceFlags,
	bool isCubeMap, 	const wchar_t* name, uint16_t srvComponentMapping, TextureLayout textureLayout, uint32_t maxThreads)
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

bool SamplerFeedBack_VK::CreateResource(const Device* pDevice, Texture* pTarget, Format format, uint32_t mipRegionWidth,
	uint32_t mipRegionHeight, uint32_t mipRegionDepth, ResourceFlag resourceFlags, bool isCubeMap, MemoryFlag memoryFlags,
	ResourceState initialResourceState, TextureLayout textureLayout, uint32_t maxThreads)
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

bool SamplerFeedBack_VK::CreateResource(const Device* pDevice, const Heap* pHeap, uint64_t heapOffset, Texture* pTarget,
	Format format, uint32_t mipRegionWidth, uint32_t mipRegionHeight, uint32_t mipRegionDepth, ResourceFlag resourceFlags,
	bool isCubeMap, 	ResourceState initialResourceState, TextureLayout textureLayout, uint32_t maxThreads)
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
