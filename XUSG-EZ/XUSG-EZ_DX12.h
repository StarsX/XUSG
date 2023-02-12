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
			CommandList_DX12(XUSG::CommandList* pCommandList,
				uint32_t samplerHeapSize, uint32_t cbvSrvUavHeapSize,
				const uint32_t maxSamplers[Shader::Stage::NUM_STAGE],
				const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_STAGE],
				const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_STAGE],
				const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_STAGE],
				const uint32_t maxCbvSpaces[Shader::Stage::NUM_STAGE],
				const uint32_t maxSrvSpaces[Shader::Stage::NUM_STAGE],
				const uint32_t maxUavSpaces[Shader::Stage::NUM_STAGE]);
			virtual ~CommandList_DX12();

			bool Create(XUSG::CommandList* pCommandList,
				uint32_t samplerHeapSize, uint32_t cbvSrvUavHeapSize,
				const uint32_t maxSamplers[Shader::Stage::NUM_STAGE] = nullptr,
				const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
				const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
				const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
				const uint32_t maxCbvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
				const uint32_t maxSrvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
				const uint32_t maxUavSpaces[Shader::Stage::NUM_STAGE] = nullptr);
			bool Create(const Device* pDevice, void* pHandle,
				uint32_t samplerHeapSize, uint32_t cbvSrvUavHeapSize,
				const uint32_t maxSamplers[Shader::Stage::NUM_STAGE] = nullptr,
				const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
				const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
				const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
				const uint32_t maxCbvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
				const uint32_t maxSrvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
				const uint32_t maxUavSpaces[Shader::Stage::NUM_STAGE] = nullptr,
				const wchar_t* name = nullptr);
			bool Close(RenderTarget* pBackBuffer = nullptr);
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
			void DrawIndirect(const CommandLayout* pCommandlayout,
				uint32_t maxCommandCount,
				Resource* pArgumentBuffer,
				uint64_t argumentBufferOffset = 0,
				Resource* pCountBuffer = nullptr,
				uint64_t countBufferOffset = 0);
			void Dispatch(
				uint32_t threadGroupCountX,
				uint32_t threadGroupCountY,
				uint32_t threadGroupCountZ);
			void DispatchIndirect(const CommandLayout* pCommandlayout,
				uint32_t maxCommandCount,
				Resource* pArgumentBuffer,
				uint64_t argumentBufferOffset = 0,
				Resource* pCountBuffer = nullptr,
				uint64_t countBufferOffset = 0);
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
			void RSSetState(const Graphics::Rasterizer* pRasterizer);
			void RSSetState(Graphics::RasterizerPreset preset);
			void RSSetViewports(uint32_t numViewports, const Viewport* pViewports) const
			{
				XUSG::CommandList_DX12::RSSetViewports(numViewports, pViewports);
			}

			void RSSetScissorRects(uint32_t numRects, const RectRange* pRects) const
			{
				XUSG::CommandList_DX12::RSSetScissorRects(numRects, pRects);
			}

			void OMSetBlendState(const Graphics::Blend* pBlend, uint32_t sampleMask = UINT_MAX);
			void OMSetBlendState(Graphics::BlendPreset preset, uint8_t numColorRTs = 1, uint32_t sampleMask = UINT_MAX);
			void OMSetBlendFactor(const float blendFactor[4]) const { XUSG::CommandList_DX12::OMSetBlendFactor(blendFactor); }
			void OMSetStencilRef(uint32_t stencilRef) const { XUSG::CommandList_DX12::OMSetStencilRef(stencilRef); }
			void OMSetSample(uint8_t count, uint8_t quality = 0);
			void DSSetState(const Graphics::DepthStencil* pDepthStencil);
			void DSSetState(Graphics::DepthStencilPreset preset);
			void SetGraphicsShader(Shader::Stage stage, const Blob& shader);
			void SetGraphicsNodeMask(uint32_t nodeMask);
			void SetComputeShader(const Blob& shader);
			void SetComputeNodeMask(uint32_t nodeMask);
			void SetPipelineState(const Pipeline& pipelineState);
			void ExecuteBundle(const XUSG::CommandList* pCommandList) const { XUSG::CommandList_DX12::ExecuteBundle(pCommandList); }
			void SetSamplers(Shader::Stage stage, uint32_t startBinding, uint32_t numSamplers, const Sampler* const* ppSamplers);
			void SetSamplerStates(Shader::Stage stage, uint32_t startBinding, uint32_t numSamplers, const SamplerPreset* pSamplerPresets);
			void SetResources(Shader::Stage stage, DescriptorType descriptorType, uint32_t startBinding,
				uint32_t numResources, const ResourceView* pResourceViews, uint32_t space = 0);
			void IASetPrimitiveTopology(PrimitiveTopology primitiveTopology);
			void IASetIndexBuffer(const IndexBufferView& view);
			void IASetVertexBuffers(uint32_t startSlot, uint32_t numViews, const VertexBufferView* pViews);
			void SOSetTargets(uint32_t startSlot, uint32_t numViews, const StreamOutBufferView* pViews, Resource* const* ppResources);
			void OMSetRenderTargets(
				uint32_t numRenderTargets,
				const ResourceView* pRenderTargetViews,
				const ResourceView* pDepthStencilView = nullptr);
			void ClearDepthStencilView(const ResourceView& depthStencilView, ClearFlag clearFlags,
				float depth, uint8_t stencil = 0, uint32_t numRects = 0, const RectRange* pRects = nullptr);
			void ClearRenderTargetView(const ResourceView& renderTargetView, const float colorRGBA[4],
				uint32_t numRects = 0, const RectRange* pRects = nullptr);
			void ClearUnorderedAccessViewUint(const ResourceView& unorderedAccessView, const uint32_t values[4],
				uint32_t numRects = 0, const RectRange* pRects = nullptr);
			void ClearUnorderedAccessViewFloat(const ResourceView& unorderedAccessView, const float values[4],
				uint32_t numRects = 0, const RectRange* pRects = nullptr);
			void DiscardResource(const Resource* pResource, uint32_t numRects, const RectRange* pRects,
				uint32_t firstSubresource, uint32_t numSubresources)
			{
				XUSG::CommandList_DX12::DiscardResource(pResource, numRects,
					pRects, firstSubresource, numSubresources);
			}

			void BeginQuery(const QueryHeap& queryHeap, QueryType type, uint32_t index) const
			{
				XUSG::CommandList_DX12::BeginQuery(queryHeap, type, index);
			}

			void EndQuery(const QueryHeap& queryHeap, QueryType type, uint32_t index) const
			{
				XUSG::CommandList_DX12::EndQuery(queryHeap, type, index);
			}

			void ResolveQueryData(const QueryHeap& queryHeap, QueryType type, uint32_t startIndex,
				uint32_t numQueries, const Resource* pDstBuffer, uint64_t alignedDstBufferOffset) const
			{
				XUSG::CommandList_DX12::ResolveQueryData(queryHeap, type, startIndex,
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

			// For resize window
			void ResetDescriptorHeap(DescriptorHeapType type);
			void Resize();

			void Blit(Texture* pDstResource, Texture* pSrcResource, SamplerPreset sampler,
				const Blob& customShader = nullptr, uint32_t dstMip = 0, uint32_t srcMip = 0);
			void Blit(Texture3D* pDstResource, Texture* pSrcResource, SamplerPreset sampler,
				const Blob& customShader = nullptr, uint32_t dstMip = 0, uint32_t srcMip = 0);

			void GenerateMips(Texture* pResource, SamplerPreset sampler, const Blob& customShader = nullptr);
			void GenerateMips(Texture3D* pResource, SamplerPreset sampler, const Blob& customShader = nullptr);

			const Graphics::Blend* GetBlend(Graphics::BlendPreset preset, uint8_t numColorRTs = 1);
			const Graphics::Rasterizer* GetRasterizer(Graphics::RasterizerPreset preset);
			const Graphics::DepthStencil* GetDepthStencil(Graphics::DepthStencilPreset preset);

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

			enum ShaderIndex
			{
				VS_BLIT_2D,
				PS_BLIT_2D,
				CS_BLIT_2D,
				CS_BLIT_3D,

				NUM_SHADER
			};

			struct ClearDSV
			{
				const Resource* pResource;
				Descriptor DepthStencilView;
				ClearFlag ClearFlags;
				float Depth;
				uint8_t Stencil;
				std::vector<RectRange> Rects;
			};

			struct ClearRTV
			{
				const Resource* pResource;
				Descriptor RenderTargetView;
				float ColorRGBA[4];
				std::vector<RectRange> Rects;
			};

			struct ClearUAVUint
			{
				const Resource* pResource;
				Descriptor UnorderedAccessView;
				DescriptorTable UAVTable;
				uint32_t Values[4];
				std::vector<RectRange> Rects;
			};

			struct ClearUAVFloat
			{
				const Resource* pResource;
				Descriptor UnorderedAccessView;
				DescriptorTable UAVTable;
				float Values[4];
				std::vector<RectRange> Rects;
			};

			static const uint8_t CbvSrvUavTypes = 3;

			bool init(XUSG::CommandList* pCommandList, uint32_t samplerHeapSize, uint32_t cbvSrvUavHeapSize);
			bool createGraphicsPipelineLayouts(
				const uint32_t maxSamplers[Shader::Stage::NUM_GRAPHICS],
				const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_GRAPHICS],
				const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_GRAPHICS],
				const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_GRAPHICS],
				const uint32_t maxCbvSpaces[Shader::Stage::NUM_GRAPHICS],
				const uint32_t maxSrvSpaces[Shader::Stage::NUM_GRAPHICS],
				const uint32_t maxUavSpaces[Shader::Stage::NUM_GRAPHICS]);
			bool createComputePipelineLayouts(uint32_t maxSamplers, const uint32_t* pMaxCbvsEachSpace,
				const uint32_t* pMaxSrvsEachSpace, const uint32_t* pMaxUavsEachSpace,
				uint32_t maxCbvSpaces, uint32_t maxSrvSpaces, uint32_t maxUavSpaces);
			bool createShaders();

			void predraw();
			void predispatch();
			void preexecuteIndirect(Resource* pArgumentBuffer, Resource* pCountBuffer);
			void clearDSVs();
			void clearRTVs();
			void clearUAVsUint();
			void clearUAVsFloat();
			void setBarriers(uint32_t numResources, const ResourceView* pResourceViews);

			static uint32_t generateBarriers(ResourceBarrier* pBarriers, const ResourceView& resrouceView,
				uint32_t numBarriers = 0, BarrierFlag flags = BarrierFlag::NONE);

			ShaderLib::uptr m_shaderLib;
			Graphics::PipelineLib::uptr	m_graphicsPipelineLib;
			Compute::PipelineLib::uptr	m_computePipelineLib;
			PipelineLayoutLib::uptr		m_pipelineLayoutLib;
			DescriptorTableLib::uptr	m_descriptorTableLib;

			const XUSG::InputLayout* m_pInputLayout;
			XUSG::PipelineLayout m_pipelineLayouts[NUM_PIPELINE_LAYOUT];
			XUSG::Pipeline m_pipeline;

			Graphics::State::uptr m_graphicsState;
			Compute::State::uptr m_computeState;

			uint32_t m_isGraphicsDirty;
			uint32_t m_isComputeDirty;

			std::vector<Descriptor> m_descriptors;
			std::vector<Util::DescriptorTable::uptr> m_cbvSrvUavTables[Shader::Stage::NUM_STAGE][CbvSrvUavTypes];
			Util::DescriptorTable::uptr m_samplerTables[Shader::Stage::NUM_STAGE];

			std::vector<ResourceBarrier> m_barriers;
			std::vector<ClearDSV> m_clearDSVs;
			std::vector<ClearRTV> m_clearRTVs;
			std::vector<ClearUAVUint> m_clearUAVsUint;
			std::vector<ClearUAVFloat> m_clearUAVsFloat;

			std::vector<uint32_t> m_graphicsSpaceToParamIndexMap[Shader::Stage::NUM_GRAPHICS][CbvSrvUavTypes];
			std::vector<uint32_t> m_computeSpaceToParamIndexMap[CbvSrvUavTypes];

			Blob m_shaders[NUM_SHADER];
		};
	}
}
