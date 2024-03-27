//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSG.h"

namespace XUSG
{
	//--------------------------------------------------------------------------------------
	// Resource
	//--------------------------------------------------------------------------------------
	class Resource_DX12 :
		public virtual Resource
	{
	public:
		Resource_DX12();
		virtual ~Resource_DX12();

		bool Initialize(const Device* pDevice);

		Descriptor AllocateCbvSrvUavHeap(uint32_t numDescriptors);

		uint32_t SetBarrier(ResourceBarrier* pBarriers, ResourceState dstState,
			uint32_t numBarriers = 0, uint32_t subresource = XUSG_BARRIER_ALL_SUBRESOURCES,
			BarrierFlag flags = BarrierFlag::NONE, ResourceState srcState = ResourceState::AUTO,
			uint32_t threadIdx = 0);

		ResourceState Transition(ResourceState dstState, uint32_t subresource = XUSG_BARRIER_ALL_SUBRESOURCES,
			BarrierFlag flag = BarrierFlag::NONE, ResourceState srcState = ResourceState::AUTO, uint32_t threadIdx = 0);
		ResourceState GetResourceState(uint32_t subresource = 0, uint32_t threadIdx = 0) const;

		uint64_t GetWidth() const;

		uint64_t GetVirtualAddress(int offset = 0) const;

		void Create(void* pDeviceHandle, void* pResourceHandle,
			const wchar_t* name = nullptr, uint32_t maxThreads = 1);
		void SetName(const wchar_t* name);

		void* GetHandle() const;

		com_ptr<ID3D12Resource>& GetResource();

	protected:
		com_ptr<ID3D12Device>	m_device;
		com_ptr<ID3D12Resource>	m_resource;
		std::vector<std::vector<ResourceState>> m_states;

		com_ptr<ID3D12DescriptorHeap> m_cbvSrvUavHeap;

		bool m_hasPromotion;

		std::wstring m_name;
	};

	//--------------------------------------------------------------------------------------
	// Constant buffer
	//--------------------------------------------------------------------------------------
	class ConstantBuffer_DX12 :
		public virtual ConstantBuffer,
		public virtual Resource_DX12
	{
	public:
		ConstantBuffer_DX12();
		virtual ~ConstantBuffer_DX12();

		bool Create(const Device* pDevice, size_t byteWidth, uint32_t numCBVs = 1,
			const size_t* cbvByteOffsets = nullptr, MemoryType memoryType = MemoryType::UPLOAD,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr);
		bool CreateResource(size_t byteWidth, MemoryType memoryType = MemoryType::UPLOAD,
			MemoryFlag memoryFlags = MemoryFlag::NONE, ResourceState initialResourceState = ResourceState::COMMON);
		bool Upload(CommandList* pCommandList, Resource* pUploader, const void* pData,
			size_t size, size_t byteOffset = 0, ResourceState srcState = ResourceState::COMMON,
			ResourceState dstState = ResourceState::COMMON);
		bool Upload(CommandList* pCommandList, uint32_t cbvIndex, Resource* pUploader,
			const void* pData, size_t byteSize, ResourceState srcState = ResourceState::COMMON,
			ResourceState dstState = ResourceState::COMMON);

		Descriptor CreateCBV(const Descriptor& cbvHeapStart, uint32_t descriptorIdx,
			uint32_t byteSize, size_t byteOffset = 0);

		void* Map(uint32_t cbvIndex = 0);
		void Unmap();

		const Descriptor& GetCBV(uint32_t index = 0) const;
		uint32_t GetCBVOffset(uint32_t index) const;

	protected:
		std::vector<Descriptor>	m_cbvs;
		std::vector<size_t> m_cbvByteOffsets;

		void* m_pDataBegin;
	};

	//--------------------------------------------------------------------------------------
	// Resource base
	//--------------------------------------------------------------------------------------
	class ShaderResource_DX12 :
		public virtual ShaderResource,
		public virtual Resource_DX12
	{
	public:
		ShaderResource_DX12();
		virtual ~ShaderResource_DX12();

		const Descriptor& GetSRV(uint32_t index = 0) const;

		Format GetFormat() const;

	protected:
		Format m_format;

		std::vector<Descriptor> m_srvs;
	};

	//--------------------------------------------------------------------------------------
	// Texture
	//--------------------------------------------------------------------------------------
	class Texture_DX12 :
		public virtual Texture,
		public virtual ShaderResource_DX12
	{
	public:
		Texture_DX12();
		virtual ~Texture_DX12();

		bool Create(const Device* pDevice, uint32_t width, uint32_t height, Format format,
			uint16_t arraySize = 1, ResourceFlag resourceFlags = ResourceFlag::NONE,
			uint8_t numMips = 1, uint8_t sampleCount = 1, bool isCubeMap = false,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr,
			uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
			TextureLayout textureLayout = TextureLayout::UNKNOWN,
			uint32_t maxThreads = 1);
		bool CreateResource(uint32_t width, uint32_t height, Format format, uint16_t arraySize = 1,
			ResourceFlag resourceFlags = ResourceFlag::NONE, uint8_t numMips = 1, uint8_t sampleCount = 1,
			MemoryFlag memoryFlags = MemoryFlag::NONE, ResourceState initialResourceState = ResourceState::COMMON,
			TextureLayout textureLayout = TextureLayout::UNKNOWN, uint32_t maxThreads = 1);
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

		Descriptor CreateSRV(const Descriptor& srvHeapStart, uint32_t descriptorIdx, uint16_t arraySize,
			uint16_t firstArraySlice = 0, Format format = Format::UNKNOWN, uint8_t numMips = 1,
			uint8_t mostDetailedMip = 0, bool multisamples = false, bool isCubeMap = false,
			uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING, uint8_t plane = 0);
		Descriptor CreateUAV(const Descriptor& uavHeapStart, uint32_t descriptorIdx, uint16_t arraySize,
			uint16_t firstArraySlice = 0, Format format = Format::UNKNOWN, uint8_t mipLevel = 0,
			uint8_t plane = 0);

		uint32_t SetBarrier(ResourceBarrier* pBarriers, ResourceState dstState,
			uint32_t numBarriers = 0, uint32_t subresource = XUSG_BARRIER_ALL_SUBRESOURCES,
			BarrierFlag flags = BarrierFlag::NONE, ResourceState srcState = ResourceState::AUTO,
			uint32_t threadIdx = 0);
		uint32_t SetBarrier(ResourceBarrier* pBarriers, uint8_t mipLevel, ResourceState dstState,
			uint32_t numBarriers = 0, uint32_t slice = 0, BarrierFlag flags = BarrierFlag::NONE,
			ResourceState srcState = ResourceState::AUTO, uint32_t threadIdx = 0);

		void Blit(const CommandList* pCommandList, uint32_t groupSizeX, uint32_t groupSizeY,
			uint32_t groupSizeZ, const DescriptorTable& uavSrvTable, uint32_t uavSrvSlot = 0,
			uint8_t mipLevel = 0, const DescriptorTable& srvTable = XUSG_NULL, uint32_t srvSlot = 0,
			const DescriptorTable& samplerTable = XUSG_NULL, uint32_t samplerSlot = 1,
			const Pipeline& pipeline = nullptr);

		uint32_t Blit(CommandList* pCommandList, ResourceBarrier* pBarriers, uint32_t groupSizeX,
			uint32_t groupSizeY, uint32_t groupSizeZ, uint8_t mipLevel, int8_t srcMipLevel,
			ResourceState srcState, const DescriptorTable& uavSrvTable, uint32_t uavSrvSlot = 0,
			uint32_t numBarriers = 0, const DescriptorTable& srvTable = XUSG_NULL, uint32_t srvSlot = 0,
			uint16_t baseSlice = 0, uint16_t numSlices = 0, uint32_t threadIdx = 0);
		uint32_t GenerateMips(CommandList* pCommandList, ResourceBarrier* pBarriers, uint32_t groupSizeX,
			uint32_t groupSizeY, uint32_t groupSizeZ, ResourceState dstState, const PipelineLayout& pipelineLayout,
			const Pipeline& pipeline, const DescriptorTable* pUavSrvTables, uint32_t uavSrvSlot = 0,
			const DescriptorTable& samplerTable = XUSG_NULL, uint32_t samplerSlot = 1, uint32_t numBarriers = 0,
			const DescriptorTable* pSrvTables = nullptr, uint32_t srvSlot = 0, uint8_t baseMip = 1,
			uint8_t numMips = 0, uint16_t baseSlice = 0, uint16_t numSlices = 0, uint32_t threadIdx = 0);

		const Descriptor& GetUAV(uint8_t index = 0) const;
		const Descriptor& GetPackedUAV(uint8_t index = 0) const;
		const Descriptor& GetSRVLevel(uint8_t level) const;

		uint32_t	GetHeight() const;
		uint16_t	GetArraySize() const;
		uint8_t		GetNumMips() const;

		size_t GetRequiredIntermediateSize(uint32_t firstSubresource, uint32_t numSubresources) const;

	protected:
		bool createSRVs(uint32_t& descriptorIdx, uint16_t arraySize, Format format, uint8_t numMips,
			bool multisamples, bool isCubeMap, uint16_t srvComponentMapping);
		bool createSRVLevels(uint32_t& descriptorIdx, uint16_t arraySize, uint8_t numMips, Format format,
			bool multisamples, bool isCubeMap, uint16_t srvComponentMapping);
		bool createUAVs(uint32_t& descriptorIdx, uint16_t arraySize, Format format, uint8_t numMips);
		bool createPackedUAVs(uint32_t& descriptorIdx, uint16_t arraySize, Format format, uint8_t numMips);

		std::vector<Descriptor>	m_uavs;
		std::vector<Descriptor>	m_packedUavs;
		std::vector<Descriptor>	m_srvLevels;
	};

	//--------------------------------------------------------------------------------------
	// Render target
	//--------------------------------------------------------------------------------------
	class RenderTarget_DX12 :
		public virtual RenderTarget,
		public virtual Texture_DX12
	{
	public:
		RenderTarget_DX12();
		virtual ~RenderTarget_DX12();

		bool Create(const Device* pDevice, uint32_t width, uint32_t height, Format format,
			uint16_t arraySize = 1, ResourceFlag resourceFlags = ResourceFlag::NONE, uint8_t numMips = 1,
			uint8_t sampleCount = 1, const float* pClearColor = nullptr, bool isCubeMap = false,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr,
			uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
			TextureLayout textureLayout = TextureLayout::UNKNOWN,
			uint32_t maxThreads = 1);
		bool CreateArray(const Device* pDevice, uint32_t width, uint32_t height, uint16_t arraySize,
			Format format, ResourceFlag resourceFlags = ResourceFlag::NONE, uint8_t numMips = 1,
			uint8_t sampleCount = 1, const float* pClearColor = nullptr, bool isCubeMap = false,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr,
			uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
			TextureLayout textureLayout = TextureLayout::UNKNOWN,
			uint32_t maxThreads = 1);
		bool Initialize(const Device* pDevice);
		bool CreateResource(uint32_t width, uint32_t height, Format format, uint16_t arraySize = 1,
			ResourceFlag resourceFlags = ResourceFlag::NONE, uint8_t numMips = 1, uint8_t sampleCount = 1,
			MemoryFlag memoryFlags = MemoryFlag::NONE, ResourceState initialResourceState = ResourceState::COMMON,
			const float* pClearColor = nullptr, TextureLayout textureLayout = TextureLayout::UNKNOWN,
			uint32_t maxThreads = 1);
		bool CreateFromSwapChain(const Device* pDevice, const SwapChain* pSwapChain,
			uint32_t bufferIndex, uint32_t maxThreads = 1);

		Descriptor AllocateRtvHeap(uint32_t numDescriptors);
		Descriptor CreateRTV(const Descriptor& rtvHeapStart, uint32_t descriptorIdx, uint16_t arraySize,
			uint16_t firstArraySlice = 0, Format format = Format::UNKNOWN, uint8_t mipLevel = 0,
			bool multisamples = false);

		void Blit(const CommandList* pCommandList, const DescriptorTable& srcSrvTable,
			uint32_t srcSlot = 0, uint8_t mipLevel = 0, uint16_t baseSlice = 0,
			uint16_t numSlices = 0, const DescriptorTable& samplerTable = XUSG_NULL,
			uint32_t samplerSlot = 1, const Pipeline& pipeline = nullptr,
			uint32_t offsetForSliceId = 0, uint32_t cbSlot = 2);

		uint32_t Blit(CommandList* pCommandList, ResourceBarrier* pBarriers, uint8_t mipLevel,
			int8_t srcMipLevel, ResourceState srcState, const DescriptorTable& srcSrvTable,
			uint32_t srcSlot = 0, uint32_t numBarriers = 0, uint16_t baseSlice = 0, uint16_t numSlices = 0,
			uint32_t offsetForSliceId = 0, uint32_t cbSlot = 2, uint32_t threadIdx = 0);
		uint32_t GenerateMips(CommandList* pCommandList, ResourceBarrier* pBarriers, ResourceState dstState,
			const PipelineLayout& pipelineLayout, const Pipeline& pipeline, const DescriptorTable* pSrcSrvTables,
			uint32_t srcSlot = 0, const DescriptorTable& samplerTable = XUSG_NULL, uint32_t samplerSlot = 1,
			uint32_t numBarriers = 0, uint8_t baseMip = 1, uint8_t numMips = 0, uint16_t baseSlice = 0,
			uint16_t numSlices = 0, uint32_t offsetForSliceId = 0, uint32_t cbSlot = 2, uint32_t threadIdx = 0);

		const Descriptor& GetRTV(uint16_t slice = 0, uint8_t mipLevel = 0) const;

	protected:
		bool create(const Device* pDevice, uint32_t width, uint32_t height, uint16_t arraySize,
			Format format, uint8_t numMips, uint8_t sampleCount, ResourceFlag resourceFlags,
			const float* pClearColor, bool isCubeMap, MemoryFlag memoryFlags, const wchar_t* name,
			uint16_t srvComponentMapping, TextureLayout textureLayout, uint32_t maxThreads);

		com_ptr<ID3D12DescriptorHeap> m_rtvHeap;

		std::vector<std::vector<Descriptor>> m_rtvs;
	};

	//--------------------------------------------------------------------------------------
	// Depth stencil
	//--------------------------------------------------------------------------------------
	class DepthStencil_DX12 :
		public virtual DepthStencil,
		public virtual Texture_DX12
	{
	public:
		DepthStencil_DX12();
		virtual ~DepthStencil_DX12();

		bool Create(const Device* pDevice, uint32_t width, uint32_t height,
			Format format = Format::UNKNOWN, ResourceFlag resourceFlags = ResourceFlag::NONE,
			uint16_t arraySize = 1, uint8_t numMips = 1, uint8_t sampleCount = 1,
			float clearDepth = 1.0f, uint8_t clearStencil = 0, bool isCubeMap = false,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr,
			uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
			uint16_t stencilSrvComponentMapping = XUSG_DEFAULT_STENCIL_SRV_COMPONENT_MAPPING,
			TextureLayout textureLayout = TextureLayout::UNKNOWN, uint32_t maxThreads = 1);
		bool CreateArray(const Device* pDevice, uint32_t width, uint32_t height, uint16_t arraySize,
			Format format = Format::UNKNOWN, ResourceFlag resourceFlags = ResourceFlag::NONE,
			uint8_t numMips = 1, uint8_t sampleCount = 1, float clearDepth = 1.0f, uint8_t clearStencil = 0,
			bool isCubeMap = false, MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr,
			uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
			uint16_t stencilSrvComponentMapping = XUSG_DEFAULT_STENCIL_SRV_COMPONENT_MAPPING,
			TextureLayout textureLayout = TextureLayout::UNKNOWN, uint32_t maxThreads = 1);
		bool Initialize(const Device* pDevice);
		bool CreateResource(uint32_t width, uint32_t height, Format format, uint16_t arraySize = 1,
			ResourceFlag resourceFlags = ResourceFlag::NONE, uint8_t numMips = 1, uint8_t sampleCount = 1,
			MemoryFlag memoryFlags = MemoryFlag::NONE, ResourceState initialResourceState = ResourceState::DEPTH_WRITE,
			float clearDepth = 1.0f, uint8_t clearStencil = 0, TextureLayout textureLayout = TextureLayout::UNKNOWN,
			uint32_t maxThreads = 1);

		Descriptor AllocateDsvHeap(uint32_t numDescriptors);
		Descriptor CreateDSV(const Descriptor& dsvHeapStart, uint32_t descriptorIdx, uint16_t arraySize,
			uint16_t firstArraySlice = 0, Format format = Format::UNKNOWN, uint8_t mipLevel = 0,
			bool multisamples = false, bool readOnlyDepth = false, bool readOnlyStencil = false);

		const Descriptor& GetDSV(uint16_t slice = 0, uint8_t mipLevel = 0) const;
		const Descriptor& GetReadOnlyDSV(uint16_t slice = 0, uint8_t mipLevel = 0) const;
		const Descriptor& GetStencilSRV() const;

	protected:
		bool create(const Device* pDevice, uint32_t width, uint32_t height, uint16_t arraySize, uint8_t numMips,
			uint8_t sampleCount, Format format, ResourceFlag resourceFlags, float clearDepth, uint8_t clearStencil,
			bool& hasSRV, Format& formatStencil, bool isCubeMap, MemoryFlag memoryFlags, const wchar_t* name,
			uint16_t srvComponentMapping, uint16_t stencilSrvComponentMapping, TextureLayout textureLayout,
			uint32_t maxThreads);

		static Format mapResourceFormat(Format format);

		com_ptr<ID3D12DescriptorHeap> m_dsvHeap;

		std::vector<std::vector<Descriptor>> m_dsvs;
		std::vector<std::vector<Descriptor>> m_readOnlyDsvs;

		Descriptor m_stencilSrv;
	};

	//--------------------------------------------------------------------------------------
	// 3D Texture
	//--------------------------------------------------------------------------------------
	class Texture3D_DX12 :
		public virtual Texture3D,
		public virtual Texture_DX12
	{
	public:
		Texture3D_DX12();
		virtual ~Texture3D_DX12();

		bool Create(const Device* pDevice, uint32_t width, uint32_t height, uint16_t depth,
			Format format, ResourceFlag resourceFlags = ResourceFlag::NONE, uint8_t numMips = 1,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr,
			uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
			TextureLayout textureLayout = TextureLayout::UNKNOWN,
			uint32_t maxThreads = 1);
		bool CreateResource(uint32_t width, uint32_t height, uint16_t depth,
			Format format, ResourceFlag resourceFlags = ResourceFlag::NONE,
			uint8_t numMips = 1, MemoryFlag memoryFlags = MemoryFlag::NONE,
			ResourceState initialResourceState = ResourceState::COMMON,
			TextureLayout textureLayout = TextureLayout::UNKNOWN,
			uint32_t maxThreads = 1);

		Descriptor CreateSRV(const Descriptor& srvHeapStart, uint32_t descriptorIdx,
			Format format = Format::UNKNOWN, uint8_t numMips = 1, uint8_t mostDetailedMip = 0,
			uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING);
		Descriptor CreateUAV(const Descriptor& uavHeapStart, uint32_t descriptorIdx, uint16_t wSize,
			uint16_t firstWSlice = 0, Format format = Format::UNKNOWN, uint8_t mipLevel = 0);

		uint16_t GetDepth() const;
	};

	//--------------------------------------------------------------------------------------
	// Buffer/raw buffer
	//--------------------------------------------------------------------------------------
	class Buffer_DX12 :
		public virtual Buffer,
		public virtual ShaderResource_DX12
	{
	public:
		Buffer_DX12();
		virtual ~Buffer_DX12();

		bool Create(const Device* pDevice, size_t byteWidth, ResourceFlag resourceFlags = ResourceFlag::NONE,
			MemoryType memoryType = MemoryType::DEFAULT, uint32_t numSRVs = 1,
			const uint32_t* firstSrvElements = nullptr, uint32_t numUAVs = 1,
			const uint32_t* firstUavElements = nullptr, MemoryFlag memoryFlags = MemoryFlag::NONE,
			const wchar_t* name = nullptr, const size_t* counterByteOffsets = nullptr, uint32_t maxThreads = 1);
		bool CreateResource(size_t byteWidth, ResourceFlag resourceFlags = ResourceFlag::NONE,
			MemoryType memoryType = MemoryType::DEFAULT, MemoryFlag memoryFlags = MemoryFlag::NONE,
			ResourceState initialResourceState = ResourceState::COMMON, uint32_t maxThreads = 1);
		bool Upload(CommandList* pCommandList, Resource* pUploader, const void* pData, size_t size,
			size_t offset = 0, ResourceState dstState = ResourceState::COMMON, uint32_t threadIdx = 0);
		bool Upload(CommandList* pCommandList, uint32_t descriptorIndex, Resource* pUploader, const void* pData,
			size_t size, ResourceState dstState = ResourceState::COMMON, uint32_t threadIdx = 0);
		bool ReadBack(CommandList* pCommandList, Buffer* pReadBuffer, size_t size = 0, size_t dstOffset = 0,
			size_t srcOffset = 0, ResourceState dstState = ResourceState::COMMON, uint32_t threadIdx = 0);

		Descriptor CreateSRV(const Descriptor& srvHeapStart, uint32_t descriptorIdx,
			uint32_t numElements, uint32_t byteStride, Format format, uint32_t firstElement = 0,
			uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING);
		Descriptor CreateUAV(const Descriptor& uavHeapStart, uint32_t descriptorIdx, uint32_t numElements,
			uint32_t byteStride, Format format, uint32_t firstElement = 0, size_t counterByteOffset = 0);

		const Descriptor& GetUAV(uint32_t index = 0) const;

		void* Map(uint32_t descriptorIndex = 0, uintptr_t readBegin = 0, uintptr_t readEnd = 0);
		void* Map(const Range* pReadRange, uint32_t descriptorIndex = 0);

		void Unmap();
		void SetCounter(const Resource::sptr& counter);

		Resource::sptr GetCounter() const;

	protected:
		bool create(const Device* pDevice, size_t byteWidth, ResourceFlag resourceFlags,
			MemoryType memoryType, uint32_t numSRVs, uint32_t numUAVs, MemoryFlag memoryFlags,
			const wchar_t* name, uint32_t maxThreads);

		void getViewRange(uint32_t& viewElements, uint32_t& firstElement, uint32_t i,
			uint32_t bufferElements, uint32_t byteStride, const uint32_t* firstElements,
			uint32_t numDescriptors, size_t* pByteOffset = nullptr);

		std::vector<Descriptor>	m_uavs;
		std::vector<size_t>	m_srvByteOffsets;

		std::shared_ptr<Resource> m_counter;

		void* m_pDataBegin;
	};

	//--------------------------------------------------------------------------------------
	// Structured buffer
	//--------------------------------------------------------------------------------------
	class StructuredBuffer_DX12 :
		public virtual StructuredBuffer,
		public virtual Buffer_DX12
	{
	public:
		StructuredBuffer_DX12();
		virtual ~StructuredBuffer_DX12();

		bool Create(const Device* pDevice, uint32_t numElements, uint32_t byteStride,
			ResourceFlag resourceFlags = ResourceFlag::NONE, MemoryType memoryType = MemoryType::DEFAULT,
			uint32_t numSRVs = 1, const uint32_t* firstSrvElements = nullptr,
			uint32_t numUAVs = 1, const uint32_t* firstUavElements = nullptr,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr,
			const size_t* counterByteOffsets = nullptr, uint32_t maxThreads = 1);
	};

	//--------------------------------------------------------------------------------------
	// Typed buffer
	//--------------------------------------------------------------------------------------
	class TypedBuffer_DX12 :
		public virtual TypedBuffer,
		public virtual Buffer_DX12
	{
	public:
		TypedBuffer_DX12();
		virtual ~TypedBuffer_DX12();

		bool Create(const Device* pDevice, uint32_t numElements, uint32_t byteStride, Format format,
			ResourceFlag resourceFlags = ResourceFlag::NONE, MemoryType memoryType = MemoryType::DEFAULT,
			uint32_t numSRVs = 1, const uint32_t* firstSrvElements = nullptr,
			uint32_t numUAVs = 1, const uint32_t* firstUavElements = nullptr,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr,
			uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
			const size_t* counterByteOffsets = nullptr, uint32_t maxThreads = 1);

		const Descriptor& GetPackedUAV(uint32_t index = 0) const;

	protected:
		std::vector<Descriptor>	m_packedUavs;
	};

	//--------------------------------------------------------------------------------------
	// Vertex buffer
	//--------------------------------------------------------------------------------------
	class VertexBuffer_DX12 :
		public virtual VertexBuffer,
		public virtual StructuredBuffer_DX12
	{
	public:
		VertexBuffer_DX12();
		virtual ~VertexBuffer_DX12();

		bool Create(const Device* pDevice, uint32_t numVertices, uint32_t byteStride,
			ResourceFlag resourceFlags = ResourceFlag::NONE, MemoryType memoryType = MemoryType::DEFAULT,
			uint32_t numVBVs = 1, const uint32_t* firstVertices = nullptr,
			uint32_t numSRVs = 1, const uint32_t* firstSrvElements = nullptr,
			uint32_t numUAVs = 1, const uint32_t* firstUavElements = nullptr,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr,
			const size_t* counterByteOffsets = nullptr, uint32_t maxThreads = 1);
		bool CreateAsRaw(const Device* pDevice, uint32_t numVertices, uint32_t byteStride,
			ResourceFlag resourceFlags = ResourceFlag::NONE, MemoryType memoryType = MemoryType::DEFAULT,
			uint32_t numVBVs = 1, const uint32_t* firstVertices = nullptr,
			uint32_t numSRVs = 1, const uint32_t* firstSrvElements = nullptr,
			uint32_t numUAVs = 1, const uint32_t* firstUavElements = nullptr,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr,
			const size_t* counterByteOffsets = nullptr, uint32_t maxThreads = 1);

		void CreateVBV(VertexBufferView& vbv, uint32_t numVertices, uint32_t byteStride, uint32_t firstVertex = 0);

		const VertexBufferView& GetVBV(uint32_t index = 0) const;

	protected:
		void createVBVs(uint32_t numVertices, uint32_t byteStride, uint32_t numVBVs, const uint32_t* firstVertices);

		std::vector<VertexBufferView> m_vbvs;
	};

	//--------------------------------------------------------------------------------------
	// Index buffer
	//--------------------------------------------------------------------------------------
	class IndexBuffer_DX12 :
		public virtual IndexBuffer,
		public virtual TypedBuffer_DX12
	{
	public:
		IndexBuffer_DX12();
		virtual ~IndexBuffer_DX12();

		bool Create(const Device* pDevice, size_t byteWidth, Format format = Format::R32_UINT,
			ResourceFlag resourceFlags = ResourceFlag::DENY_SHADER_RESOURCE,
			MemoryType memoryType = MemoryType::DEFAULT,
			uint32_t numIBVs = 1, const size_t* ibvByteOffsets = nullptr,
			uint32_t numSRVs = 1, const uint32_t* firstSrvElements = nullptr,
			uint32_t numUAVs = 1, const uint32_t* firstUavElements = nullptr,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr,
			uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
			const size_t* counterByteOffsets = nullptr, uint32_t maxThreads = 1);

		void CreateIBV(IndexBufferView& ibv, Format format, uint32_t byteSize, size_t byteOffset = 0);

		const IndexBufferView& GetIBV(uint32_t index = 0) const;

	protected:
		std::vector<IndexBufferView> m_ibvs;
	};
}
