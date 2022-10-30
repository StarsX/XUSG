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
				CommandList_DX12(Ultimate::CommandList* pCommandList,
					uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
					const uint32_t maxSamplers[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxCbvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxSrvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxUavSpaces[Shader::Stage::NUM_STAGE] = nullptr);
				virtual ~CommandList_DX12();

				bool Create(Ultimate::CommandList* pCommandList,
					uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
					const uint32_t maxSamplers[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxCbvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxSrvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxUavSpaces[Shader::Stage::NUM_STAGE] = nullptr);
				bool Create(const Device* pDevice, void* pHandle,
					uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
					const uint32_t maxSamplers[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxCbvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxSrvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxUavSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const wchar_t* name = nullptr);

				void SetSamplePositions(uint8_t numSamplesPerPixel, uint8_t numPixels, SamplePosition* pPositions) const
				{
					Ultimate::CommandList_DX12::SetSamplePositions(numSamplesPerPixel, numPixels, pPositions);
				}

				void ResolveSubresourceRegion(Resource* pDstResource, uint32_t dstSubresource,
					uint32_t dstX, uint32_t dstY, Resource* pSrcResource, uint32_t srcSubresource,
					const RectRange& srcRect, Format format, ResolveMode resolveMode);

				void RSSetShadingRate(ShadingRate baseShadingRate, const ShadingRateCombiner* pCombiners) const
				{
					Ultimate::CommandList_DX12::RSSetShadingRate(baseShadingRate, pCombiners);
				}

				void RSSetShadingRateImage(Resource* pShadingRateImage);

				void SetPipelineState(const Pipeline& pipelineState);
				void MSSetBlendState(MeshShader::BlendPreset preset, uint8_t numColorRTs = 1, uint32_t sampleMask = UINT_MAX);
				void MSSetSample(uint8_t count, uint8_t quality = 0);
				void MSSetRasterizerState(MeshShader::RasterizerPreset preset);
				void MSSetDepthStencilState(MeshShader::DepthStencilPreset preset);
				void MSSetShader(Shader::Stage stage, const Blob& shader);
				void MSSetNodeMask(uint32_t nodeMask);
				void DispatchMesh(uint32_t ThreadGroupCountX, uint32_t ThreadGroupCountY, uint32_t ThreadGroupCountZ);
				void DispatchMeshIndirect(const CommandLayout* pCommandlayout,
					uint32_t maxCommandCount,
					Resource* pArgumentBuffer,
					uint64_t argumentBufferOffset = 0,
					Resource* pCountBuffer = nullptr,
					uint64_t countBufferOffset = 0);

				Ultimate::CommandList* AsUltimateCommandList() { return dynamic_cast<Ultimate::CommandList*>(this); }

			protected:
				enum StageIndex : uint8_t
				{
					PS,
					MS,
					AS,

					NUM_STAGE
				};

				bool init(Ultimate::CommandList* pCommandList, uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize);
				bool createMeshShaderPipelineLayouts(
					const uint32_t maxSamplers[Shader::Stage::NUM_STAGE],
					const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_STAGE],
					const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_STAGE],
					const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_STAGE],
					const uint32_t maxCbvSpaces[Shader::Stage::NUM_STAGE],
					const uint32_t maxSrvSpaces[Shader::Stage::NUM_STAGE],
					const uint32_t maxUavSpaces[Shader::Stage::NUM_STAGE]);

				void predispatchMesh();

				const Shader::Stage& getShaderStage(uint8_t index) const;

				MeshShader::PipelineLib::uptr m_meshShaderPipelineLib;

				XUSG::PipelineLayout m_pipelineLayout;

				MeshShader::State::uptr m_meshShaderState;
				bool m_isMSStateDirty;

				std::vector<uint32_t> m_meshShaderSpaceToParamIndexMap[NUM_STAGE][CbvSrvUavTypes];
			};
		}
	}
}
