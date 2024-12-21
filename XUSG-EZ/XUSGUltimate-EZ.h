//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSG-EZ.h"
#include "Ultimate/XUSGUltimate.h"

namespace XUSG
{
	namespace Ultimate
	{
		namespace EZ
		{
			XUSG_INTERFACE XUSG::EZ::ResourceView GetUAV(SamplerFeedBack* pResource);

			//--------------------------------------------------------------------------------------
			// Ultimate command list
			//--------------------------------------------------------------------------------------
			class XUSG_INTERFACE CommandList :
				public virtual XUSG::EZ::CommandList
			{
			public:
				//CommandList();
				virtual ~CommandList() {}

				using uptr = std::unique_ptr<CommandList>;
				using sptr = std::shared_ptr<CommandList>;

				// By default maxSamplers[stage] = 16, maxCbvsEachSpace[stage] = 14, maxSrvsEachSpace[stage] = 32, and maxUavsEachSpace[stage] = 16
				virtual bool Create(Ultimate::CommandList* pCommandList,
					uint32_t samplerHeapSize, uint32_t cbvSrvUavHeapSize,
					const uint32_t maxSamplers[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxCbvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxSrvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxUavSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t max32BitConstants[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t constantSlots[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t constantSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					uint32_t slotExt = 0, uint32_t spaceExt = 0x7FFF0ADE) = 0;
				virtual bool Create(const Device* pDevice, void* pHandle,
					uint32_t samplerHeapSize, uint32_t cbvSrvUavHeapSize,
					const uint32_t maxSamplers[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxCbvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxSrvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxUavSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t max32BitConstants[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t constantSlots[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t constantSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					uint32_t slotExt = 0, uint32_t spaceExt = 0x7FFF0ADE,
					const wchar_t* name = nullptr) = 0;

				virtual void SetSamplePositions(uint8_t numSamplesPerPixel, uint8_t numPixels, SamplePosition* pPositions) const = 0;
				virtual void ResolveSubresourceRegion(Resource* pDstResource, uint32_t dstSubresource,
					uint32_t dstX, uint32_t dstY, Resource* pSrcResource, uint32_t srcSubresource,
					const RectRange& srcRect, Format format, ResolveMode resolveMode) = 0;

				virtual void RSSetShadingRate(ShadingRate baseShadingRate, const ShadingRateCombiner* pCombiners) const = 0;
				virtual void RSSetShadingRateImage(Resource* pShadingRateImage) = 0;

				using XUSG::EZ::CommandList::SetGraphicsPipelineState;
				virtual void SetGraphicsPipelineState(const Pipeline& pipelineState, const State* pState = nullptr) = 0;
				virtual void OMSetBlendState(const Graphics::Blend* pBlend, uint32_t sampleMask = UINT_MAX) = 0;
				virtual void OMSetBlendState(Graphics::BlendPreset preset, uint8_t numColorRTs = 1, uint32_t sampleMask = UINT_MAX) = 0;
				virtual void OMSetSample(uint8_t count, uint8_t quality = 0) = 0;
				virtual void RSSetState(const Graphics::Rasterizer* pRasterizer) = 0;
				virtual void RSSetState(Graphics::RasterizerPreset preset) = 0;
				virtual void DSSetState(const Graphics::DepthStencil* pDepthStencil) = 0;
				virtual void DSSetState(Graphics::DepthStencilPreset preset) = 0;
				virtual void SetGraphicsShader(Shader::Stage stage, const Blob& shader) = 0;
				virtual void MSSetGraphicsShader(Shader::Stage stage, const Blob& shader) = 0;
				virtual void SetGraphicsNodeMask(uint32_t nodeMask) = 0;
				virtual void DispatchMesh(uint32_t ThreadGroupCountX, uint32_t ThreadGroupCountY, uint32_t ThreadGroupCountZ) = 0;
				virtual void DispatchMeshIndirect(const CommandLayout* pCommandlayout,
					uint32_t maxCommandCount,
					Resource* pArgumentBuffer,
					uint64_t argumentBufferOffset = 0,
					Resource* pCountBuffer = nullptr,
					uint64_t countBufferOffset = 0) = 0;
				virtual void OMSetRenderTargets(
					uint32_t numRenderTargets,
					const XUSG::EZ::ResourceView* pRenderTargetViews,
					const XUSG::EZ::ResourceView* pDepthStencilView = nullptr) = 0;
				virtual void WGSetShaderLibrary(uint32_t index, const Blob& shaderLib,
					uint32_t numShaders = 0, const wchar_t** pShaderNames = nullptr) = 0;
				virtual void WGSetProgramName(const wchar_t* name) = 0;
				virtual void WGOverrideDispatchGrid(const wchar_t* shaderName, uint32_t x, uint32_t y, uint32_t z,
					WorkGraph::BoolOverride isEntry = WorkGraph::BoolOverride::IS_NULL) = 0;
				virtual void WGOverrideMaxDispatchGrid(const wchar_t* shaderName, uint32_t x, uint32_t y, uint32_t z,
					WorkGraph::BoolOverride isEntry = WorkGraph::BoolOverride::IS_NULL) = 0;
				virtual void DispatchGraph(uint32_t numNodeInputs, const NodeCPUInput* pNodeInputs, uint64_t nodeInputByteStride = 0) = 0;
				virtual void DispatchGraph(uint64_t nodeGPUInputAddress, bool isMultiNodes = false) = 0;

				virtual const XUSG::PipelineLayout& GetMSPipelineLayout() const = 0;

				virtual uint32_t GetMSConstantParamIndex(Shader::Stage stage) const = 0;

				virtual uint32_t WGGetIndex() = 0;
				virtual uint32_t WGGetNumNodes() = 0;
				virtual uint32_t WGGetNodeIndex(const WorkGraph::NodeID& nodeID) = 0;
				virtual uint32_t WGGetNumEntrypoints() = 0;
				virtual uint32_t WGGetEntrypointIndex(const WorkGraph::NodeID& nodeID) = 0;
				virtual uint32_t WGGetEntrypointRecordSizeInBytes(uint32_t entryPointIndex)  = 0;

				virtual WorkGraph::NodeID WGGetNodeID(uint32_t nodeIndex) = 0;
				virtual WorkGraph::NodeID WGGetEntrypointID(uint32_t entryPointIndex) = 0;

				static uptr MakeUnique(API api = API::DIRECTX_12);
				static sptr MakeShared(API api = API::DIRECTX_12);
				static uptr MakeUnique(Ultimate::CommandList* pCommandList,
					uint32_t samplerHeapSize, uint32_t cbvSrvUavHeapSize,
					const uint32_t maxSamplers[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxCbvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxSrvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxUavSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t max32BitConstants[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t constantSlots[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t constantSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					uint32_t slotExt = 0, uint32_t spaceExt = 0x7FFF0ADE,
					API api = API::DIRECTX_12);
				static sptr MakeShared(Ultimate::CommandList* pCommandList,
					uint32_t samplerHeapSize, uint32_t cbvSrvUavHeapSize,
					const uint32_t maxSamplers[Shader::Stage::NUM_STAGE],
					const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxCbvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxSrvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxUavSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t max32BitConstants[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t constantSlots[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t constantSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					uint32_t slotExt = 0, uint32_t spaceExt = 0x7FFF0ADE,
					API api = API::DIRECTX_12);
			};
		}
	}
}
