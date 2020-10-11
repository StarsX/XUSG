//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSGUltimate.h"

namespace XUSG
{
	namespace Meshlet
	{
		enum Stage
		{
			PS,
			MS,
			AS,

			NUM_STAGE
		};

		struct Key
		{
			void* PipelineLayout;
			void* Shaders[NUM_STAGE];
			void* Blend;
			void* Rasterizer;
			void* DepthStencil;
			PrimitiveTopologyType PrimTopologyType;
			uint8_t	NumRenderTargets;
			Format RTVFormats[8];
			Format	DSVFormat;
			uint8_t	SampleCount;
		};

		class State_DX12 :
			public virtual State
		{
		public:
			State_DX12();
			virtual ~State_DX12();

			void SetPipelineLayout(const PipelineLayout& layout);
			void SetShader(Shader::Stage stage, Blob shader);

			void OMSetBlendState(const Blend& blend);
			void RSSetState(const Rasterizer& rasterizer);
			void DSSetState(const DepthStencil& depthStencil);

			void OMSetBlendState(BlendPreset preset, PipelineCache& pipelineCache, uint8_t numColorRTs = 1);
			void RSSetState(RasterizerPreset preset, PipelineCache& pipelineCache);
			void DSSetState(DepthStencilPreset preset, PipelineCache& pipelineCache);

			void OMSetNumRenderTargets(uint8_t n);
			void OMSetRTVFormat(uint8_t i, Format format);
			void OMSetRTVFormats(const Format* formats, uint8_t n);
			void OMSetDSVFormat(Format format);

			Pipeline CreatePipeline(PipelineCache& pipelineCache, const wchar_t* name = nullptr) const;
			Pipeline GetPipeline(PipelineCache& pipelineCache, const wchar_t* name = nullptr) const;

			const std::string& GetKey() const;

		protected:
			Key* m_pKey;
			std::string m_key;
		};

		class PipelineCache_DX12 :
			public virtual PipelineCache,
			public virtual Graphics::PipelineCache_DX12
		{
		public:
			PipelineCache_DX12();
			PipelineCache_DX12(const Device& device);
			virtual ~PipelineCache_DX12();

			Pipeline CreatePipeline(const State& state, const wchar_t* name = nullptr);
			Pipeline GetPipeline(const State& state, const wchar_t* name = nullptr);

		protected:
			Pipeline createPipeline(const Key* pKey, const wchar_t* name);
			Pipeline getPipeline(const std::string& key, const wchar_t* name);
		};
	}
}
