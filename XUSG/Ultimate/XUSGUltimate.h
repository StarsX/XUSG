//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Core/XUSG.h"

#define XUSG_MAKE_COARSE_SHADING_RATE(x,y) ((x) << 2 | (y))

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
			WORK_GRAPH
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

		struct ResourceBarrier
		{
			BarrierSync SyncBefore;
			BarrierSync SyncAfter;
			BarrierAccess AccessBefore;
			BarrierAccess AccessAfter;
			BarrierLayout LayoutBefore;
			BarrierLayout LayoutAfter;
			const Resource* pResource;
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

		//--------------------------------------------------------------------------------------
		// Command list
		//--------------------------------------------------------------------------------------
		class XUSG_INTERFACE CommandList :
			public virtual XUSG::CommandList
		{
		public:
			//CommandList();
			virtual ~CommandList() {};

			virtual bool CreateInterface() = 0;

			using XUSG::CommandList::Barrier;
			virtual void Barrier(uint32_t numBarrierGroups, BarrierGroup* pBarrierGroups) = 0;
			virtual void Barrier(uint32_t numBufferBarriers, ResourceBarrier* pBufferBarriers,
				uint32_t numTextureBarriers, ResourceBarrier* pTextureBarriers,
				uint32_t numGlobalBarriers, ResourceBarrier* pGlobalBarriers) = 0;
			virtual void SetSamplePositions(uint8_t numSamplesPerPixel, uint8_t numPixels, SamplePosition* pPositions) const = 0;
			virtual void ResolveSubresourceRegion(const Resource* pDstResource, uint32_t dstSubresource,
				uint32_t dstX, uint32_t dstY, const Resource* pSrcResource, uint32_t srcSubresource,
				const RectRange& srcRect, Format format, ResolveMode resolveMode) const = 0;

			virtual void RSSetShadingRate(ShadingRate baseShadingRate, const ShadingRateCombiner* pCombiners) const = 0;
			virtual void RSSetShadingRateImage(const Resource* pShadingRateImage) const = 0;
			virtual void DispatchMesh(
				uint32_t ThreadGroupCountX,
				uint32_t ThreadGroupCountY,
				uint32_t ThreadGroupCountZ) const = 0;

			virtual void SetStateObject(const Pipeline& stateObject) = 0;
			virtual void SetProgram(ProgramType type, ProgramIdentifier identifier, WorkGraphFlag flags = WorkGraphFlag::NONE,
				uint64_t backingMemoryAddress = 0, uint64_t backingMemoryByteSize = 0,
				uint64_t localRootArgTableAddress = 0, uint64_t localRootArgTableByteSize = 0,
				uint64_t localRootArgTableByteStride = 0) = 0;
			virtual void DispatchGraph(uint32_t numNodeInputs, const NodeCPUInput* pNodeInputs, uint64_t nodeInputByteStride = 0) = 0;
			virtual void DispatchGraph(uint64_t nodeGPUInputAddress, bool isMultiNodes = false) = 0;

			using uptr = std::unique_ptr<CommandList>;
			using sptr = std::shared_ptr<CommandList>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
			static uptr MakeUnique(XUSG::CommandList& commandList, API api = API::DIRECTX_12);
			static sptr MakeShared(XUSG::CommandList& commandList, API api = API::DIRECTX_12);
		};

		//--------------------------------------------------------------------------------------
		// Pipeline layout
		//--------------------------------------------------------------------------------------
		class XUSG_INTERFACE PipelineLayoutLib :
			public virtual XUSG::PipelineLayoutLib
		{
		public:
			//PipelineLayoutLib();
			//PipelineLayoutLib(const Device* pDevice) = 0;
			virtual ~PipelineLayoutLib() {};

			virtual PipelineLayout CreateRootSignatureFromLibSubobject(const Blob& blobLib,
				const wchar_t* name, uint32_t nodeMask = 0) = 0;
			virtual PipelineLayout GetRootSignatureFromLibSubobject(const Blob& blobLib,
				const wchar_t* name, bool create = true, uint32_t nodeMask = 0) = 0;

			using uptr = std::unique_ptr<PipelineLayoutLib>;
			using sptr = std::shared_ptr<PipelineLayoutLib>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
			static uptr MakeUnique(const Device* pDevice, API api = API::DIRECTX_12);
			static sptr MakeShared(const Device* pDevice, API api = API::DIRECTX_12);
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
			virtual void OMSetRTVFormats(const Format* formats, uint8_t n) = 0;
			virtual void OMSetDSVFormat(Format format) = 0;
			virtual void OMSetSample(uint8_t count, uint8_t quality = 0) = 0;

			virtual void SetNumViewInstances(uint8_t n, ViewInstanceFlag flags) = 0;
			virtual void SetViewInstance(uint8_t i, const ViewInstance& viewInstance) = 0;
			virtual void SetViewInstances(const ViewInstance* viewInstances, uint8_t n, ViewInstanceFlag flags) = 0;

			virtual Pipeline CreatePipeline(PipelineLib* pPipelineLib, const wchar_t* name = nullptr) const = 0;
			virtual Pipeline GetPipeline(PipelineLib* pPipelineLib, const wchar_t* name = nullptr) const = 0;

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

			using uptr = std::unique_ptr<State>;
			using sptr = std::shared_ptr<State>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
		};

		class XUSG_INTERFACE PipelineLib
		{
		public:
			//PipelineLib();
			//PipelineLib(const Device* pDevice);
			virtual ~PipelineLib() {};

			virtual void SetDevice(const Device* pDevice) = 0;
			virtual void SetPipeline(const State* pState, const Pipeline& pipeline) = 0;

			virtual void SetInputLayout(uint32_t index, const InputElement* pElements, uint32_t numElements) = 0;
			virtual const InputLayout* GetInputLayout(uint32_t index) const = 0;
			virtual const InputLayout* CreateInputLayout(const InputElement* pElements, uint32_t numElements) = 0;

			virtual Pipeline CreatePipeline(const State* pState, const wchar_t* name = nullptr) = 0;
			virtual Pipeline GetPipeline(const State* pState, const wchar_t* name = nullptr) = 0;

			virtual const Graphics::Blend* GetBlend(Graphics::BlendPreset preset, uint8_t numColorRTs = 1) = 0;
			virtual const Graphics::Rasterizer* GetRasterizer(Graphics::RasterizerPreset preset) = 0;
			virtual const Graphics::DepthStencil* GetDepthStencil(Graphics::DepthStencilPreset preset) = 0;

			using uptr = std::unique_ptr<PipelineLib>;
			using sptr = std::shared_ptr<PipelineLib>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
			static uptr MakeUnique(const Device* pDevice, API api = API::DIRECTX_12);
			static sptr MakeShared(const Device* pDevice, API api = API::DIRECTX_12);
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
			virtual bool Create(const Device* pDevice, const Texture* pTarget, Format format,
				uint32_t mipRegionWidth, uint32_t mipRegionHeight, uint32_t mipRegionDepth,
				ResourceFlag resourceFlags = ResourceFlag::NONE, bool isCubeMap = false,
				MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr,
				uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
				TextureLayout textureLayout = TextureLayout::UNKNOWN,
				uint32_t maxThreads = 1) = 0;
			virtual bool CreateResource(const Texture* pTarget, Format format,
				uint32_t mipRegionWidth, uint32_t mipRegionHeight, uint32_t mipRegionDepth,
				ResourceFlag resourceFlags = ResourceFlag::NONE, bool isCubeMap = false,
				MemoryFlag memoryFlags = MemoryFlag::NONE,
				ResourceState initialResourceState = ResourceState::COMMON,
				TextureLayout textureLayout = TextureLayout::UNKNOWN,
				uint32_t maxThreads = 1) = 0;

			virtual Descriptor CreateUAV(const Descriptor& uavHeapStart, uint32_t descriptorIdx, const Resource* pTarget) = 0;

			using uptr = std::unique_ptr<SamplerFeedBack>;
			using sptr = std::shared_ptr<SamplerFeedBack>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
		};

		XUSG_INTERFACE uint32_t SetBarrier(ResourceBarrier* pBufferBarriers, Buffer* pBuffer, ResourceState dstState,
			uint32_t numBarriers = 0, BarrierFlag flags = BarrierFlag::NONE, ResourceState srcState = ResourceState::AUTO,
			uint32_t threadIdx = 0);
		XUSG_INTERFACE uint32_t SetBarrier(ResourceBarrier* pTextureBarriers, Texture* pTexture, ResourceState dstState,
			uint32_t numBarriers = 0, uint32_t indexOrFirstMip = XUSG_BARRIER_ALL_SUBRESOURCES, uint8_t numMipLevels = 0,
			uint16_t firstArraySlice = 0, uint16_t numArraySlices = 0, uint8_t firstPlane = 0, uint8_t numPlanes = 0,
			BarrierFlag flags = BarrierFlag::NONE, ResourceState srcState = ResourceState::AUTO,
			TextureBarrierFlag textureFlags = TextureBarrierFlag::NONE, uint32_t threadIdx = 0);

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

			using uptr = std::unique_ptr<State>;
			using sptr = std::shared_ptr<State>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
		};

		class XUSG_INTERFACE PipelineLib
		{
		public:
			//PipelineLib();
			//PipelineLib(const Device* pDevice);
			virtual ~PipelineLib() {}

			virtual void SetDevice(const Device* pDevice) = 0;
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

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
			static uptr MakeUnique(const Device* pDevice, API api = API::DIRECTX_12);
			static sptr MakeShared(const Device* pDevice, API api = API::DIRECTX_12);
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
			virtual void OMSetRTVFormats(const Format* formats, uint8_t n) = 0;
			virtual void OMSetDSVFormat(Format format) = 0;
			virtual void OMSetSample(uint8_t count, uint8_t quality = 0) = 0;

			virtual void SetNumViewInstances(uint8_t n, ViewInstanceFlag flags) = 0;
			virtual void SetViewInstance(uint8_t i, const ViewInstance& viewInstance) = 0;
			virtual void SetViewInstances(const ViewInstance* viewInstances, uint8_t n, ViewInstanceFlag flags) = 0;

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

			using uptr = std::unique_ptr<State>;
			using sptr = std::shared_ptr<State>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
		};

		class XUSG_INTERFACE PipelineLib
		{
		public:
			//PipelineLib();
			//PipelineLib(const Device* pDevice);
			virtual ~PipelineLib() {};

			virtual void SetDevice(const Device* pDevice) = 0;
			virtual void SetPipeline(const State* pState, const Pipeline& pipeline) = 0;

			virtual void SetInputLayout(uint32_t index, const InputElement* pElements, uint32_t numElements) = 0;
			virtual const InputLayout* GetInputLayout(uint32_t index) const = 0;
			virtual const InputLayout* CreateInputLayout(const InputElement* pElements, uint32_t numElements) = 0;

			virtual Pipeline CreatePipeline(const State* pState, const wchar_t* name = nullptr) = 0;
			virtual Pipeline GetPipeline(const State* pState, const wchar_t* name = nullptr) = 0;

			virtual const Blend* GetBlend(BlendPreset preset, uint8_t numColorRTs = 1) = 0;
			virtual const Rasterizer* GetRasterizer(RasterizerPreset preset) = 0;
			virtual const DepthStencil* GetDepthStencil(DepthStencilPreset preset) = 0;

			virtual ProgramIdentifier GetProgramIdentifier(const Pipeline& stateObject, const wchar_t* programName) const = 0;

			using uptr = std::unique_ptr<PipelineLib>;
			using sptr = std::shared_ptr<PipelineLib>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
			static uptr MakeUnique(const Device* pDevice, API api = API::DIRECTX_12);
			static sptr MakeShared(const Device* pDevice, API api = API::DIRECTX_12);
		};
	}
}
