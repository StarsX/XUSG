//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Core/XUSG.h"

namespace XUSG
{
	//--------------------------------------------------------------------------------------
	// Command list
	//--------------------------------------------------------------------------------------
	namespace Ultimate
	{
		struct SamplePosition
		{
			int8_t X;
			int8_t Y;
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

		class DLL_INTERFACE CommandList :
			public virtual XUSG::CommandList
		{
		public:
			//CommandList();
			virtual ~CommandList() {};

			virtual bool CreateInterface() = 0;

			virtual void SetSamplePositions(uint8_t numSamplesPerPixel, uint8_t numPixels, SamplePosition* pPositions) = 0;
			virtual void RSSetShadingRate(ShadingRate baseShadingRate,
				const ShadingRateCombiner* pCombiners) = 0;
			virtual void RSSetShadingRateImage(const Resource& shadingRateImage) = 0;
			virtual void DispatchMesh(
				uint32_t ThreadGroupCountX,
				uint32_t ThreadGroupCountY,
				uint32_t ThreadGroupCountZ) = 0;

			using uptr = std::unique_ptr<CommandList>;
			using sptr = std::shared_ptr<CommandList>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
			static uptr MakeUnique(XUSG::CommandList& commandList, API api = API::DIRECTX_12);
			static sptr MakeShared(XUSG::CommandList& commandList, API api = API::DIRECTX_12);
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

		class PipelineCache;

		class DLL_INTERFACE State
		{
		public:
			//State();
			virtual ~State() {};

			virtual void SetPipelineLayout(const PipelineLayout& layout) = 0;
			virtual void SetShader(Shader::Stage stage, Blob shader) = 0;
			virtual void SetCachedPipeline(const void* pCachedBlob, size_t size) = 0;
			virtual void SetNodeMask(uint32_t nodeMask) = 0;

			virtual void OMSetBlendState(const Blend* pBlend, uint32_t sampleMask = UINT_MAX) = 0;
			virtual void RSSetState(const Rasterizer* pRasterizer) = 0;
			virtual void DSSetState(const DepthStencil* pDepthStencil) = 0;

			virtual void OMSetBlendState(BlendPreset preset, PipelineCache& pipelineCache,
				uint8_t numColorRTs = 1, uint32_t sampleMask = UINT_MAX) = 0;
			virtual void RSSetState(RasterizerPreset preset, PipelineCache& pipelineCache) = 0;
			virtual void DSSetState(DepthStencilPreset preset, PipelineCache& pipelineCache) = 0;

			virtual void OMSetNumRenderTargets(uint8_t n) = 0;
			virtual void OMSetRTVFormat(uint8_t i, Format format) = 0;
			virtual void OMSetRTVFormats(const Format* formats, uint8_t n) = 0;
			virtual void OMSetDSVFormat(Format format) = 0;
			virtual void OMSetSample(uint8_t count, uint8_t quality = 0) = 0;

			virtual Pipeline CreatePipeline(PipelineCache& pipelineCache, const wchar_t* name = nullptr) const = 0;
			virtual Pipeline GetPipeline(PipelineCache& pipelineCache, const wchar_t* name = nullptr) const = 0;

			virtual const std::string& GetKey() const = 0;

			using uptr = std::unique_ptr<State>;
			using sptr = std::shared_ptr<State>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
		};

		class DLL_INTERFACE PipelineCache :
			public virtual Graphics::PipelineCache
		{
		public:
			//PipelineCache();
			//PipelineCache(const Device& device);
			virtual ~PipelineCache() {};

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
