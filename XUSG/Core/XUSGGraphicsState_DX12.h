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
			PipelineLayout Layout;
			Blob Shaders[Shader::Stage::NUM_GRAPHICS];
			const Blend* pBlend;
			const Rasterizer* pRasterizer;
			const DepthStencil* pDepthStencil;
			const InputLayout* pInputLayout;
			Blob CachedPipeline;
			PrimitiveTopologyType PrimTopologyType;
			uint8_t	NumRenderTargets;
			Format RTVFormats[8];
			Format	DSVFormat;
			uint8_t	SampleCount;
			uint8_t SampleQuality;
			uint32_t SampleMask;
			uint8_t IBStripCutValue;
			uint32_t NodeMask;
			PipelineFlag Flags;
		};

		class State_DX12 :
			public virtual State
		{
		public:
			State_DX12();
			virtual ~State_DX12();

			void SetPipelineLayout(const PipelineLayout& layout);
			void SetShader(Shader::Stage stage, const Blob& shader);
			void SetCachedPipeline(const Blob& cachedPipeline);
			void SetNodeMask(uint32_t nodeMask);
			void SetFlags(PipelineFlag flag);

			void OMSetBlendState(const Blend* pBlend, uint32_t sampleMask = UINT_MAX);
			void RSSetState(const Rasterizer* pRasterizer);
			void DSSetState(const DepthStencil* pDepthStencil);

			void OMSetBlendState(BlendPreset preset, PipelineLib* pPipelineLib,
				uint8_t numColorRTs = 1, uint32_t sampleMask = UINT_MAX);
			void RSSetState(RasterizerPreset preset, PipelineLib* pPipelineLib);
			void DSSetState(DepthStencilPreset preset, PipelineLib* pPipelineLib);

			void IASetInputLayout(const InputLayout* pLayout);
			void IASetPrimitiveTopologyType(PrimitiveTopologyType type);
			void IASetIndexBufferStripCutValue(IBStripCutValue ibStripCutValue);

			void OMSetNumRenderTargets(uint8_t n);
			void OMSetRTVFormat(uint8_t i, Format format);
			void OMSetRTVFormats(const Format* formats, uint8_t n);
			void OMSetDSVFormat(Format format);
			void OMSetSample(uint8_t count, uint8_t quality = 0);

			Pipeline CreatePipeline(PipelineLib* pPipelineLib, const wchar_t* name = nullptr) const;
			Pipeline GetPipeline(PipelineLib* pPipelineLib, const wchar_t* name = nullptr) const;

			const std::string& GetKey() const;

			void GetHandleDesc(void* pHandleDesc, void* pInputElements, PipelineLib* pPipelineLib) const;

		protected:
			PipelineDesc* m_pKey;
			std::string m_key;
		};

		class PipelineLib_DX12 :
			public virtual PipelineLib
		{
		public:
			PipelineLib_DX12();
			PipelineLib_DX12(const Device* pDevice);
			virtual ~PipelineLib_DX12();

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

			void GetHandleDesc(void* pHandleDesc, void* pInputElements, const std::string& key);

		protected:
			Pipeline createPipeline(const std::string& key, const wchar_t* name);
			Pipeline getPipeline(const std::string& key, const wchar_t* name);

			com_ptr<ID3D12Device> m_device;

			InputLayoutLib_DX12 m_inputLayoutLib;

			std::unordered_map<std::string, com_ptr<ID3D12PipelineState>> m_pipelines;
			std::unique_ptr<Blend>			m_blends[NUM_BLEND_PRESET];
			std::unique_ptr<Rasterizer>		m_rasterizers[NUM_RS_PRESET];
			std::unique_ptr<DepthStencil>	m_depthStencils[NUM_DS_PRESET];

			std::function<Blend(uint8_t)>	m_pfnBlends[NUM_BLEND_PRESET];
			std::function<Rasterizer()>		m_pfnRasterizers[NUM_RS_PRESET];
			std::function<DepthStencil()>	m_pfnDepthStencils[NUM_DS_PRESET];
		};
	}
}
