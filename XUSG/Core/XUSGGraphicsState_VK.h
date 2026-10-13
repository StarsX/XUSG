//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSG_VK.h"
#include "XUSGInputLayout.h"

namespace XUSG
{
	struct GraphicsPipelineCreateVK
	{
		const PipelineLayoutObject* pLayout;
		const Blob* pShaders;
		const char* const* pEntryPoints;
		uint32_t ShaderCount;
		const Graphics::Blend* pBlend;
		const Graphics::Rasterizer* pRasterizer;
		const Graphics::DepthStencil* pDepthStencil;
		const InputLayout* pInputLayout;
		PrimitiveTopologyType Topology;
		uint8_t NumRenderTargets;
		const Format* RTVFormats;
		Format DSVFormat;
		uint8_t SampleCount;
		const wchar_t* Name;
	};

	Pipeline_VK* CreateVKGraphicsPipeline(const Device_VK* pDevice, const GraphicsPipelineCreateVK& desc);
	Pipeline_VK* CreateVKComputePipeline(const Device_VK* pDevice, const PipelineLayoutObject* pLayout,
		const Blob& shader, const char* entryPoint, const wchar_t* name);
	void InitGraphicsPresets(std::function<Graphics::Blend(uint8_t)>* pBlends,
		std::function<Graphics::Rasterizer()>* pRasterizers,
		std::function<Graphics::DepthStencil()>* pDepthStencils);

	namespace Graphics
	{
		struct PipelineDesc;

		class State_VK :
			public virtual State
		{
		public:
			State_VK();
			virtual ~State_VK();
			void* GetBackend() override;

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
			void OMSetRTVFormats(uint8_t n, const Format* formats);
			void OMSetDSVFormat(Format format);
			void OMSetSample(uint8_t count, uint8_t quality = 0);

			Pipeline CreatePipeline(PipelineLib* pPipelineLib, const wchar_t* name = nullptr) const;
			Pipeline GetPipeline(PipelineLib* pPipelineLib, const wchar_t* name = nullptr) const;

			PipelineLayout GetPipelineLayout() const;
			Blob GetShader(Shader::Stage stage) const;
			Blob GetCachedPipeline() const;
			uint32_t GetNodeMask() const;
			PipelineFlag GetFlags() const;

			uint32_t OMGetSampleMask() const;
			const Blend* OMGetBlendState() const;
			const Rasterizer* RSGetState() const;
			const DepthStencil* DSGetState() const;

			const InputLayout* IAGetInputLayout() const;
			PrimitiveTopologyType IAGetPrimitiveTopologyType() const;
			IBStripCutValue IAGetIndexBufferStripCutValue() const;

			uint8_t OMGetNumRenderTargets() const;
			Format OMGetRTVFormat(uint8_t i) const;
			Format OMGetDSVFormat() const;
			uint8_t OMGetSampleCount() const;
			uint8_t OMGetSampleQuality() const;

			void GetHandleDesc(void* pHandleDesc, void* pInputElements) const;

			const std::string& GetKey() const;

		protected:
			PipelineDesc* m_pKey;
			std::string m_key;
		};

		class PipelineLib_VK :
			public virtual PipelineLib
		{
		public:
			PipelineLib_VK();
			PipelineLib_VK(Device* pDevice);
			virtual ~PipelineLib_VK();

			void SetDevice(Device* pDevice);
			void SetPipeline(const State* pState, const Pipeline& pipeline);

			void SetInputLayout(uint32_t index, uint32_t numElements, const InputElement* pElements);
			const InputLayout* GetInputLayout(uint32_t index) const;
			const InputLayout* CreateInputLayout(uint32_t numElements, const InputElement* pElements);

			Pipeline CreatePipeline(const State* pState, const wchar_t* name = nullptr);
			Pipeline GetPipeline(const State* pState, const wchar_t* name = nullptr);

			const Blend* GetBlend(BlendPreset preset, uint8_t numColorRTs = 1);
			const Rasterizer* GetRasterizer(RasterizerPreset preset);
			const DepthStencil* GetDepthStencil(DepthStencilPreset preset);

		protected:
			Pipeline createPipeline(const std::string& key, const wchar_t* name);
			Pipeline getPipeline(const std::string& key, const wchar_t* name);

			Device_VK* m_pDevice;

			InputLayoutLib_Impl m_inputLayoutLib;

			std::unordered_map<std::string, Pipeline_VK*> m_pipelines;
			std::unique_ptr<Blend>			m_blends[NUM_BLEND_PRESET];
			std::unique_ptr<Rasterizer>		m_rasterizers[NUM_RS_PRESET];
			std::unique_ptr<DepthStencil>	m_depthStencils[NUM_DS_PRESET];

			std::function<Blend(uint8_t)>	m_pfnBlends[NUM_BLEND_PRESET];
			std::function<Rasterizer()>		m_pfnRasterizers[NUM_RS_PRESET];
			std::function<DepthStencil()>	m_pfnDepthStencils[NUM_DS_PRESET];

			std::mutex m_mtx;
		};

		inline State_VK* AsVK(State* p)
		{
			assert(!p || p->GetBackend());
			return static_cast<State_VK*>(p ? p->GetBackend() : nullptr);
		}

		inline const State_VK* AsVK(const State* p)
		{
			assert(!p || const_cast<State*>(p)->GetBackend());
			return static_cast<const State_VK*>(p ? const_cast<State*>(p)->GetBackend() : nullptr);
		}
	}
}
