//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGResource_VK.h"
#include "XUSGCommand_VK.h"
#include "XUSGEnum_VK.h"

using namespace std;
using namespace XUSG;

namespace
{
	VkComponentMapping ComponentMapping(uint16_t mapping)
	{
		VkComponentMapping components = {};
		components.r = GetVKComponentSwizzle(static_cast<SrvComponentMapping>((mapping >> 0) & 7));
		components.g = GetVKComponentSwizzle(static_cast<SrvComponentMapping>((mapping >> 3) & 7));
		components.b = GetVKComponentSwizzle(static_cast<SrvComponentMapping>((mapping >> 6) & 7));
		components.a = GetVKComponentSwizzle(static_cast<SrvComponentMapping>((mapping >> 9) & 7));

		return components;
	}

	uint32_t RowPitch(Format format, uint32_t width)
	{
		const auto bits = GetVKFormatBits(format);
		const auto packed = IsCompressedFormat(format) ? XUSG_DIV_UP(width, 4u) * (bits <= 4 ? 8u : 16u) :
			XUSG_DIV_UP(width * (max)(bits, 8u), 8u);

		return XUSG_DIV_UP(packed, 256u) * 256u;
	}

	uint32_t Gcd(uint32_t a, uint32_t b)
	{
		while (b)
		{
			const auto t = a % b;
			a = b;
			b = t;
		}

		return a ? a : 1u;
	}

	uint32_t AlignMultiple(uint32_t value, uint32_t base, uint32_t unit)
	{
		if (!unit || value % unit == 0) return value;

		const auto alignment = base / Gcd(base, unit) * unit;

		return XUSG_DIV_UP(value, alignment) * alignment;
	}

	uint32_t CopyTexelBytes(Format format)
	{
		if (IsCompressedFormat(format)) return GetVKFormatBits(format) <= 4 ? 8u : 16u;

		return (max)(GetVKFormatBits(format), 8u) / 8u;
	}

	VkImageAspectFlags ImageViewAspect(Format format, uint8_t plane)
	{
		if (!IsDepthStencilFormat(format)) return VK_IMAGE_ASPECT_COLOR_BIT;

		return plane ? VK_IMAGE_ASPECT_STENCIL_BIT : VK_IMAGE_ASPECT_DEPTH_BIT;
	}

	uint32_t BufferMemoryTypeBits(VkDevice device)
	{
		VkBufferCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		info.size = 256;
		info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		VkBuffer buffer = VK_NULL_HANDLE;
		if (vkCreateBuffer(device, &info, nullptr, &buffer) < 0) return 0;

		VkMemoryRequirements requirements = {};
		vkGetBufferMemoryRequirements(device, buffer, &requirements);
		vkDestroyBuffer(device, buffer, nullptr);

		return requirements.memoryTypeBits;
	}

	uint32_t ImageMemoryTypeBits(VkDevice device, VkFormat format, VkImageUsageFlags usage)
	{
		VkImageCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		info.imageType = VK_IMAGE_TYPE_2D;
		info.format = format;
		info.extent = { 1, 1, 1 };
		info.mipLevels = 1;
		info.arrayLayers = 1;
		info.samples = VK_SAMPLE_COUNT_1_BIT;
		info.tiling = VK_IMAGE_TILING_OPTIMAL;
		info.usage = usage;
		info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkImage image = VK_NULL_HANDLE;
		if (vkCreateImage(device, &info, nullptr, &image) < 0) return 0;

		VkMemoryRequirements requirements = {};
		vkGetImageMemoryRequirements(device, image, &requirements);
		vkDestroyImage(device, image, nullptr);

		return requirements.memoryTypeBits;
	}

	uint32_t HeapMemoryTypeBits(VkDevice device, MemoryType memoryType)
	{
		const auto bufferBits = BufferMemoryTypeBits(device);
		if (memoryType != MemoryType::DEFAULT) return bufferBits;

		const auto colorBits = ImageMemoryTypeBits(device, VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
		const auto depthBits = ImageMemoryTypeBits(device, VK_FORMAT_D32_SFLOAT,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
		auto imageBits = colorBits & depthBits;
		if (!imageBits) imageBits = colorBits ? colorBits : depthBits;
		const auto both = bufferBits & imageBits;

		return both ? both : imageBits;
	}
}

Heap_VK::Heap_VK() :
	m_device(VK_NULL_HANDLE),
	m_memory(VK_NULL_HANDLE),
	m_pDevice(nullptr),
	m_size(0),
	m_alignment(0),
	m_memoryTypeIndex(0),
	m_memoryType(MemoryType::DEFAULT)
{
}

Heap_VK::~Heap_VK()
{
	if (m_memory) vkFreeMemory(m_device, m_memory, nullptr);
}

bool Heap_VK::Create(const Device* pDevice, uint64_t byteSize, MemoryType memoryType,
	MemoryFlag memoryFlags, uint64_t alignment, const wchar_t* name)
{
	(void)memoryFlags;
	m_pDevice = dynamic_cast<const Device_VK*>(pDevice);
	assert(m_pDevice);
	m_device = m_pDevice->GetDevice();
	m_size = byteSize;
	m_alignment = alignment;
	m_memoryType = memoryType;

	VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	if (memoryType == MemoryType::READBACK)
		properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
	else if (memoryType == MemoryType::UPLOAD || memoryType == MemoryType::GPU_UPLOAD)
		properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	m_memoryTypeIndex = m_pDevice->FindMemoryType(HeapMemoryTypeBits(m_device, memoryType), properties);
	assert(m_memoryTypeIndex != UINT32_MAX);

	VkMemoryAllocateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	info.allocationSize = byteSize;
	info.memoryTypeIndex = m_memoryTypeIndex;
	XUSG_VK_CHECK(vkAllocateMemory(m_device, &info, nullptr, &m_memory), cerr, false);
	m_pDevice->SetDebugName(VK_OBJECT_TYPE_DEVICE_MEMORY, reinterpret_cast<uint64_t>(m_memory), name);

	return true;
}

uint64_t Heap_VK::GetSize() const
{
	return m_size;
}

uint64_t Heap_VK::GetAlignment() const
{
	return m_alignment;
}

MemoryType Heap_VK::GetType() const
{
	return m_memoryType;
}

void Heap_VK::Create(void* pDeviceHandle, void* pHeapHandle, const wchar_t* name)
{
	m_device = static_cast<VkDevice>(pDeviceHandle);
	m_memory = static_cast<VkDeviceMemory>(pHeapHandle);
	(void)name;
}

void Heap_VK::SetName(const wchar_t* name)
{
	if (name) m_name = name;
}

void* Heap_VK::GetHandle() const
{
	return m_memory;
}

VkDeviceMemory Heap_VK::GetMemory() const
{
	return m_memory;
}

uint32_t Heap_VK::GetMemoryType() const
{
	return m_memoryTypeIndex;
}

//--------------------------------------------------------------------------------------

Resource_VK::Resource_VK() :
	m_device(VK_NULL_HANDLE),
	m_pDevice(nullptr),
	m_image(VK_NULL_HANDLE),
	m_buffer(VK_NULL_HANDLE),
	m_memory(VK_NULL_HANDLE),
	m_address(0),
	m_pDataBegin(nullptr),
	m_is3D(false),
	m_isImage(false),
	m_ownsImage(false),
	m_ownsBuffer(false),
	m_ownsMemory(false),
	m_hasPromotion(false),
	m_byteSize(0),
	m_width(0),
	m_height(1),
	m_depthOrArray(1),
	m_mipLevels(1),
	m_sampleCount(1),
	m_vkFormat(VK_FORMAT_UNDEFINED)
{
}

Resource_VK::~Resource_VK()
{
	for (auto& view : m_resourceViews)
		if (view && view->ImageView) vkDestroyImageView(m_device, view->ImageView, nullptr);

	if (m_pDataBegin) vkUnmapMemory(m_device, m_memory);
	if (m_buffer) UnregisterBuffer(m_buffer);
	if (m_ownsImage && m_image) vkDestroyImage(m_device, m_image, nullptr);
	if (m_ownsBuffer && m_buffer) vkDestroyBuffer(m_device, m_buffer, nullptr);
	if (m_ownsMemory && m_memory) vkFreeMemory(m_device, m_memory, nullptr);
}

bool Resource_VK::ReadFromSubresource(void* pDstData, uint32_t dstRowPitch, uint32_t dstDepthPitch,
	uint32_t srcSubresource, const BoxRange* pSrcBox)
{
	(void)dstDepthPitch;
	(void)srcSubresource;
	if (!m_pDataBegin || !pDstData) return false;

	const auto row = pSrcBox ? pSrcBox->Right - pSrcBox->Left : static_cast<uint32_t>(m_width);
	memcpy(pDstData, m_pDataBegin, static_cast<size_t>(dstRowPitch ? dstRowPitch : row));

	return true;
}

bool Resource_VK::WriteToSubresource(uint32_t dstSubresource, const void* pSrcData, uint32_t srcRowPitch,
	uint32_t srcDepthPitch, const BoxRange* pDstBox)
{
	(void)dstSubresource;
	(void)srcDepthPitch;
	(void)pDstBox;
	if (!m_pDataBegin || !pSrcData) return false;

	memcpy(m_pDataBegin, pSrcData, srcRowPitch);

	return true;
}

uint32_t Resource_VK::SetBarrier(ResourceBarrier* pBarriers, ResourceState dstState, uint32_t numBarriers,
	uint32_t subresource, BarrierFlag flags, ResourceState srcState, uint32_t threadIdx)
{
	assert(m_states.empty() || (!m_image && !m_buffer) || !m_states.empty());
	if (!m_states.empty())
	{
		const auto& state = m_states[threadIdx][subresource == XUSG_BARRIER_ALL_SUBRESOURCES ? 0 : subresource];
		if ((state & dstState) != dstState || dstState == ResourceState::UNORDERED_ACCESS || dstState == ResourceState::COMMON)
		{
			srcState = Transition(dstState, subresource, flags, srcState, threadIdx);

			static const auto depthOpState = ResourceState::DEPTH_READ | ResourceState::DEPTH_WRITE;
			static const auto promotionState = ResourceState::ALL_SHADER_RESOURCE | ResourceState::COPY_SOURCE | ResourceState::COPY_DEST;
			const auto isPromoted = !m_isImage && srcState == ResourceState::COMMON &&
				(dstState & depthOpState) == ResourceState::COMMON &&
				(m_hasPromotion || (dstState & promotionState) != ResourceState::COMMON);

			if (!isPromoted) pBarriers[numBarriers++] = { this, srcState, dstState, subresource, flags };
		}
	}

	return numBarriers;
}

ResourceState Resource_VK::Transition(ResourceState dstState, uint32_t subresource,
	BarrierFlag flags, ResourceState srcState, uint32_t threadIdx)
{
	const bool isBeginOnly = (flags & BarrierFlag::BEGIN_ONLY) == BarrierFlag::BEGIN_ONLY;
	if (subresource == XUSG_BARRIER_ALL_SUBRESOURCES)
	{
		srcState = srcState == ResourceState::AUTO ? m_states[threadIdx][0] : srcState;
		if (!isBeginOnly)
			for (auto& state : m_states[threadIdx]) state = dstState;
	}
	else
	{
		srcState = srcState == ResourceState::AUTO ? m_states[threadIdx][subresource] : srcState;
		m_states[threadIdx][subresource] = isBeginOnly ? srcState : dstState;
	}

	return srcState;
}

ResourceState Resource_VK::GetResourceState(uint32_t subresource, uint32_t threadIdx) const
{
	return !m_states.empty() && threadIdx < m_states.size() && subresource < m_states[threadIdx].size() ?
		m_states[threadIdx][subresource] : ResourceState::COMMON;
}

uint64_t Resource_VK::GetWidth() const
{
	return m_width;
}

uint64_t Resource_VK::GetVirtualAddress(int offset) const
{
	return m_address + offset;
}

void Resource_VK::Unmap(const Range* pWrittenRange)
{
	(void)pWrittenRange;
	if (m_pDataBegin && m_memory)
	{
		vkUnmapMemory(m_device, m_memory);
		m_pDataBegin = nullptr;
	}
}

void Resource_VK::GetTiling(uint32_t* pTotalNumTiles, PackedMipInfo* pPackedMipInfo,
	TileShape* pStandardTileShapeForNonPackedMips, uint32_t* pNumSubresourceTilings,
	uint32_t firstSubresourceTiling, SubresourceTiling* pSubresourceTilingsForNonPackedMips) const
{
	assert(!"GetTiling is not implemented on Vulkan");
	(void)firstSubresourceTiling;
	if (pTotalNumTiles) *pTotalNumTiles = 0;
	if (pPackedMipInfo) *pPackedMipInfo = {};
	if (pStandardTileShapeForNonPackedMips) *pStandardTileShapeForNonPackedMips = {};
	if (pNumSubresourceTilings) *pNumSubresourceTilings = 0;
	if (pSubresourceTilingsForNonPackedMips) *pSubresourceTilingsForNonPackedMips = {};
}

void Resource_VK::Create(void* pDeviceHandle, void* pResourceHandle, const wchar_t* name, uint32_t maxThreads)
{
	m_device = static_cast<VkDevice>(pDeviceHandle);
	m_image = static_cast<VkImage>(pResourceHandle);
	m_isImage = true;
	m_ownsImage = false;
	m_undefined.assign(1, 1);
	setState(ResourceState::COMMON, 1, maxThreads);
	SetName(name);
}

void Resource_VK::SetName(const wchar_t* name)
{
	if (!name) return;

	m_name = name;
	if (m_pDevice)
	{
		const auto handle = m_isImage ? reinterpret_cast<uint64_t>(m_image) : reinterpret_cast<uint64_t>(m_buffer);
		m_pDevice->SetDebugName(m_isImage ? VK_OBJECT_TYPE_IMAGE : VK_OBJECT_TYPE_BUFFER, handle, name);
	}
}

void* Resource_VK::GetHandle() const
{
	return m_isImage ? static_cast<void*>(m_image) : static_cast<void*>(m_buffer);
}

bool Resource_VK::Is3D() const
{
	return m_is3D;
}

bool Resource_VK::IsImage() const
{
	return m_isImage;
}

VkImage Resource_VK::GetImage() const
{
	return m_image;
}

VkBuffer Resource_VK::GetBuffer() const
{
	return m_buffer;
}

VkFormat Resource_VK::GetVkFormat() const
{
	return m_vkFormat;
}

uint32_t Resource_VK::GetHeight() const
{
	return m_height;
}

uint32_t Resource_VK::GetDepthOrArray() const
{
	return m_depthOrArray;
}

uint32_t Resource_VK::GetMipLevels() const
{
	return m_mipLevels;
}

uint32_t Resource_VK::GetSampleCount() const
{
	return m_sampleCount;
}

void Resource_VK::WrapImage(VkDevice device, VkImage image, VkFormat format, uint32_t width, uint32_t height)
{
	m_device = device;
	m_image = image;
	m_isImage = true;
	m_ownsImage = false;
	m_undefined.assign(1, 1);
	m_vkFormat = format;
	m_width = width;
	m_height = height;
	m_depthOrArray = 1;
	m_mipLevels = 1;
	setState(ResourceState::COMMON, 1, 1);
}

uint64_t Resource_VK::GetByteSize() const
{
	return m_byteSize;
}

bool Resource_VK::IsUndefinedLayout(uint32_t subresource) const
{
	return subresource < m_undefined.size() && m_undefined[subresource];
}

bool Resource_VK::needsUndefinedLayout(uint32_t subresource) const
{
	return isDiscarded(subresource) || IsUndefinedLayout(subresource);
}

void Resource_VK::consumeLayoutMark(uint32_t subresource)
{
	consumeDiscarded(subresource);
	if (subresource < m_undefined.size()) m_undefined[subresource] = 0;
}

uint32_t Resource_VK::subresourceCount() const
{
	const auto mips = m_mipLevels ? m_mipLevels : 1u;
	if (!m_isImage || m_is3D) return mips;

	return mips * (m_depthOrArray ? m_depthOrArray : 1u);
}

uint32_t Resource_VK::subresourceIndex(uint32_t mip, uint32_t layer) const
{
	const auto mips = m_mipLevels ? m_mipLevels : 1u;

	return m_is3D ? mip : mip + layer * mips;
}

bool Resource_VK::isDiscarded(uint32_t subresource) const
{
	return subresource < m_discarded.size() && m_discarded[subresource];
}

void Resource_VK::consumeDiscarded(uint32_t subresource)
{
	if (subresource < m_discarded.size()) m_discarded[subresource] = 0;
}

void Resource_VK::discardRange(const VkImageSubresourceRange& range, uint32_t& baseMip, uint32_t& mipCount,
	uint32_t& baseLayer, uint32_t& layerCount) const
{
	const auto mips = m_mipLevels ? m_mipLevels : 1u;
	const auto layers = m_is3D ? 1u : (m_depthOrArray ? m_depthOrArray : 1u);
	baseMip = range.baseMipLevel;
	baseLayer = m_is3D ? 0 : range.baseArrayLayer;
	mipCount = range.levelCount == VK_REMAINING_MIP_LEVELS ? (baseMip < mips ? mips - baseMip : 0) :
		(min)(range.levelCount, baseMip < mips ? mips - baseMip : 0);
	layerCount = m_is3D || range.layerCount == VK_REMAINING_ARRAY_LAYERS ?
		(baseLayer < layers ? layers - baseLayer : 0) : (min)(range.layerCount, baseLayer < layers ? layers - baseLayer : 0);
	if (m_is3D) layerCount = mipCount ? 1 : 0;
}

void Resource_VK::Discard(uint32_t firstSubresource, uint32_t numSubresources)
{
	const auto count = subresourceCount();
	if (!count) return;
	if (!numSubresources)
	{
		firstSubresource = 0;
		numSubresources = count;
	}
	if (firstSubresource >= count) return;
	numSubresources = (min)(numSubresources, count - firstSubresource);
	if (m_discarded.size() < count) m_discarded.resize(count);
	for (auto i = 0u; i < numSubresources; ++i) m_discarded[firstSubresource + i] = 1;
}

VkAttachmentLoadOp Resource_VK::ConsumeDiscardLoadOp(uint32_t baseMip, uint32_t mipCount, uint32_t baseLayer, uint32_t layerCount)
{
	if (!mipCount) mipCount = 1;
	if (!layerCount) layerCount = 1;
	if (m_is3D)
	{
		baseLayer = 0;
		layerCount = 1;
	}

	auto any = false;
	auto all = true;
	const auto mips = m_mipLevels ? m_mipLevels : 1u;
	for (auto layer = baseLayer; layer < baseLayer + layerCount; ++layer)
	{
		for (auto mip = baseMip; mip < baseMip + mipCount && mip < mips; ++mip)
		{
			if (isDiscarded(subresourceIndex(mip, layer))) any = true;
			else all = false;
		}
	}
	if (!any) return VK_ATTACHMENT_LOAD_OP_LOAD;

	for (auto layer = baseLayer; layer < baseLayer + layerCount; ++layer)
		for (auto mip = baseMip; mip < baseMip + mipCount && mip < mips; ++mip)
			if (isDiscarded(subresourceIndex(mip, layer))) consumeDiscarded(subresourceIndex(mip, layer));

	return all ? VK_ATTACHMENT_LOAD_OP_DONT_CARE : VK_ATTACHMENT_LOAD_OP_LOAD;
}

void Resource_VK::AppendImageBarrier(VkImageMemoryBarrier2 image, std::vector<VkImageMemoryBarrier2>& barriers)
{
	uint32_t baseMip = 0, mipCount = 0, baseLayer = 0, layerCount = 0;
	discardRange(image.subresourceRange, baseMip, mipCount, baseLayer, layerCount);
	auto any = false;
	for (auto layer = baseLayer; layer < baseLayer + layerCount && !any; ++layer)
		for (auto mip = baseMip; mip < baseMip + mipCount; ++mip)
			if (needsUndefinedLayout(subresourceIndex(mip, layer))) any = true;

	if (!any || image.oldLayout == VK_IMAGE_LAYOUT_UNDEFINED)
	{
		if (image.oldLayout == VK_IMAGE_LAYOUT_UNDEFINED || any)
		{
			for (auto layer = baseLayer; layer < baseLayer + layerCount; ++layer)
				for (auto mip = baseMip; mip < baseMip + mipCount; ++mip)
					consumeLayoutMark(subresourceIndex(mip, layer));
			if (any)
			{
				image.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				image.srcAccessMask = VK_ACCESS_2_NONE;
			}
		}
		barriers.push_back(image);

		return;
	}

	for (auto layer = baseLayer; layer < baseLayer + layerCount; ++layer)
	{
		auto mip = baseMip;
		while (mip < baseMip + mipCount)
		{
			const auto undefined = needsUndefinedLayout(subresourceIndex(mip, layer));
			auto end = mip + 1;
			while (end < baseMip + mipCount && needsUndefinedLayout(subresourceIndex(end, layer)) == undefined) ++end;

			auto piece = image;
			piece.subresourceRange.baseMipLevel = mip;
			piece.subresourceRange.levelCount = end - mip;
			piece.subresourceRange.baseArrayLayer = layer;
			piece.subresourceRange.layerCount = 1;
			if (undefined)
			{
				piece.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				piece.srcAccessMask = VK_ACCESS_2_NONE;
				for (auto i = mip; i < end; ++i) consumeLayoutMark(subresourceIndex(i, layer));
			}
			barriers.push_back(piece);
			mip = end;
		}
	}
}

bool Resource_VK::CreateUploadBuffer(const Device* pDevice, size_t byteSize)
{
	XUSG_N_RETURN(initialize(pDevice ? pDevice : m_pDevice, 1), false);
	if (m_buffer && m_byteSize >= byteSize) return true;

	return createBuffer(nullptr, 0, byteSize, ResourceFlag::NONE, MemoryType::UPLOAD);
}

void* Resource_VK::GetMappedData() const
{
	return m_pDataBegin;
}

bool Resource_VK::initialize(const Device* pDevice, uint32_t maxThreads)
{
	(void)maxThreads;
	if (!pDevice) return m_device != VK_NULL_HANDLE;

	m_pDevice = dynamic_cast<const Device_VK*>(pDevice);
	assert(m_pDevice);
	m_device = m_pDevice->GetDevice();
	m_states.clear();
	m_pDataBegin = nullptr;

	return true;
}

bool Resource_VK::allocateMemory(VkMemoryRequirements requirements, MemoryType memoryType, bool hostVisible)
{
	VkMemoryPropertyFlags properties = hostVisible ?
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT :
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	if (memoryType == MemoryType::UPLOAD || memoryType == MemoryType::READBACK) hostVisible = true;
	if (memoryType == MemoryType::READBACK)
		properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
	else if (hostVisible)
		properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	VkMemoryAllocateFlagsInfo flags = {};
	flags.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
	flags.flags = m_buffer ? VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT : 0;

	VkMemoryAllocateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	info.pNext = m_buffer ? &flags : nullptr;
	info.allocationSize = requirements.size;
	info.memoryTypeIndex = m_pDevice->FindMemoryType(requirements.memoryTypeBits, properties);
	XUSG_VK_CHECK(vkAllocateMemory(m_device, &info, nullptr, &m_memory), cerr, false);
	m_ownsMemory = true;
	m_byteSize = requirements.size;
	if (m_image) XUSG_VK_CHECK(vkBindImageMemory(m_device, m_image, m_memory, 0), cerr, false);
	if (m_buffer) XUSG_VK_CHECK(vkBindBufferMemory(m_device, m_buffer, m_memory, 0), cerr, false);

	if (m_buffer)
	{
		VkBufferDeviceAddressInfo address = {};
		address.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		address.buffer = m_buffer;
		m_address = vkGetBufferDeviceAddress(m_device, &address);
		BufferNative native = {};
		native.Buffer = m_buffer;
		native.Address = m_address;
		native.Size = m_byteSize;
		RegisterBuffer(native);
	}

	if (hostVisible) XUSG_VK_CHECK(vkMapMemory(m_device, m_memory, 0, m_byteSize, 0, &m_pDataBegin), cerr, false);

	return true;
}

void Resource_VK::setState(ResourceState state, uint32_t subresources, uint32_t maxThreads)
{
	m_states.resize(maxThreads ? maxThreads : 1);
	for (auto& states : m_states) states.assign(subresources ? subresources : 1, state);
}

bool Resource_VK::createBuffer(const Heap* pHeap, uint64_t heapOffset, size_t byteWidth,
	ResourceFlag resourceFlags, MemoryType memoryType)
{
	if (m_buffer) UnregisterBuffer(m_buffer);
	if (m_ownsBuffer && m_buffer) vkDestroyBuffer(m_device, m_buffer, nullptr);

	uint32_t sharingCount = 0;
	const uint32_t* pSharingFamilies = nullptr;
	if (m_pDevice) m_pDevice->GetSharingFamilies(sharingCount, pSharingFamilies);

	VkBufferCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.size = byteWidth;
	info.usage = GetVKBufferUsage(resourceFlags);
	info.sharingMode = sharingCount > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
	info.queueFamilyIndexCount = sharingCount;
	info.pQueueFamilyIndices = pSharingFamilies;
	if (m_pDevice && m_pDevice->HasConditionalRendering())
		info.usage |= VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT;
	XUSG_VK_CHECK(vkCreateBuffer(m_device, &info, nullptr, &m_buffer), cerr, false);
	m_ownsBuffer = true;
	m_isImage = false;
	m_width = byteWidth;
	m_byteSize = byteWidth;

	if (pHeap)
	{
		const auto heap = dynamic_cast<const Heap_VK*>(pHeap);
		VkMemoryRequirements requirements = {};
		vkGetBufferMemoryRequirements(m_device, m_buffer, &requirements);
		assert((requirements.memoryTypeBits & (1u << heap->GetMemoryType())) &&
			"Placed buffer memory type is incompatible with the heap");
		if ((requirements.memoryTypeBits & (1u << heap->GetMemoryType())) == 0) return false;
		XUSG_VK_CHECK(vkBindBufferMemory(m_device, m_buffer, heap->GetMemory(), heapOffset), cerr, false);
		VkBufferDeviceAddressInfo address = {};
		address.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		address.buffer = m_buffer;
		m_address = vkGetBufferDeviceAddress(m_device, &address);
		BufferNative native = {};
		native.Buffer = m_buffer;
		native.Address = m_address;
		native.Size = byteWidth;
		RegisterBuffer(native);
	}
	else
	{
		VkMemoryRequirements requirements = {};
		vkGetBufferMemoryRequirements(m_device, m_buffer, &requirements);
		const auto host = memoryType == MemoryType::UPLOAD || memoryType == MemoryType::READBACK || memoryType == MemoryType::GPU_UPLOAD;
		XUSG_N_RETURN(allocateMemory(requirements, memoryType, host), false);
	}

	return true;
}

bool Resource_VK::createImage(const Heap* pHeap, uint64_t heapOffset, uint32_t width, uint32_t height,
	uint32_t depth, Format format, uint16_t arraySize, ResourceFlag resourceFlags, uint8_t numMips,
	uint8_t sampleCount, MemoryType memoryType, bool is3D, bool cube, uint8_t numCastableFormats,
	const Format* pCastableFormats)
{
	m_is3D = is3D;
	m_isImage = true;
	m_width = width;
	m_height = height;
	m_depthOrArray = is3D ? depth : arraySize;
	m_mipLevels = numMips ? numMips : 1;
	m_sampleCount = sampleCount ? sampleCount : 1;
	m_vkFormat = GetVKFormat(format);
	m_undefined.assign(subresourceCount(), 1);

	vector<VkFormat> viewFormats;
	VkImageFormatListCreateInfo formatList = {};
	formatList.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO;
	if (numCastableFormats && pCastableFormats)
	{
		viewFormats.push_back(m_vkFormat);
		for (auto i = 0u; i < numCastableFormats; ++i)
		{
			const auto viewFormat = GetVKFormat(pCastableFormats[i]);
			if (viewFormat != VK_FORMAT_UNDEFINED && viewFormat != m_vkFormat) viewFormats.push_back(viewFormat);
		}
		if (viewFormats.size() > 1)
		{
			formatList.viewFormatCount = static_cast<uint32_t>(viewFormats.size());
			formatList.pViewFormats = viewFormats.data();
		}
	}

	uint32_t sharingCount = 0;
	const uint32_t* pSharingFamilies = nullptr;
	if (m_pDevice) m_pDevice->GetSharingFamilies(sharingCount, pSharingFamilies);

	VkImageCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	info.pNext = formatList.viewFormatCount ? &formatList : nullptr;
	info.sharingMode = sharingCount > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
	info.queueFamilyIndexCount = sharingCount;
	info.pQueueFamilyIndices = pSharingFamilies;
	info.flags = cube ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
	if (formatList.viewFormatCount) info.flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
	info.imageType = is3D ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
	info.format = m_vkFormat;
	info.extent = { width, (max)(height, 1u), is3D ? depth : 1u };
	info.mipLevels = m_mipLevels;
	info.arrayLayers = is3D ? 1u : arraySize;
	info.samples = static_cast<VkSampleCountFlagBits>(m_sampleCount);
	info.tiling = VK_IMAGE_TILING_OPTIMAL;
	info.usage = GetVKImageUsage(resourceFlags);
	info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	XUSG_VK_CHECK(vkCreateImage(m_device, &info, nullptr, &m_image), cerr, false);
	m_ownsImage = true;

	if (pHeap)
	{
		const auto heap = dynamic_cast<const Heap_VK*>(pHeap);
		VkMemoryRequirements requirements = {};
		vkGetImageMemoryRequirements(m_device, m_image, &requirements);
		assert((requirements.memoryTypeBits & (1u << heap->GetMemoryType())) &&
			"Placed image memory type is incompatible with the heap");
		if ((requirements.memoryTypeBits & (1u << heap->GetMemoryType())) == 0) return false;
		XUSG_VK_CHECK(vkBindImageMemory(m_device, m_image, heap->GetMemory(), heapOffset), cerr, false);
	}
	else
	{
		VkMemoryRequirements requirements = {};
		vkGetImageMemoryRequirements(m_device, m_image, &requirements);
		m_byteSize = requirements.size;
		XUSG_N_RETURN(allocateMemory(requirements, memoryType, false), false);
	}

	return true;
}

shared_ptr<ResourceView_VK> Resource_VK::storeView(ResourceViewType type, VkDescriptorType vkType, bool image, bool texel)
{
	auto view = make_shared<ResourceView_VK>();
	*view = {};
	view->Type = type;
	view->pResource = this;
	view->VkType = vkType;
	view->IsImage = image;
	view->IsTexelBuffer = texel;
	view->ViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	m_resourceViews.push_back(view);

	return view;
}

//--------------------------------------------------------------------------------------

ConstantBuffer_VK::ConstantBuffer_VK()
{
}

ConstantBuffer_VK::~ConstantBuffer_VK()
{
}

bool ConstantBuffer_VK::Create(const Device* pDevice, size_t byteWidth, uint32_t numCBVs,
	const size_t* cbvByteOffsets, MemoryType memoryType, MemoryFlag memoryFlags, const wchar_t* name)
{
	(void)memoryFlags;
	XUSG_N_RETURN(CreateResource(pDevice, byteWidth, memoryType), false);
	SetName(name);
	m_cbvs.resize(numCBVs);
	m_cbvByteOffsets.resize(numCBVs);
	for (auto i = 0u; i < numCBVs; ++i)
	{
		const auto offset = cbvByteOffsets ? cbvByteOffsets[i] : AlignCBV(byteWidth) * i;
		m_cbvByteOffsets[i] = offset;
		m_cbvs[i] = CreateCBV(static_cast<uint32_t>(byteWidth), offset);
	}

	return true;
}

bool ConstantBuffer_VK::Create(const Device* pDevice, const Heap* pHeap, uint64_t heapOffset, size_t byteWidth,
	uint32_t numCBVs, const size_t* cbvByteOffsets, const wchar_t* name)
{
	XUSG_N_RETURN(CreateResource(pDevice, pHeap, heapOffset, byteWidth), false);
	SetName(name);
	m_cbvs.resize(numCBVs);
	m_cbvByteOffsets.resize(numCBVs);
	for (auto i = 0u; i < numCBVs; ++i)
	{
		const auto offset = cbvByteOffsets ? cbvByteOffsets[i] : 0;
		m_cbvByteOffsets[i] = offset;
		m_cbvs[i] = CreateCBV(static_cast<uint32_t>(byteWidth), offset);
	}

	return true;
}

bool ConstantBuffer_VK::CreateResource(const Device* pDevice, size_t byteWidth, MemoryType memoryType,
	MemoryFlag memoryFlags, ResourceState initialResourceState)
{
	(void)memoryFlags;
	XUSG_N_RETURN(initialize(pDevice, 1), false);
	XUSG_N_RETURN(createBuffer(nullptr, 0, byteWidth, ResourceFlag::NONE, memoryType), false);
	setState(initialResourceState, 1, 1);

	return true;
}

bool ConstantBuffer_VK::CreateResource(const Device* pDevice, const Heap* pHeap, uint64_t heapOffset, size_t byteWidth,
	ResourceState initialResourceState)
{
	XUSG_N_RETURN(initialize(pDevice, 1), false);
	XUSG_N_RETURN(createBuffer(pHeap, heapOffset, byteWidth, ResourceFlag::NONE, MemoryType::DEFAULT), false);
	setState(initialResourceState, 1, 1);

	return true;
}

bool ConstantBuffer_VK::Upload(CommandList* pCommandList, Resource* pUploader, const void* pData,
	size_t size, size_t byteOffset, ResourceState srcState, ResourceState dstState)
{
	const auto uploader = dynamic_cast<Resource_VK*>(pUploader);
	assert(uploader);
	XUSG_N_RETURN(uploader->CreateUploadBuffer(m_pDevice, size), false);
	auto dst = static_cast<uint8_t*>(uploader->GetMappedData());
	memcpy(dst, pData, size);

	ResourceBarrier barrier = { this, srcState == ResourceState::AUTO ? GetResourceState() : srcState, ResourceState::COPY_DEST };
	if (barrier.StateBefore != barrier.StateAfter)
	{
		Transition(ResourceState::COPY_DEST);
		pCommandList->Barrier(1, &barrier);
	}
	pCommandList->CopyBufferRegion(this, byteOffset, uploader, 0, size);
	barrier = { this, ResourceState::COPY_DEST, dstState };
	Transition(dstState);
	pCommandList->Barrier(1, &barrier);

	return true;
}

bool ConstantBuffer_VK::Upload(CommandList* pCommandList, uint32_t cbvIndex, Resource* pUploader,
	const void* pData, size_t byteSize, ResourceState srcState, ResourceState dstState)
{
	return Upload(pCommandList, pUploader, pData, byteSize, m_cbvByteOffsets[cbvIndex], srcState, dstState);
}

Descriptor ConstantBuffer_VK::CreateCBV(uint32_t byteSize, size_t byteOffset)
{
	auto view = storeView(ResourceViewType::CBV, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, false, false);
	view->Address = m_address + byteOffset;
	view->Range = byteSize;

	return view.get();
}

void* ConstantBuffer_VK::Map(uint32_t cbvIndex, uintptr_t readBegin, uintptr_t readEnd)
{
	(void)readBegin;
	(void)readEnd;

	return Map(nullptr, cbvIndex);
}

void* ConstantBuffer_VK::Map(const Range* pReadRange, uint32_t cbvIndex)
{
	(void)pReadRange;
	if (!m_pDataBegin && m_memory) vkMapMemory(m_device, m_memory, 0, m_byteSize, 0, &m_pDataBegin);

	return static_cast<uint8_t*>(m_pDataBegin) + (cbvIndex < m_cbvByteOffsets.size() ? m_cbvByteOffsets[cbvIndex] : 0);
}

const Descriptor& ConstantBuffer_VK::GetCBV(uint32_t index) const
{
	return m_cbvs[index];
}

uint32_t ConstantBuffer_VK::GetCBVOffset(uint32_t index) const
{
	return static_cast<uint32_t>(m_cbvByteOffsets[index]);
}

size_t ConstantBuffer_VK::GetTiledResourceTileSize() const
{
	return 65536;
}

void ConstantBuffer_VK::GetAllocationInfo(uint64_t& byteSize, uint64_t& alignment,
	const Device* pDevice, size_t byteWidth, uint64_t requiredAlignment)
{
	(void)pDevice;
	alignment = (max)(requiredAlignment, 256ull);
	byteSize = AlignCBV(byteWidth);
}

size_t ConstantBuffer_VK::AlignCBV(size_t byteSize)
{
	return XUSG_DIV_UP(byteSize, 256) * 256;
}

//--------------------------------------------------------------------------------------

ShaderResource_VK::ShaderResource_VK() :
	m_format(Format::UNKNOWN)
{
}

ShaderResource_VK::~ShaderResource_VK()
{
}

const Descriptor& ShaderResource_VK::GetSRV(uint32_t index) const
{
	return m_srvs[index];
}

Format ShaderResource_VK::GetFormat() const
{
	return m_format;
}

bool ShaderResource_VK::checkCastableFormats(Format& format, uint8_t numUavFormats, const Format* uavFormats)
{
	(void)numUavFormats;
	(void)uavFormats;

	return format != Format::UNKNOWN;
}

bool ShaderResource_VK::getTypelessFormat(Format& format)
{
	(void)format;

	return true;
}

//--------------------------------------------------------------------------------------

Texture_VK::Texture_VK() :
	m_cube(false)
{
}

Texture_VK::~Texture_VK()
{
}

bool Texture_VK::Create(const Device* pDevice, uint32_t width, uint32_t height, Format format,
	uint16_t arraySize, ResourceFlag resourceFlags, uint8_t numMips, uint8_t sampleCount, bool isCubeMap,
	MemoryFlag memoryFlags, const wchar_t* name, uint16_t srvComponentMapping, TextureLayout textureLayout,
	uint8_t numUavFormats, const Format* uavFormats, uint32_t maxThreads)
{
	(void)memoryFlags;
	(void)textureLayout;
	XUSG_N_RETURN(CreateResource(pDevice, width, height, format, arraySize, resourceFlags, numMips, sampleCount,
		MemoryFlag::NONE, ResourceState::COMMON, TextureLayout::UNKNOWN, numUavFormats, uavFormats, maxThreads), false);
	m_cube = isCubeMap;
	SetName(name);
	createSRVs(arraySize, format, numMips, sampleCount > 1, isCubeMap, srvComponentMapping);
	createSingleLevelSRVs(arraySize, numMips, format, sampleCount > 1, isCubeMap, srvComponentMapping);
	if ((resourceFlags & ResourceFlag::ALLOW_UNORDERED_ACCESS) != ResourceFlag::NONE)
	{
		createUAVs(arraySize, format, numMips);
		if (numUavFormats && uavFormats)
			for (auto i = 0u; i < numUavFormats; ++i) createCastableUAVs(arraySize, uavFormats[i], numMips);
	}

	return true;
}

bool Texture_VK::Create(const Device* pDevice, const Heap* pHeap, uint64_t heapOffset, uint32_t width, uint32_t height,
	Format format, uint16_t arraySize, ResourceFlag resourceFlags, uint8_t numMips, uint8_t sampleCount, bool isCubeMap,
	const wchar_t* name, uint16_t srvComponentMapping, TextureLayout textureLayout, uint8_t numUavFormats,
	const Format* uavFormats, uint32_t maxThreads)
{
	(void)textureLayout;
	XUSG_N_RETURN(CreateResource(pDevice, pHeap, heapOffset, width, height, format, arraySize, resourceFlags,
		numMips, sampleCount, ResourceState::COMMON, TextureLayout::UNKNOWN, numUavFormats, uavFormats, maxThreads), false);
	m_cube = isCubeMap;
	SetName(name);
	createSRVs(arraySize, format, numMips, sampleCount > 1, isCubeMap, srvComponentMapping);
	createSingleLevelSRVs(arraySize, numMips, format, sampleCount > 1, isCubeMap, srvComponentMapping);
	if ((resourceFlags & ResourceFlag::ALLOW_UNORDERED_ACCESS) != ResourceFlag::NONE) createUAVs(arraySize, format, numMips);

	return true;
}

bool Texture_VK::CreateResource(const Device* pDevice, uint32_t width, uint32_t height, Format format,
	uint16_t arraySize, ResourceFlag resourceFlags, uint8_t numMips, uint8_t sampleCount, MemoryFlag memoryFlags,
	ResourceState initialResourceState, TextureLayout textureLayout, uint8_t numCastableFormats,
	const Format* pCastableFormats, uint32_t maxThreads)
{
	(void)memoryFlags;
	(void)textureLayout;
	XUSG_N_RETURN(initialize(pDevice, maxThreads), false);
	m_format = format;
	m_hasPromotion = (resourceFlags & ResourceFlag::ALLOW_SIMULTANEOUS_ACCESS) == ResourceFlag::ALLOW_SIMULTANEOUS_ACCESS;
	XUSG_N_RETURN(createImage(nullptr, 0, width, height, 1, format, arraySize, resourceFlags, numMips, sampleCount,
		MemoryType::DEFAULT, false, false, numCastableFormats, pCastableFormats), false);
	setState(initialResourceState, static_cast<uint32_t>(arraySize) * (numMips ? numMips : 1), maxThreads);

	return true;
}

bool Texture_VK::CreateResource(const Device* pDevice, const Heap* pHeap, uint64_t heapOffset, uint32_t width,
	uint32_t height, Format format, uint16_t arraySize, ResourceFlag resourceFlags, uint8_t numMips,
	uint8_t sampleCount, ResourceState initialResourceState, TextureLayout textureLayout, uint8_t numCastableFormats,
	const Format* pCastableFormats, uint32_t maxThreads)
{
	(void)textureLayout;
	XUSG_N_RETURN(initialize(pDevice, maxThreads), false);
	m_format = format;
	XUSG_N_RETURN(createImage(pHeap, heapOffset, width, height, 1, format, arraySize, resourceFlags, numMips,
		sampleCount, MemoryType::DEFAULT, false, false, numCastableFormats, pCastableFormats), false);
	setState(initialResourceState, static_cast<uint32_t>(arraySize) * (numMips ? numMips : 1), maxThreads);

	return true;
}

bool Texture_VK::Upload(CommandList* pCommandList, Resource* pUploader, const SubresourceData* pSubresourceData,
	uint32_t numSubresources, ResourceState dstState, uint32_t firstSubresource, uint32_t threadIdx)
{
	XUSG_N_RETURN(pSubresourceData, false);
	const auto uploader = dynamic_cast<Resource_VK*>(pUploader);
	assert(uploader);
	const auto uploadSize = GetRequiredIntermediateSize(firstSubresource, numSubresources);
	XUSG_N_RETURN(uploader->CreateUploadBuffer(m_pDevice, uploadSize), false);

	vector<ResourceBarrier> barriers(numSubresources);
	auto numBarriers = 0u;
	for (auto i = 0u; i < numSubresources; ++i)
		numBarriers = SetBarrier(barriers.data(), ResourceState::COPY_DEST, numBarriers, firstSubresource + i,
			BarrierFlag::NONE, ResourceState::AUTO, threadIdx);
	if (numBarriers) pCommandList->Barrier(numBarriers, barriers.data());

	auto dst = static_cast<uint8_t*>(uploader->GetMappedData());
	auto offset = 0ull;
	const auto commandList = dynamic_cast<CommandList_VK*>(pCommandList);
	assert(commandList);
	for (auto i = 0u; i < numSubresources; ++i)
	{
		const auto subresource = firstSubresource + i;
		const auto mip = subresource % (max)(m_mipLevels, 1u);
		const auto width = static_cast<uint32_t>((max)(m_width >> mip, 1ull));
		const auto height = (max)(m_height >> mip, 1u);
		const auto pitch = RowPitch(m_format, width);
		const auto srcPitch = static_cast<uint32_t>(pSubresourceData[i].RowPitch);
		for (auto row = 0u; row < height; ++row)
			memcpy(dst + offset + static_cast<size_t>(row) * pitch,
				static_cast<const uint8_t*>(pSubresourceData[i].pData) + static_cast<size_t>(row) * srcPitch,
				(min)(pitch, srcPitch));
		commandList->CopyBufferToImage(this, subresource, uploader, offset, width, height);
		offset += static_cast<uint64_t>(pitch) * height;
	}

	numBarriers = 0;
	for (auto i = 0u; i < numSubresources; ++i)
		numBarriers = SetBarrier(barriers.data(), dstState, numBarriers, firstSubresource + i,
			BarrierFlag::NONE, ResourceState::AUTO, threadIdx);
	if (numBarriers) pCommandList->Barrier(numBarriers, barriers.data());

	return true;
}

bool Texture_VK::Upload(CommandList* pCommandList, Resource* pUploader, const void* pData,
	uint8_t byteStride, ResourceState dstState, uint32_t threadIdx)
{
	SubresourceData data;
	data.pData = pData;
	data.RowPitch = byteStride * static_cast<intptr_t>(m_width);
	data.SlicePitch = data.RowPitch * m_height;

	return Upload(pCommandList, pUploader, &data, 1, dstState, 0, threadIdx);
}

bool Texture_VK::ReadBack(CommandList* pCommandList, Buffer* pReadBuffer, uint32_t* pRowPitches,
	uint32_t numSubresources, uint32_t firstSubresource, size_t offset, ResourceState dstState, uint32_t threadIdx)
{
	assert(pReadBuffer);
	assert(m_sampleCount <= 1 && "Texture::ReadBack does not support multisampled images on Vulkan");
	const auto texelBytes = CopyTexelBytes(m_format);
	assert(texelBytes && (offset % texelBytes) == 0);
	const auto block = IsCompressedFormat(m_format) ? 4u : 1u;
	const auto mips = (max)(m_mipLevels, 1u);
	const auto layers = m_is3D ? 1u : (max)(m_depthOrArray, 1u);

	struct Footprint
	{
		uint64_t Offset;
		uint32_t RowPitch;
		uint32_t RowLength;
		uint32_t ImageHeight;
		uint32_t Mip;
		uint32_t Layer;
	};
	vector<Footprint> footprints(numSubresources);
	auto cursor = static_cast<uint64_t>(offset);
	const auto placement = static_cast<uint64_t>(AlignMultiple(512, 512, texelBytes));
	for (auto i = 0u; i < numSubresources; ++i)
	{
		const auto subresource = firstSubresource + i;
		assert(threadIdx < m_states.size() && subresource < m_states[threadIdx].size() &&
			"Texture::ReadBack subresource is out of range on Vulkan");
		const auto mip = subresource % mips;
		const auto layer = m_is3D ? 0u : (subresource / mips) % layers;
		const auto width = static_cast<uint32_t>((max)(m_width >> mip, 1ull));
		const auto height = (max)(m_height >> mip, 1u);
		const auto depth = m_is3D ? (max)(m_depthOrArray >> mip, 1u) : 1u;
		const auto pitch = AlignMultiple(RowPitch(m_format, width), 256, texelBytes);
		if (i) cursor = XUSG_DIV_UP(cursor, placement) * placement;

		footprints[i] = {};
		footprints[i].Offset = cursor;
		footprints[i].RowPitch = pitch;
		footprints[i].RowLength = pitch / texelBytes * block;
		footprints[i].ImageHeight = XUSG_DIV_UP(height, block) * block;
		footprints[i].Mip = mip;
		footprints[i].Layer = layer;
		cursor += static_cast<uint64_t>(pitch) * XUSG_DIV_UP(height, block) * depth;
	}

	if (!pReadBuffer->GetHandle())
		XUSG_N_RETURN(pReadBuffer->Create(pCommandList->GetDevice(), static_cast<size_t>(cursor),
			ResourceFlag::DENY_SHADER_RESOURCE, MemoryType::READBACK, 0, nullptr, 0, nullptr, MemoryFlag::NONE,
			(m_name + L".ReadResource").c_str()), false);

	vector<ResourceState> dstStates(numSubresources);
	vector<ResourceBarrier> barriers;
	barriers.reserve(numSubresources);
	for (auto i = 0u; i < numSubresources; ++i)
	{
		const auto subresource = firstSubresource + i;
		const auto srcState = m_states[threadIdx][subresource];
		dstStates[i] = dstState == ResourceState::COMMON ? srcState : dstState;
		if ((srcState & ResourceState::COPY_SOURCE) == ResourceState::COPY_SOURCE) continue;

		Transition(ResourceState::COPY_SOURCE, subresource, BarrierFlag::NONE, srcState, threadIdx);
		barriers.push_back({ this, srcState, ResourceState::COPY_SOURCE, subresource });
	}
	if (!barriers.empty()) pCommandList->Barrier(static_cast<uint32_t>(barriers.size()), barriers.data());

	const auto commandList = dynamic_cast<CommandList_VK*>(pCommandList);
	assert(commandList);
	const auto aspect = IsDepthStencilFormat(m_format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	for (auto i = 0u; i < numSubresources; ++i)
	{
		commandList->CopyImageToBuffer(this, footprints[i].Mip, footprints[i].Layer, pReadBuffer, footprints[i].Offset,
			footprints[i].RowLength, footprints[i].ImageHeight, aspect);
		if (pRowPitches) pRowPitches[i] = footprints[i].RowPitch;
	}

	barriers.clear();
	for (auto i = 0u; i < numSubresources; ++i)
	{
		if (dstStates[i] == ResourceState::COPY_SOURCE) continue;

		const auto subresource = firstSubresource + i;
		Transition(dstStates[i], subresource, BarrierFlag::NONE, ResourceState::COPY_SOURCE, threadIdx);
		barriers.push_back({ this, ResourceState::COPY_SOURCE, dstStates[i], subresource });
	}
	if (!barriers.empty()) pCommandList->Barrier(static_cast<uint32_t>(barriers.size()), barriers.data());

	return true;
}

Descriptor Texture_VK::CreateSRV(uint16_t arraySize, uint16_t firstArraySlice, Format format, uint8_t numMips,
	uint8_t mostDetailedMip, bool multisamples, bool isCubeMap, uint16_t srvComponentMapping, uint8_t plane)
{
	(void)multisamples;
	auto view = storeView(ResourceViewType::SRV, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, true, false);
	const auto viewType = isCubeMap ? (arraySize > 6 ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE) :
		arraySize > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : m_height > 1 ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_1D;
	const auto viewFormat = format == Format::UNKNOWN ? m_format : format;
	fillImageView(*view, viewType, GetVKFormat(viewFormat), mostDetailedMip, numMips,
		firstArraySlice, arraySize, ImageViewAspect(viewFormat, plane),
		srvComponentMapping, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);

	return view.get();
}

Descriptor Texture_VK::CreateUAV(uint16_t arraySize, uint16_t firstArraySlice, Format format, uint8_t mipLevel, uint8_t plane)
{
	(void)plane;
	auto view = storeView(ResourceViewType::UAV, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, true, false);
	const auto viewType = arraySize > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : m_height > 1 ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_1D;
	fillImageView(*view, viewType, GetVKFormat(format == Format::UNKNOWN ? m_format : format), mipLevel, 1,
		firstArraySlice, arraySize, VK_IMAGE_ASPECT_COLOR_BIT, XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
		VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

	return view.get();
}

uint32_t Texture_VK::SetBarrier(ResourceBarrier* pBarriers, uint8_t mipLevel, ResourceState dstState,
	uint32_t numBarriers, uint16_t arraySlice, BarrierFlag flags, ResourceState srcState, uint8_t plane, uint32_t threadIdx)
{
	return SetBarrier(pBarriers, dstState, numBarriers, CalculateSubresource(mipLevel, arraySlice, plane), flags, srcState, threadIdx);
}

void Texture_VK::Blit(const CommandList* pCommandList, uint32_t groupSizeX, uint32_t groupSizeY,
	uint32_t groupSizeZ, const DescriptorTable& uavSrvTable, uint32_t uavSrvSlot, uint8_t mipLevel,
	const DescriptorTable& srvTable, uint32_t srvSlot, const DescriptorTable& samplerTable,
	uint32_t samplerSlot, const Pipeline& pipeline)
{
	if (uavSrvTable) pCommandList->SetComputeDescriptorTable(uavSrvSlot, uavSrvTable);
	if (srvTable) pCommandList->SetComputeDescriptorTable(srvSlot, srvTable);
	if (samplerTable) pCommandList->SetComputeDescriptorTable(samplerSlot, samplerTable);
	if (pipeline) pCommandList->SetPipelineState(pipeline);

	const auto width = (max)(static_cast<uint32_t>(m_width >> mipLevel), 1u);
	const auto height = (max)(m_height >> mipLevel, 1u);
	pCommandList->Dispatch(XUSG_DIV_UP(width, groupSizeX), XUSG_DIV_UP(height, groupSizeY),
		XUSG_DIV_UP(m_depthOrArray, groupSizeZ));
}

uint32_t Texture_VK::Blit(CommandList* pCommandList, ResourceBarrier* pBarriers, uint32_t groupSizeX,
	uint32_t groupSizeY, uint32_t groupSizeZ, uint8_t mipLevel, int8_t srcMipLevel, ResourceState srcState,
	const DescriptorTable& uavSrvTable, uint32_t uavSrvSlot, uint32_t numBarriers, const DescriptorTable& srvTable,
	uint32_t srvSlot, uint16_t baseSlice, uint16_t numSlices, uint32_t threadIdx)
{
	const auto prevBarriers = numBarriers;
	if (!numSlices) numSlices = GetArraySize() - baseSlice;
	if (!mipLevel && srcMipLevel <= mipLevel)
		numBarriers = SetBarrier(pBarriers, ResourceState::UNORDERED_ACCESS, numBarriers,
			XUSG_BARRIER_ALL_SUBRESOURCES, BarrierFlag::NONE, ResourceState::AUTO, threadIdx);
	else for (uint16_t i = 0; i < numSlices; ++i)
	{
		const uint16_t j = baseSlice + i;
		const auto subresource = CalculateSubresource(mipLevel, j);
		if (m_states[threadIdx][subresource] != ResourceState::UNORDERED_ACCESS)
		{
			pBarriers[numBarriers++] = { this, m_states[threadIdx][subresource], ResourceState::UNORDERED_ACCESS, subresource };
			m_states[threadIdx][subresource] = ResourceState::UNORDERED_ACCESS;
		}
		numBarriers = SetBarrier(pBarriers, srcMipLevel, srcState, numBarriers, j, BarrierFlag::NONE, ResourceState::AUTO, 0, threadIdx);
	}

	if (numBarriers > prevBarriers)
	{
		pCommandList->Barrier(numBarriers, pBarriers);
		numBarriers = 0;
	}
	Blit(pCommandList, groupSizeX, groupSizeY, groupSizeZ, uavSrvTable, uavSrvSlot, mipLevel, srvTable, srvSlot);

	return numBarriers;
}

uint32_t Texture_VK::GenerateMips(CommandList* pCommandList, ResourceBarrier* pBarriers, uint32_t groupSizeX,
	uint32_t groupSizeY, uint32_t groupSizeZ, ResourceState dstState, const PipelineLayout& pipelineLayout,
	const Pipeline& pipeline, const DescriptorTable* pUavSrvTables, uint32_t uavSrvSlot, const DescriptorTable& samplerTable,
	uint32_t samplerSlot, uint32_t numBarriers, const DescriptorTable* pSrvTables, uint32_t srvSlot, uint8_t baseMip,
	uint8_t numMips, uint16_t baseSlice, uint16_t numSlices, uint32_t threadIdx)
{
	if (pipelineLayout) pCommandList->SetComputePipelineLayout(pipelineLayout);
	if (samplerTable) pCommandList->SetComputeDescriptorTable(samplerSlot, samplerTable);
	if (pipeline) pCommandList->SetPipelineState(pipeline);
	if (!(numMips || baseMip || numSlices || baseSlice))
		numBarriers = SetBarrier(pBarriers, ResourceState::UNORDERED_ACCESS, numBarriers,
			XUSG_BARRIER_ALL_SUBRESOURCES, BarrierFlag::NONE, ResourceState::AUTO, threadIdx);
	if (!numSlices) numSlices = GetArraySize() - baseSlice;
	if (!numMips) numMips = GetNumMips() - baseMip;

	for (uint8_t i = 0; i < numMips; ++i)
	{
		const uint8_t j = baseMip + i;
		if (j) for (uint16_t k = 0; k < numSlices; ++k)
		{
			const uint16_t n = baseSlice + k;
			auto subresource = CalculateSubresource(j, n);
			if (m_states[threadIdx][subresource] != ResourceState::UNORDERED_ACCESS)
			{
				pBarriers[numBarriers++] = { this, m_states[threadIdx][subresource], ResourceState::UNORDERED_ACCESS, subresource };
				m_states[threadIdx][subresource] = ResourceState::UNORDERED_ACCESS;
			}
			subresource = CalculateSubresource(j - 1, n);
			if ((m_states[threadIdx][subresource] & dstState) != dstState)
				numBarriers = SetBarrier(pBarriers, dstState, numBarriers, subresource, BarrierFlag::NONE, ResourceState::AUTO, threadIdx);
		}
		pCommandList->Barrier(numBarriers, pBarriers);
		numBarriers = 0;
		Blit(pCommandList, groupSizeX, groupSizeY, groupSizeZ, pUavSrvTables[i], uavSrvSlot, j,
			pSrvTables ? pSrvTables[i] : XUSG_NULL, srvSlot);
	}

	const auto m = baseMip + numMips - 1;
	for (uint16_t i = 0; i < numSlices; ++i)
		numBarriers = SetBarrier(pBarriers, m, dstState, numBarriers, baseSlice + i, BarrierFlag::NONE, ResourceState::AUTO, 0, threadIdx);

	return numBarriers;
}

const Descriptor& Texture_VK::GetUAV(uint8_t level, Format format) const
{
	if (format != Format::UNKNOWN && format != m_format)
	{
		const auto iter = m_castableUavs.find(format);
		assert(iter != m_castableUavs.cend());

		return iter->second[level];
	}

	return m_uavs[level];
}

const Descriptor& Texture_VK::GetSRV(uint8_t firstLevel, bool singleLevel) const
{
	if (singleLevel) return m_singleLevelSrvs[firstLevel];

	return ShaderResource_VK::GetSRV(firstLevel);
}

uint32_t Texture_VK::CalculateSubresource(uint8_t mipLevel, uint16_t arraySlice, uint8_t plane) const
{
	const auto numMips = GetNumMips();

	return mipLevel + arraySlice * numMips + plane * numMips * GetArraySize();
}

uint32_t Texture_VK::GetHeight() const
{
	return m_height;
}

uint16_t Texture_VK::GetArraySize() const
{
	return static_cast<uint16_t>(m_depthOrArray);
}

uint8_t Texture_VK::GetNumMips() const
{
	return static_cast<uint8_t>(m_mipLevels);
}

size_t Texture_VK::GetRequiredIntermediateSize(uint32_t firstSubresource, uint32_t numSubresources) const
{
	auto size = 0ull;
	for (auto i = 0u; i < numSubresources; ++i)
	{
		const auto mip = (firstSubresource + i) % (max)(m_mipLevels, 1u);
		const auto width = static_cast<uint32_t>((max)(m_width >> mip, 1ull));
		const auto height = (max)(m_height >> mip, 1u);
		size += static_cast<uint64_t>(RowPitch(m_format, width)) * height;
	}

	return static_cast<size_t>(size);
}

size_t Texture_VK::GetTiledResourceTileSize() const
{
	return 65536;
}

void* Texture_VK::Map(uint32_t subresource, uintptr_t readBegin, uintptr_t readEnd)
{
	(void)subresource;
	(void)readBegin;
	(void)readEnd;

	return Map(nullptr, subresource);
}

void* Texture_VK::Map(const Range* pReadRange, uint32_t subresource)
{
	(void)pReadRange;
	(void)subresource;
	if (!m_pDataBegin && m_memory) vkMapMemory(m_device, m_memory, 0, m_byteSize, 0, &m_pDataBegin);

	return m_pDataBegin;
}

void Texture_VK::GetAllocationInfo(uint64_t& byteSize, uint64_t& alignment, const Device* pDevice,
	uint32_t width, uint32_t height, Format format, uint16_t arraySize, ResourceFlag resourceFlags,
	uint8_t numMips, uint8_t sampleCount, TextureLayout textureLayout, uint64_t requiredAlignment)
{
	(void)textureLayout;
	const auto device = dynamic_cast<const Device_VK*>(pDevice);
	assert(device);
	VkImageCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	info.imageType = height > 1 ? VK_IMAGE_TYPE_2D : VK_IMAGE_TYPE_1D;
	info.format = GetVKFormat(format);
	info.extent = { width, (max)(height, 1u), 1 };
	info.mipLevels = numMips ? numMips : 1;
	info.arrayLayers = arraySize ? arraySize : 1;
	info.samples = static_cast<VkSampleCountFlagBits>(sampleCount ? sampleCount : 1);
	info.tiling = VK_IMAGE_TILING_OPTIMAL;
	info.usage = GetVKImageUsage(resourceFlags);
	VkImage image = VK_NULL_HANDLE;
	if (vkCreateImage(device->GetDevice(), &info, nullptr, &image) < 0)
	{
		byteSize = 0;
		alignment = requiredAlignment;

		return;
	}
	VkMemoryRequirements requirements = {};
	vkGetImageMemoryRequirements(device->GetDevice(), image, &requirements);
	vkDestroyImage(device->GetDevice(), image, nullptr);
	byteSize = requirements.size;
	alignment = (max)(requirements.alignment, requiredAlignment);
}

bool Texture_VK::createSRVs(uint16_t arraySize, Format format, uint8_t numMips, bool multisamples, bool isCubeMap, uint16_t srvComponentMapping)
{
	m_srvs.resize(numMips);
	for (auto i = 0u; i < numMips; ++i)
		m_srvs[i] = CreateSRV(arraySize, 0, format, static_cast<uint8_t>(numMips - i), static_cast<uint8_t>(i),
			multisamples, isCubeMap, srvComponentMapping);

	return true;
}

bool Texture_VK::createSingleLevelSRVs(uint16_t arraySize, uint8_t numMips, Format format, bool multisamples,
	bool isCubeMap, uint16_t srvComponentMapping)
{
	m_singleLevelSrvs.resize(numMips);
	for (auto i = 0u; i < numMips; ++i)
		m_singleLevelSrvs[i] = CreateSRV(arraySize, 0, format, 1, static_cast<uint8_t>(i), multisamples, isCubeMap, srvComponentMapping);

	return true;
}

bool Texture_VK::createUAVs(uint16_t arraySize, Format format, uint8_t numMips)
{
	m_uavs.resize(numMips);
	for (auto i = 0u; i < numMips; ++i)
		m_uavs[i] = CreateUAV(arraySize, 0, format, static_cast<uint8_t>(i));

	return true;
}

bool Texture_VK::createCastableUAVs(uint16_t arraySize, Format format, uint8_t numMips)
{
	auto& uavs = m_castableUavs[format];
	uavs.resize(numMips);
	for (auto i = 0u; i < numMips; ++i) uavs[i] = CreateUAV(arraySize, 0, format, static_cast<uint8_t>(i));

	return true;
}

void Texture_VK::fillImageView(ResourceView_VK& view, VkImageViewType viewType, VkFormat format, uint8_t baseMip,
	uint8_t mipCount, uint16_t baseLayer, uint16_t layerCount, VkImageAspectFlags aspect, uint16_t componentMapping,
	VkImageLayout layout, VkDescriptorType type)
{
	view.ViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view.ViewInfo.image = m_image;
	view.ViewInfo.viewType = viewType;
	view.ViewInfo.format = format;
	view.ViewInfo.components = ComponentMapping(componentMapping);
	view.ViewInfo.subresourceRange.aspectMask = aspect;
	view.ViewInfo.subresourceRange.baseMipLevel = baseMip;
	view.ViewInfo.subresourceRange.levelCount = mipCount ? mipCount : 1;
	view.ViewInfo.subresourceRange.baseArrayLayer = baseLayer;
	view.ViewInfo.subresourceRange.layerCount = layerCount ? layerCount : 1;
	view.Layout = layout;
	view.VkType = type;
	view.IsImage = true;
	if (view.Type == ResourceViewType::RTV || view.Type == ResourceViewType::DSV)
		vkCreateImageView(m_device, &view.ViewInfo, nullptr, &view.ImageView);
}

//--------------------------------------------------------------------------------------

RenderTarget_VK::RenderTarget_VK()
{
}

RenderTarget_VK::~RenderTarget_VK()
{
}

bool RenderTarget_VK::Create(const Device* pDevice, uint32_t width, uint32_t height, Format format, uint16_t arraySize,
	ResourceFlag resourceFlags, uint8_t numMips, uint8_t sampleCount, const float* pClearColor, bool isCubeMap,
	MemoryFlag memoryFlags, const wchar_t* name, uint16_t srvComponentMapping, TextureLayout textureLayout,
	uint8_t numUavFormats, const Format* uavFormats, uint32_t maxThreads)
{
	(void)pClearColor;
	resourceFlags = resourceFlags | ResourceFlag::ALLOW_RENDER_TARGET;
	XUSG_N_RETURN(Texture_VK::Create(pDevice, width, height, format, arraySize, resourceFlags, numMips, sampleCount,
		isCubeMap, memoryFlags, name, srvComponentMapping, textureLayout, numUavFormats, uavFormats, maxThreads), false);
	m_rtvs.resize(numMips);
	for (auto mip = 0u; mip < numMips; ++mip)
	{
		m_rtvs[mip].resize(arraySize);
		for (auto slice = 0u; slice < arraySize; ++slice)
			m_rtvs[mip][slice] = CreateRTV(1, static_cast<uint16_t>(slice), format, static_cast<uint8_t>(mip), sampleCount > 1);
	}

	return true;
}

bool RenderTarget_VK::Create(const Device* pDevice, const Heap* pHeap, uint64_t heapOffset, uint32_t width, uint32_t height,
	Format format, uint16_t arraySize, ResourceFlag resourceFlags, uint8_t numMips, uint8_t sampleCount, const float* pClearColor,
	bool isCubeMap, const wchar_t* name, uint16_t srvComponentMapping, TextureLayout textureLayout,
	uint8_t numUavFormats, const Format* uavFormats, uint32_t maxThreads)
{
	(void)pClearColor;
	resourceFlags = resourceFlags | ResourceFlag::ALLOW_RENDER_TARGET;
	XUSG_N_RETURN(Texture_VK::Create(pDevice, pHeap, heapOffset, width, height, format, arraySize, resourceFlags, numMips,
		sampleCount, isCubeMap, name, srvComponentMapping, textureLayout, numUavFormats, uavFormats, maxThreads), false);

	return true;
}

bool RenderTarget_VK::CreateArray(const Device* pDevice, uint32_t width, uint32_t height, uint16_t arraySize, Format format,
	ResourceFlag resourceFlags, uint8_t numMips, uint8_t sampleCount, const float* pClearColor, bool isCubeMap,
	MemoryFlag memoryFlags, const wchar_t* name, uint16_t srvComponentMapping, TextureLayout textureLayout,
	uint8_t numUavFormats, const Format* uavFormats, uint32_t maxThreads)
{
	return Create(pDevice, width, height, format, arraySize, resourceFlags, numMips, sampleCount, pClearColor, isCubeMap,
		memoryFlags, name, srvComponentMapping, textureLayout, numUavFormats, uavFormats, maxThreads);
}

bool RenderTarget_VK::CreateArray(const Device* pDevice, const Heap* pHeap, uint64_t heapOffset, uint32_t width, uint32_t height,
	uint16_t arraySize, Format format, ResourceFlag resourceFlags, uint8_t numMips, uint8_t sampleCount, const float* pClearColor,
	bool isCubeMap, const wchar_t* name, uint16_t srvComponentMapping, TextureLayout textureLayout,
	uint8_t numUavFormats, const Format* uavFormats, uint32_t maxThreads)
{
	return Create(pDevice, pHeap, heapOffset, width, height, format, arraySize, resourceFlags, numMips, sampleCount,
		pClearColor, isCubeMap, name, srvComponentMapping, textureLayout, numUavFormats, uavFormats, maxThreads);
}

bool RenderTarget_VK::CreateFromSwapChain(const Device* pDevice, const SwapChain* pSwapChain, uint32_t bufferIndex, uint32_t maxThreads)
{
	const auto swapChain = dynamic_cast<const SwapChain_VK*>(pSwapChain);
	assert(swapChain);
	XUSG_N_RETURN(initialize(pDevice, maxThreads), false);
	WrapImage(swapChain->GetHandle() ? static_cast<VkDevice>(pDevice->GetHandle()) : VK_NULL_HANDLE,
		swapChain->GetImage(static_cast<uint8_t>(bufferIndex)), swapChain->GetVkFormat(), swapChain->GetWidth(), swapChain->GetHeight());
	m_device = static_cast<VkDevice>(pDevice->GetHandle());
	m_format = GetFormatFromVK(m_vkFormat);
	m_rtvs.resize(1);
	m_rtvs[0].resize(1);
	m_rtvs[0][0] = CreateRTV(1, 0, m_format, 0, false);

	return true;
}

bool RenderTarget_VK::CreateResource(const Device* pDevice, uint32_t width, uint32_t height, Format format, uint16_t arraySize,
	ResourceFlag resourceFlags, uint8_t numMips, uint8_t sampleCount, MemoryFlag memoryFlags, ResourceState initialResourceState,
	const float* pClearColor, TextureLayout textureLayout, uint8_t numCastableFormats, const Format* pCastableFormats, uint32_t maxThreads)
{
	(void)pClearColor;

	return Texture_VK::CreateResource(pDevice, width, height, format, arraySize, resourceFlags | ResourceFlag::ALLOW_RENDER_TARGET,
		numMips, sampleCount, memoryFlags, initialResourceState, textureLayout, numCastableFormats, pCastableFormats, maxThreads);
}

bool RenderTarget_VK::CreateResource(const Device* pDevice, const Heap* pHeap, uint64_t heapOffset, uint32_t width,
	uint32_t height, Format format, uint16_t arraySize, ResourceFlag resourceFlags, uint8_t numMips, uint8_t sampleCount,
	ResourceState initialResourceState, const float* pClearColor, TextureLayout textureLayout, uint8_t numCastableFormats,
	const Format* pCastableFormats, uint32_t maxThreads)
{
	(void)pClearColor;

	return Texture_VK::CreateResource(pDevice, pHeap, heapOffset, width, height, format, arraySize,
		resourceFlags | ResourceFlag::ALLOW_RENDER_TARGET, numMips, sampleCount, initialResourceState, textureLayout,
		numCastableFormats, pCastableFormats, maxThreads);
}

Descriptor RenderTarget_VK::CreateRTV(uint16_t arraySize, uint16_t firstArraySlice, Format format, uint8_t mipLevel, bool multisamples)
{
	(void)multisamples;
	auto view = storeView(ResourceViewType::RTV, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, true, false);
	const auto viewType = arraySize > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
	fillImageView(*view, viewType, GetVKFormat(format == Format::UNKNOWN ? m_format : format), mipLevel, 1,
		firstArraySlice, arraySize, VK_IMAGE_ASPECT_COLOR_BIT, XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);

	return view.get();
}

void RenderTarget_VK::Blit(const CommandList* pCommandList, const DescriptorTable& srcSrvTable, uint32_t srcSlot,
	uint8_t mipLevel, uint16_t baseSlice, uint16_t numSlices, const DescriptorTable& samplerTable, uint32_t samplerSlot,
	const Pipeline& pipeline, uint32_t offsetForSliceId, uint32_t cbSlot)
{
	if (srcSrvTable) pCommandList->SetGraphicsDescriptorTable(srcSlot, srcSrvTable);
	if (samplerTable) pCommandList->SetGraphicsDescriptorTable(samplerSlot, samplerTable);
	if (pipeline) pCommandList->SetPipelineState(pipeline);

	const auto width = (max)(static_cast<uint32_t>(m_width >> mipLevel), 1u);
	const auto height = (max)(m_height >> mipLevel, 1u);
	const Viewport viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
	const RectRange rect(0, 0, width, height);
	pCommandList->RSSetViewports(1, &viewport);
	pCommandList->RSSetScissorRects(1, &rect);
	if (numSlices == 0) numSlices = static_cast<uint16_t>(m_depthOrArray - baseSlice);
	pCommandList->IASetPrimitiveTopology(PrimitiveTopology::TRIANGLELIST);
	if (numSlices == 1)
	{
		pCommandList->OMSetRenderTargets(1, &GetRTV(baseSlice, mipLevel));
		pCommandList->Draw(3, 1, 0, 0);
	}
	else for (uint16_t i = 0; i < numSlices; ++i)
	{
		pCommandList->OMSetRenderTargets(1, &GetRTV(baseSlice + i, mipLevel));
		pCommandList->SetGraphics32BitConstant(cbSlot, i, offsetForSliceId);
		pCommandList->Draw(3, 1, 0, 0);
	}
}

uint32_t RenderTarget_VK::Blit(CommandList* pCommandList, ResourceBarrier* pBarriers, uint8_t mipLevel, int8_t srcMipLevel,
	ResourceState srcState, const DescriptorTable& srcSrvTable, uint32_t srcSlot, uint32_t numBarriers, uint16_t baseSlice,
	uint16_t numSlices, uint32_t offsetForSliceId, uint32_t cbSlot, uint32_t threadIdx)
{
	const auto prevBarriers = numBarriers;
	if (!numSlices) numSlices = static_cast<uint16_t>(m_depthOrArray - baseSlice);
	if (!mipLevel && srcMipLevel <= mipLevel)
		numBarriers = SetBarrier(pBarriers, ResourceState::RENDER_TARGET, numBarriers, XUSG_BARRIER_ALL_SUBRESOURCES,
			BarrierFlag::NONE, ResourceState::AUTO, threadIdx);
	else for (uint16_t i = 0; i < numSlices; ++i)
	{
		const auto j = baseSlice + i;
		numBarriers = SetBarrier(pBarriers, mipLevel, ResourceState::RENDER_TARGET, numBarriers, j, BarrierFlag::NONE, ResourceState::AUTO, 0, threadIdx);
		numBarriers = SetBarrier(pBarriers, srcMipLevel, srcState, numBarriers, j, BarrierFlag::NONE, ResourceState::AUTO, 0, threadIdx);
	}
	if (numBarriers > prevBarriers)
	{
		pCommandList->Barrier(numBarriers, pBarriers);
		numBarriers = 0;
	}
	Blit(pCommandList, srcSrvTable, srcSlot, mipLevel, baseSlice, numSlices, XUSG_NULL, 1, nullptr, offsetForSliceId, cbSlot);

	return numBarriers;
}

uint32_t RenderTarget_VK::GenerateMips(CommandList* pCommandList, ResourceBarrier* pBarriers, ResourceState dstState,
	const PipelineLayout& pipelineLayout, const Pipeline& pipeline, const DescriptorTable* pSrcSrvTables, uint32_t srcSlot,
	const DescriptorTable& samplerTable, uint32_t samplerSlot, uint32_t numBarriers, uint8_t baseMip, uint8_t numMips,
	uint16_t baseSlice, uint16_t numSlices, uint32_t offsetForSliceId, uint32_t cbSlot, uint32_t threadIdx)
{
	if (pipelineLayout) pCommandList->SetGraphicsPipelineLayout(pipelineLayout);
	if (samplerTable) pCommandList->SetGraphicsDescriptorTable(samplerSlot, samplerTable);
	if (pipeline) pCommandList->SetPipelineState(pipeline);
	if (!numSlices) numSlices = static_cast<uint16_t>(m_depthOrArray - baseSlice);
	if (!numMips) numMips = static_cast<uint8_t>(m_mipLevels - baseMip);
	for (uint8_t i = 0; i < numMips; ++i)
	{
		const uint8_t j = baseMip + i;
		for (uint16_t k = 0; k < numSlices; ++k)
			numBarriers = SetBarrier(pBarriers, j, ResourceState::RENDER_TARGET, numBarriers, baseSlice + k,
				BarrierFlag::NONE, ResourceState::AUTO, 0, threadIdx);
		if (numBarriers)
		{
			pCommandList->Barrier(numBarriers, pBarriers);
			numBarriers = 0;
		}
		Blit(pCommandList, pSrcSrvTables ? pSrcSrvTables[i] : XUSG_NULL, srcSlot, j, baseSlice, numSlices,
			XUSG_NULL, samplerSlot, nullptr, offsetForSliceId, cbSlot);
	}
	for (uint16_t i = 0; i < numSlices; ++i)
		numBarriers = SetBarrier(pBarriers, static_cast<uint8_t>(baseMip + numMips - 1), dstState, numBarriers,
			baseSlice + i, BarrierFlag::NONE, ResourceState::AUTO, 0, threadIdx);

	return numBarriers;
}

const Descriptor& RenderTarget_VK::GetRTV(uint16_t slice, uint8_t mipLevel) const
{
	return m_rtvs[mipLevel][slice];
}

void RenderTarget_VK::GetAllocationInfo(uint64_t& byteSize, uint64_t& alignment, const Device* pDevice, uint32_t width,
	uint32_t height, Format format, uint16_t arraySize, ResourceFlag resourceFlags, uint8_t numMips, uint8_t sampleCount,
	TextureLayout textureLayout, uint64_t requiredAlignment)
{
	Texture_VK::GetAllocationInfo(byteSize, alignment, pDevice, width, height, format, arraySize,
		resourceFlags | ResourceFlag::ALLOW_RENDER_TARGET, numMips, sampleCount, textureLayout, requiredAlignment);
}

//--------------------------------------------------------------------------------------

DepthStencil_VK::DepthStencil_VK()
{
	m_stencilSrv = nullptr;
}

DepthStencil_VK::~DepthStencil_VK()
{
}

bool DepthStencil_VK::Create(const Device* pDevice, uint32_t width, uint32_t height, Format format, ResourceFlag resourceFlags,
	uint16_t arraySize, uint8_t numMips, uint8_t sampleCount, float clearDepth, uint8_t clearStencil, bool isCubeMap,
	MemoryFlag memoryFlags, const wchar_t* name, uint16_t srvComponentMapping, uint16_t stencilSrvComponentMapping,
	TextureLayout textureLayout, uint32_t maxThreads)
{
	(void)clearDepth;
	(void)clearStencil;
	mapFormat(format);
	resourceFlags = resourceFlags | ResourceFlag::ALLOW_DEPTH_STENCIL;
	XUSG_N_RETURN(Texture_VK::Create(pDevice, width, height, format, arraySize, resourceFlags, numMips, sampleCount,
		isCubeMap, memoryFlags, name, srvComponentMapping, textureLayout, 0, nullptr, maxThreads), false);
	m_dsvs.resize(numMips);
	m_readOnlyDsvs.resize(numMips);
	for (auto mip = 0u; mip < numMips; ++mip)
	{
		m_dsvs[mip].resize(arraySize);
		m_readOnlyDsvs[mip].resize(arraySize);
		for (auto slice = 0u; slice < arraySize; ++slice)
		{
			m_dsvs[mip][slice] = CreateDSV(1, static_cast<uint16_t>(slice), format, static_cast<uint8_t>(mip), sampleCount > 1, false, false);
			m_readOnlyDsvs[mip][slice] = CreateDSV(1, static_cast<uint16_t>(slice), format, static_cast<uint8_t>(mip), sampleCount > 1, true, true);
		}
	}
	m_stencilSrv = CreateSRV(arraySize, 0, format, numMips, 0, sampleCount > 1, isCubeMap, stencilSrvComponentMapping, 1);

	return true;
}

bool DepthStencil_VK::Create(const Device* pDevice, const Heap* pHeap, uint64_t heapOffset, uint32_t width, uint32_t height,
	Format format, ResourceFlag resourceFlags, uint16_t arraySize, uint8_t numMips, uint8_t sampleCount, float clearDepth,
	uint8_t clearStencil, bool isCubeMap, const wchar_t* name, uint16_t srvComponentMapping, uint16_t stencilSrvComponentMapping,
	TextureLayout textureLayout, uint32_t maxThreads)
{
	(void)clearDepth;
	(void)clearStencil;
	(void)stencilSrvComponentMapping;
	mapFormat(format);

	return Texture_VK::Create(pDevice, pHeap, heapOffset, width, height, format, arraySize,
		resourceFlags | ResourceFlag::ALLOW_DEPTH_STENCIL, numMips, sampleCount, isCubeMap, name, srvComponentMapping,
		textureLayout, 0, nullptr, maxThreads);
}

bool DepthStencil_VK::CreateArray(const Device* pDevice, uint32_t width, uint32_t height, uint16_t arraySize, Format format,
	ResourceFlag resourceFlags, uint8_t numMips, uint8_t sampleCount, float clearDepth, uint8_t clearStencil, bool isCubeMap,
	MemoryFlag memoryFlags, const wchar_t* name, uint16_t srvComponentMapping, uint16_t stencilSrvComponentMapping,
	TextureLayout textureLayout, uint32_t maxThreads)
{
	return Create(pDevice, width, height, format, resourceFlags, arraySize, numMips, sampleCount, clearDepth, clearStencil,
		isCubeMap, memoryFlags, name, srvComponentMapping, stencilSrvComponentMapping, textureLayout, maxThreads);
}

bool DepthStencil_VK::CreateArray(const Device* pDevice, const Heap* pHeap, uint64_t heapOffset, uint32_t width, uint32_t height,
	uint16_t arraySize, Format format, ResourceFlag resourceFlags, uint8_t numMips, uint8_t sampleCount, float clearDepth,
	uint8_t clearStencil, bool isCubeMap, const wchar_t* name, uint16_t srvComponentMapping, uint16_t stencilSrvComponentMapping,
	TextureLayout textureLayout, uint32_t maxThreads)
{
	return Create(pDevice, pHeap, heapOffset, width, height, format, resourceFlags, arraySize, numMips, sampleCount,
		clearDepth, clearStencil, isCubeMap, name, srvComponentMapping, stencilSrvComponentMapping, textureLayout, maxThreads);
}

bool DepthStencil_VK::CreateResource(const Device* pDevice, uint32_t width, uint32_t height, Format format, uint16_t arraySize,
	ResourceFlag resourceFlags, uint8_t numMips, uint8_t sampleCount, MemoryFlag memoryFlags, ResourceState initialResourceState,
	float clearDepth, uint8_t clearStencil, TextureLayout textureLayout, uint32_t maxThreads)
{
	(void)clearDepth;
	(void)clearStencil;
	mapFormat(format);

	return Texture_VK::CreateResource(pDevice, width, height, format, arraySize, resourceFlags | ResourceFlag::ALLOW_DEPTH_STENCIL,
		numMips, sampleCount, memoryFlags, initialResourceState, textureLayout, 0, nullptr, maxThreads);
}

bool DepthStencil_VK::CreateResource(const Device* pDevice, const Heap* pHeap, uint64_t heapOffset, uint32_t width,
	uint32_t height, Format format, uint16_t arraySize, ResourceFlag resourceFlags, uint8_t numMips, uint8_t sampleCount,
	ResourceState initialResourceState, float clearDepth, uint8_t clearStencil, TextureLayout textureLayout, uint32_t maxThreads)
{
	(void)clearDepth;
	(void)clearStencil;
	mapFormat(format);

	return Texture_VK::CreateResource(pDevice, pHeap, heapOffset, width, height, format, arraySize,
		resourceFlags | ResourceFlag::ALLOW_DEPTH_STENCIL, numMips, sampleCount, initialResourceState, textureLayout, 0, nullptr, maxThreads);
}

Descriptor DepthStencil_VK::CreateDSV(uint16_t arraySize, uint16_t firstArraySlice, Format format, uint8_t mipLevel,
	bool multisamples, bool readOnlyDepth, bool readOnlyStencil)
{
	(void)multisamples;
	(void)readOnlyStencil;
	auto view = storeView(ResourceViewType::DSV, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, true, false);
	const auto viewType = arraySize > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
	fillImageView(*view, viewType, GetVKFormat(format == Format::UNKNOWN ? m_format : format), mipLevel, 1, firstArraySlice, arraySize,
		GetVKImageAspect(format == Format::UNKNOWN ? m_format : format), XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
		readOnlyDepth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);

	return view.get();
}

const Descriptor& DepthStencil_VK::GetDSV(uint16_t slice, uint8_t mipLevel, bool readOnly) const
{
	return readOnly ? m_readOnlyDsvs[mipLevel][slice] : m_dsvs[mipLevel][slice];
}

const Descriptor& DepthStencil_VK::GetSRV(uint8_t firstLevel, bool singleLevel, bool stencil) const
{
	if (stencil) return m_stencilSrv;

	return Texture_VK::GetSRV(firstLevel, singleLevel);
}

void DepthStencil_VK::GetAllocationInfo(uint64_t& byteSize, uint64_t& alignment, const Device* pDevice, uint32_t width,
	uint32_t height, Format format, uint16_t arraySize, ResourceFlag resourceFlags, uint8_t numMips, uint8_t sampleCount,
	TextureLayout textureLayout, uint64_t requiredAlignment)
{
	Texture_VK::GetAllocationInfo(byteSize, alignment, pDevice, width, height, format, arraySize,
		resourceFlags | ResourceFlag::ALLOW_DEPTH_STENCIL, numMips, sampleCount, textureLayout, requiredAlignment);
}

void DepthStencil_VK::mapFormat(Format& format)
{
	if (format == Format::UNKNOWN) format = Format::D32_FLOAT;
	m_format = format;
}

//--------------------------------------------------------------------------------------

Texture3D_VK::Texture3D_VK()
{
}

Texture3D_VK::~Texture3D_VK()
{
}

bool Texture3D_VK::Create(const Device* pDevice, uint32_t width, uint32_t height, uint16_t depth, Format format,
	ResourceFlag resourceFlags, uint8_t numMips, MemoryFlag memoryFlags, const wchar_t* name, uint16_t srvComponentMapping,
	TextureLayout textureLayout, uint8_t numUavFormats, const Format* uavFormats, uint32_t maxThreads)
{
	(void)memoryFlags;
	(void)textureLayout;
	XUSG_N_RETURN(initialize(pDevice, maxThreads), false);
	m_format = format;
	XUSG_N_RETURN(createImage(nullptr, 0, width, height, depth, format, 1, resourceFlags, numMips, 1, MemoryType::DEFAULT, true, false), false);
	setState(ResourceState::COMMON, numMips, maxThreads);
	SetName(name);
	m_srvs.resize(1);
	m_srvs[0] = CreateSRV(format, numMips, 0, srvComponentMapping);
	if ((resourceFlags & ResourceFlag::ALLOW_UNORDERED_ACCESS) != ResourceFlag::NONE)
	{
		m_uavs.resize(numMips);
		for (auto i = 0u; i < numMips; ++i) m_uavs[i] = CreateUAV(static_cast<uint16_t>((max)(depth >> i, 1)), 0, format, static_cast<uint8_t>(i));
	}
	(void)numUavFormats;
	(void)uavFormats;

	return true;
}

bool Texture3D_VK::Create(const Device* pDevice, const Heap* pHeap, uint64_t heapOffset, uint32_t width, uint32_t height,
	uint16_t depth, Format format, ResourceFlag resourceFlags, uint8_t numMips, const wchar_t* name, uint16_t srvComponentMapping,
	TextureLayout textureLayout, uint8_t numUavFormats, const Format* uavFormats, uint32_t maxThreads)
{
	(void)textureLayout;
	(void)numUavFormats;
	(void)uavFormats;
	XUSG_N_RETURN(initialize(pDevice, maxThreads), false);
	m_format = format;
	XUSG_N_RETURN(createImage(pHeap, heapOffset, width, height, depth, format, 1, resourceFlags, numMips, 1, MemoryType::DEFAULT, true, false), false);
	setState(ResourceState::COMMON, numMips, maxThreads);
	SetName(name);
	m_srvs.resize(1);
	m_srvs[0] = CreateSRV(format, numMips, 0, srvComponentMapping);

	return true;
}

bool Texture3D_VK::CreateResource(const Device* pDevice, uint32_t width, uint32_t height, uint16_t depth, Format format,
	ResourceFlag resourceFlags, uint8_t numMips, MemoryFlag memoryFlags, ResourceState initialResourceState,
	TextureLayout textureLayout, uint8_t numCastableFormats, const Format* pCastableFormats, uint32_t maxThreads)
{
	(void)memoryFlags;
	(void)textureLayout;
	XUSG_N_RETURN(initialize(pDevice, maxThreads), false);
	m_format = format;
	XUSG_N_RETURN(createImage(nullptr, 0, width, height, depth, format, 1, resourceFlags, numMips, 1,
		MemoryType::DEFAULT, true, false, numCastableFormats, pCastableFormats), false);
	setState(initialResourceState, numMips, maxThreads);

	return true;
}

bool Texture3D_VK::CreateResource(const Device* pDevice, const Heap* pHeap, uint64_t heapOffset, uint32_t width, uint32_t height,
	uint16_t depth, Format format, ResourceFlag resourceFlags, uint8_t numMips, ResourceState initialResourceState,
	TextureLayout textureLayout, uint8_t numCastableFormats, const Format* pCastableFormats, uint32_t maxThreads)
{
	(void)textureLayout;
	XUSG_N_RETURN(initialize(pDevice, maxThreads), false);
	m_format = format;
	XUSG_N_RETURN(createImage(pHeap, heapOffset, width, height, depth, format, 1, resourceFlags, numMips, 1,
		MemoryType::DEFAULT, true, false, numCastableFormats, pCastableFormats), false);
	setState(initialResourceState, numMips, maxThreads);

	return true;
}

Descriptor Texture3D_VK::CreateSRV(Format format, uint8_t numMips, uint8_t mostDetailedMip, uint16_t srvComponentMapping)
{
	auto view = storeView(ResourceViewType::SRV, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, true, false);
	fillImageView(*view, VK_IMAGE_VIEW_TYPE_3D, GetVKFormat(format == Format::UNKNOWN ? m_format : format), mostDetailedMip, numMips,
		0, 1, VK_IMAGE_ASPECT_COLOR_BIT, srvComponentMapping, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);

	return view.get();
}

Descriptor Texture3D_VK::CreateUAV(uint16_t wSize, uint16_t firstWSlice, Format format, uint8_t mipLevel)
{
	(void)firstWSlice;
	(void)wSize;
	auto view = storeView(ResourceViewType::UAV, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, true, false);
	fillImageView(*view, VK_IMAGE_VIEW_TYPE_3D, GetVKFormat(format == Format::UNKNOWN ? m_format : format), mipLevel, 1,
		0, 1, VK_IMAGE_ASPECT_COLOR_BIT, XUSG_DEFAULT_SRV_COMPONENT_MAPPING, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

	return view.get();
}

uint32_t Texture3D_VK::CalculateSubresource(uint8_t mipLevel) const
{
	return mipLevel;
}

uint16_t Texture3D_VK::GetDepth() const
{
	return static_cast<uint16_t>(m_depthOrArray);
}

void Texture3D_VK::GetAllocationInfo(uint64_t& byteSize, uint64_t& alignment, const Device* pDevice, uint32_t width,
	uint32_t height, uint16_t depth, Format format, ResourceFlag resourceFlags, uint8_t numMips,
	TextureLayout textureLayout, uint64_t requiredAlignment)
{
	(void)depth;
	Texture_VK::GetAllocationInfo(byteSize, alignment, pDevice, width, height, format, 1, resourceFlags, numMips, 1, textureLayout, requiredAlignment);
}

//--------------------------------------------------------------------------------------

Buffer_VK::Buffer_VK()
{
}

Buffer_VK::~Buffer_VK()
{
}

bool Buffer_VK::Create(const Device* pDevice, size_t byteWidth, ResourceFlag resourceFlags, MemoryType memoryType,
	uint32_t numSRVs, const uintptr_t* firstSrvElements, uint32_t numUAVs, const uintptr_t* firstUavElements,
	MemoryFlag memoryFlags, const wchar_t* name, const uintptr_t* counterByteOffsets, uint32_t maxThreads)
{
	(void)memoryFlags;
	XUSG_N_RETURN(CreateResource(pDevice, byteWidth, resourceFlags, memoryType, MemoryFlag::NONE,
		ResourceState::COMMON, 0, nullptr, maxThreads), false);
	SetName(name);
	const auto stride = 4u;
	createViews(byteWidth / stride, stride, Format::R32_TYPELESS, numSRVs, firstSrvElements,
		numUAVs, firstUavElements, counterByteOffsets, XUSG_DEFAULT_SRV_COMPONENT_MAPPING);

	return true;
}

bool Buffer_VK::Create(const Device* pDevice, const Heap* pHeap, uint64_t heapOffset, size_t byteWidth, ResourceFlag resourceFlags,
	uint32_t numSRVs, const uintptr_t* firstSrvElements, uint32_t numUAVs, const uintptr_t* firstUavElements,
	const wchar_t* name, const uintptr_t* counterByteOffsets, uint32_t maxThreads)
{
	XUSG_N_RETURN(CreateResource(pDevice, pHeap, heapOffset, byteWidth, resourceFlags, ResourceState::COMMON, 0, nullptr, maxThreads), false);
	SetName(name);
	createViews(byteWidth / 4, 4, Format::R32_TYPELESS, numSRVs, firstSrvElements, numUAVs, firstUavElements, counterByteOffsets,
		XUSG_DEFAULT_SRV_COMPONENT_MAPPING);

	return true;
}

bool Buffer_VK::CreateResource(const Device* pDevice, size_t byteWidth, ResourceFlag resourceFlags, MemoryType memoryType,
	MemoryFlag memoryFlags, ResourceState initialResourceState, uint8_t numCastableFormats, const Format* pCastableFormats, uint32_t maxThreads)
{
	(void)memoryFlags;
	(void)numCastableFormats;
	(void)pCastableFormats;
	XUSG_N_RETURN(initialize(pDevice, maxThreads), false);
	XUSG_N_RETURN(createBuffer(nullptr, 0, byteWidth, resourceFlags, memoryType), false);
	setState(initialResourceState, 1, maxThreads);
	m_format = Format::UNKNOWN;

	return true;
}

bool Buffer_VK::CreateResource(const Device* pDevice, const Heap* pHeap, uint64_t heapOffset, size_t byteWidth,
	ResourceFlag resourceFlags, ResourceState initialResourceState, uint8_t numCastableFormats, const Format* pCastableFormats, uint32_t maxThreads)
{
	(void)numCastableFormats;
	(void)pCastableFormats;
	XUSG_N_RETURN(initialize(pDevice, maxThreads), false);
	XUSG_N_RETURN(createBuffer(pHeap, heapOffset, byteWidth, resourceFlags, MemoryType::DEFAULT), false);
	setState(initialResourceState, 1, maxThreads);

	return true;
}

bool Buffer_VK::Upload(CommandList* pCommandList, Resource* pUploader, const void* pData, size_t size,
	size_t offset, ResourceState dstState, uint32_t threadIdx)
{
	const auto uploader = dynamic_cast<Resource_VK*>(pUploader);
	assert(uploader);
	XUSG_N_RETURN(uploader->CreateUploadBuffer(m_pDevice, size), false);
	memcpy(uploader->GetMappedData(), pData, size);
	ResourceBarrier barrier;
	auto numBarriers = SetBarrier(&barrier, ResourceState::COPY_DEST, 0, 0, BarrierFlag::NONE, ResourceState::AUTO, threadIdx);
	if (numBarriers) pCommandList->Barrier(numBarriers, &barrier);
	pCommandList->CopyBufferRegion(this, offset, uploader, 0, size);
	numBarriers = SetBarrier(&barrier, dstState, 0, 0, BarrierFlag::NONE, ResourceState::AUTO, threadIdx);
	if (numBarriers) pCommandList->Barrier(numBarriers, &barrier);

	return true;
}

bool Buffer_VK::Upload(CommandList* pCommandList, uint32_t descriptorIndex, Resource* pUploader, const void* pData,
	size_t size, ResourceState dstState, uint32_t threadIdx)
{
	const auto offset = descriptorIndex < m_srvByteOffsets.size() ? m_srvByteOffsets[descriptorIndex] : 0;

	return Upload(pCommandList, pUploader, pData, size, offset, dstState, threadIdx);
}

bool Buffer_VK::ReadBack(CommandList* pCommandList, Buffer* pReadBuffer, size_t size, size_t dstOffset,
	size_t srcOffset, ResourceState dstState, uint32_t threadIdx)
{
	if (!size) size = static_cast<size_t>(m_byteSize - srcOffset);
	ResourceBarrier barrier;
	auto numBarriers = SetBarrier(&barrier, ResourceState::COPY_SOURCE, 0, 0, BarrierFlag::NONE, ResourceState::AUTO, threadIdx);
	if (numBarriers) pCommandList->Barrier(numBarriers, &barrier);
	pCommandList->CopyBufferRegion(pReadBuffer, dstOffset, this, srcOffset, size);
	numBarriers = SetBarrier(&barrier, dstState, 0, 0, BarrierFlag::NONE, ResourceState::AUTO, threadIdx);
	if (numBarriers) pCommandList->Barrier(numBarriers, &barrier);

	return true;
}

Descriptor Buffer_VK::CreateSRV(uint32_t numElements, uint32_t byteStride, Format format, size_t firstElement, uint16_t srvComponentMapping)
{
	(void)srvComponentMapping;
	const auto texel = format != Format::UNKNOWN && format != Format::R32_TYPELESS;
	auto view = storeView(ResourceViewType::SRV, texel ? VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, false, texel);
	view->Address = m_address + firstElement * byteStride;
	view->Range = static_cast<VkDeviceSize>(numElements) * byteStride;
	view->Stride = byteStride;
	view->ViewInfo.format = GetVKFormat(format);
	m_format = format;

	return view.get();
}

Descriptor Buffer_VK::CreateUAV(uint32_t numElements, uint32_t byteStride, Format format, size_t firstElement, size_t counterByteOffset)
{
	(void)counterByteOffset;
	const auto texel = format != Format::UNKNOWN && format != Format::R32_TYPELESS;
	auto view = storeView(ResourceViewType::UAV, texel ? VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, false, texel);
	view->Address = m_address + firstElement * byteStride;
	view->Range = static_cast<VkDeviceSize>(numElements) * byteStride;
	view->Stride = byteStride;
	view->ViewInfo.format = GetVKFormat(format);

	return view.get();
}

const Descriptor& Buffer_VK::GetUAV(uint32_t index) const
{
	return m_uavs[index];
}

void* Buffer_VK::Map(uint32_t descriptorIndex, uintptr_t readBegin, uintptr_t readEnd)
{
	(void)readBegin;
	(void)readEnd;

	return Map(nullptr, descriptorIndex);
}

void* Buffer_VK::Map(const Range* pReadRange, uint32_t descriptorIndex)
{
	(void)pReadRange;
	if (!m_pDataBegin && m_memory) vkMapMemory(m_device, m_memory, 0, m_byteSize, 0, &m_pDataBegin);
	const auto offset = descriptorIndex < m_srvByteOffsets.size() ? m_srvByteOffsets[descriptorIndex] : 0;

	return static_cast<uint8_t*>(m_pDataBegin) + offset;
}

void Buffer_VK::SetCounter(const Resource::sptr& counter)
{
	m_counter = counter;
}

Resource::sptr Buffer_VK::GetCounter() const
{
	return m_counter;
}

size_t Buffer_VK::GetTiledResourceTileSize() const
{
	return 65536;
}

void Buffer_VK::GetAllocationInfo(uint64_t& byteSize, uint64_t& alignment, const Device* pDevice, size_t byteWidth,
	ResourceFlag resourceFlags, uint64_t requiredAlignment)
{
	(void)pDevice;
	(void)resourceFlags;
	alignment = (max)(requiredAlignment, 65536ull);
	byteSize = XUSG_DIV_UP(byteWidth, alignment) * alignment;
}

size_t Buffer_VK::AlignRawView(size_t byteSize)
{
	return XUSG_DIV_UP(byteSize, 16) * 16;
}

size_t Buffer_VK::AlignCounter(size_t byteSize)
{
	return XUSG_DIV_UP(byteSize, 4096) * 4096;
}

bool Buffer_VK::createViews(size_t numElements, uint32_t byteStride, Format format, uint32_t numSRVs,
	const uintptr_t* firstSrvElements, uint32_t numUAVs, const uintptr_t* firstUavElements,
	const uintptr_t* counterByteOffsets, uint16_t srvComponentMapping)
{
	m_srvs.resize(numSRVs);
	m_uavs.resize(numUAVs);
	m_srvByteOffsets.resize(numSRVs);
	for (auto i = 0u; i < numSRVs; ++i)
	{
		uint32_t viewElements = static_cast<uint32_t>(numElements);
		uintptr_t first = 0;
		getViewRange(viewElements, first, i, numElements, byteStride, firstSrvElements, numSRVs);
		m_srvByteOffsets[i] = first * byteStride;
		m_srvs[i] = CreateSRV(viewElements, byteStride, format, first, srvComponentMapping);
	}
	for (auto i = 0u; i < numUAVs; ++i)
	{
		uint32_t viewElements = static_cast<uint32_t>(numElements);
		uintptr_t first = 0;
		getViewRange(viewElements, first, i, numElements, byteStride, firstUavElements, numUAVs);
		const auto counter = counterByteOffsets ? counterByteOffsets[i] : 0;
		m_uavs[i] = CreateUAV(viewElements, byteStride, format, first, counter);
	}

	return true;
}

void Buffer_VK::getViewRange(uint32_t& viewElements, uintptr_t& firstElement, uint32_t i, size_t bufferElements,
	uint32_t byteStride, const uintptr_t* firstElements, uint32_t numDescriptors)
{
	(void)byteStride;
	firstElement = firstElements ? firstElements[i] : 0;
	const auto next = (firstElements && i + 1 < numDescriptors) ? firstElements[i + 1] : bufferElements;
	if (next > firstElement) viewElements = static_cast<uint32_t>(next - firstElement);
}

//--------------------------------------------------------------------------------------

StructuredBuffer_VK::StructuredBuffer_VK()
{
}

StructuredBuffer_VK::~StructuredBuffer_VK()
{
}

bool StructuredBuffer_VK::Create(const Device* pDevice, size_t numElements, uint32_t byteStride, ResourceFlag resourceFlags,
	MemoryType memoryType, uint32_t numSRVs, const uintptr_t* firstSrvElements, uint32_t numUAVs, const uintptr_t* firstUavElements,
	MemoryFlag memoryFlags, const wchar_t* name, const uintptr_t* counterByteOffsets, uint32_t maxThreads)
{
	(void)memoryFlags;
	XUSG_N_RETURN(CreateResource(pDevice, numElements * byteStride, resourceFlags, memoryType), false);
	SetName(name);
	(void)maxThreads;

	return createViews(numElements, byteStride, Format::UNKNOWN, numSRVs, firstSrvElements, numUAVs, firstUavElements,
		counterByteOffsets, XUSG_DEFAULT_SRV_COMPONENT_MAPPING);
}

bool StructuredBuffer_VK::Create(const Device* pDevice, const Heap* pHeap, uint64_t heapOffset, size_t numElements,
	uint32_t byteStride, ResourceFlag resourceFlags, uint32_t numSRVs, const uintptr_t* firstSrvElements,
	uint32_t numUAVs, const uintptr_t* firstUavElements, const wchar_t* name, const uintptr_t* counterByteOffsets, uint32_t maxThreads)
{
	XUSG_N_RETURN(CreateResource(pDevice, pHeap, heapOffset, numElements * byteStride, resourceFlags), false);
	SetName(name);
	(void)maxThreads;

	return createViews(numElements, byteStride, Format::UNKNOWN, numSRVs, firstSrvElements, numUAVs, firstUavElements,
		counterByteOffsets, XUSG_DEFAULT_SRV_COMPONENT_MAPPING);
}

//--------------------------------------------------------------------------------------

TypedBuffer_VK::TypedBuffer_VK()
{
}

TypedBuffer_VK::~TypedBuffer_VK()
{
}

bool TypedBuffer_VK::Create(const Device* pDevice, size_t numElements, uint32_t byteStride, Format format, ResourceFlag resourceFlags,
	MemoryType memoryType, uint32_t numSRVs, const uintptr_t* firstSrvElements, uint32_t numUAVs, const uintptr_t* firstUavElements,
	MemoryFlag memoryFlags, const wchar_t* name, uint16_t srvComponentMapping, const uintptr_t* counterByteOffsets,
	uint8_t numUavFormats, const Format* uavFormats, uint32_t maxThreads)
{
	(void)memoryFlags;
	(void)numUavFormats;
	(void)uavFormats;
	XUSG_N_RETURN(CreateResource(pDevice, numElements * byteStride, resourceFlags, memoryType, MemoryFlag::NONE,
		ResourceState::COMMON, 0, nullptr, maxThreads), false);
	SetName(name);
	m_format = format;

	return createViews(numElements, byteStride, format, numSRVs, firstSrvElements, numUAVs, firstUavElements,
		counterByteOffsets, srvComponentMapping);
}

bool TypedBuffer_VK::Create(const Device* pDevice, const Heap* pHeap, uint64_t heapOffset, size_t numElements, uint32_t byteStride,
	Format format, ResourceFlag resourceFlags, uint32_t numSRVs, const uintptr_t* firstSrvElements, uint32_t numUAVs,
	const uintptr_t* firstUavElements, const wchar_t* name, uint16_t srvComponentMapping, const uintptr_t* counterByteOffsets,
	uint8_t numUavFormats, const Format* uavFormats, uint32_t maxThreads)
{
	(void)numUavFormats;
	(void)uavFormats;
	(void)maxThreads;
	XUSG_N_RETURN(CreateResource(pDevice, pHeap, heapOffset, numElements * byteStride, resourceFlags), false);
	SetName(name);
	m_format = format;

	return createViews(numElements, byteStride, format, numSRVs, firstSrvElements, numUAVs, firstUavElements,
		counterByteOffsets, srvComponentMapping);
}

const Descriptor& TypedBuffer_VK::GetUAV(uint32_t index, Format format) const
{
	if (format != Format::UNKNOWN && format != m_format)
	{
		const auto iter = m_castableUavs.find(format);
		assert(iter != m_castableUavs.cend());

		return iter->second[index];
	}

	return m_uavs[index];
}

//--------------------------------------------------------------------------------------

VertexBuffer_VK::VertexBuffer_VK()
{
}

VertexBuffer_VK::~VertexBuffer_VK()
{
}

bool VertexBuffer_VK::Create(const Device* pDevice, size_t numVertices, uint32_t byteStride, ResourceFlag resourceFlags,
	MemoryType memoryType, uint32_t numVBVs, const uintptr_t* firstVertices, uint32_t numSRVs, const uintptr_t* firstSrvElements,
	uint32_t numUAVs, const uintptr_t* firstUavElements, MemoryFlag memoryFlags, const wchar_t* name,
	const uintptr_t* counterByteOffsets, uint32_t maxThreads)
{
	XUSG_N_RETURN(StructuredBuffer_VK::Create(pDevice, numVertices, byteStride, resourceFlags, memoryType, numSRVs, firstSrvElements,
		numUAVs, firstUavElements, memoryFlags, name, counterByteOffsets, maxThreads), false);
	createVBVs(numVertices, byteStride, numVBVs, firstVertices);

	return true;
}

bool VertexBuffer_VK::Create(const Device* pDevice, const Heap* pHeap, uint64_t heapOffset, size_t numVertices, uint32_t byteStride,
	ResourceFlag resourceFlags, uint32_t numVBVs, const uintptr_t* firstVertices, uint32_t numSRVs, const uintptr_t* firstSrvElements,
	uint32_t numUAVs, const uintptr_t* firstUavElements, const wchar_t* name, const uintptr_t* counterByteOffsets, uint32_t maxThreads)
{
	XUSG_N_RETURN(StructuredBuffer_VK::Create(pDevice, pHeap, heapOffset, numVertices, byteStride, resourceFlags, numSRVs,
		firstSrvElements, numUAVs, firstUavElements, name, counterByteOffsets, maxThreads), false);
	createVBVs(numVertices, byteStride, numVBVs, firstVertices);

	return true;
}

bool VertexBuffer_VK::CreateAsRaw(const Device* pDevice, size_t numVertices, uint32_t byteStride, ResourceFlag resourceFlags,
	MemoryType memoryType, uint32_t numVBVs, const uintptr_t* firstVertices, uint32_t numSRVs, const uintptr_t* firstSrvElements,
	uint32_t numUAVs, const uintptr_t* firstUavElements, MemoryFlag memoryFlags, const wchar_t* name,
	const uintptr_t* counterByteOffsets, uint32_t maxThreads)
{
	XUSG_N_RETURN(Buffer_VK::Create(pDevice, numVertices * byteStride, resourceFlags, memoryType, numSRVs, firstSrvElements,
		numUAVs, firstUavElements, memoryFlags, name, counterByteOffsets, maxThreads), false);
	createVBVs(numVertices, byteStride, numVBVs, firstVertices);

	return true;
}

bool VertexBuffer_VK::CreateAsRaw(const Device* pDevice, const Heap* pHeap, uint64_t heapOffset, size_t numVertices, uint32_t byteStride,
	ResourceFlag resourceFlags, uint32_t numVBVs, const uintptr_t* firstVertices, uint32_t numSRVs, const uintptr_t* firstSrvElements,
	uint32_t numUAVs, const uintptr_t* firstUavElements, const wchar_t* name, const uintptr_t* counterByteOffsets, uint32_t maxThreads)
{
	XUSG_N_RETURN(Buffer_VK::Create(pDevice, pHeap, heapOffset, numVertices * byteStride, resourceFlags, numSRVs, firstSrvElements,
		numUAVs, firstUavElements, name, counterByteOffsets, maxThreads), false);
	createVBVs(numVertices, byteStride, numVBVs, firstVertices);

	return true;
}

void VertexBuffer_VK::CreateVBV(VertexBufferView& vbv, uint32_t numVertices, uint32_t byteStride, size_t firstVertex)
{
	vbv.BufferLocation = GetVirtualAddress(static_cast<int>(firstVertex * byteStride));
	vbv.SizeInBytes = numVertices * byteStride;
	vbv.StrideInBytes = byteStride;
}

const VertexBufferView& VertexBuffer_VK::GetVBV(uint32_t index) const
{
	return m_vbvs[index];
}

void VertexBuffer_VK::createVBVs(size_t numVertices, uint32_t byteStride, uint32_t numVBVs, const uintptr_t* firstVertices)
{
	m_vbvs.resize(numVBVs);
	for (auto i = 0u; i < numVBVs; ++i)
	{
		const auto first = firstVertices ? firstVertices[i] : 0;
		const auto next = (firstVertices && i + 1 < numVBVs) ? firstVertices[i + 1] : numVertices;
		CreateVBV(m_vbvs[i], static_cast<uint32_t>(next - first), byteStride, first);
	}
}

//--------------------------------------------------------------------------------------

IndexBuffer_VK::IndexBuffer_VK()
{
}

IndexBuffer_VK::~IndexBuffer_VK()
{
}

bool IndexBuffer_VK::Create(const Device* pDevice, size_t byteWidth, Format format, ResourceFlag resourceFlags, MemoryType memoryType,
	uint32_t numIBVs, const uintptr_t* ibvByteOffsets, uint32_t numSRVs, const uintptr_t* firstSrvElements, uint32_t numUAVs,
	const uintptr_t* firstUavElements, MemoryFlag memoryFlags, const wchar_t* name, uint16_t srvComponentMapping,
	const uintptr_t* counterByteOffsets, uint32_t maxThreads)
{
	const auto stride = format == Format::R16_UINT ? 2u : 4u;
	XUSG_N_RETURN(TypedBuffer_VK::Create(pDevice, byteWidth / stride, stride, format, resourceFlags, memoryType, numSRVs, firstSrvElements,
		numUAVs, firstUavElements, memoryFlags, name, srvComponentMapping, counterByteOffsets, 0, nullptr, maxThreads), false);
	m_ibvs.resize(numIBVs);
	for (auto i = 0u; i < numIBVs; ++i)
		CreateIBV(m_ibvs[i], format, static_cast<uint32_t>(byteWidth), ibvByteOffsets ? ibvByteOffsets[i] : 0);

	return true;
}

bool IndexBuffer_VK::Create(const Device* pDevice, const Heap* pHeap, uint64_t heapOffset, size_t byteWidth, Format format,
	ResourceFlag resourceFlags, uint32_t numIBVs, const uintptr_t* ibvByteOffsets, uint32_t numSRVs, const uintptr_t* firstSrvElements,
	uint32_t numUAVs, const uintptr_t* firstUavElements, const wchar_t* name, uint16_t srvComponentMapping,
	const uintptr_t* counterByteOffsets, uint32_t maxThreads)
{
	const auto stride = format == Format::R16_UINT ? 2u : 4u;
	XUSG_N_RETURN(TypedBuffer_VK::Create(pDevice, pHeap, heapOffset, byteWidth / stride, stride, format, resourceFlags, numSRVs,
		firstSrvElements, numUAVs, firstUavElements, name, srvComponentMapping, counterByteOffsets, 0, nullptr, maxThreads), false);
	m_ibvs.resize(numIBVs);
	for (auto i = 0u; i < numIBVs; ++i)
		CreateIBV(m_ibvs[i], format, static_cast<uint32_t>(byteWidth), ibvByteOffsets ? ibvByteOffsets[i] : 0);

	return true;
}

void IndexBuffer_VK::CreateIBV(IndexBufferView& ibv, Format format, uint32_t byteSize, size_t byteOffset)
{
	ibv.BufferLocation = GetVirtualAddress(static_cast<int>(byteOffset));
	ibv.SizeInBytes = byteSize;
	ibv.Format = format;
}

const IndexBufferView& IndexBuffer_VK::GetIBV(uint32_t index) const
{
	return m_ibvs[index];
}
