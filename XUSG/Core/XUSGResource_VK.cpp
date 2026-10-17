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

	constexpr VkDeviceSize TileBytes = 65536;

	void StandardTileTexels(Format format, bool is3D, uint32_t sampleCount, uint32_t& width, uint32_t& height, uint32_t& depth)
	{
		const auto bits = (max)(GetVKFormatBits(format), 8u);
		const auto compressed = IsCompressedFormat(format);
		const auto block = compressed ? 4u : 1u;
		const auto bytes = compressed ? (bits <= 4 ? 8u : 16u) : bits / 8u;
		width = 128;
		height = 128;
		depth = 1;

		if (is3D)
		{
			struct Shape3D { uint32_t Bytes; uint32_t W; uint32_t H; uint32_t D; };
			const Shape3D shapes[] =
			{
				{ 1, 64, 32, 32 },
				{ 2, 32, 32, 32 },
				{ 4, 32, 32, 16 },
				{ 8, 32, 16, 16 },
				{ 16, 16, 16, 16 }
			};
			for (const auto& shape : shapes)
			{
				if (shape.Bytes != bytes) continue;

				width = shape.W * block;
				height = shape.H * block;
				depth = shape.D;

				return;
			}

			return;
		}

		struct Shape2D { uint32_t Bytes; uint32_t Samples; uint32_t W; uint32_t H; };
		const Shape2D shapes[] =
		{
			{ 1, 1, 256, 256 }, { 1, 2, 128, 128 }, { 1, 4, 128, 256 }, { 1, 8, 64, 128 }, { 1, 16, 64, 64 },
			{ 2, 1, 256, 128 }, { 2, 2, 128, 128 }, { 2, 4, 128, 64 }, { 2, 8, 64, 64 }, { 2, 16, 64, 32 },
			{ 4, 1, 128, 128 }, { 4, 2, 64, 128 }, { 4, 4, 64, 64 }, { 4, 8, 32, 64 }, { 4, 16, 32, 32 },
			{ 8, 1, 128, 64 }, { 8, 2, 64, 64 }, { 8, 4, 64, 32 }, { 8, 8, 32, 32 }, { 8, 16, 32, 16 },
			{ 16, 1, 64, 64 }, { 16, 2, 32, 64 }, { 16, 4, 32, 32 }, { 16, 8, 16, 32 }, { 16, 16, 16, 16 }
		};
		const auto samples = sampleCount ? sampleCount : 1u;
		for (const auto& shape : shapes)
		{
			if (shape.Bytes != bytes || shape.Samples != samples) continue;

			width = shape.W * block;
			height = shape.H * block;
			depth = 1;

			return;
		}
	}

	uint64_t MipByteSize(Format format, uint32_t width, uint32_t height, uint32_t depth)
	{
		const auto block = IsCompressedFormat(format) ? 4u : 1u;

		return static_cast<uint64_t>(XUSG_DIV_UP(width, block)) * XUSG_DIV_UP(height, block) * depth * CopyTexelBytes(format);
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

//--------------------------------------------------------------------------------------
// Resource heap
//--------------------------------------------------------------------------------------

Heap_VK::Heap_VK() :
	m_pDevice(nullptr),
	m_memory(VK_NULL_HANDLE),
	m_size(0),
	m_alignment(0),
	m_memoryTypeIndex(0),
	m_memoryType(MemoryType::DEFAULT)
{
}

Heap_VK::~Heap_VK()
{
	if (m_memory != VK_NULL_HANDLE) vkFreeMemory(m_pDevice->GetDevice(), m_memory, nullptr);
}

bool Heap_VK::Create(Device* pDevice, uint64_t byteSize, MemoryType memoryType,
	MemoryFlag memoryFlags, uint64_t alignment, const wchar_t* name)
{
	(void)memoryFlags;
	m_pDevice = AsVK(pDevice);
	assert(m_pDevice);
	const auto device = m_pDevice->GetDevice();
	m_size = byteSize;
	m_alignment = alignment;
	m_memoryType = memoryType;

	VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	if (memoryType == MemoryType::READBACK)
		properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
	else if (memoryType == MemoryType::UPLOAD || memoryType == MemoryType::GPU_UPLOAD)
		properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	m_memoryTypeIndex = m_pDevice->FindMemoryType(HeapMemoryTypeBits(device, memoryType), properties);
	assert(m_memoryTypeIndex != UINT32_MAX);

	VkMemoryAllocateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	info.allocationSize = byteSize;
	info.memoryTypeIndex = m_memoryTypeIndex;
	XUSG_VK_CHECK(vkAllocateMemory(device, &info, nullptr, &m_memory), cerr, false);
	m_pDevice->SetDebugName(VK_OBJECT_TYPE_DEVICE_MEMORY, (uint64_t)m_memory, name);

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

void Heap_VK::Create(uint64_t nativeHandle, Device* pDevice)
{
	m_pDevice = AsVK(pDevice);
	assert(m_pDevice);

	m_memory = (VkDeviceMemory)nativeHandle;
	m_name.clear();
}

void Heap_VK::SetName(const wchar_t* name)
{
	if (name) m_name = name;
}

uint64_t Heap_VK::GetNativeHandle() const
{
	return (uint64_t)m_memory;
}

void* Heap_VK::GetBackend()
{
	return this;
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
// Resource base
//--------------------------------------------------------------------------------------

Resource_VK::Resource_VK() :
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
	m_presentable(false),
	m_commonLayout(VK_IMAGE_LAYOUT_GENERAL),
	m_byteSize(0),
	m_width(0),
	m_height(1),
	m_depthOrArray(1),
	m_mipLevels(1),
	m_sampleCount(1),
	m_vkFormat(VK_FORMAT_UNDEFINED),
	m_pendingLayout(0),
	m_discardCount(0),
	m_sparse(false),
	m_hasSparseInfo(false),
	m_resourceFormat(Format::UNKNOWN),
	m_bufferType(BufferType::GENERIC),
	m_metadata(VK_NULL_HANDLE),
	m_sparseTypeBits(0),
	m_sparseInfo{},
	m_tilingReady(false),
	m_totalTiles(0),
	m_singleMipTail(false),
	m_packed{},
	m_tileShape{}
{
}

Resource_VK::~Resource_VK()
{
	if (!m_pDevice) return;

	const auto device = m_pDevice->GetDevice();
	for (auto& view : m_resourceViews)
		if (view && (view->Type == ResourceViewType::RTV || view->Type == ResourceViewType::DSV) &&
			view->Target.Rendering.imageView != VK_NULL_HANDLE)
			vkDestroyImageView(device, view->Target.Rendering.imageView, nullptr);

	if (m_pDataBegin) vkUnmapMemory(device, m_memory);
	if (m_ownsImage && m_image != VK_NULL_HANDLE) vkDestroyImage(device, m_image, nullptr);
	if (m_ownsBuffer && m_buffer != VK_NULL_HANDLE) vkDestroyBuffer(device, m_buffer, nullptr);
	releaseSparseMetadata();
	if (m_ownsMemory && m_memory != VK_NULL_HANDLE) vkFreeMemory(device, m_memory, nullptr);
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
	assert(m_states.empty() || (m_image == VK_NULL_HANDLE && m_buffer == VK_NULL_HANDLE) || !m_states.empty());
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
	if (m_pDataBegin && m_memory != VK_NULL_HANDLE)
	{
		vkUnmapMemory(m_pDevice->GetDevice(), m_memory);
		m_pDataBegin = nullptr;
	}
}

void Resource_VK::GetTiling(uint32_t* pTotalNumTiles, PackedMipInfo* pPackedMipInfo,
	TileShape* pStandardTileShapeForNonPackedMips, uint32_t* pNumSubresourceTilings,
	uint32_t firstSubresourceTiling, SubresourceTiling* pSubresourceTilingsForNonPackedMips) const
{
	ensureTiling();
	if (pTotalNumTiles) *pTotalNumTiles = m_totalTiles;
	if (pPackedMipInfo) *pPackedMipInfo = m_packed;
	if (pStandardTileShapeForNonPackedMips) *pStandardTileShapeForNonPackedMips = m_tileShape;

	const auto count = static_cast<uint32_t>(m_nonPacked.size());
	if (!pSubresourceTilingsForNonPackedMips)
	{
		if (pNumSubresourceTilings) *pNumSubresourceTilings = count;

		return;
	}

	if (firstSubresourceTiling >= count)
	{
		*pSubresourceTilingsForNonPackedMips = {};
		if (pNumSubresourceTilings) *pNumSubresourceTilings = 0;

		return;
	}

	const auto& item = m_subresources[m_nonPacked[firstSubresourceTiling]];
	pSubresourceTilingsForNonPackedMips->WidthInTiles = item.WidthInTiles;
	pSubresourceTilingsForNonPackedMips->HeightInTiles = static_cast<uint16_t>(item.HeightInTiles);
	pSubresourceTilingsForNonPackedMips->DepthInTiles = static_cast<uint16_t>(item.DepthInTiles);
	pSubresourceTilingsForNonPackedMips->StartTileIndexInOverallResource = item.StartTile;
	if (pNumSubresourceTilings) *pNumSubresourceTilings = 1;
}

bool Resource_VK::IsSparse() const
{
	return m_sparse;
}

bool Resource_VK::ImageTileCopy(const TiledResourceCoord& coord, const TileRegionSize& region, uint32_t index,
	VkDeviceSize bufferOffset, VkBufferImageCopy& copy) const
{
	uint32_t tile = 0;
	if (!tileIndex(coord, region, index, tile) || tile >= m_tiles.size()) return false;

	const auto& info = m_tiles[tile];
	if (info.Opaque) return false;

	copy = {};
	copy.bufferOffset = bufferOffset + TileBytes * index;
	copy.bufferRowLength = m_tileShape.WidthInTexels;
	copy.bufferImageHeight = m_tileShape.HeightInTexels;
	copy.imageSubresource.aspectMask = info.Subresource.aspectMask;
	copy.imageSubresource.mipLevel = info.Subresource.mipLevel;
	copy.imageSubresource.baseArrayLayer = info.Subresource.arrayLayer;
	copy.imageSubresource.layerCount = 1;
	copy.imageOffset = info.Offset;
	copy.imageExtent = info.Extent;

	return copy.imageExtent.width && copy.imageExtent.height && copy.imageExtent.depth;
}

bool Resource_VK::BufferTileCopy(uint32_t startTile, uint32_t numTiles, VkDeviceSize& offset, VkDeviceSize& size) const
{
	ensureTiling();
	if (!numTiles || m_tiles.empty() || startTile >= m_tiles.size()) return false;
	if (startTile + numTiles > m_tiles.size()) return false;

	const auto& first = m_tiles[startTile];
	const auto& last = m_tiles[startTile + numTiles - 1];
	offset = first.ResourceOffset;
	size = last.ResourceOffset + last.Size - first.ResourceOffset;

	return size > 0;
}

void Resource_VK::BindTileMappings(VkQueue queue, uint32_t numResourceRegions,
	const TiledResourceCoord* pResourceRegionStartCoords, const TileRegionSize* pResourceRegionSizes,
	Heap* pHeap, uint32_t numHeapRanges, const TileRangeFlag* pHeapRangeFlags,
	const uint32_t* pHeapRangeStartOffsets, const uint32_t* pHeapRangeTileCounts)
{
	ensureTiling();
	assert(m_sparse);
	if (!m_sparse || !m_totalTiles || !numHeapRanges) return;
	assert(pHeapRangeFlags);
	if (!pHeapRangeFlags) return;

	const auto pVkHeap = AsVK(pHeap);
	assert(pVkHeap);
	TiledResourceCoord whole = {};
	TileRegionSize wholeRegion = {};
	wholeRegion.NumTiles = m_totalTiles;
	if (!numResourceRegions)
	{
		numResourceRegions = 1;
		pResourceRegionStartCoords = &whole;
		pResourceRegionSizes = &wholeRegion;
	}

	auto regionTile = 0u;
	auto regionIndex = 0u;
	auto rangeTile = 0u;
	auto rangeIndex = 0u;
	auto rangeCount = ~0u;
	auto rangeOffset = 0u;
	auto rangeFlags = TileRangeFlag::NONE;
	vector<SparseTileBind> binds;
	while (regionIndex < numResourceRegions && rangeIndex < numHeapRanges)
	{
		if (!rangeTile)
		{
			rangeFlags = pHeapRangeFlags[rangeIndex];
			rangeCount = pHeapRangeTileCounts ? pHeapRangeTileCounts[rangeIndex] : ~0u;
			rangeOffset = pHeapRangeStartOffsets ? pHeapRangeStartOffsets[rangeIndex] : 0u;
			if (!pHeapRangeTileCounts && numHeapRanges != 1)
			{
				assert(!"pHeapRangeTileCounts is null");
				return;
			}
		}

		const auto coord = pResourceRegionStartCoords ? pResourceRegionStartCoords[regionIndex] : whole;
		TileRegionSize fallback = {};
		fallback.NumTiles = pResourceRegionStartCoords ? 1u : m_totalTiles;
		const auto region = pResourceRegionSizes ? pResourceRegionSizes[regionIndex] : fallback;
		if (!rangeCount)
		{
			++rangeIndex;
			rangeTile = 0;
			continue;
		}
		if (!region.NumTiles)
		{
			++regionIndex;
			regionTile = 0;
			continue;
		}

		const auto skip = (rangeFlags & TileRangeFlag::SKIP) == TileRangeFlag::SKIP;
		const auto nullRange = (rangeFlags & TileRangeFlag::NULL_RANGE) == TileRangeFlag::NULL_RANGE;
		const auto reuse = (rangeFlags & TileRangeFlag::REUSE_SINGLE_TILE) == TileRangeFlag::REUSE_SINGLE_TILE;
		if (!skip)
		{
			uint32_t tile = 0;
			if (tileIndex(coord, region, regionTile, tile))
			{
				SparseTileBind bind = {};
				bind.Tile = tile;
				if (!nullRange)
				{
					assert(pVkHeap);
					if (!pVkHeap) return;
					if (reuse)
					{
						const auto sparseAliased = m_pDevice && m_pDevice->SupportsSparseAliased();
						assert(sparseAliased);
						if (!sparseAliased) return;
					}

					const auto tileOffset = static_cast<VkDeviceSize>(reuse ? rangeOffset : rangeOffset + rangeTile) * TileBytes;
					if (pVkHeap->GetSize() && tileOffset + TileBytes > pVkHeap->GetSize())
					{
						assert(!"Tile heap offset is outside the heap");
						return;
					}
					if (m_sparseTypeBits && (m_sparseTypeBits & (1u << pVkHeap->GetMemoryType())) == 0)
					{
						assert(!"Tile heap memory type is incompatible with the sparse resource");
						return;
					}

					bind.Memory = pVkHeap->GetMemory();
					bind.Offset = tileOffset;
				}
				binds.push_back(bind);
			}
			else assert(!"Tile coordinate is outside the resource");
		}

		if (++rangeTile == rangeCount)
		{
			++rangeIndex;
			rangeTile = 0;
		}
		if (++regionTile == region.NumTiles)
		{
			++regionIndex;
			regionTile = 0;
		}
	}

	bindTiles(queue, binds);
}

void Resource_VK::CopyBoundTiles(VkQueue queue, const TiledResourceCoord& dstCoord, const Resource_VK* pSrc,
	const TiledResourceCoord& srcCoord, const TileRegionSize& region)
{
	assert(pSrc);
	assert(m_sparse && pSrc->m_sparse);
	if (!pSrc || !m_sparse || !pSrc->m_sparse) return;

	vector<SparseTileBind> binds;
	binds.reserve(region.NumTiles);
	for (auto i = 0u; i < region.NumTiles; ++i)
	{
		uint32_t dstTile = 0;
		uint32_t srcTile = 0;
		if (!tileIndex(dstCoord, region, i, dstTile) || !pSrc->tileIndex(srcCoord, region, i, srcTile) ||
			srcTile >= pSrc->m_tiles.size())
		{
			assert(!"CopyTileMappings coordinate is outside the resource");
			return;
		}

		SparseTileBind bind = {};
		bind.Tile = dstTile;
		bind.Memory = pSrc->m_tiles[srcTile].Memory;
		bind.Offset = pSrc->m_tiles[srcTile].MemoryOffset;
		binds.push_back(bind);
	}

	bindTiles(queue, binds);
}

void Resource_VK::Create(uint64_t nativeHandle, Device* pDevice, uint32_t maxThreads)
{
	initialize(pDevice, maxThreads);

	m_image = (VkImage)nativeHandle;
	m_isImage = true;
	m_ownsImage = false;
	m_undefined.assign(1, 1);
	m_discarded.clear();
	m_pendingLayout = 1;
	setState(ResourceState::COMMON, 1, maxThreads);
	m_name.clear();
}

void Resource_VK::SetName(const wchar_t* name)
{
	if (!name) return;

	m_name = name;
	if (m_pDevice)
	{
		const auto handle = m_isImage ? (uint64_t)m_image : (uint64_t)m_buffer;
		m_pDevice->SetDebugName(m_isImage ? VK_OBJECT_TYPE_IMAGE : VK_OBJECT_TYPE_BUFFER, handle, name);
	}
}

uint64_t Resource_VK::GetNativeHandle() const
{
	return m_isImage ? (uint64_t)m_image : (uint64_t)m_buffer;
}

void* Resource_VK::GetBackend()
{
	return this;
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

void Resource_VK::WrapImage(VkDevice device, VkImage image, VkFormat format, uint32_t width, uint32_t height,
	bool presentable)
{
	(void)device;
	m_image = image;
	m_isImage = true;
	m_ownsImage = false;
	m_undefined.assign(1, 1);
	m_discarded.clear();
	m_pendingLayout = 1;
	m_vkFormat = format;
	m_width = width;
	m_height = height;
	m_depthOrArray = 1;
	m_mipLevels = 1;
	m_presentable = presentable;
	// Non-WSI images use COLOR_ATTACHMENT as COMMON (vkd3d user backbuffer).
	if (!presentable)
		m_commonLayout = m_pDevice ? m_pDevice->PreferredImageLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) :
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	setState(ResourceState::COMMON, 1, 1);
}

VkImageLayout Resource_VK::ImageLayout(ResourceState state) const
{
	// PRESENT aliases COMMON. Swapchain images use PRESENT_SRC; other images use
	// the per-resource common layout (vkd3d-style) so COMMON<->SRV often needs no transition.
	//if (state == ResourceState::COMMON || state == ResourceState::PRESENT)
	if (state == ResourceState::COMMON)
	{
		if (m_presentable) return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		return m_isImage ? m_commonLayout : VK_IMAGE_LAYOUT_GENERAL;
	}

	auto layout = GetVKImageLayout(state);
	if (layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		switch (m_vkFormat)
		{
		case VK_FORMAT_D16_UNORM:
		case VK_FORMAT_D16_UNORM_S8_UINT:
		case VK_FORMAT_D24_UNORM_S8_UINT:
		case VK_FORMAT_D32_SFLOAT:
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
			layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			break;
		default:
			break;
		}
	}

	return m_pDevice ? m_pDevice->PreferredImageLayout(layout) : layout;
}

uint64_t Resource_VK::GetByteSize() const
{
	return m_byteSize;
}

bool Resource_VK::IsUndefinedLayout(uint32_t subresource) const
{
	return subresource < m_undefined.size() && m_undefined[subresource];
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
	for (auto i = 0u; i < numSubresources; ++i)
	{
		const auto index = firstSubresource + i;
		const auto pending = needsUndefinedLayout(index);
		if (!m_discarded[index])
		{
			m_discarded[index] = 1;
			++m_discardCount;
		}
		if (!pending) ++m_pendingLayout;
	}
}

bool Resource_VK::HasPendingDiscard() const
{
	return m_discardCount != 0;
}

VkAttachmentLoadOp Resource_VK::ConsumeDiscardLoadOp(uint32_t subresource)
{
	if (!isDiscarded(subresource)) return VK_ATTACHMENT_LOAD_OP_LOAD;

	consumeDiscarded(subresource);

	return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
}

VkAttachmentLoadOp Resource_VK::ConsumeDiscardLoadOp(uint32_t baseMip, uint32_t mipCount, uint32_t baseLayer, uint32_t layerCount)
{
	if (!m_discardCount) return VK_ATTACHMENT_LOAD_OP_LOAD;
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

void Resource_VK::AppendImageBarrier(VkImageMemoryBarrier2 image, vector<VkImageMemoryBarrier2>& barriers)
{
	if (!m_pendingLayout && image.oldLayout != VK_IMAGE_LAYOUT_UNDEFINED)
	{
		barriers.push_back(image);
		return;
	}

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

bool Resource_VK::CreateUploadBuffer(Device* pDevice, size_t byteSize)
{
	if (!m_pDevice || m_pDevice->GetDevice() == VK_NULL_HANDLE)
		XUSG_N_RETURN(initialize(pDevice ? pDevice : m_pDevice, 1), false);
	if (m_buffer != VK_NULL_HANDLE && m_byteSize >= byteSize && m_pDataBegin) return true;

	return createBuffer(nullptr, 0, byteSize, ResourceFlag::NONE, MemoryType::UPLOAD);
}

void* Resource_VK::GetMappedData() const
{
	return m_pDataBegin;
}

bool Resource_VK::initialize(Device* pDevice, uint32_t maxThreads)
{
	(void)maxThreads;
	if (!pDevice) return m_pDevice && m_pDevice->GetDevice() != VK_NULL_HANDLE;

	m_pDevice = AsVK(pDevice);
	assert(m_pDevice);
	m_states.clear();
	m_pDataBegin = nullptr;

	return true;
}

bool Resource_VK::allocateMemory(VkMemoryRequirements requirements, MemoryType memoryType, bool hostVisible)
{
	const auto device = m_pDevice->GetDevice();

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
	info.pNext = m_buffer != VK_NULL_HANDLE ? &flags : nullptr;
	info.allocationSize = requirements.size;
	info.memoryTypeIndex = m_pDevice->FindMemoryType(requirements.memoryTypeBits, properties);
	XUSG_VK_CHECK(vkAllocateMemory(device, &info, nullptr, &m_memory), cerr, false);
	m_ownsMemory = true;
	m_byteSize = requirements.size;
	if (m_image != VK_NULL_HANDLE) XUSG_VK_CHECK(vkBindImageMemory(device, m_image, m_memory, 0), cerr, false);
	if (m_buffer != VK_NULL_HANDLE) XUSG_VK_CHECK(vkBindBufferMemory(device, m_buffer, m_memory, 0), cerr, false);

	if (m_buffer != VK_NULL_HANDLE)
	{
		VkBufferDeviceAddressInfo address = {};
		address.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		address.buffer = m_buffer;
		m_address = vkGetBufferDeviceAddress(device, &address);
	}

	if (hostVisible) XUSG_VK_CHECK(vkMapMemory(device, m_memory, 0, m_byteSize, 0, &m_pDataBegin), cerr, false);

	return true;
}

void Resource_VK::setState(ResourceState state, uint32_t subresources, uint32_t maxThreads)
{
	m_states.resize(maxThreads ? maxThreads : 1);
	for (auto& states : m_states) states.assign(subresources ? subresources : 1, state);
}

VkBufferUsageFlags Resource_VK::getVKBufferUsage(ResourceFlag resourceFlags) const
{
	auto usage = GetVKBufferUsage(resourceFlags);
	const auto allowUAV = (resourceFlags & ResourceFlag::ALLOW_UNORDERED_ACCESS) != ResourceFlag::NONE;
	const auto denySRV = (resourceFlags & ResourceFlag::DENY_SHADER_RESOURCE) != ResourceFlag::NONE;

	switch (m_bufferType)
	{
	case BufferType::CONSTANT:
		usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		break;
	case BufferType::TYPED:
		if (!denySRV) usage |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
		if (allowUAV)
		{
			usage |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
			usage &= ~VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		}
		break;
	case BufferType::VERTEX:
		usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		break;
	case BufferType::INDEX:
		usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		if (!denySRV) usage |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
		if (allowUAV)
		{
			usage |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
			usage &= ~VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		}
		break;
	default:
		break;
	}

	return usage;
}

bool Resource_VK::createBuffer(Heap* pHeap, uint64_t heapOffset, size_t byteWidth,
	ResourceFlag resourceFlags, MemoryType memoryType, bool reserved)
{
	const auto device = m_pDevice->GetDevice();
	if (m_ownsBuffer && m_buffer != VK_NULL_HANDLE) vkDestroyBuffer(device, m_buffer, nullptr);

	m_sparse = false;
	m_hasSparseInfo = false;
	m_tilingReady = false;
	m_resourceFormat = Format::UNKNOWN;
	if (reserved)
	{
		const auto sparseBuffer = m_pDevice && m_pDevice->SupportsSparseBuffer();
		assert(sparseBuffer);
		if (!sparseBuffer) return false;
	}

	uint32_t sharingCount = 0;
	const uint32_t* pSharingFamilies = nullptr;
	if (m_pDevice) m_pDevice->GetSharingFamilies(sharingCount, pSharingFamilies);

	const auto size = reserved ? XUSG_DIV_UP(static_cast<uint64_t>(byteWidth), TileBytes) * TileBytes : byteWidth;
	VkBufferCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.size = size;
	info.usage = getVKBufferUsage(resourceFlags);
	info.sharingMode = sharingCount > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
	info.queueFamilyIndexCount = sharingCount;
	info.pQueueFamilyIndices = pSharingFamilies;
	if (reserved)
	{
		info.flags |= VK_BUFFER_CREATE_SPARSE_BINDING_BIT | VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT;
		if (m_pDevice->SupportsSparseAliased()) info.flags |= VK_BUFFER_CREATE_SPARSE_ALIASED_BIT;
	}
	if (m_pDevice && m_pDevice->HasConditionalRendering())
		info.usage |= VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT;
	if (m_pDevice && m_pDevice->HasTransformFeedback())
		info.usage |= VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT |
		VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_COUNTER_BUFFER_BIT_EXT;
	if (m_pDevice && m_pDevice->HasMicromap() && !m_pDevice->HasOpacityMicromapKhr())
	{
		info.usage |= VK_BUFFER_USAGE_MICROMAP_BUILD_INPUT_READ_ONLY_BIT_EXT;
		if ((resourceFlags & ResourceFlag::ACCELERATION_STRUCTURE) != ResourceFlag::NONE)
			info.usage |= VK_BUFFER_USAGE_MICROMAP_STORAGE_BIT_EXT;
	}
	XUSG_VK_CHECK(vkCreateBuffer(device, &info, nullptr, &m_buffer), cerr, false);
	m_ownsBuffer = true;
	m_isImage = false;
	m_sparse = reserved;
	m_width = size;
	m_byteSize = size;

	if (pHeap)
	{
		const auto pVkHeap = AsVK(pHeap);
		assert(pVkHeap);
		VkMemoryRequirements requirements = {};
		vkGetBufferMemoryRequirements(device, m_buffer, &requirements);
		assert((requirements.memoryTypeBits & (1u << pVkHeap->GetMemoryType())) &&
			"Placed buffer memory type is incompatible with the heap");
		if ((requirements.memoryTypeBits & (1u << pVkHeap->GetMemoryType())) == 0) return false;
		XUSG_VK_CHECK(vkBindBufferMemory(device, m_buffer, pVkHeap->GetMemory(), heapOffset), cerr, false);
		VkBufferDeviceAddressInfo address = {};
		address.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		address.buffer = m_buffer;
		m_address = vkGetBufferDeviceAddress(device, &address);
	}
	else if (reserved)
	{
		VkMemoryRequirements requirements = {};
		vkGetBufferMemoryRequirements(device, m_buffer, &requirements);
		m_sparseTypeBits = requirements.memoryTypeBits;
		m_address = 0;
		assert(requirements.alignment == 0 || TileBytes % requirements.alignment == 0);
	}
	else
	{
		VkMemoryRequirements requirements = {};
		vkGetBufferMemoryRequirements(device, m_buffer, &requirements);
		const auto host = memoryType == MemoryType::UPLOAD || memoryType == MemoryType::READBACK || memoryType == MemoryType::GPU_UPLOAD;
		XUSG_N_RETURN(allocateMemory(requirements, memoryType, host), false);
	}

	return true;
}

bool Resource_VK::createImage(Heap* pHeap, uint64_t heapOffset, uint32_t width, uint32_t height,
	uint32_t depth, Format format, uint16_t arraySize, ResourceFlag resourceFlags, uint8_t numMips,
	uint8_t sampleCount, MemoryType memoryType, bool is3D, bool cube, uint8_t numCastableFormats,
	const Format* pCastableFormats, bool reserved)
{
	const auto device = m_pDevice->GetDevice();
	if (m_ownsImage && m_image != VK_NULL_HANDLE)
	{
		vkDestroyImage(device, m_image, nullptr);
		m_image = VK_NULL_HANDLE;
		m_ownsImage = false;
	}
	releaseSparseMetadata();

	m_is3D = is3D;
	m_isImage = true;
	m_sparse = false;
	m_hasSparseInfo = false;
	m_tilingReady = false;
	m_resourceFormat = format;
	m_width = width;
	m_height = height;
	m_depthOrArray = is3D ? depth : arraySize;
	m_mipLevels = numMips ? numMips : 1;
	m_sampleCount = sampleCount ? sampleCount : 1;
	m_vkFormat = GetVKFormat(m_pDevice, format);
	// Mirror vkd3d vk_common_image_layout_from_d3d12_desc for non-presentable images.
	if ((resourceFlags & ResourceFlag::ALLOW_SIMULTANEOUS_ACCESS) != ResourceFlag::NONE)
		m_commonLayout = VK_IMAGE_LAYOUT_GENERAL;
	else if ((resourceFlags & ResourceFlag::DENY_SHADER_RESOURCE) != ResourceFlag::NONE)
		m_commonLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	else if ((resourceFlags & ResourceFlag::ALLOW_DEPTH_STENCIL) != ResourceFlag::NONE)
		m_commonLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	else m_commonLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	m_commonLayout = m_pDevice->PreferredImageLayout(m_commonLayout);
	if (reserved)
	{
		const auto sparseImage = m_pDevice && m_pDevice->SupportsSparseImage(is3D, m_sampleCount);
		assert(sparseImage);
		if (!sparseImage) return false;
	}
	m_undefined.assign(subresourceCount(), 1);
	m_discarded.clear();
	m_pendingLayout = static_cast<uint32_t>(m_undefined.size());

	vector<VkFormat> viewFormats;
	VkImageFormatListCreateInfo formatList = {};
	formatList.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO;
	if (numCastableFormats && pCastableFormats)
	{
		viewFormats.push_back(m_vkFormat);
		for (auto i = 0u; i < numCastableFormats; ++i)
		{
			const auto viewFormat = GetVKFormat(m_pDevice, pCastableFormats[i]);
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
	if (m_pDevice && m_pDevice->HasSampleLocations() && IsDepthStencilFormat(format))
		info.flags |= VK_IMAGE_CREATE_SAMPLE_LOCATIONS_COMPATIBLE_DEPTH_BIT_EXT;
	if (reserved)
	{
		info.flags |= VK_IMAGE_CREATE_SPARSE_BINDING_BIT | VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT;
		if (m_pDevice->SupportsSparseAliased()) info.flags |= VK_IMAGE_CREATE_SPARSE_ALIASED_BIT;
	}
	info.imageType = is3D ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
	info.format = m_vkFormat;
	info.extent = { width, (max)(height, 1u), is3D ? depth : 1u };
	info.mipLevels = m_mipLevels;
	info.arrayLayers = is3D ? 1u : arraySize;
	info.samples = static_cast<VkSampleCountFlagBits>(m_sampleCount);
	info.tiling = VK_IMAGE_TILING_OPTIMAL;
	info.usage = GetVKImageUsage(resourceFlags);
	info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	XUSG_VK_CHECK(vkCreateImage(device, &info, nullptr, &m_image), cerr, false);
	m_ownsImage = true;
	m_sparse = reserved;

	if (reserved) return finishSparseImage();

	if (pHeap)
	{
		const auto pVkHeap = AsVK(pHeap);
		assert(pVkHeap);
		VkMemoryRequirements requirements = {};
		vkGetImageMemoryRequirements(device, m_image, &requirements);
		assert((requirements.memoryTypeBits & (1u << pVkHeap->GetMemoryType())) &&
			"Placed image memory type is incompatible with the heap");
		if ((requirements.memoryTypeBits & (1u << pVkHeap->GetMemoryType())) == 0) return false;
		XUSG_VK_CHECK(vkBindImageMemory(device, m_image, pVkHeap->GetMemory(), heapOffset), cerr, false);
	}
	else
	{
		VkMemoryRequirements requirements = {};
		vkGetImageMemoryRequirements(device, m_image, &requirements);
		m_byteSize = requirements.size;
		XUSG_N_RETURN(allocateMemory(requirements, memoryType, false), false);
	}

	return true;
}

bool Resource_VK::finishSparseImage()
{
	uint32_t count = 0;
	const auto device = m_pDevice->GetDevice();
	vkGetImageSparseMemoryRequirements(device, m_image, &count, nullptr);
	assert(count);
	if (!count) return false;

	vector<VkSparseImageMemoryRequirements> requirements(count);
	vkGetImageSparseMemoryRequirements(device, m_image, &count, requirements.data());

	VkMemoryRequirements memory = {};
	vkGetImageMemoryRequirements(device, m_image, &memory);
	m_sparseTypeBits = memory.memoryTypeBits;

	vector<VkSparseMemoryBind> metadataBinds;
	VkDeviceSize metadataSize = 0;
	m_hasSparseInfo = false;
	for (const auto& requirement : requirements)
	{
		const auto aspect = requirement.formatProperties.aspectMask;
		const auto layers = (requirement.formatProperties.flags & VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT) || m_is3D ?
			1u : m_depthOrArray;
		if ((aspect & VK_IMAGE_ASPECT_METADATA_BIT) != 0)
		{
			for (auto layer = 0u; layer < layers; ++layer)
			{
				if (!requirement.imageMipTailSize) continue;

				VkSparseMemoryBind bind = {};
				bind.resourceOffset = requirement.imageMipTailOffset + requirement.imageMipTailStride * layer;
				bind.size = requirement.imageMipTailSize;
				bind.memoryOffset = metadataSize;
				bind.flags = VK_SPARSE_MEMORY_BIND_METADATA_BIT;
				metadataBinds.push_back(bind);
				metadataSize += requirement.imageMipTailSize;
			}
			continue;
		}

		const auto prefer = (aspect & (VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT)) != 0;
		if (!m_hasSparseInfo || prefer)
		{
			m_sparseInfo = requirement;
			m_hasSparseInfo = true;
		}
	}
	assert(m_hasSparseInfo);
	if (!m_hasSparseInfo) return false;

	if (metadataSize)
	{
		auto typeIndex = m_pDevice->FindMemoryType(memory.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		if (typeIndex == UINT32_MAX) typeIndex = m_pDevice->FindMemoryType(memory.memoryTypeBits, 0);
		assert(typeIndex != UINT32_MAX);
		if (typeIndex == UINT32_MAX) return false;

		VkMemoryAllocateInfo alloc = {};
		alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		alloc.allocationSize = metadataSize;
		alloc.memoryTypeIndex = typeIndex;
		XUSG_VK_CHECK(vkAllocateMemory(device, &alloc, nullptr, &m_metadata), cerr, false);
		for (auto& bind : metadataBinds) bind.memory = m_metadata;

		VkQueue queue = VK_NULL_HANDLE;
		const auto sparseQueue = m_pDevice->GetSparseQueue(queue);
		assert(sparseQueue);
		if (!sparseQueue) return false;

		VkSparseImageOpaqueMemoryBindInfo opaque = {};
		opaque.image = m_image;
		opaque.bindCount = static_cast<uint32_t>(metadataBinds.size());
		opaque.pBinds = metadataBinds.data();
		VkBindSparseInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_BIND_SPARSE_INFO;
		info.imageOpaqueBindCount = 1;
		info.pImageOpaqueBinds = &opaque;
		if (!m_pDevice->QueueSparseBind(queue, info)) return false;
	}

	return true;
}

void Resource_VK::releaseSparseMetadata()
{
	const auto device = m_pDevice->GetDevice();
	if (m_metadata != VK_NULL_HANDLE && device != VK_NULL_HANDLE) vkFreeMemory(device, m_metadata, nullptr);
	m_metadata = VK_NULL_HANDLE;
}

void Resource_VK::ensureTiling() const
{
	if (m_tilingReady) return;

	m_tilingReady = true;
	m_totalTiles = 0;
	m_singleMipTail = false;
	m_packed = {};
	m_tileShape = {};
	m_subresources.clear();
	m_nonPacked.clear();
	m_tiles.clear();

	auto mipDims = [&](uint32_t mip, uint32_t& width, uint32_t& height, uint32_t& depth)
	{
		width = static_cast<uint32_t>((max)(m_width >> mip, 1ull));
		height = (max)(m_height >> mip, 1u);
		depth = m_is3D ? (max)(m_depthOrArray >> mip, 1u) : 1u;
	};

	if (!m_isImage)
	{
		const auto bytes = m_byteSize ? m_byteSize : m_width;
		m_totalTiles = static_cast<uint32_t>(XUSG_DIV_UP(bytes, TileBytes));
		m_tileShape.WidthInTexels = static_cast<uint32_t>(TileBytes);
		m_tileShape.HeightInTexels = 1;
		m_tileShape.DepthInTexels = 1;
		TileSubresource subresource = {};
		subresource.WidthInTiles = m_totalTiles;
		subresource.HeightInTiles = 1;
		subresource.DepthInTiles = 1;
		m_subresources.push_back(subresource);
		m_nonPacked.push_back(0);
		if (!m_sparse) return;

		m_tiles.resize(m_totalTiles);
		for (auto i = 0u; i < m_totalTiles; ++i)
		{
			m_tiles[i] = {};
			m_tiles[i].Opaque = true;
			m_tiles[i].ResourceOffset = TileBytes * i;
			m_tiles[i].Size = TileBytes;
		}

		return;
	}

	const auto layers = m_is3D ? 1u : (max)(m_depthOrArray, 1u);
	uint32_t tileW = 1;
	uint32_t tileH = 1;
	uint32_t tileD = 1;
	uint32_t standardMips = m_mipLevels;
	uint32_t packedTiles = 0;
	if (m_sparse && m_hasSparseInfo)
	{
		const auto granularity = m_sparseInfo.formatProperties.imageGranularity;
		tileW = granularity.width ? granularity.width : 1u;
		tileH = granularity.height ? granularity.height : 1u;
		tileD = granularity.depth ? granularity.depth : 1u;
		standardMips = m_sparseInfo.imageMipTailSize ? (min)(m_mipLevels, m_sparseInfo.imageMipTailFirstLod) : m_mipLevels;
		const auto tailTiles = m_sparseInfo.imageMipTailSize ?
			static_cast<uint32_t>(XUSG_DIV_UP(m_sparseInfo.imageMipTailSize, TileBytes)) : 0u;
		m_singleMipTail = (m_sparseInfo.formatProperties.flags & VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT) != 0;
		if (standardMips < m_mipLevels)
			packedTiles = m_singleMipTail || m_is3D ? tailTiles : tailTiles * layers;
	}
	else
	{
		StandardTileTexels(m_resourceFormat, m_is3D, m_sampleCount, tileW, tileH, tileD);
		for (auto mip = 0u; mip < m_mipLevels; ++mip)
		{
			uint32_t width = 1;
			uint32_t height = 1;
			uint32_t depth = 1;
			mipDims(mip, width, height, depth);
			if (width < tileW || height < tileH || depth < tileD)
			{
				standardMips = mip;
				break;
			}
		}
		uint64_t bytes = 0;
		for (auto mip = standardMips; mip < m_mipLevels; ++mip)
		{
			uint32_t width = 1;
			uint32_t height = 1;
			uint32_t depth = 1;
			mipDims(mip, width, height, depth);
			bytes += MipByteSize(m_resourceFormat, width, height, depth);
		}
		if (!m_is3D) bytes *= layers;
		packedTiles = static_cast<uint32_t>(XUSG_DIV_UP(bytes, TileBytes));
	}

	m_tileShape.WidthInTexels = tileW;
	m_tileShape.HeightInTexels = tileH;
	m_tileShape.DepthInTexels = tileD;
	m_subresources.assign(static_cast<size_t>(layers) * m_mipLevels, {});
	auto cursor = 0u;
	for (auto layer = 0u; layer < layers; ++layer)
	{
		for (auto mip = 0u; mip < m_mipLevels; ++mip)
		{
			const auto subresource = mip + layer * m_mipLevels;
			auto& item = m_subresources[subresource];
			if (mip >= standardMips)
			{
				item.Packed = true;
				item.StartTile = UINT32_MAX;
				continue;
			}

			uint32_t width = 1;
			uint32_t height = 1;
			uint32_t depth = 1;
			mipDims(mip, width, height, depth);
			item.WidthInTiles = XUSG_DIV_UP(width, tileW);
			item.HeightInTiles = XUSG_DIV_UP(height, tileH);
			item.DepthInTiles = XUSG_DIV_UP(depth, tileD);
			item.StartTile = cursor;
			cursor += item.WidthInTiles * item.HeightInTiles * item.DepthInTiles;
			m_nonPacked.push_back(subresource);
		}
	}

	m_packed.NumStandardMips = static_cast<uint8_t>((min)(standardMips, 255u));
	m_packed.NumPackedMips = static_cast<uint8_t>((min)(m_mipLevels - standardMips, 255u));
	m_packed.NumTilesForPackedMips = packedTiles;
	m_packed.StartTileIndexInOverallResource = packedTiles ? cursor : 0u;
	m_totalTiles = cursor + packedTiles;
	if (!m_sparse) return;

	m_tiles.resize(m_totalTiles);
	auto subresource = 0u;
	auto tileX = 0;
	auto tileY = 0;
	auto tileZ = 0;
	for (auto i = 0u; i < m_totalTiles; ++i)
	{
		auto& tile = m_tiles[i];
		tile = {};
		if (m_packed.NumPackedMips && i >= m_packed.StartTileIndexInOverallResource)
		{
			auto packedIndex = i - m_packed.StartTileIndexInOverallResource;
			auto layer = 0u;
			if (m_hasSparseInfo && !m_singleMipTail && !m_is3D)
			{
				const auto perLayer = (max)(static_cast<uint32_t>(XUSG_DIV_UP(m_sparseInfo.imageMipTailSize, TileBytes)), 1u);
				layer = packedIndex / perLayer;
				packedIndex %= perLayer;
			}
			const auto byteOffset = TileBytes * packedIndex;
			tile.Opaque = true;
			if (m_hasSparseInfo)
			{
				tile.ResourceOffset = m_sparseInfo.imageMipTailOffset + m_sparseInfo.imageMipTailStride * layer + byteOffset;
				tile.Size = byteOffset < m_sparseInfo.imageMipTailSize ?
					(min)(TileBytes, m_sparseInfo.imageMipTailSize - byteOffset) : TileBytes;
			}
			else
			{
				tile.ResourceOffset = byteOffset;
				tile.Size = TileBytes;
			}
			continue;
		}

		while (subresource < m_subresources.size() && m_subresources[subresource].Packed) ++subresource;
		if (subresource >= m_subresources.size()) break;

		const auto& item = m_subresources[subresource];
		const auto mip = m_mipLevels ? subresource % m_mipLevels : 0u;
		const auto layer = m_mipLevels ? subresource / m_mipLevels : 0u;
		uint32_t width = 1;
		uint32_t height = 1;
		uint32_t depth = 1;
		mipDims(mip, width, height, depth);
		tile.SubresourceIndex = subresource;
		tile.Subresource.aspectMask = m_hasSparseInfo ? m_sparseInfo.formatProperties.aspectMask : VK_IMAGE_ASPECT_COLOR_BIT;
		tile.Subresource.mipLevel = mip;
		tile.Subresource.arrayLayer = m_is3D ? 0u : layer;
		tile.Offset.x = tileX * static_cast<int32_t>(tileW);
		tile.Offset.y = tileY * static_cast<int32_t>(tileH);
		tile.Offset.z = tileZ * static_cast<int32_t>(tileD);
		tile.Extent.width = (min)(tileW, width - static_cast<uint32_t>(tile.Offset.x));
		tile.Extent.height = (min)(tileH, height - static_cast<uint32_t>(tile.Offset.y));
		tile.Extent.depth = (min)(tileD, depth - static_cast<uint32_t>(tile.Offset.z));
		tile.Size = TileBytes;

		if (++tileX == static_cast<int32_t>(item.WidthInTiles))
		{
			tileX = 0;
			if (++tileY == static_cast<int32_t>(item.HeightInTiles))
			{
				tileY = 0;
				if (++tileZ == static_cast<int32_t>(item.DepthInTiles))
				{
					tileZ = 0;
					++subresource;
				}
			}
		}
	}
}

bool Resource_VK::tileIndex(const TiledResourceCoord& coord, const TileRegionSize& region, uint32_t indexInRegion,
	uint32_t& tile) const
{
	ensureTiling();
	if (coord.Subresource >= m_subresources.size()) return false;

	const auto& subresource = m_subresources[coord.Subresource];
	if (!region.UseBox)
	{
		const auto base = subresource.Packed ? m_packed.StartTileIndexInOverallResource + coord.X :
			subresource.StartTile + coord.X + subresource.WidthInTiles * (coord.Y + subresource.HeightInTiles * coord.Z);
		tile = base + indexInRegion;

		return tile < m_totalTiles;
	}

	if (subresource.Packed || !region.Width || !region.Height || !region.Depth) return false;

	const auto slice = static_cast<uint64_t>(region.Width) * region.Height;
	const auto x = coord.X + indexInRegion % region.Width;
	const auto y = coord.Y + (indexInRegion / region.Width) % region.Height;
	const auto z = coord.Z + static_cast<uint32_t>(indexInRegion / slice);
	if (x >= subresource.WidthInTiles || y >= subresource.HeightInTiles || z >= subresource.DepthInTiles) return false;

	tile = subresource.StartTile + x + subresource.WidthInTiles * (y + subresource.HeightInTiles * z);

	return tile < m_totalTiles;
}

void Resource_VK::bindTiles(VkQueue queue, const vector<SparseTileBind>& binds)
{
	if (binds.empty()) return;

	ensureTiling();
	vector<uint32_t> slot(m_totalTiles, UINT32_MAX);
	vector<SparseTileBind> unique;
	unique.reserve(binds.size());
	for (const auto& bind : binds)
	{
		if (bind.Tile >= m_totalTiles)
		{
			assert(!"Sparse tile index is outside the resource");
			continue;
		}
		if (slot[bind.Tile] == UINT32_MAX)
		{
			slot[bind.Tile] = static_cast<uint32_t>(unique.size());
			unique.push_back(bind);
		}
		else unique[slot[bind.Tile]] = bind;
	}

	vector<SparseTileBind> ordered;
	ordered.reserve(unique.size());
	for (auto tile = 0u; tile < m_totalTiles; ++tile)
		if (slot[tile] != UINT32_MAX) ordered.push_back(unique[slot[tile]]);
	if (ordered.empty()) return;

	vector<VkSparseMemoryBind> bufferBinds;
	vector<VkSparseImageMemoryBind> imageBinds;
	vector<VkSparseMemoryBind> opaqueBinds;
	auto emit = [&](uint32_t begin, uint32_t end)
	{
		const auto& first = ordered[begin];
		const auto& firstTile = m_tiles[first.Tile];
		const auto& lastTile = m_tiles[ordered[end - 1].Tile];
		if (!m_isImage || firstTile.Opaque)
		{
			VkSparseMemoryBind memory = {};
			memory.resourceOffset = firstTile.ResourceOffset;
			memory.size = lastTile.ResourceOffset + lastTile.Size - firstTile.ResourceOffset;
			memory.memory = first.Memory;
			memory.memoryOffset = first.Offset;
			if (m_isImage) opaqueBinds.push_back(memory);
			else bufferBinds.push_back(memory);

			return;
		}

		auto index = begin;
		while (index < end)
		{
			const auto& tile = m_tiles[ordered[index].Tile];
			const auto& subresource = m_subresources[tile.SubresourceIndex];
			const auto subTiles = subresource.WidthInTiles * subresource.HeightInTiles * subresource.DepthInTiles;
			auto covers = subTiles && ordered[index].Tile == subresource.StartTile && end - index >= subTiles;
			if (covers)
			{
				for (auto t = 0u; t < subTiles; ++t)
					if (ordered[index + t].Tile != subresource.StartTile + t) covers = false;
			}
			if (covers)
			{
				const auto& last = m_tiles[ordered[index + subTiles - 1].Tile];
				VkSparseImageMemoryBind image = {};
				image.subresource = tile.Subresource;
				image.offset = tile.Offset;
				image.extent.width = static_cast<uint32_t>(last.Offset.x) + last.Extent.width;
				image.extent.height = static_cast<uint32_t>(last.Offset.y) + last.Extent.height;
				image.extent.depth = static_cast<uint32_t>(last.Offset.z) + last.Extent.depth;
				image.memory = ordered[index].Memory;
				image.memoryOffset = ordered[index].Offset;
				imageBinds.push_back(image);
				index += subTiles;
			}
			else
			{
				VkSparseImageMemoryBind image = {};
				image.subresource = tile.Subresource;
				image.offset = tile.Offset;
				image.extent = tile.Extent;
				image.memory = ordered[index].Memory;
				image.memoryOffset = ordered[index].Offset;
				imageBinds.push_back(image);
				++index;
			}
		}
	};

	auto run = 0u;
	for (auto i = 1u; i <= ordered.size(); ++i)
	{
		auto split = i == ordered.size();
		if (!split)
		{
			const auto& previous = ordered[i - 1];
			const auto& next = ordered[i];
			const auto contiguous = next.Tile == previous.Tile + 1 && next.Memory == previous.Memory &&
				m_tiles[next.Tile].Opaque == m_tiles[previous.Tile].Opaque &&
				(next.Memory == VK_NULL_HANDLE || next.Offset == previous.Offset + TileBytes);
			split = !contiguous;
		}
		if (!split) continue;

		emit(run, i);
		run = i;
	}

	VkSparseBufferMemoryBindInfo bufferInfo = {};
	bufferInfo.buffer = m_buffer;
	bufferInfo.bindCount = static_cast<uint32_t>(bufferBinds.size());
	bufferInfo.pBinds = bufferBinds.data();
	VkSparseImageMemoryBindInfo imageInfo = {};
	imageInfo.image = m_image;
	imageInfo.bindCount = static_cast<uint32_t>(imageBinds.size());
	imageInfo.pBinds = imageBinds.data();
	VkSparseImageOpaqueMemoryBindInfo opaqueInfo = {};
	opaqueInfo.image = m_image;
	opaqueInfo.bindCount = static_cast<uint32_t>(opaqueBinds.size());
	opaqueInfo.pBinds = opaqueBinds.data();
	VkBindSparseInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_BIND_SPARSE_INFO;
	if (bufferInfo.bindCount)
	{
		info.bufferBindCount = 1;
		info.pBufferBinds = &bufferInfo;
	}
	if (imageInfo.bindCount)
	{
		info.imageBindCount = 1;
		info.pImageBinds = &imageInfo;
	}
	if (opaqueInfo.bindCount)
	{
		info.imageOpaqueBindCount = 1;
		info.pImageOpaqueBinds = &opaqueInfo;
	}
	if (!info.bufferBindCount && !info.imageBindCount && !info.imageOpaqueBindCount) return;
	if (!m_pDevice || !m_pDevice->QueueSparseBind(queue, info)) return;

	for (const auto& bind : ordered)
	{
		m_tiles[bind.Tile].Memory = bind.Memory;
		m_tiles[bind.Tile].MemoryOffset = bind.Offset;
	}
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
	if (subresource < m_discarded.size() && m_discarded[subresource])
	{
		m_discarded[subresource] = 0;
		if (m_discardCount) --m_discardCount;
	}
}

bool Resource_VK::needsUndefinedLayout(uint32_t subresource) const
{
	return isDiscarded(subresource) || IsUndefinedLayout(subresource);
}

void Resource_VK::consumeLayoutMark(uint32_t subresource)
{
	const auto pending = needsUndefinedLayout(subresource);
	consumeDiscarded(subresource);
	if (subresource < m_undefined.size()) m_undefined[subresource] = 0;
	if (pending && m_pendingLayout) --m_pendingLayout;
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

//--------------------------------------------------------------------------------------
// Constant buffer
//--------------------------------------------------------------------------------------

ConstantBuffer_VK::ConstantBuffer_VK()
{
	m_bufferType = BufferType::CONSTANT;
}

ConstantBuffer_VK::~ConstantBuffer_VK()
{
}

bool ConstantBuffer_VK::Create(Device* pDevice, size_t byteWidth, uint32_t numCBVs,
	const size_t* cbvByteOffsets, MemoryType memoryType, MemoryFlag memoryFlags, const wchar_t* name)
{
	(void)memoryFlags;
	assert(byteWidth);
	vector<size_t> offsetList;
	if (!cbvByteOffsets)
	{
		// Same placement as the DX12 path: each CBV is 256-byte aligned, which also
		// satisfies Vulkan minUniformBufferOffsetAlignment on current devices.
		size_t numBytes = 0;
		const auto cbvSize = AlignCBV(byteWidth / numCBVs);
		offsetList.resize(numCBVs);
		for (auto& offset : offsetList)
		{
			offset = numBytes;
			numBytes += cbvSize;
		}
		cbvByteOffsets = offsetList.data();
		byteWidth = cbvSize * numCBVs;
	}

	XUSG_N_RETURN(CreateResource(pDevice, byteWidth, memoryType), false);
	SetName(name);
	m_cbvs.resize(numCBVs);
	m_cbvByteOffsets.resize(numCBVs);
	const auto maxSize = static_cast<uint32_t>(byteWidth);
	for (auto i = 0u; i < numCBVs; ++i)
	{
		const auto byteOffset = cbvByteOffsets[i];
		const auto byteSize = static_cast<uint32_t>((i + 1 >= numCBVs ? maxSize : cbvByteOffsets[i + 1]) - byteOffset);
		m_cbvByteOffsets[i] = byteOffset;
		m_cbvs[i] = CreateCBV(byteSize, byteOffset);
	}

	return true;
}

bool ConstantBuffer_VK::Create(Device* pDevice, Heap* pHeap, uint64_t heapOffset, size_t byteWidth,
	uint32_t numCBVs, const size_t* cbvByteOffsets, const wchar_t* name)
{
	assert(byteWidth);
	vector<size_t> offsetList;
	if (!cbvByteOffsets)
	{
		size_t numBytes = 0;
		const auto cbvSize = AlignCBV(byteWidth / numCBVs);
		offsetList.resize(numCBVs);
		for (auto& offset : offsetList)
		{
			offset = numBytes;
			numBytes += cbvSize;
		}
		cbvByteOffsets = offsetList.data();
		byteWidth = cbvSize * numCBVs;
	}

	XUSG_N_RETURN(CreateResource(pDevice, pHeap, heapOffset, byteWidth), false);
	SetName(name);
	m_cbvs.resize(numCBVs);
	m_cbvByteOffsets.resize(numCBVs);
	const auto maxSize = static_cast<uint32_t>(byteWidth);
	for (auto i = 0u; i < numCBVs; ++i)
	{
		const auto byteOffset = cbvByteOffsets[i];
		const auto byteSize = static_cast<uint32_t>((i + 1 >= numCBVs ? maxSize : cbvByteOffsets[i + 1]) - byteOffset);
		m_cbvByteOffsets[i] = byteOffset;
		m_cbvs[i] = CreateCBV(byteSize, byteOffset);
	}

	return true;
}

bool ConstantBuffer_VK::CreateResource(Device* pDevice, size_t byteWidth, MemoryType memoryType,
	MemoryFlag memoryFlags, ResourceState initialResourceState)
{
	(void)memoryFlags;
	XUSG_N_RETURN(initialize(pDevice, 1), false);
	XUSG_N_RETURN(createBuffer(nullptr, 0, byteWidth, ResourceFlag::NONE, memoryType), false);
	setState(initialResourceState, 1, 1);

	return true;
}

bool ConstantBuffer_VK::CreateResource(Device* pDevice, Heap* pHeap, uint64_t heapOffset, size_t byteWidth,
	ResourceState initialResourceState)
{
	XUSG_N_RETURN(initialize(pDevice, 1), false);
	XUSG_N_RETURN(createBuffer(pHeap, heapOffset, byteWidth, ResourceFlag::NONE, MemoryType::DEFAULT, pHeap == nullptr), false);
	setState(initialResourceState, 1, 1);

	return true;
}

bool ConstantBuffer_VK::Upload(CommandList* pCommandList, Resource* pUploader, const void* pData,
	size_t size, size_t byteOffset, ResourceState srcState, ResourceState dstState)
{
	assert(pUploader);
	const auto pVkUploader = AsVK(pUploader);
	assert(pVkUploader);
	XUSG_N_RETURN(pVkUploader->CreateUploadBuffer(m_pDevice, size), false);
	auto dst = static_cast<uint8_t*>(pVkUploader->GetMappedData());
	memcpy(dst, pData, size);

	ResourceBarrier barrier = { this, srcState == ResourceState::AUTO ? GetResourceState() : srcState, ResourceState::COPY_DEST };
	if (barrier.StateBefore != barrier.StateAfter)
	{
		Transition(ResourceState::COPY_DEST);
		pCommandList->Barrier(1, &barrier);
	}
	pCommandList->CopyBufferRegion(this, byteOffset, pVkUploader, 0, size);
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
	if (!m_pDataBegin && m_memory != VK_NULL_HANDLE) vkMapMemory(m_pDevice->GetDevice(), m_memory, 0, m_byteSize, 0, &m_pDataBegin);

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
	Device* pDevice, size_t byteWidth, uint64_t requiredAlignment)
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
// Resource base
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

bool Texture_VK::Create(Device* pDevice, uint32_t width, uint32_t height, Format format,
	uint16_t arraySize, ResourceFlag resourceFlags, uint8_t numMips, uint8_t sampleCount, bool isCubeMap,
	MemoryFlag memoryFlags, const wchar_t* name, uint16_t srvComponentMapping, TextureLayout textureLayout,
	uint8_t numUavFormats, const Format* uavFormats, uint32_t maxThreads)
{
	(void)memoryFlags;
	(void)textureLayout;
	assert(sampleCount >= 1 && sampleCount <= 16);
	assert(sampleCount <= 1 || numMips <= 1);
	XUSG_N_RETURN(initialize(pDevice, maxThreads), false);
	m_format = format;
	m_hasPromotion = (resourceFlags & ResourceFlag::ALLOW_SIMULTANEOUS_ACCESS) == ResourceFlag::ALLOW_SIMULTANEOUS_ACCESS;
	m_cube = isCubeMap;

	const auto hasSRV = (resourceFlags & ResourceFlag::DENY_SHADER_RESOURCE) == ResourceFlag::NONE;
	const auto hasUAV = (resourceFlags & ResourceFlag::ALLOW_UNORDERED_ACCESS) == ResourceFlag::ALLOW_UNORDERED_ACCESS;
	const auto fullMips = CalculateMipLevels(width, height);
	numMips = numMips ? (min)(numMips, fullMips) : fullMips;
	numMips = (max)(numMips, static_cast<uint8_t>(1));

	XUSG_N_RETURN(createImage(nullptr, 0, width, height, 1, format, arraySize, resourceFlags, numMips, sampleCount,
		MemoryType::DEFAULT, false, isCubeMap, numUavFormats, uavFormats), false);
	setState(ResourceState::COMMON, static_cast<uint32_t>(arraySize) * numMips, maxThreads);
	SetName(name);
	if (hasSRV) createSRVs(arraySize, format, numMips, sampleCount > 1, isCubeMap, srvComponentMapping);
	if (hasUAV)
	{
		createUAVs(arraySize, format, numMips);
		if (numUavFormats && uavFormats)
			for (auto i = 0u; i < numUavFormats; ++i) createCastableUAVs(arraySize, uavFormats[i], numMips);
	}
	if (hasSRV && hasUAV)
		createSingleLevelSRVs(arraySize, numMips, format, sampleCount > 1, isCubeMap, srvComponentMapping);

	return true;
}

bool Texture_VK::Create(Device* pDevice, Heap* pHeap, uint64_t heapOffset, uint32_t width, uint32_t height,
	Format format, uint16_t arraySize, ResourceFlag resourceFlags, uint8_t numMips, uint8_t sampleCount, bool isCubeMap,
	const wchar_t* name, uint16_t srvComponentMapping, TextureLayout textureLayout, uint8_t numUavFormats,
	const Format* uavFormats, uint32_t maxThreads)
{
	(void)textureLayout;
	assert(sampleCount >= 1 && sampleCount <= 16);
	assert(sampleCount <= 1 || numMips <= 1);
	XUSG_N_RETURN(initialize(pDevice, maxThreads), false);
	m_format = format;
	m_hasPromotion = (resourceFlags & ResourceFlag::ALLOW_SIMULTANEOUS_ACCESS) == ResourceFlag::ALLOW_SIMULTANEOUS_ACCESS;
	m_cube = isCubeMap;

	const auto hasSRV = (resourceFlags & ResourceFlag::DENY_SHADER_RESOURCE) == ResourceFlag::NONE;
	const auto hasUAV = (resourceFlags & ResourceFlag::ALLOW_UNORDERED_ACCESS) == ResourceFlag::ALLOW_UNORDERED_ACCESS;
	const auto fullMips = CalculateMipLevels(width, height);
	numMips = numMips ? (min)(numMips, fullMips) : fullMips;
	numMips = (max)(numMips, static_cast<uint8_t>(1));

	XUSG_N_RETURN(createImage(pHeap, heapOffset, width, height, 1, format, arraySize, resourceFlags, numMips,
		sampleCount, MemoryType::DEFAULT, false, isCubeMap, numUavFormats, uavFormats, pHeap == nullptr), false);
	setState(ResourceState::COMMON, static_cast<uint32_t>(arraySize) * numMips, maxThreads);
	SetName(name);
	if (hasSRV) createSRVs(arraySize, format, numMips, sampleCount > 1, isCubeMap, srvComponentMapping);
	if (hasUAV) createUAVs(arraySize, format, numMips);
	if (hasSRV && hasUAV)
		createSingleLevelSRVs(arraySize, numMips, format, sampleCount > 1, isCubeMap, srvComponentMapping);

	return true;
}

bool Texture_VK::CreateResource(Device* pDevice, uint32_t width, uint32_t height, Format format,
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

bool Texture_VK::CreateResource(Device* pDevice, Heap* pHeap, uint64_t heapOffset, uint32_t width,
	uint32_t height, Format format, uint16_t arraySize, ResourceFlag resourceFlags, uint8_t numMips,
	uint8_t sampleCount, ResourceState initialResourceState, TextureLayout textureLayout, uint8_t numCastableFormats,
	const Format* pCastableFormats, uint32_t maxThreads)
{
	(void)textureLayout;
	XUSG_N_RETURN(initialize(pDevice, maxThreads), false);
	m_format = format;
	XUSG_N_RETURN(createImage(pHeap, heapOffset, width, height, 1, format, arraySize, resourceFlags, numMips,
		sampleCount, MemoryType::DEFAULT, false, false, numCastableFormats, pCastableFormats, pHeap == nullptr), false);
	setState(initialResourceState, static_cast<uint32_t>(arraySize) * (numMips ? numMips : 1), maxThreads);

	return true;
}

bool Texture_VK::Upload(CommandList* pCommandList, Resource* pUploader, const SubresourceData* pSubresourceData,
	uint32_t numSubresources, ResourceState dstState, uint32_t firstSubresource, uint32_t threadIdx)
{
	assert(pCommandList);
	XUSG_N_RETURN(pSubresourceData, false);
	const auto pVkUploader = AsVK(pUploader);
	assert(pVkUploader);
	const auto uploadSize = GetRequiredIntermediateSize(firstSubresource, numSubresources);
	XUSG_N_RETURN(pVkUploader->CreateUploadBuffer(m_pDevice, uploadSize), false);

	vector<ResourceBarrier> barriers(numSubresources);
	auto numBarriers = 0u;
	for (auto i = 0u; i < numSubresources; ++i)
		numBarriers = SetBarrier(barriers.data(), ResourceState::COPY_DEST, numBarriers, firstSubresource + i,
			BarrierFlag::NONE, ResourceState::AUTO, threadIdx);
	if (numBarriers) pCommandList->Barrier(numBarriers, barriers.data());

	auto dst = static_cast<uint8_t*>(pVkUploader->GetMappedData());
	auto offset = 0ull;
	const auto pVkCommandList = AsVK(pCommandList);
	assert(pVkCommandList);
	for (auto i = 0u; i < numSubresources; ++i)
	{
		const auto subresource = firstSubresource + i;
		const auto mip = subresource % (max)(m_mipLevels, 1u);
		const auto width = static_cast<uint32_t>((max)(m_width >> mip, 1ull));
		const auto height = (max)(m_height >> mip, 1u);
		const auto depth = m_is3D ? (max)(m_depthOrArray >> mip, 1u) : 1u;
		const auto block = IsCompressedFormat(m_format) ? 4u : 1u;
		const auto rows = XUSG_DIV_UP(height, block);
		const auto pitch = RowPitch(m_format, width);
		const auto srcPitch = static_cast<uint32_t>(pSubresourceData[i].RowPitch);
		const auto srcSlicePitch = pSubresourceData[i].SlicePitch ?
			static_cast<size_t>(pSubresourceData[i].SlicePitch) : static_cast<size_t>(srcPitch) * rows;
		const auto copyBytes = (min)(pitch, srcPitch);
		for (auto slice = 0u; slice < depth; ++slice)
		{
			const auto srcSlice = static_cast<const uint8_t*>(pSubresourceData[i].pData) + slice * srcSlicePitch;
			for (auto row = 0u; row < rows; ++row)
				memcpy(dst + offset + (static_cast<size_t>(slice) * rows + row) * pitch,
					srcSlice + static_cast<size_t>(row) * srcPitch, copyBytes);
		}
		const auto texelBytes = CopyTexelBytes(m_format);
		const auto bufferRowLength = texelBytes ? pitch / texelBytes * block : width;
		const auto bufferImageHeight = XUSG_DIV_UP(height, block) * block;
		pVkCommandList->CopyBufferToImage(this, subresource, pVkUploader, offset, bufferRowLength, bufferImageHeight);
		offset += static_cast<uint64_t>(pitch) * rows * depth;
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

	if ((VkBuffer)pReadBuffer->GetNativeHandle() == VK_NULL_HANDLE)
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

	assert(pCommandList);
	const auto pVkCommandList = AsVK(pCommandList);
	assert(pVkCommandList);
	const auto aspect = IsDepthStencilFormat(m_format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	for (auto i = 0u; i < numSubresources; ++i)
	{
		pVkCommandList->CopyImageToBuffer(this, footprints[i].Mip, footprints[i].Layer, pReadBuffer, footprints[i].Offset,
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
	const auto layout = IsDepthStencilFormat(viewFormat) ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL :
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	fillImageView(*view, viewType, GetVKFormat(m_pDevice, viewFormat), mostDetailedMip, numMips,
		firstArraySlice, arraySize, ImageViewAspect(viewFormat, plane),
		srvComponentMapping, layout, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);

	return view.get();
}

Descriptor Texture_VK::CreateUAV(uint16_t arraySize, uint16_t firstArraySlice, Format format, uint8_t mipLevel, uint8_t plane)
{
	(void)plane;
	auto view = storeView(ResourceViewType::UAV, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, true, false);
	const auto viewType = arraySize > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : m_height > 1 ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_1D;
	fillImageView(*view, viewType, GetVKFormat(m_pDevice, format == Format::UNKNOWN ? m_format : format), mipLevel, 1,
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
	uint32_t samplerSlot, const Pipeline& pipeline) const
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

uint32_t Texture_VK::Blit(const CommandList* pCommandList, ResourceBarrier* pBarriers, uint32_t groupSizeX,
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

uint32_t Texture_VK::GenerateMips(const CommandList* pCommandList, ResourceBarrier* pBarriers, uint32_t groupSizeX,
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

	// Chain samples the previous mip in CS; keep intermediate barriers to NPSR only.
	const auto sampleState = ResourceState::NON_PIXEL_SHADER_RESOURCE;
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
			if ((m_states[threadIdx][subresource] & sampleState) != sampleState)
				numBarriers = SetBarrier(pBarriers, sampleState, numBarriers, subresource, BarrierFlag::NONE, ResourceState::AUTO, threadIdx);
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
		const auto depth = m_is3D ? (max)(m_depthOrArray >> mip, 1u) : 1u;
		const auto block = IsCompressedFormat(m_format) ? 4u : 1u;
		size += static_cast<uint64_t>(RowPitch(m_format, width)) * XUSG_DIV_UP(height, block) * depth;
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
	if (!m_pDataBegin && m_memory != VK_NULL_HANDLE) vkMapMemory(m_pDevice->GetDevice(), m_memory, 0, m_byteSize, 0, &m_pDataBegin);

	return m_pDataBegin;
}

void Texture_VK::GetAllocationInfo(uint64_t& byteSize, uint64_t& alignment, Device* pDevice,
	uint32_t width, uint32_t height, Format format, uint16_t arraySize, ResourceFlag resourceFlags,
	uint8_t numMips, uint8_t sampleCount, TextureLayout textureLayout, uint64_t requiredAlignment)
{
	(void)textureLayout;
	assert(pDevice);
	const auto pVkDevice = AsVK(pDevice);
	assert(pVkDevice);
	const auto device = pVkDevice->GetDevice();
	VkImageCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	info.imageType = height > 1 ? VK_IMAGE_TYPE_2D : VK_IMAGE_TYPE_1D;
	info.format = GetVKFormat(pVkDevice, format);
	info.extent = { width, (max)(height, 1u), 1 };
	info.mipLevels = numMips ? numMips : 1;
	info.arrayLayers = arraySize ? arraySize : 1;
	info.samples = static_cast<VkSampleCountFlagBits>(sampleCount ? sampleCount : 1);
	info.tiling = VK_IMAGE_TILING_OPTIMAL;
	info.usage = GetVKImageUsage(resourceFlags);
	VkImage image = VK_NULL_HANDLE;
	if (vkCreateImage(device, &info, nullptr, &image) < 0)
	{
		byteSize = 0;
		alignment = requiredAlignment;

		return;
	}
	VkMemoryRequirements requirements = {};
	vkGetImageMemoryRequirements(device, image, &requirements);
	vkDestroyImage(device, image, nullptr);
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
	layout = m_pDevice ? m_pDevice->PreferredImageLayout(layout) : layout;
	view.Layout = layout;
	view.VkType = type;
	view.IsImage = true;
	if (view.Type != ResourceViewType::RTV && view.Type != ResourceViewType::DSV) return;

	VkImageViewCreateInfo info = view.ViewInfo;
	auto& target = view.Target;
	target = {};
	target.Rendering.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	target.Rendering.imageLayout = layout;
	target.Rendering.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	target.Rendering.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	vkCreateImageView(m_pDevice->GetDevice(), &info, nullptr, &target.Rendering.imageView);
	target.Width = static_cast<uint32_t>((max)(m_width >> baseMip, 1ull));
	target.Height = (max)(m_height >> baseMip, 1u);
	target.BaseMip = baseMip;
	target.MipCount = static_cast<uint16_t>(info.subresourceRange.levelCount);
	target.BaseLayer = baseLayer;
	target.Layers = static_cast<uint16_t>(info.subresourceRange.layerCount);
	target.SingleSubresource = target.MipCount == 1 && target.Layers == 1;
	target.Subresource = subresourceIndex(baseMip, baseLayer);
	target.HasStencil = format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
		format == VK_FORMAT_D16_UNORM_S8_UINT;
	target.ReadOnly = 0;
}

//--------------------------------------------------------------------------------------
// Render target
//--------------------------------------------------------------------------------------

RenderTarget_VK::RenderTarget_VK()
{
}

RenderTarget_VK::~RenderTarget_VK()
{
}

bool RenderTarget_VK::Create(Device* pDevice, uint32_t width, uint32_t height, Format format, uint16_t arraySize,
	ResourceFlag resourceFlags, uint8_t numMips, uint8_t sampleCount, const float* pClearColor, bool isCubeMap,
	MemoryFlag memoryFlags, const wchar_t* name, uint16_t srvComponentMapping, TextureLayout textureLayout,
	uint8_t numUavFormats, const Format* uavFormats, uint32_t maxThreads)
{
	(void)pClearColor;
	resourceFlags = resourceFlags | ResourceFlag::ALLOW_RENDER_TARGET;
	XUSG_N_RETURN(Texture_VK::Create(pDevice, width, height, format, arraySize, resourceFlags, numMips, sampleCount,
		isCubeMap, memoryFlags, name, srvComponentMapping, textureLayout, numUavFormats, uavFormats, maxThreads), false);
	if ((resourceFlags & ResourceFlag::DENY_SHADER_RESOURCE) == ResourceFlag::NONE &&
		(resourceFlags & ResourceFlag::ALLOW_UNORDERED_ACCESS) == ResourceFlag::NONE)
		createSingleLevelSRVs(arraySize, static_cast<uint8_t>(m_mipLevels), format, sampleCount > 1, isCubeMap, srvComponentMapping);
	m_rtvs.resize(arraySize);
	for (uint16_t slice = 0; slice < arraySize; ++slice)
	{
		m_rtvs[slice].resize(m_mipLevels);
		for (auto mip = 0u; mip < m_mipLevels; ++mip)
			m_rtvs[slice][mip] = CreateRTV(1, slice, format, static_cast<uint8_t>(mip), sampleCount > 1);
	}

	return true;
}

bool RenderTarget_VK::Create(Device* pDevice, Heap* pHeap, uint64_t heapOffset, uint32_t width, uint32_t height,
	Format format, uint16_t arraySize, ResourceFlag resourceFlags, uint8_t numMips, uint8_t sampleCount, const float* pClearColor,
	bool isCubeMap, const wchar_t* name, uint16_t srvComponentMapping, TextureLayout textureLayout,
	uint8_t numUavFormats, const Format* uavFormats, uint32_t maxThreads)
{
	(void)pClearColor;
	resourceFlags = resourceFlags | ResourceFlag::ALLOW_RENDER_TARGET;
	XUSG_N_RETURN(Texture_VK::Create(pDevice, pHeap, heapOffset, width, height, format, arraySize, resourceFlags, numMips,
		sampleCount, isCubeMap, name, srvComponentMapping, textureLayout, numUavFormats, uavFormats, maxThreads), false);
	if ((resourceFlags & ResourceFlag::DENY_SHADER_RESOURCE) == ResourceFlag::NONE &&
		(resourceFlags & ResourceFlag::ALLOW_UNORDERED_ACCESS) == ResourceFlag::NONE)
		createSingleLevelSRVs(arraySize, static_cast<uint8_t>(m_mipLevels), format, sampleCount > 1, isCubeMap, srvComponentMapping);
	m_rtvs.resize(arraySize);
	for (uint16_t slice = 0; slice < arraySize; ++slice)
	{
		m_rtvs[slice].resize(m_mipLevels);
		for (auto mip = 0u; mip < m_mipLevels; ++mip)
			m_rtvs[slice][mip] = CreateRTV(1, slice, format, static_cast<uint8_t>(mip), sampleCount > 1);
	}

	return true;
}

bool RenderTarget_VK::CreateArray(Device* pDevice, uint32_t width, uint32_t height, uint16_t arraySize, Format format,
	ResourceFlag resourceFlags, uint8_t numMips, uint8_t sampleCount, const float* pClearColor, bool isCubeMap,
	MemoryFlag memoryFlags, const wchar_t* name, uint16_t srvComponentMapping, TextureLayout textureLayout,
	uint8_t numUavFormats, const Format* uavFormats, uint32_t maxThreads)
{
	(void)pClearColor;
	resourceFlags = resourceFlags | ResourceFlag::ALLOW_RENDER_TARGET;
	XUSG_N_RETURN(Texture_VK::Create(pDevice, width, height, format, arraySize, resourceFlags, numMips, sampleCount,
		isCubeMap, memoryFlags, name, srvComponentMapping, textureLayout, numUavFormats, uavFormats, maxThreads), false);
	if ((resourceFlags & ResourceFlag::DENY_SHADER_RESOURCE) == ResourceFlag::NONE &&
		(resourceFlags & ResourceFlag::ALLOW_UNORDERED_ACCESS) == ResourceFlag::NONE)
		createSingleLevelSRVs(arraySize, static_cast<uint8_t>(m_mipLevels), format, sampleCount > 1, isCubeMap, srvComponentMapping);
	m_rtvs.resize(1);
	m_rtvs[0].resize(m_mipLevels);
	for (auto mip = 0u; mip < m_mipLevels; ++mip)
		m_rtvs[0][mip] = CreateRTV(arraySize, 0, format, static_cast<uint8_t>(mip), sampleCount > 1);

	return true;
}

bool RenderTarget_VK::CreateArray(Device* pDevice, Heap* pHeap, uint64_t heapOffset, uint32_t width, uint32_t height,
	uint16_t arraySize, Format format, ResourceFlag resourceFlags, uint8_t numMips, uint8_t sampleCount, const float* pClearColor,
	bool isCubeMap, const wchar_t* name, uint16_t srvComponentMapping, TextureLayout textureLayout,
	uint8_t numUavFormats, const Format* uavFormats, uint32_t maxThreads)
{
	(void)pClearColor;
	resourceFlags = resourceFlags | ResourceFlag::ALLOW_RENDER_TARGET;
	XUSG_N_RETURN(Texture_VK::Create(pDevice, pHeap, heapOffset, width, height, format, arraySize, resourceFlags, numMips,
		sampleCount, isCubeMap, name, srvComponentMapping, textureLayout, numUavFormats, uavFormats, maxThreads), false);
	if ((resourceFlags & ResourceFlag::DENY_SHADER_RESOURCE) == ResourceFlag::NONE &&
		(resourceFlags & ResourceFlag::ALLOW_UNORDERED_ACCESS) == ResourceFlag::NONE)
		createSingleLevelSRVs(arraySize, static_cast<uint8_t>(m_mipLevels), format, sampleCount > 1, isCubeMap, srvComponentMapping);
	m_rtvs.resize(1);
	m_rtvs[0].resize(m_mipLevels);
	for (auto mip = 0u; mip < m_mipLevels; ++mip)
		m_rtvs[0][mip] = CreateRTV(arraySize, 0, format, static_cast<uint8_t>(mip), sampleCount > 1);

	return true;
}

bool RenderTarget_VK::CreateFromSwapChain(Device* pDevice, SwapChain* pSwapChain, uint32_t bufferIndex, uint32_t maxThreads)
{
	assert(pSwapChain);
	m_pDevice = AsVK(pDevice);
	assert(m_pDevice);
	XUSG_N_RETURN(initialize(pDevice, maxThreads), false);
	// GetBuffer sets presentable=true for direct WSI, false for user-BB blit (AMD).
	XUSG_N_RETURN(pSwapChain->GetBuffer(static_cast<uint8_t>(bufferIndex), this), false);
	m_format = GetFormatFromVK(m_vkFormat);
	m_rtvs.resize(1);
	m_rtvs[0].resize(1);
	m_rtvs[0][0] = CreateRTV(1, 0, m_format, 0, false);

	return true;
}

bool RenderTarget_VK::CreateResource(Device* pDevice, uint32_t width, uint32_t height, Format format, uint16_t arraySize,
	ResourceFlag resourceFlags, uint8_t numMips, uint8_t sampleCount, MemoryFlag memoryFlags, ResourceState initialResourceState,
	const float* pClearColor, TextureLayout textureLayout, uint8_t numCastableFormats, const Format* pCastableFormats, uint32_t maxThreads)
{
	(void)pClearColor;

	return Texture_VK::CreateResource(pDevice, width, height, format, arraySize, resourceFlags | ResourceFlag::ALLOW_RENDER_TARGET,
		numMips, sampleCount, memoryFlags, initialResourceState, textureLayout, numCastableFormats, pCastableFormats, maxThreads);
}

bool RenderTarget_VK::CreateResource(Device* pDevice, Heap* pHeap, uint64_t heapOffset, uint32_t width,
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
	fillImageView(*view, viewType, GetVKFormat(m_pDevice, format == Format::UNKNOWN ? m_format : format), mipLevel, 1,
		firstArraySlice, arraySize, VK_IMAGE_ASPECT_COLOR_BIT, XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);

	return view.get();
}

void RenderTarget_VK::Blit(const CommandList* pCommandList, const DescriptorTable& srcSrvTable, uint32_t srcSlot,
	uint8_t mipLevel, uint16_t baseSlice, uint16_t numSlices, const DescriptorTable& samplerTable, uint32_t samplerSlot,
	const Pipeline& pipeline, uint32_t offsetForSliceId, uint32_t cbSlot) const
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

uint32_t RenderTarget_VK::Blit(const CommandList* pCommandList, ResourceBarrier* pBarriers, uint8_t mipLevel, int8_t srcMipLevel,
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

uint32_t RenderTarget_VK::GenerateMips(const CommandList* pCommandList, ResourceBarrier* pBarriers, ResourceState dstState,
	const PipelineLayout& pipelineLayout, const Pipeline& pipeline, const DescriptorTable* pSrcSrvTables, uint32_t srcSlot,
	const DescriptorTable& samplerTable, uint32_t samplerSlot, uint32_t numBarriers, uint8_t baseMip, uint8_t numMips,
	uint16_t baseSlice, uint16_t numSlices, uint32_t offsetForSliceId, uint32_t cbSlot, uint32_t threadIdx)
{
	// Graphics blit: baseMip == 0 may sample an external SRV into mip0.
	if (pipelineLayout) pCommandList->SetGraphicsPipelineLayout(pipelineLayout);
	if (samplerTable) pCommandList->SetGraphicsDescriptorTable(samplerSlot, samplerTable);
	if (pipeline) pCommandList->SetPipelineState(pipeline);
	if (!(numMips || baseMip || numSlices || baseSlice))
		numBarriers = SetBarrier(pBarriers, ResourceState::RENDER_TARGET, numBarriers,
			XUSG_BARRIER_ALL_SUBRESOURCES, BarrierFlag::NONE, ResourceState::AUTO, threadIdx);
	if (!numSlices) numSlices = static_cast<uint16_t>(m_depthOrArray - baseSlice);
	if (!numMips) numMips = static_cast<uint8_t>(m_mipLevels - baseMip);
	// Chain samples the previous mip in PS; keep intermediate barriers to PSR only.
	// Final mip uses caller dstState (may include NPSR / ASR).
	const auto sampleState = ResourceState::PIXEL_SHADER_RESOURCE;
	for (uint8_t i = 0; i < numMips; ++i)
	{
		const uint8_t j = baseMip + i;
		if (j) for (uint16_t k = 0; k < numSlices; ++k)
		{
			const uint16_t n = baseSlice + k;
			// Dest mip fully overwritten - end live pass if needed, UNDEFINED for barrier.
			pCommandList->DiscardResource(const_cast<RenderTarget_VK*>(this), 0, nullptr,
				CalculateSubresource(j, n), 1);
			numBarriers = SetBarrier(pBarriers, j, ResourceState::RENDER_TARGET, numBarriers, n,
				BarrierFlag::NONE, ResourceState::AUTO, 0, threadIdx);
			numBarriers = SetBarrier(pBarriers, static_cast<uint8_t>(j - 1), sampleState, numBarriers, n,
				BarrierFlag::NONE, ResourceState::AUTO, 0, threadIdx);
		}
		else for (uint16_t k = 0; k < numSlices; ++k)
		{
			const uint16_t n = baseSlice + k;
			pCommandList->DiscardResource(const_cast<RenderTarget_VK*>(this), 0, nullptr,
				CalculateSubresource(j, n), 1);
			numBarriers = SetBarrier(pBarriers, j, ResourceState::RENDER_TARGET, numBarriers, n,
				BarrierFlag::NONE, ResourceState::AUTO, 0, threadIdx);
		}
		if (numBarriers)
		{
			pCommandList->Barrier(numBarriers, pBarriers);
			numBarriers = 0;
		}
		Blit(pCommandList, pSrcSrvTables ? pSrcSrvTables[i] : XUSG_NULL, srcSlot, j, baseSlice, numSlices,
			XUSG_NULL, samplerSlot, nullptr, offsetForSliceId, cbSlot);
	}
	const auto lastMip = static_cast<uint8_t>(baseMip + numMips - 1);
	for (uint16_t i = 0; i < numSlices; ++i)
		numBarriers = SetBarrier(pBarriers, lastMip, dstState, numBarriers, baseSlice + i,
			BarrierFlag::NONE, ResourceState::AUTO, 0, threadIdx);

	return numBarriers;
}

const Descriptor& RenderTarget_VK::GetRTV(uint16_t slice, uint8_t mipLevel) const
{
	assert(m_rtvs.size() > slice && m_rtvs[slice].size() > mipLevel);

	return m_rtvs[slice][mipLevel];
}

void RenderTarget_VK::GetAllocationInfo(uint64_t& byteSize, uint64_t& alignment, Device* pDevice, uint32_t width,
	uint32_t height, Format format, uint16_t arraySize, ResourceFlag resourceFlags, uint8_t numMips, uint8_t sampleCount,
	TextureLayout textureLayout, uint64_t requiredAlignment)
{
	Texture_VK::GetAllocationInfo(byteSize, alignment, pDevice, width, height, format, arraySize,
		resourceFlags | ResourceFlag::ALLOW_RENDER_TARGET, numMips, sampleCount, textureLayout, requiredAlignment);
}

//--------------------------------------------------------------------------------------
// Depth stencil
//--------------------------------------------------------------------------------------

DepthStencil_VK::DepthStencil_VK()
{
	m_stencilSrv = nullptr;
}

DepthStencil_VK::~DepthStencil_VK()
{
}

bool DepthStencil_VK::Create(Device* pDevice, uint32_t width, uint32_t height, Format format, ResourceFlag resourceFlags,
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

	const auto hasSRV = (resourceFlags & ResourceFlag::DENY_SHADER_RESOURCE) == ResourceFlag::NONE;
	const auto hasStencil = format == Format::D24_UNORM_S8_UINT || format == Format::D32_FLOAT_S8X24_UINT;
	m_dsvs.resize(arraySize);
	m_readOnlyDsvs.resize(arraySize);
	for (uint16_t slice = 0; slice < arraySize; ++slice)
	{
		m_dsvs[slice].resize(m_mipLevels);
		m_readOnlyDsvs[slice].resize(m_mipLevels);
		for (auto mip = 0u; mip < m_mipLevels; ++mip)
		{
			m_dsvs[slice][mip] = CreateDSV(1, slice, format, static_cast<uint8_t>(mip), sampleCount > 1, false, false);
			m_readOnlyDsvs[slice][mip] = hasSRV ?
				CreateDSV(1, slice, format, static_cast<uint8_t>(mip), sampleCount > 1, true, hasStencil) :
				m_dsvs[slice][mip];
		}
	}
	if (hasSRV && hasStencil)
		m_stencilSrv = CreateSRV(arraySize, 0, format, m_mipLevels, 0, sampleCount > 1, isCubeMap, stencilSrvComponentMapping, 1);

	return true;
}

bool DepthStencil_VK::Create(Device* pDevice, Heap* pHeap, uint64_t heapOffset, uint32_t width, uint32_t height,
	Format format, ResourceFlag resourceFlags, uint16_t arraySize, uint8_t numMips, uint8_t sampleCount, float clearDepth,
	uint8_t clearStencil, bool isCubeMap, const wchar_t* name, uint16_t srvComponentMapping, uint16_t stencilSrvComponentMapping,
	TextureLayout textureLayout, uint32_t maxThreads)
{
	(void)clearDepth;
	(void)clearStencil;
	mapFormat(format);
	resourceFlags = resourceFlags | ResourceFlag::ALLOW_DEPTH_STENCIL;
	XUSG_N_RETURN(Texture_VK::Create(pDevice, pHeap, heapOffset, width, height, format, arraySize, resourceFlags, numMips,
		sampleCount, isCubeMap, name, srvComponentMapping, textureLayout, 0, nullptr, maxThreads), false);

	const auto hasSRV = (resourceFlags & ResourceFlag::DENY_SHADER_RESOURCE) == ResourceFlag::NONE;
	const auto hasStencil = format == Format::D24_UNORM_S8_UINT || format == Format::D32_FLOAT_S8X24_UINT;
	m_dsvs.resize(arraySize);
	m_readOnlyDsvs.resize(arraySize);
	for (uint16_t slice = 0; slice < arraySize; ++slice)
	{
		m_dsvs[slice].resize(m_mipLevels);
		m_readOnlyDsvs[slice].resize(m_mipLevels);
		for (auto mip = 0u; mip < m_mipLevels; ++mip)
		{
			m_dsvs[slice][mip] = CreateDSV(1, slice, format, static_cast<uint8_t>(mip), sampleCount > 1, false, false);
			m_readOnlyDsvs[slice][mip] = hasSRV ?
				CreateDSV(1, slice, format, static_cast<uint8_t>(mip), sampleCount > 1, true, hasStencil) :
				m_dsvs[slice][mip];
		}
	}
	if (hasSRV && hasStencil)
		m_stencilSrv = CreateSRV(arraySize, 0, format, m_mipLevels, 0, sampleCount > 1, isCubeMap, stencilSrvComponentMapping, 1);

	return true;
}

bool DepthStencil_VK::CreateArray(Device* pDevice, uint32_t width, uint32_t height, uint16_t arraySize, Format format,
	ResourceFlag resourceFlags, uint8_t numMips, uint8_t sampleCount, float clearDepth, uint8_t clearStencil, bool isCubeMap,
	MemoryFlag memoryFlags, const wchar_t* name, uint16_t srvComponentMapping, uint16_t stencilSrvComponentMapping,
	TextureLayout textureLayout, uint32_t maxThreads)
{
	(void)clearDepth;
	(void)clearStencil;
	mapFormat(format);
	resourceFlags = resourceFlags | ResourceFlag::ALLOW_DEPTH_STENCIL;
	XUSG_N_RETURN(Texture_VK::Create(pDevice, width, height, format, arraySize, resourceFlags, numMips, sampleCount,
		isCubeMap, memoryFlags, name, srvComponentMapping, textureLayout, 0, nullptr, maxThreads), false);

	const auto hasSRV = (resourceFlags & ResourceFlag::DENY_SHADER_RESOURCE) == ResourceFlag::NONE;
	const auto hasStencil = format == Format::D24_UNORM_S8_UINT || format == Format::D32_FLOAT_S8X24_UINT;
	m_dsvs.resize(1);
	m_readOnlyDsvs.resize(1);
	m_dsvs[0].resize(m_mipLevels);
	m_readOnlyDsvs[0].resize(m_mipLevels);
	for (auto mip = 0u; mip < m_mipLevels; ++mip)
	{
		m_dsvs[0][mip] = CreateDSV(arraySize, 0, format, static_cast<uint8_t>(mip), sampleCount > 1, false, false);
		m_readOnlyDsvs[0][mip] = hasSRV ?
			CreateDSV(arraySize, 0, format, static_cast<uint8_t>(mip), sampleCount > 1, true, hasStencil) :
			m_dsvs[0][mip];
	}
	if (hasSRV && hasStencil)
		m_stencilSrv = CreateSRV(arraySize, 0, format, m_mipLevels, 0, sampleCount > 1, isCubeMap, stencilSrvComponentMapping, 1);

	return true;
}

bool DepthStencil_VK::CreateArray(Device* pDevice, Heap* pHeap, uint64_t heapOffset, uint32_t width, uint32_t height,
	uint16_t arraySize, Format format, ResourceFlag resourceFlags, uint8_t numMips, uint8_t sampleCount, float clearDepth,
	uint8_t clearStencil, bool isCubeMap, const wchar_t* name, uint16_t srvComponentMapping, uint16_t stencilSrvComponentMapping,
	TextureLayout textureLayout, uint32_t maxThreads)
{
	(void)clearDepth;
	(void)clearStencil;
	mapFormat(format);
	resourceFlags = resourceFlags | ResourceFlag::ALLOW_DEPTH_STENCIL;
	XUSG_N_RETURN(Texture_VK::Create(pDevice, pHeap, heapOffset, width, height, format, arraySize, resourceFlags, numMips,
		sampleCount, isCubeMap, name, srvComponentMapping, textureLayout, 0, nullptr, maxThreads), false);

	const auto hasSRV = (resourceFlags & ResourceFlag::DENY_SHADER_RESOURCE) == ResourceFlag::NONE;
	const auto hasStencil = format == Format::D24_UNORM_S8_UINT || format == Format::D32_FLOAT_S8X24_UINT;
	m_dsvs.resize(1);
	m_readOnlyDsvs.resize(1);
	m_dsvs[0].resize(m_mipLevels);
	m_readOnlyDsvs[0].resize(m_mipLevels);
	for (auto mip = 0u; mip < m_mipLevels; ++mip)
	{
		m_dsvs[0][mip] = CreateDSV(arraySize, 0, format, static_cast<uint8_t>(mip), sampleCount > 1, false, false);
		m_readOnlyDsvs[0][mip] = hasSRV ?
			CreateDSV(arraySize, 0, format, static_cast<uint8_t>(mip), sampleCount > 1, true, hasStencil) :
			m_dsvs[0][mip];
	}
	if (hasSRV && hasStencil)
		m_stencilSrv = CreateSRV(arraySize, 0, format, m_mipLevels, 0, sampleCount > 1, isCubeMap, stencilSrvComponentMapping, 1);

	return true;
}

bool DepthStencil_VK::CreateResource(Device* pDevice, uint32_t width, uint32_t height, Format format, uint16_t arraySize,
	ResourceFlag resourceFlags, uint8_t numMips, uint8_t sampleCount, MemoryFlag memoryFlags, ResourceState initialResourceState,
	float clearDepth, uint8_t clearStencil, TextureLayout textureLayout, uint32_t maxThreads)
{
	(void)clearDepth;
	(void)clearStencil;
	mapFormat(format);

	return Texture_VK::CreateResource(pDevice, width, height, format, arraySize, resourceFlags | ResourceFlag::ALLOW_DEPTH_STENCIL,
		numMips, sampleCount, memoryFlags, initialResourceState, textureLayout, 0, nullptr, maxThreads);
}

bool DepthStencil_VK::CreateResource(Device* pDevice, Heap* pHeap, uint64_t heapOffset, uint32_t width,
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
	fillImageView(*view, viewType, GetVKFormat(m_pDevice, format == Format::UNKNOWN ? m_format : format), mipLevel, 1, firstArraySlice, arraySize,
		GetVKImageAspect(format == Format::UNKNOWN ? m_format : format), XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
		readOnlyDepth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
	view->Target.ReadOnly = readOnlyDepth ? 1 : 0;

	return view.get();
}

const Descriptor& DepthStencil_VK::GetDSV(uint16_t slice, uint8_t mipLevel, bool readOnly) const
{
	const auto& dsvs = readOnly ? m_readOnlyDsvs : m_dsvs;
	assert(dsvs.size() > slice && dsvs[slice].size() > mipLevel);

	return dsvs[slice][mipLevel];
}

const Descriptor& DepthStencil_VK::GetSRV(uint8_t firstLevel, bool singleLevel, bool stencil) const
{
	if (stencil) return m_stencilSrv;

	return Texture_VK::GetSRV(firstLevel, singleLevel);
}

void DepthStencil_VK::GetAllocationInfo(uint64_t& byteSize, uint64_t& alignment, Device* pDevice, uint32_t width,
	uint32_t height, Format format, uint16_t arraySize, ResourceFlag resourceFlags, uint8_t numMips, uint8_t sampleCount,
	TextureLayout textureLayout, uint64_t requiredAlignment)
{
	Texture_VK::GetAllocationInfo(byteSize, alignment, pDevice, width, height, format, arraySize,
		resourceFlags | ResourceFlag::ALLOW_DEPTH_STENCIL, numMips, sampleCount, textureLayout, requiredAlignment);
}

void DepthStencil_VK::mapFormat(Format& format)
{
	// Same default as DepthStencil_DX12: an unknown depth format is D24S8.
	// GetVKFormat substitutes D32S8 when the device cannot attach D24S8.
	switch (format)
	{
	case Format::D32_FLOAT_S8X24_UINT:
	case Format::R32_FLOAT_X8X24_TYPELESS:
	case Format::R32G8X24_TYPELESS:
	case Format::X32_TYPELESS_G8X24_UINT:
		format = Format::D32_FLOAT_S8X24_UINT;
		break;
	case Format::D16_UNORM:
	case Format::R16_UNORM:
	case Format::R16_TYPELESS:
		format = Format::D16_UNORM;
		break;
	case Format::D32_FLOAT:
	case Format::R32_FLOAT:
	case Format::R32_TYPELESS:
		format = Format::D32_FLOAT;
		break;
	default:
		format = Format::D24_UNORM_S8_UINT;
	}

	m_format = format;
}

//--------------------------------------------------------------------------------------

Texture3D_VK::Texture3D_VK()
{
}

Texture3D_VK::~Texture3D_VK()
{
}

bool Texture3D_VK::Create(Device* pDevice, uint32_t width, uint32_t height, uint16_t depth, Format format,
	ResourceFlag resourceFlags, uint8_t numMips, MemoryFlag memoryFlags, const wchar_t* name, uint16_t srvComponentMapping,
	TextureLayout textureLayout, uint8_t numUavFormats, const Format* uavFormats, uint32_t maxThreads)
{
	(void)memoryFlags;
	(void)textureLayout;
	XUSG_N_RETURN(initialize(pDevice, maxThreads), false);
	m_format = format;

	const auto hasSRV = (resourceFlags & ResourceFlag::DENY_SHADER_RESOURCE) == ResourceFlag::NONE;
	const auto hasUAV = (resourceFlags & ResourceFlag::ALLOW_UNORDERED_ACCESS) == ResourceFlag::ALLOW_UNORDERED_ACCESS;
	const auto fullMips = CalculateMipLevels(width, height);
	numMips = numMips ? (min)(numMips, fullMips) : fullMips;
	numMips = (max)(numMips, static_cast<uint8_t>(1));

	XUSG_N_RETURN(createImage(nullptr, 0, width, height, depth, format, 1, resourceFlags, numMips, 1, MemoryType::DEFAULT, true, false,
		numUavFormats, uavFormats), false);
	setState(ResourceState::COMMON, numMips, maxThreads);
	SetName(name);
	if (hasSRV)
	{
		m_srvs.resize(numMips);
		for (auto i = 0u; i < numMips; ++i)
			m_srvs[i] = CreateSRV(format, static_cast<uint8_t>(numMips - i), static_cast<uint8_t>(i), srvComponentMapping);
	}
	if (hasUAV)
	{
		m_uavs.resize(numMips);
		for (auto i = 0u; i < numMips; ++i)
			m_uavs[i] = CreateUAV(static_cast<uint16_t>((max)(depth >> i, 1)), 0, format, static_cast<uint8_t>(i));
		if (numUavFormats && uavFormats)
		{
			for (auto i = 0u; i < numUavFormats; ++i)
			{
				auto& castable = m_castableUavs[uavFormats[i]];
				castable.resize(numMips);
				for (auto mip = 0u; mip < numMips; ++mip)
					castable[mip] = CreateUAV(static_cast<uint16_t>((max)(depth >> mip, 1)), 0, uavFormats[i], static_cast<uint8_t>(mip));
			}
		}
	}
	if (hasSRV && hasUAV)
	{
		m_singleLevelSrvs.resize(numMips);
		if (numMips <= 1 && !m_srvs.empty()) m_singleLevelSrvs[0] = m_srvs[0];
		else for (auto i = 0u; i < numMips; ++i)
			m_singleLevelSrvs[i] = CreateSRV(format, 1, static_cast<uint8_t>(i), srvComponentMapping);
	}

	return true;
}

bool Texture3D_VK::Create(Device* pDevice, Heap* pHeap, uint64_t heapOffset, uint32_t width, uint32_t height,
	uint16_t depth, Format format, ResourceFlag resourceFlags, uint8_t numMips, const wchar_t* name, uint16_t srvComponentMapping,
	TextureLayout textureLayout, uint8_t numUavFormats, const Format* uavFormats, uint32_t maxThreads)
{
	(void)textureLayout;
	XUSG_N_RETURN(initialize(pDevice, maxThreads), false);
	m_format = format;

	const auto hasSRV = (resourceFlags & ResourceFlag::DENY_SHADER_RESOURCE) == ResourceFlag::NONE;
	const auto hasUAV = (resourceFlags & ResourceFlag::ALLOW_UNORDERED_ACCESS) == ResourceFlag::ALLOW_UNORDERED_ACCESS;
	const auto fullMips = CalculateMipLevels(width, height);
	numMips = numMips ? (min)(numMips, fullMips) : fullMips;
	numMips = (max)(numMips, static_cast<uint8_t>(1));

	XUSG_N_RETURN(createImage(pHeap, heapOffset, width, height, depth, format, 1, resourceFlags, numMips, 1,
		MemoryType::DEFAULT, true, false, numUavFormats, uavFormats, pHeap == nullptr), false);
	setState(ResourceState::COMMON, numMips, maxThreads);
	SetName(name);
	if (hasSRV)
	{
		m_srvs.resize(numMips);
		for (auto i = 0u; i < numMips; ++i)
			m_srvs[i] = CreateSRV(format, static_cast<uint8_t>(numMips - i), static_cast<uint8_t>(i), srvComponentMapping);
	}
	if (hasUAV)
	{
		m_uavs.resize(numMips);
		for (auto i = 0u; i < numMips; ++i)
			m_uavs[i] = CreateUAV(static_cast<uint16_t>((max)(depth >> i, 1)), 0, format, static_cast<uint8_t>(i));
	}

	return true;
}

bool Texture3D_VK::CreateResource(Device* pDevice, uint32_t width, uint32_t height, uint16_t depth, Format format,
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

bool Texture3D_VK::CreateResource(Device* pDevice, Heap* pHeap, uint64_t heapOffset, uint32_t width, uint32_t height,
	uint16_t depth, Format format, ResourceFlag resourceFlags, uint8_t numMips, ResourceState initialResourceState,
	TextureLayout textureLayout, uint8_t numCastableFormats, const Format* pCastableFormats, uint32_t maxThreads)
{
	(void)textureLayout;
	XUSG_N_RETURN(initialize(pDevice, maxThreads), false);
	m_format = format;
	XUSG_N_RETURN(createImage(pHeap, heapOffset, width, height, depth, format, 1, resourceFlags, numMips, 1,
		MemoryType::DEFAULT, true, false, numCastableFormats, pCastableFormats, pHeap == nullptr), false);
	setState(initialResourceState, numMips, maxThreads);

	return true;
}

Descriptor Texture3D_VK::CreateSRV(Format format, uint8_t numMips, uint8_t mostDetailedMip, uint16_t srvComponentMapping)
{
	auto view = storeView(ResourceViewType::SRV, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, true, false);
	fillImageView(*view, VK_IMAGE_VIEW_TYPE_3D, GetVKFormat(m_pDevice, format == Format::UNKNOWN ? m_format : format), mostDetailedMip, numMips,
		0, 1, VK_IMAGE_ASPECT_COLOR_BIT, srvComponentMapping, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);

	return view.get();
}

Descriptor Texture3D_VK::CreateUAV(uint16_t wSize, uint16_t firstWSlice, Format format, uint8_t mipLevel)
{
	(void)firstWSlice;
	(void)wSize;
	auto view = storeView(ResourceViewType::UAV, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, true, false);
	fillImageView(*view, VK_IMAGE_VIEW_TYPE_3D, GetVKFormat(m_pDevice, format == Format::UNKNOWN ? m_format : format), mipLevel, 1,
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

void Texture3D_VK::GetAllocationInfo(uint64_t& byteSize, uint64_t& alignment, Device* pDevice, uint32_t width,
	uint32_t height, uint16_t depth, Format format, ResourceFlag resourceFlags, uint8_t numMips,
	TextureLayout textureLayout, uint64_t requiredAlignment)
{
	(void)depth;
	Texture_VK::GetAllocationInfo(byteSize, alignment, pDevice, width, height, format, 1, resourceFlags, numMips, 1, textureLayout, requiredAlignment);
}

//--------------------------------------------------------------------------------------
// Raw buffer
//--------------------------------------------------------------------------------------

Buffer_VK::Buffer_VK()
{
}

Buffer_VK::~Buffer_VK()
{
}

bool Buffer_VK::Create(Device* pDevice, size_t byteWidth, ResourceFlag resourceFlags, MemoryType memoryType,
	uint32_t numSRVs, const uintptr_t* firstSrvElements, uint32_t numUAVs, const uintptr_t* firstUavElements,
	MemoryFlag memoryFlags, const wchar_t* name, const uintptr_t* counterByteOffsets, uint32_t maxThreads)
{
	XUSG_N_RETURN(CreateResource(pDevice, byteWidth, resourceFlags, memoryType, memoryFlags,
		ResourceState::COMMON, 0, nullptr, maxThreads), false);
	SetName(name);
	const auto stride = 4u;
	createViews(byteWidth / stride, stride, Format::R32_TYPELESS, numSRVs, firstSrvElements,
		numUAVs, firstUavElements, counterByteOffsets, XUSG_DEFAULT_SRV_COMPONENT_MAPPING);

	return true;
}

bool Buffer_VK::Create(Device* pDevice, Heap* pHeap, uint64_t heapOffset, size_t byteWidth, ResourceFlag resourceFlags,
	uint32_t numSRVs, const uintptr_t* firstSrvElements, uint32_t numUAVs, const uintptr_t* firstUavElements,
	const wchar_t* name, const uintptr_t* counterByteOffsets, uint32_t maxThreads)
{
	XUSG_N_RETURN(CreateResource(pDevice, pHeap, heapOffset, byteWidth, resourceFlags, ResourceState::COMMON, 0, nullptr, maxThreads), false);
	SetName(name);
	createViews(byteWidth / 4, 4, Format::R32_TYPELESS, numSRVs, firstSrvElements, numUAVs, firstUavElements, counterByteOffsets,
		XUSG_DEFAULT_SRV_COMPONENT_MAPPING);

	return true;
}

bool Buffer_VK::CreateResource(Device* pDevice, size_t byteWidth, ResourceFlag resourceFlags, MemoryType memoryType,
	MemoryFlag memoryFlags, ResourceState initialResourceState, uint8_t numCastableFormats, const Format* pCastableFormats, uint32_t maxThreads)
{
	(void)memoryFlags;
	(void)numCastableFormats;
	(void)pCastableFormats;
	XUSG_N_RETURN(initialize(pDevice, maxThreads), false);
	XUSG_N_RETURN(createBuffer(nullptr, 0, byteWidth, resourceFlags, memoryType), false);
	const auto isRaytracingAS = (resourceFlags & ResourceFlag::ACCELERATION_STRUCTURE) == ResourceFlag::ACCELERATION_STRUCTURE;
	setState(isRaytracingAS ? ResourceState::RAYTRACING_ACCELERATION_STRUCTURE : initialResourceState, 1, maxThreads);
	m_format = Format::UNKNOWN;

	return true;
}

bool Buffer_VK::CreateResource(Device* pDevice, Heap* pHeap, uint64_t heapOffset, size_t byteWidth,
	ResourceFlag resourceFlags, ResourceState initialResourceState, uint8_t numCastableFormats, const Format* pCastableFormats, uint32_t maxThreads)
{
	(void)numCastableFormats;
	(void)pCastableFormats;
	XUSG_N_RETURN(initialize(pDevice, maxThreads), false);
	XUSG_N_RETURN(createBuffer(pHeap, heapOffset, byteWidth, resourceFlags, MemoryType::DEFAULT, pHeap == nullptr), false);
	const auto isRaytracingAS = (resourceFlags & ResourceFlag::ACCELERATION_STRUCTURE) == ResourceFlag::ACCELERATION_STRUCTURE;
	setState(isRaytracingAS ? ResourceState::RAYTRACING_ACCELERATION_STRUCTURE : initialResourceState, 1, maxThreads);

	return true;
}

bool Buffer_VK::Upload(CommandList* pCommandList, Resource* pUploader, const void* pData, size_t size,
	size_t offset, ResourceState dstState, uint32_t threadIdx)
{
	const auto pVkUploader = AsVK(pUploader);
	assert(pVkUploader);
	XUSG_N_RETURN(pVkUploader->CreateUploadBuffer(m_pDevice, size), false);
	memcpy(pVkUploader->GetMappedData(), pData, size);
	ResourceBarrier barrier;
	auto numBarriers = SetBarrier(&barrier, ResourceState::COPY_DEST, 0, 0, BarrierFlag::NONE, ResourceState::AUTO, threadIdx);
	if (numBarriers) pCommandList->Barrier(numBarriers, &barrier);
	pCommandList->CopyBufferRegion(this, offset, pVkUploader, 0, size);
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
	if (!m_pDataBegin && m_memory != VK_NULL_HANDLE) vkMapMemory(m_pDevice->GetDevice(), m_memory, 0, m_byteSize, 0, &m_pDataBegin);
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

void Buffer_VK::GetAllocationInfo(uint64_t& byteSize, uint64_t& alignment, Device* pDevice, size_t byteWidth,
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
	assert(next >= firstElement);
	viewElements = static_cast<uint32_t>(next - firstElement);
}

//--------------------------------------------------------------------------------------
// Structured buffer
//--------------------------------------------------------------------------------------

StructuredBuffer_VK::StructuredBuffer_VK()
{
}

StructuredBuffer_VK::~StructuredBuffer_VK()
{
}

bool StructuredBuffer_VK::Create(Device* pDevice, size_t numElements, uint32_t byteStride, ResourceFlag resourceFlags,
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

bool StructuredBuffer_VK::Create(Device* pDevice, Heap* pHeap, uint64_t heapOffset, size_t numElements,
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
// Typed buffer
//--------------------------------------------------------------------------------------

TypedBuffer_VK::TypedBuffer_VK()
{
	m_bufferType = BufferType::TYPED;
}

TypedBuffer_VK::~TypedBuffer_VK()
{
}

bool TypedBuffer_VK::Create(Device* pDevice, size_t numElements, uint32_t byteStride, Format format, ResourceFlag resourceFlags,
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

bool TypedBuffer_VK::Create(Device* pDevice, Heap* pHeap, uint64_t heapOffset, size_t numElements, uint32_t byteStride,
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
// Vertex buffer
//--------------------------------------------------------------------------------------

VertexBuffer_VK::VertexBuffer_VK()
{
	m_bufferType = BufferType::VERTEX;
}

VertexBuffer_VK::~VertexBuffer_VK()
{
}

bool VertexBuffer_VK::Create(Device* pDevice, size_t numVertices, uint32_t byteStride, ResourceFlag resourceFlags,
	MemoryType memoryType, uint32_t numVBVs, const uintptr_t* firstVertices, uint32_t numSRVs, const uintptr_t* firstSrvElements,
	uint32_t numUAVs, const uintptr_t* firstUavElements, MemoryFlag memoryFlags, const wchar_t* name,
	const uintptr_t* counterByteOffsets, uint32_t maxThreads)
{
	XUSG_N_RETURN(StructuredBuffer_VK::Create(pDevice, numVertices, byteStride, resourceFlags, memoryType, numSRVs, firstSrvElements,
		numUAVs, firstUavElements, memoryFlags, name, counterByteOffsets, maxThreads), false);
	createVBVs(numVertices, byteStride, numVBVs, firstVertices);

	return true;
}

bool VertexBuffer_VK::Create(Device* pDevice, Heap* pHeap, uint64_t heapOffset, size_t numVertices, uint32_t byteStride,
	ResourceFlag resourceFlags, uint32_t numVBVs, const uintptr_t* firstVertices, uint32_t numSRVs, const uintptr_t* firstSrvElements,
	uint32_t numUAVs, const uintptr_t* firstUavElements, const wchar_t* name, const uintptr_t* counterByteOffsets, uint32_t maxThreads)
{
	XUSG_N_RETURN(StructuredBuffer_VK::Create(pDevice, pHeap, heapOffset, numVertices, byteStride, resourceFlags, numSRVs,
		firstSrvElements, numUAVs, firstUavElements, name, counterByteOffsets, maxThreads), false);
	createVBVs(numVertices, byteStride, numVBVs, firstVertices);

	return true;
}

bool VertexBuffer_VK::CreateAsRaw(Device* pDevice, size_t numVertices, uint32_t byteStride, ResourceFlag resourceFlags,
	MemoryType memoryType, uint32_t numVBVs, const uintptr_t* firstVertices, uint32_t numSRVs, const uintptr_t* firstSrvElements,
	uint32_t numUAVs, const uintptr_t* firstUavElements, MemoryFlag memoryFlags, const wchar_t* name,
	const uintptr_t* counterByteOffsets, uint32_t maxThreads)
{
	XUSG_N_RETURN(Buffer_VK::Create(pDevice, numVertices * byteStride, resourceFlags, memoryType, numSRVs, firstSrvElements,
		numUAVs, firstUavElements, memoryFlags, name, counterByteOffsets, maxThreads), false);
	createVBVs(numVertices, byteStride, numVBVs, firstVertices);

	return true;
}

bool VertexBuffer_VK::CreateAsRaw(Device* pDevice, Heap* pHeap, uint64_t heapOffset, size_t numVertices, uint32_t byteStride,
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
	vbv.pResource = this;
	vbv.ByteOffset = byteStride * firstVertex;
	vbv.ByteStride = byteStride;
	vbv.ByteSize = byteStride * numVertices;
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
// Index buffer
//--------------------------------------------------------------------------------------

IndexBuffer_VK::IndexBuffer_VK()
{
	m_bufferType = BufferType::INDEX;
}

IndexBuffer_VK::~IndexBuffer_VK()
{
}

bool IndexBuffer_VK::Create(Device* pDevice, size_t byteWidth, Format format, ResourceFlag resourceFlags, MemoryType memoryType,
	uint32_t numIBVs, const uintptr_t* ibvByteOffsets, uint32_t numSRVs, const uintptr_t* firstSrvElements, uint32_t numUAVs,
	const uintptr_t* firstUavElements, MemoryFlag memoryFlags, const wchar_t* name, uint16_t srvComponentMapping,
	const uintptr_t* counterByteOffsets, uint32_t maxThreads)
{
	const auto stride = format == Format::R16_UINT ? 2u : 4u;
	XUSG_N_RETURN(TypedBuffer_VK::Create(pDevice, byteWidth / stride, stride, format, resourceFlags, memoryType, numSRVs, firstSrvElements,
		numUAVs, firstUavElements, memoryFlags, name, srvComponentMapping, counterByteOffsets, 0, nullptr, maxThreads), false);
	m_ibvs.resize(numIBVs);
	for (auto i = 0u; i < numIBVs; ++i)
	{
		const auto byteOffset = ibvByteOffsets ? ibvByteOffsets[i] : 0;
		const auto byteSize = static_cast<uint32_t>((!ibvByteOffsets || i + 1 >= numIBVs ?
			byteWidth : ibvByteOffsets[i + 1]) - byteOffset);
		CreateIBV(m_ibvs[i], format, byteSize, byteOffset);
	}

	return true;
}

bool IndexBuffer_VK::Create(Device* pDevice, Heap* pHeap, uint64_t heapOffset, size_t byteWidth, Format format,
	ResourceFlag resourceFlags, uint32_t numIBVs, const uintptr_t* ibvByteOffsets, uint32_t numSRVs, const uintptr_t* firstSrvElements,
	uint32_t numUAVs, const uintptr_t* firstUavElements, const wchar_t* name, uint16_t srvComponentMapping,
	const uintptr_t* counterByteOffsets, uint32_t maxThreads)
{
	const auto stride = format == Format::R16_UINT ? 2u : 4u;
	XUSG_N_RETURN(TypedBuffer_VK::Create(pDevice, pHeap, heapOffset, byteWidth / stride, stride, format, resourceFlags, numSRVs,
		firstSrvElements, numUAVs, firstUavElements, name, srvComponentMapping, counterByteOffsets, 0, nullptr, maxThreads), false);
	m_ibvs.resize(numIBVs);
	for (auto i = 0u; i < numIBVs; ++i)
	{
		const auto byteOffset = ibvByteOffsets ? ibvByteOffsets[i] : 0;
		const auto byteSize = static_cast<uint32_t>((!ibvByteOffsets || i + 1 >= numIBVs ?
			byteWidth : ibvByteOffsets[i + 1]) - byteOffset);
		CreateIBV(m_ibvs[i], format, byteSize, byteOffset);
	}

	return true;
}

void IndexBuffer_VK::CreateIBV(IndexBufferView& ibv, Format format, uint32_t byteSize, size_t byteOffset)
{
	ibv.pResource = this;
	ibv.ByteOffset = byteOffset;
	ibv.ByteSize = byteSize;
	ibv.Format = format;
}

const IndexBufferView& IndexBuffer_VK::GetIBV(uint32_t index) const
{
	return m_ibvs[index];
}
