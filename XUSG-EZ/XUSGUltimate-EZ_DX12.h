//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSGUltimate-EZ.h"

namespace XUSG
{
	namespace Ultimate
	{
		namespace EZ
		{
			class CommandList_DX12 :
				virtual public CommandList,
				virtual public Ultimate::CommandList_DX12,
				virtual public XUSG::EZ::CommandList_DX12
			{
			public:
				CommandList_DX12();
				CommandList_DX12(Ultimate::CommandList* pCommandList, uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
					uint32_t maxSamplers = 16, const uint32_t* pMaxCbvsEachSpace = nullptr, const uint32_t* pMaxSrvsEachSpace = nullptr,
					const uint32_t* pMaxUavsEachSpace = nullptr, uint32_t maxCbvSpaces = 1, uint32_t maxSrvSpaces = 1, uint32_t maxUavSpaces = 1);
				virtual ~CommandList_DX12();

				bool Create(Ultimate::CommandList* pCommandList, uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
					uint32_t maxSamplers = 16, const uint32_t* pMaxCbvsEachSpace = nullptr, const uint32_t* pMaxSrvsEachSpace = nullptr,
					const uint32_t* pMaxUavsEachSpace = nullptr, uint32_t maxCbvSpaces = 1, uint32_t maxSrvSpaces = 1, uint32_t maxUavSpaces = 1);
				bool Create(const Device* pDevice, void* pHandle, uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
					uint32_t maxSamplers = 16, const uint32_t* pMaxCbvsEachSpace = nullptr, const uint32_t* pMaxSrvsEachSpace = nullptr,
					const uint32_t* pMaxUavsEachSpace = nullptr, uint32_t maxCbvSpaces = 1, uint32_t maxSrvSpaces = 1, uint32_t maxUavSpaces = 1,
					const wchar_t* name = nullptr);

				void SetSamplePositions(uint8_t numSamplesPerPixel, uint8_t numPixels, SamplePosition* pPositions) const
				{
					Ultimate::CommandList_DX12::SetSamplePositions(numSamplesPerPixel, numPixels, pPositions);
				}

				void ResolveSubresourceRegion(const Resource* pDstResource, uint32_t dstSubresource,
					uint32_t dstX, uint32_t dstY, const Resource* pSrcResource, uint32_t srcSubresource,
					const RectRange& srcRect, Format format, ResolveMode resolveMode);

				void RSSetShadingRate(ShadingRate baseShadingRate, const ShadingRateCombiner* pCombiners) const
				{
					Ultimate::CommandList_DX12::RSSetShadingRate(baseShadingRate, pCombiners);
				}

				void RSSetShadingRateImage(const Resource* pShadingRateImage);

				void SetPipelineState(const Pipeline& pipelineState);
				void MSSetBlendState(MeshShader::BlendPreset preset, uint8_t numColorRTs = 1, uint32_t sampleMask = UINT_MAX);
				void MSSetSample(uint8_t count, uint8_t quality = 0);
				void MSSetRasterizerState(MeshShader::RasterizerPreset preset);
				void MSSetDepthStencilState(MeshShader::DepthStencilPreset preset);
				void MSSetShader(Shader::Stage stage, const Blob& shader);
				void MSSetNodeMask(uint32_t nodeMask);
				void DispatchMesh(uint32_t ThreadGroupCountX, uint32_t ThreadGroupCountY, uint32_t ThreadGroupCountZ);

				Ultimate::CommandList* AsUltimateCommandList() { return dynamic_cast<Ultimate::CommandList*>(this); }

			protected:
				enum StageIndex : uint8_t
				{
					PS,
					MS,
					AS,

					NUM_STAGE
				};

				bool createMeshShaderPipelineLayouts(uint32_t maxSamplers, const uint32_t* pMaxCbvsEachSpace,
					const uint32_t* pMaxSrvsEachSpace, const uint32_t* pMaxUavsEachSpace,
					uint32_t maxCbvSpaces, uint32_t maxSrvSpaces, uint32_t maxUavSpaces);

				const Shader::Stage& getShaderStage(uint8_t index) const;

				MeshShader::PipelineCache::uptr m_meshShaderPipelineCache;

				XUSG::PipelineLayout m_pipelineLayout;

				MeshShader::State::uptr m_meshShaderState;
				bool m_isMSStateDirty;

				std::vector<uint32_t> m_meshShaderSpaceToParamIndexMap[NUM_STAGE][CbvSrvUavTypes];
			};
		}
	}
}
