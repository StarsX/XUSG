//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Core/XUSG.h"

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

		enum class ShadingRate : uint8_t
		{
			TILE_1X1 = 0,
			TILE_1X2 = 0x1,
			TILE_2X1 = 0x4,
			TILE_2X2 = 0x5,
			TILE_2X4 = 0x6,
			TILE_4X2 = 0x9,
			TILE_4X4 = 0xa
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
			void* pRecords;
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

			virtual void SetProgram(ProgramType type, ProgramIdentifier identifier, WorkGraphFlag flags,
				uint64_t backingMemoryAddress = 0, uint64_t backingMemoryByteSize = 0,
				uint64_t localRootArgTableAddress = 0, uint64_t localRootArgTableByteSize = 0,
				uint64_t localRootArgTableByteStride = 0) const = 0;
			virtual void DispatchGraph(uint32_t numNodeInputs, const NodeCPUInput* pNodeInputs, uint64_t nodeInputByteStride = 0) = 0;
			virtual void DispatchGraph(uint64_t nodeGPUInputAddress, bool isMultiNodes = false) const = 0;

			using uptr = std::unique_ptr<CommandList>;
			using sptr = std::shared_ptr<CommandList>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
			static uptr MakeUnique(XUSG::CommandList& commandList, API api = API::DIRECTX_12);
			static sptr MakeShared(XUSG::CommandList& commandList, API api = API::DIRECTX_12);
		};

		//--------------------------------------------------------------------------------------
		// Pipeline state
		//--------------------------------------------------------------------------------------
		using BlendPreset = Graphics::BlendPreset;
		using RasterizerPreset = Graphics::RasterizerPreset;
		using DepthStencilPreset = Graphics::DepthStencilPreset;

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

			virtual Pipeline CreatePipeline(PipelineLib* pPipelineLib, const wchar_t* name = nullptr) const = 0;
			virtual Pipeline GetPipeline(PipelineLib* pPipelineLib, const wchar_t* name = nullptr) const = 0;

			virtual const std::string& GetKey() const = 0;

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
			virtual void SetPipeline(const std::string& key, const Pipeline& pipeline) = 0;

			virtual void SetInputLayout(uint32_t index, const InputElement* pElements, uint32_t numElements) = 0;
			virtual const InputLayout* GetInputLayout(uint32_t index) const = 0;
			virtual const InputLayout* CreateInputLayout(const InputElement* pElements, uint32_t numElements) = 0;

			virtual Pipeline CreatePipeline(const State* pState, const wchar_t* name = nullptr) = 0;
			virtual Pipeline GetPipeline(const State* pState, const wchar_t* name = nullptr) = 0;

			virtual const Graphics::Blend* GetBlend(BlendPreset preset, uint8_t numColorRTs = 1) = 0;
			virtual const Graphics::Rasterizer* GetRasterizer(RasterizerPreset preset) = 0;
			virtual const Graphics::DepthStencil* GetDepthStencil(DepthStencilPreset preset) = 0;

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

		XUSG_INTERFACE ProgramIdentifier GetProgramIdentifier(const Pipeline& stateObject, const wchar_t* programName);
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
			virtual void SetProgram(const wchar_t* programName) = 0;
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

			virtual const std::string& GetKey() = 0;

			virtual const wchar_t* GetProgramName(uint32_t workGraphIndex) const = 0;

			virtual uint32_t GetNumWorkGraphs() const = 0;
			virtual uint32_t GetWorkGraphIndex(const wchar_t* pProgramName) const = 0;
			virtual uint32_t GetNumNodes(uint32_t workGraphIndex) const = 0;
			virtual uint32_t GetNodeIndex(uint32_t workGraphIndex, const NodeID& nodeID) const = 0;
			virtual uint32_t GetNodeLocalRootArgumentsTableIndex(uint32_t workGraphIndex, uint32_t nodeIndex) const = 0;
			virtual uint32_t GetNumEntrypoints(uint32_t workGraphIndex) const = 0;
			virtual uint32_t GetEntrypointIndex(uint32_t workGraphIndex, const NodeID& nodeID) const = 0;
			virtual uint32_t GetEntrypointRecordSizeInBytes(uint32_t workGraphIndex, uint32_t entrypointIndex) const = 0;

			virtual NodeID GetNodeID(uint32_t workGraphIndex, uint32_t nodeIndex) const = 0;
			virtual NodeID* GetNodeID(NodeID* pRetVal, uint32_t workGraphIndex, uint32_t nodeIndex) const = 0;
			virtual NodeID GetEntrypointID(uint32_t workGraphIndex, uint32_t entrypointIndex) const = 0;
			virtual NodeID* GetEntrypointID(NodeID* pRetVal, uint32_t workGraphIndex, uint32_t entrypointIndex) const = 0;

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
			virtual void SetPipeline(const std::string& key, const Pipeline& pipeline) = 0;

			virtual Pipeline CreatePipeline(State* pState, const wchar_t* name = nullptr) = 0;
			virtual Pipeline GetPipeline(State* pState, const wchar_t* name = nullptr) = 0;

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
	namespace GenericProgram
	{
		using BlendPreset = Ultimate::BlendPreset;
		using RasterizerPreset = Ultimate::RasterizerPreset;
		using DepthStencilPreset = Ultimate::DepthStencilPreset;

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

			virtual Pipeline CreatePipeline(PipelineLib* pPipelineLib, const wchar_t* name = nullptr) const = 0;
			virtual Pipeline GetPipeline(PipelineLib* pPipelineLib, const wchar_t* name = nullptr) const = 0;

			virtual const std::string& GetKey() const = 0;

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
			virtual void SetPipeline(const std::string& key, const Pipeline& pipeline) = 0;

			virtual void SetInputLayout(uint32_t index, const InputElement* pElements, uint32_t numElements) = 0;
			virtual const InputLayout* GetInputLayout(uint32_t index) const = 0;
			virtual const InputLayout* CreateInputLayout(const InputElement* pElements, uint32_t numElements) = 0;

			virtual Pipeline CreatePipeline(const State* pState, const wchar_t* name = nullptr) = 0;
			virtual Pipeline GetPipeline(const State* pState, const wchar_t* name = nullptr) = 0;

			virtual const Blend* GetBlend(BlendPreset preset, uint8_t numColorRTs = 1) = 0;
			virtual const Rasterizer* GetRasterizer(RasterizerPreset preset) = 0;
			virtual const DepthStencil* GetDepthStencil(DepthStencilPreset preset) = 0;

			using uptr = std::unique_ptr<PipelineLib>;
			using sptr = std::shared_ptr<PipelineLib>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
			static uptr MakeUnique(const Device* pDevice, API api = API::DIRECTX_12);
			static sptr MakeShared(const Device* pDevice, API api = API::DIRECTX_12);
		};
	}
}
