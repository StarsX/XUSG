//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Core/XUSG.h"

#define XUSG_MAKE_COARSE_SHADING_RATE(x,y)			((x) << 2 | (y))

#define XUSG_LINEAR_ALGEBRA_MATRIX_ALIGNMENT		128
#define XUSG_LINEAR_ALGEBRA_MATRIX_STRIDE_ALIGNMENT	16
#define XUSG_LINEAR_ALGEBRA_BIAS_ALIGNMENT			64

namespace XUSG
{
	struct ProgramIdentifier
	{
		uint64_t OpaqueData[4];
	};

	//--------------------------------------------------------------------------------------
	// Command list
	//--------------------------------------------------------------------------------------
	namespace Ultimate
	{
		enum class ResolveMode : uint8_t
		{
			DECOMPRESS,
			MIN,
			MAX,
			AVERAGE,
			ENCODE_SAMPLER_FEEDBACK,
			DECODE_SAMPLER_FEEDBACK
		};

		enum class BarrierSync : uint32_t
		{
			NONE = 0,
			ALL = (1 << 0),
			DRAW = (1 << 1),
			INDEX_INPUT = (1 << 2),
			VERTEX_SHADING = (1 << 3),
			PIXEL_SHADING = (1 << 4),
			DEPTH_STENCIL = (1 << 5),
			RENDER_TARGET = (1 << 6),
			COMPUTE_SHADING = (1 << 7),
			RAYTRACING = (1 << 8),
			COPY = (1 << 9),
			RESOLVE = (1 << 10),
			EXECUTE_INDIRECT = (1 << 11),
			ALL_SHADING = (1 << 12),
			NON_PIXEL_SHADING = (1 << 13),
			EMIT_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO = (1 << 14),
			CLEAR_UNORDERED_ACCESS_VIEW = (1 << 15),
			VIDEO_DECODE = (1 << 16),
			VIDEO_PROCESS = (1 << 17),
			VIDEO_ENCODE = (1 << 18),
			BUILD_RAYTRACING_ACCELERATION_STRUCTURE = (1 << 19),
			COPY_RAYTRACING_ACCELERATION_STRUCTURE = (1 << 20),
			SPLIT = (1 << 21),
			CONVERT_LINEAR_ALGEBRA_MATRIX = (1 << 22),

			PREDICATION = EXECUTE_INDIRECT,
		};

		XUSG_DEF_ENUM_FLAG_OPERATORS(BarrierSync);

		enum class BarrierAccess : uint32_t
		{
			COMMON = 0,
			VERTEX_BUFFER = (1 << 0),
			CONSTANT_BUFFER = (1 << 1),
			INDEX_BUFFER = (1 << 2),
			RENDER_TARGET = (1 << 3),
			UNORDERED_ACCESS = (1 << 4),
			DEPTH_STENCIL_WRITE = (1 << 5),
			DEPTH_STENCIL_READ = (1 << 6),
			SHADER_RESOURCE = (1 << 7),
			STREAM_OUTPUT = (1 << 8),
			INDIRECT_ARGUMENT = (1 << 9),
			COPY_DEST = (1 << 10),
			COPY_SOURCE = (1 << 11),
			RESOLVE_DEST = (1 << 12),
			RESOLVE_SOURCE = (1 << 13),
			RAYTRACING_ACCELERATION_STRUCTURE_READ = (1 << 14),
			RAYTRACING_ACCELERATION_STRUCTURE_WRITE = (1 << 15),
			SHADING_RATE_SOURCE = (1 << 16),
			VIDEO_DECODE_READ = (1 << 17),
			VIDEO_DECODE_WRITE = (1 << 18),
			VIDEO_PROCESS_READ = (1 << 19),
			VIDEO_PROCESS_WRITE = (1 << 20),
			VIDEO_ENCODE_READ = (1 << 21),
			VIDEO_ENCODE_WRITE = (1 << 22),
			NO_ACCESS = (1 << 23),

			PREDICATION = INDIRECT_ARGUMENT,
			RAYTRACING_ACCELERATION_STRUCTURE = UNORDERED_ACCESS | RAYTRACING_ACCELERATION_STRUCTURE_WRITE | RAYTRACING_ACCELERATION_STRUCTURE_READ
		};

		XUSG_DEF_ENUM_FLAG_OPERATORS(BarrierAccess);

		enum class TextureBarrierFlag
		{
			NONE = 0,
			DISCARD = (1 << 0)
		};

		XUSG_DEF_ENUM_FLAG_OPERATORS(TextureBarrierFlag);

		enum AxisShadingRate : uint8_t
		{
			AXIS_SHADING_RATE_1X = 0,
			AXIS_SHADING_RATE_2X = 0x1,
			AXIS_SHADING_RATE_4X = 0x2
		};

		XUSG_DEF_ENUM_FLAG_OPERATORS(AxisShadingRate);

		enum class ShadingRate : uint8_t
		{
			COARSE_1X1 = XUSG_MAKE_COARSE_SHADING_RATE(AXIS_SHADING_RATE_1X, AXIS_SHADING_RATE_1X),
			COARSE_1X2 = XUSG_MAKE_COARSE_SHADING_RATE(AXIS_SHADING_RATE_1X, AXIS_SHADING_RATE_2X),
			COARSE_2X1 = XUSG_MAKE_COARSE_SHADING_RATE(AXIS_SHADING_RATE_2X, AXIS_SHADING_RATE_1X),
			COARSE_2X2 = XUSG_MAKE_COARSE_SHADING_RATE(AXIS_SHADING_RATE_2X, AXIS_SHADING_RATE_2X),
			COARSE_2X4 = XUSG_MAKE_COARSE_SHADING_RATE(AXIS_SHADING_RATE_2X, AXIS_SHADING_RATE_4X),
			COARSE_4X2 = XUSG_MAKE_COARSE_SHADING_RATE(AXIS_SHADING_RATE_4X, AXIS_SHADING_RATE_2X),
			COARSE_4X4 = XUSG_MAKE_COARSE_SHADING_RATE(AXIS_SHADING_RATE_4X, AXIS_SHADING_RATE_4X)
		};

		enum class ShadingRateCombiner : uint8_t
		{
			COMBINER_PASSTHROUGH,
			COMBINER_OVERRIDE,
			COMBINER_MIN,
			COMBINER_MAX,
			COMBINER_SUM
		};

		enum class ProgramType : uint8_t
		{
			GENERIC_PIPELINE,
			RAYTRACING_PIPELINE,
			WORK_GRAPH,
			MLIR_PROGRAM
		};

		enum class ViewInstanceFlag : uint8_t
		{
			NONE = 0,
			ENABLE_VIEW_INSTANCE_MASKING = (1 << 0)
		};

		XUSG_DEF_ENUM_FLAG_OPERATORS(ViewInstanceFlag);

		enum class WorkGraphFlag : uint8_t
		{
			NONE = 0,
			INITIALIZE = (1 << 0)
		};

		XUSG_DEF_ENUM_FLAG_OPERATORS(WorkGraphFlag);

		enum class LinearAlgebraMatrixLayout : uint8_t
		{
			ROW_MAJOR,
			COLUMN_MAJOR,
			MUL_OPTIMAL,
			OUTER_PRODUCT_OPTIMAL
		};

		enum class LinearAlgebraDataType : uint8_t
		{
			SINT16,
			UINT16,
			SINT32,
			UINT32,
			FLOAT16,
			FLOAT32,
			UINT8,
			SINT8,
			FLOAT8_E4M3FN,
			FLOAT8_E5M2
		};

		struct ResourceBarrier
		{
			BarrierSync SyncBefore;
			BarrierSync SyncAfter;
			BarrierAccess AccessBefore;
			BarrierAccess AccessAfter;
			BarrierLayout LayoutBefore;
			BarrierLayout LayoutAfter;
			Resource* pResource;
			union
			{
				struct
				{
					uint32_t IndexOrFirstMipLevel;
					uint8_t NumMipLevels;
					uint16_t FirstArraySlice;
					uint16_t NumArraySlices;
					uint8_t FirstPlane;
					uint8_t NumPlanes;
					TextureBarrierFlag Flags;
				};
				struct
				{
					uint64_t Offset;
					uint64_t Size;
				};
			};
		};

		struct BarrierGroup
		{
			uint32_t NumBarriers;
			const ResourceBarrier* pBarriers;
		};

		struct ViewInstance
		{
			uint32_t ViewportArrayIndex;
			uint32_t RenderTargetArrayIndex;
		};

		struct SamplePosition
		{
			int8_t X;
			int8_t Y;
		};

		struct NodeCPUInput
		{
			uint32_t EntrypointIndex;
			uint32_t NumRecords;
			const void* pRecords;
			uint64_t RecordByteStride;
		};

		struct NodeGPUInput
		{
			uint32_t EntrypointIndex;
			uint32_t NumRecords;
			uint64_t RecordAddress;
			uint64_t RecordByteStride;
		};

		struct MultiNodeGPUInput
		{
			uint32_t NumNodeInputs;
			uint64_t NodeInputAddress;
			uint64_t NodeInputByteStride;
		};

		enum class ProgramBindingFlag : uint8_t
		{
			NONE = 0,
			VOLATILE_DESCRIPTOR = (1 << 0)
		};

		XUSG_DEF_ENUM_FLAG_OPERATORS(ProgramBindingFlag);

		enum class ProgramBindingType : uint8_t
		{
			GPU_VIRTUAL_ADDRESS,
			GPU_DESCRIPTOR,
			CPU
		};

		struct ProgramCPUBinding
		{
			const void* pData;
			uint64_t DataByteSize;
		};

		struct ProgramBinding
		{
			uint32_t BindPointIndex;
			ProgramBindingFlag Flags;
			ProgramBindingType Type;
			union
			{
				struct
				{
					uint64_t StartAddress;
					uint64_t ByteSize;
				} GpuVirtualAddress;
				uint64_t GpuDescriptor;
				ProgramCPUBinding Cpu;
			};
		};

		struct LinearAlgebraMatrixInfo 
		{
			uint32_t Size;
			LinearAlgebraMatrixLayout Layout;
			uint32_t Stride;
			uint16_t NumRows;
			uint16_t NumColumns;
			LinearAlgebraDataType DataType;
		};

		struct LinearAlgebraMatrixConversionInfo
		{
			LinearAlgebraMatrixInfo DstInfo;
			uint32_t SrcSize;
			LinearAlgebraDataType SrcDataType;
			LinearAlgebraMatrixLayout SrcLayout;
			uint32_t SrcStride;
			uint64_t Dst;
			uint64_t Src;
		};

		enum class AsyncCommandsImpl : uint8_t
		{
			NOT_SUPPORTED,
			FALLBACK,
			NATIVE
		};

		enum class TextureCopyDescType : uint8_t
		{
			TEXTURE_TO_TEXTURE,
			BUFFER_TO_TEXTURE,
			TEXTURE_TO_BUFFER
		};

		struct ClearData
		{
			union
			{
				float Floats[4];
				uint32_t Uints[4];
				int32_t Sints[4];
				uint8_t Bytes[16];
			};
		};

		struct SubresourceFootprint
		{
			Format Format;
			uint32_t Width;
			uint32_t Height;
			uint16_t Depth;
			uint32_t RowPitch;
		};

		struct PlacedSubresourceFootprint
		{
			uint64_t Offset;
			SubresourceFootprint Footprint;
		};

		struct TextureCopyRegion
		{
			uint32_t DestX;
			uint32_t DestY;
			uint32_t DestZ;
			const BoxRange* pSourceBox;
		};

		struct TextureTextureCopyDesc
		{
			Resource* pDest;
			uint32_t DestSubresourceIndex;
			Resource* pSource;
			uint32_t SourceSubresourceIndex;
		};

		struct BufferTextureCopyDesc
		{
			Resource* pBuffer;
			Resource* pTexture;
			uint32_t SubresourceIndex;
			PlacedSubresourceFootprint PlacedFootprint;
		};

		struct TextureCopyDesc
		{
			TextureCopyDescType Type;
			uint32_t NumRegions;
			const TextureCopyRegion* pCopyRegions;
			union
			{
				TextureTextureCopyDesc TextureTextureCopy;
				BufferTextureCopyDesc BufferTextureCopy;
			};
		};

		struct ClearTextureDesc
		{
			Resource* pTexture;
			uint32_t SubresourceIndex;
			ClearData ClearValue;
			Format Format;
			const BoxRange* pRegion;
		};

		struct FillBufferDesc
		{
			Resource* pBuffer;
			uint64_t Offset;
			ClearData FillValue;
			Format Format;
			uint32_t RawPatternSizeInBytes;
			uint32_t RepeatCount;
		};

		//--------------------------------------------------------------------------------------
		// Command list
		//--------------------------------------------------------------------------------------
		class XUSG_INTERFACE CommandList :
			public virtual XUSG::CommandList
		{
		public:
			//CommandList();
			virtual ~CommandList() {};

			using XUSG::CommandList::Barrier;
			virtual void Barrier(uint32_t numBarrierGroups, BarrierGroup* pBarrierGroups) = 0;
			virtual void Barrier(uint32_t numBufferBarriers, ResourceBarrier* pBufferBarriers,
				uint32_t numTextureBarriers, ResourceBarrier* pTextureBarriers,
				uint32_t numGlobalBarriers, ResourceBarrier* pGlobalBarriers) = 0;
			virtual void SignalBarrier(uint32_t numBarrierGroups, BarrierGroup* pBarrierGroups,
				Fence* pFence, uint64_t fenceValue) = 0;
			virtual void WaitBarrier(uint32_t numBarrierGroups, BarrierGroup* pBarrierGroups,
				uint32_t numFences, Fence* const* ppFences, const uint64_t* pFenceValues) = 0;
			virtual void SetSamplePositions(uint8_t numSamplesPerPixel, uint8_t numPixels, SamplePosition* pPositions) const = 0;
			virtual void ResolveSubresourceRegion(Resource* pDstResource, uint32_t dstSubresource,
				uint32_t dstX, uint32_t dstY, Resource* pSrcResource, uint32_t srcSubresource,
				const RectRange& srcRect, Format format, ResolveMode resolveMode) const = 0;

			virtual void RSSetShadingRate(ShadingRate baseShadingRate, const ShadingRateCombiner* pCombiners) const = 0;
			virtual void RSSetShadingRateImage(Resource* pShadingRateImage) const = 0;
			virtual void DispatchMesh(
				uint32_t ThreadGroupCountX,
				uint32_t ThreadGroupCountY,
				uint32_t ThreadGroupCountZ) const = 0;

			virtual void SetStateObject(const Pipeline& stateObject) const = 0;
			virtual void SetProgram(ProgramType type, ProgramIdentifier identifier, WorkGraphFlag flags = WorkGraphFlag::NONE,
				uint64_t backingMemoryAddress = 0, uint64_t backingMemoryByteSize = 0,
				uint64_t localRootArgTableAddress = 0, uint64_t localRootArgTableByteSize = 0,
				uint64_t localRootArgTableByteStride = 0) = 0;
			virtual void DispatchGraph(uint32_t numNodeInputs, const NodeCPUInput* pNodeInputs, uint64_t nodeInputByteStride) = 0;
			virtual void DispatchGraph(uint64_t nodeGPUInputAddress, bool isMultiNodes = false) = 0;
			virtual void DispatchGraph(uint32_t numBindings, const ProgramBinding* pBindings) = 0;

			virtual void ConvertLinearAlgebraMatrix(const LinearAlgebraMatrixConversionInfo* pConversions, uint32_t numConversions) = 0;

			virtual void CopyBufferRegions(uint32_t numRegions, Resource* const* ppDestBuffers, const uint64_t* pDestOffsets,
				Resource* const* ppSourceBuffers, const uint64_t* pSourceOffsets, const uint64_t* pSizes) = 0;
			virtual void CopyTextureRegions(uint32_t numCopyDescs, const TextureCopyDesc* pCopyDescs) = 0;
			virtual void CopyResources(uint32_t numSourceDestPairs, Resource* const* ppDestResources,
				Resource* const* ppSourceResources) = 0;
			virtual void CopyTilesAsync(Resource* pTiledResource, const TiledResourceCoord* pTileRegionStartCoord,
				const TileRegionSize& tileRegionSize, Resource* pBuffer, uint64_t bufferOffset,
				TileCopyFlag flags = TileCopyFlag::NONE) = 0;
			virtual void ResolveSubresourceRegionAsync(Resource* pDstResource, uint32_t dstSubresource,
				uint32_t dstX, uint32_t dstY, Resource* pSrcResource, uint32_t srcSubresource,
				const RectRange* pSrcRect, Format format, ResolveMode resolveMode) = 0;
			virtual void ResolveQueryDataAsync(const QueryHeap& queryHeap, QueryType type, uint32_t startIndex,
				uint32_t numQueries, Resource* pDestBuffer, uint64_t destOffset) = 0;
			virtual void ClearBoundDepthStencilView(ClearFlag clearFlags, float depth, uint8_t stencil = 0,
				uint32_t numRegions = 0, const BoxRange* pRegions = nullptr) = 0;
			virtual void ClearBoundRenderTargetViews(uint32_t renderTargetMask, const ClearData* pClearValues,
				const uint32_t* pNumRegions = nullptr, const BoxRange* const* ppRegions = nullptr) = 0;
			virtual void ClearTextureSubresources(uint32_t numClearDescs, const ClearTextureDesc* pClearDescs) = 0;
			virtual void FillBuffers(uint32_t numFillDescs, const FillBufferDesc* pFillDescs) = 0;

			using uptr = std::unique_ptr<CommandList>;
			using sptr = std::shared_ptr<CommandList>;

			static uptr MakeUnique();
			static sptr MakeShared();
			static uptr MakeUnique(XUSG::CommandList& commandList);
			static sptr MakeShared(XUSG::CommandList& commandList);
		};

		//--------------------------------------------------------------------------------------
		// Pipeline layout
		//--------------------------------------------------------------------------------------
		class XUSG_INTERFACE PipelineLayoutLib :
			public virtual XUSG::PipelineLayoutLib
		{
		public:
			//PipelineLayoutLib();
			//PipelineLayoutLib(Device* pDevice) = 0;
			virtual ~PipelineLayoutLib() {};

			virtual PipelineLayout CreateRootSignatureFromLibSubobject(const Blob& blobLib,
				const wchar_t* name, uint32_t nodeMask = 0) = 0;
			virtual PipelineLayout GetRootSignatureFromLibSubobject(const Blob& blobLib,
				const wchar_t* name, bool create = true, uint32_t nodeMask = 0) = 0;

			using uptr = std::unique_ptr<PipelineLayoutLib>;
			using sptr = std::shared_ptr<PipelineLayoutLib>;

			static uptr MakeUnique();
			static sptr MakeShared();
			static uptr MakeUnique(Device* pDevice);
			static sptr MakeShared(Device* pDevice);
		};

		//--------------------------------------------------------------------------------------
		// Pipeline state
		//--------------------------------------------------------------------------------------
		class PipelineLib;

		class XUSG_INTERFACE State
		{
		public:
			//State();
			virtual ~State() {};

			virtual void SetPipelineLayout(const PipelineLayout& layout) = 0;
			virtual void SetShader(Shader::Stage stage, const Blob& shader) = 0;
			virtual void SetCachedPipeline(const Blob& cachedPipeline) = 0;
			virtual void SetNodeMask(uint32_t nodeMask) = 0;
			virtual void SetFlags(PipelineFlag flag) = 0;

			virtual void OMSetBlendState(const Graphics::Blend* pBlend, uint32_t sampleMask = UINT_MAX) = 0;
			virtual void RSSetState(const Graphics::Rasterizer* pRasterizer) = 0;
			virtual void DSSetState(const Graphics::DepthStencil* pDepthStencil) = 0;

			virtual void OMSetBlendState(Graphics::BlendPreset preset, PipelineLib* pPipelineLib,
				uint8_t numColorRTs = 1, uint32_t sampleMask = UINT_MAX) = 0;
			virtual void RSSetState(Graphics::RasterizerPreset preset, PipelineLib* pPipelineLib) = 0;
			virtual void DSSetState(Graphics::DepthStencilPreset preset, PipelineLib* pPipelineLib) = 0;

			virtual void IASetInputLayout(const InputLayout* pLayout) = 0;
			virtual void IASetPrimitiveTopologyType(PrimitiveTopologyType type) = 0;
			virtual void IASetIndexBufferStripCutValue(IBStripCutValue ibStripCutValue) = 0;

			virtual void OMSetNumRenderTargets(uint8_t n) = 0;
			virtual void OMSetRTVFormat(uint8_t i, Format format) = 0;
			virtual void OMSetRTVFormats(uint8_t n, const Format* formats) = 0;
			virtual void OMSetDSVFormat(Format format) = 0;
			virtual void OMSetSample(uint8_t count, uint8_t quality = 0) = 0;

			virtual void SetNumViewInstances(uint8_t n, ViewInstanceFlag flags) = 0;
			virtual void SetViewInstance(uint8_t i, const ViewInstance& viewInstance) = 0;
			virtual void SetViewInstances(uint8_t n, const ViewInstance* viewInstances, ViewInstanceFlag flags) = 0;

			virtual Pipeline CreatePipeline(PipelineLib* pPipelineLib, const wchar_t* name = nullptr) = 0;
			virtual Pipeline GetPipeline(PipelineLib* pPipelineLib, const wchar_t* name = nullptr) = 0;

			virtual PipelineLayout GetPipelineLayout() const = 0;
			virtual Blob GetShader(Shader::Stage stage) const = 0;
			virtual Blob GetCachedPipeline() const = 0;
			virtual uint32_t GetNodeMask() const = 0;
			virtual PipelineFlag GetFlags() const = 0;

			virtual uint32_t OMGetSampleMask() const = 0;
			virtual const Graphics::Blend* OMGetBlendState() const = 0;
			virtual const Graphics::Rasterizer* RSGetState() const = 0;
			virtual const Graphics::DepthStencil* DSGetState() const = 0;

			virtual const InputLayout* IAGetInputLayout() const = 0;
			virtual PrimitiveTopologyType IAGetPrimitiveTopologyType() const = 0;
			virtual IBStripCutValue IAGetIndexBufferStripCutValue() const = 0;

			virtual uint8_t OMGetNumRenderTargets() const = 0;
			virtual Format OMGetRTVFormat(uint8_t i) const = 0;
			virtual Format OMGetDSVFormat() const = 0;
			virtual uint8_t OMGetSampleCount() const = 0;
			virtual uint8_t OMGetSampleQuality() const = 0;

			virtual uint8_t GetNumViewInstances(uint8_t n, ViewInstanceFlag flags) const = 0;
			virtual ViewInstanceFlag GetViewInstanceFlags() const = 0;
			virtual const ViewInstance& GetViewInstance(uint8_t i) const = 0;

			// Backend C++ object (Ultimate::State_DX12* / State_VK*, not available to users), internal use only.
			virtual void* GetBackend() = 0;

			using uptr = std::unique_ptr<State>;
			using sptr = std::shared_ptr<State>;

			static uptr MakeUnique();
			static sptr MakeShared();
		};

		class XUSG_INTERFACE PipelineLib
		{
		public:
			//PipelineLib();
			//PipelineLib(Device* pDevice);
			virtual ~PipelineLib() {};

			virtual void SetDevice(Device* pDevice) = 0;
			virtual void SetPipeline(State* pState, const Pipeline& pipeline) = 0;

			virtual void SetInputLayout(uint32_t index, uint32_t numElements, const InputElement* pElements) = 0;
			virtual const InputLayout* GetInputLayout(uint32_t index) = 0;
			virtual const InputLayout* CreateInputLayout(uint32_t numElements, const InputElement* pElements) = 0;

			virtual Pipeline CreatePipeline(State* pState, const wchar_t* name = nullptr) = 0;
			virtual Pipeline GetPipeline(State* pState, const wchar_t* name = nullptr) = 0;

			virtual const Graphics::Blend* GetBlend(Graphics::BlendPreset preset, uint8_t numColorRTs = 1) = 0;
			virtual const Graphics::Rasterizer* GetRasterizer(Graphics::RasterizerPreset preset) = 0;
			virtual const Graphics::DepthStencil* GetDepthStencil(Graphics::DepthStencilPreset preset) = 0;

			using uptr = std::unique_ptr<PipelineLib>;
			using sptr = std::shared_ptr<PipelineLib>;

			static uptr MakeUnique();
			static sptr MakeShared();
			static uptr MakeUnique(Device* pDevice);
			static sptr MakeShared(Device* pDevice);
		};

		//--------------------------------------------------------------------------------------
		// Sampler feedback
		//--------------------------------------------------------------------------------------
		class XUSG_INTERFACE SamplerFeedBack :
			public virtual Texture
		{
		public:
			//SamplerFeedBack();
			virtual ~SamplerFeedBack() {};

			using Resource::Create;

			// Create a texture with auto SRVs and UAVs
			virtual bool Create(Device* pDevice, Texture* pTarget, Format format, uint32_t mipRegionWidth,
				uint32_t mipRegionHeight, uint32_t mipRegionDepth, ResourceFlag resourceFlags = ResourceFlag::NONE,
				bool isCubeMap = false, MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr,
				uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
				const TextureLayoutDesc& textureLayout = {},
				uint32_t maxThreads = 1) = 0;
			virtual bool Create(Device* pDevice, Heap* pHeap, uint64_t heapOffset, Texture* pTarget,
				Format format, uint32_t mipRegionWidth, uint32_t mipRegionHeight, uint32_t mipRegionDepth,
				ResourceFlag resourceFlags = ResourceFlag::NONE, bool isCubeMap = false,
				const wchar_t* name = nullptr, uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
				const TextureLayoutDesc& textureLayout = {},
				uint32_t maxThreads = 1) = 0;
			virtual bool CreateResource(Device* pDevice, Texture* pTarget, Format format, uint32_t mipRegionWidth,
				uint32_t mipRegionHeight, uint32_t mipRegionDepth, ResourceFlag resourceFlags = ResourceFlag::NONE,
				bool isCubeMap = false, MemoryFlag memoryFlags = MemoryFlag::NONE,
				ResourceState initialResourceState = ResourceState::COMMON,
				const TextureLayoutDesc& textureLayout = {},
				uint32_t maxThreads = 1) = 0;
			virtual bool CreateResource(Device* pDevice, Heap* pHeap, uint64_t heapOffset, Texture* pTarget,
				Format format, uint32_t mipRegionWidth, uint32_t mipRegionHeight, uint32_t mipRegionDepth,
				ResourceFlag resourceFlags = ResourceFlag::NONE, bool isCubeMap = false,
				ResourceState initialResourceState = ResourceState::COMMON,
				const TextureLayoutDesc& textureLayout = {},
				uint32_t maxThreads = 1) = 0;

			virtual Descriptor CreateUAV(Resource* pTarget) = 0;

			using uptr = std::unique_ptr<SamplerFeedBack>;
			using sptr = std::shared_ptr<SamplerFeedBack>;

			static uptr MakeUnique();
			static sptr MakeShared();
		};

		XUSG_INTERFACE uint32_t SetBarrier(ResourceBarrier* pBufferBarriers, Buffer* pBuffer, ResourceState dstState,
			uint32_t numBarriers = 0, BarrierFlag flags = BarrierFlag::NONE, ResourceState srcState = ResourceState::AUTO,
			uint32_t threadIdx = 0);
		XUSG_INTERFACE uint32_t SetBarrier(ResourceBarrier* pTextureBarriers, Texture* pTexture, ResourceState dstState,
			uint32_t numBarriers = 0, uint32_t indexOrFirstMip = XUSG_BARRIER_ALL_SUBRESOURCES, uint8_t numMipLevels = 0,
			uint16_t firstArraySlice = 0, uint16_t numArraySlices = 0, uint8_t firstPlane = 0, uint8_t numPlanes = 0,
			BarrierFlag flags = BarrierFlag::NONE, ResourceState srcState = ResourceState::AUTO,
			TextureBarrierFlag textureFlags = TextureBarrierFlag::NONE, uint32_t threadIdx = 0);

		XUSG_INTERFACE void GetLinearAlgebraMatrixInfo(const Device* pDevice, LinearAlgebraMatrixInfo& matrixInfo);
		XUSG_INTERFACE void SetLinearAlgebraMatrixConversionInfo(LinearAlgebraMatrixConversionInfo& conversion,
			uint64_t dst, uint64_t src, LinearAlgebraDataType srcDataType, LinearAlgebraMatrixLayout srcLayout,
			uint32_t srcSize = 0, uint32_t srcStride = 0);

		// Call before Device::Create. Prefers native async commands: probes the adapter first and
		// enables the Agility runtime fallback layer only when Impl is not NATIVE.
		XUSG_INTERFACE void EnableAsyncCommandsFallback(bool enable = true);
		XUSG_INTERFACE bool GetAsyncCommandsImpl(Device* pDevice, AsyncCommandsImpl* pImpl);

		XUSG_INTERFACE void MapBarrierState(BarrierSync& barrierSync, BarrierAccess& barrierAccess, ResourceState resourceState);
		XUSG_INTERFACE void MapBarrierState(BarrierSync& barrierSync, BarrierAccess& barrierAccess,
			BarrierLayout &barrierLayout, ResourceState resourceState);

		XUSG_INTERFACE BarrierSync GetBarrierSync(ResourceState resourceState);

		XUSG_INTERFACE BarrierAccess GetBarrierAccess(ResourceState resourceState);
	}

	//--------------------------------------------------------------------------------------
	// Work-graph pipeline state
	//--------------------------------------------------------------------------------------
	namespace WorkGraph
	{
		enum class BoolOverride : uint8_t
		{
			IS_NULL,
			IS_FALSE,
			IS_TRUE
		};

		struct NodeID
		{
			const wchar_t* Name;
			uint32_t ArrayIndex;
		};

		struct MemoryRequirements
		{
			uint64_t MinByteSize;
			uint64_t MaxByteSize;
			uint32_t SizeGranularityInBytes;
		};

		//--------------------------------------------------------------------------------------
		// Pipeline state
		//--------------------------------------------------------------------------------------
		class PipelineLib;

		class XUSG_INTERFACE State
		{
		public:
			//State();
			virtual ~State() {}

			virtual void SetShaderLibrary(uint32_t index, const Blob& shaderLib,
				uint32_t numShaders = 0, const wchar_t** pShaderNames = nullptr) = 0;
			virtual void SetProgramName(const wchar_t* programName) = 0;
			virtual void SetLocalPipelineLayout(uint32_t index, const PipelineLayout& layout,
				uint32_t numShaders, const wchar_t** pShaderNames) = 0;
			virtual void SetGlobalPipelineLayout(const PipelineLayout& layout) = 0;
			virtual void SetNodeMask(uint32_t nodeMask) = 0;
			virtual void OverrideDispatchGrid(const wchar_t* shaderName, uint32_t x, uint32_t y, uint32_t z,
				BoolOverride isEntry = BoolOverride::IS_NULL) = 0;
			virtual void OverrideMaxDispatchGrid(const wchar_t* shaderName, uint32_t x, uint32_t y, uint32_t z,
				BoolOverride isEntry = BoolOverride::IS_NULL) = 0;

			virtual Pipeline CreatePipeline(PipelineLib* pPipelineLib, const wchar_t* name = nullptr) = 0;
			virtual Pipeline GetPipeline(PipelineLib* pPipelineLib, const wchar_t* name = nullptr) = 0;

			virtual const wchar_t* GetProgramName(uint32_t workGraphIndex) const = 0;
			virtual ProgramIdentifier GetProgramIdentifier(const wchar_t* programName) const = 0;

			virtual uint32_t GetNumWorkGraphs() const = 0;
			virtual uint32_t GetWorkGraphIndex(const wchar_t* pProgramName) const = 0;
			virtual uint32_t GetNumNodes(uint32_t workGraphIndex) const = 0;
			virtual uint32_t GetNodeIndex(uint32_t workGraphIndex, const NodeID& nodeID) const = 0;
			virtual uint32_t GetNodeLocalRootArgumentsTableIndex(uint32_t workGraphIndex, uint32_t nodeIndex) const = 0;
			virtual uint32_t GetNumEntrypoints(uint32_t workGraphIndex) const = 0;
			virtual uint32_t GetEntrypointIndex(uint32_t workGraphIndex, const NodeID& nodeID) const = 0;
			virtual uint32_t GetEntrypointRecordSizeInBytes(uint32_t workGraphIndex, uint32_t entrypointIndex) const = 0;

			virtual NodeID GetNodeID(uint32_t workGraphIndex, uint32_t nodeIndex) const = 0;
			virtual NodeID GetEntrypointID(uint32_t workGraphIndex, uint32_t entrypointIndex) const = 0;

			virtual void GetMemoryRequirements(uint32_t workGraphIndex, MemoryRequirements* pMemoryReq) const = 0;

			// Backend C++ object (WorkGraph::State_DX12*, not available to users), internal use only.
			virtual void* GetBackend() = 0;

			using uptr = std::unique_ptr<State>;
			using sptr = std::shared_ptr<State>;

			static uptr MakeUnique();
			static sptr MakeShared();
		};

		class XUSG_INTERFACE PipelineLib
		{
		public:
			//PipelineLib();
			//PipelineLib(Device* pDevice);
			virtual ~PipelineLib() {}

			virtual void SetDevice(Device* pDevice) = 0;
			virtual void SetPipeline(State* pState, const Pipeline& pipeline) = 0;

			virtual Pipeline CreatePipeline(State* pState, const wchar_t* name = nullptr) = 0;
			virtual Pipeline GetPipeline(State* pState, const wchar_t* name = nullptr) = 0;

			virtual const wchar_t* GetProgramName(const Pipeline& stateObject, uint32_t workGraphIndex) const = 0;
			virtual ProgramIdentifier GetProgramIdentifier(const Pipeline& stateObject, const wchar_t* programName) const = 0;

			virtual uint32_t GetNumWorkGraphs(const Pipeline& stateObject) const = 0;
			virtual uint32_t GetWorkGraphIndex(const Pipeline& stateObject, const wchar_t* pProgramName) const = 0;
			virtual uint32_t GetNumNodes(const Pipeline& stateObject, uint32_t workGraphIndex) const = 0;
			virtual uint32_t GetNodeIndex(const Pipeline& stateObject, uint32_t workGraphIndex, const NodeID& nodeID) const = 0;
			virtual uint32_t GetNodeLocalRootArgumentsTableIndex(const Pipeline& stateObject, uint32_t workGraphIndex, uint32_t nodeIndex) const = 0;
			virtual uint32_t GetNumEntrypoints(const Pipeline& stateObject, uint32_t workGraphIndex) const = 0;
			virtual uint32_t GetEntrypointIndex(const Pipeline& stateObject, uint32_t workGraphIndex, const NodeID& nodeID) const = 0;
			virtual uint32_t GetEntrypointRecordSizeInBytes(const Pipeline& stateObject, uint32_t workGraphIndex, uint32_t entrypointIndex) const = 0;

			virtual NodeID GetNodeID(const Pipeline& stateObject, uint32_t workGraphIndex, uint32_t nodeIndex) const = 0;
			virtual NodeID GetEntrypointID(const Pipeline& stateObject, uint32_t workGraphIndex, uint32_t entrypointIndex) const = 0;

			virtual void GetMemoryRequirements(const Pipeline& stateObject, uint32_t workGraphIndex, MemoryRequirements* pMemoryReq) const = 0;

			using uptr = std::unique_ptr<PipelineLib>;
			using sptr = std::shared_ptr<PipelineLib>;

			static uptr MakeUnique();
			static sptr MakeShared();
			static uptr MakeUnique(Device* pDevice);
			static sptr MakeShared(Device* pDevice);
		};
	}

	//--------------------------------------------------------------------------------------
	// Generic program pipeline state
	//--------------------------------------------------------------------------------------
	namespace Generic
	{
		using BlendPreset = Graphics::BlendPreset;
		using RasterizerPreset = Graphics::RasterizerPreset;
		using DepthStencilPreset = Graphics::DepthStencilPreset;

		using Blend = Graphics::Blend;
		using Rasterizer = Graphics::Rasterizer;
		using DepthStencil = Graphics::DepthStencil;

		using ViewInstance = Ultimate::ViewInstance;
		using ViewInstanceFlag = Ultimate::ViewInstanceFlag;

		enum class PartialProgramType : uint8_t
		{
			NONE,
			PRERASTERIZATION,
			PIXEL,
			COMBINED
		};

		enum class PartialProgramFlag : uint16_t
		{
			NONE = 0,
			LATE_LINK_INPUT_LAYOUT = (1 << 0),
			LATE_LINK_BLEND = (1 << 1),
			LATE_LINK_RASTERIZER = (1 << 2),
			LATE_LINK_SAMPLE_MASK = (1 << 3),
			LATE_LINK_SAMPLE_DESC = (1 << 4),
			LATE_LINK_DEPTH_STENCIL_FORMAT = (1 << 5),
			LATE_LINK_RENDER_TARGET_FORMAT = (1 << 6),
			LATE_LINK_DEPTH_STENCIL = (1 << 7),
			EXCLUDE_PS = (1 << 8)
		};

		XUSG_DEF_ENUM_FLAG_OPERATORS(PartialProgramFlag);

		class PipelineLib;

		class XUSG_INTERFACE State
		{
		public:
			//State();
			virtual ~State() {};

			virtual void SetPipelineLayout(const PipelineLayout& layout) = 0;
			virtual void SetShaderLibrary(uint8_t index, const Blob& shaderLib) = 0;
			virtual void SetShader(Shader::Stage stage, uint8_t libIndex, const wchar_t* shaderName) = 0;
			virtual void SetProgram(const wchar_t* programName) = 0;
			virtual void SetNodeMask(uint32_t nodeMask) = 0;
			virtual void SetFlags(PipelineFlag flag) = 0;
			virtual void SetPartialProgramType(PartialProgramType type) = 0;
			virtual void SetPartialProgramFlags(PartialProgramFlag flags) = 0;
			virtual void SetExistingCollection(Pipeline collection) = 0;
			virtual void AddPartialProgram(const wchar_t* programName) = 0;
			virtual void AddPartialLibrary(Pipeline library) = 0;

			virtual void OMSetBlendState(const Blend* pBlend, uint32_t sampleMask = UINT_MAX) = 0;
			virtual void RSSetState(const Rasterizer* pRasterizer) = 0;
			virtual void DSSetState(const DepthStencil* pDepthStencil) = 0;

			virtual void OMSetBlendState(BlendPreset preset, PipelineLib* pPipelineLib,
				uint8_t numColorRTs = 1, uint32_t sampleMask = UINT_MAX) = 0;
			virtual void RSSetState(RasterizerPreset preset, PipelineLib* pPipelineLib) = 0;
			virtual void DSSetState(DepthStencilPreset preset, PipelineLib* pPipelineLib) = 0;

			virtual void IASetInputLayout(const InputLayout* pLayout) = 0;
			virtual void IASetPrimitiveTopologyType(PrimitiveTopologyType type) = 0;
			virtual void IASetIndexBufferStripCutValue(IBStripCutValue ibStripCutValue) = 0;

			virtual void OMSetNumRenderTargets(uint8_t n) = 0;
			virtual void OMSetRTVFormat(uint8_t i, Format format) = 0;
			virtual void OMSetRTVFormats(uint8_t n, const Format* formats) = 0;
			virtual void OMSetDSVFormat(Format format) = 0;
			virtual void OMSetSample(uint8_t count, uint8_t quality = 0) = 0;

			virtual void SetNumViewInstances(uint8_t n, ViewInstanceFlag flags) = 0;
			virtual void SetViewInstance(uint8_t i, const ViewInstance& viewInstance) = 0;
			virtual void SetViewInstances(uint8_t n, const ViewInstance* viewInstances, ViewInstanceFlag flags) = 0;

			virtual Pipeline CreatePipeline(PipelineLib* pPipelineLib, const wchar_t* name = nullptr) = 0;
			virtual Pipeline GetPipeline(PipelineLib* pPipelineLib, const wchar_t* name = nullptr) = 0;

			virtual PipelineLayout GetPipelineLayout() const = 0;
			virtual Blob GetShaderLibrary(uint8_t index) const = 0;
			virtual uint8_t GetShaderLibraryIndex(Shader::Stage stage) const = 0;
			virtual const wchar_t* GetShaderName(Shader::Stage stage) const = 0;
			virtual const wchar_t* GetProgramName() const = 0;
			virtual ProgramIdentifier GetProgramIdentifier(const wchar_t* programName = nullptr) const = 0;
			virtual uint32_t GetNodeMask() const = 0;
			virtual PipelineFlag GetFlags() const = 0;
			virtual PartialProgramType GetPartialProgramType() const = 0;
			virtual PartialProgramFlag GetPartialProgramFlags() const = 0;

			virtual uint32_t OMGetSampleMask() const = 0;
			virtual const Graphics::Blend* OMGetBlendState() const = 0;
			virtual const Graphics::Rasterizer* RSGetState() const = 0;
			virtual const Graphics::DepthStencil* DSGetState() const = 0;

			virtual const InputLayout* IAGetInputLayout() const = 0;
			virtual PrimitiveTopologyType IAGetPrimitiveTopologyType() const = 0;
			virtual IBStripCutValue IAGetIndexBufferStripCutValue() const = 0;

			virtual uint8_t OMGetNumRenderTargets() const = 0;
			virtual Format OMGetRTVFormat(uint8_t i) const = 0;
			virtual Format OMGetDSVFormat() const = 0;
			virtual uint8_t OMGetSampleCount() const = 0;
			virtual uint8_t OMGetSampleQuality() const = 0;

			virtual uint8_t GetNumViewInstances(uint8_t n, ViewInstanceFlag flags) const = 0;
			virtual ViewInstanceFlag GetViewInstanceFlags() const = 0;
			virtual const ViewInstance& GetViewInstance(uint8_t i) const = 0;

			// Backend C++ object (Generic::State_DX12* / State_VK*, not available to users), internal use only.
			virtual void* GetBackend() = 0;

			using uptr = std::unique_ptr<State>;
			using sptr = std::shared_ptr<State>;

			static uptr MakeUnique();
			static sptr MakeShared();
		};

		class XUSG_INTERFACE PipelineLib
		{
		public:
			//PipelineLib();
			//PipelineLib(Device* pDevice);
			virtual ~PipelineLib() {};

			virtual void SetDevice(Device* pDevice) = 0;
			virtual void SetPipeline(State* pState, const Pipeline& pipeline) = 0;

			virtual void SetInputLayout(uint32_t index, uint32_t numElements, const InputElement* pElements) = 0;
			virtual const InputLayout* GetInputLayout(uint32_t index) = 0;
			virtual const InputLayout* CreateInputLayout(uint32_t numElements, const InputElement* pElements) = 0;

			virtual Pipeline CreatePipeline(State* pState, const wchar_t* name = nullptr) = 0;
			virtual Pipeline GetPipeline(State* pState, const wchar_t* name = nullptr) = 0;

			virtual const Blend* GetBlend(BlendPreset preset, uint8_t numColorRTs = 1) = 0;
			virtual const Rasterizer* GetRasterizer(RasterizerPreset preset) = 0;
			virtual const DepthStencil* GetDepthStencil(DepthStencilPreset preset) = 0;

			virtual ProgramIdentifier GetProgramIdentifier(const Pipeline& stateObject, const wchar_t* programName) const = 0;

			using uptr = std::unique_ptr<PipelineLib>;
			using sptr = std::shared_ptr<PipelineLib>;

			static uptr MakeUnique();
			static sptr MakeShared();
			static uptr MakeUnique(Device* pDevice);
			static sptr MakeShared(Device* pDevice);
		};
	}

	//--------------------------------------------------------------------------------------
	// Compute-graph (MLIR program) pipeline state
	//--------------------------------------------------------------------------------------
	namespace ComputeGraph
	{
		enum class ExchangeType : uint8_t
		{
			SUBGRAPH_DECLARATION,
			SUBGRAPH_SPECIALIZATION,
			TARGET_CONFIG
		};

		struct Support
		{
			bool MultisubgraphPartitionsSupported;
			bool CpuBindingSupported;
		};

		XUSG_INTERFACE bool GetVersion(Device* pDevice, uint64_t maxVersion, uint64_t* pHighestVersion);
		XUSG_INTERFACE bool GetSupport(Device* pDevice, Support* pSupport);
		XUSG_INTERFACE bool Exchange(Device* pDevice, ExchangeType type, uint64_t irVersion,
			const void* pInputData, size_t inputDataByteSize,
			void* pOutputData, size_t* pOutputDataByteSize);

		//--------------------------------------------------------------------------------------
		// Pipeline state
		//--------------------------------------------------------------------------------------
		class PipelineLib;

		class XUSG_INTERFACE State
		{
		public:
			//State();
			virtual ~State() {}

			virtual void SetProgramName(const wchar_t* programName) = 0;
			virtual void SetBytecode(const void* pBytecode, size_t bytecodeByteSize) = 0;
			virtual void SetNodeMask(uint32_t nodeMask) = 0;

			virtual Pipeline CreatePipeline(PipelineLib* pPipelineLib, const wchar_t* name = nullptr) = 0;
			virtual Pipeline GetPipeline(PipelineLib* pPipelineLib, const wchar_t* name = nullptr) = 0;

			virtual const wchar_t* GetProgramName() const = 0;
			virtual ProgramIdentifier GetProgramIdentifier(const wchar_t* programName = nullptr) const = 0;

			// Backend C++ object (ComputeGraph::State_DX12*, not available to users), internal use only.
			virtual void* GetBackend() = 0;

			using uptr = std::unique_ptr<State>;
			using sptr = std::shared_ptr<State>;

			static uptr MakeUnique();
			static sptr MakeShared();
		};

		class XUSG_INTERFACE PipelineLib
		{
		public:
			//PipelineLib();
			//PipelineLib(Device* pDevice);
			virtual ~PipelineLib() {}

			virtual void SetDevice(Device* pDevice) = 0;
			virtual void SetPipeline(State* pState, const Pipeline& pipeline) = 0;

			virtual Pipeline CreatePipeline(State* pState, const wchar_t* name = nullptr) = 0;
			virtual Pipeline GetPipeline(State* pState, const wchar_t* name = nullptr) = 0;

			virtual ProgramIdentifier GetProgramIdentifier(const Pipeline& stateObject, const wchar_t* programName) const = 0;

			using uptr = std::unique_ptr<PipelineLib>;
			using sptr = std::shared_ptr<PipelineLib>;

			static uptr MakeUnique();
			static sptr MakeShared();
			static uptr MakeUnique(Device* pDevice);
			static sptr MakeShared(Device* pDevice);
		};
	}
}
