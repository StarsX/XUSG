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
		enum class ResolveMode
		{
			DECOMPRESS,
			MIN,
			MAX,
			AVERAGE,
			ENCODE_SAMPLER_FEEDBACK,
			DECODE_SAMPLER_FEEDBACK
		};

		enum class ShadingRate
		{
			_1X1 = 0,
			_1X2 = 0x1,
			_2X1 = 0x4,
			_2X2 = 0x5,
			_2X4 = 0x6,
			_4X2 = 0x9,
			_4X4 = 0xa
		};

		enum class ShadingRateCombiner
		{
			COMBINER_PASSTHROUGH,
			COMBINER_OVERRIDE,
			COMBINER_MIN,
			COMBINER_MAX,
			COMBINER_SUM
		};

		enum class ProgramType
		{
			GENERIC_PIPELINE,
			RAYTRACING_PIPELINE,
			WORK_GRAPH
		};

		enum class WorkGraphFlag
		{
			NONE = 0,
			INITIALIZE = 0x1
		};

		XUSG_DEF_ENUM_FLAG_OPERATORS(WorkGraphFlag);

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
			virtual void DispatchGraph(uint32_t numNodeInputs, const NodeCPUInput* pNodeInputs, uint64_t nodeInputByteStride = 0) const = 0;
			virtual void DispatchGraph(uint64_t nodeGPUInputAddress, bool isMultiNodes = false) const = 0;

			using uptr = std::unique_ptr<CommandList>;
			using sptr = std::shared_ptr<CommandList>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
			static uptr MakeUnique(XUSG::CommandList& commandList, API api = API::DIRECTX_12);
			static sptr MakeShared(XUSG::CommandList& commandList, API api = API::DIRECTX_12);
		};

		XUSG_INTERFACE ProgramIdentifier GetProgramIdentifier(const Pipeline& stateObject, const wchar_t* programName);

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
				uint32_t maxThreads = 1) = 0;
			virtual bool CreateUAV(const Resource* pTarget) = 0;

			using uptr = std::unique_ptr<SamplerFeedBack>;
			using sptr = std::shared_ptr<SamplerFeedBack>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
		};
	}

	//--------------------------------------------------------------------------------------
	// Mesh-shader pipeline state
	//--------------------------------------------------------------------------------------
	namespace MeshShader
	{
		using BlendPreset = Graphics::BlendPreset;
		using RasterizerPreset = Graphics::RasterizerPreset;
		using DepthStencilPreset = Graphics::DepthStencilPreset;

		using Blend = Graphics::Blend;
		using Rasterizer = Graphics::Rasterizer;
		using DepthStencil = Graphics::DepthStencil;

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

			virtual void OMSetBlendState(const Blend* pBlend, uint32_t sampleMask = UINT_MAX) = 0;
			virtual void RSSetState(const Rasterizer* pRasterizer) = 0;
			virtual void DSSetState(const DepthStencil* pDepthStencil) = 0;

			virtual void OMSetBlendState(BlendPreset preset, PipelineLib* pPipelineLib,
				uint8_t numColorRTs = 1, uint32_t sampleMask = UINT_MAX) = 0;
			virtual void RSSetState(RasterizerPreset preset, PipelineLib* pPipelineLib) = 0;
			virtual void DSSetState(DepthStencilPreset preset, PipelineLib* pPipelineLib) = 0;

			virtual void OMSetNumRenderTargets(uint8_t n) = 0;
			virtual void OMSetRTVFormat(uint8_t i, Format format) = 0;
			virtual void OMSetRTVFormats(const Format* formats, uint8_t n) = 0;
			virtual void OMSetDSVFormat(Format format) = 0;
			virtual void OMSetSample(uint8_t count, uint8_t quality = 0) = 0;

			virtual Pipeline CreatePipeline(PipelineLib* pPipelineLib, const wchar_t* name = nullptr) const = 0;
			virtual Pipeline GetPipeline(PipelineLib* pPipelineLib, const wchar_t* name = nullptr) const = 0;

			virtual const std::string& GetKey() const = 0;

			using uptr = std::unique_ptr<State>;
			using sptr = std::shared_ptr<State>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
		};

		class XUSG_INTERFACE PipelineLib :
			public virtual Graphics::PipelineLib
		{
		public:
			//PipelineLib();
			//PipelineLib(const Device* pDevice);
			virtual ~PipelineLib() {};

			virtual Pipeline CreatePipeline(const State* pState, const wchar_t* name = nullptr) = 0;
			virtual Pipeline GetPipeline(const State* pState, const wchar_t* name = nullptr) = 0;

			using uptr = std::unique_ptr<PipelineLib>;
			using sptr = std::shared_ptr<PipelineLib>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
			static uptr MakeUnique(const Device* pDevice, API api = API::DIRECTX_12);
			static sptr MakeShared(const Device* pDevice, API api = API::DIRECTX_12);
		};
	}

	//--------------------------------------------------------------------------------------
	// Work-graph pipeline state
	//--------------------------------------------------------------------------------------
	namespace WorkGraph
	{
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
				uint32_t numShaders = 0, const /*wchar_t**/void** pShaders = nullptr) = 0;	// pShaders - shader names for DX12
			virtual void SetProgram(const /*wchar_t*/void* program) = 0;					// program - program name for DX12
			virtual void SetLocalPipelineLayout(uint32_t index, const PipelineLayout& layout,
				uint32_t numShaders, const /*wchar_t**/void** pShaders) = 0;				// pShaders - shader names for DX12
			virtual void SetGlobalPipelineLayout(const PipelineLayout& layout) = 0;

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
}
