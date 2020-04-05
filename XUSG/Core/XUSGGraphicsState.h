//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSG.h"
#include "XUSGInputLayout.h"

namespace XUSG
{
	namespace Graphics
	{
		struct Key
		{
			void* PipelineLayout;
			void* Shaders[Shader::Stage::NUM_GRAPHICS];
			void* Blend;
			void* Rasterizer;
			void* DepthStencil;
			void* InputLayout;
			PrimitiveTopologyType PrimTopologyType;
			uint8_t	NumRenderTargets;
			Format RTVFormats[8];
			Format	DSVFormat;
			uint8_t	SampleCount;
		};

		class State_DX12 :
			public State
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

			void IASetInputLayout(const InputLayout& layout);
			void IASetPrimitiveTopologyType(PrimitiveTopologyType type);

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
			public PipelineCache
		{
		public:
			PipelineCache_DX12();
			PipelineCache_DX12(const Device& device);
			virtual ~PipelineCache_DX12();

			void SetDevice(const Device& device);
			void SetPipeline(const std::string& key, const Pipeline& pipeline);

			void SetInputLayout(uint32_t index, const InputElementTable& elementTable);
			InputLayout GetInputLayout(uint32_t index) const;
			InputLayout CreateInputLayout(const InputElementTable& elementTable);

			Pipeline CreatePipeline(const State& state, const wchar_t* name = nullptr);
			Pipeline GetPipeline(const State& state, const wchar_t* name = nullptr);

			const Blend& GetBlend(BlendPreset preset, uint8_t numColorRTs = 1);
			const Rasterizer& GetRasterizer(RasterizerPreset preset);
			const DepthStencil& GetDepthStencil(DepthStencilPreset preset);

		protected:
			Pipeline createPipeline(const Key* pKey, const wchar_t* name);
			Pipeline getPipeline(const std::string& key, const wchar_t* name);

			Device m_device;

			InputLayoutPool	m_inputLayoutPool;

			std::unordered_map<std::string, Pipeline> m_pipelines;
			Blend			m_blends[NUM_BLEND_PRESET];
			Rasterizer		m_rasterizers[NUM_RS_PRESET];
			DepthStencil	m_depthStencils[NUM_DS_PRESET];

			std::function<Blend(uint8_t)>	m_pfnBlends[NUM_BLEND_PRESET];
			std::function<Rasterizer()>		m_pfnRasterizers[NUM_RS_PRESET];
			std::function<DepthStencil()>	m_pfnDepthStencils[NUM_DS_PRESET];

			static DepthStencil DepthStencilDefault();
			static DepthStencil DepthStencilNone();
			static DepthStencil DepthRead();
			static DepthStencil DepthReadLessEqual();
			static DepthStencil DepthReadEqual();

			static Blend DefaultOpaque(uint8_t n);
			static Blend Premultiplied(uint8_t n);
			static Blend Additive(uint8_t n);
			static Blend NonPremultiplied(uint8_t n);
			static Blend NonPremultipliedRT0(uint8_t n);
			static Blend AlphaToCoverage(uint8_t n);
			static Blend Accumulative(uint8_t n);
			static Blend AutoNonPremultiplied(uint8_t n);
			static Blend ZeroAlphaNonPremultiplied(uint8_t n);
			static Blend Multiplied(uint8_t n);
			static Blend WeightedPremul(uint8_t n);
			static Blend WeightedPremulPerRT(uint8_t n);
			static Blend WeightedPerRT(uint8_t n);
			static Blend SelectMin(uint8_t n);
			static Blend SelectMax(uint8_t n);

			static Rasterizer RasterizerDefault();
			static Rasterizer CullBack();
			static Rasterizer CullNone();
			static Rasterizer CullFront();
			static Rasterizer CullWireframe();
		};
	}
}
