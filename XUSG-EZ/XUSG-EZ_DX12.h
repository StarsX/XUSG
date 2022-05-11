//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSG-EZ.h"

namespace XUSG
{
	namespace EZ
	{
		class CommandList_DX12 :
			public virtual CommandList,
			public virtual XUSG::CommandList_DX12
		{
		public:
			CommandList_DX12();
			CommandList_DX12(XUSG::CommandList* pCommandList, uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
				uint32_t maxSamplers = 16, const uint32_t* pMaxCbvsEachSpace = nullptr, const uint32_t* pMaxSrvsEachSpace = nullptr,
				const uint32_t* pMaxUavsEachSpace = nullptr, uint32_t maxCbvSpaces = 1, uint32_t maxSrvSpaces = 1, uint32_t maxUavSpaces = 1);
			virtual ~CommandList_DX12();

			bool Create(XUSG::CommandList* pCommandList, uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
				uint32_t maxSamplers = 16, const uint32_t* pMaxCbvsEachSpace = nullptr, const uint32_t* pMaxSrvsEachSpace = nullptr,
				const uint32_t* pMaxUavsEachSpace = nullptr, uint32_t maxCbvSpaces = 1, uint32_t maxSrvSpaces = 1, uint32_t maxUavSpaces = 1);
			bool Create(const Device* pDevice, void* pHandle, uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
				uint32_t maxSamplers = 16, const uint32_t* pMaxCbvsEachSpace = nullptr, const uint32_t* pMaxSrvsEachSpace = nullptr,
				const uint32_t* pMaxUavsEachSpace = nullptr, uint32_t maxCbvSpaces = 1, uint32_t maxSrvSpaces = 1, uint32_t maxUavSpaces = 1,
				const wchar_t* name = nullptr);
			bool Close() const { return XUSG::CommandList_DX12::Close(); }
			bool Reset(const CommandAllocator* pAllocator, const Pipeline& initialState);

			void ClearState(const Pipeline& initialState) const { XUSG::CommandList_DX12::ClearState(initialState); }
			void Draw(
				uint32_t vertexCountPerInstance,
				uint32_t instanceCount,
				uint32_t startVertexLocation,
				uint32_t startInstanceLocation);
			void DrawIndexed(
				uint32_t indexCountPerInstance,
				uint32_t instanceCount,
				uint32_t startIndexLocation,
				int32_t baseVertexLocation,
				uint32_t startInstanceLocation);
			void Dispatch(
				uint32_t threadGroupCountX,
				uint32_t threadGroupCountY,
				uint32_t threadGroupCountZ);
			void CopyBufferRegion(Resource* pDstBuffer, uint64_t dstOffset,
				Resource* pSrcBuffer, uint64_t srcOffset, uint64_t numBytes);
			void CopyTextureRegion(const TextureCopyLocation& dst,
				uint32_t dstX, uint32_t dstY, uint32_t dstZ,
				const TextureCopyLocation& src, const BoxRange* pSrcBox = nullptr);
			void CopyResource(Resource* pDstResource, Resource* pSrcResource);
			void CopyTiles(Resource* pTiledResource, const TiledResourceCoord* pTileRegionStartCoord,
				const TileRegionSize* pTileRegionSize, const Resource* pBuffer, uint64_t bufferStartOffsetInBytes,
				TileCopyFlag flags);
			void ResolveSubresource(Resource* pDstResource, uint32_t dstSubresource,
				Resource* pSrcResource, uint32_t srcSubresource, Format format);
			void IASetInputLayout(const InputLayout* pLayout);
			void IASetIndexBufferStripCutValue(IBStripCutValue ibStripCutValue);
			void RSSetState(Graphics::RasterizerPreset preset);
			void RSSetViewports(uint32_t numViewports, const Viewport* pViewports) const
			{
				XUSG::CommandList_DX12::RSSetViewports(numViewports, pViewports);
			}

			void RSSetScissorRects(uint32_t numRects, const RectRange* pRects) const
			{
				XUSG::CommandList_DX12::RSSetScissorRects(numRects, pRects);
			}

			void OMSetBlendState(Graphics::BlendPreset preset, uint8_t numColorRTs = 1, uint32_t sampleMask = UINT_MAX);
			void OMSetBlendFactor(const float blendFactor[4]) const { XUSG::CommandList_DX12::OMSetBlendFactor(blendFactor); }
			void OMSetStencilRef(uint32_t stencilRef) const { XUSG::CommandList_DX12::OMSetStencilRef(stencilRef); }
			void OMSetSample(uint8_t count, uint8_t quality = 0);
			void DSSetState(Graphics::DepthStencilPreset preset);
			void SetGraphicsShader(Shader::Stage stage, const Blob& shader);
			void SetGraphicsNodeMask(uint32_t nodeMask);
			void SetComputeShader(const Blob& shader);
			void SetComputeNodeMask(uint32_t nodeMask);
			void SetPipelineState(const Pipeline& pipelineState);
			void ExecuteBundle(const XUSG::CommandList* pCommandList) const { XUSG::CommandList_DX12::ExecuteBundle(pCommandList); }
			void SetGraphicsSamplerStates(uint32_t startBinding, uint32_t numSamplers, const SamplerPreset* pSamplerPresets);
			void SetGraphicsResources(Shader::Stage stage, DescriptorType descriptorType, uint32_t startBinding,
				uint32_t numResources, const ResourceView* pResourceViews, uint32_t space = 0);
			void SetComputeSamplerStates(uint32_t startBinding, uint32_t numSamplers, const SamplerPreset* pSamplerPresets);
			void SetComputeResources(DescriptorType descriptorType, uint32_t startBinding,
				uint32_t numResources, const ResourceView* pResourceViews, uint32_t space = 0);
			void IASetPrimitiveTopology(PrimitiveTopology primitiveTopology);
			void IASetIndexBuffer(const IndexBufferView& view) const { XUSG::CommandList_DX12::IASetIndexBuffer(view); }
			void IASetVertexBuffers(uint32_t startSlot, uint32_t numViews, const VertexBufferView* pViews) const
			{
				XUSG::CommandList_DX12::IASetVertexBuffers(startSlot, numViews, pViews);
			}

			void SOSetTargets(uint32_t startSlot, uint32_t numViews, const StreamOutBufferView* pViews, Resource* const* ppResources);
			void OMSetRenderTargets(
				uint32_t numRenderTargets,
				const ResourceView* pRenderTargetViews,
				const ResourceView* pDepthStencilView = nullptr);
			void ClearDepthStencilView(ResourceView& depthStencilView, ClearFlag clearFlags,
				float depth, uint8_t stencil = 0, uint32_t numRects = 0, const RectRange* pRects = nullptr);
			void ClearRenderTargetView(ResourceView& renderTargetView, const float colorRGBA[4],
				uint32_t numRects = 0, const RectRange* pRects = nullptr);
			void ClearUnorderedAccessViewUint(ResourceView& unorderedAccessView, const uint32_t values[4],
				uint32_t numRects = 0, const RectRange* pRects = nullptr);
			void ClearUnorderedAccessViewFloat(ResourceView& unorderedAccessView, const float values[4],
				uint32_t numRects = 0, const RectRange* pRects = nullptr);
			void DiscardResource(const Resource* pResource, uint32_t numRects, const RectRange* pRects,
				uint32_t firstSubresource, uint32_t numSubresources)
			{
				XUSG::CommandList_DX12::DiscardResource(pResource, numRects,
					pRects, firstSubresource, numSubresources);
			}

			void BeginQuery(const QueryPool& queryPool, QueryType type, uint32_t index) const
			{
				XUSG::CommandList_DX12::BeginQuery(queryPool, type, index);
			}

			void EndQuery(const QueryPool& queryPool, QueryType type, uint32_t index) const
			{
				XUSG::CommandList_DX12::EndQuery(queryPool, type, index);
			}

			void ResolveQueryData(const QueryPool& queryPool, QueryType type, uint32_t startIndex,
				uint32_t numQueries, const Resource* pDstBuffer, uint64_t alignedDstBufferOffset) const
			{
				XUSG::CommandList_DX12::ResolveQueryData(queryPool, type, startIndex,
					numQueries, pDstBuffer, alignedDstBufferOffset);
			}

			void SetPredication(const Resource* pBuffer, uint64_t alignedBufferOffset, bool opEqualZero) const
			{
				XUSG::CommandList_DX12::SetPredication(pBuffer, alignedBufferOffset, opEqualZero);
			}

			void SetMarker(uint32_t metaData, const void* pData, uint32_t size) const
			{
				XUSG::CommandList_DX12::SetMarker(metaData, pData, size);
			}

			void BeginEvent(uint32_t metaData, const void* pData, uint32_t size) const
			{
				XUSG::CommandList_DX12::BeginEvent(metaData, pData, size);
			}

			void EndEvent() { XUSG::CommandList_DX12::EndEvent(); }
			void ExecuteIndirect(const CommandLayout* pCommandlayout, uint32_t maxCommandCount,
				const Resource* pArgumentBuffer, uint64_t argumentBufferOffset = 0,
				const Resource* pCountBuffer = nullptr, uint64_t countBufferOffset = 0)
			{
				XUSG::CommandList_DX12::ExecuteIndirect(pCommandlayout, maxCommandCount,
					pArgumentBuffer, argumentBufferOffset, pCountBuffer, countBufferOffset);
			}

			// For resize window
			void ResetDescriptorPool(DescriptorPoolType type);
			void Resize();

			void* GetHandle() const { return XUSG::CommandList_DX12::GetHandle(); }
			void* GetDeviceHandle() const { return XUSG::CommandList_DX12::GetDeviceHandle(); }

			const Device* GetDevice() const { return XUSG::CommandList_DX12::GetDevice(); }
			XUSG::CommandList* AsCommandList() { return dynamic_cast<XUSG::CommandList*>(this); }

		protected:
			enum PipelineLayoutIndex
			{
				GRAPHICS,
				COMPUTE,

				NUM_PIPELINE_LAYOUT
			};

			struct ClearDSV
			{
				Descriptor DepthStencilView;
				ClearFlag ClearFlags;
				float Depth;
				uint8_t Stencil;
				uint32_t NumRects;
				const RectRange* pRects;
			};

			struct ClearRTV
			{
				Descriptor RenderTargetView;
				const float* ColorRGBA;
				uint32_t NumRects;
				const RectRange* pRects;
			};

			static const uint8_t CbvSrvUavTypes = 3;

			bool createPipelineLayouts(uint32_t maxSamplers, const uint32_t* pMaxCbvsEachSpace,
				const uint32_t* pMaxSrvsEachSpace, const uint32_t* pMaxUavsEachSpace,
				uint32_t maxCbvSpaces, uint32_t maxSrvSpaces, uint32_t maxUavSpaces);

			void predraw();
			void setBarriers(uint32_t numResources, const ResourceView* pResourceViews, ResourceState dstState);

			static uint32_t generateBarriers(ResourceBarrier* pBarriers, const ResourceView& resrouceView,
				ResourceState dstState, uint32_t numBarriers = 0, BarrierFlag flags = BarrierFlag::NONE);

			Graphics::PipelineCache::uptr	m_graphicsPipelineCache;
			Compute::PipelineCache::uptr	m_computePipelineCache;
			PipelineLayoutCache::uptr		m_pipelineLayoutCache;
			DescriptorTableCache::uptr		m_descriptorTableCache;

			const XUSG::InputLayout* m_pInputLayout;
			XUSG::PipelineLayout m_pipelineLayouts[NUM_PIPELINE_LAYOUT];
			XUSG::Pipeline m_pipeline;

			Graphics::State::uptr m_graphicsState;
			Compute::State::uptr m_computeState;

			uint32_t m_isGraphicsDirty;
			uint32_t m_isComputeDirty;

			std::vector<Descriptor> m_descriptors;
			std::vector<Util::DescriptorTable::uptr> m_graphicsCbvSrvUavTables[Shader::Stage::NUM_GRAPHICS][CbvSrvUavTypes];
			std::vector<Util::DescriptorTable::uptr> m_computeCbvSrvUavTables[CbvSrvUavTypes];
			Util::DescriptorTable::uptr m_samplerTables[NUM_PIPELINE_LAYOUT];

			std::vector<ResourceBarrier> m_barriers;
			std::vector<ClearDSV> m_clearDSVs;
			std::vector<ClearRTV> m_clearRTVs;

			std::vector<uint32_t> m_graphicsSpaceToParamIndexMap[Shader::Stage::NUM_GRAPHICS][3];
			std::vector<uint32_t> m_computeSpaceToParamIndexMap[CbvSrvUavTypes];
		};
	}
}
