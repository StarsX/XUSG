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

	Resource_VK* AsResource(Resource* pResource)
	{
		return dynamic_cast<Resource_VK*>(pResource);
	}

	const Resource_VK* AsResource(const Resource* pResource)
	{
		return dynamic_cast<const Resource_VK*>(pResource);
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

	string Label(const void* pData, uint32_t size)
	{
		if (!pData || !size) return {};

		return string(static_cast<const char*>(pData), static_cast<const char*>(pData) + size);
	}

	template <typename T>
	void Grow(vector<T>& values, uint32_t count)
	{
		if (values.size() < count) values.resize(count);
	}
}

CommandAllocator_VK::CommandAllocator_VK() :
	m_device(VK_NULL_HANDLE),
	m_pool(VK_NULL_HANDLE),
	m_family(0),
	m_pDevice(nullptr)
{
}

CommandAllocator_VK::~CommandAllocator_VK()
{
	if (m_pool) vkDestroyCommandPool(m_device, m_pool, nullptr);
}

bool CommandAllocator_VK::Create(const Device* pDevice, CommandListType type, const wchar_t* name)
{
	assert(type != CommandListType::VIDEO_DECODE && type != CommandListType::VIDEO_PROCESS && type != CommandListType::VIDEO_ENCODE);
	const auto pDeviceVK = dynamic_cast<const Device_VK*>(pDevice);
	assert(pDeviceVK);
	m_pDevice = pDevice;
	m_device = pDeviceVK->GetDevice();
	m_family = pDeviceVK->GetQueueFamily(type);
	if (m_pool) vkDestroyCommandPool(m_device, m_pool, nullptr);

	VkCommandPoolCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	info.queueFamilyIndex = m_family;
	XUSG_VK_CHECK(vkCreateCommandPool(m_device, &info, nullptr, &m_pool), cerr, false);
	pDeviceVK->SetDebugName(VK_OBJECT_TYPE_COMMAND_POOL, reinterpret_cast<uint64_t>(m_pool), name);

	return true;
}

bool CommandAllocator_VK::Reset()
{
	XUSG_VK_CHECK(vkResetCommandPool(m_device, m_pool, 0), cerr, false);

	return true;
}

void CommandAllocator_VK::Create(void* pHandle, const wchar_t* name)
{
	m_pool = static_cast<VkCommandPool>(pHandle);
	(void)name;
}

void* CommandAllocator_VK::GetHandle() const
{
	return m_pool;
}

void* CommandAllocator_VK::GetDeviceHandle() const
{
	return m_device;
}

const Device* CommandAllocator_VK::GetDevice() const
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
	m_device(VK_NULL_HANDLE),
	m_pool(VK_NULL_HANDLE),
	m_commandBuffer(VK_NULL_HANDLE),
	m_ownsCommandBuffer(false),
	m_pDevice(nullptr),
	m_pDeviceVK(nullptr),
	m_type(CommandListType::DIRECT),
	m_beginRendering(nullptr),
	m_endRendering(nullptr),
	m_setPrimitiveTopology(nullptr),
	m_bindVertexBuffers2(nullptr),
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
	m_renderWidth(1),
	m_renderHeight(1),
	m_graphicsLayout(nullptr),
	m_computeLayout(nullptr),
	m_pipeline(nullptr),
	m_resourceHeap(nullptr),
	m_samplerHeap(nullptr),
	m_preprocessBuffer(VK_NULL_HANDLE),
	m_preprocessMemory(VK_NULL_HANDLE),
	m_preprocessAddress(0),
	m_preprocessSize(0)
{
	m_depth = {};
}

CommandList_VK::~CommandList_VK()
{
	if (m_ownsCommandBuffer && m_commandBuffer) vkFreeCommandBuffers(m_device, m_pool, 1, &m_commandBuffer);
	if (m_preprocessBuffer) vkDestroyBuffer(m_device, m_preprocessBuffer, nullptr);
	if (m_preprocessMemory) vkFreeMemory(m_device, m_preprocessMemory, nullptr);
}

bool CommandList_VK::Create(const Device* pDevice, uint32_t nodeMask, CommandListType type,
	const CommandAllocator* pAllocator, const Pipeline& pipeline, const wchar_t* name)
{
	(void)nodeMask;
	assert(type != CommandListType::VIDEO_DECODE && type != CommandListType::VIDEO_PROCESS && type != CommandListType::VIDEO_ENCODE);
	m_pDeviceVK = dynamic_cast<const Device_VK*>(pDevice);
	assert(m_pDeviceVK);
	m_pDevice = pDevice;
	m_device = m_pDeviceVK->GetDevice();
	m_type = type;
	m_beginRendering = m_pDeviceVK->GetBeginRendering();
	m_endRendering = m_pDeviceVK->GetEndRendering();
	m_setPrimitiveTopology = m_pDeviceVK->GetSetPrimitiveTopology();
	m_bindVertexBuffers2 = m_pDeviceVK->GetBindVertexBuffers2();
	if (m_pDeviceVK->HasDebugUtils())
	{
		const auto instance = m_pDeviceVK->GetInstance();
		m_beginLabel = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdBeginDebugUtilsLabelEXT"));
		m_endLabel = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdEndDebugUtilsLabelEXT"));
		m_insertLabel = reinterpret_cast<PFN_vkCmdInsertDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdInsertDebugUtilsLabelEXT"));
	}

	const auto allocator = dynamic_cast<const CommandAllocator_VK*>(pAllocator);
	assert(allocator);
	if (m_ownsCommandBuffer && m_commandBuffer) vkFreeCommandBuffers(m_device, m_pool, 1, &m_commandBuffer);
	m_pool = allocator->GetPool();

	VkCommandBufferAllocateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	info.commandPool = m_pool;
	info.level = type == CommandListType::BUNDLE ? VK_COMMAND_BUFFER_LEVEL_SECONDARY : VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	info.commandBufferCount = 1;
	XUSG_VK_CHECK(vkAllocateCommandBuffers(m_device, &info, &m_commandBuffer), cerr, false);
	m_ownsCommandBuffer = true;
	m_pDeviceVK->SetDebugName(VK_OBJECT_TYPE_COMMAND_BUFFER, reinterpret_cast<uint64_t>(m_commandBuffer), name);

	VkCommandBufferBeginInfo begin = {};
	begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	if (type == CommandListType::BUNDLE) begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	XUSG_VK_CHECK(vkBeginCommandBuffer(m_commandBuffer, &begin), cerr, false);
	resetState();
	if (pipeline) SetPipelineState(pipeline);

	return true;
}

bool CommandList_VK::Close() const
{
	endRendering();
	XUSG_VK_CHECK(vkEndCommandBuffer(m_commandBuffer), cerr, false);

	return true;
}

bool CommandList_VK::Reset(const CommandAllocator* pAllocator, const Pipeline& initialState) const
{
	(void)pAllocator;
	endRendering();
	XUSG_VK_CHECK(vkResetCommandBuffer(m_commandBuffer, 0), cerr, false);

	VkCommandBufferBeginInfo begin = {};
	begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	if (m_type == CommandListType::BUNDLE) begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	XUSG_VK_CHECK(vkBeginCommandBuffer(m_commandBuffer, &begin), cerr, false);
	resetState();
	if (initialState) SetPipelineState(initialState);

	return true;
}

void CommandList_VK::ClearState(const Pipeline& initialState) const
{
	endRendering();
	resetState();
	if (initialState) SetPipelineState(initialState);
}

void CommandList_VK::Draw(uint32_t vertexCountPerInstance, uint32_t instanceCount,
	uint32_t startVertexLocation, uint32_t startInstanceLocation) const
{
	beginDraw();
	vkCmdDraw(m_commandBuffer, vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
}

void CommandList_VK::DrawIndexed(uint32_t indexCountPerInstance, uint32_t instanceCount,
	uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation) const
{
	beginDraw();
	vkCmdDrawIndexed(m_commandBuffer, indexCountPerInstance, instanceCount,
		startIndexLocation, baseVertexLocation, startInstanceLocation);
}

void CommandList_VK::Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) const
{
	endRendering();
	beginPredication();
	vkCmdDispatch(m_commandBuffer, threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}

void CommandList_VK::CopyBufferRegion(const Resource* pDstBuffer, uint64_t dstOffset,
	const Resource* pSrcBuffer, uint64_t srcOffset, uint64_t numBytes) const
{
	endRendering();
	assert(pDstBuffer);
	assert(pSrcBuffer);
	VkBufferCopy region = {};
	region.srcOffset = srcOffset;
	region.dstOffset = dstOffset;
	region.size = numBytes;
	vkCmdCopyBuffer(m_commandBuffer, AsResource(pSrcBuffer)->GetBuffer(), AsResource(pDstBuffer)->GetBuffer(), 1, &region);
}

void CommandList_VK::CopyTextureRegion(const TextureCopyLocation& dst, uint32_t dstX, uint32_t dstY, uint32_t dstZ,
	const TextureCopyLocation& src, const BoxRange* pSrcBox) const
{
	endRendering();
	assert(dst.pResource);
	assert(src.pResource);
	const auto pDst = AsResource(dst.pResource);
	const auto pSrc = AsResource(src.pResource);

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

void CommandList_VK::CopyResource(const Resource* pDstResource, const Resource* pSrcResource) const
{
	endRendering();
	assert(pDstResource);
	assert(pSrcResource);
	const auto pDst = AsResource(pDstResource);
	const auto pSrc = AsResource(pSrcResource);
	if (pSrc->IsImage() && pDst->IsImage())
	{
		const auto mips = (max)(pSrc->GetMipLevels(), 1u);
		const auto layers = pSrc->Is3D() ? 1u : (max)(pSrc->GetDepthOrArray(), 1u);
		Grow(m_imageCopies, mips);
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

void CommandList_VK::CopyTiles(const Resource* pTiledResource, const TiledResourceCoord* pTileRegionStartCoord,
	const TileRegionSize& tileRegionSize, const Resource* pBuffer, uint64_t bufferStartOffsetInBytes,
	TileCopyFlag flags) const
{
	assert(!"CopyTiles is not implemented on Vulkan");
	(void)pTiledResource;
	(void)pTileRegionStartCoord;
	(void)tileRegionSize;
	(void)pBuffer;
	(void)bufferStartOffsetInBytes;
	(void)flags;
}

void CommandList_VK::ResolveSubresource(const Resource* pDstResource, uint32_t dstSubresource,
	const Resource* pSrcResource, uint32_t srcSubresource, Format format) const
{
	endRendering();
	assert(pDstResource);
	assert(pSrcResource);
	const auto pDst = AsResource(pDstResource);
	const auto pSrc = AsResource(pSrcResource);
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
		viewports[i].x = pViewports[i].TopLeftX;
		viewports[i].y = pViewports[i].TopLeftY;
		viewports[i].width = pViewports[i].Width;
		viewports[i].height = pViewports[i].Height;
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
	m_pipeline = static_cast<const Pipeline_VK*>(pipelineState);
	if (m_pipeline && m_pipeline->Handle)
		vkCmdBindPipeline(m_commandBuffer, m_pipeline->BindPoint, m_pipeline->Handle);
}

void CommandList_VK::Barrier(uint32_t numBarriers, const ResourceBarrier* pBarriers)
{
	if (!numBarriers) return;

	endRendering();
	assert(pBarriers);
	m_globals.clear();
	m_buffers.clear();
	m_images.clear();
	for (auto i = 0u; i < numBarriers; ++i)
	{
		const auto& barrier = pBarriers[i];
		if ((barrier.Flags & BarrierFlag::BEGIN_ONLY) == BarrierFlag::BEGIN_ONLY) continue;

		const auto same = barrier.StateBefore == barrier.StateAfter;
		if (!barrier.pResource)
		{
			VkMemoryBarrier2 global = {};
			global.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
			global.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
			global.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
			global.srcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
			global.dstAccessMask = global.srcAccessMask;
			m_globals.push_back(global);
			continue;
		}

		const auto pResource = AsResource(barrier.pResource);
		if (same && barrier.StateAfter != ResourceState::UNORDERED_ACCESS && barrier.pResourceAfter)
		{
			VkMemoryBarrier2 global = {};
			global.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
			global.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
			global.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
			global.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
			global.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
			m_globals.push_back(global);
			continue;
		}

		const auto srcStage = GetVKStage(barrier.StateBefore);
		const auto dstStage = GetVKStage(barrier.StateAfter);
		const auto srcAccess = GetVKAccess(barrier.StateBefore);
		const auto dstAccess = GetVKAccess(barrier.StateAfter);
		if (pResource->IsImage())
		{
			VkImageMemoryBarrier2 image = {};
			image.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
			image.srcStageMask = srcStage;
			image.srcAccessMask = srcAccess;
			image.dstStageMask = dstStage;
			image.dstAccessMask = dstAccess;
			image.oldLayout = GetVKImageLayout(barrier.StateBefore);
			image.newLayout = GetVKImageLayout(barrier.StateAfter);
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
			pResource->AppendImageBarrier(image, m_images);
		}
		else
		{
			VkBufferMemoryBarrier2 buffer = {};
			buffer.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
			buffer.srcStageMask = srcStage;
			buffer.srcAccessMask = srcAccess;
			buffer.dstStageMask = dstStage;
			buffer.dstAccessMask = dstAccess;
			buffer.buffer = pResource->GetBuffer();
			buffer.size = VK_WHOLE_SIZE;
			m_buffers.push_back(buffer);
		}
	}

	if (m_globals.empty() && m_buffers.empty() && m_images.empty()) return;

	VkDependencyInfo dependency = {};
	dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependency.memoryBarrierCount = static_cast<uint32_t>(m_globals.size());
	dependency.pMemoryBarriers = m_globals.empty() ? nullptr : m_globals.data();
	dependency.bufferMemoryBarrierCount = static_cast<uint32_t>(m_buffers.size());
	dependency.pBufferMemoryBarriers = m_buffers.empty() ? nullptr : m_buffers.data();
	dependency.imageMemoryBarrierCount = static_cast<uint32_t>(m_images.size());
	dependency.pImageMemoryBarriers = m_images.empty() ? nullptr : m_images.data();
	vkCmdPipelineBarrier2(m_commandBuffer, &dependency);
}

void CommandList_VK::ExecuteBundle(const CommandList* pCommandList) const
{
	endRendering();
	assert(pCommandList);
	const auto buffer = static_cast<VkCommandBuffer>(pCommandList->GetHandle());
	vkCmdExecuteCommands(m_commandBuffer, 1, &buffer);
}

void CommandList_VK::SetDescriptorHeaps(uint32_t numDescriptorHeaps, const DescriptorHeap* pDescriptorHeaps)
{
	assert(numDescriptorHeaps == 0 || pDescriptorHeaps);
	for (auto i = 0u; i < numDescriptorHeaps; ++i)
		bindHeap(pDescriptorHeaps[i]);
}

void CommandList_VK::SetComputePipelineLayout(const PipelineLayout& pipelineLayout) const
{
	m_computeLayout = static_cast<const PipelineLayoutObject*>(pipelineLayout);
}

void CommandList_VK::SetGraphicsPipelineLayout(const PipelineLayout& pipelineLayout) const
{
	m_graphicsLayout = static_cast<const PipelineLayoutObject*>(pipelineLayout);
}

void CommandList_VK::SetComputeDescriptorTable(uint32_t index, const DescriptorTable& descriptorTable) const
{
	const auto& table = reinterpret_cast<const DescriptorTableProperty&>(descriptorTable);
	const auto stride = table.HeapType == SAMPLER_HEAP ? m_pDeviceVK->GetSamplerDescriptorStride() : m_pDeviceVK->GetResourceDescriptorStride();
	setDescriptorTable(m_computeLayout, index, stride ? static_cast<uint32_t>(table.Offset / stride) : 0);
}

void CommandList_VK::SetGraphicsDescriptorTable(uint32_t index, const DescriptorTable& descriptorTable) const
{
	const auto& table = reinterpret_cast<const DescriptorTableProperty&>(descriptorTable);
	const auto stride = table.HeapType == SAMPLER_HEAP ? m_pDeviceVK->GetSamplerDescriptorStride() : m_pDeviceVK->GetResourceDescriptorStride();
	setDescriptorTable(m_graphicsLayout, index, stride ? static_cast<uint32_t>(table.Offset / stride) : 0);
}

void CommandList_VK::SetComputeDescriptorTable(uint32_t index, const DescriptorHeap& descriptorHeap, int32_t offset) const
{
	bindHeap(descriptorHeap);
	const auto heap = static_cast<const DescriptorHeap_VK*>(descriptorHeap);
	const auto stride = heap ? heap->GetStride() : 1u;
	setDescriptorTable(m_computeLayout, index, static_cast<uint32_t>(offset / stride));
}

void CommandList_VK::SetGraphicsDescriptorTable(uint32_t index, const DescriptorHeap& descriptorHeap, int32_t offset) const
{
	bindHeap(descriptorHeap);
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
	BufferNative native = {};
	assert(FindBuffer(view.BufferLocation, native));
	const VkDeviceSize offset = view.BufferLocation - native.Address;
	vkCmdBindIndexBuffer(m_commandBuffer, native.Buffer, offset, GetVKIndexType(view.Format));
}

void CommandList_VK::IASetVertexBuffers(uint32_t startSlot, uint32_t numViews, const VertexBufferView* pViews) const
{
	if (!pViews)
	{
		const VkBuffer buffer = VK_NULL_HANDLE;
		const VkDeviceSize zero = 0;
		for (auto i = 0u; i < numViews; ++i)
		{
			if (m_bindVertexBuffers2)
				m_bindVertexBuffers2(m_commandBuffer, startSlot + i, 1, &buffer, &zero, &zero, &zero);
			else vkCmdBindVertexBuffers(m_commandBuffer, startSlot + i, 1, &buffer, &zero);
		}

		return;
	}

	for (auto i = 0u; i < numViews; ++i)
	{
		BufferNative native = {};
		assert(FindBuffer(pViews[i].BufferLocation, native));
		const VkDeviceSize offset = pViews[i].BufferLocation - native.Address;
		if (m_bindVertexBuffers2)
		{
			const VkDeviceSize size = pViews[i].SizeInBytes;
			const VkDeviceSize stride = pViews[i].StrideInBytes;
			m_bindVertexBuffers2(m_commandBuffer, startSlot + i, 1, &native.Buffer, &offset, &size, &stride);
		}
		else vkCmdBindVertexBuffers(m_commandBuffer, startSlot + i, 1, &native.Buffer, &offset);
	}
}

void CommandList_VK::SOSetTargets(uint32_t startSlot, uint32_t numViews, const StreamOutBufferView* pViews) const
{
	assert(!"SOSetTargets is not implemented on Vulkan");
	(void)startSlot;
	(void)numViews;
	(void)pViews;
}

void CommandList_VK::OMSetRenderTargets(uint32_t numRenderTargetDescriptors,
	const Descriptor* pRenderTargetViews, const Descriptor* pDepthStencilView) const
{
	endRendering();
	assert(numRenderTargetDescriptors <= 8);
	assert(numRenderTargetDescriptors == 0 || pRenderTargetViews);
	m_colorCount = numRenderTargetDescriptors;
	m_hasDepth = false;
	for (auto i = 0u; i < numRenderTargetDescriptors; ++i)
	{
		const auto pView = static_cast<const ResourceView_VK*>(pRenderTargetViews[i]);
		assert(pView && pView->Type == ResourceViewType::RTV);
		const auto pResource = AsResource(pView->pResource);
		auto& color = m_colors[i];
		color.View = pView->ImageView;
		color.Format = pView->ViewInfo.format;
		color.Layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		setAttachmentRange(color, pView, pResource);
		const auto mip = pView->ViewInfo.subresourceRange.baseMipLevel;
		color.Width = pResource ? static_cast<uint32_t>((max)(pResource->GetWidth() >> mip, 1ull)) : 1u;
		color.Height = pResource ? (max)(pResource->GetHeight() >> mip, 1u) : 1u;
		m_renderWidth = (max)(m_renderWidth, color.Width);
		m_renderHeight = (max)(m_renderHeight, color.Height);
	}

	if (pDepthStencilView && *pDepthStencilView)
	{
		const auto pView = static_cast<const ResourceView_VK*>(*pDepthStencilView);
		assert(pView->Type == ResourceViewType::DSV);
		const auto pResource = AsResource(pView->pResource);
		m_hasDepth = true;
		m_depth.View = pView->ImageView;
		m_depth.Format = pView->ViewInfo.format;
		m_depth.Layout = pView->Layout ? pView->Layout : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		setAttachmentRange(m_depth, pView, pResource);
		const auto mip = pView->ViewInfo.subresourceRange.baseMipLevel;
		m_depth.Width = pResource ? static_cast<uint32_t>((max)(pResource->GetWidth() >> mip, 1ull)) : 1u;
		m_depth.Height = pResource ? (max)(pResource->GetHeight() >> mip, 1u) : 1u;
		m_renderWidth = (max)(m_renderWidth, m_depth.Width);
		m_renderHeight = (max)(m_renderHeight, m_depth.Height);
	}

	if (numRenderTargetDescriptors || m_hasDepth) beginRendering();
}

void CommandList_VK::ClearDepthStencilView(ClearFlag clearFlags, float depth, uint8_t stencil,
	uint32_t numRects, const RectRange* pRects)
{
	if (!m_hasDepth) return;

	if (!m_renderingBegun) beginRendering();
	clearDepth(m_depth, clearFlags, depth, stencil, numRects, pRects);
}

void CommandList_VK::ClearDepthStencilView(const Descriptor& depthStencilView, ClearFlag clearFlags, float depth,
	uint8_t stencil, uint32_t numRects, const RectRange* pRects)
{
	if (!depthStencilView)
	{
		ClearDepthStencilView(clearFlags, depth, stencil, numRects, pRects);

		return;
	}

	const auto pView = static_cast<const ResourceView_VK*>(depthStencilView);
	if (m_hasDepth && pView->ImageView == m_depth.View)
	{
		ClearDepthStencilView(clearFlags, depth, stencil, numRects, pRects);

		return;
	}

	endRendering();
	RenderAttachment attachment = {};
	attachment.View = pView->ImageView;
	attachment.Format = pView->ViewInfo.format;
	attachment.Layout = pView->Layout ? pView->Layout : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	const auto pResource = AsResource(pView->pResource);
	setAttachmentRange(attachment, pView, pResource);
	const auto mip = pView->ViewInfo.subresourceRange.baseMipLevel;
	attachment.Width = pResource ? static_cast<uint32_t>((max)(pResource->GetWidth() >> mip, 1ull)) : 1u;
	attachment.Height = pResource ? (max)(pResource->GetHeight() >> mip, 1u) : 1u;

	RenderAttachment savedColors[8];
	const auto savedCount = m_colorCount;
	for (auto i = 0u; i < savedCount; ++i) savedColors[i] = m_colors[i];
	const auto savedDepth = m_depth;
	const auto savedHasDepth = m_hasDepth;
	m_colorCount = 0;
	m_depth = attachment;
	m_hasDepth = true;
	beginRendering();
	clearDepth(attachment, clearFlags, depth, stencil, numRects, pRects);
	endRendering();
	m_colorCount = savedCount;
	for (auto i = 0u; i < savedCount; ++i) m_colors[i] = savedColors[i];
	m_depth = savedDepth;
	m_hasDepth = savedHasDepth;
}

void CommandList_VK::ClearRenderTargetView(const float colorRGBA[4], uint8_t renderTargetViewIndex,
	uint32_t numRects, const RectRange* pRects)
{
	if (renderTargetViewIndex >= m_colorCount) return;

	if (!m_renderingBegun) beginRendering();
	clearColor(renderTargetViewIndex, colorRGBA, numRects, pRects);
}

void CommandList_VK::ClearRenderTargetView(const Descriptor& renderTargetView, const float colorRGBA[4],
	uint32_t numRects, const RectRange* pRects)
{
	assert(renderTargetView);
	const auto pView = static_cast<const ResourceView_VK*>(renderTargetView);
	for (auto i = 0u; i < m_colorCount; ++i)
	{
		if (m_colors[i].View == pView->ImageView)
		{
			ClearRenderTargetView(colorRGBA, static_cast<uint8_t>(i), numRects, pRects);

			return;
		}
	}

	endRendering();
	RenderAttachment attachment = {};
	attachment.View = pView->ImageView;
	attachment.Format = pView->ViewInfo.format;
	attachment.Layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	const auto pResource = AsResource(pView->pResource);
	setAttachmentRange(attachment, pView, pResource);
	const auto mip = pView->ViewInfo.subresourceRange.baseMipLevel;
	attachment.Width = pResource ? static_cast<uint32_t>((max)(pResource->GetWidth() >> mip, 1ull)) : 1u;
	attachment.Height = pResource ? (max)(pResource->GetHeight() >> mip, 1u) : 1u;

	RenderAttachment savedColors[8];
	const auto savedCount = m_colorCount;
	for (auto i = 0u; i < savedCount; ++i) savedColors[i] = m_colors[i];
	const auto savedDepth = m_depth;
	const auto savedHasDepth = m_hasDepth;
	m_colorCount = 1;
	m_colors[0] = attachment;
	m_hasDepth = false;
	beginRendering();
	clearColor(0, colorRGBA, numRects, pRects);
	endRendering();
	m_colorCount = savedCount;
	for (auto i = 0u; i < savedCount; ++i) m_colors[i] = savedColors[i];
	m_depth = savedDepth;
	m_hasDepth = savedHasDepth;
}

void CommandList_VK::ClearUnorderedAccessViewUint(const DescriptorTable& descriptorTable, const Descriptor& descriptor,
	const uint32_t values[4], uint32_t numRects, const RectRange* pRects)
{
	(void)descriptorTable;
	(void)pRects;
	VkClearColorValue color = {};
	memcpy(color.uint32, values, sizeof(uint32_t) * 4);
	clearUnorderedAccessView(descriptor, color, numRects);
}

void CommandList_VK::ClearUnorderedAccessViewFloat(const DescriptorTable& descriptorTable, const Descriptor& descriptor,
	const float values[4], uint32_t numRects, const RectRange* pRects)
{
	(void)descriptorTable;
	(void)pRects;
	VkClearColorValue color = {};
	memcpy(color.float32, values, sizeof(float) * 4);
	clearUnorderedAccessView(descriptor, color, numRects);
}

void CommandList_VK::setAttachmentRange(RenderAttachment& attachment, const ResourceView_VK* pView, Resource_VK* pResource) const
{
	attachment.Resource = pResource;
	attachment.BaseMip = pView->ViewInfo.subresourceRange.baseMipLevel;
	attachment.MipCount = pView->ViewInfo.subresourceRange.levelCount ? pView->ViewInfo.subresourceRange.levelCount : 1u;
	attachment.BaseLayer = pView->ViewInfo.subresourceRange.baseArrayLayer;
	attachment.Layers = (max)(pView->ViewInfo.subresourceRange.layerCount, 1u);
}

void CommandList_VK::submitClearBarrier(Resource_VK* pResource, uint32_t regionCount, bool toCopy)
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
			image.oldLayout = undefined ? VK_IMAGE_LAYOUT_UNDEFINED : GetVKImageLayout(src);
			if (undefined) image.srcAccessMask = VK_ACCESS_2_NONE;
			image.newLayout = GetVKImageLayout(dst);
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
	vkCmdPipelineBarrier2(m_commandBuffer, &dependency);
}

void CommandList_VK::clearUnorderedAccessView(const Descriptor& descriptor, const VkClearColorValue& color, uint32_t numRects)
{
	endRendering();
	assert(numRects == 0 && "ClearUnorderedAccessView rects are not implemented on Vulkan");
	assert(descriptor);
	const auto pView = static_cast<const ResourceView_VK*>(descriptor);
	assert(pView->Type == ResourceViewType::UAV);
	auto pResource = dynamic_cast<Resource_VK*>(pView->pResource);
	assert(pResource);

	auto regionCount = 1u;
	Grow(m_clearRegions, 1);
	if (!pView->IsImage) m_clearRegions[0] = { 0, ResourceState::COMMON };
	else if (pResource->Is3D())
	{
		Grow(m_clearRegions, 1);
		m_clearRegions[0] = { pView->ViewInfo.subresourceRange.baseMipLevel, ResourceState::COMMON };
	}
	else
	{
		const auto mips = (max)(pResource->GetMipLevels(), 1u);
		const auto& range = pView->ViewInfo.subresourceRange;
		const auto levels = range.levelCount ? range.levelCount : 1u;
		const auto layers = range.layerCount ? range.layerCount : 1u;
		regionCount = levels * layers;
		Grow(m_clearRegions, regionCount);
		auto index = 0u;
		for (auto layer = 0u; layer < layers; ++layer)
			for (auto level = 0u; level < levels; ++level)
				m_clearRegions[index++] = { range.baseMipLevel + level + (range.baseArrayLayer + layer) * mips, ResourceState::COMMON };
	}

	for (auto i = 0u; i < regionCount; ++i)
	{
		m_clearRegions[i].State = pResource->GetResourceState(m_clearRegions[i].Subresource);
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

void CommandList_VK::DiscardResource(Resource* pResource, uint32_t numRects, const RectRange* pRects,
	uint32_t firstSubresource, uint32_t numSubresources)
{
	endRendering();
	assert(pResource);
	assert(numRects == 0 && "DiscardResource rectangles are not supported on Vulkan");
	(void)pRects;
	auto resource = dynamic_cast<Resource_VK*>(pResource);
	assert(resource);
	assert(resource->IsImage() && "DiscardResource buffers are not supported on Vulkan");
	resource->Discard(firstSubresource, numSubresources);
}

void CommandList_VK::BeginQuery(const QueryHeap& queryHeap, QueryType type, uint32_t index) const
{
	if (type == QueryType::TIMESTAMP) return;

	const auto flags = type == QueryType::OCCLUSION ? VK_QUERY_CONTROL_PRECISE_BIT : 0u;
	vkCmdBeginQuery(m_commandBuffer, static_cast<VkQueryPool>(queryHeap), index, flags);
}

void CommandList_VK::EndQuery(const QueryHeap& queryHeap, QueryType type, uint32_t index) const
{
	if (type == QueryType::TIMESTAMP)
	{
		vkCmdWriteTimestamp2(m_commandBuffer, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, static_cast<VkQueryPool>(queryHeap), index);

		return;
	}

	vkCmdEndQuery(m_commandBuffer, static_cast<VkQueryPool>(queryHeap), index);
}

void CommandList_VK::ResolveQueryData(const QueryHeap& queryHeap, QueryType type, uint32_t startIndex,
	uint32_t numQueries, const Resource* pDstBuffer, uint64_t alignedDstBufferOffset) const
{
	(void)type;
	assert(pDstBuffer);
	vkCmdCopyQueryPoolResults(m_commandBuffer, static_cast<VkQueryPool>(queryHeap), startIndex, numQueries,
		AsResource(pDstBuffer)->GetBuffer(), alignedDstBufferOffset, sizeof(uint64_t),
		VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
}

void CommandList_VK::SetPredication(const Resource* pBuffer, uint64_t alignedBufferOffset, bool opEqualZero) const
{
	endPredication();
	m_predicateBuffer = VK_NULL_HANDLE;
	if (!pBuffer) return;

	assert(m_pDeviceVK->HasConditionalRendering() && "SetPredication requires VK_EXT_conditional_rendering");
	assert((alignedBufferOffset % 4) == 0);
	const auto buffer = AsResource(pBuffer);
	assert(buffer && buffer->GetBuffer());
	m_predicateBuffer = buffer->GetBuffer();
	m_predicateOffset = alignedBufferOffset;
	m_predicateEqualZero = opEqualZero;
}

void CommandList_VK::SetMarker(uint32_t metaData, const void* pData, uint32_t size) const
{
	(void)metaData;
	if (!m_insertLabel) return;

	const auto text = Label(pData, size);
	VkDebugUtilsLabelEXT label = {};
	label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
	label.pLabelName = text.c_str();
	m_insertLabel(m_commandBuffer, &label);
}

void CommandList_VK::BeginEvent(uint32_t metaData, const void* pData, uint32_t size) const
{
	(void)metaData;
	if (!m_beginLabel) return;

	const auto text = Label(pData, size);
	VkDebugUtilsLabelEXT label = {};
	label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
	label.pLabelName = text.c_str();
	m_beginLabel(m_commandBuffer, &label);
}

void CommandList_VK::EndEvent()
{
	if (m_endLabel) m_endLabel(m_commandBuffer);
}

void CommandList_VK::ExecuteIndirect(const CommandLayout* pCommandlayout, uint32_t maxCommandCount,
	const Resource* pArgumentBuffer, uint64_t argumentBufferOffset, const Resource* pCountBuffer, uint64_t countBufferOffset)
{
	assert(pCommandlayout);
	assert(pArgumentBuffer);
	const auto layout = dynamic_cast<const CommandLayout_VK*>(pCommandlayout);
	assert(layout);
	if (layout->IsDispatch()) endRendering();
	else beginDraw();
	beginPredication();

	const auto argument = AsResource(pArgumentBuffer);
	VkGeneratedCommandsMemoryRequirementsInfoEXT requirementsInfo = {};
	requirementsInfo.sType = VK_STRUCTURE_TYPE_GENERATED_COMMANDS_MEMORY_REQUIREMENTS_INFO_EXT;
	requirementsInfo.indirectCommandsLayout = layout->GetLayout();
	requirementsInfo.maxSequenceCount = maxCommandCount;
	VkMemoryRequirements2 requirements = {};
	requirements.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
	m_pDeviceVK->GetExtProcs().GetGeneratedCommandsMemoryRequirements(m_device, &requirementsInfo, &requirements);
	ensurePreprocess(requirements.memoryRequirements.size);

	VkGeneratedCommandsPipelineInfoEXT pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GENERATED_COMMANDS_PIPELINE_INFO_EXT;
	pipelineInfo.pipeline = m_pipeline ? m_pipeline->Handle : VK_NULL_HANDLE;

	VkGeneratedCommandsInfoEXT info = {};
	info.sType = VK_STRUCTURE_TYPE_GENERATED_COMMANDS_INFO_EXT;
	info.pNext = pipelineInfo.pipeline ? &pipelineInfo : nullptr;
	info.shaderStages = layout->GetShaderStages();
	info.indirectCommandsLayout = layout->GetLayout();
	info.indirectAddress = argument->GetVirtualAddress(static_cast<int>(argumentBufferOffset));
	info.indirectAddressSize = static_cast<VkDeviceSize>(maxCommandCount) * (max)(layout->GetByteStride(), 1u);
	info.preprocessAddress = m_preprocessAddress;
	info.preprocessSize = m_preprocessSize;
	info.maxSequenceCount = maxCommandCount;
	if (pCountBuffer) info.sequenceCountAddress = pCountBuffer->GetVirtualAddress(static_cast<int>(countBufferOffset));
	info.maxDrawCount = maxCommandCount;
	m_pDeviceVK->GetExtProcs().PreprocessGeneratedCommands(m_commandBuffer, &info, m_commandBuffer);
	m_pDeviceVK->GetExtProcs().ExecuteGeneratedCommands(m_commandBuffer, VK_TRUE, &info);
}

void CommandList_VK::Create(void* pHandle, const wchar_t* name)
{
	if (m_ownsCommandBuffer && m_commandBuffer) vkFreeCommandBuffers(m_device, m_pool, 1, &m_commandBuffer);
	m_commandBuffer = static_cast<VkCommandBuffer>(pHandle);
	m_ownsCommandBuffer = false;
	(void)name;
}

void CommandList_VK::Attach(const CommandList_VK& source)
{
	if (m_ownsCommandBuffer && m_commandBuffer) vkFreeCommandBuffers(m_device, m_pool, 1, &m_commandBuffer);
	if (m_preprocessBuffer) vkDestroyBuffer(m_device, m_preprocessBuffer, nullptr);
	if (m_preprocessMemory) vkFreeMemory(m_device, m_preprocessMemory, nullptr);

	m_device = source.m_device;
	m_pool = VK_NULL_HANDLE;
	m_commandBuffer = source.m_commandBuffer;
	m_ownsCommandBuffer = false;
	m_pDevice = source.m_pDevice;
	m_pDeviceVK = source.m_pDeviceVK;
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
}

void* CommandList_VK::GetHandle() const
{
	return m_commandBuffer;
}

void* CommandList_VK::GetDeviceHandle() const
{
	return m_device;
}

const Device* CommandList_VK::GetDevice() const
{
	return m_pDevice;
}

VkCommandBuffer CommandList_VK::GetCommandBuffer() const
{
	return m_commandBuffer;
}

void CommandList_VK::CopyBufferToImage(const Resource* pDst, uint32_t dstSubresource, const Resource* pSrc,
	uint64_t srcOffset, uint32_t bufferRowLength, uint32_t bufferImageHeight) const
{
	endRendering();
	const auto pDstResource = AsResource(pDst);
	const auto pSrcResource = AsResource(pSrc);
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

void CommandList_VK::CopyImageToBuffer(const Resource* pSrc, uint32_t mipLevel, uint32_t arrayLayer, const Resource* pDst,
	uint64_t dstOffset, uint32_t bufferRowLength, uint32_t bufferImageHeight, VkImageAspectFlags aspect) const
{
	endRendering();
	const auto pSrcResource = AsResource(pSrc);
	const auto pDstResource = AsResource(pDst);
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

void CommandList_VK::endPredication() const
{
	if (!m_predicationActive) return;

	const auto end = m_pDeviceVK->GetExtProcs().EndConditionalRendering;
	assert(end);
	end(m_commandBuffer);
	m_predicationActive = false;
}

void CommandList_VK::beginPredication() const
{
	if (!m_predicateBuffer || m_predicationActive) return;

	const auto begin = m_pDeviceVK->GetExtProcs().BeginConditionalRendering;
	assert(begin);
	VkConditionalRenderingBeginInfoEXT info = {};
	info.sType = VK_STRUCTURE_TYPE_CONDITIONAL_RENDERING_BEGIN_INFO_EXT;
	info.buffer = m_predicateBuffer;
	info.offset = m_predicateOffset;
	info.flags = m_predicateEqualZero ? VK_CONDITIONAL_RENDERING_INVERTED_BIT_EXT : 0;
	begin(m_commandBuffer, &info);
	m_predicationActive = true;
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
	m_renderWidth = 1;
	m_renderHeight = 1;
}

void CommandList_VK::endRendering() const
{
	endPredication();
	if (!m_renderingBegun) return;

	assert(m_endRendering);
	m_endRendering(m_commandBuffer);
	m_renderingBegun = false;
}

const void* CommandList_VK::renderingPNext() const
{
	return nullptr;
}

void CommandList_VK::beginRendering() const
{
	if (m_renderingBegun) return;

	endPredication();

	VkRenderingAttachmentInfo colors[8];
	for (auto i = 0u; i < m_colorCount; ++i)
	{
		colors[i] = {};
		colors[i].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		colors[i].imageView = m_colors[i].View;
		colors[i].imageLayout = m_colors[i].Layout;
		colors[i].loadOp = m_colors[i].Resource ? m_colors[i].Resource->ConsumeDiscardLoadOp(
			m_colors[i].BaseMip, m_colors[i].MipCount, m_colors[i].BaseLayer, m_colors[i].Layers) : VK_ATTACHMENT_LOAD_OP_LOAD;
		colors[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	}

	VkRenderingAttachmentInfo depth = {};
	depth.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	depth.imageView = m_depth.View;
	depth.imageLayout = m_depth.Layout;
	depth.loadOp = m_hasDepth && m_depth.Resource ? m_depth.Resource->ConsumeDiscardLoadOp(
		m_depth.BaseMip, m_depth.MipCount, m_depth.BaseLayer, m_depth.Layers) : VK_ATTACHMENT_LOAD_OP_LOAD;
	depth.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

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
	const auto stencil = m_hasDepth && (m_depth.Format == VK_FORMAT_D24_UNORM_S8_UINT ||
		m_depth.Format == VK_FORMAT_D32_SFLOAT_S8_UINT || m_depth.Format == VK_FORMAT_D16_UNORM_S8_UINT);
	info.pNext = renderingPNext();
	info.pDepthAttachment = m_hasDepth ? &depth : nullptr;
	info.pStencilAttachment = stencil ? &depth : nullptr;
	assert(m_beginRendering);
	m_beginRendering(m_commandBuffer, &info);
	m_renderingBegun = true;
}

void CommandList_VK::beginDraw() const
{
	if (m_type == CommandListType::DIRECT && !m_renderingBegun) beginRendering();
	beginPredication();
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
	if (pHeap->GetType() == SAMPLER_HEAP)
	{
		m_pDeviceVK->GetExtProcs().BindSamplerHeap(m_commandBuffer, &info);
		m_samplerHeap = heap;
	}
	else
	{
		m_pDeviceVK->GetExtProcs().BindResourceHeap(m_commandBuffer, &info);
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
	m_pDeviceVK->GetExtProcs().PushData(m_commandBuffer, &info);
}

void CommandList_VK::setDescriptorTable(const PipelineLayoutObject* pLayout, uint32_t index, uint32_t descriptorIndex) const
{
	assert(pLayout);
	assert(index < pLayout->RootParameters.size());
	const auto value = descriptorIndex;
	pushData(pLayout->RootParameters[index].PushByteOffset, &value, sizeof(value));
}

void CommandList_VK::setConstants(const PipelineLayoutObject* pLayout, uint32_t index, uint32_t num32BitValues,
	const void* pData, uint32_t destOffsetIn32BitValues) const
{
	assert(pLayout);
	assert(index < pLayout->RootParameters.size());
	const auto offset = pLayout->RootParameters[index].PushByteOffset + destOffsetIn32BitValues * static_cast<uint32_t>(sizeof(uint32_t));
	pushData(offset, pData, num32BitValues * static_cast<uint32_t>(sizeof(uint32_t)));
}

void CommandList_VK::setRootAddress(const PipelineLayoutObject* pLayout, uint32_t index, uint64_t address) const
{
	assert(pLayout);
	assert(index < pLayout->RootParameters.size());
	pushData(pLayout->RootParameters[index].PushByteOffset, &address, sizeof(address));
}

bool CommandList_VK::ensurePreprocess(VkDeviceSize size) const
{
	if (!size || (m_preprocessBuffer && m_preprocessSize >= size)) return true;

	if (m_preprocessBuffer) vkDestroyBuffer(m_device, m_preprocessBuffer, nullptr);
	if (m_preprocessMemory) vkFreeMemory(m_device, m_preprocessMemory, nullptr);
	m_preprocessBuffer = VK_NULL_HANDLE;
	m_preprocessMemory = VK_NULL_HANDLE;

	VkBufferUsageFlags2CreateInfo usage = {};
	usage.sType = VK_STRUCTURE_TYPE_BUFFER_USAGE_FLAGS_2_CREATE_INFO;
	usage.usage = VK_BUFFER_USAGE_2_PREPROCESS_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

	VkBufferCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.pNext = &usage;
	info.size = size;
	XUSG_VK_CHECK(vkCreateBuffer(m_device, &info, nullptr, &m_preprocessBuffer), cerr, false);

	VkMemoryRequirements requirements = {};
	vkGetBufferMemoryRequirements(m_device, m_preprocessBuffer, &requirements);
	VkMemoryAllocateFlagsInfo flags = {};
	flags.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
	flags.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
	VkMemoryAllocateInfo alloc = {};
	alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc.pNext = &flags;
	alloc.allocationSize = requirements.size;
	alloc.memoryTypeIndex = m_pDeviceVK->FindMemoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	XUSG_VK_CHECK(vkAllocateMemory(m_device, &alloc, nullptr, &m_preprocessMemory), cerr, false);
	XUSG_VK_CHECK(vkBindBufferMemory(m_device, m_preprocessBuffer, m_preprocessMemory, 0), cerr, false);

	VkBufferDeviceAddressInfo address = {};
	address.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	address.buffer = m_preprocessBuffer;
	m_preprocessAddress = vkGetBufferDeviceAddress(m_device, &address);
	m_preprocessSize = size;

	return true;
}

void CommandList_VK::clearColor(uint32_t attachment, const float color[4], uint32_t numRects, const RectRange* pRects) const
{
	VkClearAttachment clear = {};
	clear.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	clear.colorAttachment = attachment;
	memcpy(clear.clearValue.color.float32, color, sizeof(float) * 4);

	const auto rectCount = numRects ? numRects : 1u;
	Grow(m_clearRects, rectCount);
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
	Grow(m_clearRects, rectCount);
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

//--------------------------------------------------------------------------------------

CommandQueue_VK::CommandQueue_VK() :
	m_device(VK_NULL_HANDLE),
	m_queue(VK_NULL_HANDLE),
	m_pDevice(nullptr),
	m_family(0),
	m_queueIndex(0),
	m_registered(false),
	m_ownsSlot(false)
{
}

CommandQueue_VK::~CommandQueue_VK()
{
	if (m_registered) UnregisterQueue(m_queue);
	if (m_ownsSlot)
	{
		const auto device = dynamic_cast<const Device_VK*>(m_pDevice);
		if (device) device->ReleaseQueue(m_family, m_queueIndex);
	}
}

bool CommandQueue_VK::Create(const Device* pDevice, CommandListType type, CommandQueueFlag flags,
	int32_t priority, uint32_t nodeMask, const wchar_t* name)
{
	assert(type != CommandListType::VIDEO_DECODE && type != CommandListType::VIDEO_PROCESS && type != CommandListType::VIDEO_ENCODE);
	(void)flags;
	(void)priority;
	(void)nodeMask;
	const auto pDeviceVK = dynamic_cast<const Device_VK*>(pDevice);
	assert(pDeviceVK);
	if (m_ownsSlot)
	{
		const auto previous = dynamic_cast<const Device_VK*>(m_pDevice);
		if (previous) previous->ReleaseQueue(m_family, m_queueIndex);
		m_ownsSlot = false;
	}
	if (m_registered) UnregisterQueue(m_queue);

	m_pDevice = pDevice;
	m_device = pDeviceVK->GetDevice();
	XUSG_N_RETURN(pDeviceVK->AcquireQueue(type, m_family, m_queueIndex), false);
	m_ownsSlot = true;
	vkGetDeviceQueue(m_device, m_family, m_queueIndex, &m_queue);

	QueueNative native = {};
	native.Queue = m_queue;
	native.Device = m_device;
	native.PhysicalDevice = pDeviceVK->GetPhysicalDevice();
	native.Instance = pDeviceVK->GetInstance();
	native.Family = m_family;
	native.ApiVersion = pDeviceVK->GetApiVersion();
	RegisterQueue(native);
	m_registered = true;
	pDeviceVK->SetDebugName(VK_OBJECT_TYPE_QUEUE, reinterpret_cast<uint64_t>(m_queue), name);

	return true;
}

bool CommandQueue_VK::SubmitCommandLists(uint32_t numCommandLists, const CommandList* const* ppCommandLists,
	const Semaphore* pWaits, uint32_t numWaits, const Semaphore* pSignals, uint32_t numSignals)
{
	return submit(numCommandLists, ppCommandLists, pWaits, numWaits, pSignals, numSignals);
}

bool CommandQueue_VK::SubmitCommandList(const CommandList* pCommandList,
	const Semaphore* pWaits, uint32_t numWaits, const Semaphore* pSignals, uint32_t numSignals)
{
	return submit(1, &pCommandList, pWaits, numWaits, pSignals, numSignals);
}

bool CommandQueue_VK::Wait(const Fence* pFence, uint64_t value)
{
	assert(pFence);
	VkSemaphoreSubmitInfo wait = {};
	wait.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	wait.semaphore = static_cast<VkSemaphore>(pFence->GetHandle());
	wait.value = value;
	wait.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

	VkSubmitInfo2 info = {};
	info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	info.waitSemaphoreInfoCount = 1;
	info.pWaitSemaphoreInfos = &wait;
	XUSG_VK_CHECK(vkQueueSubmit2(m_queue, 1, &info, VK_NULL_HANDLE), cerr, false);

	return true;
}

bool CommandQueue_VK::Signal(const Fence* pFence, uint64_t value)
{
	assert(pFence);
	VkSemaphoreSubmitInfo signal = {};
	signal.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	signal.semaphore = static_cast<VkSemaphore>(pFence->GetHandle());
	signal.value = value;
	signal.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

	VkSubmitInfo2 info = {};
	info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	info.signalSemaphoreInfoCount = 1;
	info.pSignalSemaphoreInfos = &signal;
	XUSG_VK_CHECK(vkQueueSubmit2(m_queue, 1, &info, VK_NULL_HANDLE), cerr, false);

	return true;
}

void CommandQueue_VK::ExecuteCommandLists(uint32_t numCommandLists, const CommandList* const* ppCommandLists)
{
	submit(numCommandLists, ppCommandLists, nullptr, 0, nullptr, 0);
}

void CommandQueue_VK::ExecuteCommandList(const CommandList* pCommandList)
{
	submit(1, &pCommandList, nullptr, 0, nullptr, 0);
}

void CommandQueue_VK::UpdateTileMappings(const Resource* pResource, uint32_t numResourceRegions,
	const TiledResourceCoord* pResourceRegionStartCoords, const TileRegionSize* pResourceRegionSizes,
	const Heap* pHeap, uint32_t numHeapRanges, const TileRangeFlag* pHeapRangeFlags,
	const uint32_t* pHeapRangeStartOffsets, const uint32_t* pHeapRangeTileCounts, TileMappingFlag flags)
{
	assert(!"UpdateTileMappings is not implemented on Vulkan");
	(void)pResource;
	(void)numResourceRegions;
	(void)pResourceRegionStartCoords;
	(void)pResourceRegionSizes;
	(void)pHeap;
	(void)numHeapRanges;
	(void)pHeapRangeFlags;
	(void)pHeapRangeStartOffsets;
	(void)pHeapRangeTileCounts;
	(void)flags;
}

void CommandQueue_VK::CopyTileMappings(const Resource* pDstResource, const TiledResourceCoord* pDstRegionStartCoord,
	const Resource* pSrcResource, const TiledResourceCoord* pSrcRegionStartCoordinate,
	const TileRegionSize& regionSize, TileMappingFlag flags) const
{
	assert(!"CopyTileMappings is not implemented on Vulkan");
	(void)pDstResource;
	(void)pDstRegionStartCoord;
	(void)pSrcResource;
	(void)pSrcRegionStartCoordinate;
	(void)regionSize;
	(void)flags;
}

void CommandQueue_VK::Create(void* pHandle, const wchar_t* name)
{
	if (m_ownsSlot)
	{
		const auto device = dynamic_cast<const Device_VK*>(m_pDevice);
		if (device) device->ReleaseQueue(m_family, m_queueIndex);
		m_ownsSlot = false;
	}
	if (m_registered) UnregisterQueue(m_queue);
	m_queue = static_cast<VkQueue>(pHandle);
	m_registered = false;
	(void)name;
}

void* CommandQueue_VK::GetHandle() const
{
	return m_queue;
}

void* CommandQueue_VK::GetDeviceHandle() const
{
	return m_device;
}

const Device* CommandQueue_VK::GetDevice() const
{
	return m_pDevice;
}

bool CommandQueue_VK::submit(uint32_t numCommandLists, const CommandList* const* ppCommandLists,
	const Semaphore* pWaits, uint32_t numWaits, const Semaphore* pSignals, uint32_t numSignals)
{
	Grow(m_commandInfos, numCommandLists);
	for (auto i = 0u; i < numCommandLists; ++i)
	{
		m_commandInfos[i] = {};
		m_commandInfos[i].sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
		m_commandInfos[i].commandBuffer = static_cast<VkCommandBuffer>(ppCommandLists[i]->GetHandle());
	}

	Grow(m_waitInfos, numWaits);
	for (auto i = 0u; i < numWaits; ++i)
	{
		m_waitInfos[i] = {};
		m_waitInfos[i].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		m_waitInfos[i].semaphore = static_cast<VkSemaphore>(pWaits[i].Fence->GetHandle());
		m_waitInfos[i].value = pWaits[i].Value;
		m_waitInfos[i].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	}
	VkSemaphore acquireSemaphore = VK_NULL_HANDLE;
	if (TakeQueueAcquireWait(m_queue, acquireSemaphore))
	{
		VkSemaphoreSubmitInfo wait = {};
		wait.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		wait.semaphore = acquireSemaphore;
		wait.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		Grow(m_waitInfos, numWaits + 1);
		m_waitInfos[numWaits] = wait;
		++numWaits;
	}

	Grow(m_signalInfos, numSignals);
	for (auto i = 0u; i < numSignals; ++i)
	{
		m_signalInfos[i] = {};
		m_signalInfos[i].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		m_signalInfos[i].semaphore = static_cast<VkSemaphore>(pSignals[i].Fence->GetHandle());
		m_signalInfos[i].value = pSignals[i].Value;
		m_signalInfos[i].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	}

	VkSubmitInfo2 info = {};
	info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	info.waitSemaphoreInfoCount = numWaits;
	info.pWaitSemaphoreInfos = numWaits ? m_waitInfos.data() : nullptr;
	info.commandBufferInfoCount = numCommandLists;
	info.pCommandBufferInfos = numCommandLists ? m_commandInfos.data() : nullptr;
	info.signalSemaphoreInfoCount = numSignals;
	info.pSignalSemaphoreInfos = numSignals ? m_signalInfos.data() : nullptr;
	XUSG_VK_CHECK(vkQueueSubmit2(m_queue, 1, &info, VK_NULL_HANDLE), cerr, false);

	return true;
}
