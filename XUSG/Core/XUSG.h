//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#pragma warning(disable:4250)

#include "XUSG_DX12.h"

#define C_RETURN(x, r)		if (x) return r
#define N_RETURN(x, r)		C_RETURN(!(x), r)
#define X_RETURN(x, f, r)	{ x = f; N_RETURN(x, r); }

#define DIV_UP(x, n)		(((x) - 1) / (n) + 1)
#define SizeOfInUint32(obj)	DIV_UP(sizeof(obj), sizeof(uint32_t))

namespace XUSG
{
	enum API
	{
		API_DIRECTX12
	};

	//--------------------------------------------------------------------------------------
	// Command list
	//--------------------------------------------------------------------------------------
	class DLL_EXPORT CommandList
	{
	public:
		//CommandList();
		virtual ~CommandList() {};

		virtual bool Close() const = 0;
		virtual bool Reset(const CommandAllocator& allocator,
			const Pipeline& initialState) const = 0;

		virtual void ClearState(const Pipeline& initialState) const = 0;
		virtual void Draw(
			uint32_t vertexCountPerInstance,
			uint32_t instanceCount,
			uint32_t startVertexLocation,
			uint32_t startInstanceLocation) const = 0;
		virtual void DrawIndexed(
			uint32_t indexCountPerInstance,
			uint32_t instanceCount,
			uint32_t startIndexLocation,
			int32_t baseVertexLocation,
			uint32_t startInstanceLocation) const = 0;
		virtual void Dispatch(
			uint32_t threadGroupCountX,
			uint32_t threadGroupCountY,
			uint32_t threadGroupCountZ) const = 0;
		virtual void CopyBufferRegion(const Resource& dstBuffer, uint64_t dstOffset,
			const Resource& srcBuffer, uint64_t srcOffset, uint64_t numBytes) const = 0;
		virtual void CopyTextureRegion(const TextureCopyLocation& dst,
			uint32_t dstX, uint32_t dstY, uint32_t dstZ,
			const TextureCopyLocation& src, const BoxRange* pSrcBox = nullptr) const = 0;
		virtual void CopyResource(const Resource& dstResource, const Resource& srcResource) const = 0;
		virtual void CopyTiles(const Resource& tiledResource, const TiledResourceCoord* pTileRegionStartCoord,
			const TileRegionSize* pTileRegionSize, const Resource& buffer, uint64_t bufferStartOffsetInBytes,
			TileCopyFlags flags) const = 0;
		virtual void ResolveSubresource(const Resource& dstResource, uint32_t dstSubresource,
			const Resource& srcResource, uint32_t srcSubresource, Format format) const = 0;
		virtual void IASetPrimitiveTopology(PrimitiveTopology primitiveTopology) const = 0;
		virtual void RSSetViewports(uint32_t numViewports, const Viewport* pViewports) const = 0;
		virtual void RSSetScissorRects(uint32_t numRects, const RectRange* pRects) const = 0;
		virtual void OMSetBlendFactor(const float blendFactor[4]) const = 0;
		virtual void OMSetStencilRef(uint32_t stencilRef) const = 0;
		virtual void SetPipelineState(const Pipeline& pipelineState) const = 0;
		virtual void Barrier(uint32_t numBarriers, const ResourceBarrier* pBarriers) const = 0;
		virtual void ExecuteBundle(GraphicsCommandList& commandList) const = 0;
		virtual void SetDescriptorPools(uint32_t numDescriptorPools, const DescriptorPool* pDescriptorPools) const = 0;
		virtual void SetComputePipelineLayout(const PipelineLayout& pipelineLayout) const = 0;
		virtual void SetGraphicsPipelineLayout(const PipelineLayout& pipelineLayout) const = 0;
		virtual void SetComputeDescriptorTable(uint32_t index, const DescriptorTable& descriptorTable) const = 0;
		virtual void SetGraphicsDescriptorTable(uint32_t index, const DescriptorTable& descriptorTable) const = 0;
		virtual void SetCompute32BitConstant(uint32_t index, uint32_t srcData, uint32_t destOffsetIn32BitValues = 0) const = 0;
		virtual void SetGraphics32BitConstant(uint32_t index, uint32_t srcData, uint32_t destOffsetIn32BitValues = 0) const = 0;
		virtual void SetCompute32BitConstants(uint32_t index, uint32_t num32BitValuesToSet,
			const void* pSrcData, uint32_t destOffsetIn32BitValues = 0) const = 0;
		virtual void SetGraphics32BitConstants(uint32_t index, uint32_t num32BitValuesToSet,
			const void* pSrcData, uint32_t destOffsetIn32BitValues = 0) const = 0;
		virtual void SetComputeRootConstantBufferView(uint32_t index, const Resource& resource, int offset = 0) const = 0;
		virtual void SetGraphicsRootConstantBufferView(uint32_t index, const Resource& resource, int offset = 0) const = 0;
		virtual void SetComputeRootShaderResourceView(uint32_t index, const Resource& resource, int offset = 0) const = 0;
		virtual void SetGraphicsRootShaderResourceView(uint32_t index, const Resource& resource, int offset = 0) const = 0;
		virtual void SetComputeRootUnorderedAccessView(uint32_t index, const Resource& resource, int offset = 0) const = 0;
		virtual void SetGraphicsRootUnorderedAccessView(uint32_t index, const Resource& resource, int offset = 0) const = 0;
		virtual void IASetIndexBuffer(const IndexBufferView& view) const = 0;
		virtual void IASetVertexBuffers(uint32_t startSlot, uint32_t numViews, const VertexBufferView* pViews) const = 0;
		virtual void SOSetTargets(uint32_t startSlot, uint32_t numViews, const StreamOutBufferView* pViews) const = 0;
		virtual void OMSetFramebuffer(const Framebuffer& framebuffer) const = 0;
		virtual void OMSetRenderTargets(
			uint32_t numRenderTargetDescriptors,
			const Descriptor* pRenderTargetViews,
			const Descriptor* pDepthStencilView = nullptr,
			bool rtsSingleHandleToDescriptorRange = false) const = 0;
		virtual void ClearDepthStencilView(const Framebuffer& framebuffer, ClearFlag clearFlags,
			float depth, uint8_t stencil = 0, uint32_t numRects = 0, const RectRange* pRects = nullptr) const = 0;
		virtual void ClearDepthStencilView(const Descriptor& depthStencilView, ClearFlag clearFlags,
			float depth, uint8_t stencil = 0, uint32_t numRects = 0, const RectRange* pRects = nullptr) const = 0;
		virtual void ClearRenderTargetView(const Descriptor& renderTargetView, const float colorRGBA[4],
			uint32_t numRects = 0, const RectRange* pRects = nullptr) const = 0;
		virtual void ClearUnorderedAccessViewUint(const DescriptorTable& descriptorTable,
			const Descriptor& descriptor, const Resource& resource, const uint32_t values[4],
			uint32_t numRects = 0, const RectRange* pRects = nullptr) const = 0;
		virtual void ClearUnorderedAccessViewFloat(const DescriptorTable& descriptorTable,
			const Descriptor& descriptor, const Resource& resource, const float values[4],
			uint32_t numRects = 0, const RectRange* pRects = nullptr) const = 0;
		//virtual void DiscardResource(const Resource &resource, const D3D12_DISCARD_REGION* pRegion) const = 0;
		//virtual void BeginQuery(ID3D12QueryHeap* pQueryHeap, D3D12_QUERY_TYPE type, uint32_t index) const = 0;
		//virtual void EndQuery(ID3D12QueryHeap* pQueryHeap, D3D12_QUERY_TYPE Type, uint32_t index) const = 0;
		//virtual void ResolveQueryData(ID3D12QueryHeap* pQueryHeap, D3D12_QUERY_TYPE type, uint32_t startIndex,
			//uint32_t numQueries, const Resource &dstBuffer, uint64_t alignedDstBufferOffset) const = 0;
		//virtual void SetPredication(const Resource &buffer, uint64_t alignedBufferOffset, D3D12_PREDICATION_OP op)const = 0;
		virtual void SetMarker(uint32_t metaData, const void* pData, uint32_t size) const = 0;
		virtual void BeginEvent(uint32_t metaData, const void* pData, uint32_t size) const = 0;
		virtual void EndEvent() = 0;
		virtual void ExecuteIndirect(CommandLayout commandlayout, uint32_t maxCommandCount,
			const Resource& argumentBuffer, uint64_t argumentBufferOffset = 0,
			const Resource& countBuffer = nullptr, uint64_t countBufferOffset = 0) = 0;

		virtual GraphicsCommandList& GetCommandList() = 0;

		static std::unique_ptr<CommandList> MakeUnique(API api = API_DIRECTX12);
		static std::shared_ptr<CommandList> MakeShared(API api = API_DIRECTX12);
	};

	using CommandList_uptr = std::unique_ptr<CommandList>;
	using CommandList_sptr = std::shared_ptr<CommandList>;

	//--------------------------------------------------------------------------------------
	// Constant buffer
	//--------------------------------------------------------------------------------------
	class DLL_EXPORT ConstantBuffer
	{
	public:
		//ConstantBuffer();
		virtual ~ConstantBuffer() {};

		virtual bool Create(const Device& device, uint64_t byteWidth, uint32_t numCBVs = 1,
			const uint32_t* offsets = nullptr, MemoryType memoryType = MemoryType::UPLOAD,
			const wchar_t* name = nullptr) = 0;
		virtual bool Upload(const CommandList& commandList, Resource& uploader, const void* pData,
			size_t size, uint32_t cbvIndex = 0, ResourceState srcState = ResourceState::COMMON,
			ResourceState dstState = ResourceState::COMMON) = 0;

		virtual void* Map(uint32_t cbvIndex = 0) = 0;
		virtual void Unmap() = 0;

		virtual const Resource& GetResource() const = 0;
		virtual Descriptor		GetCBV(uint32_t index = 0) const = 0;

		static std::unique_ptr<ConstantBuffer> MakeUnique(API api = API_DIRECTX12);
		static std::shared_ptr<ConstantBuffer> MakeShared(API api = API_DIRECTX12);
	};

	using ConstantBuffer_uptr = std::unique_ptr<ConstantBuffer>;
	using ConstantBuffer_sptr = std::shared_ptr<ConstantBuffer>;

	//--------------------------------------------------------------------------------------
	// Resource base
	//--------------------------------------------------------------------------------------
	class DLL_EXPORT ResourceBase
	{
	public:
		//ResourceBase();
		virtual ~ResourceBase() {};

		virtual uint32_t SetBarrier(ResourceBarrier* pBarriers, ResourceState dstState,
			uint32_t numBarriers = 0, uint32_t subresource = BARRIER_ALL_SUBRESOURCES,
			BarrierFlag flags = BarrierFlag::NONE) = 0;

		virtual const Resource& GetResource() const = 0;
		virtual Descriptor		GetSRV(uint32_t index = 0) const = 0;

		virtual ResourceBarrier	Transition(ResourceState dstState, uint32_t subresource = BARRIER_ALL_SUBRESOURCES,
			BarrierFlag flag = BarrierFlag::NONE) = 0;
		virtual ResourceState	GetResourceState(uint32_t subresource = 0) const = 0;

		virtual Format GetFormat() const = 0;
		virtual uint32_t GetWidth() const = 0;

		static std::unique_ptr<ResourceBase> MakeUnique(API api = API_DIRECTX12);
		static std::shared_ptr<ResourceBase> MakeShared(API api = API_DIRECTX12);
	};

	using ResourceBase_uptr = std::unique_ptr<ResourceBase>;
	using ResourceBase_sptr = std::shared_ptr<ResourceBase>;

	//--------------------------------------------------------------------------------------
	// 2D Texture
	//--------------------------------------------------------------------------------------
	class DLL_EXPORT Texture2D :
		public virtual ResourceBase
	{
	public:
		//Texture2D();
		virtual ~Texture2D() {};

		virtual bool Create(const Device& device, uint32_t width, uint32_t height, Format format,
			uint32_t arraySize = 1, ResourceFlag resourceFlags = ResourceFlag::NONE,
			uint8_t numMips = 1, uint8_t sampleCount = 1, MemoryType memoryType = MemoryType::DEFAULT,
			bool isCubeMap = false, const wchar_t* name = nullptr) = 0;
		virtual bool Upload(const CommandList& commandList, Resource& uploader,
			SubresourceData* pSubresourceData, uint32_t numSubresources = 1,
			ResourceState dstState = ResourceState::COMMON, uint32_t firstSubresource = 0) = 0;
		virtual bool Upload(const CommandList& commandList, Resource& uploader, const void* pData,
			uint8_t stride = sizeof(float), ResourceState dstState = ResourceState::COMMON) = 0;
		virtual bool CreateSRVs(uint32_t arraySize, Format format = Format::UNKNOWN, uint8_t numMips = 1,
			uint8_t sampleCount = 1, bool isCubeMap = false) = 0;
		virtual bool CreateSRVLevels(uint32_t arraySize, uint8_t numMips, Format format = Format::UNKNOWN,
			uint8_t sampleCount = 1, bool isCubeMap = false) = 0;
		virtual bool CreateUAVs(uint32_t arraySize, Format format = Format::UNKNOWN, uint8_t numMips = 1,
			std::vector<Descriptor>* pUavs = nullptr) = 0;

		virtual uint32_t SetBarrier(ResourceBarrier* pBarriers, ResourceState dstState,
			uint32_t numBarriers = 0, uint32_t subresource = BARRIER_ALL_SUBRESOURCES,
			BarrierFlag flags = BarrierFlag::NONE) = 0;
		virtual uint32_t SetBarrier(ResourceBarrier* pBarriers, uint8_t mipLevel, ResourceState dstState,
			uint32_t numBarriers = 0, uint32_t slice = 0, BarrierFlag flags = BarrierFlag::NONE) = 0;

		virtual void Blit(const CommandList& commandList, uint32_t groupSizeX, uint32_t groupSizeY,
			uint32_t groupSizeZ, const DescriptorTable& uavSrvTable, uint32_t uavSrvSlot = 0,
			uint8_t mipLevel = 0, const DescriptorTable& srvTable = nullptr, uint32_t srvSlot = 0,
			const DescriptorTable& samplerTable = nullptr, uint32_t samplerSlot = 1,
			const Pipeline& pipeline = nullptr) = 0;

		virtual uint32_t Blit(const CommandList& commandList, ResourceBarrier* pBarriers, uint32_t groupSizeX,
			uint32_t groupSizeY, uint32_t groupSizeZ, uint8_t mipLevel, int8_t srcMipLevel,
			ResourceState srcState, const DescriptorTable& uavSrvTable, uint32_t uavSrvSlot = 0,
			uint32_t numBarriers = 0, const DescriptorTable& srvTable = nullptr,
			uint32_t srvSlot = 0, uint32_t baseSlice = 0, uint32_t numSlices = 0) = 0;
		virtual uint32_t GenerateMips(const CommandList& commandList, ResourceBarrier* pBarriers, uint32_t groupSizeX,
			uint32_t groupSizeY, uint32_t groupSizeZ, ResourceState dstState, const PipelineLayout& pipelineLayout,
			const Pipeline& pipeline, const DescriptorTable* pUavSrvTables, uint32_t uavSrvSlot = 0,
			const DescriptorTable& samplerTable = nullptr, uint32_t samplerSlot = 1, uint32_t numBarriers = 0,
			const DescriptorTable* pSrvTables = nullptr, uint32_t srvSlot = 0, uint8_t baseMip = 1,
			uint8_t numMips = 0, uint32_t baseSlice = 0, uint32_t numSlices = 0) = 0;

		virtual Descriptor GetUAV(uint8_t index = 0) const = 0;
		virtual Descriptor GetPackedUAV(uint8_t index = 0) const = 0;
		virtual Descriptor GetSRVLevel(uint8_t level) const = 0;

		virtual uint32_t GetHeight() const = 0;

		static std::unique_ptr<Texture2D> MakeUnique(API api = API_DIRECTX12);
		static std::shared_ptr<Texture2D> MakeShared(API api = API_DIRECTX12);
	};

	using Texture2D_uptr = std::unique_ptr<Texture2D>;
	using Texture2D_sptr = std::shared_ptr<Texture2D>;

	//--------------------------------------------------------------------------------------
	// Render target
	//--------------------------------------------------------------------------------------
	class DLL_EXPORT RenderTarget :
		public virtual Texture2D
	{
	public:
		//RenderTarget();
		virtual ~RenderTarget() {};

		virtual bool Create(const Device& device, uint32_t width, uint32_t height, Format format,
			uint32_t arraySize = 1, ResourceFlag resourceFlags = ResourceFlag::NONE,
			uint8_t numMips = 1, uint8_t sampleCount = 1, const float* pClearColor = nullptr,
			bool isCubeMap = false, const wchar_t* name = nullptr) = 0;
		virtual bool CreateArray(const Device& device, uint32_t width, uint32_t height, uint32_t arraySize,
			Format format, ResourceFlag resourceFlags = ResourceFlag::NONE, uint8_t numMips = 1,
			uint8_t sampleCount = 1, const float* pClearColor = nullptr, bool isCubeMap = false,
			const wchar_t* name = nullptr) = 0;
		virtual bool CreateFromSwapChain(const Device& device, const SwapChain& swapChain, uint32_t bufferIndex) = 0;

		virtual void Blit(const CommandList& commandList, const DescriptorTable& srcSrvTable,
			uint32_t srcSlot = 0, uint8_t mipLevel = 0, uint32_t baseSlice = 0,
			uint32_t numSlices = 0, const DescriptorTable& samplerTable = nullptr,
			uint32_t samplerSlot = 1, const Pipeline& pipeline = nullptr,
			uint32_t offsetForSliceId = 0, uint32_t cbSlot = 2) = 0;

		virtual uint32_t Blit(const CommandList& commandList, ResourceBarrier* pBarriers, uint8_t mipLevel,
			int8_t srcMipLevel, ResourceState srcState, const DescriptorTable& srcSrvTable,
			uint32_t srcSlot = 0, uint32_t numBarriers = 0, uint32_t baseSlice = 0, uint32_t numSlices = 0,
			uint32_t offsetForSliceId = 0, uint32_t cbSlot = 2) = 0;
		virtual uint32_t GenerateMips(const CommandList& commandList, ResourceBarrier* pBarriers, ResourceState dstState,
			const PipelineLayout& pipelineLayout, const Pipeline& pipeline, const DescriptorTable* pSrcSrvTables,
			uint32_t srcSlot = 0, const DescriptorTable& samplerTable = nullptr, uint32_t samplerSlot = 1,
			uint32_t numBarriers = 0, uint8_t baseMip = 1, uint8_t numMips = 0, uint32_t baseSlice = 0,
			uint32_t numSlices = 0, uint32_t offsetForSliceId = 0, uint32_t cbSlot = 2) = 0;

		virtual Descriptor	GetRTV(uint32_t slice = 0, uint8_t mipLevel = 0) const = 0;
		virtual uint32_t	GetArraySize() const = 0;
		virtual uint8_t		GetNumMips(uint32_t slice = 0) const = 0;

		static std::unique_ptr<RenderTarget> MakeUnique(API api = API_DIRECTX12);
		static std::shared_ptr<RenderTarget> MakeShared(API api = API_DIRECTX12);
	};

	using RenderTarget_uptr = std::unique_ptr<RenderTarget>;
	using RenderTarget_sptr = std::shared_ptr<RenderTarget>;

	//--------------------------------------------------------------------------------------
	// Depth stencil
	//--------------------------------------------------------------------------------------
	class DLL_EXPORT DepthStencil :
		public virtual Texture2D
	{
	public:
		//DepthStencil();
		virtual ~DepthStencil() {};

		virtual bool Create(const Device& device, uint32_t width, uint32_t height,
			Format format = Format::UNKNOWN, ResourceFlag resourceFlags = ResourceFlag::NONE,
			uint32_t arraySize = 1, uint8_t numMips = 1, uint8_t sampleCount = 1,
			float clearDepth = 1.0f, uint8_t clearStencil = 0, bool isCubeMap = false,
			const wchar_t* name = nullptr) = 0;
		virtual bool CreateArray(const Device& device, uint32_t width, uint32_t height, uint32_t arraySize,
			Format format = Format::UNKNOWN, ResourceFlag resourceFlags = ResourceFlag::NONE,
			uint8_t numMips = 1, uint8_t sampleCount = 1, float clearDepth = 1.0f,
			uint8_t clearStencil = 0, bool isCubeMap = false, const wchar_t* name = nullptr) = 0;

		virtual Descriptor GetDSV(uint32_t slice = 0, uint8_t mipLevel = 0) const = 0;
		virtual Descriptor GetReadOnlyDSV(uint32_t slice = 0, uint8_t mipLevel = 0) const = 0;
		virtual const Descriptor& GetStencilSRV() const = 0;

		virtual Format		GetDSVFormat() const = 0;
		virtual uint32_t	GetArraySize() const = 0;
		virtual uint8_t		GetNumMips() const = 0;

		static std::unique_ptr<DepthStencil> MakeUnique(API api = API_DIRECTX12);
		static std::shared_ptr<DepthStencil> MakeShared(API api = API_DIRECTX12);
	};

	using DepthStencil_uptr = std::unique_ptr<DepthStencil>;
	using DepthStencil_sptr = std::shared_ptr<DepthStencil>;

	//--------------------------------------------------------------------------------------
	// 3D Texture
	//--------------------------------------------------------------------------------------
	class DLL_EXPORT Texture3D :
		public virtual Texture2D
	{
	public:
		//Texture3D();
		virtual ~Texture3D() {};

		virtual bool Create(const Device& device, uint32_t width, uint32_t height, uint32_t depth,
			Format format, ResourceFlag resourceFlags = ResourceFlag::NONE, uint8_t numMips = 1,
			MemoryType memoryType = MemoryType::DEFAULT, const wchar_t* name = nullptr) = 0;
		virtual bool CreateSRVs(Format format = Format::UNKNOWN, uint8_t numMips = 1) = 0;
		virtual bool CreateSRVLevels(uint8_t numMips, Format format = Format::UNKNOWN) = 0;
		virtual bool CreateUAVs(Format format = Format::UNKNOWN, uint8_t numMips = 1,
			std::vector<Descriptor>* pUavs = nullptr) = 0;

		virtual uint32_t GetDepth() const = 0;

		static std::unique_ptr<Texture3D> MakeUnique(API api = API_DIRECTX12);
		static std::shared_ptr<Texture3D> MakeShared(API api = API_DIRECTX12);
	};

	using Texture3D_uptr = std::unique_ptr<Texture3D>;
	using Texture3D_sptr = std::shared_ptr<Texture3D>;

	//--------------------------------------------------------------------------------------
	// Raw buffer
	//--------------------------------------------------------------------------------------
	class DLL_EXPORT RawBuffer :
		public virtual ResourceBase
	{
	public:
		//RawBuffer();
		virtual ~RawBuffer() {};

		virtual bool Create(const Device& device, uint64_t byteWidth, ResourceFlag resourceFlags = ResourceFlag::NONE,
			MemoryType memoryType = MemoryType::DEFAULT, uint32_t numSRVs = 1,
			const uint32_t* firstSRVElements = nullptr, uint32_t numUAVs = 1,
			const uint32_t* firstUAVElements = nullptr, const wchar_t* name = nullptr) = 0;
		virtual bool Upload(const CommandList& commandList, Resource& uploader, const void* pData, size_t size,
			uint32_t descriptorIndex = 0, ResourceState dstState = ResourceState::COMMON) = 0;
		virtual bool CreateSRVs(uint64_t byteWidth, const uint32_t* firstElements = nullptr,
			uint32_t numDescriptors = 1) = 0;
		virtual bool CreateUAVs(uint64_t byteWidth, const uint32_t* firstElements = nullptr,
			uint32_t numDescriptors = 1) = 0;

		virtual Descriptor GetUAV(uint32_t index = 0) const = 0;

		virtual void* Map(uint32_t descriptorIndex = 0, size_t readBegin = 0, size_t readEnd = 0) = 0;
		virtual void* Map(const Range* pReadRange, uint32_t descriptorIndex = 0) = 0;
		virtual void Unmap() = 0;

		static std::unique_ptr<RawBuffer> MakeUnique(API api = API_DIRECTX12);
		static std::shared_ptr<RawBuffer> MakeShared(API api = API_DIRECTX12);
	};

	using RawBuffer_uptr = std::unique_ptr<RawBuffer>;
	using RawBuffer_sptr = std::shared_ptr<RawBuffer>;

	//--------------------------------------------------------------------------------------
	// Structured buffer
	//--------------------------------------------------------------------------------------
	class DLL_EXPORT StructuredBuffer :
		public virtual RawBuffer
	{
	public:
		//StructuredBuffer();
		virtual ~StructuredBuffer() {};

		virtual bool Create(const Device& device, uint32_t numElements, uint32_t stride,
			ResourceFlag resourceFlags = ResourceFlag::NONE, MemoryType memoryType = MemoryType::DEFAULT,
			uint32_t numSRVs = 1, const uint32_t* firstSRVElements = nullptr,
			uint32_t numUAVs = 1, const uint32_t* firstUAVElements = nullptr,
			const wchar_t* name = nullptr, const uint64_t* counterOffsetsInBytes = nullptr) = 0;

		virtual bool CreateSRVs(uint32_t numElements, uint32_t stride,
			const uint32_t* firstElements = nullptr, uint32_t numDescriptors = 1) = 0;
		virtual bool CreateUAVs(uint32_t numElements, uint32_t stride,
			const uint32_t* firstElements = nullptr, uint32_t numDescriptors = 1,
			const uint64_t* counterOffsetsInBytes = nullptr) = 0;

		virtual void SetCounter(const Resource& counter) = 0;
		virtual Resource& GetCounter() = 0;

		static std::unique_ptr<StructuredBuffer> MakeUnique(API api = API_DIRECTX12);
		static std::shared_ptr<StructuredBuffer> MakeShared(API api = API_DIRECTX12);
	};

	using StructuredBuffer_uptr = std::unique_ptr<StructuredBuffer>;
	using StructuredBuffer_sptr = std::shared_ptr<StructuredBuffer>;

	//--------------------------------------------------------------------------------------
	// Typed buffer
	//--------------------------------------------------------------------------------------
	class DLL_EXPORT TypedBuffer :
		public virtual RawBuffer
	{
	public:
		//TypedBuffer();
		virtual ~TypedBuffer() {};

		virtual bool Create(const Device& device, uint32_t numElements, uint32_t stride, Format format,
			ResourceFlag resourceFlags = ResourceFlag::NONE, MemoryType memoryType = MemoryType::DEFAULT,
			uint32_t numSRVs = 1, const uint32_t* firstSRVElements = nullptr,
			uint32_t numUAVs = 1, const uint32_t* firstUAVElements = nullptr,
			const wchar_t* name = nullptr) = 0;

		virtual bool CreateSRVs(uint32_t numElements, Format format, uint32_t stride,
			const uint32_t* firstElements = nullptr, uint32_t numDescriptors = 1) = 0;
		virtual bool CreateUAVs(uint32_t numElements, Format format, uint32_t stride,
			const uint32_t* firstElements = nullptr, uint32_t numDescriptors = 1,
			std::vector<Descriptor>* pUavs = nullptr) = 0;

		virtual Descriptor GetPackedUAV(uint32_t index = 0) const = 0;

		static std::unique_ptr<TypedBuffer> MakeUnique(API api = API_DIRECTX12);
		static std::shared_ptr<TypedBuffer> MakeShared(API api = API_DIRECTX12);
	};

	using TypedBuffer_uptr = std::unique_ptr<TypedBuffer>;
	using TypedBuffer_sptr = std::shared_ptr<TypedBuffer>;

	//--------------------------------------------------------------------------------------
	// Vertex buffer
	//--------------------------------------------------------------------------------------
	class DLL_EXPORT VertexBuffer :
		public virtual StructuredBuffer
	{
	public:
		//VertexBuffer();
		virtual ~VertexBuffer() {};

		virtual bool Create(const Device& device, uint32_t numVertices, uint32_t stride,
			ResourceFlag resourceFlags = ResourceFlag::NONE, MemoryType memoryType = MemoryType::DEFAULT,
			uint32_t numVBVs = 1, const uint32_t* firstVertices = nullptr,
			uint32_t numSRVs = 1, const uint32_t* firstSRVElements = nullptr,
			uint32_t numUAVs = 1, const uint32_t* firstUAVElements = nullptr,
			const wchar_t* name = nullptr) = 0;
		virtual bool CreateAsRaw(const Device& device, uint32_t numVertices, uint32_t stride,
			ResourceFlag resourceFlags = ResourceFlag::NONE, MemoryType memoryType = MemoryType::DEFAULT,
			uint32_t numVBVs = 1, const uint32_t* firstVertices = nullptr,
			uint32_t numSRVs = 1, const uint32_t* firstSRVElements = nullptr,
			uint32_t numUAVs = 1, const uint32_t* firstUAVElements = nullptr,
			const wchar_t* name = nullptr) = 0;

		virtual VertexBufferView GetVBV(uint32_t index = 0) const = 0;

		static std::unique_ptr<VertexBuffer> MakeUnique(API api = API_DIRECTX12);
		static std::shared_ptr<VertexBuffer> MakeShared(API api = API_DIRECTX12);
	};

	using VertexBuffer_uptr = std::unique_ptr<VertexBuffer>;
	using VertexBuffer_sptr = std::shared_ptr<VertexBuffer>;

	//--------------------------------------------------------------------------------------
	// Index buffer
	//--------------------------------------------------------------------------------------
	class DLL_EXPORT IndexBuffer :
		public virtual TypedBuffer
	{
	public:
		//IndexBuffer();
		virtual ~IndexBuffer() {};

		virtual bool Create(const Device& device, uint64_t byteWidth, Format format = Format::R32_UINT,
			ResourceFlag resourceFlags = ResourceFlag::DENY_SHADER_RESOURCE,
			MemoryType memoryType = MemoryType::DEFAULT,
			uint32_t numIBVs = 1, const uint32_t* offsets = nullptr,
			uint32_t numSRVs = 1, const uint32_t* firstSRVElements = nullptr,
			uint32_t numUAVs = 1, const uint32_t* firstUAVElements = nullptr,
			const wchar_t* name = nullptr) = 0;

		virtual IndexBufferView GetIBV(uint32_t index = 0) const = 0;

		static std::unique_ptr<IndexBuffer> MakeUnique(API api = API_DIRECTX12);
		static std::shared_ptr<IndexBuffer> MakeShared(API api = API_DIRECTX12);
	};

	using IndexBuffer_uptr = std::unique_ptr<IndexBuffer>;
	using IndexBuffer_sptr = std::shared_ptr<IndexBuffer>;

	//--------------------------------------------------------------------------------------
	// Descriptor
	//--------------------------------------------------------------------------------------
	enum DescriptorPoolType : uint8_t
	{
		CBV_SRV_UAV_POOL,
		SAMPLER_POOL,
		RTV_POOL,

		NUM_DESCRIPTOR_POOL
	};

	enum SamplerPreset : uint8_t
	{
		POINT_WRAP,
		POINT_MIRROR,
		POINT_CLAMP,
		POINT_BORDER,
		POINT_LESS_EQUAL,

		LINEAR_WRAP,
		LINEAR_MIRROR,
		LINEAR_CLAMP,
		LINEAR_BORDER,
		LINEAR_LESS_EQUAL,

		ANISOTROPIC_WRAP,
		ANISOTROPIC_MIRROR,
		ANISOTROPIC_CLAMP,
		ANISOTROPIC_BORDER,
		ANISOTROPIC_LESS_EQUAL,

		NUM_SAMPLER_PRESET
	};

	class DescriptorTableCache;
	using DescriptorTableCache_uptr = std::unique_ptr<DescriptorTableCache>;
	using DescriptorTableCache_sptr = std::shared_ptr<DescriptorTableCache>;

	namespace Util
	{
		class DLL_EXPORT DescriptorTable
		{
		public:
			//DescriptorTable();
			virtual ~DescriptorTable() {};

			virtual void SetDescriptors(uint32_t start, uint32_t num, const Descriptor* srcDescriptors,
				uint8_t descriptorPoolIndex = 0) = 0;
			virtual void SetSamplers(uint32_t start, uint32_t num, const SamplerPreset* presets,
				DescriptorTableCache& descriptorTableCache, uint8_t descriptorPoolIndex = 0) = 0;

			virtual XUSG::DescriptorTable CreateCbvSrvUavTable(DescriptorTableCache& descriptorTableCache,
				const XUSG::DescriptorTable& table = nullptr) = 0;
			virtual XUSG::DescriptorTable GetCbvSrvUavTable(DescriptorTableCache& descriptorTableCache,
				const XUSG::DescriptorTable& table = nullptr) = 0;

			virtual XUSG::DescriptorTable CreateSamplerTable(DescriptorTableCache& descriptorTableCache,
				const XUSG::DescriptorTable& table = nullptr) = 0;
			virtual XUSG::DescriptorTable GetSamplerTable(DescriptorTableCache& descriptorTableCache,
				const XUSG::DescriptorTable& table = nullptr) = 0;

			virtual Framebuffer CreateFramebuffer(DescriptorTableCache& descriptorTableCache,
				const Descriptor* pDsv = nullptr, const Framebuffer* pFramebuffer = nullptr) = 0;
			virtual Framebuffer GetFramebuffer(DescriptorTableCache& descriptorTableCache,
				const Descriptor* pDsv = nullptr, const Framebuffer* pFramebuffer = nullptr) = 0;

			virtual const std::string& GetKey() const = 0;

			static std::unique_ptr<DescriptorTable> MakeUnique(API api = API_DIRECTX12);
			static std::shared_ptr<DescriptorTable> MakeShared(API api = API_DIRECTX12);
		};

		using DescriptorTable_uptr = std::unique_ptr<DescriptorTable>;
		using DescriptorTable_sptr = std::shared_ptr<DescriptorTable>;
	}

	class DLL_EXPORT DescriptorTableCache
	{
	public:
		//DescriptorTableCache();
		//DescriptorTableCache(const Device& device, const wchar_t* name = nullptr);
		virtual ~DescriptorTableCache() {};

		virtual void SetDevice(const Device& device) = 0;
		virtual void SetName(const wchar_t* name) = 0;
		virtual void ResetDescriptorPool(DescriptorPoolType type, uint8_t index) = 0;

		virtual bool AllocateDescriptorPool(DescriptorPoolType type, uint32_t numDescriptors, uint8_t index = 0) = 0;

		virtual DescriptorTable CreateCbvSrvUavTable(const Util::DescriptorTable& util, const DescriptorTable& table = nullptr) = 0;
		virtual DescriptorTable GetCbvSrvUavTable(const Util::DescriptorTable& util, const DescriptorTable& table = nullptr) = 0;

		virtual DescriptorTable CreateSamplerTable(const Util::DescriptorTable& util, const DescriptorTable& table = nullptr) = 0;
		virtual DescriptorTable GetSamplerTable(const Util::DescriptorTable& util, const DescriptorTable& table = nullptr) = 0;

		virtual Framebuffer CreateFramebuffer(const Util::DescriptorTable& util,
			const Descriptor* pDsv = nullptr, const Framebuffer* pFramebuffer = nullptr) = 0;
		virtual Framebuffer GetFramebuffer(const Util::DescriptorTable& util,
			const Descriptor* pDsv = nullptr, const Framebuffer* pFramebuffer = nullptr) = 0;

		virtual const DescriptorPool& GetDescriptorPool(DescriptorPoolType type, uint8_t index = 0) const = 0;

		virtual const std::shared_ptr<Sampler>& GetSampler(SamplerPreset preset) = 0;

		virtual uint32_t GetDescriptorStride(DescriptorPoolType type) const = 0;

		static std::unique_ptr<DescriptorTableCache> MakeUnique(API api = API_DIRECTX12);
		static std::shared_ptr<DescriptorTableCache> MakeShared(API api = API_DIRECTX12);
		static std::unique_ptr<DescriptorTableCache> MakeUnique(const Device& device, const wchar_t* name = nullptr, API api = API_DIRECTX12);
		static std::shared_ptr<DescriptorTableCache> MakeShared(const Device& device, const wchar_t* name = nullptr, API api = API_DIRECTX12);
	};

	//--------------------------------------------------------------------------------------
	// Shader reflector
	//--------------------------------------------------------------------------------------
	class DLL_EXPORT Reflector
	{
	public:
		//Reflector();
		virtual ~Reflector() {};

		virtual bool SetShader(const Blob& shader) = 0;
		virtual bool IsValid() const = 0;
		virtual uint32_t GetResourceBindingPointByName(const char* name, uint32_t defaultVal = UINT32_MAX) const = 0;

		static std::unique_ptr<Reflector> MakeUnique(API api = API_DIRECTX12);
		static std::shared_ptr<Reflector> MakeShared(API api = API_DIRECTX12);
	};

	using Reflector_uptr = std::unique_ptr<Reflector>;
	using Reflector_sptr = std::shared_ptr<Reflector>;

	//--------------------------------------------------------------------------------------
	// Shader
	//--------------------------------------------------------------------------------------
	namespace Shader
	{
		enum Stage : uint8_t
		{
			VS,
			PS,
			DS,
			HS,
			GS,
			CS,
			ALL = CS,

			NUM_GRAPHICS = ALL,
			NUM_STAGE,
		};
	}

	class DLL_EXPORT ShaderPool
	{
	public:
		//ShaderPool();
		virtual ~ShaderPool() {};

		virtual void SetShader(Shader::Stage stage, uint32_t index, const Blob& shader) = 0;
		virtual void SetShader(Shader::Stage stage, uint32_t index, const Blob& shader, const Reflector_sptr& reflector) = 0;
		virtual void SetReflector(Shader::Stage stage, uint32_t index, const Reflector_sptr& reflector) = 0;

		virtual Blob CreateShader(Shader::Stage stage, uint32_t index, const std::wstring& fileName) = 0;
		virtual Blob GetShader(Shader::Stage stage, uint32_t index) const = 0;
		virtual Reflector_sptr GetReflector(Shader::Stage stage, uint32_t index) const = 0;

		static std::unique_ptr<ShaderPool> MakeUnique(API api = API_DIRECTX12);
		static std::shared_ptr<ShaderPool> MakeShared(API api = API_DIRECTX12);
	};

	using ShaderPool_uptr = std::unique_ptr<ShaderPool>;
	using ShaderPool_sptr = std::shared_ptr<ShaderPool>;

	//--------------------------------------------------------------------------------------
	// Pipeline layout
	//--------------------------------------------------------------------------------------
	enum class DescriptorType : uint8_t
	{
		SRV,
		UAV,
		CBV,
		SAMPLER,
		CONSTANT,
		ROOT_SRV,
		ROOT_UAV,
		ROOT_CBV,

		NUM
	};

	class PipelineLayoutCache;
	using PipelineLayoutCache_uptr = std::unique_ptr<PipelineLayoutCache>;
	using PipelineLayoutCache_sptr = std::shared_ptr<PipelineLayoutCache>;

	namespace Util
	{
		class DLL_EXPORT PipelineLayout
		{
		public:
			//PipelineLayout();
			virtual ~PipelineLayout() {};

			virtual void SetShaderStage(uint32_t index, Shader::Stage stage) = 0;
			virtual void SetRange(uint32_t index, DescriptorType type, uint32_t num, uint32_t baseBinding,
				uint32_t space = 0, DescriptorRangeFlag flags = DescriptorRangeFlag::NONE) = 0;
			virtual void SetConstants(uint32_t index, uint32_t num32BitValues, uint32_t binding,
				uint32_t space = 0, Shader::Stage stage = Shader::Stage::ALL) = 0;
			virtual void SetRootSRV(uint32_t index, uint32_t binding, uint32_t space = 0,
				DescriptorRangeFlag flags = DescriptorRangeFlag::DATA_STATIC, Shader::Stage stage = Shader::Stage::ALL) = 0;
			virtual void SetRootUAV(uint32_t index, uint32_t binding, uint32_t space = 0,
				DescriptorRangeFlag flags = DescriptorRangeFlag::NONE, Shader::Stage stage = Shader::Stage::ALL) = 0;
			virtual void SetRootCBV(uint32_t index, uint32_t binding, uint32_t space = 0,
				DescriptorRangeFlag flags = DescriptorRangeFlag::NONE, Shader::Stage stage = Shader::Stage::ALL) = 0;

			virtual XUSG::PipelineLayout CreatePipelineLayout(PipelineLayoutCache& pipelineLayoutCache, PipelineLayoutFlag flags,
				const wchar_t* name = nullptr) = 0;
			virtual XUSG::PipelineLayout GetPipelineLayout(PipelineLayoutCache& pipelineLayoutCache, PipelineLayoutFlag flags,
				const wchar_t* name = nullptr) = 0;

			virtual DescriptorTableLayout CreateDescriptorTableLayout(uint32_t index, PipelineLayoutCache& pipelineLayoutCache) const = 0;
			virtual DescriptorTableLayout GetDescriptorTableLayout(uint32_t index, PipelineLayoutCache& pipelineLayoutCache) const = 0;

			virtual const std::vector<std::string>& GetDescriptorTableLayoutKeys() const = 0;
			virtual std::string& GetPipelineLayoutKey(PipelineLayoutCache* pPipelineLayoutCache) = 0;

			static std::unique_ptr<PipelineLayout> MakeUnique(API api = API_DIRECTX12);
			static std::shared_ptr<PipelineLayout> MakeShared(API api = API_DIRECTX12);
		};

		using PipelineLayout_uptr = std::unique_ptr<PipelineLayout>;
		using PipelineLayout_sptr = std::shared_ptr<PipelineLayout>;
	}

	class DLL_EXPORT PipelineLayoutCache
	{
	public:
		//PipelineLayoutCache();
		//PipelineLayoutCache(const Device& device) = 0;
		virtual ~PipelineLayoutCache() {};

		virtual void SetDevice(const Device& device) = 0;
		virtual void SetPipelineLayout(const std::string& key, const PipelineLayout& pipelineLayout) = 0;

		virtual PipelineLayout CreatePipelineLayout(Util::PipelineLayout& util, PipelineLayoutFlag flags,
			const wchar_t* name = nullptr) = 0;
		virtual PipelineLayout GetPipelineLayout(Util::PipelineLayout& util, PipelineLayoutFlag flags,
			const wchar_t* name = nullptr, bool create = true) = 0;

		virtual DescriptorTableLayout CreateDescriptorTableLayout(uint32_t index, const Util::PipelineLayout& util) = 0;
		virtual DescriptorTableLayout GetDescriptorTableLayout(uint32_t index, const Util::PipelineLayout& util) = 0;

		static std::unique_ptr<PipelineLayoutCache> MakeUnique(API api = API_DIRECTX12);
		static std::shared_ptr<PipelineLayoutCache> MakeShared(API api = API_DIRECTX12);
		static std::unique_ptr<PipelineLayoutCache> MakeUnique(const Device& device, API api = API_DIRECTX12);
		static std::shared_ptr<PipelineLayoutCache> MakeShared(const Device& device, API api = API_DIRECTX12);
	};

	//--------------------------------------------------------------------------------------
	// Graphics pipeline state
	//--------------------------------------------------------------------------------------
	namespace Graphics
	{
		enum BlendPreset : uint8_t
		{
			DEFAULT_OPAQUE,
			PREMULTIPLITED,
			ADDTIVE,
			NON_PRE_MUL,
			NON_PREMUL_RT0,
			ALPHA_TO_COVERAGE,
			ACCUMULATIVE,
			AUTO_NON_PREMUL,
			ZERO_ALPHA_PREMUL,
			MULTIPLITED,
			WEIGHTED_PREMUL,
			WEIGHTED_PREMUL_PER_RT,
			WEIGHTED_PER_RT,
			SELECT_MIN,
			SELECT_MAX,

			NUM_BLEND_PRESET
		};

		enum RasterizerPreset : uint8_t
		{
			CULL_BACK,
			CULL_NONE,
			CULL_FRONT,
			FILL_WIREFRAME,

			NUM_RS_PRESET
		};

		enum DepthStencilPreset : uint8_t
		{
			DEFAULT_LESS,
			DEPTH_STENCIL_NONE,
			DEPTH_READ_LESS,
			DEPTH_READ_LESS_EQUAL,
			DEPTH_READ_EQUAL,

			NUM_DS_PRESET
		};

		class PipelineCache;
		using PipelineCache_uptr = std::unique_ptr<PipelineCache>;
		using PipelineCache_sptr = std::shared_ptr<PipelineCache>;

		class DLL_EXPORT State
		{
		public:
			//State();
			virtual ~State() {};

			virtual void SetPipelineLayout(const PipelineLayout& layout) = 0;
			virtual void SetShader(Shader::Stage stage, Blob shader) = 0;

			virtual void OMSetBlendState(const Blend& blend) = 0;
			virtual void RSSetState(const Rasterizer& rasterizer) = 0;
			virtual void DSSetState(const DepthStencil& depthStencil) = 0;

			virtual void OMSetBlendState(BlendPreset preset, PipelineCache& pipelineCache, uint8_t numColorRTs = 1) = 0;
			virtual void RSSetState(RasterizerPreset preset, PipelineCache& pipelineCache) = 0;
			virtual void DSSetState(DepthStencilPreset preset, PipelineCache& pipelineCache) = 0;

			virtual void IASetInputLayout(const InputLayout& layout) = 0;
			virtual void IASetPrimitiveTopologyType(PrimitiveTopologyType type) = 0;

			virtual void OMSetNumRenderTargets(uint8_t n) = 0;
			virtual void OMSetRTVFormat(uint8_t i, Format format) = 0;
			virtual void OMSetRTVFormats(const Format* formats, uint8_t n) = 0;
			virtual void OMSetDSVFormat(Format format) = 0;

			virtual Pipeline CreatePipeline(PipelineCache& pipelineCache, const wchar_t* name = nullptr) const = 0;
			virtual Pipeline GetPipeline(PipelineCache& pipelineCache, const wchar_t* name = nullptr) const = 0;

			virtual const std::string& GetKey() const = 0;

			static std::unique_ptr<State> MakeUnique(API api = API_DIRECTX12);
			static std::shared_ptr<State> MakeShared(API api = API_DIRECTX12);
		};

		using State_uptr = std::unique_ptr<State>;
		using State_sptr = std::shared_ptr<State>;

		class DLL_EXPORT PipelineCache
		{
		public:
			//PipelineCache();
			//PipelineCache(const Device& device);
			virtual ~PipelineCache() {};

			virtual void SetDevice(const Device& device) = 0;
			virtual void SetPipeline(const std::string& key, const Pipeline& pipeline) = 0;

			virtual void SetInputLayout(uint32_t index, const InputElementTable& elementTable) = 0;
			virtual InputLayout GetInputLayout(uint32_t index) const = 0;
			virtual InputLayout CreateInputLayout(const InputElementTable& elementTable) = 0;

			virtual Pipeline CreatePipeline(const State& state, const wchar_t* name = nullptr) = 0;
			virtual Pipeline GetPipeline(const State& state, const wchar_t* name = nullptr) = 0;

			virtual const Blend& GetBlend(BlendPreset preset, uint8_t numColorRTs = 1) = 0;
			virtual const Rasterizer& GetRasterizer(RasterizerPreset preset) = 0;
			virtual const DepthStencil& GetDepthStencil(DepthStencilPreset preset) = 0;

			static std::unique_ptr<PipelineCache> MakeUnique(API api = API_DIRECTX12);
			static std::shared_ptr<PipelineCache> MakeShared(API api = API_DIRECTX12);
			static std::unique_ptr<PipelineCache> MakeUnique(const Device& device, API api = API_DIRECTX12);
			static std::shared_ptr<PipelineCache> MakeShared(const Device& device, API api = API_DIRECTX12);
		};
	}

	namespace Compute
	{
		class PipelineCache;
		using PipelineCache_uptr = std::unique_ptr<PipelineCache>;
		using PipelineCache_sptr = std::shared_ptr<PipelineCache>;

		class DLL_EXPORT State
		{
		public:
			//State();
			virtual ~State() {};

			virtual void SetPipelineLayout(const PipelineLayout& layout) = 0;
			virtual void SetShader(Blob shader) = 0;

			virtual Pipeline CreatePipeline(PipelineCache& pipelineCache, const wchar_t* name = nullptr) const = 0;
			virtual Pipeline GetPipeline(PipelineCache& pipelineCache, const wchar_t* name = nullptr) const = 0;

			virtual const std::string& GetKey() const = 0;

			static std::unique_ptr<State> MakeUnique(API api = API_DIRECTX12);
			static std::shared_ptr<State> MakeShared(API api = API_DIRECTX12);
		};

		class DLL_EXPORT PipelineCache
		{
		public:
			//PipelineCache();
			//PipelineCache(const Device& device);
			virtual ~PipelineCache() {};

			virtual void SetDevice(const Device& device) = 0;
			virtual void SetPipeline(const std::string& key, const Pipeline& pipeline) = 0;

			virtual Pipeline CreatePipeline(const State& state, const wchar_t* name = nullptr) = 0;
			virtual Pipeline GetPipeline(const State& state, const wchar_t* name = nullptr) = 0;

			static std::unique_ptr<PipelineCache> MakeUnique(API api = API_DIRECTX12);
			static std::shared_ptr<PipelineCache> MakeShared(API api = API_DIRECTX12);
			static std::unique_ptr<PipelineCache> MakeUnique(const Device& device, API api = API_DIRECTX12);
			static std::shared_ptr<PipelineCache> MakeShared(const Device& device, API api = API_DIRECTX12);
		};

		using State_uptr = std::unique_ptr<State>;
		using State_sptr = std::shared_ptr<State>;
	}
}
