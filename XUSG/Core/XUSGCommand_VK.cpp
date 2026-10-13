//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGCommand_VK.h"
#include "XUSGDescriptor_VK.h"
#include "XUSGResource_VK.h"
#include "XUSGEnum_VK.h"

using namespace std;
using namespace XUSG;

namespace
{
	constexpr uint32_t MaxViewportAndScissorCount = 16;
	constexpr uint32_t MaxVertexBufferSlotCount = 32;
	constexpr uint8_t PENDING_CLEAR_COLOR = 1;
	constexpr uint8_t PENDING_CLEAR_DEPTH = 2;
	constexpr uint8_t PENDING_CLEAR_STENCIL = 4;

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

	VkImageAspectFlags CopyAspect(VkFormat format)
	{
		const auto aspect = GetVKImageAspect(GetFormatFromVK(format));
		if ((aspect & VK_IMAGE_ASPECT_DEPTH_BIT) != 0) return VK_IMAGE_ASPECT_DEPTH_BIT;

		return aspect;
	}

	VkExtent3D MipExtent(const Resource_VK* pResource, uint32_t mip, uint32_t depth)
	{
		VkExtent3D extent = {};
		extent.width = static_cast<uint32_t>((max)(pResource->GetWidth() >> mip, 1ull));
		extent.height = (max)(pResource->GetHeight() >> mip, 1u);
		extent.depth = depth;

		return extent;
	}

	const char* DebugLabel(const void* pData, uint32_t size, char* stack, uint32_t capacity)
	{
		if (!pData || !size || capacity < 2) return "";

		const auto text = static_cast<const char*>(pData);
		if (text[size - 1] == '\0') return text;

		const auto count = (min)(size, capacity - 1);
		memcpy(stack, text, count);
		stack[count] = '\0';

		return stack;
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

	void AddGlobalBarrier(VkMemoryBarrier2& memory, VkPipelineStageFlags2 srcStage, VkAccessFlags2 srcAccess,
		VkPipelineStageFlags2 dstStage, VkAccessFlags2 dstAccess)
	{
		memory.srcStageMask |= srcStage;
		memory.srcAccessMask |= srcAccess;
		memory.dstStageMask |= dstStage;
		memory.dstAccessMask |= dstAccess;
	}

	void LegacyStages(VkPipelineStageFlags2 srcStage, VkPipelineStageFlags2 dstStage,
		VkPipelineStageFlags& source, VkPipelineStageFlags& dest)
	{
		source = srcStage ? static_cast<VkPipelineStageFlags>(srcStage) : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dest = dstStage ? static_cast<VkPipelineStageFlags>(dstStage) : VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	}

	bool IsStreamOutQuery(QueryType type)
	{
		return type >= QueryType::SO_STATISTICS_STREAM0 && type <= QueryType::SO_STATISTICS_STREAM3;
	}

	uint32_t StreamOutIndex(QueryType type)
	{
		return static_cast<uint32_t>(type) - static_cast<uint32_t>(QueryType::SO_STATISTICS_STREAM0);
	}
}

CommandAllocator_VK::CommandAllocator_VK() :
	m_pDevice(nullptr),
	m_pool(VK_NULL_HANDLE),
	m_family(0)
{
}

CommandAllocator_VK::~CommandAllocator_VK()
{
	if (m_pool != VK_NULL_HANDLE) vkDestroyCommandPool(m_pDevice->GetDevice(), m_pool, nullptr);
}

bool CommandAllocator_VK::Create(Device* pDevice, CommandListType type, const wchar_t* name)
{
	assert(type != CommandListType::VIDEO_DECODE && type != CommandListType::VIDEO_PROCESS && type != CommandListType::VIDEO_ENCODE);
	m_pDevice = AsVK(pDevice);
	assert(m_pDevice);
	const auto device = m_pDevice->GetDevice();
	m_family = m_pDevice->GetQueueFamily(type);
	if (m_pool != VK_NULL_HANDLE) vkDestroyCommandPool(device, m_pool, nullptr);

	VkCommandPoolCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	info.queueFamilyIndex = m_family;
	XUSG_VK_CHECK(vkCreateCommandPool(device, &info, nullptr, &m_pool), cerr, false);
	m_pDevice->SetDebugName(VK_OBJECT_TYPE_COMMAND_POOL, (uint64_t)m_pool, name);

	return true;
}

bool CommandAllocator_VK::Reset()
{
	XUSG_VK_CHECK(vkResetCommandPool(m_pDevice->GetDevice(), m_pool, 0), cerr, false);

	return true;
}

void CommandAllocator_VK::Create(uint64_t nativeHandle, Device* pDevice)
{
	m_pDevice = AsVK(pDevice);
	assert(m_pDevice);

	m_pool = (VkCommandPool)nativeHandle;
}

uint64_t CommandAllocator_VK::GetNativeHandle() const
{
	return (uint64_t)m_pool;
}

void* CommandAllocator_VK::GetDeviceNativeHandle() const
{
	return m_pDevice->GetDevice();
}

Device* CommandAllocator_VK::GetDevice() const
{
	return m_pDevice;
}

VkCommandPool CommandAllocator_VK::GetPool() const
{
	return m_pool;
}

uint32_t CommandAllocator_VK::GetFamily() const
{
	return m_family;
}

//--------------------------------------------------------------------------------------

CommandList_VK::CommandList_VK() :
	m_pool(VK_NULL_HANDLE),
	m_commandBuffer(VK_NULL_HANDLE),
	m_ownsCommandBuffer(false),
	m_pDevice(nullptr),
	m_type(CommandListType::DIRECT),
	m_beginRendering(nullptr),
	m_endRendering(nullptr),
	m_setPrimitiveTopology(nullptr),
	m_bindVertexBuffers2(nullptr),
	m_bindIndexBuffer2(nullptr),
	m_beginLabel(nullptr),
	m_endLabel(nullptr),
	m_insertLabel(nullptr),
	m_predicateBuffer(VK_NULL_HANDLE),
	m_predicateOffset(0),
	m_predicateEqualZero(false),
	m_predicationActive(false),
	m_renderingBegun(false),
	m_hasDepth(false),
	m_colorCount(0),
	m_colors(),
	m_renderWidth(1),
	m_renderHeight(1),
	m_graphicsLayout(nullptr),
	m_computeLayout(nullptr),
	m_pipeline(nullptr),
	m_resourceHeap(nullptr),
	m_samplerHeap(nullptr),
	m_graphicsPushData{},
	m_computePushData{},
	m_graphicsPushDirty(0),
	m_computePushDirty(0),
	m_preprocessBuffer(VK_NULL_HANDLE),
	m_preprocessMemory(VK_NULL_HANDLE),
	m_preprocessAddress(0),
	m_preprocessSize(0),
	m_preprocessLayout(VK_NULL_HANDLE),
	m_preprocessPipeline(VK_NULL_HANDLE),
	m_preprocessSequences(0),
	m_preprocessRequired(0),
	m_soBuffers{},
	m_soOffsets{},
	m_soSizes{},
	m_soCounters{},
	m_soCounterOffsets{},
	m_xfbActive(0),
	m_nullXfb(VK_NULL_HANDLE),
	m_nullXfbMemory(VK_NULL_HANDLE)
{
	m_depth = {};
}

CommandList_VK::~CommandList_VK()
{
	const auto device = m_pDevice ? m_pDevice->GetDevice() : VK_NULL_HANDLE;
	if (m_ownsCommandBuffer)
	{
		for (const auto& entry : m_poolBuffers)
			if (entry.Buffer != VK_NULL_HANDLE) vkFreeCommandBuffers(device, entry.Pool, 1, &entry.Buffer);
	}
	if (m_preprocessBuffer != VK_NULL_HANDLE) vkDestroyBuffer(device, m_preprocessBuffer, nullptr);
	if (m_preprocessMemory != VK_NULL_HANDLE) vkFreeMemory(device, m_preprocessMemory, nullptr);
	if (m_nullXfb != VK_NULL_HANDLE) vkDestroyBuffer(device, m_nullXfb, nullptr);
	if (m_nullXfbMemory != VK_NULL_HANDLE) vkFreeMemory(device, m_nullXfbMemory, nullptr);
}

bool CommandList_VK::Create(Device* pDevice, uint32_t nodeMask, CommandListType type,
	CommandAllocator* pAllocator, const Pipeline& pipeline, const wchar_t* name)
{
	(void)nodeMask;
	assert(type != CommandListType::VIDEO_DECODE && type != CommandListType::VIDEO_PROCESS && type != CommandListType::VIDEO_ENCODE);
	m_pDevice = AsVK(pDevice);
	assert(m_pDevice);
	const auto device = m_pDevice->GetDevice();
	m_type = type;
	m_beginRendering = m_pDevice->GetBeginRendering();
	m_endRendering = m_pDevice->GetEndRendering();
	m_setPrimitiveTopology = m_pDevice->GetSetPrimitiveTopology();
	m_bindVertexBuffers2 = m_pDevice->GetBindVertexBuffers2();
	m_bindIndexBuffer2 = reinterpret_cast<PFN_vkCmdBindIndexBuffer2>(vkGetDeviceProcAddr(device, "vkCmdBindIndexBuffer2KHR"));
	if (m_pDevice->HasDebugUtils())
	{
		const auto instance = m_pDevice->GetInstance();
		m_beginLabel = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdBeginDebugUtilsLabelEXT"));
		m_endLabel = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdEndDebugUtilsLabelEXT"));
		m_insertLabel = reinterpret_cast<PFN_vkCmdInsertDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdInsertDebugUtilsLabelEXT"));
	}

	if (m_ownsCommandBuffer)
	{
		for (const auto& entry : m_poolBuffers)
			if (entry.Buffer != VK_NULL_HANDLE) vkFreeCommandBuffers(device, entry.Pool, 1, &entry.Buffer);
		m_poolBuffers.clear();
	}
	m_pool = pAllocator ? (VkCommandPool)pAllocator->GetNativeHandle() : VK_NULL_HANDLE;
	assert(m_pool != VK_NULL_HANDLE);

	VkCommandBufferAllocateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	info.commandPool = m_pool;
	info.level = type == CommandListType::BUNDLE ? VK_COMMAND_BUFFER_LEVEL_SECONDARY : VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	info.commandBufferCount = 1;
	XUSG_VK_CHECK(vkAllocateCommandBuffers(device, &info, &m_commandBuffer), cerr, false);
	m_ownsCommandBuffer = true;
	m_poolBuffers.push_back({ m_pool, m_commandBuffer });
	m_pDevice->SetDebugName(VK_OBJECT_TYPE_COMMAND_BUFFER, reinterpret_cast<uintptr_t>(m_commandBuffer), name);

	VkCommandBufferBeginInfo begin = {};
	begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	XUSG_VK_CHECK(vkBeginCommandBuffer(m_commandBuffer, &begin), cerr, false);
	resetState();
	if (pipeline) SetPipelineState(pipeline);

	return true;
}

bool CommandList_VK::Close() const
{
	endRendering();
	flushDeferredClears();
	endPredication();
	XUSG_VK_CHECK(vkEndCommandBuffer(m_commandBuffer), cerr, false);

	return true;
}

bool CommandList_VK::Reset(CommandAllocator* pAllocator, const Pipeline& initialState) const
{
	endRendering();
	m_deferredClears.clear();
	endPredication();

	const auto pool = pAllocator ? (VkCommandPool)pAllocator->GetNativeHandle() : VK_NULL_HANDLE;
	if (pool != VK_NULL_HANDLE && pool != m_pool)
	{
		VkCommandBuffer buffer = VK_NULL_HANDLE;
		for (const auto& entry : m_poolBuffers)
			if (entry.Pool == pool) buffer = entry.Buffer;
		if (buffer == VK_NULL_HANDLE)
		{
			VkCommandBufferAllocateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			info.commandPool = pool;
			info.level = m_type == CommandListType::BUNDLE ? VK_COMMAND_BUFFER_LEVEL_SECONDARY : VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			info.commandBufferCount = 1;
			XUSG_VK_CHECK(vkAllocateCommandBuffers(m_pDevice->GetDevice(), &info, &buffer), cerr, false);
			m_poolBuffers.push_back({ pool, buffer });
		}
		m_pool = pool;
		m_commandBuffer = buffer;
	}
	XUSG_VK_CHECK(vkResetCommandBuffer(m_commandBuffer, 0), cerr, false);

	VkCommandBufferBeginInfo begin = {};
	begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	XUSG_VK_CHECK(vkBeginCommandBuffer(m_commandBuffer, &begin), cerr, false);
	resetState();
	if (initialState) SetPipelineState(initialState);

	return true;
}

void CommandList_VK::ClearState(const Pipeline& initialState) const
{
	endRendering();
	flushDeferredClears();
	resetState();
	if (initialState) SetPipelineState(initialState);
}

void CommandList_VK::Draw(uint32_t vertexCountPerInstance, uint32_t instanceCount,
	uint32_t startVertexLocation, uint32_t startInstanceLocation) const
{
	if (m_type == CommandListType::DIRECT) beginRendering();
	flushPush(true);
	vkCmdDraw(m_commandBuffer, vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
}

void CommandList_VK::DrawIndexed(uint32_t indexCountPerInstance, uint32_t instanceCount,
	uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation) const
{
	if (m_type == CommandListType::DIRECT) beginRendering();
	flushPush(true);
	vkCmdDrawIndexed(m_commandBuffer, indexCountPerInstance, instanceCount,
		startIndexLocation, baseVertexLocation, startInstanceLocation);
}

void CommandList_VK::Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) const
{
	recordOutside();
	flushPush(false);
	vkCmdDispatch(m_commandBuffer, threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}

void CommandList_VK::CopyBufferRegion(Resource* pDstBuffer, uint64_t dstOffset,
	Resource* pSrcBuffer, uint64_t srcOffset, uint64_t numBytes) const
{
	recordOutside();
	assert(pDstBuffer);
	assert(pSrcBuffer);
	VkBufferCopy region = {};
	region.srcOffset = srcOffset;
	region.dstOffset = dstOffset;
	region.size = numBytes;
	vkCmdCopyBuffer(m_commandBuffer, (VkBuffer)(pSrcBuffer->GetNativeHandle()), (VkBuffer)pDstBuffer->GetNativeHandle(), 1, &region);
}

void CommandList_VK::CopyTextureRegion(const TextureCopyLocation& dst, uint32_t dstX, uint32_t dstY, uint32_t dstZ,
	const TextureCopyLocation& src, const BoxRange* pSrcBox) const
{
	recordOutside();
	assert(dst.pResource);
	assert(src.pResource);
	const auto pDst = AsVK(dst.pResource);
	const auto pSrc = AsVK(src.pResource);

	uint32_t srcMip = 0, srcLayer = 0, srcDepth = 1;
	uint32_t dstMip = 0, dstLayer = 0, dstDepth = 1;
	DecodeSubresource(pSrc, src.SubresourceIndex, srcMip, srcLayer, srcDepth);
	DecodeSubresource(pDst, dst.SubresourceIndex, dstMip, dstLayer, dstDepth);
	const auto srcExtent = MipExtent(pSrc, srcMip, srcDepth);

	VkOffset3D srcOffset = {};
	VkExtent3D extent = srcExtent;
	if (pSrcBox)
	{
		srcOffset.x = static_cast<int32_t>(pSrcBox->Left);
		srcOffset.y = static_cast<int32_t>(pSrcBox->Top);
		srcOffset.z = static_cast<int32_t>(pSrcBox->Front);
		extent.width = pSrcBox->Right - pSrcBox->Left;
		extent.height = pSrcBox->Bottom - pSrcBox->Top;
		extent.depth = pSrcBox->Back - pSrcBox->Front;
	}

	if (pSrc->IsImage() && pDst->IsImage())
	{
		VkImageCopy region = {};
		region.srcSubresource.aspectMask = CopyAspect(pSrc->GetVkFormat());
		region.srcSubresource.mipLevel = srcMip;
		region.srcSubresource.baseArrayLayer = srcLayer;
		region.srcSubresource.layerCount = 1;
		region.srcOffset = srcOffset;
		region.dstSubresource.aspectMask = CopyAspect(pDst->GetVkFormat());
		region.dstSubresource.mipLevel = dstMip;
		region.dstSubresource.baseArrayLayer = dstLayer;
		region.dstSubresource.layerCount = 1;
		region.dstOffset = { static_cast<int32_t>(dstX), static_cast<int32_t>(dstY), static_cast<int32_t>(dstZ) };
		region.extent = extent;
		vkCmdCopyImage(m_commandBuffer, pSrc->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			pDst->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	}
	else if (pSrc->IsImage())
	{
		VkBufferImageCopy region = {};
		region.bufferOffset = 0;
		region.imageSubresource.aspectMask = CopyAspect(pSrc->GetVkFormat());
		region.imageSubresource.mipLevel = srcMip;
		region.imageSubresource.baseArrayLayer = srcLayer;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = srcOffset;
		region.imageExtent = extent;
		vkCmdCopyImageToBuffer(m_commandBuffer, pSrc->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			pDst->GetBuffer(), 1, &region);
	}
	else if (pDst->IsImage())
	{
		VkBufferImageCopy region = {};
		region.bufferOffset = 0;
		region.imageSubresource.aspectMask = CopyAspect(pDst->GetVkFormat());
		region.imageSubresource.mipLevel = dstMip;
		region.imageSubresource.baseArrayLayer = dstLayer;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { static_cast<int32_t>(dstX), static_cast<int32_t>(dstY), static_cast<int32_t>(dstZ) };
		region.imageExtent = extent;
		vkCmdCopyBufferToImage(m_commandBuffer, pSrc->GetBuffer(), pDst->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	}
	else
	{
		VkBufferCopy region = {};
		region.size = static_cast<VkDeviceSize>(extent.width) * extent.height * extent.depth;
		vkCmdCopyBuffer(m_commandBuffer, pSrc->GetBuffer(), pDst->GetBuffer(), 1, &region);
	}
}

void CommandList_VK::CopyResource(Resource* pDstResource, Resource* pSrcResource) const
{
	recordOutside();
	assert(pDstResource);
	assert(pSrcResource);
	const auto pDst = AsVK(pDstResource);
	const auto pSrc = AsVK(pSrcResource);
	if (pSrc->IsImage() && pDst->IsImage())
	{
		const auto mips = (max)(pSrc->GetMipLevels(), 1u);
		const auto layers = pSrc->Is3D() ? 1u : (max)(pSrc->GetDepthOrArray(), 1u);
		if (m_imageCopies.size() < mips) m_imageCopies.resize(mips);
		for (auto mip = 0u; mip < mips; ++mip)
		{
			uint32_t mipLevel = 0, layer = 0, depth = 1;
			DecodeSubresource(pSrc, mip, mipLevel, layer, depth);
			m_imageCopies[mip] = {};
			m_imageCopies[mip].srcSubresource.aspectMask = CopyAspect(pSrc->GetVkFormat());
			m_imageCopies[mip].srcSubresource.mipLevel = mipLevel;
			m_imageCopies[mip].srcSubresource.layerCount = layers;
			m_imageCopies[mip].dstSubresource = m_imageCopies[mip].srcSubresource;
			m_imageCopies[mip].dstSubresource.aspectMask = CopyAspect(pDst->GetVkFormat());
			m_imageCopies[mip].extent = MipExtent(pSrc, mipLevel, pSrc->Is3D() ? depth : 1u);
		}
		vkCmdCopyImage(m_commandBuffer, pSrc->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			pDst->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mips, m_imageCopies.data());
	}
	else
	{
		VkBufferCopy region = {};
		region.size = (min)(pSrc->GetByteSize(), pDst->GetByteSize());
		vkCmdCopyBuffer(m_commandBuffer, pSrc->GetBuffer(), pDst->GetBuffer(), 1, &region);
	}
}

void CommandList_VK::CopyTiles(Resource* pTiledResource, const TiledResourceCoord* pTileRegionStartCoord,
	const TileRegionSize& tileRegionSize, Resource* pBuffer, uint64_t bufferStartByteOffset,
	TileCopyFlag flags) const
{
	recordOutside();
	assert(pTiledResource);
	assert(pTileRegionStartCoord);
	assert(pBuffer);
	const auto pTiled = AsVK(pTiledResource);
	const auto pVkBuffer = AsVK(pBuffer);
	assert(pTiled && pTiled->IsSparse());
	assert(pVkBuffer && pVkBuffer->GetBuffer() != VK_NULL_HANDLE);
	if (!pTiled || !pVkBuffer || !pTiled->IsSparse() || pVkBuffer->GetBuffer() == VK_NULL_HANDLE || !tileRegionSize.NumTiles) return;
	if (pTiled->GetSampleCount() > 1)
	{
		assert(!"CopyTiles does not support multisample tiled resources");
		return;
	}

	const auto toImage = (flags & TileCopyFlag::LINEAR_BUFFER_TO_SWIZZLED_TILED_RESOURCE) ==
		TileCopyFlag::LINEAR_BUFFER_TO_SWIZZLED_TILED_RESOURCE;
	const auto toBuffer = (flags & TileCopyFlag::SWIZZLED_TILED_RESOURCE_TO_LINEAR_BUFFER) ==
		TileCopyFlag::SWIZZLED_TILED_RESOURCE_TO_LINEAR_BUFFER;
	assert(!(toImage && toBuffer));
	if (toImage && toBuffer) return;

	if (!pTiled->IsImage())
	{
		assert(!tileRegionSize.UseBox);
		VkDeviceSize tiledOffset = 0;
		VkDeviceSize size = 0;
		if (!pTiled->BufferTileCopy(pTileRegionStartCoord->X, tileRegionSize.NumTiles, tiledOffset, size))
		{
			assert(!"CopyTiles buffer range is outside the tiled resource");
			return;
		}

		VkBufferCopy region = {};
		region.size = size;
		if (toImage)
		{
			region.srcOffset = bufferStartByteOffset;
			region.dstOffset = tiledOffset;
			vkCmdCopyBuffer(m_commandBuffer, pVkBuffer->GetBuffer(), pTiled->GetBuffer(), 1, &region);
		}
		else
		{
			region.srcOffset = tiledOffset;
			region.dstOffset = bufferStartByteOffset;
			vkCmdCopyBuffer(m_commandBuffer, pTiled->GetBuffer(), pVkBuffer->GetBuffer(), 1, &region);
		}

		return;
	}

	if (m_tileCopies.size() < tileRegionSize.NumTiles) m_tileCopies.resize(tileRegionSize.NumTiles);
	for (auto i = 0u; i < tileRegionSize.NumTiles; ++i)
	{
		if (pTiled->ImageTileCopy(*pTileRegionStartCoord, tileRegionSize, i, bufferStartByteOffset, m_tileCopies[i])) continue;

		assert(!"CopyTiles packed mip tail is not a linear image copy");
		return;
	}

	if (toImage)
	{
		vkCmdCopyBufferToImage(m_commandBuffer, pVkBuffer->GetBuffer(), pTiled->GetImage(),
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, tileRegionSize.NumTiles, m_tileCopies.data());
	}
	else
	{
		vkCmdCopyImageToBuffer(m_commandBuffer, pTiled->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			pVkBuffer->GetBuffer(), tileRegionSize.NumTiles, m_tileCopies.data());
	}
}

void CommandList_VK::ResolveSubresource(Resource* pDstResource, uint32_t dstSubresource,
	Resource* pSrcResource, uint32_t srcSubresource, Format format) const
{
	recordOutside();
	assert(pDstResource);
	assert(pSrcResource);
	const auto pDst = AsVK(pDstResource);
	const auto pSrc = AsVK(pSrcResource);
	uint32_t srcMip = 0, srcLayer = 0, srcDepth = 1;
	uint32_t dstMip = 0, dstLayer = 0, dstDepth = 1;
	DecodeSubresource(pSrc, srcSubresource, srcMip, srcLayer, srcDepth);
	DecodeSubresource(pDst, dstSubresource, dstMip, dstLayer, dstDepth);

	VkImageResolve region = {};
	region.srcSubresource.aspectMask = CopyAspect(GetVKFormat(format));
	region.srcSubresource.mipLevel = srcMip;
	region.srcSubresource.baseArrayLayer = srcLayer;
	region.srcSubresource.layerCount = 1;
	region.dstSubresource.aspectMask = region.srcSubresource.aspectMask;
	region.dstSubresource.mipLevel = dstMip;
	region.dstSubresource.baseArrayLayer = dstLayer;
	region.dstSubresource.layerCount = 1;
	region.extent = MipExtent(pSrc, srcMip, srcDepth);
	vkCmdResolveImage(m_commandBuffer, pSrc->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		pDst->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void CommandList_VK::IASetPrimitiveTopology(PrimitiveTopology primitiveTopology) const
{
	assert(m_setPrimitiveTopology);
	m_setPrimitiveTopology(m_commandBuffer, GetVKPrimitiveTopology(primitiveTopology));
}

void CommandList_VK::RSSetViewports(uint32_t numViewports, const Viewport* pViewports) const
{
	assert(numViewports == 0 || pViewports);
	assert(numViewports <= MaxViewportAndScissorCount);

	VkViewport viewports[MaxViewportAndScissorCount];
	for (uint8_t i = 0; i < numViewports; ++i)
	{
		// SPIR-V is compiled with HLSL clip space (Y up). A negative viewport
		// height maps that onto the framebuffer without flipping every shader.
		viewports[i].x = pViewports[i].TopLeftX;
		viewports[i].y = pViewports[i].TopLeftY + pViewports[i].Height;
		viewports[i].width = pViewports[i].Width;
		viewports[i].height = -pViewports[i].Height;
		viewports[i].minDepth = pViewports[i].MinDepth;
		viewports[i].maxDepth = pViewports[i].MaxDepth;
		m_renderWidth = (max)(m_renderWidth, static_cast<uint32_t>(pViewports[i].TopLeftX + pViewports[i].Width));
		m_renderHeight = (max)(m_renderHeight, static_cast<uint32_t>(pViewports[i].TopLeftY + pViewports[i].Height));
	}

	if (numViewports) vkCmdSetViewport(m_commandBuffer, 0, numViewports, viewports);
}

void CommandList_VK::RSSetScissorRects(uint32_t numRects, const RectRange* pRects) const
{
	assert(numRects == 0 || pRects);
	assert(numRects <= MaxViewportAndScissorCount);

	VkRect2D scissors[MaxViewportAndScissorCount];
	for (uint8_t i = 0; i < numRects; ++i)
	{
		scissors[i].offset.x = static_cast<int32_t>(pRects[i].Left);
		scissors[i].offset.y = static_cast<int32_t>(pRects[i].Top);
		scissors[i].extent.width = static_cast<uint32_t>(pRects[i].Right - pRects[i].Left);
		scissors[i].extent.height = static_cast<uint32_t>(pRects[i].Bottom - pRects[i].Top);
	}

	if (numRects) vkCmdSetScissor(m_commandBuffer, 0, numRects, scissors);
}

void CommandList_VK::OMSetBlendFactor(const float blendFactor[4]) const
{
	vkCmdSetBlendConstants(m_commandBuffer, blendFactor);
}

void CommandList_VK::OMSetStencilRef(uint32_t stencilRef) const
{
	vkCmdSetStencilReference(m_commandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, stencilRef);
}

void CommandList_VK::SetPipelineState(const Pipeline& pipelineState) const
{
	const auto next = static_cast<const Pipeline_VK*>(pipelineState);
	if (m_pipeline == next) return;

	const auto resumePredication = m_predicationActive && m_renderingBegun &&
		(m_xfbActive || (next && next->XfbBuffers));
	if (m_xfbActive)
	{
		endPredication();
		endTransformFeedback();
	}
	else if (resumePredication) endPredication();

	m_pipeline = next;
	if (m_pipeline && m_pipeline->Handle != VK_NULL_HANDLE)
		vkCmdBindPipeline(m_commandBuffer, m_pipeline->BindPoint, m_pipeline->Handle);
	if (m_renderingBegun) beginTransformFeedback();
	if (resumePredication) beginPredication();
}

void CommandList_VK::Barrier(uint32_t numBarriers, const ResourceBarrier* pBarriers) const
{
	if (!numBarriers) return;

	if (m_renderingBegun) endRendering();
	flushDeferredClears();
	endPredication();
	assert(pBarriers);

	// Buffer, UAV and aliasing barriers share one memory barrier. Image layout
	// transitions stay on the image. Vulkan applies every barrier in one call
	// at once, so an overlapping layout change has to be submitted first.
	VkMemoryBarrier2 memory = {};
	memory.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
	m_images.clear();
	for (auto i = 0u; i < numBarriers; ++i)
	{
		const auto& barrier = pBarriers[i];
		if ((barrier.Flags & BarrierFlag::BEGIN_ONLY) == BarrierFlag::BEGIN_ONLY) continue;

		const auto same = barrier.StateBefore == barrier.StateAfter;
		const auto uav = same ? barrier.StateAfter == ResourceState::UNORDERED_ACCESS :
			!barrier.pResource && barrier.StateBefore == ResourceState::UNORDERED_ACCESS;
		if (uav)
		{
			const auto stage = GetVKStage(ResourceState::UNORDERED_ACCESS);
			const auto access = GetVKAccess(ResourceState::UNORDERED_ACCESS);
			AddGlobalBarrier(memory, stage, access, stage, access);
			continue;
		}
		if (same || !barrier.pResource)
		{
			const auto pAfter = AsVK(barrier.pResourceAfter);
			const auto dstAccess = pAfter && pAfter->IsImage() ? VK_ACCESS_2_NONE :
				VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
			AddGlobalBarrier(memory, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_MEMORY_WRITE_BIT,
				VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, dstAccess);
			continue;
		}

		const auto pResource = AsVK(barrier.pResource);
		auto srcStage = GetVKStage(barrier.StateBefore);
		auto dstStage = GetVKStage(barrier.StateAfter);
		auto srcAccess = GetVKAccess(barrier.StateBefore);
		auto dstAccess = GetVKAccess(barrier.StateAfter);
		// PRESENT aliases COMMON. Narrow WSI transitions so the GPU is not
		// stalled on ALL_COMMANDS / MEMORY_* for every presentable barrier.
		if (pResource->IsPresentable() && barrier.StateBefore == ResourceState::COMMON)
		{
			srcStage = VK_PIPELINE_STAGE_2_NONE;
			srcAccess = VK_ACCESS_2_NONE;
		}
		if (pResource->IsPresentable() && barrier.StateAfter == ResourceState::COMMON)
		{
			// Match vkd3d WSI -> PRESENT_SRC: no dst stage/access (not BOTTOM_OF_PIPE).
			dstStage = VK_PIPELINE_STAGE_2_NONE;
			dstAccess = VK_ACCESS_2_NONE;
		}
		if (!pResource->IsImage())
		{
			AddGlobalBarrier(memory, srcStage, srcAccess, dstStage, dstAccess);
			continue;
		}

		VkImageMemoryBarrier2 image = {};
		image.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		image.srcStageMask = srcStage;
		image.srcAccessMask = srcAccess;
		image.dstStageMask = dstStage;
		image.dstAccessMask = dstAccess;
		image.oldLayout = pResource->ImageLayout(barrier.StateBefore);
		image.newLayout = pResource->ImageLayout(barrier.StateAfter);
		image.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		image.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		image.image = pResource->GetImage();
		image.subresourceRange.aspectMask = GetVKImageAspect(GetFormatFromVK(pResource->GetVkFormat()));
		if (barrier.Subresource == XUSG_BARRIER_ALL_SUBRESOURCES)
		{
			image.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
			image.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
		}
		else
		{
			uint32_t mip = 0, layer = 0, depth = 1;
			DecodeSubresource(pResource, barrier.Subresource, mip, layer, depth);
			image.subresourceRange.baseMipLevel = mip;
			image.subresourceRange.levelCount = 1;
			image.subresourceRange.baseArrayLayer = layer;
			image.subresourceRange.layerCount = 1;
		}
		// vkd3d common-layout promotion: COMMON -> compatible read layout needs no transition.
		if (image.oldLayout == image.newLayout && barrier.StateBefore == ResourceState::COMMON)
			continue;
		if (image.oldLayout != image.newLayout)
		{
			for (const auto& queued : m_images)
			{
				if (ImageBarrierOverlaps(image, queued) && queued.oldLayout != queued.newLayout)
				{
					submitBarrierBatch(memory, m_images);
					break;
				}
			}
		}
		pResource->AppendImageBarrier(image, m_images);
	}

	submitBarrierBatch(memory, m_images);
}

void CommandList_VK::ExecuteBundle(CommandList* pCommandList) const
{
	endRendering();
	flushDeferredClears();
	endPredication();
	assert(pCommandList);
	const auto buffer = static_cast<VkCommandBuffer>(pCommandList->GetNativeHandle());
	vkCmdExecuteCommands(m_commandBuffer, 1, &buffer);
}

void CommandList_VK::SetDescriptorHeaps(uint32_t numDescriptorHeaps, const DescriptorHeap* pDescriptorHeaps) const
{
	assert(numDescriptorHeaps == 0 || pDescriptorHeaps);
	for (auto i = 0u; i < numDescriptorHeaps; ++i)
		bindHeap(pDescriptorHeaps[i]);
}

void CommandList_VK::SetComputePipelineLayout(const PipelineLayout& pipelineLayout) const
{
	m_computeLayout = static_cast<const PipelineLayoutObject*>(pipelineLayout);
	m_computePushDirty = 0;
}

void CommandList_VK::SetGraphicsPipelineLayout(const PipelineLayout& pipelineLayout) const
{
	m_graphicsLayout = static_cast<const PipelineLayoutObject*>(pipelineLayout);
	m_graphicsPushDirty = 0;
}

void CommandList_VK::SetComputeDescriptorTable(uint32_t index, const DescriptorTable& descriptorTable) const
{
	const auto& table = reinterpret_cast<const DescriptorTableProperty_VK&>(descriptorTable);
	setDescriptorTable(m_computeLayout, index, table.DescriptorIndex);
}

void CommandList_VK::SetGraphicsDescriptorTable(uint32_t index, const DescriptorTable& descriptorTable) const
{
	const auto& table = reinterpret_cast<const DescriptorTableProperty_VK&>(descriptorTable);
	setDescriptorTable(m_graphicsLayout, index, table.DescriptorIndex);
}

void CommandList_VK::SetComputeDescriptorTable(uint32_t index, const DescriptorHeap& descriptorHeap, int32_t offset) const
{
	const auto heap = static_cast<const DescriptorHeap_VK*>(descriptorHeap);
	const auto stride = heap ? heap->GetStride() : 1u;
	setDescriptorTable(m_computeLayout, index, static_cast<uint32_t>(offset / stride));
}

void CommandList_VK::SetGraphicsDescriptorTable(uint32_t index, const DescriptorHeap& descriptorHeap, int32_t offset) const
{
	const auto heap = static_cast<const DescriptorHeap_VK*>(descriptorHeap);
	const auto stride = heap ? heap->GetStride() : 1u;
	setDescriptorTable(m_graphicsLayout, index, static_cast<uint32_t>(offset / stride));
}

void CommandList_VK::SetCompute32BitConstant(uint32_t index, uint32_t srcData, uint32_t destOffsetIn32BitValues) const
{
	setConstants(m_computeLayout, index, 1, &srcData, destOffsetIn32BitValues);
}

void CommandList_VK::SetGraphics32BitConstant(uint32_t index, uint32_t srcData, uint32_t destOffsetIn32BitValues) const
{
	setConstants(m_graphicsLayout, index, 1, &srcData, destOffsetIn32BitValues);
}

void CommandList_VK::SetCompute32BitConstants(uint32_t index, uint32_t num32BitValuesToSet,
	const void* pSrcData, uint32_t destOffsetIn32BitValues) const
{
	setConstants(m_computeLayout, index, num32BitValuesToSet, pSrcData, destOffsetIn32BitValues);
}

void CommandList_VK::SetGraphics32BitConstants(uint32_t index, uint32_t num32BitValuesToSet,
	const void* pSrcData, uint32_t destOffsetIn32BitValues) const
{
	setConstants(m_graphicsLayout, index, num32BitValuesToSet, pSrcData, destOffsetIn32BitValues);
}

void CommandList_VK::SetComputeRootConstantBufferView(uint32_t index, const Resource* pResource, int32_t offset) const
{
	assert(pResource);
	setRootAddress(m_computeLayout, index, pResource->GetVirtualAddress(offset));
}

void CommandList_VK::SetGraphicsRootConstantBufferView(uint32_t index, const Resource* pResource, int32_t offset) const
{
	assert(pResource);
	setRootAddress(m_graphicsLayout, index, pResource->GetVirtualAddress(offset));
}

void CommandList_VK::SetComputeRootShaderResourceView(uint32_t index, const Resource* pResource, int32_t offset) const
{
	assert(pResource);
	setRootAddress(m_computeLayout, index, pResource->GetVirtualAddress(offset));
}

void CommandList_VK::SetGraphicsRootShaderResourceView(uint32_t index, const Resource* pResource, int32_t offset) const
{
	assert(pResource);
	setRootAddress(m_graphicsLayout, index, pResource->GetVirtualAddress(offset));
}

void CommandList_VK::SetComputeRootUnorderedAccessView(uint32_t index, const Resource* pResource, int32_t offset) const
{
	assert(pResource);
	setRootAddress(m_computeLayout, index, pResource->GetVirtualAddress(offset));
}

void CommandList_VK::SetGraphicsRootUnorderedAccessView(uint32_t index, const Resource* pResource, int32_t offset) const
{
	assert(pResource);
	setRootAddress(m_graphicsLayout, index, pResource->GetVirtualAddress(offset));
}

void CommandList_VK::SetComputeRootConstantBufferView(uint32_t index, uint64_t address) const
{
	setRootAddress(m_computeLayout, index, address);
}

void CommandList_VK::SetGraphicsRootConstantBufferView(uint32_t index, uint64_t address) const
{
	setRootAddress(m_graphicsLayout, index, address);
}

void CommandList_VK::SetComputeRootShaderResourceView(uint32_t index, uint64_t address) const
{
	setRootAddress(m_computeLayout, index, address);
}

void CommandList_VK::SetGraphicsRootShaderResourceView(uint32_t index, uint64_t address) const
{
	setRootAddress(m_graphicsLayout, index, address);
}

void CommandList_VK::SetComputeRootUnorderedAccessView(uint32_t index, uint64_t address) const
{
	setRootAddress(m_computeLayout, index, address);
}

void CommandList_VK::SetGraphicsRootUnorderedAccessView(uint32_t index, uint64_t address) const
{
	setRootAddress(m_graphicsLayout, index, address);
}

void CommandList_VK::IASetIndexBuffer(const IndexBufferView& view) const
{
	const auto buffer = view.pResource ? (VkBuffer)view.pResource->GetNativeHandle() : VK_NULL_HANDLE;
	assert(buffer != VK_NULL_HANDLE);

	const auto size = view.ByteSize ? static_cast<VkDeviceSize>(view.ByteSize) : VK_WHOLE_SIZE;
	if (m_bindIndexBuffer2)
		m_bindIndexBuffer2(m_commandBuffer, buffer, view.ByteOffset, size, GetVKIndexType(view.Format));
	else vkCmdBindIndexBuffer(m_commandBuffer, buffer, view.ByteOffset, GetVKIndexType(view.Format));
}

void CommandList_VK::IASetVertexBuffers(uint32_t startSlot, uint32_t numViews, const VertexBufferView* pViews) const
{
	assert(numViews <= MaxVertexBufferSlotCount);
	if (!numViews) return;

	VkBuffer buffers[MaxVertexBufferSlotCount];
	VkDeviceSize offsets[MaxVertexBufferSlotCount];
	VkDeviceSize sizes[MaxVertexBufferSlotCount];
	VkDeviceSize strides[MaxVertexBufferSlotCount];
	if (!pViews)
	{
		for (auto i = 0u; i < numViews; ++i)
		{
			buffers[i] = VK_NULL_HANDLE;
			offsets[i] = 0;
			sizes[i] = 0;
			strides[i] = 0;
		}
	}
	else
	{
		for (auto i = 0u; i < numViews; ++i)
		{
			const auto& view = pViews[i];
			buffers[i] = view.pResource ? (VkBuffer)view.pResource->GetNativeHandle() : VK_NULL_HANDLE;
			assert(buffers[i] != VK_NULL_HANDLE);
			offsets[i] = view.ByteOffset;
			sizes[i] = view.ByteSize;
			strides[i] = view.ByteStride;
		}
	}

	if (m_bindVertexBuffers2)
		m_bindVertexBuffers2(m_commandBuffer, startSlot, numViews, buffers, offsets, sizes, strides);
	else vkCmdBindVertexBuffers(m_commandBuffer, startSlot, numViews, buffers, offsets);
}

void CommandList_VK::SOSetTargets(uint32_t startSlot, uint32_t numViews, const StreamOutBufferView* pViews) const
{
	assert(numViews == 0 || pViews);
	assert(startSlot <= MaxStreamOutCount && startSlot + numViews <= MaxStreamOutCount);
	const auto transformFeedback = m_pDevice && m_pDevice->HasTransformFeedback();
	assert(transformFeedback);
	if (!transformFeedback) return;

	const auto resumePredication = m_xfbActive && m_predicationActive;
	if (m_xfbActive)
	{
		endPredication();
		endTransformFeedback();
	}

	for (auto i = 0u; i < numViews; ++i)
	{
		const auto slot = startSlot + i;
		const auto& view = pViews[i];
		const auto resource = AsVK(view.pResource);
		if (!resource || !view.ByteSize)
		{
			m_soBuffers[slot] = VK_NULL_HANDLE;
			m_soOffsets[slot] = 0;
			m_soSizes[slot] = 0;
			m_soCounters[slot] = VK_NULL_HANDLE;
			m_soCounterOffsets[slot] = 0;
			continue;
		}

		assert(resource->GetBuffer() != VK_NULL_HANDLE);
		assert((view.ByteOffset & 3) == 0);
		const auto counter = AsVK(view.pBufferFilledSize);
		assert(counter && counter->GetBuffer() != VK_NULL_HANDLE);
		assert((view.BufferFilledSizeByteOffset & 3) == 0);
		if (resource->GetBuffer() == VK_NULL_HANDLE || !counter || counter->GetBuffer() == VK_NULL_HANDLE)
		{
			m_soBuffers[slot] = VK_NULL_HANDLE;
			m_soOffsets[slot] = 0;
			m_soSizes[slot] = 0;
			m_soCounters[slot] = VK_NULL_HANDLE;
			m_soCounterOffsets[slot] = 0;
			continue;
		}
		m_soBuffers[slot] = resource->GetBuffer();
		m_soOffsets[slot] = view.ByteOffset;
		m_soSizes[slot] = view.ByteSize;
		m_soCounters[slot] = counter ? counter->GetBuffer() : VK_NULL_HANDLE;
		m_soCounterOffsets[slot] = view.BufferFilledSizeByteOffset;
	}

	if (m_renderingBegun) beginTransformFeedback();
	if (resumePredication) beginPredication();
}

void CommandList_VK::OMSetRenderTargets(uint32_t numRenderTargetDescriptors,
	const Descriptor* pRenderTargetViews, const Descriptor* pDepthStencilView) const
{
	assert(numRenderTargetDescriptors <= MaxRenderTargetCount);
	assert(numRenderTargetDescriptors == 0 || pRenderTargetViews);
	if (sameRenderTargets(numRenderTargetDescriptors, pRenderTargetViews, pDepthStencilView)) return;

	if (!m_renderingBegun && hasPendingClear())
	{
		beginRendering();
		endRendering();
	}
	else endRendering();

	m_colorCount = numRenderTargetDescriptors;
	m_hasDepth = false;
	for (auto i = 0u; i < numRenderTargetDescriptors; ++i)
	{
		const auto pView = static_cast<const ResourceView_VK*>(pRenderTargetViews[i]);
		assert(pView && pView->Type == ResourceViewType::RTV);
		loadAttachment(m_colors[i], pView);
		m_renderWidth = (max)(m_renderWidth, m_colors[i].Width);
		m_renderHeight = (max)(m_renderHeight, m_colors[i].Height);
	}

	if (pDepthStencilView && *pDepthStencilView)
	{
		const auto pView = static_cast<const ResourceView_VK*>(*pDepthStencilView);
		assert(pView->Type == ResourceViewType::DSV);
		m_hasDepth = true;
		loadAttachment(m_depth, pView);
		m_renderWidth = (max)(m_renderWidth, m_depth.Width);
		m_renderHeight = (max)(m_renderHeight, m_depth.Height);
	}
}

void CommandList_VK::ClearDepthStencilView(ClearFlag clearFlags, float depth, uint8_t stencil,
	uint32_t numRects, const RectRange* pRects) const
{
	if (!m_hasDepth) return;

	if (!numRects && !m_renderingBegun && m_predicateBuffer == VK_NULL_HANDLE)
	{
		if ((clearFlags & ClearFlag::DEPTH) != ClearFlag::NONE)
		{
			m_depth.PendingClear |= PENDING_CLEAR_DEPTH;
			m_depth.Rendering.clearValue.depthStencil.depth = depth;
		}
		if ((clearFlags & ClearFlag::STENCIL) != ClearFlag::NONE)
		{
			m_depth.PendingClear |= PENDING_CLEAR_STENCIL;
			m_depth.Rendering.clearValue.depthStencil.stencil = stencil;
		}

		return;
	}

	if (m_type == CommandListType::DIRECT) beginRendering();
	assert(m_renderingBegun);
	clearDepth(m_depth, clearFlags, depth, stencil, numRects, pRects);
}

void CommandList_VK::ClearDepthStencilView(const Descriptor& depthStencilView, ClearFlag clearFlags, float depth,
	uint8_t stencil, uint32_t numRects, const RectRange* pRects) const
{
	if (!depthStencilView)
	{
		ClearDepthStencilView(clearFlags, depth, stencil, numRects, pRects);

		return;
	}

	const auto pView = static_cast<const ResourceView_VK*>(depthStencilView);
	if (m_hasDepth && pView->Target.Rendering.imageView == m_depth.Rendering.imageView)
	{
		ClearDepthStencilView(clearFlags, depth, stencil, numRects, pRects);

		return;
	}

	RenderAttachment attachment = {};
	loadAttachment(attachment, pView);

	const auto image = attachment.Resource ? attachment.Resource->GetImage() : VK_NULL_HANDLE;
	if (!numRects && !imageBound(image))
	{
		DeferredClear clear = {};
		clear.Attachment = attachment;
		clear.Depth = true;
		clear.Flags = clearFlags;
		clear.DepthValue = depth;
		clear.Stencil = stencil;
		m_deferredClears.push_back(clear);

		return;
	}

	const auto resumePass = m_renderingBegun;
	endRendering();
	beginPredication();
	clearOutside(attachment, true, clearFlags, depth, stencil, nullptr, numRects, pRects);
	if (resumePass) beginRendering();
}

void CommandList_VK::ClearRenderTargetView(const float colorRGBA[4], uint8_t renderTargetViewIndex,
	uint32_t numRects, const RectRange* pRects) const
{
	if (renderTargetViewIndex >= m_colorCount) return;

	if (!numRects && !m_renderingBegun && m_predicateBuffer == VK_NULL_HANDLE)
	{
		auto& attachment = m_colors[renderTargetViewIndex];
		attachment.PendingClear |= PENDING_CLEAR_COLOR;
		memcpy(attachment.Rendering.clearValue.color.float32, colorRGBA, sizeof(float) * 4);

		return;
	}

	if (m_type == CommandListType::DIRECT) beginRendering();
	assert(m_renderingBegun);
	clearColor(renderTargetViewIndex, colorRGBA, numRects, pRects);
}

void CommandList_VK::ClearRenderTargetView(const Descriptor& renderTargetView, const float colorRGBA[4],
	uint32_t numRects, const RectRange* pRects) const
{
	assert(renderTargetView);
	const auto pView = static_cast<const ResourceView_VK*>(renderTargetView);
	for (auto i = 0u; i < m_colorCount; ++i)
	{
		if (m_colors[i].Rendering.imageView == pView->Target.Rendering.imageView)
		{
			ClearRenderTargetView(colorRGBA, static_cast<uint8_t>(i), numRects, pRects);

			return;
		}
	}

	RenderAttachment attachment = {};
	loadAttachment(attachment, pView);

	const auto image = attachment.Resource ? attachment.Resource->GetImage() : VK_NULL_HANDLE;
	if (!numRects && !imageBound(image))
	{
		DeferredClear clear = {};
		clear.Attachment = attachment;
		clear.Depth = false;
		memcpy(clear.Color, colorRGBA, sizeof(float) * 4);
		m_deferredClears.push_back(clear);

		return;
	}

	const auto resumePass = m_renderingBegun;
	endRendering();
	beginPredication();
	clearOutside(attachment, false, ClearFlag::NONE, 0.0f, 0, colorRGBA, numRects, pRects);
	if (resumePass) beginRendering();
}

void CommandList_VK::ClearUnorderedAccessViewUint(const DescriptorTable& descriptorTable, const Descriptor& descriptor,
	const uint32_t values[4], uint32_t numRects, const RectRange* pRects) const
{
	(void)descriptorTable;
	(void)pRects;
	VkClearColorValue color = {};
	memcpy(color.uint32, values, sizeof(uint32_t) * 4);
	clearUnorderedAccessView(descriptor, color, numRects);
}

void CommandList_VK::ClearUnorderedAccessViewFloat(const DescriptorTable& descriptorTable, const Descriptor& descriptor,
	const float values[4], uint32_t numRects, const RectRange* pRects) const
{
	(void)descriptorTable;
	(void)pRects;
	VkClearColorValue color = {};
	memcpy(color.float32, values, sizeof(float) * 4);
	clearUnorderedAccessView(descriptor, color, numRects);
}

void CommandList_VK::DiscardResource(Resource* pResource, uint32_t numRects, const RectRange* pRects,
	uint32_t firstSubresource, uint32_t numSubresources) const
{
	assert(pResource);
	assert(numRects == 0 && "DiscardResource rectangles are not supported on Vulkan");
	(void)pRects;
	auto pVkResource = AsVK(pResource);
	assert(pVkResource);
	assert(pVkResource->IsImage() && "DiscardResource buffers are not supported on Vulkan");
	pVkResource->Discard(firstSubresource, numSubresources);
	if (imageBound(pVkResource->GetImage()))
	{
		endRendering();
		beginRendering();

		return;
	}

	for (auto i = 0u; i < m_colorCount; ++i)
		if (m_colors[i].Resource == pVkResource) m_colors[i].PendingClear = 0;
	if (m_depth.Resource == pVkResource) m_depth.PendingClear = 0;
}

void CommandList_VK::BeginQuery(const QueryHeap& queryHeap, QueryType type, uint32_t index) const
{
	if (type == QueryType::TIMESTAMP) return;

	if (IsStreamOutQuery(type))
	{
		assert(m_pDevice && m_pDevice->HasTransformFeedback());
		const auto stream = StreamOutIndex(type);
		const auto geometryStreams = m_pDevice->HasGeometryStreams();
		if (stream && !geometryStreams)
		{
			assert(geometryStreams);

			return;
		}
		const auto begin = m_pDevice->GetExtProcs().BeginQueryIndexed;
		assert(begin);
		if (!begin) return;
		begin(m_commandBuffer, (VkQueryPool)queryHeap, index, 0, stream);

		return;
	}

	const auto flags = type == QueryType::OCCLUSION ? VK_QUERY_CONTROL_PRECISE_BIT : 0u;
	vkCmdBeginQuery(m_commandBuffer, (VkQueryPool)queryHeap, index, flags);
}

void CommandList_VK::EndQuery(const QueryHeap& queryHeap, QueryType type, uint32_t index) const
{
	if (type == QueryType::TIMESTAMP)
	{
		vkCmdWriteTimestamp2(m_commandBuffer, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, (VkQueryPool)queryHeap, index);

		return;
	}

	if (IsStreamOutQuery(type))
	{
		const auto end = m_pDevice->GetExtProcs().EndQueryIndexed;
		assert(end);
		if (!end) return;
		end(m_commandBuffer, (VkQueryPool)queryHeap, index, StreamOutIndex(type));

		return;
	}

	vkCmdEndQuery(m_commandBuffer, (VkQueryPool)queryHeap, index);
}

void CommandList_VK::ResolveQueryData(const QueryHeap& queryHeap, QueryType type, uint32_t startIndex,
	uint32_t numQueries, Resource* pDstBuffer, uint64_t alignedDstBufferOffset) const
{
	assert(pDstBuffer);
	recordOutside();
	const auto stride = IsStreamOutQuery(type) ? sizeof(uint64_t) * 2 : sizeof(uint64_t);
	vkCmdCopyQueryPoolResults(m_commandBuffer, (VkQueryPool)queryHeap, startIndex, numQueries,
		(VkBuffer)pDstBuffer->GetNativeHandle(), alignedDstBufferOffset, stride,
		VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
}

void CommandList_VK::SetPredication(Resource* pBuffer, uint64_t alignedBufferOffset, bool opEqualZero) const
{
	endPredication();
	m_predicateBuffer = VK_NULL_HANDLE;
	if (!pBuffer) return;

	const auto conditionalRendering = m_pDevice && m_pDevice->HasConditionalRendering();
	assert(conditionalRendering);
	if (!conditionalRendering) return;
	assert((alignedBufferOffset % 4) == 0);
	const auto buffer = pBuffer ? (VkBuffer)pBuffer->GetNativeHandle() : VK_NULL_HANDLE;
	assert(buffer != VK_NULL_HANDLE);
	m_predicateBuffer = buffer;
	m_predicateOffset = alignedBufferOffset;
	m_predicateEqualZero = opEqualZero;
	beginPredication();
}

void CommandList_VK::SetMarker(uint32_t metaData, const void* pData, uint32_t size) const
{
	(void)metaData;
	if (!m_insertLabel) return;

	char stack[256];
	VkDebugUtilsLabelEXT label = {};
	label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
	label.pLabelName = DebugLabel(pData, size, stack, static_cast<uint32_t>(sizeof(stack)));
	m_insertLabel(m_commandBuffer, &label);
}

void CommandList_VK::BeginEvent(uint32_t metaData, const void* pData, uint32_t size) const
{
	(void)metaData;
	if (!m_beginLabel) return;

	char stack[256];
	VkDebugUtilsLabelEXT label = {};
	label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
	label.pLabelName = DebugLabel(pData, size, stack, static_cast<uint32_t>(sizeof(stack)));
	m_beginLabel(m_commandBuffer, &label);
}

void CommandList_VK::EndEvent() const
{
	if (m_endLabel) m_endLabel(m_commandBuffer);
}

void CommandList_VK::ExecuteIndirect(CommandLayout* pCommandlayout, uint32_t maxCommandCount,
	Resource* pArgumentBuffer, uint64_t argumentBufferOffset,
	Resource* pCountBuffer, uint64_t countBufferOffset) const
{
	assert(m_pDevice);
	assert(pArgumentBuffer);
	if (!m_pDevice) return;
	const auto& ext = m_pDevice->GetExtProcs();
	const auto deviceGeneratedCommands = m_pDevice->HasDeviceGeneratedCommands();
	const auto layout = AsVK(pCommandlayout);
	assert(layout);
	if (!layout->IsTraceRaysOnly())
	{
		assert(deviceGeneratedCommands);
		if (!deviceGeneratedCommands) return;
	}
	if (!maxCommandCount) return;
	if (layout->IsDispatch() || layout->IsTraceRays())
	{
		recordOutside();
		flushPush(false);
	}
	else
	{
		if (m_type == CommandListType::DIRECT) beginRendering();
		flushPush(true);
	}

	if (layout->IsTraceRays())
	{
		const auto traceRaysIndirect2 = m_pDevice->HasRayTracingPipelineTraceRaysIndirect2();
		assert(traceRaysIndirect2);
		assert(m_pipeline && m_pipeline->BindPoint == VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR);
		if (!traceRaysIndirect2 || !m_pipeline ||
			m_pipeline->BindPoint != VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR)
			return;

		// vkd3d-proton emits one vkCmdTraceRaysIndirect2KHR when the signature has no state changes.
		// Repeat it for MaxCommandCount. A count buffer still needs the DGC sequence count.
		if (layout->IsTraceRaysOnly() && !pCountBuffer)
		{
			const auto trace = ext.TraceRaysIndirect2;
			assert(trace);
			if (!trace) return;
			auto address = pArgumentBuffer->GetVirtualAddress(static_cast<int>(argumentBufferOffset));
			const auto stride = static_cast<VkDeviceAddress>((max)(layout->GetByteStride(), 1u));
			for (auto i = 0u; i < maxCommandCount; ++i)
			{
				trace(m_commandBuffer, address);
				address += stride;
			}

			return;
		}

		if (layout->GetLayout() == VK_NULL_HANDLE)
		{
			assert(!"Indirect ray dispatch with a count buffer needs device generated command ray stages");
			return;
		}
	}
	const auto pipeline = m_pipeline ? m_pipeline->Handle : VK_NULL_HANDLE;
	const auto commandsLayout = layout->GetLayout();
	auto bytes = m_preprocessRequired;
	if (m_preprocessLayout != commandsLayout || m_preprocessPipeline != pipeline || m_preprocessSequences != maxCommandCount)
	{
		VkGeneratedCommandsPipelineInfoEXT pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GENERATED_COMMANDS_PIPELINE_INFO_EXT;
		pipelineInfo.pipeline = pipeline;
		VkGeneratedCommandsMemoryRequirementsInfoEXT requirementsInfo = {};
		requirementsInfo.sType = VK_STRUCTURE_TYPE_GENERATED_COMMANDS_MEMORY_REQUIREMENTS_INFO_EXT;
		requirementsInfo.pNext = pipeline ? &pipelineInfo : nullptr;
		requirementsInfo.indirectCommandsLayout = commandsLayout;
		requirementsInfo.maxSequenceCount = maxCommandCount;
		VkMemoryRequirements2 requirements = {};
		requirements.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
		ext.GetGeneratedCommandsMemoryRequirements(m_pDevice->GetDevice(), &requirementsInfo, &requirements);
		m_preprocessLayout = commandsLayout;
		m_preprocessPipeline = pipeline;
		m_preprocessSequences = maxCommandCount;
		m_preprocessRequired = requirements.memoryRequirements.size;
		bytes = m_preprocessRequired;
	}
	if (bytes) ensurePreprocess(bytes);

	VkGeneratedCommandsPipelineInfoEXT pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GENERATED_COMMANDS_PIPELINE_INFO_EXT;
	pipelineInfo.pipeline = m_pipeline ? m_pipeline->Handle : VK_NULL_HANDLE;

	VkGeneratedCommandsInfoEXT info = {};
	info.sType = VK_STRUCTURE_TYPE_GENERATED_COMMANDS_INFO_EXT;
	info.pNext = pipelineInfo.pipeline ? &pipelineInfo : nullptr;
	info.shaderStages = layout->GetShaderStages();
	info.indirectCommandsLayout = layout->GetLayout();
	info.indirectAddress = pArgumentBuffer->GetVirtualAddress(static_cast<int>(argumentBufferOffset));
	info.indirectAddressSize = static_cast<VkDeviceSize>(maxCommandCount) * (max)(layout->GetByteStride(), 1u);
	info.preprocessAddress = bytes ? m_preprocessAddress : 0;
	info.preprocessSize = bytes ? m_preprocessSize : 0;
	info.maxSequenceCount = maxCommandCount;
	if (pCountBuffer) info.sequenceCountAddress = pCountBuffer->GetVirtualAddress(static_cast<int>(countBufferOffset));
	info.maxDrawCount = maxCommandCount;
	ext.PreprocessGeneratedCommands(m_commandBuffer, &info, m_commandBuffer);
	ext.ExecuteGeneratedCommands(m_commandBuffer, VK_TRUE, &info);
}

void CommandList_VK::Create(void* pNativeHandle, Device* pDevice)
{
	if (m_ownsCommandBuffer && m_commandBuffer != VK_NULL_HANDLE && m_pDevice)
		vkFreeCommandBuffers(m_pDevice->GetDevice(), m_pool, 1, &m_commandBuffer);
	m_pDevice = AsVK(pDevice);
	assert(m_pDevice);

	m_commandBuffer = static_cast<VkCommandBuffer>(pNativeHandle);
	m_ownsCommandBuffer = false;
}

void CommandList_VK::Attach(const CommandList_VK& source)
{
	const auto device = m_pDevice ? m_pDevice->GetDevice() : VK_NULL_HANDLE;
	if (m_ownsCommandBuffer && m_commandBuffer != VK_NULL_HANDLE) vkFreeCommandBuffers(device, m_pool, 1, &m_commandBuffer);
	if (m_preprocessBuffer != VK_NULL_HANDLE) vkDestroyBuffer(device, m_preprocessBuffer, nullptr);
	if (m_preprocessMemory != VK_NULL_HANDLE) vkFreeMemory(device, m_preprocessMemory, nullptr);
	if (m_nullXfb != VK_NULL_HANDLE) vkDestroyBuffer(device, m_nullXfb, nullptr);
	if (m_nullXfbMemory != VK_NULL_HANDLE) vkFreeMemory(device, m_nullXfbMemory, nullptr);

	m_pool = VK_NULL_HANDLE;
	m_commandBuffer = source.m_commandBuffer;
	m_ownsCommandBuffer = false;
	m_pDevice = source.m_pDevice;
	m_type = source.m_type;
	m_beginRendering = source.m_beginRendering;
	m_endRendering = source.m_endRendering;
	m_setPrimitiveTopology = source.m_setPrimitiveTopology;
	m_bindVertexBuffers2 = source.m_bindVertexBuffers2;
	m_beginLabel = source.m_beginLabel;
	m_endLabel = source.m_endLabel;
	m_insertLabel = source.m_insertLabel;
	m_preprocessBuffer = VK_NULL_HANDLE;
	m_preprocessMemory = VK_NULL_HANDLE;
	m_preprocessAddress = 0;
	m_preprocessSize = 0;
	m_preprocessLayout = VK_NULL_HANDLE;
	m_preprocessPipeline = VK_NULL_HANDLE;
	m_preprocessSequences = 0;
	m_preprocessRequired = 0;
	m_xfbActive = 0;
	m_nullXfb = VK_NULL_HANDLE;
	m_nullXfbMemory = VK_NULL_HANDLE;
	for (auto i = 0u; i < MaxStreamOutCount; ++i)
	{
		m_soBuffers[i] = VK_NULL_HANDLE;
		m_soOffsets[i] = 0;
		m_soSizes[i] = 0;
		m_soCounters[i] = VK_NULL_HANDLE;
		m_soCounterOffsets[i] = 0;
	}
}

void* CommandList_VK::GetNativeHandle()
{
	return m_commandBuffer;
}

void* CommandList_VK::GetDeviceNativeHandle() const
{
	return m_pDevice->GetDevice();
}

void* CommandList_VK::GetBackend()
{
	return this;
}

Device* CommandList_VK::GetDevice() const
{
	return m_pDevice;
}

VkCommandBuffer CommandList_VK::GetCommandBuffer() const
{
	return m_commandBuffer;
}

void CommandList_VK::CopyBufferToImage(Resource* pDst, uint32_t dstSubresource, Resource* pSrc,
	uint64_t srcOffset, uint32_t bufferRowLength, uint32_t bufferImageHeight) const
{
	recordOutside();
	const auto pDstResource = AsVK(pDst);
	const auto pSrcResource = AsVK(pSrc);
	uint32_t mip = 0, layer = 0, depth = 1;
	DecodeSubresource(pDstResource, dstSubresource, mip, layer, depth);

	VkBufferImageCopy region = {};
	region.bufferOffset = srcOffset;
	region.bufferRowLength = bufferRowLength;
	region.bufferImageHeight = bufferImageHeight;
	region.imageSubresource.aspectMask = CopyAspect(pDstResource->GetVkFormat());
	region.imageSubresource.mipLevel = mip;
	region.imageSubresource.baseArrayLayer = layer;
	region.imageSubresource.layerCount = 1;
	region.imageExtent = MipExtent(pDstResource, mip, depth);
	vkCmdCopyBufferToImage(m_commandBuffer, pSrcResource->GetBuffer(), pDstResource->GetImage(),
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void CommandList_VK::CopyImageToBuffer(Resource* pSrc, uint32_t mipLevel, uint32_t arrayLayer, Resource* pDst,
	uint64_t dstOffset, uint32_t bufferRowLength, uint32_t bufferImageHeight, VkImageAspectFlags aspect) const
{
	recordOutside();
	const auto pSrcResource = AsVK(pSrc);
	const auto pDstResource = AsVK(pDst);
	const auto depth = pSrcResource->Is3D() ? (max)(pSrcResource->GetDepthOrArray() >> mipLevel, 1u) : 1u;

	VkBufferImageCopy region = {};
	region.bufferOffset = dstOffset;
	region.bufferRowLength = bufferRowLength;
	region.bufferImageHeight = bufferImageHeight;
	region.imageSubresource.aspectMask = aspect;
	region.imageSubresource.mipLevel = mipLevel;
	region.imageSubresource.baseArrayLayer = arrayLayer;
	region.imageSubresource.layerCount = 1;
	region.imageExtent = MipExtent(pSrcResource, mipLevel, depth);
	vkCmdCopyImageToBuffer(m_commandBuffer, pSrcResource->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		pDstResource->GetBuffer(), 1, &region);
}

void CommandList_VK::resetState() const
{
	m_predicateBuffer = VK_NULL_HANDLE;
	m_predicateOffset = 0;
	m_predicateEqualZero = false;
	m_predicationActive = false;
	m_renderingBegun = false;
	m_hasDepth = false;
	m_colorCount = 0;
	m_depth = {};
	m_graphicsLayout = nullptr;
	m_computeLayout = nullptr;
	m_pipeline = nullptr;
	m_resourceHeap = nullptr;
	m_samplerHeap = nullptr;
	m_graphicsPushDirty = 0;
	m_computePushDirty = 0;
	m_renderWidth = 1;
	m_renderHeight = 1;
	m_deferredClears.clear();
	m_xfbActive = 0;
	for (auto i = 0u; i < MaxStreamOutCount; ++i)
	{
		m_soBuffers[i] = VK_NULL_HANDLE;
		m_soOffsets[i] = 0;
		m_soSizes[i] = 0;
		m_soCounters[i] = VK_NULL_HANDLE;
		m_soCounterOffsets[i] = 0;
	}
	if ((m_type == CommandListType::DIRECT || m_type == CommandListType::BUNDLE) &&
		m_pDevice && m_pDevice->HasSampleLocations())
		m_pDevice->GetExtProcs().SetSampleLocationsEnable(m_commandBuffer, VK_FALSE);
}

void CommandList_VK::endPredication() const
{
	if (!m_predicationActive) return;

	const auto end = m_pDevice->GetExtProcs().EndConditionalRendering;
	assert(end);
	end(m_commandBuffer);
	m_predicationActive = false;
}

void CommandList_VK::beginPredication() const
{
	if (m_predicateBuffer == VK_NULL_HANDLE || m_predicationActive) return;

	const auto begin = m_pDevice->GetExtProcs().BeginConditionalRendering;
	assert(begin);
	VkConditionalRenderingBeginInfoEXT info = {};
	info.sType = VK_STRUCTURE_TYPE_CONDITIONAL_RENDERING_BEGIN_INFO_EXT;
	info.buffer = m_predicateBuffer;
	info.offset = m_predicateOffset;
	info.flags = m_predicateEqualZero ? VK_CONDITIONAL_RENDERING_INVERTED_BIT_EXT : 0;
	begin(m_commandBuffer, &info);
	m_predicationActive = true;
}

void CommandList_VK::endRendering() const
{
	if (!m_renderingBegun) return;

	// Conditional rendering must sit entirely inside or entirely outside the pass.
	// Leave it off here. The next draw or copy turns it back on, so a barrier or
	// render-target rebind does not open an extra predication segment.
	endPredication();
	endTransformFeedback();
	assert(m_endRendering);
	m_endRendering(m_commandBuffer);
	m_renderingBegun = false;
}

bool CommandList_VK::sameRenderTargets(uint32_t numRenderTargetDescriptors, const Descriptor* pRenderTargetViews,
	const Descriptor* pDepthStencilView) const
{
	if (m_colorCount != numRenderTargetDescriptors) return false;

	for (auto i = 0u; i < numRenderTargetDescriptors; ++i)
	{
		const auto pView = static_cast<const ResourceView_VK*>(pRenderTargetViews[i]);
		if (!pView || m_colors[i].Rendering.imageView != pView->Target.Rendering.imageView) return false;
	}

	const auto pDepth = pDepthStencilView && *pDepthStencilView ?
		static_cast<const ResourceView_VK*>(*pDepthStencilView) : nullptr;
	if (!pDepth) return !m_hasDepth;

	return m_hasDepth && m_depth.Rendering.imageView == pDepth->Target.Rendering.imageView;
}

bool CommandList_VK::hasPendingClear() const
{
	for (auto i = 0u; i < m_colorCount; ++i)
		if (m_colors[i].PendingClear) return true;

	return m_hasDepth && m_depth.PendingClear;
}

void CommandList_VK::beginRendering() const
{
	if (m_renderingBegun) return;

	DeferredClear matched[MaxRenderTargetCount + 1];
	auto matchedCount = 0u;
	auto pending = 0u;
	for (auto i = 0u; i < m_deferredClears.size(); ++i)
	{
		const auto& clear = m_deferredClears[i];
		auto hit = false;
		if (!clear.Depth)
		{
			for (auto color = 0u; color < m_colorCount; ++color)
				hit = hit || m_colors[color].Rendering.imageView == clear.Attachment.Rendering.imageView;
		}
		else hit = m_hasDepth && m_depth.Rendering.imageView == clear.Attachment.Rendering.imageView;

		if (hit && matchedCount < MaxRenderTargetCount + 1) matched[matchedCount++] = clear;
		else m_deferredClears[pending++] = clear;
	}
	m_deferredClears.resize(pending);
	flushDeferredClears();

	endPredication();

	VkRenderingAttachmentInfo colors[MaxRenderTargetCount];
	for (auto i = 0u; i < m_colorCount; ++i)
	{
		colors[i] = m_colors[i].Rendering;
		colors[i].loadOp = attachmentLoadOp(m_colors[i]);
		if (m_colors[i].PendingClear)
		{
			colors[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colors[i].clearValue = m_colors[i].Rendering.clearValue;
			m_colors[i].PendingClear = 0;
		}
	}

	VkRenderingAttachmentInfo depth = m_depth.Rendering;
	VkRenderingAttachmentInfo stencil = m_depth.Rendering;
	depth.loadOp = m_hasDepth ? attachmentLoadOp(m_depth) : VK_ATTACHMENT_LOAD_OP_LOAD;
	stencil.loadOp = depth.loadOp;
	if (m_hasDepth && (m_depth.PendingClear & PENDING_CLEAR_DEPTH))
		depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	if (m_hasDepth && (m_depth.PendingClear & PENDING_CLEAR_STENCIL))
		stencil.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	if (m_hasDepth) m_depth.PendingClear = 0;
	const auto splitStencil = m_hasDepth && m_depth.HasStencil && depth.loadOp != stencil.loadOp;

	auto width = m_renderWidth;
	auto height = m_renderHeight;
	auto layers = 1u;
	if (m_colorCount)
	{
		width = m_colors[0].Width;
		height = m_colors[0].Height;
		layers = m_colors[0].Layers;
	}
	else if (m_hasDepth)
	{
		width = m_depth.Width;
		height = m_depth.Height;
		layers = m_depth.Layers;
	}

	VkRenderingInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	info.renderArea.extent = { width ? width : 1u, height ? height : 1u };
	info.layerCount = layers ? layers : 1u;
	info.colorAttachmentCount = m_colorCount;
	info.pColorAttachments = m_colorCount ? colors : nullptr;
	info.pNext = renderingPNext();
	info.pDepthAttachment = m_hasDepth ? &depth : nullptr;
	info.pStencilAttachment = !m_hasDepth || !m_depth.HasStencil ? nullptr : splitStencil ? &stencil : &depth;
	assert(m_beginRendering);
	m_beginRendering(m_commandBuffer, &info);
	m_renderingBegun = true;
	beginTransformFeedback();
	beginPredication();
	for (auto i = 0u; i < matchedCount; ++i)
	{
		if (matched[i].Depth)
			clearDepth(m_depth, matched[i].Flags, matched[i].DepthValue, matched[i].Stencil, 0, nullptr);
		else
		{
			for (auto color = 0u; color < m_colorCount; ++color)
				if (m_colors[color].Rendering.imageView == matched[i].Attachment.Rendering.imageView)
					clearColor(color, matched[i].Color, 0, nullptr);
		}
	}
}

void CommandList_VK::beginTransformFeedback() const
{
	if (m_xfbActive || !m_renderingBegun || !m_pipeline || !m_pipeline->XfbBuffers) return;
	const auto transformFeedback = m_pDevice && m_pDevice->HasTransformFeedback();
	assert(transformFeedback);
	if (!transformFeedback) return;

	const auto count = (min)(m_pipeline->XfbBuffers, MaxStreamOutCount);
	VkBuffer buffers[MaxStreamOutCount];
	VkDeviceSize offsets[MaxStreamOutCount];
	VkDeviceSize sizes[MaxStreamOutCount];
	for (auto i = 0u; i < count; ++i)
	{
		if (m_soBuffers[i])
		{
			buffers[i] = m_soBuffers[i];
			offsets[i] = m_soOffsets[i];
			sizes[i] = m_soSizes[i];
		}
		else
		{
			if (!ensureNullTransformFeedback()) return;
			buffers[i] = m_nullXfb;
			offsets[i] = 0;
			sizes[i] = 4;
		}
	}

	const auto& ext = m_pDevice->GetExtProcs();
	ext.BindTransformFeedbackBuffers(m_commandBuffer, 0, count, buffers, offsets, sizes);
	ext.BeginTransformFeedback(m_commandBuffer, 0, count, m_soCounters, m_soCounterOffsets);
	m_xfbActive = count;
}

void CommandList_VK::endTransformFeedback() const
{
	if (!m_xfbActive) return;

	const auto count = m_xfbActive;
	m_xfbActive = 0;
	const auto& ext = m_pDevice->GetExtProcs();
	ext.EndTransformFeedback(m_commandBuffer, 0, count, m_soCounters, m_soCounterOffsets);

	VkMemoryBarrier2 barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
	barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFORM_FEEDBACK_BIT_EXT;
	barrier.srcAccessMask = VK_ACCESS_2_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT;
	barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFORM_FEEDBACK_BIT_EXT;
	barrier.dstAccessMask = VK_ACCESS_2_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT;
	VkDependencyInfo dependency = {};
	dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependency.memoryBarrierCount = 1;
	dependency.pMemoryBarriers = &barrier;
	pipelineBarrier(dependency);
}

bool CommandList_VK::ensureNullTransformFeedback() const
{
	if (m_nullXfb != VK_NULL_HANDLE) return true;

	VkBufferCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.size = 4;
	info.usage = VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT;
	m_nullXfb = VK_NULL_HANDLE;
	const auto device = m_pDevice->GetDevice();
	const auto created = vkCreateBuffer(device, &info, nullptr, &m_nullXfb);
	if (created < 0)
	{
		cerr << "VkResult " << created << endl;
		m_nullXfb = VK_NULL_HANDLE;
		assert(created >= 0);

		return false;
	}

	VkMemoryRequirements requirements = {};
	vkGetBufferMemoryRequirements(device, m_nullXfb, &requirements);
	auto memoryType = m_pDevice->FindMemoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	if (memoryType == UINT32_MAX)
		memoryType = m_pDevice->FindMemoryType(requirements.memoryTypeBits, 0);
	if (memoryType == UINT32_MAX)
	{
		vkDestroyBuffer(device, m_nullXfb, nullptr);
		m_nullXfb = VK_NULL_HANDLE;
		assert(!"No memory type for a null transform-feedback buffer");

		return false;
	}

	VkMemoryAllocateInfo alloc = {};
	alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc.allocationSize = requirements.size;
	alloc.memoryTypeIndex = memoryType;
	const auto allocated = vkAllocateMemory(device, &alloc, nullptr, &m_nullXfbMemory);
	if (allocated < 0)
	{
		cerr << "VkResult " << allocated << endl;
		vkDestroyBuffer(device, m_nullXfb, nullptr);
		m_nullXfb = VK_NULL_HANDLE;
		assert(allocated >= 0);

		return false;
	}

	const auto bound = vkBindBufferMemory(device, m_nullXfb, m_nullXfbMemory, 0);
	if (bound < 0)
	{
		cerr << "VkResult " << bound << endl;
		vkDestroyBuffer(device, m_nullXfb, nullptr);
		vkFreeMemory(device, m_nullXfbMemory, nullptr);
		m_nullXfb = VK_NULL_HANDLE;
		m_nullXfbMemory = VK_NULL_HANDLE;
		assert(bound >= 0);

		return false;
	}

	return true;
}

void CommandList_VK::recordOutside() const
{
	endRendering();
	flushDeferredClears();
	beginPredication();
}

const void* CommandList_VK::renderingPNext() const
{
	return nullptr;
}

void CommandList_VK::bindHeap(DescriptorHeap heap) const
{
	const auto pHeap = static_cast<DescriptorHeap_VK*>(heap);
	if (!pHeap) return;

	VkBindHeapInfoEXT info = {};
	info.sType = VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT;
	info.heapRange.address = pHeap->GetAddress();
	info.heapRange.size = pHeap->GetSize();
	info.reservedRangeSize = pHeap->GetReservedSize();
	const auto& ext = m_pDevice->GetExtProcs();
	if (pHeap->GetType() == SAMPLER_HEAP)
	{
		ext.BindSamplerHeap(m_commandBuffer, &info);
		m_samplerHeap = heap;
	}
	else
	{
		ext.BindResourceHeap(m_commandBuffer, &info);
		m_resourceHeap = heap;
	}
}

void CommandList_VK::pushData(uint32_t offset, const void* pData, uint32_t size) const
{
	VkPushDataInfoEXT info = {};
	info.sType = VK_STRUCTURE_TYPE_PUSH_DATA_INFO_EXT;
	info.offset = offset;
	info.data.address = pData;
	info.data.size = size;
	m_pDevice->GetExtProcs().PushData(m_commandBuffer, &info);
}

void CommandList_VK::writePush(bool graphics, uint32_t rootIndex, uint32_t offset, const void* pData, uint32_t size) const
{
	assert(pData && size);
	assert(rootIndex < MaxRootParameters);
	assert(offset + size <= MaxPushDataBytes);
	auto* buffer = graphics ? m_graphicsPushData : m_computePushData;
	auto& dirty = graphics ? m_graphicsPushDirty : m_computePushDirty;
	memcpy(buffer + offset, pData, size);
	dirty |= 1ull << rootIndex;
}

void CommandList_VK::flushPush(bool graphics) const
{
	auto& dirty = graphics ? m_graphicsPushDirty : m_computePushDirty;
	if (!dirty) return;

	const auto* layout = graphics ? m_graphicsLayout : m_computeLayout;
	if (!layout)
	{
		dirty = 0;

		return;
	}

	const auto* buffer = graphics ? m_graphicsPushData : m_computePushData;
	const auto& roots = layout->RootParameters;
	const auto count = static_cast<uint32_t>(roots.size());
	assert(count <= MaxRootParameters);

	auto hasRange = false;
	auto rangeBegin = 0u;
	auto rangeEnd = 0u;
	for (auto i = 0u; i < count; ++i)
	{
		if ((dirty & (1ull << i)) == 0) continue;

		const auto begin = roots[i].PushByteOffset;
		const auto end = begin + roots[i].SizeInBytes;
		if (!hasRange)
		{
			rangeBegin = begin;
			rangeEnd = end;
			hasRange = true;
		}
		else if (begin <= rangeEnd)
		{
			rangeEnd = (max)(rangeEnd, end);
		}
		else
		{
			const auto alignedBegin = rangeBegin & ~3u;
			const auto alignedEnd = (rangeEnd + 3u) & ~3u;
			pushData(alignedBegin, buffer + alignedBegin, alignedEnd - alignedBegin);
			rangeBegin = begin;
			rangeEnd = end;
		}
	}
	if (hasRange)
	{
		const auto alignedBegin = rangeBegin & ~3u;
		const auto alignedEnd = (rangeEnd + 3u) & ~3u;
		pushData(alignedBegin, buffer + alignedBegin, alignedEnd - alignedBegin);
	}
	dirty = 0;
}

void CommandList_VK::setDescriptorTable(const PipelineLayoutObject* pLayout, uint32_t index, uint32_t descriptorIndex) const
{
	assert(pLayout);
	assert(index < pLayout->RootParameters.size());
	const auto value = descriptorIndex;
	const auto graphics = pLayout == m_graphicsLayout;
	writePush(graphics, index, pLayout->RootParameters[index].PushByteOffset, &value, sizeof(value));
}

void CommandList_VK::setConstants(const PipelineLayoutObject* pLayout, uint32_t index, uint32_t num32BitValues,
	const void* pData, uint32_t destOffsetIn32BitValues) const
{
	assert(pLayout);
	assert(index < pLayout->RootParameters.size());
	const auto offset = pLayout->RootParameters[index].PushByteOffset + destOffsetIn32BitValues * static_cast<uint32_t>(sizeof(uint32_t));
	const auto graphics = pLayout == m_graphicsLayout;
	writePush(graphics, index, offset, pData, num32BitValues * static_cast<uint32_t>(sizeof(uint32_t)));
}

void CommandList_VK::setRootAddress(const PipelineLayoutObject* pLayout, uint32_t index, uint64_t address) const
{
	assert(pLayout);
	assert(index < pLayout->RootParameters.size());
	const auto graphics = pLayout == m_graphicsLayout;
	writePush(graphics, index, pLayout->RootParameters[index].PushByteOffset, &address, sizeof(address));
}

bool CommandList_VK::ensurePreprocess(VkDeviceSize size) const
{
	if (!size || (m_preprocessBuffer != VK_NULL_HANDLE && m_preprocessSize >= size)) return true;

	const auto device = m_pDevice->GetDevice();
	const auto allocate = m_preprocessSize ? (max)(size, m_preprocessSize * 2) : size;
	if (m_preprocessBuffer != VK_NULL_HANDLE) vkDestroyBuffer(device, m_preprocessBuffer, nullptr);
	if (m_preprocessMemory != VK_NULL_HANDLE) vkFreeMemory(device, m_preprocessMemory, nullptr);
	m_preprocessBuffer = VK_NULL_HANDLE;
	m_preprocessMemory = VK_NULL_HANDLE;

	VkBufferUsageFlags2CreateInfo usage = {};
	usage.sType = VK_STRUCTURE_TYPE_BUFFER_USAGE_FLAGS_2_CREATE_INFO;
	usage.usage = VK_BUFFER_USAGE_2_PREPROCESS_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

	VkBufferCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.pNext = &usage;
	info.size = allocate;
	XUSG_VK_CHECK(vkCreateBuffer(device, &info, nullptr, &m_preprocessBuffer), cerr, false);

	VkMemoryRequirements requirements = {};
	vkGetBufferMemoryRequirements(device, m_preprocessBuffer, &requirements);
	VkMemoryAllocateFlagsInfo flags = {};
	flags.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
	flags.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
	VkMemoryAllocateInfo alloc = {};
	alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc.pNext = &flags;
	alloc.allocationSize = requirements.size;
	alloc.memoryTypeIndex = m_pDevice->FindMemoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	XUSG_VK_CHECK(vkAllocateMemory(device, &alloc, nullptr, &m_preprocessMemory), cerr, false);
	XUSG_VK_CHECK(vkBindBufferMemory(device, m_preprocessBuffer, m_preprocessMemory, 0), cerr, false);

	VkBufferDeviceAddressInfo address = {};
	address.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	address.buffer = m_preprocessBuffer;
	m_preprocessAddress = vkGetBufferDeviceAddress(device, &address);
	m_preprocessSize = allocate;

	return true;
}

void CommandList_VK::clearColor(uint32_t attachment, const float color[4], uint32_t numRects, const RectRange* pRects) const
{
	VkClearAttachment clear = {};
	clear.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	clear.colorAttachment = attachment;
	memcpy(clear.clearValue.color.float32, color, sizeof(float) * 4);

	const auto rectCount = numRects ? numRects : 1u;
	if (m_clearRects.size() < rectCount) m_clearRects.resize(rectCount);
	if (!numRects)
	{
		m_clearRects[0].rect.offset = {};
		m_clearRects[0].rect.extent = { m_colors[attachment].Width, m_colors[attachment].Height };
		m_clearRects[0].baseArrayLayer = 0;
		m_clearRects[0].layerCount = m_colors[attachment].Layers;
	}
	else
	{
		for (auto i = 0u; i < numRects; ++i)
		{
			m_clearRects[i].rect.offset = { static_cast<int32_t>(pRects[i].Left), static_cast<int32_t>(pRects[i].Top) };
			m_clearRects[i].rect.extent.width = static_cast<uint32_t>(pRects[i].Right - pRects[i].Left);
			m_clearRects[i].rect.extent.height = static_cast<uint32_t>(pRects[i].Bottom - pRects[i].Top);
			m_clearRects[i].baseArrayLayer = 0;
			m_clearRects[i].layerCount = m_colors[attachment].Layers;
		}
	}

	vkCmdClearAttachments(m_commandBuffer, 1, &clear, rectCount, m_clearRects.data());
}

void CommandList_VK::clearOutside(const RenderAttachment& attachment, bool depth, ClearFlag flags, float depthValue,
	uint8_t stencil, const float color[4], uint32_t numRects, const RectRange* pRects) const
{
	assert(!m_renderingBegun);
	const auto restart = m_predicationActive;
	endPredication();

	VkRenderingAttachmentInfo renderingAttachment = attachment.Rendering;
	renderingAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	renderingAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	VkRenderingInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	info.renderArea.extent = { attachment.Width ? attachment.Width : 1u, attachment.Height ? attachment.Height : 1u };
	info.layerCount = attachment.Layers ? attachment.Layers : 1u;
	if (!depth)
	{
		info.colorAttachmentCount = 1;
		info.pColorAttachments = &renderingAttachment;
	}
	else
	{
		info.pDepthAttachment = (flags & ClearFlag::DEPTH) != ClearFlag::NONE ? &renderingAttachment : nullptr;
		info.pStencilAttachment = attachment.HasStencil && (flags & ClearFlag::STENCIL) != ClearFlag::NONE ? &renderingAttachment : nullptr;
	}

	assert(m_beginRendering && m_endRendering);
	m_beginRendering(m_commandBuffer, &info);
	if (restart) beginPredication();
	if (depth) clearDepth(attachment, flags, depthValue, stencil, numRects, pRects);
	else
	{
		VkClearAttachment clear = {};
		clear.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		if (color) memcpy(clear.clearValue.color.float32, color, sizeof(float) * 4);
		const auto rectCount = numRects ? numRects : 1u;
		if (m_clearRects.size() < rectCount) m_clearRects.resize(rectCount);
		if (!numRects)
		{
			m_clearRects[0].rect.offset = {};
			m_clearRects[0].rect.extent = { attachment.Width, attachment.Height };
			m_clearRects[0].baseArrayLayer = 0;
			m_clearRects[0].layerCount = attachment.Layers ? attachment.Layers : 1u;
		}
		else
		{
			for (auto i = 0u; i < numRects; ++i)
			{
				m_clearRects[i].rect.offset = { static_cast<int32_t>(pRects[i].Left), static_cast<int32_t>(pRects[i].Top) };
				m_clearRects[i].rect.extent.width = static_cast<uint32_t>(pRects[i].Right - pRects[i].Left);
				m_clearRects[i].rect.extent.height = static_cast<uint32_t>(pRects[i].Bottom - pRects[i].Top);
				m_clearRects[i].baseArrayLayer = 0;
				m_clearRects[i].layerCount = attachment.Layers ? attachment.Layers : 1u;
			}
		}
		vkCmdClearAttachments(m_commandBuffer, 1, &clear, rectCount, m_clearRects.data());
	}
	if (restart) endPredication();
	m_endRendering(m_commandBuffer);
	if (restart) beginPredication();
}

void CommandList_VK::flushDeferredClears() const
{
	if (m_deferredClears.empty() || m_renderingBegun) return;

	const auto clears = m_deferredClears;
	m_deferredClears.clear();
	beginPredication();
	for (const auto& clear : clears)
		clearOutside(clear.Attachment, clear.Depth, clear.Flags, clear.DepthValue, clear.Stencil, clear.Color, 0, nullptr);
	endPredication();
}

bool CommandList_VK::imageBound(VkImage image) const
{
	if (!m_renderingBegun || image == VK_NULL_HANDLE) return false;

	for (auto i = 0u; i < m_colorCount; ++i)
		if (m_colors[i].Resource && m_colors[i].Resource->GetImage() == image) return true;

	return m_hasDepth && m_depth.Resource && m_depth.Resource->GetImage() == image;
}

void CommandList_VK::clearDepth(const RenderAttachment& attachment, ClearFlag clearFlags, float depth, uint8_t stencil,
	uint32_t numRects, const RectRange* pRects) const
{
	VkClearAttachment clear = {};
	if ((clearFlags & ClearFlag::DEPTH) != ClearFlag::NONE) clear.aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
	if ((clearFlags & ClearFlag::STENCIL) != ClearFlag::NONE) clear.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	clear.clearValue.depthStencil.depth = depth;
	clear.clearValue.depthStencil.stencil = stencil;
	if (!clear.aspectMask) return;

	const auto rectCount = numRects ? numRects : 1u;
	if (m_clearRects.size() < rectCount) m_clearRects.resize(rectCount);
	if (!numRects)
	{
		m_clearRects[0].rect.offset = {};
		m_clearRects[0].rect.extent = { attachment.Width, attachment.Height };
		m_clearRects[0].baseArrayLayer = 0;
		m_clearRects[0].layerCount = attachment.Layers;
	}
	else
	{
		for (auto i = 0u; i < numRects; ++i)
		{
			m_clearRects[i].rect.offset = { static_cast<int32_t>(pRects[i].Left), static_cast<int32_t>(pRects[i].Top) };
			m_clearRects[i].rect.extent.width = static_cast<uint32_t>(pRects[i].Right - pRects[i].Left);
			m_clearRects[i].rect.extent.height = static_cast<uint32_t>(pRects[i].Bottom - pRects[i].Top);
			m_clearRects[i].baseArrayLayer = 0;
			m_clearRects[i].layerCount = attachment.Layers;
		}
	}

	vkCmdClearAttachments(m_commandBuffer, 1, &clear, rectCount, m_clearRects.data());
}

void CommandList_VK::clearUnorderedAccessView(const Descriptor& descriptor, const VkClearColorValue& color, uint32_t numRects) const
{
	recordOutside();
	assert(numRects == 0 && "ClearUnorderedAccessView rects are not implemented on Vulkan");
	assert(descriptor);
	const auto pView = static_cast<const ResourceView_VK*>(descriptor);
	assert(pView->Type == ResourceViewType::UAV);
	auto pResource = AsVK(pView->pResource);
	assert(pResource);

	auto regionCount = 1u;
	if (m_clearRegions.empty()) m_clearRegions.resize(1);
	if (!pView->IsImage) m_clearRegions[0] = { 0, ResourceState::COMMON };
	else if (pResource->Is3D())
	{
		if (m_clearRegions.empty()) m_clearRegions.resize(1);
		m_clearRegions[0] = { pView->ViewInfo.subresourceRange.baseMipLevel, ResourceState::COMMON };
	}
	else
	{
		const auto mips = (max)(pResource->GetMipLevels(), 1u);
		const auto& range = pView->ViewInfo.subresourceRange;
		const auto levels = range.levelCount ? range.levelCount : 1u;
		const auto layers = range.layerCount ? range.layerCount : 1u;
		regionCount = levels * layers;
		if (m_clearRegions.size() < regionCount) m_clearRegions.resize(regionCount);
		auto index = 0u;
		for (auto layer = 0u; layer < layers; ++layer)
			for (auto level = 0u; level < levels; ++level)
				m_clearRegions[index++] = { range.baseMipLevel + level + (range.baseArrayLayer + layer) * mips, ResourceState::COMMON };
	}

	for (auto i = 0u; i < regionCount; ++i)
		m_clearRegions[i].State = pResource->GetResourceState(m_clearRegions[i].Subresource);
	auto sameState = true;
	for (auto i = 1u; i < regionCount; ++i)
		sameState = sameState && m_clearRegions[i].State == m_clearRegions[0].State;

	const auto state = m_clearRegions[0].State;
	const auto layout = pView->IsImage ? pResource->ImageLayout(state) : VK_IMAGE_LAYOUT_UNDEFINED;
	const auto clearInPlace = sameState && (!pView->IsImage || layout == VK_IMAGE_LAYOUT_GENERAL ||
		layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	if (clearInPlace)
	{
		const auto srcStage = GetVKStage(state);
		const auto srcAccess = GetVKAccess(state);
		VkDependencyInfo dependency = {};
		dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		VkImageMemoryBarrier2 image = {};
		VkBufferMemoryBarrier2 buffer = {};
		if (pView->IsImage)
		{
			auto range = pView->ViewInfo.subresourceRange;
			if (!range.levelCount) range.levelCount = 1;
			if (!range.layerCount) range.layerCount = 1;
			image.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
			image.srcStageMask = srcStage;
			image.srcAccessMask = srcAccess;
			image.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			image.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			image.oldLayout = layout;
			image.newLayout = layout;
			image.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			image.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			image.image = pResource->GetImage();
			image.subresourceRange = range;
			dependency.imageMemoryBarrierCount = 1;
			dependency.pImageMemoryBarriers = &image;
			pipelineBarrier(dependency);
			vkCmdClearColorImage(m_commandBuffer, pResource->GetImage(), layout, &color, 1, &range);
			image.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			image.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			image.dstStageMask = srcStage;
			image.dstAccessMask = srcAccess;
			pipelineBarrier(dependency);
		}
		else
		{
			const auto base = pResource->GetVirtualAddress();
			assert(pView->Address >= base);
			const auto offset = pView->Address - base;
			buffer.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
			buffer.srcStageMask = srcStage;
			buffer.srcAccessMask = srcAccess;
			buffer.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			buffer.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			buffer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			buffer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			buffer.buffer = pResource->GetBuffer();
			buffer.offset = offset;
			buffer.size = pView->Range;
			dependency.bufferMemoryBarrierCount = 1;
			dependency.pBufferMemoryBarriers = &buffer;
			pipelineBarrier(dependency);
			const auto stride = pView->Stride ? pView->Stride : 4u;
			assert(pView->Range && (offset % 4) == 0 && (pView->Range % 4) == 0 && (stride % 4) == 0);
			assert(offset + pView->Range <= pResource->GetByteSize());
			if (pView->IsTexelBuffer)
				assert((pView->ViewInfo.format == VK_FORMAT_R32_UINT || pView->ViewInfo.format == VK_FORMAT_R32_SINT ||
					pView->ViewInfo.format == VK_FORMAT_R32_SFLOAT) &&
					"ClearUnorderedAccessView typed buffer format is not implemented on Vulkan");
			const auto components = (min)(stride / 4, 4u);
			for (auto i = 1u; i < components; ++i)
				assert(color.uint32[i] == color.uint32[0] && "ClearUnorderedAccessView buffer pattern is wider than 4 bytes");
			vkCmdFillBuffer(m_commandBuffer, pResource->GetBuffer(), offset, pView->Range, color.uint32[0]);
			buffer.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			buffer.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			buffer.dstStageMask = srcStage;
			buffer.dstAccessMask = srcAccess;
			pipelineBarrier(dependency);
		}

		return;
	}

	for (auto i = 0u; i < regionCount; ++i)
	{
		ResourceBarrier barrier;
		pResource->SetBarrier(&barrier, ResourceState::COPY_DEST, 0, m_clearRegions[i].Subresource);
	}
	submitClearBarrier(pResource, regionCount, true);

	if (pView->IsImage)
	{
		auto range = pView->ViewInfo.subresourceRange;
		if (!range.levelCount) range.levelCount = 1;
		if (!range.layerCount) range.layerCount = 1;
		vkCmdClearColorImage(m_commandBuffer, pResource->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &color, 1, &range);
	}
	else
	{
		const auto base = pResource->GetVirtualAddress();
		assert(pView->Address >= base);
		const auto offset = pView->Address - base;
		const auto stride = pView->Stride ? pView->Stride : 4u;
		assert(pView->Range && (offset % 4) == 0 && (pView->Range % 4) == 0 && (stride % 4) == 0);
		assert(offset + pView->Range <= pResource->GetByteSize());
		if (pView->IsTexelBuffer)
			assert((pView->ViewInfo.format == VK_FORMAT_R32_UINT || pView->ViewInfo.format == VK_FORMAT_R32_SINT ||
				pView->ViewInfo.format == VK_FORMAT_R32_SFLOAT) &&
				"ClearUnorderedAccessView typed buffer format is not implemented on Vulkan");
		const auto components = (min)(stride / 4, 4u);
		for (auto i = 1u; i < components; ++i)
			assert(color.uint32[i] == color.uint32[0] && "ClearUnorderedAccessView buffer pattern is wider than 4 bytes");
		vkCmdFillBuffer(m_commandBuffer, pResource->GetBuffer(), offset, pView->Range, color.uint32[0]);
	}

	for (auto i = 0u; i < regionCount; ++i)
	{
		ResourceBarrier barrier;
		pResource->SetBarrier(&barrier, m_clearRegions[i].State, 0, m_clearRegions[i].Subresource);
	}
	submitClearBarrier(pResource, regionCount, false);
}

void CommandList_VK::loadAttachment(RenderAttachment& attachment, const ResourceView_VK* pView) const
{
	const auto& target = pView->Target;
	attachment.Rendering = target.Rendering;
	attachment.Resource = AsVK(pView->pResource);
	attachment.Width = target.Width;
	attachment.Height = target.Height;
	attachment.BaseMip = target.BaseMip;
	attachment.MipCount = target.MipCount;
	attachment.BaseLayer = target.BaseLayer;
	attachment.Layers = target.Layers;
	attachment.Subresource = target.Subresource;
	attachment.SingleSubresource = target.SingleSubresource;
	attachment.HasStencil = target.HasStencil;
	attachment.PendingClear = 0;
}

VkAttachmentLoadOp CommandList_VK::attachmentLoadOp(const RenderAttachment& attachment) const
{
	const auto resource = attachment.Resource;
	if (!resource || !resource->HasPendingDiscard()) return attachment.Rendering.loadOp;

	return attachment.SingleSubresource ? resource->ConsumeDiscardLoadOp(attachment.Subresource) :
		resource->ConsumeDiscardLoadOp(attachment.BaseMip, attachment.MipCount, attachment.BaseLayer, attachment.Layers);
}

void CommandList_VK::submitClearBarrier(Resource_VK* pResource, uint32_t regionCount, bool toCopy) const
{
	m_buffers.clear();
	m_images.clear();
	for (auto i = 0u; i < regionCount; ++i)
	{
		const auto& region = m_clearRegions[i];
		const auto src = toCopy ? region.State : ResourceState::COPY_DEST;
		const auto dst = toCopy ? ResourceState::COPY_DEST : region.State;
		const auto undefined = pResource->IsImage() && pResource->IsUndefinedLayout(region.Subresource);
		if (!undefined && src == dst) continue;

		if (pResource->IsImage())
		{
			VkImageMemoryBarrier2 image = {};
			image.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
			image.srcStageMask = GetVKStage(src);
			image.srcAccessMask = GetVKAccess(src);
			image.dstStageMask = GetVKStage(dst);
			image.dstAccessMask = GetVKAccess(dst);
			image.oldLayout = undefined ? VK_IMAGE_LAYOUT_UNDEFINED : pResource->ImageLayout(src);
			if (undefined) image.srcAccessMask = VK_ACCESS_2_NONE;
			image.newLayout = pResource->ImageLayout(dst);
			image.image = pResource->GetImage();
			image.subresourceRange.aspectMask = GetVKImageAspect(GetFormatFromVK(pResource->GetVkFormat()));
			uint32_t mip = 0, layer = 0, depth = 1;
			DecodeSubresource(pResource, region.Subresource, mip, layer, depth);
			image.subresourceRange.baseMipLevel = mip;
			image.subresourceRange.levelCount = 1;
			image.subresourceRange.baseArrayLayer = layer;
			image.subresourceRange.layerCount = 1;
			pResource->AppendImageBarrier(image, m_images);
		}
		else
		{
			VkBufferMemoryBarrier2 buffer = {};
			buffer.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
			buffer.srcStageMask = GetVKStage(src);
			buffer.srcAccessMask = GetVKAccess(src);
			buffer.dstStageMask = GetVKStage(dst);
			buffer.dstAccessMask = GetVKAccess(dst);
			buffer.buffer = pResource->GetBuffer();
			buffer.size = VK_WHOLE_SIZE;
			m_buffers.push_back(buffer);
		}
	}

	if (m_buffers.empty() && m_images.empty()) return;

	VkDependencyInfo dependency = {};
	dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependency.bufferMemoryBarrierCount = static_cast<uint32_t>(m_buffers.size());
	dependency.pBufferMemoryBarriers = m_buffers.empty() ? nullptr : m_buffers.data();
	dependency.imageMemoryBarrierCount = static_cast<uint32_t>(m_images.size());
	dependency.pImageMemoryBarriers = m_images.empty() ? nullptr : m_images.data();
	pipelineBarrier(dependency);
}

void CommandList_VK::submitBarrierBatch(VkMemoryBarrier2& memory, vector<VkImageMemoryBarrier2>& images) const
{
	if (!memory.srcStageMask && !memory.dstStageMask && images.empty()) return;

	VkDependencyInfo dependency = {};
	dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	if (memory.srcStageMask || memory.dstStageMask)
	{
		dependency.memoryBarrierCount = 1;
		dependency.pMemoryBarriers = &memory;
	}
	dependency.imageMemoryBarrierCount = static_cast<uint32_t>(images.size());
	dependency.pImageMemoryBarriers = images.empty() ? nullptr : images.data();
	pipelineBarrier(dependency);
	memory.srcStageMask = 0;
	memory.srcAccessMask = 0;
	memory.dstStageMask = 0;
	memory.dstAccessMask = 0;
	images.clear();
}

void CommandList_VK::pipelineBarrier(const VkDependencyInfo& dependency) const
{
	const auto memoryCount = dependency.memoryBarrierCount;
	const auto bufferCount = dependency.bufferMemoryBarrierCount;
	const auto imageCount = dependency.imageMemoryBarrierCount;
	if (!memoryCount && !bufferCount && !imageCount) return;
	if (m_pDevice && m_pDevice->HasSynchronization2())
	{
		vkCmdPipelineBarrier2(m_commandBuffer, &dependency);

		return;
	}

	m_legacySrcStages.clear();
	m_legacyDstStages.clear();
	for (auto i = 0u; i < memoryCount; ++i)
	{
		VkPipelineStageFlags source = 0;
		VkPipelineStageFlags dest = 0;
		LegacyStages(dependency.pMemoryBarriers[i].srcStageMask, dependency.pMemoryBarriers[i].dstStageMask, source, dest);
		auto found = false;
		for (auto group = 0u; group < m_legacySrcStages.size(); ++group)
			found = found || (m_legacySrcStages[group] == source && m_legacyDstStages[group] == dest);
		if (found) continue;
		m_legacySrcStages.push_back(source);
		m_legacyDstStages.push_back(dest);
	}
	for (auto i = 0u; i < bufferCount; ++i)
	{
		VkPipelineStageFlags source = 0;
		VkPipelineStageFlags dest = 0;
		LegacyStages(dependency.pBufferMemoryBarriers[i].srcStageMask, dependency.pBufferMemoryBarriers[i].dstStageMask, source, dest);
		auto found = false;
		for (auto group = 0u; group < m_legacySrcStages.size(); ++group)
			found = found || (m_legacySrcStages[group] == source && m_legacyDstStages[group] == dest);
		if (found) continue;
		m_legacySrcStages.push_back(source);
		m_legacyDstStages.push_back(dest);
	}
	for (auto i = 0u; i < imageCount; ++i)
	{
		VkPipelineStageFlags source = 0;
		VkPipelineStageFlags dest = 0;
		LegacyStages(dependency.pImageMemoryBarriers[i].srcStageMask, dependency.pImageMemoryBarriers[i].dstStageMask, source, dest);
		auto found = false;
		for (auto group = 0u; group < m_legacySrcStages.size(); ++group)
			found = found || (m_legacySrcStages[group] == source && m_legacyDstStages[group] == dest);
		if (found) continue;
		m_legacySrcStages.push_back(source);
		m_legacyDstStages.push_back(dest);
	}

	for (auto group = 0u; group < m_legacySrcStages.size(); ++group)
	{
		const auto source = m_legacySrcStages[group];
		const auto dest = m_legacyDstStages[group];
		m_legacyMemories.clear();
		m_legacyBuffers.clear();
		m_legacyImages.clear();
		for (auto i = 0u; i < memoryCount; ++i)
		{
			const auto& barrier = dependency.pMemoryBarriers[i];
			VkPipelineStageFlags barrierSource = 0;
			VkPipelineStageFlags barrierDest = 0;
			LegacyStages(barrier.srcStageMask, barrier.dstStageMask, barrierSource, barrierDest);
			if (barrierSource != source || barrierDest != dest) continue;

			VkMemoryBarrier memory = {};
			memory.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			memory.srcAccessMask = static_cast<VkAccessFlags>(barrier.srcAccessMask);
			memory.dstAccessMask = static_cast<VkAccessFlags>(barrier.dstAccessMask);
			m_legacyMemories.push_back(memory);
		}
		for (auto i = 0u; i < bufferCount; ++i)
		{
			const auto& barrier = dependency.pBufferMemoryBarriers[i];
			VkPipelineStageFlags barrierSource = 0;
			VkPipelineStageFlags barrierDest = 0;
			LegacyStages(barrier.srcStageMask, barrier.dstStageMask, barrierSource, barrierDest);
			if (barrierSource != source || barrierDest != dest) continue;

			VkBufferMemoryBarrier buffer = {};
			buffer.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			buffer.srcAccessMask = static_cast<VkAccessFlags>(barrier.srcAccessMask);
			buffer.dstAccessMask = static_cast<VkAccessFlags>(barrier.dstAccessMask);
			buffer.srcQueueFamilyIndex = barrier.srcQueueFamilyIndex;
			buffer.dstQueueFamilyIndex = barrier.dstQueueFamilyIndex;
			buffer.buffer = barrier.buffer;
			buffer.offset = barrier.offset;
			buffer.size = barrier.size;
			m_legacyBuffers.push_back(buffer);
		}
		for (auto i = 0u; i < imageCount; ++i)
		{
			const auto& barrier = dependency.pImageMemoryBarriers[i];
			VkPipelineStageFlags barrierSource = 0;
			VkPipelineStageFlags barrierDest = 0;
			LegacyStages(barrier.srcStageMask, barrier.dstStageMask, barrierSource, barrierDest);
			if (barrierSource != source || barrierDest != dest) continue;

			VkImageMemoryBarrier image = {};
			image.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			image.srcAccessMask = static_cast<VkAccessFlags>(barrier.srcAccessMask);
			image.dstAccessMask = static_cast<VkAccessFlags>(barrier.dstAccessMask);
			image.oldLayout = barrier.oldLayout;
			image.newLayout = barrier.newLayout;
			image.srcQueueFamilyIndex = barrier.srcQueueFamilyIndex;
			image.dstQueueFamilyIndex = barrier.dstQueueFamilyIndex;
			image.image = barrier.image;
			image.subresourceRange = barrier.subresourceRange;
			m_legacyImages.push_back(image);
		}

		vkCmdPipelineBarrier(m_commandBuffer, source, dest, 0,
			static_cast<uint32_t>(m_legacyMemories.size()), m_legacyMemories.empty() ? nullptr : m_legacyMemories.data(),
			static_cast<uint32_t>(m_legacyBuffers.size()), m_legacyBuffers.empty() ? nullptr : m_legacyBuffers.data(),
			static_cast<uint32_t>(m_legacyImages.size()), m_legacyImages.empty() ? nullptr : m_legacyImages.data());
	}
}

//--------------------------------------------------------------------------------------

CommandQueue_VK::CommandQueue_VK() :
	m_pDevice(nullptr),
	m_queue(VK_NULL_HANDLE),
	m_family(0),
	m_queueIndex(0),
	m_registered(false),
	m_ownsSlot(false),
	m_sparseQueue(false),
	m_sparseWaited(0)
{
}

CommandQueue_VK::~CommandQueue_VK()
{
	if (m_registered) UnregisterQueue(m_queue);
	if (m_ownsSlot && m_pDevice) m_pDevice->ReleaseQueue(m_family, m_queueIndex);
}

bool CommandQueue_VK::Create(Device* pDevice, CommandListType type, CommandQueueFlag flags,
	int32_t priority, uint32_t nodeMask, const wchar_t* name)
{
	assert(type != CommandListType::VIDEO_DECODE && type != CommandListType::VIDEO_PROCESS && type != CommandListType::VIDEO_ENCODE);
	(void)flags;
	(void)priority;
	(void)nodeMask;
	if (m_ownsSlot)
	{
		if (m_pDevice) m_pDevice->ReleaseQueue(m_family, m_queueIndex);
		m_ownsSlot = false;
	}
	if (m_registered) UnregisterQueue(m_queue);

	m_pDevice = AsVK(pDevice);
	assert(m_pDevice);
	const auto device = m_pDevice->GetDevice();
	XUSG_N_RETURN(m_pDevice->AcquireQueue(type, m_family, m_queueIndex), false);
	m_ownsSlot = true;
	vkGetDeviceQueue(device, m_family, m_queueIndex, &m_queue);
	m_sparseQueue = false;
	m_sparseWaited = 0;
	uint32_t familyCount = 0;
	const auto physicalDevice = m_pDevice->GetPhysicalDevice();
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, nullptr);
	if (m_family < familyCount)
	{
		vector<VkQueueFamilyProperties> families(familyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, families.data());
		m_sparseQueue = (families[m_family].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) != 0;
	}

	QueueNative native = {};
	native.Queue = m_queue;
	native.Device = m_pDevice;
	native.Family = m_family;
	RegisterQueue(native);
	m_registered = true;
	m_pDevice->SetDebugName(VK_OBJECT_TYPE_QUEUE, reinterpret_cast<uintptr_t>(m_queue), name);

	return true;
}

bool CommandQueue_VK::SubmitCommandLists(uint32_t numCommandLists, CommandList* const* ppCommandLists,
	const Semaphore* pWaits, uint32_t numWaits, const Semaphore* pSignals, uint32_t numSignals) const
{
	// Explicit submit: fold deferred Execute first. Pending Present stays on the
	// worker / queue mutex - do not sync-flush it here (avoids vk5-style gaps).
	if (!m_deferredLists.empty())
		XUSG_N_RETURN(flushDeferred(nullptr, 0, nullptr, 0), false);

	return submit(numCommandLists, ppCommandLists, pWaits, numWaits, pSignals, numSignals);
}

bool CommandQueue_VK::SubmitCommandList(CommandList* pCommandList,
	const Semaphore* pWaits, uint32_t numWaits, const Semaphore* pSignals, uint32_t numSignals) const
{
	if (!m_deferredLists.empty())
		XUSG_N_RETURN(flushDeferred(nullptr, 0, nullptr, 0), false);

	return submit(1, &pCommandList, pWaits, numWaits, pSignals, numSignals);
}

bool CommandQueue_VK::Wait(Fence* pFence, uint64_t value) const
{
	assert(pFence);
	VkSemaphoreSubmitInfo waits[2] = {};
	waits[0].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	waits[0].semaphore = (VkSemaphore)pFence->GetNativeHandle();
	waits[0].value = value;
	waits[0].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	auto waitCount = 1u;
	if (consumeSparseWait(waits[1])) ++waitCount;

	VkSubmitInfo2 info = {};
	info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	info.waitSemaphoreInfoCount = waitCount;
	info.pWaitSemaphoreInfos = waits;
	VkSemaphore waitSemaphores[2];
	VkPipelineStageFlags waitStages[2];
	uint64_t waitValues[2];

	assert(m_pDevice);
	XUSG_N_RETURN(m_pDevice->SubmitQueue(m_queue, info, VK_NULL_HANDLE,
		waitSemaphores, waitStages, waitValues, nullptr, nullptr, nullptr), false);

	return true;
}

bool CommandQueue_VK::Signal(Fence* pFence, uint64_t value) const
{
	assert(pFence);
	const Semaphore signal = { pFence, value };
	// Submit deferred lists, then flush pending blit+PresentKHR.
	XUSG_N_RETURN(flushDeferred(nullptr, 0, &signal, 1), false);
	XUSG_N_RETURN(FlushQueuePendingPresent(m_queue), false);

	return true;
}

void CommandQueue_VK::ExecuteCommandLists(uint32_t numCommandLists, CommandList* const* ppCommandLists) const
{
	assert(ppCommandLists);
	for (auto i = 0u; i < numCommandLists; ++i)
		m_deferredLists.push_back(ppCommandLists[i]);
}

void CommandQueue_VK::ExecuteCommandList(CommandList* pCommandList) const
{
	assert(pCommandList);
	m_deferredLists.push_back(pCommandList);
}

bool CommandQueue_VK::flushDeferred(const Semaphore* pWaits, uint32_t numWaits,
	const Semaphore* pSignals, uint32_t numSignals) const
{
	if (m_deferredLists.empty() && numSignals == 0 && numWaits == 0 && !HasQueuePendingPresent(m_queue))
		return true;

	const auto numLists = static_cast<uint32_t>(m_deferredLists.size());
	CommandList* const* ppLists = numLists ? m_deferredLists.data() : nullptr;
	const auto result = submit(numLists, ppLists, pWaits, numWaits, pSignals, numSignals);
	m_deferredLists.clear();

	return result;
}

void CommandQueue_VK::UpdateTileMappings(Resource* pResource, uint32_t numResourceRegions,
	const TiledResourceCoord* pResourceRegionStartCoords, const TileRegionSize* pResourceRegionSizes,
	Heap* pHeap, uint32_t numHeapRanges, const TileRangeFlag* pHeapRangeFlags,
	const uint32_t* pHeapRangeStartOffsets, const uint32_t* pHeapRangeTileCounts, TileMappingFlag flags) const
{
	(void)flags;
	assert(pResource);
	assert(!numResourceRegions || pResourceRegionStartCoords);
	assert(!numResourceRegions || pResourceRegionSizes);
	assert(!numHeapRanges || pHeapRangeFlags);
	const auto pVkResource = AsVK(pResource);
	const auto queue = sparseQueue();
	if (!pVkResource || queue == VK_NULL_HANDLE) return;

	pVkResource->BindTileMappings(queue, numResourceRegions, pResourceRegionStartCoords, pResourceRegionSizes,
		pHeap, numHeapRanges, pHeapRangeFlags, pHeapRangeStartOffsets, pHeapRangeTileCounts);
}

void CommandQueue_VK::CopyTileMappings(Resource* pDstResource, const TiledResourceCoord* pDstRegionStartCoord,
	Resource* pSrcResource, const TiledResourceCoord* pSrcRegionStartCoordinate,
	const TileRegionSize& regionSize, TileMappingFlag flags) const
{
	(void)flags;
	assert(pDstResource);
	assert(pDstRegionStartCoord);
	assert(pSrcResource);
	assert(pSrcRegionStartCoordinate);
	const auto pDst = AsVK(pDstResource);
	const auto pSrc = AsVK(pSrcResource);
	const auto queue = sparseQueue();
	if (!pDst || !pSrc || queue == VK_NULL_HANDLE) return;

	pDst->CopyBoundTiles(queue, *pDstRegionStartCoord, pSrc, *pSrcRegionStartCoordinate, regionSize);
}

void CommandQueue_VK::Create(void* pNativeHandle, Device* pDevice)
{
	if (m_ownsSlot)
	{
		if (m_pDevice) m_pDevice->ReleaseQueue(m_family, m_queueIndex);
		m_ownsSlot = false;
	}
	if (m_registered) UnregisterQueue(m_queue);
	m_pDevice = AsVK(pDevice);
	assert(m_pDevice);

	m_queue = static_cast<VkQueue>(pNativeHandle);
	m_registered = false;
}

void* CommandQueue_VK::GetNativeHandle()
{
	return m_queue;
}

void* CommandQueue_VK::GetDeviceNativeHandle() const
{
	return m_pDevice->GetDevice();
}

Device* CommandQueue_VK::GetDevice() const
{
	return m_pDevice;
}

bool CommandQueue_VK::submit(uint32_t numCommandLists, CommandList* const* ppCommandLists,
	const Semaphore* pWaits, uint32_t numWaits, const Semaphore* pSignals, uint32_t numSignals) const
{
	if (m_commandInfos.size() < numCommandLists) m_commandInfos.resize(numCommandLists);
	for (auto i = 0u; i < numCommandLists; ++i)
	{
		m_commandInfos[i] = {};
		m_commandInfos[i].sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
		m_commandInfos[i].commandBuffer = static_cast<VkCommandBuffer>(ppCommandLists[i]->GetNativeHandle());
	}

	if (m_waitInfos.size() < numWaits) m_waitInfos.resize(numWaits);
	for (auto i = 0u; i < numWaits; ++i)
	{
		m_waitInfos[i] = {};
		m_waitInfos[i].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		assert(pWaits[i].Fence);
		m_waitInfos[i].semaphore = (VkSemaphore)pWaits[i].Fence->GetNativeHandle();
		m_waitInfos[i].value = pWaits[i].Value;
		m_waitInfos[i].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	}
	VkSemaphore acquireSemaphore = VK_NULL_HANDLE;
	VkFence acquireFence = VK_NULL_HANDLE;
	VkSemaphore presentSignal = VK_NULL_HANDLE;
	if (TakeQueueAcquireWait(m_queue, acquireSemaphore, acquireFence, presentSignal))
	{
		VkSemaphoreSubmitInfo wait = {};
		wait.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		wait.semaphore = acquireSemaphore;
		wait.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_2_COPY_BIT;
		if (m_waitInfos.size() < numWaits + 1) m_waitInfos.resize(numWaits + 1);
		m_waitInfos[numWaits] = wait;
		++numWaits;
	}
	VkSemaphoreSubmitInfo sparseWait = {};
	if (consumeSparseWait(sparseWait))
	{
		if (m_waitInfos.size() < numWaits + 1) m_waitInfos.resize(numWaits + 1);
		m_waitInfos[numWaits] = sparseWait;
		++numWaits;
	}

	if (m_signalInfos.size() < numSignals) m_signalInfos.resize(numSignals);
	for (auto i = 0u; i < numSignals; ++i)
	{
		m_signalInfos[i] = {};
		m_signalInfos[i].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		assert(pSignals[i].Fence);
		m_signalInfos[i].semaphore = (VkSemaphore)pSignals[i].Fence->GetNativeHandle();
		m_signalInfos[i].value = pSignals[i].Value;
		m_signalInfos[i].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	}
	if (presentSignal != VK_NULL_HANDLE)
	{
		VkSemaphoreSubmitInfo signal = {};
		signal.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		signal.semaphore = presentSignal;
		signal.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_2_COPY_BIT;
		if (m_signalInfos.size() < numSignals + 1) m_signalInfos.resize(numSignals + 1);
		m_signalInfos[numSignals] = signal;
		++numSignals;
	}

	VkSubmitInfo2 info = {};
	info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	info.waitSemaphoreInfoCount = numWaits;
	info.pWaitSemaphoreInfos = numWaits ? m_waitInfos.data() : nullptr;
	info.commandBufferInfoCount = numCommandLists;
	info.pCommandBufferInfos = numCommandLists ? m_commandInfos.data() : nullptr;
	info.signalSemaphoreInfoCount = numSignals;
	info.pSignalSemaphoreInfos = numSignals ? m_signalInfos.data() : nullptr;
	if (m_waitSemaphores.size() < numWaits) m_waitSemaphores.resize(numWaits);
	if (m_waitStages.size() < numWaits) m_waitStages.resize(numWaits);
	if (m_waitValues.size() < numWaits) m_waitValues.resize(numWaits);
	if (m_submitBuffers.size() < numCommandLists) m_submitBuffers.resize(numCommandLists);
	if (m_signalSemaphores.size() < numSignals) m_signalSemaphores.resize(numSignals);
	if (m_signalValues.size() < numSignals) m_signalValues.resize(numSignals);

	assert(m_pDevice);
	XUSG_N_RETURN(m_pDevice->SubmitQueue(m_queue, info, acquireFence,
		numWaits ? m_waitSemaphores.data() : nullptr, numWaits ? m_waitStages.data() : nullptr,
		numWaits ? m_waitValues.data() : nullptr, numCommandLists ? m_submitBuffers.data() : nullptr,
		numSignals ? m_signalSemaphores.data() : nullptr, numSignals ? m_signalValues.data() : nullptr), false);

	return true;
}

bool CommandQueue_VK::consumeSparseWait(VkSemaphoreSubmitInfo& wait) const
{
	if (!m_pDevice) return false;

	VkSemaphore semaphore = VK_NULL_HANDLE;
	uint64_t value = 0;
	m_pDevice->ReadSparseTimeline(semaphore, value);
	if (semaphore == VK_NULL_HANDLE || value <= m_sparseWaited) return false;

	wait = {};
	wait.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	wait.semaphore = semaphore;
	wait.value = value;
	wait.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	m_sparseWaited = value;

	return true;
}

VkQueue CommandQueue_VK::sparseQueue() const
{
	if (m_sparseQueue) return m_queue;

	VkQueue queue = VK_NULL_HANDLE;
	if (m_pDevice && m_pDevice->GetSparseQueue(queue)) return queue;

	assert(!"No queue supports sparse binding");

	return VK_NULL_HANDLE;
}
