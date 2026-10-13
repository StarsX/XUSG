//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSG_VK.h"

namespace XUSG
{
	struct ResourceView_VK : DescriptorHeader
	{
		// RTV/DSV data is created once and copied into VkRenderingAttachmentInfo.
		// It overlays the shader-view fields so the descriptor stays the size of a sampled view.
		struct Attachment
		{
			VkRenderingAttachmentInfo Rendering;
			uint32_t Width;
			uint32_t Height;
			uint16_t BaseMip;
			uint16_t MipCount;
			uint16_t BaseLayer;
			uint16_t Layers;
			uint32_t Subresource;
			uint8_t SingleSubresource;
			uint8_t HasStencil;
		};

		union
		{
			struct
			{
				VkImageViewCreateInfo ViewInfo;
				VkDeviceAddress Address;
				VkDeviceSize Range;
				uint32_t Stride;
				VkDescriptorType VkType;
				VkImageLayout Layout;
				uint8_t IsImage;
				uint8_t IsTexelBuffer;
			};

			Attachment Target;
		};
	};

	static_assert(sizeof(ResourceView_VK::Attachment) <= sizeof(VkImageViewCreateInfo) + sizeof(VkDeviceAddress) +
		sizeof(VkDeviceSize) + sizeof(uint32_t) * 3 + 8,
		"RTV/DSV attachment data must fit in the shader-view side of the descriptor union");

	//--------------------------------------------------------------------------------------
	// Heap
	//--------------------------------------------------------------------------------------
	class Heap_VK :
		public virtual Heap
	{
	public:
		Heap_VK();
		virtual ~Heap_VK();

		bool Create(Device* pDevice, uint64_t byteSize, MemoryType memoryType,
			MemoryFlag memoryFlags = MemoryFlag::NONE, uint64_t alignment = 0,
			const wchar_t* name = nullptr);

		uint64_t GetSize() const;
		uint64_t GetAlignment() const;
		MemoryType GetType() const;

		void Create(uint64_t nativeHandle, Device* pDevice);
		void SetName(const wchar_t* name);

		uint64_t GetNativeHandle() const;
		void* GetBackend() override;

		VkDeviceMemory GetMemory() const;
		uint32_t GetMemoryType() const;

	protected:
		Device_VK*		m_pDevice;
		VkDeviceMemory	m_memory;

		uint64_t	m_size;
		uint64_t	m_alignment;
		uint32_t	m_memoryTypeIndex;
		MemoryType	m_memoryType;

		std::wstring m_name;
	};

	//--------------------------------------------------------------------------------------
	// Resource
	//--------------------------------------------------------------------------------------
	class Resource_VK :
		public virtual Resource
	{
	public:
		Resource_VK();
		virtual ~Resource_VK();

		bool ReadFromSubresource(void* pDstData, uint32_t dstRowPitch, uint32_t dstDepthPitch,
			uint32_t srcSubresource, const BoxRange* pSrcBox = nullptr);
		bool WriteToSubresource(uint32_t dstSubresource, const void* pSrcData, uint32_t srcRowPitch,
			uint32_t srcDepthPitch, const BoxRange* pDstBox = nullptr);

		uint32_t SetBarrier(ResourceBarrier* pBarriers, ResourceState dstState,
			uint32_t numBarriers = 0, uint32_t subresource = XUSG_BARRIER_ALL_SUBRESOURCES,
			BarrierFlag flags = BarrierFlag::NONE, ResourceState srcState = ResourceState::AUTO,
			uint32_t threadIdx = 0);

		ResourceState Transition(ResourceState dstState, uint32_t subresource = XUSG_BARRIER_ALL_SUBRESOURCES,
			BarrierFlag flag = BarrierFlag::NONE, ResourceState srcState = ResourceState::AUTO, uint32_t threadIdx = 0);
		ResourceState GetResourceState(uint32_t subresource = 0, uint32_t threadIdx = 0) const;

		uint64_t GetWidth() const;
		uint64_t GetVirtualAddress(int offset = 0) const;

		void Unmap(const Range* pWrittenRange = nullptr);
		void GetTiling(uint32_t* pTotalNumTiles, PackedMipInfo* pPackedMipInfo = nullptr,
			TileShape* pStandardTileShapeForNonPackedMips = nullptr, uint32_t* pNumSubresourceTilings = nullptr,
			uint32_t firstSubresourceTiling = 0, SubresourceTiling* pSubresourceTilingsForNonPackedMips = nullptr) const;

		struct SparseTileBind
		{
			uint32_t Tile;
			VkDeviceMemory Memory;
			VkDeviceSize Offset;
		};

		bool IsSparse() const;
		bool ImageTileCopy(const TiledResourceCoord& coord, const TileRegionSize& region, uint32_t index,
			VkDeviceSize bufferOffset, VkBufferImageCopy& copy) const;
		bool BufferTileCopy(uint32_t startTile, uint32_t numTiles, VkDeviceSize& offset, VkDeviceSize& size) const;
		void BindTileMappings(VkQueue queue, uint32_t numResourceRegions,
			const TiledResourceCoord* pResourceRegionStartCoords, const TileRegionSize* pResourceRegionSizes,
			Heap* pHeap, uint32_t numHeapRanges, const TileRangeFlag* pHeapRangeFlags,
			const uint32_t* pHeapRangeStartOffsets, const uint32_t* pHeapRangeTileCounts);
		void CopyBoundTiles(VkQueue queue, const TiledResourceCoord& dstCoord, const Resource_VK* pSrc,
			const TiledResourceCoord& srcCoord, const TileRegionSize& region);

		void Create(uint64_t nativeHandle, Device* pDevice, uint32_t maxThreads = 1);
		void SetName(const wchar_t* name);

		uint64_t GetNativeHandle() const;
		void* GetBackend() override;

		bool Is3D() const;
		bool IsImage() const;
		VkImage GetImage() const;
		VkBuffer GetBuffer() const;
		VkFormat GetVkFormat() const;
		uint32_t GetHeight() const;
		uint32_t GetDepthOrArray() const;
		uint32_t GetMipLevels() const;
		uint32_t GetSampleCount() const;
		void WrapImage(VkDevice device, VkImage image, VkFormat format, uint32_t width, uint32_t height,
			bool presentable = true);
		VkImageLayout ImageLayout(ResourceState state) const;
		bool IsPresentable() const { return m_presentable; }
		VkImageLayout GetCommonLayout() const { return m_commonLayout; }
		uint64_t GetByteSize() const;
		bool IsUndefinedLayout(uint32_t subresource) const;
		void Discard(uint32_t firstSubresource, uint32_t numSubresources);
		bool HasPendingDiscard() const;
		VkAttachmentLoadOp ConsumeDiscardLoadOp(uint32_t subresource);
		VkAttachmentLoadOp ConsumeDiscardLoadOp(uint32_t baseMip, uint32_t mipCount, uint32_t baseLayer, uint32_t layerCount);
		void AppendImageBarrier(VkImageMemoryBarrier2 image, std::vector<VkImageMemoryBarrier2>& barriers);
		bool CreateUploadBuffer(Device* pDevice, size_t byteSize);
		void* GetMappedData() const;

	protected:
		bool initialize(Device* pDevice, uint32_t maxThreads);
		bool allocateMemory(VkMemoryRequirements requirements, MemoryType memoryType, bool hostVisible);
		void setState(ResourceState state, uint32_t subresources, uint32_t maxThreads);
		bool createBuffer(Heap* pHeap, uint64_t heapOffset, size_t byteWidth,
			ResourceFlag resourceFlags, MemoryType memoryType, bool reserved = false);
		bool createImage(Heap* pHeap, uint64_t heapOffset, uint32_t width, uint32_t height,
			uint32_t depth, Format format, uint16_t arraySize, ResourceFlag resourceFlags, uint8_t numMips,
			uint8_t sampleCount, MemoryType memoryType, bool is3D, bool cube,
			uint8_t numCastableFormats = 0, const Format* pCastableFormats = nullptr, bool reserved = false);
		bool finishSparseImage();
		void releaseSparseMetadata();
		void ensureTiling() const;
		bool tileIndex(const TiledResourceCoord& coord, const TileRegionSize& region, uint32_t indexInRegion,
			uint32_t& tile) const;
		void bindTiles(VkQueue queue, const std::vector<SparseTileBind>& binds);
		std::shared_ptr<ResourceView_VK> storeView(ResourceViewType type, VkDescriptorType vkType, bool image, bool texel);
		uint32_t subresourceCount() const;
		uint32_t subresourceIndex(uint32_t mip, uint32_t layer) const;
		bool isDiscarded(uint32_t subresource) const;
		void consumeDiscarded(uint32_t subresource);
		bool needsUndefinedLayout(uint32_t subresource) const;
		void consumeLayoutMark(uint32_t subresource);
		void discardRange(const VkImageSubresourceRange& range, uint32_t& baseMip, uint32_t& mipCount,
			uint32_t& baseLayer, uint32_t& layerCount) const;

		Device_VK*		m_pDevice;
		VkImage			m_image;
		VkBuffer		m_buffer;
		VkDeviceMemory	m_memory;
		VkDeviceAddress	m_address;
		void*			m_pDataBegin;

		bool m_is3D;
		bool m_isImage;
		bool m_ownsImage;
		bool m_ownsBuffer;
		bool m_ownsMemory;
		bool m_hasPromotion;
		bool m_presentable;
		VkImageLayout m_commonLayout;

		uint64_t m_byteSize;
		uint64_t m_width;
		uint32_t m_height;
		uint32_t m_depthOrArray;
		uint32_t m_mipLevels;
		uint32_t m_sampleCount;
		VkFormat m_vkFormat;

		std::vector<std::vector<ResourceState>> m_states;
		std::vector<uint8_t> m_undefined;
		std::vector<uint8_t> m_discarded;
		uint32_t m_pendingLayout;
		uint32_t m_discardCount;
		std::vector<std::shared_ptr<ResourceView_VK>> m_resourceViews;

		std::wstring m_name;

		struct TileSubresource
		{
			uint32_t WidthInTiles;
			uint32_t HeightInTiles;
			uint32_t DepthInTiles;
			uint32_t StartTile;
			bool Packed;
		};

		struct SparseTile
		{
			bool Opaque;
			uint32_t SubresourceIndex;
			VkImageSubresource Subresource;
			VkOffset3D Offset;
			VkExtent3D Extent;
			VkDeviceSize ResourceOffset;
			VkDeviceSize Size;
			VkDeviceMemory Memory;
			VkDeviceSize MemoryOffset;
		};

		bool m_sparse;
		bool m_hasSparseInfo;
		Format m_resourceFormat;
		VkDeviceMemory m_metadata;
		uint32_t m_sparseTypeBits;
		VkSparseImageMemoryRequirements m_sparseInfo;

		mutable bool m_tilingReady;
		mutable uint32_t m_totalTiles;
		mutable bool m_singleMipTail;
		mutable PackedMipInfo m_packed;
		mutable TileShape m_tileShape;
		mutable std::vector<TileSubresource> m_subresources;
		mutable std::vector<uint32_t> m_nonPacked;
		mutable std::vector<SparseTile> m_tiles;
	};

	//--------------------------------------------------------------------------------------
	// Constant buffer
	//--------------------------------------------------------------------------------------
	class ConstantBuffer_VK :
		public virtual ConstantBuffer,
		public virtual Resource_VK
	{
	public:
		ConstantBuffer_VK();
		virtual ~ConstantBuffer_VK();

		using Resource_VK::Create;
		bool Create(Device* pDevice, size_t byteWidth, uint32_t numCBVs = 1,
			const size_t* cbvByteOffsets = nullptr, MemoryType memoryType = MemoryType::UPLOAD,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr);
		bool Create(Device* pDevice, Heap* pHeap, uint64_t heapOffset, size_t byteWidth,
			uint32_t numCBVs = 1, const size_t* cbvByteOffsets = nullptr, const wchar_t* name = nullptr);
		bool CreateResource(Device* pDevice, size_t byteWidth, MemoryType memoryType = MemoryType::UPLOAD,
			MemoryFlag memoryFlags = MemoryFlag::NONE, ResourceState initialResourceState = ResourceState::COMMON);
		bool CreateResource(Device* pDevice, Heap* pHeap, uint64_t heapOffset, size_t byteWidth,
			ResourceState initialResourceState = ResourceState::COMMON);
		bool Upload(CommandList* pCommandList, Resource* pUploader, const void* pData,
			size_t size, size_t byteOffset = 0, ResourceState srcState = ResourceState::COMMON,
			ResourceState dstState = ResourceState::COMMON);
		bool Upload(CommandList* pCommandList, uint32_t cbvIndex, Resource* pUploader,
			const void* pData, size_t byteSize, ResourceState srcState = ResourceState::COMMON,
			ResourceState dstState = ResourceState::COMMON);

		Descriptor CreateCBV(uint32_t byteSize, size_t byteOffset = 0);

		void* Map(uint32_t cbvIndex = 0, uintptr_t readBegin = 0, uintptr_t readEnd = 0);
		void* Map(const Range* pReadRange, uint32_t cbvIndex = 0);

		const Descriptor& GetCBV(uint32_t index = 0) const;
		uint32_t GetCBVOffset(uint32_t index) const;
		size_t GetTiledResourceTileSize() const;

		static void GetAllocationInfo(uint64_t& byteSize, uint64_t& alignment,
			Device* pDevice, size_t byteWidth, uint64_t requiredAlignment);
		static size_t AlignCBV(size_t byteSize);

	protected:
		std::vector<Descriptor>	m_cbvs;
		std::vector<uintptr_t>	m_cbvByteOffsets;
	};

	//--------------------------------------------------------------------------------------
	// Resource base
	//--------------------------------------------------------------------------------------
	class ShaderResource_VK :
		public virtual ShaderResource,
		public virtual Resource_VK
	{
	public:
		ShaderResource_VK();
		virtual ~ShaderResource_VK();

		const Descriptor& GetSRV(uint32_t index = 0) const;
		Format GetFormat() const;

	protected:
		static bool checkCastableFormats(Format& format, uint8_t numUavFormats, const Format* uavFormats);
		static bool getTypelessFormat(Format& format);

		Format m_format;
		std::vector<Descriptor> m_srvs;
	};

	//--------------------------------------------------------------------------------------
	// Texture
	//--------------------------------------------------------------------------------------
	class Texture_VK :
		public virtual Texture,
		public virtual ShaderResource_VK
	{
	public:
		Texture_VK();
		virtual ~Texture_VK();

		using Resource_VK::Create;
		bool Create(Device* pDevice, uint32_t width, uint32_t height, Format format,
			uint16_t arraySize = 1, ResourceFlag resourceFlags = ResourceFlag::NONE,
			uint8_t numMips = 1, uint8_t sampleCount = 1, bool isCubeMap = false,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr,
			uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
			TextureLayout textureLayout = TextureLayout::UNKNOWN,
			uint8_t numUavFormats = 0, const Format* uavFormats = nullptr,
			uint32_t maxThreads = 1);
		bool Create(Device* pDevice, Heap* pHeap, uint64_t heapOffset,
			uint32_t width, uint32_t height, Format format, uint16_t arraySize = 1,
			ResourceFlag resourceFlags = ResourceFlag::NONE, uint8_t numMips = 1,
			uint8_t sampleCount = 1, bool isCubeMap = false, const wchar_t* name = nullptr,
			uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
			TextureLayout textureLayout = TextureLayout::UNKNOWN,
			uint8_t numUavFormats = 0, const Format* uavFormats = nullptr,
			uint32_t maxThreads = 1);
		bool CreateResource(Device* pDevice, uint32_t width, uint32_t height, Format format,
			uint16_t arraySize = 1, ResourceFlag resourceFlags = ResourceFlag::NONE, uint8_t numMips = 1,
			uint8_t sampleCount = 1, MemoryFlag memoryFlags = MemoryFlag::NONE,
			ResourceState initialResourceState = ResourceState::COMMON,
			TextureLayout textureLayout = TextureLayout::UNKNOWN, uint8_t numCastableFormats = 0,
			const Format* pCastableFormats = nullptr, uint32_t maxThreads = 1);
		bool CreateResource(Device* pDevice, Heap* pHeap, uint64_t heapOffset, uint32_t width,
			uint32_t height, Format format, uint16_t arraySize = 1, ResourceFlag resourceFlags = ResourceFlag::NONE,
			uint8_t numMips = 1, uint8_t sampleCount = 1, ResourceState initialResourceState = ResourceState::COMMON,
			TextureLayout textureLayout = TextureLayout::UNKNOWN, uint8_t numCastableFormats = 0,
			const Format* pCastableFormats = nullptr, uint32_t maxThreads = 1);
		bool Upload(CommandList* pCommandList, Resource* pUploader,
			const SubresourceData* pSubresourceData, uint32_t numSubresources = 1,
			ResourceState dstState = ResourceState::COMMON, uint32_t firstSubresource = 0,
			uint32_t threadIdx = 0);
		bool Upload(CommandList* pCommandList, Resource* pUploader, const void* pData,
			uint8_t byteStride = sizeof(float), ResourceState dstState = ResourceState::COMMON,
			uint32_t threadIdx = 0);
		bool ReadBack(CommandList* pCommandList, Buffer* pReadBuffer, uint32_t* pRowPitches = nullptr,
			uint32_t numSubresources = 1, uint32_t firstSubresource = 0, size_t offset = 0,
			ResourceState dstState = ResourceState::COMMON, uint32_t threadIdx = 0);

		Descriptor CreateSRV(uint16_t arraySize, uint16_t firstArraySlice = 0, Format format = Format::UNKNOWN,
			uint8_t numMips = 1, uint8_t mostDetailedMip = 0, bool multisamples = false, bool isCubeMap = false,
			uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING, uint8_t plane = 0);
		Descriptor CreateUAV(uint16_t arraySize, uint16_t firstArraySlice = 0, Format format = Format::UNKNOWN,
			uint8_t mipLevel = 0, uint8_t plane = 0);

		using ShaderResource_VK::SetBarrier;
		uint32_t SetBarrier(ResourceBarrier* pBarriers, uint8_t mipLevel, ResourceState dstState,
			uint32_t numBarriers = 0, uint16_t arraySlice = 0, BarrierFlag flags = BarrierFlag::NONE,
			ResourceState srcState = ResourceState::AUTO, uint8_t plane = 0, uint32_t threadIdx = 0);

		void Blit(const CommandList* pCommandList, uint32_t groupSizeX, uint32_t groupSizeY,
			uint32_t groupSizeZ, const DescriptorTable& uavSrvTable, uint32_t uavSrvSlot = 0,
			uint8_t mipLevel = 0, const DescriptorTable& srvTable = XUSG_NULL, uint32_t srvSlot = 0,
			const DescriptorTable& samplerTable = XUSG_NULL, uint32_t samplerSlot = 1,
			const Pipeline& pipeline = nullptr) const;
		uint32_t Blit(const CommandList* pCommandList, ResourceBarrier* pBarriers, uint32_t groupSizeX,
			uint32_t groupSizeY, uint32_t groupSizeZ, uint8_t mipLevel, int8_t srcMipLevel,
			ResourceState srcState, const DescriptorTable& uavSrvTable, uint32_t uavSrvSlot = 0,
			uint32_t numBarriers = 0, const DescriptorTable& srvTable = XUSG_NULL, uint32_t srvSlot = 0,
			uint16_t baseSlice = 0, uint16_t numSlices = 0, uint32_t threadIdx = 0);
		uint32_t GenerateMips(const CommandList* pCommandList, ResourceBarrier* pBarriers, uint32_t groupSizeX,
			uint32_t groupSizeY, uint32_t groupSizeZ, ResourceState dstState, const PipelineLayout& pipelineLayout,
			const Pipeline& pipeline, const DescriptorTable* pUavSrvTables, uint32_t uavSrvSlot = 0,
			const DescriptorTable& samplerTable = XUSG_NULL, uint32_t samplerSlot = 1, uint32_t numBarriers = 0,
			const DescriptorTable* pSrvTables = nullptr, uint32_t srvSlot = 0, uint8_t baseMip = 1,
			uint8_t numMips = 0, uint16_t baseSlice = 0, uint16_t numSlices = 0, uint32_t threadIdx = 0);

		const Descriptor& GetUAV(uint8_t level = 0, Format format = Format::UNKNOWN) const;
		const Descriptor& GetSRV(uint8_t firstLevel = 0, bool singleLevel = false) const;

		uint32_t	CalculateSubresource(uint8_t mipLevel, uint16_t arraySlice = 0, uint8_t plane = 0) const;
		uint32_t	GetHeight() const;
		uint16_t	GetArraySize() const;
		uint8_t		GetNumMips() const;
		size_t		GetRequiredIntermediateSize(uint32_t firstSubresource, uint32_t numSubresources) const;
		size_t		GetTiledResourceTileSize() const;

		void* Map(uint32_t subresource = 0, uintptr_t readBegin = 0, uintptr_t readEnd = 0);
		void* Map(const Range* pReadRange, uint32_t subresource = 0);

		static void GetAllocationInfo(uint64_t& byteSize, uint64_t& alignment, Device* pDevice,
			uint32_t width, uint32_t height, Format format, uint16_t arraySize = 1,
			ResourceFlag resourceFlags = ResourceFlag::NONE, uint8_t numMips = 1,
			uint8_t sampleCount = 1, TextureLayout textureLayout = TextureLayout::UNKNOWN,
			uint64_t requiredAlignment = 0);

	protected:
		bool createSRVs(uint16_t arraySize, Format format, uint8_t numMips,
			bool multisamples, bool isCubeMap, uint16_t srvComponentMapping);
		bool createSingleLevelSRVs(uint16_t arraySize, uint8_t numMips, Format format,
			bool multisamples, bool isCubeMap, uint16_t srvComponentMapping);
		bool createUAVs(uint16_t arraySize, Format format, uint8_t numMips);
		bool createCastableUAVs(uint16_t arraySize, Format format, uint8_t numMips);
		void fillImageView(ResourceView_VK& view, VkImageViewType viewType, VkFormat format,
			uint8_t baseMip, uint8_t mipCount, uint16_t baseLayer, uint16_t layerCount,
			VkImageAspectFlags aspect, uint16_t componentMapping, VkImageLayout layout, VkDescriptorType type);

		std::vector<Descriptor>	m_singleLevelSrvs;
		std::vector<Descriptor>	m_uavs;
		std::unordered_map<Format, std::vector<Descriptor>> m_castableUavs;
		bool m_cube;
	};

	//--------------------------------------------------------------------------------------
	// Render target
	//--------------------------------------------------------------------------------------
	class RenderTarget_VK :
		public virtual RenderTarget,
		public virtual Texture_VK
	{
	public:
		RenderTarget_VK();
		virtual ~RenderTarget_VK();

		using Resource_VK::Create;
		bool Create(Device* pDevice, uint32_t width, uint32_t height, Format format,
			uint16_t arraySize = 1, ResourceFlag resourceFlags = ResourceFlag::NONE, uint8_t numMips = 1,
			uint8_t sampleCount = 1, const float* pClearColor = nullptr, bool isCubeMap = false,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr,
			uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
			TextureLayout textureLayout = TextureLayout::UNKNOWN,
			uint8_t numUavFormats = 0, const Format* uavFormats = nullptr,
			uint32_t maxThreads = 1);
		bool Create(Device* pDevice, Heap* pHeap, uint64_t heapOffset, uint32_t width, uint32_t height,
			Format format, uint16_t arraySize = 1, ResourceFlag resourceFlags = ResourceFlag::NONE,
			uint8_t numMips = 1, uint8_t sampleCount = 1, const float* pClearColor = nullptr,
			bool isCubeMap = false, const wchar_t* name = nullptr,
			uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
			TextureLayout textureLayout = TextureLayout::UNKNOWN,
			uint8_t numUavFormats = 0, const Format* uavFormats = nullptr,
			uint32_t maxThreads = 1);
		bool CreateArray(Device* pDevice, uint32_t width, uint32_t height, uint16_t arraySize,
			Format format, ResourceFlag resourceFlags = ResourceFlag::NONE, uint8_t numMips = 1,
			uint8_t sampleCount = 1, const float* pClearColor = nullptr, bool isCubeMap = false,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr,
			uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
			TextureLayout textureLayout = TextureLayout::UNKNOWN,
			uint8_t numUavFormats = 0, const Format* uavFormats = nullptr,
			uint32_t maxThreads = 1);
		bool CreateArray(Device* pDevice, Heap* pHeap, uint64_t heapOffset, uint32_t width, uint32_t height,
			uint16_t arraySize, Format format, ResourceFlag resourceFlags = ResourceFlag::NONE, uint8_t numMips = 1,
			uint8_t sampleCount = 1, const float* pClearColor = nullptr, bool isCubeMap = false,
			const wchar_t* name = nullptr, uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
			TextureLayout textureLayout = TextureLayout::UNKNOWN,
			uint8_t numUavFormats = 0, const Format* uavFormats = nullptr,
			uint32_t maxThreads = 1);
		bool CreateFromSwapChain(Device* pDevice, SwapChain* pSwapChain,
			uint32_t bufferIndex, uint32_t maxThreads = 1);
		bool CreateResource(Device* pDevice, uint32_t width, uint32_t height,
			Format format, uint16_t arraySize = 1, ResourceFlag resourceFlags = ResourceFlag::NONE,
			uint8_t numMips = 1, uint8_t sampleCount = 1, MemoryFlag memoryFlags = MemoryFlag::NONE,
			ResourceState initialResourceState = ResourceState::COMMON, const float* pClearColor = nullptr,
			TextureLayout textureLayout = TextureLayout::UNKNOWN, uint8_t numCastableFormats = 0,
			const Format* pCastableFormats = nullptr, uint32_t maxThreads = 1);
		bool CreateResource(Device* pDevice, Heap* pHeap, uint64_t heapOffset, uint32_t width,
			uint32_t height, Format format, uint16_t arraySize = 1, ResourceFlag resourceFlags = ResourceFlag::NONE,
			uint8_t numMips = 1, uint8_t sampleCount = 1, ResourceState initialResourceState = ResourceState::COMMON,
			const float* pClearColor = nullptr, TextureLayout textureLayout = TextureLayout::UNKNOWN,
			uint8_t numCastableFormats = 0, const Format* pCastableFormats = nullptr, uint32_t maxThreads = 1);

		Descriptor CreateRTV(uint16_t arraySize, uint16_t firstArraySlice = 0,
			Format format = Format::UNKNOWN, uint8_t mipLevel = 0, bool multisamples = false);

		using Texture_VK::Blit;
		void Blit(const CommandList* pCommandList, const DescriptorTable& srcSrvTable,
			uint32_t srcSlot = 0, uint8_t mipLevel = 0, uint16_t baseSlice = 0,
			uint16_t numSlices = 0, const DescriptorTable& samplerTable = XUSG_NULL,
			uint32_t samplerSlot = 1, const Pipeline& pipeline = nullptr,
			uint32_t offsetForSliceId = 0, uint32_t cbSlot = 2) const;
		uint32_t Blit(const CommandList* pCommandList, ResourceBarrier* pBarriers, uint8_t mipLevel,
			int8_t srcMipLevel, ResourceState srcState, const DescriptorTable& srcSrvTable,
			uint32_t srcSlot = 0, uint32_t numBarriers = 0, uint16_t baseSlice = 0, uint16_t numSlices = 0,
			uint32_t offsetForSliceId = 0, uint32_t cbSlot = 2, uint32_t threadIdx = 0);
		using Texture_VK::GenerateMips;
		uint32_t GenerateMips(const CommandList* pCommandList, ResourceBarrier* pBarriers, ResourceState dstState,
			const PipelineLayout& pipelineLayout, const Pipeline& pipeline, const DescriptorTable* pSrcSrvTables,
			uint32_t srcSlot = 0, const DescriptorTable& samplerTable = XUSG_NULL, uint32_t samplerSlot = 1,
			uint32_t numBarriers = 0, uint8_t baseMip = 1, uint8_t numMips = 0, uint16_t baseSlice = 0,
			uint16_t numSlices = 0, uint32_t offsetForSliceId = 0, uint32_t cbSlot = 2, uint32_t threadIdx = 0);

		const Descriptor& GetRTV(uint16_t slice = 0, uint8_t mipLevel = 0) const;

		static void GetAllocationInfo(uint64_t& byteSize, uint64_t& alignment, Device* pDevice,
			uint32_t width, uint32_t height, Format format, uint16_t arraySize = 1,
			ResourceFlag resourceFlags = ResourceFlag::NONE, uint8_t numMips = 1,
			uint8_t sampleCount = 1, TextureLayout textureLayout = TextureLayout::UNKNOWN,
			uint64_t requiredAlignment = 0);

	protected:
		std::vector<std::vector<Descriptor>> m_rtvs;
	};

	//--------------------------------------------------------------------------------------
	// Depth stencil
	//--------------------------------------------------------------------------------------
	class DepthStencil_VK :
		public virtual DepthStencil,
		public virtual Texture_VK
	{
	public:
		DepthStencil_VK();
		virtual ~DepthStencil_VK();

		using Resource_VK::Create;
		bool Create(Device* pDevice, uint32_t width, uint32_t height,
			Format format = Format::UNKNOWN, ResourceFlag resourceFlags = ResourceFlag::NONE,
			uint16_t arraySize = 1, uint8_t numMips = 1, uint8_t sampleCount = 1,
			float clearDepth = 1.0f, uint8_t clearStencil = 0, bool isCubeMap = false,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr,
			uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
			uint16_t stencilSrvComponentMapping = XUSG_DEFAULT_STENCIL_SRV_COMPONENT_MAPPING,
			TextureLayout textureLayout = TextureLayout::UNKNOWN, uint32_t maxThreads = 1);
		bool Create(Device* pDevice, Heap* pHeap, uint64_t heapOffset, uint32_t width, uint32_t height,
			Format format = Format::UNKNOWN, ResourceFlag resourceFlags = ResourceFlag::NONE,
			uint16_t arraySize = 1, uint8_t numMips = 1, uint8_t sampleCount = 1,
			float clearDepth = 1.0f, uint8_t clearStencil = 0, bool isCubeMap = false,
			const wchar_t* name = nullptr, uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
			uint16_t stencilSrvComponentMapping = XUSG_DEFAULT_STENCIL_SRV_COMPONENT_MAPPING,
			TextureLayout textureLayout = TextureLayout::UNKNOWN, uint32_t maxThreads = 1);
		bool CreateArray(Device* pDevice, uint32_t width, uint32_t height, uint16_t arraySize,
			Format format = Format::UNKNOWN, ResourceFlag resourceFlags = ResourceFlag::NONE,
			uint8_t numMips = 1, uint8_t sampleCount = 1, float clearDepth = 1.0f, uint8_t clearStencil = 0,
			bool isCubeMap = false, MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr,
			uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
			uint16_t stencilSrvComponentMapping = XUSG_DEFAULT_STENCIL_SRV_COMPONENT_MAPPING,
			TextureLayout textureLayout = TextureLayout::UNKNOWN, uint32_t maxThreads = 1);
		bool CreateArray(Device* pDevice, Heap* pHeap, uint64_t heapOffset, uint32_t width, uint32_t height,
			uint16_t arraySize, Format format = Format::UNKNOWN, ResourceFlag resourceFlags = ResourceFlag::NONE,
			uint8_t numMips = 1, uint8_t sampleCount = 1, float clearDepth = 1.0f, uint8_t clearStencil = 0,
			bool isCubeMap = false, const wchar_t* name = nullptr,
			uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
			uint16_t stencilSrvComponentMapping = XUSG_DEFAULT_STENCIL_SRV_COMPONENT_MAPPING,
			TextureLayout textureLayout = TextureLayout::UNKNOWN, uint32_t maxThreads = 1);
		bool CreateResource(Device* pDevice, uint32_t width, uint32_t height,
			Format format, uint16_t arraySize = 1, ResourceFlag resourceFlags = ResourceFlag::NONE,
			uint8_t numMips = 1, uint8_t sampleCount = 1, MemoryFlag memoryFlags = MemoryFlag::NONE,
			ResourceState initialResourceState = ResourceState::DEPTH_WRITE, float clearDepth = 1.0f,
			uint8_t clearStencil = 0, TextureLayout textureLayout = TextureLayout::UNKNOWN,
			uint32_t maxThreads = 1);
		bool CreateResource(Device* pDevice, Heap* pHeap, uint64_t heapOffset, uint32_t width,
			uint32_t height, Format format, uint16_t arraySize = 1, ResourceFlag resourceFlags = ResourceFlag::NONE,
			uint8_t numMips = 1, uint8_t sampleCount = 1, ResourceState initialResourceState = ResourceState::DEPTH_WRITE,
			float clearDepth = 1.0f, uint8_t clearStencil = 0, TextureLayout textureLayout = TextureLayout::UNKNOWN,
			uint32_t maxThreads = 1);

		Descriptor CreateDSV(uint16_t arraySize, uint16_t firstArraySlice = 0, Format format = Format::UNKNOWN,
			uint8_t mipLevel = 0, bool multisamples = false, bool readOnlyDepth = false, bool readOnlyStencil = false);

		const Descriptor& GetDSV(uint16_t slice = 0, uint8_t mipLevel = 0, bool readOnly = false) const;
		const Descriptor& GetSRV(uint8_t firstLevel = 0, bool singleLevel = false, bool stencil = false) const;

		static void GetAllocationInfo(uint64_t& byteSize, uint64_t& alignment, Device* pDevice,
			uint32_t width, uint32_t height, Format format, uint16_t arraySize = 1,
			ResourceFlag resourceFlags = ResourceFlag::NONE, uint8_t numMips = 1,
			uint8_t sampleCount = 1, TextureLayout textureLayout = TextureLayout::UNKNOWN,
			uint64_t requiredAlignment = 0);

	protected:
		void mapFormat(Format& format);

		std::vector<std::vector<Descriptor>> m_dsvs;
		std::vector<std::vector<Descriptor>> m_readOnlyDsvs;
		Descriptor m_stencilSrv;
	};

	//--------------------------------------------------------------------------------------
	// 3D Texture
	//--------------------------------------------------------------------------------------
	class Texture3D_VK :
		public virtual Texture3D,
		public virtual Texture_VK
	{
	public:
		Texture3D_VK();
		virtual ~Texture3D_VK();

		using Resource_VK::Create;
		bool Create(Device* pDevice, uint32_t width, uint32_t height, uint16_t depth,
			Format format, ResourceFlag resourceFlags = ResourceFlag::NONE, uint8_t numMips = 1,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr,
			uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
			TextureLayout textureLayout = TextureLayout::UNKNOWN,
			uint8_t numUavFormats = 0, const Format* uavFormats = nullptr,
			uint32_t maxThreads = 1);
		bool Create(Device* pDevice, Heap* pHeap, uint64_t heapOffset, uint32_t width, uint32_t height,
			uint16_t depth, Format format, ResourceFlag resourceFlags = ResourceFlag::NONE, uint8_t numMips = 1,
			const wchar_t* name = nullptr, uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
			TextureLayout textureLayout = TextureLayout::UNKNOWN,
			uint8_t numUavFormats = 0, const Format* uavFormats = nullptr,
			uint32_t maxThreads = 1);
		bool CreateResource(Device* pDevice, uint32_t width, uint32_t height, uint16_t depth,
			Format format, ResourceFlag resourceFlags = ResourceFlag::NONE, uint8_t numMips = 1,
			MemoryFlag memoryFlags = MemoryFlag::NONE, ResourceState initialResourceState = ResourceState::COMMON,
			TextureLayout textureLayout = TextureLayout::UNKNOWN, uint8_t numCastableFormats = 0,
			const Format* pCastableFormats = nullptr, uint32_t maxThreads = 1);
		bool CreateResource(Device* pDevice, Heap* pHeap, uint64_t heapOffset, uint32_t width,
			uint32_t height, uint16_t depth, Format format, ResourceFlag resourceFlags = ResourceFlag::NONE,
			uint8_t numMips = 1, ResourceState initialResourceState = ResourceState::COMMON,
			TextureLayout textureLayout = TextureLayout::UNKNOWN,
			uint8_t numCastableFormats = 0, const Format* pCastableFormats = nullptr,
			uint32_t maxThreads = 1);

		Descriptor CreateSRV(Format format = Format::UNKNOWN, uint8_t numMips = 1, uint8_t mostDetailedMip = 0,
			uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING);
		Descriptor CreateUAV(uint16_t wSize, uint16_t firstWSlice = 0,
			Format format = Format::UNKNOWN, uint8_t mipLevel = 0);

		uint32_t CalculateSubresource(uint8_t mipLevel) const;
		uint16_t GetDepth() const;

		static void GetAllocationInfo(uint64_t& byteSize, uint64_t& alignment, Device* pDevice,
			uint32_t width, uint32_t height, uint16_t depth, Format format,
			ResourceFlag resourceFlags = ResourceFlag::NONE, uint8_t numMips = 1,
			TextureLayout textureLayout = TextureLayout::UNKNOWN,
			uint64_t requiredAlignment = 0);
	};

	//--------------------------------------------------------------------------------------
	// Buffer/raw buffer
	//--------------------------------------------------------------------------------------
	class Buffer_VK :
		public virtual Buffer,
		public virtual ShaderResource_VK
	{
	public:
		Buffer_VK();
		virtual ~Buffer_VK();

		using Resource_VK::Create;
		bool Create(Device* pDevice, size_t byteWidth, ResourceFlag resourceFlags = ResourceFlag::NONE,
			MemoryType memoryType = MemoryType::DEFAULT, uint32_t numSRVs = 1,
			const uintptr_t* firstSrvElements = nullptr, uint32_t numUAVs = 1,
			const uintptr_t* firstUavElements = nullptr, MemoryFlag memoryFlags = MemoryFlag::NONE,
			const wchar_t* name = nullptr, const uintptr_t* counterByteOffsets = nullptr,
			uint32_t maxThreads = 1);
		bool Create(Device* pDevice, Heap* pHeap, uint64_t heapOffset, size_t byteWidth,
			ResourceFlag resourceFlags = ResourceFlag::NONE, uint32_t numSRVs = 1,
			const uintptr_t* firstSrvElements = nullptr, uint32_t numUAVs = 1,
			const uintptr_t* firstUavElements = nullptr, const wchar_t* name = nullptr,
			const uintptr_t* counterByteOffsets = nullptr, uint32_t maxThreads = 1);
		bool CreateResource(Device* pDevice, size_t byteWidth, ResourceFlag resourceFlags = ResourceFlag::NONE,
			MemoryType memoryType = MemoryType::DEFAULT, MemoryFlag memoryFlags = MemoryFlag::NONE,
			ResourceState initialResourceState = ResourceState::COMMON, uint8_t numCastableFormats = 0,
			const Format* pCastableFormats = nullptr, uint32_t maxThreads = 1);
		bool CreateResource(Device* pDevice, Heap* pHeap, uint64_t heapOffset, size_t byteWidth,
			ResourceFlag resourceFlags = ResourceFlag::NONE, ResourceState initialResourceState = ResourceState::COMMON,
			uint8_t numCastableFormats = 0, const Format* pCastableFormats = nullptr, uint32_t maxThreads = 1);
		bool Upload(CommandList* pCommandList, Resource* pUploader, const void* pData, size_t size,
			size_t offset = 0, ResourceState dstState = ResourceState::COMMON, uint32_t threadIdx = 0);
		bool Upload(CommandList* pCommandList, uint32_t descriptorIndex, Resource* pUploader, const void* pData,
			size_t size, ResourceState dstState = ResourceState::COMMON, uint32_t threadIdx = 0);
		bool ReadBack(CommandList* pCommandList, Buffer* pReadBuffer, size_t size = 0, size_t dstOffset = 0,
			size_t srcOffset = 0, ResourceState dstState = ResourceState::COMMON, uint32_t threadIdx = 0);

		Descriptor CreateSRV(uint32_t numElements, uint32_t byteStride, Format format, size_t firstElement = 0,
			uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING);
		Descriptor CreateUAV(uint32_t numElements, uint32_t byteStride, Format format,
			size_t firstElement = 0, size_t counterByteOffset = 0);

		const Descriptor& GetUAV(uint32_t index = 0) const;

		void* Map(uint32_t descriptorIndex = 0, uintptr_t readBegin = 0, uintptr_t readEnd = 0);
		void* Map(const Range* pReadRange, uint32_t descriptorIndex = 0);

		void SetCounter(const Resource::sptr& counter);
		Resource::sptr GetCounter() const;
		size_t GetTiledResourceTileSize() const;

		static void GetAllocationInfo(uint64_t& byteSize, uint64_t& alignment, Device* pDevice,
			size_t byteWidth, ResourceFlag resourceFlags = ResourceFlag::NONE,
			uint64_t requiredAlignment = 0);
		static size_t AlignRawView(size_t byteSize);
		static size_t AlignCounter(size_t byteSize);

	protected:
		bool createViews(size_t numElements, uint32_t byteStride, Format format,
			uint32_t numSRVs, const uintptr_t* firstSrvElements, uint32_t numUAVs,
			const uintptr_t* firstUavElements, const uintptr_t* counterByteOffsets,
			uint16_t srvComponentMapping);
		void getViewRange(uint32_t& viewElements, uintptr_t& firstElement, uint32_t i,
			size_t bufferElements, uint32_t byteStride, const uintptr_t* firstElements, uint32_t numDescriptors);

		std::vector<Descriptor>	m_uavs;
		std::vector<uintptr_t>	m_srvByteOffsets;
		std::shared_ptr<Resource> m_counter;
	};

	//--------------------------------------------------------------------------------------
	// Structured buffer
	//--------------------------------------------------------------------------------------
	class StructuredBuffer_VK :
		public virtual StructuredBuffer,
		public virtual Buffer_VK
	{
	public:
		StructuredBuffer_VK();
		virtual ~StructuredBuffer_VK();

		using Resource_VK::Create;
		bool Create(Device* pDevice, size_t numElements, uint32_t byteStride,
			ResourceFlag resourceFlags = ResourceFlag::NONE, MemoryType memoryType = MemoryType::DEFAULT,
			uint32_t numSRVs = 1, const uintptr_t* firstSrvElements = nullptr,
			uint32_t numUAVs = 1, const uintptr_t* firstUavElements = nullptr,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr,
			const uintptr_t* counterByteOffsets = nullptr, uint32_t maxThreads = 1);
		bool Create(Device* pDevice, Heap* pHeap, uint64_t heapOffset, size_t numElements,
			uint32_t byteStride, ResourceFlag resourceFlags = ResourceFlag::NONE,
			uint32_t numSRVs = 1, const uintptr_t* firstSrvElements = nullptr,
			uint32_t numUAVs = 1, const uintptr_t* firstUavElements = nullptr,
			const wchar_t* name = nullptr, const uintptr_t* counterByteOffsets = nullptr,
			uint32_t maxThreads = 1);
	};

	//--------------------------------------------------------------------------------------
	// Typed buffer
	//--------------------------------------------------------------------------------------
	class TypedBuffer_VK :
		public virtual TypedBuffer,
		public virtual Buffer_VK
	{
	public:
		TypedBuffer_VK();
		virtual ~TypedBuffer_VK();

		using Resource_VK::Create;
		bool Create(Device* pDevice, size_t numElements, uint32_t byteStride, Format format,
			ResourceFlag resourceFlags = ResourceFlag::NONE, MemoryType memoryType = MemoryType::DEFAULT,
			uint32_t numSRVs = 1, const uintptr_t* firstSrvElements = nullptr,
			uint32_t numUAVs = 1, const uintptr_t* firstUavElements = nullptr,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr,
			uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
			const uintptr_t* counterByteOffsets = nullptr, uint8_t numUavFormats = 0,
			const Format* uavFormats = nullptr, uint32_t maxThreads = 1);
		bool Create(Device* pDevice, Heap* pHeap, uint64_t heapOffset, size_t numElements,
			uint32_t byteStride, Format format, ResourceFlag resourceFlags = ResourceFlag::NONE,
			uint32_t numSRVs = 1, const uintptr_t* firstSrvElements = nullptr,
			uint32_t numUAVs = 1, const uintptr_t* firstUavElements = nullptr,
			const wchar_t* name = nullptr, uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
			const uintptr_t* counterByteOffsets = nullptr, uint8_t numUavFormats = 0,
			const Format* uavFormats = nullptr, uint32_t maxThreads = 1);

		const Descriptor& GetUAV(uint32_t index = 0, Format format = Format::UNKNOWN) const;

	protected:
		std::unordered_map<Format, std::vector<Descriptor>> m_castableUavs;
	};

	//--------------------------------------------------------------------------------------
	// Vertex buffer
	//--------------------------------------------------------------------------------------
	class VertexBuffer_VK :
		public virtual VertexBuffer,
		public virtual StructuredBuffer_VK
	{
	public:
		VertexBuffer_VK();
		virtual ~VertexBuffer_VK();

		using Resource_VK::Create;
		bool Create(Device* pDevice, size_t numVertices, uint32_t byteStride,
			ResourceFlag resourceFlags = ResourceFlag::NONE, MemoryType memoryType = MemoryType::DEFAULT,
			uint32_t numVBVs = 1, const uintptr_t* firstVertices = nullptr,
			uint32_t numSRVs = 1, const uintptr_t* firstSrvElements = nullptr,
			uint32_t numUAVs = 1, const uintptr_t* firstUavElements = nullptr,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr,
			const uintptr_t* counterByteOffsets = nullptr, uint32_t maxThreads = 1);
		bool Create(Device* pDevice, Heap* pHeap, uint64_t heapOffset, size_t numVertices,
			uint32_t byteStride, ResourceFlag resourceFlags = ResourceFlag::NONE,
			uint32_t numVBVs = 1, const uintptr_t* firstVertices = nullptr,
			uint32_t numSRVs = 1, const uintptr_t* firstSrvElements = nullptr,
			uint32_t numUAVs = 1, const uintptr_t* firstUavElements = nullptr,
			const wchar_t* name = nullptr, const uintptr_t* counterByteOffsets = nullptr,
			uint32_t maxThreads = 1);
		bool CreateAsRaw(Device* pDevice, size_t numVertices, uint32_t byteStride,
			ResourceFlag resourceFlags = ResourceFlag::NONE, MemoryType memoryType = MemoryType::DEFAULT,
			uint32_t numVBVs = 1, const uintptr_t* firstVertices = nullptr,
			uint32_t numSRVs = 1, const uintptr_t* firstSrvElements = nullptr,
			uint32_t numUAVs = 1, const uintptr_t* firstUavElements = nullptr,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr,
			const uintptr_t* counterByteOffsets = nullptr, uint32_t maxThreads = 1);
		bool CreateAsRaw(Device* pDevice, Heap* pHeap, uint64_t heapOffset, size_t numVertices,
			uint32_t byteStride, ResourceFlag resourceFlags = ResourceFlag::NONE,
			uint32_t numVBVs = 1, const uintptr_t* firstVertices = nullptr,
			uint32_t numSRVs = 1, const uintptr_t* firstSrvElements = nullptr,
			uint32_t numUAVs = 1, const uintptr_t* firstUavElements = nullptr,
			const wchar_t* name = nullptr, const uintptr_t* counterByteOffsets = nullptr,
			uint32_t maxThreads = 1);

		void CreateVBV(VertexBufferView& vbv, uint32_t numVertices, uint32_t byteStride, size_t firstVertex = 0);
		const VertexBufferView& GetVBV(uint32_t index = 0) const;

	protected:
		void createVBVs(size_t numVertices, uint32_t byteStride, uint32_t numVBVs, const uintptr_t* firstVertices);

		std::vector<VertexBufferView> m_vbvs;
	};

	//--------------------------------------------------------------------------------------
	// Index buffer
	//--------------------------------------------------------------------------------------
	class IndexBuffer_VK :
		public virtual IndexBuffer,
		public virtual TypedBuffer_VK
	{
	public:
		IndexBuffer_VK();
		virtual ~IndexBuffer_VK();

		using Resource_VK::Create;
		bool Create(Device* pDevice, size_t byteWidth, Format format = Format::R32_UINT,
			ResourceFlag resourceFlags = ResourceFlag::DENY_SHADER_RESOURCE,
			MemoryType memoryType = MemoryType::DEFAULT,
			uint32_t numIBVs = 1, const uintptr_t* ibvByteOffsets = nullptr,
			uint32_t numSRVs = 1, const uintptr_t* firstSrvElements = nullptr,
			uint32_t numUAVs = 1, const uintptr_t* firstUavElements = nullptr,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr,
			uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
			const uintptr_t* counterByteOffsets = nullptr, uint32_t maxThreads = 1);
		bool Create(Device* pDevice, Heap* pHeap, uint64_t heapOffset, size_t byteWidth,
			Format format = Format::R32_UINT, ResourceFlag resourceFlags = ResourceFlag::DENY_SHADER_RESOURCE,
			uint32_t numIBVs = 1, const uintptr_t* ibvByteOffsets = nullptr,
			uint32_t numSRVs = 1, const uintptr_t* firstSrvElements = nullptr,
			uint32_t numUAVs = 1, const uintptr_t* firstUavElements = nullptr,
			const wchar_t* name = nullptr, uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
			const uintptr_t* counterByteOffsets = nullptr, uint32_t maxThreads = 1);

		void CreateIBV(IndexBufferView& ibv, Format format, uint32_t byteSize, size_t byteOffset = 0);
		const IndexBufferView& GetIBV(uint32_t index = 0) const;

	protected:
		std::vector<IndexBufferView> m_ibvs;
	};

	inline Resource_VK* AsVK(Resource* p)
	{
		assert(!p || p->GetBackend());
		return static_cast<Resource_VK*>(p ? p->GetBackend() : nullptr);
	}

	inline const Resource_VK* AsVK(const Resource* p)
	{
		assert(!p || const_cast<Resource*>(p)->GetBackend());
		return static_cast<const Resource_VK*>(p ? const_cast<Resource*>(p)->GetBackend() : nullptr);
	}

	inline Heap_VK* AsVK(Heap* p)
	{
		assert(!p || p->GetBackend());
		return static_cast<Heap_VK*>(p ? p->GetBackend() : nullptr);
	}

	inline const Heap_VK* AsVK(const Heap* p)
	{
		assert(!p || const_cast<Heap*>(p)->GetBackend());
		return static_cast<const Heap_VK*>(p ? const_cast<Heap*>(p)->GetBackend() : nullptr);
	}
}
