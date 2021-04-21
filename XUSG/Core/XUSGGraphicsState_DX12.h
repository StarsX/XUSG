//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSG.h"
#include "XUSGInputLayout_DX12.h"

namespace XUSG
{
	namespace Graphics
	{
		struct PipelineDesc
		{
			void* PipelineLayout;
			void* Shaders[Shader::Stage::NUM_GRAPHICS];
			const Blend* pBlend;
			const Rasterizer* pRasterizer;
			const DepthStencil* pDepthStencil;
			const InputLayout* pInputLayout;
			const void* pCachedBlob;
			size_t CachedBlobSize;
			PrimitiveTopologyType PrimTopologyType;
			uint8_t	NumRenderTargets;
			Format RTVFormats[8];
			Format	DSVFormat;
			uint8_t	SampleCount;
			uint8_t SampleQuality;
			uint32_t SampleMask;
			uint8_t IBStripCutValue;
			uint32_t NodeMask;
		};

		class State_DX12 :
			public virtual State
		{
		public:
			State_DX12();
			virtual ~State_DX12();

			void SetPipelineLayout(const PipelineLayout& layout);
			void SetShader(Shader::Stage stage, const Blob& shader);
			void SetCachedPipeline(const void* pCachedBlob, size_t size);
			void SetNodeMask(uint32_t nodeMask);

			void OMSetBlendState(const Blend* pBlend, uint32_t sampleMask = UINT_MAX);
			void RSSetState(const Rasterizer* pRasterizer);
			void DSSetState(const DepthStencil* pDepthStencil);

			void OMSetBlendState(BlendPreset preset, PipelineCache* pPipelineCache,
				uint8_t numColorRTs = 1, uint32_t sampleMask = UINT_MAX);
			void RSSetState(RasterizerPreset preset, PipelineCache* pPipelineCache);
			void DSSetState(DepthStencilPreset preset, PipelineCache* pPipelineCache);

			void IASetInputLayout(const InputLayout* pLayout);
			void IASetPrimitiveTopologyType(PrimitiveTopologyType type);
			void IASetIndexBufferStripCutValue(IBStripCutValue ibStripCutValue);

			void OMSetNumRenderTargets(uint8_t n);
			void OMSetRTVFormat(uint8_t i, Format format);
			void OMSetRTVFormats(const Format* formats, uint8_t n);
			void OMSetDSVFormat(Format format);
			void OMSetSample(uint8_t count, uint8_t quality = 0);

			Pipeline CreatePipeline(PipelineCache* pPipelineCache, const wchar_t* name = nullptr) const;
			Pipeline GetPipeline(PipelineCache* pPipelineCache, const wchar_t* name = nullptr) const;

			const std::string& GetKey() const;

		protected:
			PipelineDesc* m_pKey;
			std::string m_key;
		};

		class PipelineCache_DX12 :
			public virtual PipelineCache
		{
		public:
			PipelineCache_DX12();
			PipelineCache_DX12(const Device* pDevice);
			virtual ~PipelineCache_DX12();

			void SetDevice(const Device* pDevice);
			void SetPipeline(const std::string& key, const Pipeline& pipeline);

			void SetInputLayout(uint32_t index, const InputElement* pElements, uint32_t numElements);
			const InputLayout* GetInputLayout(uint32_t index) const;
			const InputLayout* CreateInputLayout(const InputElement* pElements, uint32_t numElements);

			Pipeline CreatePipeline(const State* pState, const wchar_t* name = nullptr);
			Pipeline GetPipeline(const State* pState, const wchar_t* name = nullptr);

			const Blend* GetBlend(BlendPreset preset, uint8_t numColorRTs = 1);
			const Rasterizer* GetRasterizer(RasterizerPreset preset);
			const DepthStencil* GetDepthStencil(DepthStencilPreset preset);

		protected:
			Pipeline createPipeline(const std::string& key, const wchar_t* name);
			Pipeline getPipeline(const std::string& key, const wchar_t* name);

			com_ptr<ID3D12Device> m_device;

			InputLayoutPool_DX12 m_inputLayoutPool;

			std::unordered_map<std::string, com_ptr<ID3D12PipelineState>> m_pipelines;
			std::unique_ptr<Blend>			m_blends[NUM_BLEND_PRESET];
			std::unique_ptr<Rasterizer>		m_rasterizers[NUM_RS_PRESET];
			std::unique_ptr<DepthStencil>	m_depthStencils[NUM_DS_PRESET];

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
			static Rasterizer FillWireframe();
		};
	}
}
