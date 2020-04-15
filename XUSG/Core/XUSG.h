//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#pragma warning(disable:4250)

namespace XUSG
{
	enum class Format : uint32_t;
	enum class CommandListType : uint8_t;
	enum class InputClassification : uint8_t;
	enum class MemoryType : uint8_t;
	enum class PrimitiveTopologyType : uint8_t;
	enum class PrimitiveTopology : uint8_t;
	enum class ResourceDimension : uint8_t;
	enum class IndirectArgumentType : uint8_t;

	enum class CommandQueueFlag : uint8_t;
	enum class ResourceFlag : uint32_t;
	enum class ResourceState : uint32_t;
	enum class BarrierFlag : uint8_t;
	enum class DescriptorFlag : uint8_t;
	enum class PipelineLayoutFlag : uint8_t;
	enum class ClearFlag : uint8_t;
	enum class FenceFlag : uint8_t;

	enum Requirement : uint32_t
	{
		REQ_MIP_LEVELS,
		REQ_TEXTURECUBE_DIMENSION,
		REQ_TEXTURE1D_U_DIMENSION,
		REQ_TEXTURE2D_U_OR_V_DIMENSION,
		REQ_TEXTURE3D_U_V_OR_W_DIMENSION,
		REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION,
		REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION
	};
};

#include "XUSG_DX12.h"

#define C_RETURN(x, r)		if (x) return r
#define N_RETURN(x, r)		C_RETURN(!(x), r)
#define X_RETURN(x, f, r)	{ x = f; N_RETURN(x, r); }

#define DIV_UP(x, n)		(((x) - 1) / (n) + 1)
#define SizeOfInUint32(obj)	DIV_UP(sizeof(obj), sizeof(uint32_t))

#define APPEND_ALIGNED_ELEMENT 0xffffffff

namespace XUSG
{
	// Enumerations
	enum class API
	{
		DIRECTX_12
	};

	enum class Format : uint32_t
	{
		UNKNOWN,
		R32G32B32A32_TYPELESS,
		R32G32B32A32_FLOAT,
		R32G32B32A32_UINT,
		R32G32B32A32_SINT,
		R32G32B32_TYPELESS,
		R32G32B32_FLOAT,
		R32G32B32_UINT,
		R32G32B32_SINT,
		R16G16B16A16_TYPELESS,
		R16G16B16A16_FLOAT,
		R16G16B16A16_UNORM,
		R16G16B16A16_UINT,
		R16G16B16A16_SNORM,
		R16G16B16A16_SINT,
		R32G32_TYPELESS,
		R32G32_FLOAT,
		R32G32_UINT,
		R32G32_SINT,
		R32G8X24_TYPELESS,
		D32_FLOAT_S8X24_UINT,
		R32_FLOAT_X8X24_TYPELESS,
		X32_TYPELESS_G8X24_UINT,
		R10G10B10A2_TYPELESS,
		R10G10B10A2_UNORM,
		R10G10B10A2_UINT,
		R11G11B10_FLOAT,
		R8G8B8A8_TYPELESS,
		R8G8B8A8_UNORM,
		R8G8B8A8_UNORM_SRGB,
		R8G8B8A8_UINT,
		R8G8B8A8_SNORM,
		R8G8B8A8_SINT,
		R16G16_TYPELESS,
		R16G16_FLOAT,
		R16G16_UNORM,
		R16G16_UINT,
		R16G16_SNORM,
		R16G16_SINT,
		R32_TYPELESS,
		D32_FLOAT,
		R32_FLOAT,
		R32_UINT,
		R32_SINT,
		R24G8_TYPELESS,
		D24_UNORM_S8_UINT,
		R24_UNORM_X8_TYPELESS,
		X24_TYPELESS_G8_UINT,
		R8G8_TYPELESS,
		R8G8_UNORM,
		R8G8_UINT,
		R8G8_SNORM,
		R8G8_SINT,
		R16_TYPELESS,
		R16_FLOAT,
		D16_UNORM,
		R16_UNORM,
		R16_UINT,
		R16_SNORM,
		R16_SINT,
		R8_TYPELESS,
		R8_UNORM,
		R8_UINT,
		R8_SNORM,
		R8_SINT,
		A8_UNORM,
		R1_UNORM,
		R9G9B9E5_SHAREDEXP,
		R8G8_B8G8_UNORM,
		G8R8_G8B8_UNORM,
		BC1_TYPELESS,
		BC1_UNORM,
		BC1_UNORM_SRGB,
		BC2_TYPELESS,
		BC2_UNORM,
		BC2_UNORM_SRGB,
		BC3_TYPELESS,
		BC3_UNORM,
		BC3_UNORM_SRGB,
		BC4_TYPELESS,
		BC4_UNORM,
		BC4_SNORM,
		BC5_TYPELESS,
		BC5_UNORM,
		BC5_SNORM,
		B5G6R5_UNORM,
		B5G5R5A1_UNORM,
		B8G8R8A8_UNORM,
		B8G8R8X8_UNORM,
		RGB10_XR_BIAS_A2_UNORM,
		B8G8R8A8_TYPELESS,
		B8G8R8A8_UNORM_SRGB,
		B8G8R8X8_TYPELESS,
		B8G8R8X8_UNORM_SRGB,
		BC6H_TYPELESS,
		BC6H_UF16,
		BC6H_SF16,
		BC7_TYPELESS,
		BC7_UNORM,
		BC7_UNORM_SRGB,
		AYUV,
		Y410,
		Y416,
		NV12,
		P010,
		P016,
		OPAQUE_420,
		YUY2,
		Y210,
		Y216,
		NV11,
		AI44,
		IA44,
		P8,
		A8P8,
		B4G4R4A4_UNORM,

		P208,
		V208,
		V408,

		FORCE_UINT
	};

	enum class CommandListType : uint8_t
	{
		DIRECT,
		BUNDLE,
		COMPUTE,
		COPY,
		VIDEO_DECODE,
		VIDEO_PROCESS,
		VIDEO_ENCODE
	};

	enum class InputClassification : uint8_t
	{
		PER_VERTEX_DATA,
		PER_INSTANCE_DATA
	};

	enum class MemoryType : uint8_t
	{
		DEFAULT,
		UPLOAD,
		READBACK,
		CUSTOM
	};

	enum class PrimitiveTopologyType : uint8_t
	{
		UNDEFINED,
		POINT,
		LINE,
		TRIANGLE,
		PATCH
	};

	enum class PrimitiveTopology : uint8_t
	{
		UNDEFINED,
		POINTLIST,
		LINELIST,
		LINESTRIP,
		TRIANGLELIST,
		TRIANGLESTRIP,
		LINELIST_ADJ,
		LINESTRIP_ADJ,
		TRIANGLELIST_ADJ,
		TRIANGLESTRIP_ADJ,
		CONTROL_POINT1_PATCHLIST,
		CONTROL_POINT2_PATCHLIST,
		CONTROL_POINT3_PATCHLIST,
		CONTROL_POINT4_PATCHLIST,
		CONTROL_POINT5_PATCHLIST,
		CONTROL_POINT6_PATCHLIST,
		CONTROL_POINT7_PATCHLIST,
		CONTROL_POINT8_PATCHLIST,
		CONTROL_POINT9_PATCHLIST,
		CONTROL_POINT10_PATCHLIST,
		CONTROL_POINT11_PATCHLIST,
		CONTROL_POINT12_PATCHLIST,
		CONTROL_POINT13_PATCHLIST,
		CONTROL_POINT14_PATCHLIST,
		CONTROL_POINT15_PATCHLIST,
		CONTROL_POINT16_PATCHLIST,
		CONTROL_POINT17_PATCHLIST,
		CONTROL_POINT18_PATCHLIST,
		CONTROL_POINT19_PATCHLIST,
		CONTROL_POINT20_PATCHLIST,
		CONTROL_POINT21_PATCHLIST,
		CONTROL_POINT22_PATCHLIST,
		CONTROL_POINT23_PATCHLIST,
		CONTROL_POINT24_PATCHLIST,
		CONTROL_POINT25_PATCHLIST,
		CONTROL_POINT26_PATCHLIST,
		CONTROL_POINT27_PATCHLIST,
		CONTROL_POINT28_PATCHLIST,
		CONTROL_POINT29_PATCHLIST,
		CONTROL_POINT30_PATCHLIST,
		CONTROL_POINT31_PATCHLIST,
		CONTROL_POINT32_PATCHLIST
	};

	enum class ResourceDimension : uint8_t
	{
		UNKNOWN,
		BUFFER,
		TEXTURE1D,
		TEXTURE2D,
		TEXTURE3D
	};

	enum class IndirectArgumentType : uint8_t
	{
		DRAW,
		DRAW_INDEXED,
		DISPATCH,
		VERTEX_BUFFER_VIEW,
		INDEX_BUFFER_VIEW,
		CONSTANT,
		CONSTANT_BUFFER_VIEW,
		SHADER_RESOURCE_VIEW,
		UNORDERED_ACCESS_VIEW
	};

	enum class ResourceFlag : uint32_t
	{
		NONE = 0,
		ALLOW_RENDER_TARGET = (1 << 0),
		ALLOW_DEPTH_STENCIL = (1 << 1),
		ALLOW_UNORDERED_ACCESS = (1 << 2),
		DENY_SHADER_RESOURCE = (1 << 3),
		ALLOW_CROSS_ADAPTER = (1 << 4),
		ALLOW_SIMULTANEOUS_ACCESS = (1 << 5),
		VIDEO_DECODE_REFERENCE_ONLY = (1 << 6),
		NEED_PACKED_UAV = ALLOW_UNORDERED_ACCESS | 0x8000,
		ACCELERATION_STRUCTURE = ALLOW_UNORDERED_ACCESS | 0x400000
	};

	DEFINE_ENUM_FLAG_OPERATORS(ResourceFlag);

	enum class ResourceState : uint32_t
	{
		COMMON = 0,
		VERTEX_AND_CONSTANT_BUFFER = (1 << 0),
		INDEX_BUFFER = (1 << 1),
		RENDER_TARGET = (1 << 2),
		UNORDERED_ACCESS = (1 << 3),
		DEPTH_WRITE = (1 << 4),
		DEPTH_READ = (1 << 5),
		NON_PIXEL_SHADER_RESOURCE = (1 << 6),
		PIXEL_SHADER_RESOURCE = (1 << 7),
		STREAM_OUT = (1 << 8),
		INDIRECT_ARGUMENT = (1 << 9),
		COPY_DEST = (1 << 10),
		COPY_SOURCE = (1 << 11),
		RESOLVE_DEST = (1 << 12),
		RESOLVE_SOURCE = (1 << 13),
		PREDICATION = (1 << 14),
		RAYTRACING_ACCELERATION_STRUCTURE = (1 << 15),
		SHADING_RATE_SOURCE = (1 << 16),

		GENERAL_READ = (VERTEX_AND_CONSTANT_BUFFER | INDEX_BUFFER | NON_PIXEL_SHADER_RESOURCE | PIXEL_SHADER_RESOURCE | INDIRECT_ARGUMENT | COPY_SOURCE | PREDICATION),
		PRESENT = 0,

		VIDEO_DECODE_READ = (1 << 17),
		VIDEO_DECODE_WRITE = (1 << 18),
		VIDEO_PROCESS_READ = (1 << 19),
		VIDEO_PROCESS_WRITE = (1 << 20),
		VIDEO_ENCODE_READ = (1 << 21),
		VIDEO_ENCODE_WRITE = (1 << 22)
	};

	DEFINE_ENUM_FLAG_OPERATORS(ResourceState);

	enum class BarrierFlag : uint8_t
	{
		NONE = 0,
		BEGIN_ONLY = (1 << 0),
		END_ONLY = (1 << 1)
	};

	DEFINE_ENUM_FLAG_OPERATORS(BarrierFlag);

	enum class DescriptorFlag : uint8_t
	{
		NONE = 0,
		DESCRIPTORS_VOLATILE = (1 << 0),
		DATA_VOLATILE = (1 << 1),
		DATA_STATIC_WHILE_SET_AT_EXECUTE = (1 << 2),
		DATA_STATIC = (1 << 3),
		DESCRIPTORS_STATIC_KEEPING_BUFFER_BOUNDS_CHECKS = (1 << 4)
	};

	DEFINE_ENUM_FLAG_OPERATORS(DescriptorFlag);

	enum class PipelineLayoutFlag : uint8_t
	{
		NONE = 0,
		ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT = (1 << 0),
		DENY_VERTEX_SHADER_ROOT_ACCESS = (1 << 1),
		DENY_HULL_SHADER_ROOT_ACCESS = (1 << 2),
		DENY_DOMAIN_SHADER_ROOT_ACCESS = (1 << 3),
		DENY_GEOMETRY_SHADER_ROOT_ACCESS = (1 << 4),
		DENY_PIXEL_SHADER_ROOT_ACCESS = (1 << 5),
		ALLOW_STREAM_OUTPUT = (1 << 6),
		LOCAL_PIPELINE_LAYOUT = (1 << 7)
	};

	DEFINE_ENUM_FLAG_OPERATORS(PipelineLayoutFlag);

	enum class CommandQueueFlag : uint8_t
	{
		NONE = 0,
		DISABLE_GPU_TIMEOUT = (1 << 0)
	};

	DEFINE_ENUM_FLAG_OPERATORS(CommandQueueFlag);

	enum class ClearFlag : uint8_t
	{
		NONE = 0,
		DEPTH = (1 << 0),
		STENCIL = (1 << 1)
	};

	DEFINE_ENUM_FLAG_OPERATORS(ClearFlag);

	enum class FenceFlag : uint8_t
	{
		NONE = 0,
		SHARED = (1 << 0),
		SHARED_CROSS_ADAPTER = (1 << 1),
		NON_MONITORED = (1 << 2)
	};

	DEFINE_ENUM_FLAG_OPERATORS(FenceFlag);

	//--------------------------------------------------------------------------------------
	// Command list
	//--------------------------------------------------------------------------------------
	class DLL_INTERFACE CommandList
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

		using uptr = std::unique_ptr<CommandList>;
		using sptr = std::shared_ptr<CommandList>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Constant buffer
	//--------------------------------------------------------------------------------------
	class DLL_INTERFACE ConstantBuffer
	{
	public:
		//ConstantBuffer();
		virtual ~ConstantBuffer() {};

		virtual bool Create(const Device& device, uint64_t byteWidth, uint32_t numCBVs = 1,
			const uint32_t* offsets = nullptr, MemoryType memoryType = MemoryType::UPLOAD,
			const wchar_t* name = nullptr) = 0;
		virtual bool Upload(CommandList* pCommandList, Resource& uploader, const void* pData,
			size_t size, uint32_t cbvIndex = 0, ResourceState srcState = ResourceState::COMMON,
			ResourceState dstState = ResourceState::COMMON) = 0;

		virtual void* Map(uint32_t cbvIndex = 0) = 0;
		virtual void Unmap() = 0;

		virtual const Resource& GetResource() const = 0;
		virtual Descriptor		GetCBV(uint32_t index = 0) const = 0;

		using uptr = std::unique_ptr<ConstantBuffer>;
		using sptr = std::shared_ptr<ConstantBuffer>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Resource base
	//--------------------------------------------------------------------------------------
	class DLL_INTERFACE ResourceBase
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

		using uptr = std::unique_ptr<ResourceBase>;
		using sptr = std::shared_ptr<ResourceBase>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// 2D Texture
	//--------------------------------------------------------------------------------------
	class DLL_INTERFACE Texture2D :
		public virtual ResourceBase
	{
	public:
		//Texture2D();
		virtual ~Texture2D() {};

		virtual bool Create(const Device& device, uint32_t width, uint32_t height, Format format,
			uint32_t arraySize = 1, ResourceFlag resourceFlags = ResourceFlag::NONE,
			uint8_t numMips = 1, uint8_t sampleCount = 1, MemoryType memoryType = MemoryType::DEFAULT,
			bool isCubeMap = false, const wchar_t* name = nullptr) = 0;
		virtual bool Upload(CommandList* pCommandList, Resource& uploader,
			SubresourceData* pSubresourceData, uint32_t numSubresources = 1,
			ResourceState dstState = ResourceState::COMMON, uint32_t firstSubresource = 0) = 0;
		virtual bool Upload(CommandList* pCommandList, Resource& uploader, const void* pData,
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

		virtual void Blit(const CommandList* pCommandList, uint32_t groupSizeX, uint32_t groupSizeY,
			uint32_t groupSizeZ, const DescriptorTable& uavSrvTable, uint32_t uavSrvSlot = 0,
			uint8_t mipLevel = 0, const DescriptorTable& srvTable = nullptr, uint32_t srvSlot = 0,
			const DescriptorTable& samplerTable = nullptr, uint32_t samplerSlot = 1,
			const Pipeline& pipeline = nullptr) = 0;

		virtual uint32_t Blit(const CommandList* pCommandList, ResourceBarrier* pBarriers, uint32_t groupSizeX,
			uint32_t groupSizeY, uint32_t groupSizeZ, uint8_t mipLevel, int8_t srcMipLevel,
			ResourceState srcState, const DescriptorTable& uavSrvTable, uint32_t uavSrvSlot = 0,
			uint32_t numBarriers = 0, const DescriptorTable& srvTable = nullptr,
			uint32_t srvSlot = 0, uint32_t baseSlice = 0, uint32_t numSlices = 0) = 0;
		virtual uint32_t GenerateMips(const CommandList* pCommandList, ResourceBarrier* pBarriers, uint32_t groupSizeX,
			uint32_t groupSizeY, uint32_t groupSizeZ, ResourceState dstState, const PipelineLayout& pipelineLayout,
			const Pipeline& pipeline, const DescriptorTable* pUavSrvTables, uint32_t uavSrvSlot = 0,
			const DescriptorTable& samplerTable = nullptr, uint32_t samplerSlot = 1, uint32_t numBarriers = 0,
			const DescriptorTable* pSrvTables = nullptr, uint32_t srvSlot = 0, uint8_t baseMip = 1,
			uint8_t numMips = 0, uint32_t baseSlice = 0, uint32_t numSlices = 0) = 0;

		virtual Descriptor GetUAV(uint8_t index = 0) const = 0;
		virtual Descriptor GetPackedUAV(uint8_t index = 0) const = 0;
		virtual Descriptor GetSRVLevel(uint8_t level) const = 0;

		virtual uint32_t GetHeight() const = 0;

		Texture2D* AsTexture2D();

		using uptr = std::unique_ptr<Texture2D>;
		using sptr = std::shared_ptr<Texture2D>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Render target
	//--------------------------------------------------------------------------------------
	class DLL_INTERFACE RenderTarget :
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

		virtual void Blit(const CommandList* pCommandList, const DescriptorTable& srcSrvTable,
			uint32_t srcSlot = 0, uint8_t mipLevel = 0, uint32_t baseSlice = 0,
			uint32_t numSlices = 0, const DescriptorTable& samplerTable = nullptr,
			uint32_t samplerSlot = 1, const Pipeline& pipeline = nullptr,
			uint32_t offsetForSliceId = 0, uint32_t cbSlot = 2) = 0;

		virtual uint32_t Blit(const CommandList* pCommandList, ResourceBarrier* pBarriers, uint8_t mipLevel,
			int8_t srcMipLevel, ResourceState srcState, const DescriptorTable& srcSrvTable,
			uint32_t srcSlot = 0, uint32_t numBarriers = 0, uint32_t baseSlice = 0, uint32_t numSlices = 0,
			uint32_t offsetForSliceId = 0, uint32_t cbSlot = 2) = 0;
		virtual uint32_t GenerateMips(const CommandList* pCommandList, ResourceBarrier* pBarriers, ResourceState dstState,
			const PipelineLayout& pipelineLayout, const Pipeline& pipeline, const DescriptorTable* pSrcSrvTables,
			uint32_t srcSlot = 0, const DescriptorTable& samplerTable = nullptr, uint32_t samplerSlot = 1,
			uint32_t numBarriers = 0, uint8_t baseMip = 1, uint8_t numMips = 0, uint32_t baseSlice = 0,
			uint32_t numSlices = 0, uint32_t offsetForSliceId = 0, uint32_t cbSlot = 2) = 0;

		virtual Descriptor	GetRTV(uint32_t slice = 0, uint8_t mipLevel = 0) const = 0;
		virtual uint32_t	GetArraySize() const = 0;
		virtual uint8_t		GetNumMips(uint32_t slice = 0) const = 0;

		using uptr = std::unique_ptr<RenderTarget>;
		using sptr = std::shared_ptr<RenderTarget>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Depth stencil
	//--------------------------------------------------------------------------------------
	class DLL_INTERFACE DepthStencil :
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

		using uptr = std::unique_ptr<DepthStencil>;
		using sptr = std::shared_ptr<DepthStencil>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// 3D Texture
	//--------------------------------------------------------------------------------------
	class DLL_INTERFACE Texture3D :
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

		using uptr = std::unique_ptr<Texture3D>;
		using sptr = std::shared_ptr<Texture3D>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Raw buffer
	//--------------------------------------------------------------------------------------
	class DLL_INTERFACE RawBuffer :
		public virtual ResourceBase
	{
	public:
		//RawBuffer();
		virtual ~RawBuffer() {};

		virtual bool Create(const Device& device, uint64_t byteWidth, ResourceFlag resourceFlags = ResourceFlag::NONE,
			MemoryType memoryType = MemoryType::DEFAULT, uint32_t numSRVs = 1,
			const uint32_t* firstSRVElements = nullptr, uint32_t numUAVs = 1,
			const uint32_t* firstUAVElements = nullptr, const wchar_t* name = nullptr) = 0;
		virtual bool Upload(CommandList* pCommandList, Resource& uploader, const void* pData, size_t size,
			uint32_t descriptorIndex = 0, ResourceState dstState = ResourceState::COMMON) = 0;
		virtual bool CreateSRVs(uint64_t byteWidth, const uint32_t* firstElements = nullptr,
			uint32_t numDescriptors = 1) = 0;
		virtual bool CreateUAVs(uint64_t byteWidth, const uint32_t* firstElements = nullptr,
			uint32_t numDescriptors = 1) = 0;

		virtual Descriptor GetUAV(uint32_t index = 0) const = 0;

		virtual void* Map(uint32_t descriptorIndex = 0, size_t readBegin = 0, size_t readEnd = 0) = 0;
		virtual void* Map(const Range* pReadRange, uint32_t descriptorIndex = 0) = 0;
		virtual void Unmap() = 0;

		using uptr = std::unique_ptr<RawBuffer>;
		using sptr = std::shared_ptr<RawBuffer>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Structured buffer
	//--------------------------------------------------------------------------------------
	class DLL_INTERFACE StructuredBuffer :
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

		using uptr = std::unique_ptr<StructuredBuffer>;
		using sptr = std::shared_ptr<StructuredBuffer>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Typed buffer
	//--------------------------------------------------------------------------------------
	class DLL_INTERFACE TypedBuffer :
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

		using uptr = std::unique_ptr<TypedBuffer>;
		using sptr = std::shared_ptr<TypedBuffer>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Vertex buffer
	//--------------------------------------------------------------------------------------
	class DLL_INTERFACE VertexBuffer :
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

		using uptr = std::unique_ptr<VertexBuffer>;
		using sptr = std::shared_ptr<VertexBuffer>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Index buffer
	//--------------------------------------------------------------------------------------
	class DLL_INTERFACE IndexBuffer :
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

		using uptr = std::unique_ptr<IndexBuffer>;
		using sptr = std::shared_ptr<IndexBuffer>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

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

	namespace Util
	{
		class DLL_INTERFACE DescriptorTable
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

			using uptr = std::unique_ptr<DescriptorTable>;
			using sptr = std::shared_ptr<DescriptorTable>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
		};
	}

	class DLL_INTERFACE DescriptorTableCache
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

		using uptr = std::unique_ptr<DescriptorTableCache>;
		using sptr = std::shared_ptr<DescriptorTableCache>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
		static uptr MakeUnique(const Device& device, const wchar_t* name = nullptr, API api = API::DIRECTX_12);
		static sptr MakeShared(const Device& device, const wchar_t* name = nullptr, API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Shader reflector
	//--------------------------------------------------------------------------------------
	class DLL_INTERFACE Reflector
	{
	public:
		//Reflector();
		virtual ~Reflector() {};

		virtual bool SetShader(const Blob& shader) = 0;
		virtual bool IsValid() const = 0;
		virtual uint32_t GetResourceBindingPointByName(const char* name, uint32_t defaultVal = UINT32_MAX) const = 0;

		using uptr = std::unique_ptr<Reflector>;
		using sptr = std::shared_ptr<Reflector>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

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

	class DLL_INTERFACE ShaderPool
	{
	public:
		//ShaderPool();
		virtual ~ShaderPool() {};

		virtual void SetShader(Shader::Stage stage, uint32_t index, const Blob& shader) = 0;
		virtual void SetShader(Shader::Stage stage, uint32_t index, const Blob& shader, const Reflector::sptr& reflector) = 0;
		virtual void SetReflector(Shader::Stage stage, uint32_t index, const Reflector::sptr& reflector) = 0;

		virtual Blob CreateShader(Shader::Stage stage, uint32_t index, const std::wstring& fileName) = 0;
		virtual Blob GetShader(Shader::Stage stage, uint32_t index) const = 0;
		virtual Reflector::sptr GetReflector(Shader::Stage stage, uint32_t index) const = 0;

		using uptr = std::unique_ptr<ShaderPool>;
		using sptr = std::shared_ptr<ShaderPool>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

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

	namespace Util
	{
		class DLL_INTERFACE PipelineLayout
		{
		public:
			//PipelineLayout();
			virtual ~PipelineLayout() {};

			virtual void SetShaderStage(uint32_t index, Shader::Stage stage) = 0;
			virtual void SetRange(uint32_t index, DescriptorType type, uint32_t num, uint32_t baseBinding,
				uint32_t space = 0, DescriptorFlag flags = DescriptorFlag::NONE) = 0;
			virtual void SetConstants(uint32_t index, uint32_t num32BitValues, uint32_t binding,
				uint32_t space = 0, Shader::Stage stage = Shader::Stage::ALL) = 0;
			virtual void SetRootSRV(uint32_t index, uint32_t binding, uint32_t space = 0,
				DescriptorFlag flags = DescriptorFlag::DATA_STATIC, Shader::Stage stage = Shader::Stage::ALL) = 0;
			virtual void SetRootUAV(uint32_t index, uint32_t binding, uint32_t space = 0,
				DescriptorFlag flags = DescriptorFlag::NONE, Shader::Stage stage = Shader::Stage::ALL) = 0;
			virtual void SetRootCBV(uint32_t index, uint32_t binding, uint32_t space = 0,
				DescriptorFlag flags = DescriptorFlag::NONE, Shader::Stage stage = Shader::Stage::ALL) = 0;

			virtual XUSG::PipelineLayout CreatePipelineLayout(PipelineLayoutCache& pipelineLayoutCache, PipelineLayoutFlag flags,
				const wchar_t* name = nullptr) = 0;
			virtual XUSG::PipelineLayout GetPipelineLayout(PipelineLayoutCache& pipelineLayoutCache, PipelineLayoutFlag flags,
				const wchar_t* name = nullptr) = 0;

			virtual DescriptorTableLayout CreateDescriptorTableLayout(uint32_t index, PipelineLayoutCache& pipelineLayoutCache) const = 0;
			virtual DescriptorTableLayout GetDescriptorTableLayout(uint32_t index, PipelineLayoutCache& pipelineLayoutCache) const = 0;

			virtual const std::vector<std::string>& GetDescriptorTableLayoutKeys() const = 0;
			virtual std::string& GetPipelineLayoutKey(PipelineLayoutCache* pPipelineLayoutCache) = 0;

			using uptr = std::unique_ptr<PipelineLayout>;
			using sptr = std::shared_ptr<PipelineLayout>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
			static uptr CloneUnique(const PipelineLayout* pSrc, API api = API::DIRECTX_12);
			static sptr CloneShared(const PipelineLayout* pSrc, API api = API::DIRECTX_12);
		};
	}

	class DLL_INTERFACE PipelineLayoutCache
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

		using uptr = std::unique_ptr<PipelineLayoutCache>;
		using sptr = std::shared_ptr<PipelineLayoutCache>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
		static uptr MakeUnique(const Device& device, API api = API::DIRECTX_12);
		static sptr MakeShared(const Device& device, API api = API::DIRECTX_12);
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
		
		class DLL_INTERFACE State
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

			using uptr = std::unique_ptr<State>;
			using sptr = std::shared_ptr<State>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
		};

		class DLL_INTERFACE PipelineCache
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

			using uptr = std::unique_ptr<PipelineCache>;
			using sptr = std::shared_ptr<PipelineCache>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
			static uptr MakeUnique(const Device& device, API api = API::DIRECTX_12);
			static sptr MakeShared(const Device& device, API api = API::DIRECTX_12);
		};
	}

	//--------------------------------------------------------------------------------------
	// Compute pipeline state
	//--------------------------------------------------------------------------------------
	namespace Compute
	{
		class PipelineCache;
		
		class DLL_INTERFACE State
		{
		public:
			//State();
			virtual ~State() {};

			virtual void SetPipelineLayout(const PipelineLayout& layout) = 0;
			virtual void SetShader(Blob shader) = 0;

			virtual Pipeline CreatePipeline(PipelineCache& pipelineCache, const wchar_t* name = nullptr) const = 0;
			virtual Pipeline GetPipeline(PipelineCache& pipelineCache, const wchar_t* name = nullptr) const = 0;

			virtual const std::string& GetKey() const = 0;

			using uptr = std::unique_ptr<State>;
			using sptr = std::shared_ptr<State>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
		};

		class DLL_INTERFACE PipelineCache
		{
		public:
			//PipelineCache();
			//PipelineCache(const Device& device);
			virtual ~PipelineCache() {};

			virtual void SetDevice(const Device& device) = 0;
			virtual void SetPipeline(const std::string& key, const Pipeline& pipeline) = 0;

			virtual Pipeline CreatePipeline(const State& state, const wchar_t* name = nullptr) = 0;
			virtual Pipeline GetPipeline(const State& state, const wchar_t* name = nullptr) = 0;

			using uptr = std::unique_ptr<PipelineCache>;
			using sptr = std::shared_ptr<PipelineCache>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
			static uptr MakeUnique(const Device& device, API api = API::DIRECTX_12);
			static sptr MakeShared(const Device& device, API api = API::DIRECTX_12);
		};
	}
}
