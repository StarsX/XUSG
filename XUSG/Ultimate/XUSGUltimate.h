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

		struct SamplePosition
		{
			int8_t X;
			int8_t Y;
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

			using uptr = std::unique_ptr<CommandList>;
			using sptr = std::shared_ptr<CommandList>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
			static uptr MakeUnique(XUSG::CommandList& commandList, API api = API::DIRECTX_12);
			static sptr MakeShared(XUSG::CommandList& commandList, API api = API::DIRECTX_12);
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
}
