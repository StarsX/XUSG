//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSGUltimate-EZ.h"
#include "XUSG-EZ_Impl.h"
#include "Ultimate/XUSGUltimate_DX12.h"
#ifndef XUSG_NO_VULKAN
#include "Ultimate/XUSGUltimate_VK.h"
#endif

namespace XUSG
{
	namespace Ultimate
	{
		namespace EZ
		{
			template<typename CoreList, typename UltimateList>
			class CommandList_Impl :
				virtual public CommandList,
				virtual public UltimateList,
				virtual public XUSG::EZ::CommandList_Impl<CoreList>
			{
			public:
				CommandList_Impl();
				CommandList_Impl(Ultimate::CommandList* pCommandList,
					uint32_t samplerHeapSize, uint32_t cbvSrvUavHeapSize,
					const uint32_t samplerStartSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxSamplers[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t cbvStartSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxCbvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t srvStartSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxSrvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t uavStartSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxUavSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t max32BitConstants[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t constantSlots[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t constantSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					uint32_t slotExt = 0, uint32_t spaceExt = 0x7FFF0ADE);
				virtual ~CommandList_Impl();

				bool Create(Ultimate::CommandList* pCommandList,
					uint32_t samplerHeapSize, uint32_t cbvSrvUavHeapSize,
					const uint32_t samplerStartSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxSamplers[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t cbvStartSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxCbvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t srvStartSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxSrvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t uavStartSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxUavSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t max32BitConstants[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t constantSlots[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t constantSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					uint32_t slotExt = 0, uint32_t spaceExt = 0x7FFF0ADE);
				bool Create(Device* pDevice, void* pNativeHandle,
					uint32_t samplerHeapSize, uint32_t cbvSrvUavHeapSize,
					const uint32_t samplerStartSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxSamplers[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t cbvStartSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxCbvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t srvStartSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxSrvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t uavStartSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxUavSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t max32BitConstants[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t constantSlots[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t constantSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					uint32_t slotExt = 0, uint32_t spaceExt = 0x7FFF0ADE);

				void SetSamplePositions(uint8_t numSamplesPerPixel, uint8_t numPixels, SamplePosition* pPositions) const
				{
					UltimateList::SetSamplePositions(numSamplesPerPixel, numPixels, pPositions);
				}

				void ResolveSubresourceRegion(Resource* pDstResource, uint32_t dstSubresource,
					uint32_t dstX, uint32_t dstY, Resource* pSrcResource, uint32_t srcSubresource,
					const RectRange& srcRect, Format format, ResolveMode resolveMode);

				void RSSetShadingRate(ShadingRate baseShadingRate, const ShadingRateCombiner* pCombiners) const
				{
					UltimateList::RSSetShadingRate(baseShadingRate, pCombiners);
				}

				void RSSetShadingRateImage(Resource* pShadingRateImage);

				using XUSG::EZ::CommandList_Impl<CoreList>::SetGraphicsPipelineState;
				void SetGraphicsPipelineState(const Pipeline& pipelineState, const State* pState = nullptr);
				void OMSetBlendState(const Graphics::Blend* pBlend, uint32_t sampleMask = UINT_MAX);
				void OMSetBlendState(Graphics::BlendPreset preset, uint8_t numColorRTs = 1, uint32_t sampleMask = UINT_MAX);
				void OMSetSample(uint8_t count, uint8_t quality = 0);
				void RSSetState(const Graphics::Rasterizer* pRasterizer);
				void RSSetState(Graphics::RasterizerPreset preset);
				void DSSetState(const Graphics::DepthStencil* pDepthStencil);
				void DSSetState(Graphics::DepthStencilPreset preset);
				void SetGraphicsShader(Shader::Stage stage, const Blob& shader);
				void MSSetGraphicsShader(Shader::Stage stage, const Blob& shader);
				void SetGraphicsNodeMask(uint32_t nodeMask);
				void DispatchMesh(uint32_t ThreadGroupCountX, uint32_t ThreadGroupCountY, uint32_t ThreadGroupCountZ);
				void DispatchMeshIndirect(CommandLayout* pCommandlayout,
					uint32_t maxCommandCount,
					Resource* pArgumentBuffer,
					uint64_t argumentBufferOffset = 0,
					Resource* pCountBuffer = nullptr,
					uint64_t countBufferOffset = 0);
				void OMSetRenderTargets(
					uint32_t numRenderTargets,
					const XUSG::EZ::ResourceView* pRenderTargetViews,
					const XUSG::EZ::ResourceView* pDepthStencilView = nullptr);
				void WGSetShaderLibrary(uint32_t index, const Blob& shaderLib,
					uint32_t numShaders = 0, const wchar_t** pShaderNames = nullptr);
				void WGSetProgramName(const wchar_t* name);
				void WGOverrideDispatchGrid(const wchar_t* shaderName, uint32_t x, uint32_t y, uint32_t z,
					WorkGraph::BoolOverride isEntry = WorkGraph::BoolOverride::IS_NULL);
				void WGOverrideMaxDispatchGrid(const wchar_t* shaderName, uint32_t x, uint32_t y, uint32_t z,
					WorkGraph::BoolOverride isEntry = WorkGraph::BoolOverride::IS_NULL);
				void DispatchGraph(uint32_t numNodeInputs, const NodeCPUInput* pNodeInputs, uint64_t nodeInputByteStride = 0);
				void DispatchGraph(uint64_t nodeGPUInputAddress, bool isMultiNodes = false);

				const XUSG::PipelineLayout& GetMSPipelineLayout() const;

				uint32_t GetMSConstantParamIndex(Shader::Stage stage) const;

				uint32_t WGGetIndex();
				uint32_t WGGetNumNodes();
				uint32_t WGGetNodeIndex(const WorkGraph::NodeID& nodeID);
				uint32_t WGGetNumEntrypoints();
				uint32_t WGGetEntrypointIndex(const WorkGraph::NodeID& nodeID);
				uint32_t WGGetEntrypointRecordSizeInBytes(uint32_t entryPointIndex);

				WorkGraph::NodeID WGGetNodeID(uint32_t nodeIndex);
				WorkGraph::NodeID WGGetEntrypointID(uint32_t entryPointIndex);

				Ultimate::CommandList* AsUltimateCommandList();

			protected:
				enum StageIndex : uint8_t
				{
					PS,
					MS,
					AS,

					NUM_STAGE
				};

				struct WorkGraphProperty
				{
					uint32_t Index;
					std::vector<uint32_t> RecordByteSizes;
					std::vector<WorkGraph::NodeID> EntrypointIDs;
					std::vector<WorkGraph::NodeID> NodeIDs;
					XUSG::ProgramIdentifier Identifier;
					Buffer::uptr BackingMemory;
					std::unordered_map<const wchar_t*, std::vector<uint32_t>> EntryPointIndices;
					std::unordered_map<const wchar_t*, std::vector<uint32_t>> NodeIndices;
				};

				virtual bool adoptUltimate(Ultimate::CommandList* pCommandList) = 0;
				virtual bool hasMeshShader() const = 0;
				virtual bool hasWorkGraph() const = 0;

				using CoreEZ = XUSG::EZ::CommandList_Impl<CoreList>;
				using CoreEZ::CbvSrvUavTypes;
				using CoreEZ::createGraphicsPipelineLayouts;
				using CoreEZ::createComputePipelineLayouts;
				using CoreEZ::predraw;
				using CoreEZ::predispatch;
				using CoreEZ::preexecuteIndirect;
				using CoreEZ::clearDSVs;
				using CoreEZ::clearRTVs;
				using CoreEZ::clearUAVs;
				using CoreEZ::setBarriers;
				using CoreEZ::m_graphicsPipelineLib;
				using CoreEZ::m_computePipelineLib;
				using CoreEZ::m_pipelineLayoutLib;
				using CoreEZ::m_descriptorTableLib;
				using CoreEZ::m_pipelineLayouts;
				using CoreEZ::m_pipeline;
				using CoreEZ::m_graphicsState;
				using CoreEZ::m_computeState;
				using CoreEZ::m_isGraphicsDirty;
				using CoreEZ::m_isComputeDirty;
				using CoreEZ::m_cbvSrvUavTables;
				using CoreEZ::m_samplerTables;
				using CoreEZ::m_barriers;
				using CoreEZ::m_graphicsSpaceToParamIndexMap;
				using CoreEZ::m_computeSpaceToParamIndexMap;
				using CoreEZ::m_spaceStarts;
				using CoreEZ::recordSpaceStarts;
				using CoreEZ::spaceStart;
				using CoreEZ::m_graphicsSetDescriptorTables;
				using CoreEZ::m_graphicsSetSingle32BitConstants;
				using CoreEZ::m_graphicsSet32BitConstants;
				using CoreEZ::COMPUTE;
				using CoreEZ::m_graphicsConstantParamIndices;
				using CoreEZ::m_computeConstantParamIndex;
				using CoreList::m_pDevice;

				bool init(Ultimate::CommandList* pCommandList, uint32_t samplerHeapSize, uint32_t cbvSrvUavHeapSize);
				bool createMeshShaderPipelineLayouts(
					const uint32_t maxSamplers[Shader::Stage::NUM_STAGE],
					const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_STAGE],
					const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_STAGE],
					const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_STAGE],
					const uint32_t maxCbvSpaces[Shader::Stage::NUM_STAGE],
					const uint32_t maxSrvSpaces[Shader::Stage::NUM_STAGE],
					const uint32_t maxUavSpaces[Shader::Stage::NUM_STAGE],
					const uint32_t max32BitConstants[Shader::Stage::NUM_STAGE],
					const uint32_t constantSlots[Shader::Stage::NUM_STAGE],
					const uint32_t constantSpaces[Shader::Stage::NUM_STAGE],
					uint32_t slotExt, uint32_t spaceExt);

				void predispatchMesh();
				void predispatchGraph();
				void getWorkGraphPipeline();

				const Shader::Stage& getShaderStage(uint8_t index) const;
				StageIndex getShaderStageIndex(Shader::Stage stage) const;

				PipelineLib::uptr		m_meshShaderPipelineLib;
				WorkGraph::PipelineLib::uptr m_workGraphPipelineLib;

				XUSG::PipelineLayout	m_pipelineLayout;

				State::uptr				m_meshShaderState;
				WorkGraph::State::uptr  m_workGraphState;
				std::wstring			m_workGraphName;
				const WorkGraphProperty* m_workGraphProperty;
				bool					m_isWGStateDirty;

				std::unordered_map<Pipeline, std::vector<WorkGraphProperty>> m_workGraphProperties;

				std::vector<uint32_t> m_meshShaderSpaceToParamIndexMap[NUM_STAGE][CbvSrvUavTypes];

				uint32_t m_meshShaderConstantParamIndices[NUM_STAGE];
			};

			extern template class CommandList_Impl<XUSG::CommandList_DX12, Ultimate::CommandList_DX12>;
#ifndef XUSG_NO_VULKAN
			extern template class CommandList_Impl<XUSG::CommandList_VK, Ultimate::CommandList_VK>;
#endif

			class CommandList_DX12 :
				public virtual CommandList_Impl<XUSG::CommandList_DX12, Ultimate::CommandList_DX12>,
				public virtual XUSG::EZ::CommandList_DX12
			{
			public:
				using CommandList_Impl<XUSG::CommandList_DX12, Ultimate::CommandList_DX12>::CommandList_Impl;

			protected:
				bool adoptUltimate(Ultimate::CommandList* pCommandList) override;
				bool hasMeshShader() const override;
				bool hasWorkGraph() const override;
			};

#ifndef XUSG_NO_VULKAN
			class CommandList_VK :
				public virtual CommandList_Impl<XUSG::CommandList_VK, Ultimate::CommandList_VK>,
				public virtual XUSG::EZ::CommandList_VK
			{
			public:
				using CommandList_Impl<XUSG::CommandList_VK, Ultimate::CommandList_VK>::CommandList_Impl;

			protected:
				bool adoptUltimate(Ultimate::CommandList* pCommandList) override;
				bool hasMeshShader() const override;
				bool hasWorkGraph() const override;
			};
#endif
		}
	}
}
