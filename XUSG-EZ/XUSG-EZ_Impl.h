//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSG-EZ.h"
#include "Core/XUSGCommand_DX12.h"
#ifndef XUSG_NO_VULKAN
#include "Core/XUSGCommand_VK.h"
#endif

namespace XUSG
{
	namespace EZ
	{
		template<typename Base>
		class CommandList_Impl :
			public virtual CommandList,
			public virtual Base
		{
		public:
			CommandList_Impl();
			CommandList_Impl(XUSG::CommandList* pCommandList,
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

			bool Create(XUSG::CommandList* pCommandList,
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
			bool Close(RenderTarget* pBackBuffer = nullptr);
			bool Reset(CommandAllocator* pAllocator, const Pipeline& initialState);

			void ClearState(const Pipeline& initialState) const { Base::ClearState(initialState); }
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
			void DrawIndirect(CommandLayout* pCommandlayout,
				uint32_t maxCommandCount,
				Resource* pArgumentBuffer,
				uint64_t argumentBufferOffset = 0,
				Resource* pCountBuffer = nullptr,
				uint64_t countBufferOffset = 0);
			void Dispatch(
				uint32_t threadGroupCountX,
				uint32_t threadGroupCountY,
				uint32_t threadGroupCountZ);
			void DispatchIndirect(CommandLayout* pCommandlayout,
				uint32_t maxCommandCount,
				Resource* pArgumentBuffer,
				uint64_t argumentBufferOffset = 0,
				Resource* pCountBuffer = nullptr,
				uint64_t countBufferOffset = 0);
			void CopyBufferRegion(Resource* pDstBuffer, uint64_t dstOffset,
				Resource* pSrcBuffer, uint64_t srcOffset, uint64_t numBytes);
			void CopyTextureRegion(TextureCopyLocation& dst, uint32_t dstX, uint32_t dstY, uint32_t dstZ,
				TextureCopyLocation& src, const BoxRange* pSrcBox = nullptr);
			void CopyResource(Resource* pDstResource, Resource* pSrcResource);
			void CopyTiles(Resource* pTiledResource, const TiledResourceCoord* pTileRegionStartCoord,
				const TileRegionSize& tileRegionSize, Resource* pBuffer, uint64_t bufferStartOffsetInBytes,
				TileCopyFlag flags = TileCopyFlag::NONE);
			void ResolveSubresource(Resource* pDstResource, uint32_t dstSubresource,
				Resource* pSrcResource, uint32_t srcSubresource, Format format);
			void IASetInputLayout(const InputLayout* pLayout);
			void IASetIndexBufferStripCutValue(IBStripCutValue ibStripCutValue);
			void RSSetState(const Graphics::Rasterizer* pRasterizer);
			void RSSetState(Graphics::RasterizerPreset preset);
			void RSSetViewports(uint32_t numViewports, const Viewport* pViewports) const
			{
				Base::RSSetViewports(numViewports, pViewports);
			}

			void RSSetScissorRects(uint32_t numRects, const RectRange* pRects) const
			{
				Base::RSSetScissorRects(numRects, pRects);
			}

			void OMSetBlendState(const Graphics::Blend* pBlend, uint32_t sampleMask = UINT_MAX);
			void OMSetBlendState(Graphics::BlendPreset preset, uint8_t numColorRTs = 1, uint32_t sampleMask = UINT_MAX);
			void OMSetBlendFactor(const float blendFactor[4]) const { Base::OMSetBlendFactor(blendFactor); }
			void OMSetStencilRef(uint32_t stencilRef) const { Base::OMSetStencilRef(stencilRef); }
			void OMSetSample(uint8_t count, uint8_t quality = 0);
			void DSSetState(const Graphics::DepthStencil* pDepthStencil);
			void DSSetState(Graphics::DepthStencilPreset preset);
			void SetGraphicsShader(Shader::Stage stage, const Blob& shader);
			void SetGraphicsNodeMask(uint32_t nodeMask);
			void SetComputeShader(const Blob& shader);
			void SetComputeNodeMask(uint32_t nodeMask);
			void SetGraphicsPipelineState(const Pipeline& pipelineState, const Graphics::State* pState = nullptr);
			void SetComputePipelineState(const Pipeline& pipelineState, const Compute::State* pState = nullptr);
			void ExecuteBundle(XUSG::CommandList* pCommandList) const { Base::ExecuteBundle(pCommandList); }
			void SetSamplers(Shader::Stage stage, uint32_t startBinding, uint32_t numSamplers, const Sampler* const* ppSamplers);
			void SetSamplerStates(Shader::Stage stage, uint32_t startBinding, uint32_t numSamplers, const SamplerPreset* pSamplerPresets);
			void SetResources(Shader::Stage stage, DescriptorType descriptorType, uint32_t startBinding,
				uint32_t numResources, const ResourceView* pResourceViews, uint32_t space = 0);
			void SetGraphicsDescriptorTable(Shader::Stage stage, DescriptorType descriptorType, const DescriptorTable& descriptorTable, uint32_t space);
			void SetComputeDescriptorTable(DescriptorType descriptorType, const DescriptorTable& descriptorTable, uint32_t space);
			void SetGraphics32BitConstant(Shader::Stage stage, uint32_t srcData, uint32_t destOffsetIn32BitValues = 0);
			void SetCompute32BitConstant(uint32_t srcData, uint32_t destOffsetIn32BitValues = 0);
			void SetGraphics32BitConstants(Shader::Stage stage, uint32_t num32BitValuesToSet, const void* pSrcData, uint32_t destOffsetIn32BitValues = 0);
			void SetCompute32BitConstants(uint32_t num32BitValuesToSet, const void* pSrcData, uint32_t destOffsetIn32BitValues = 0);
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
			void DiscardResource(Resource* pResource, uint32_t numRects, const RectRange* pRects,
				uint32_t firstSubresource, uint32_t numSubresources)
			{
				Base::DiscardResource(pResource, numRects,
					pRects, firstSubresource, numSubresources);
			}

			void BeginQuery(const QueryHeap& queryHeap, QueryType type, uint32_t index) const
			{
				Base::BeginQuery(queryHeap, type, index);
			}

			void EndQuery(const QueryHeap& queryHeap, QueryType type, uint32_t index) const
			{
				Base::EndQuery(queryHeap, type, index);
			}

			void ResolveQueryData(const QueryHeap& queryHeap, QueryType type, uint32_t startIndex,
				uint32_t numQueries, Resource* pDstBuffer, uint64_t alignedDstBufferOffset) const
			{
				Base::ResolveQueryData(queryHeap, type, startIndex,
					numQueries, pDstBuffer, alignedDstBufferOffset);
			}

			void SetPredication(Resource* pBuffer, uint64_t alignedBufferOffset, bool opEqualZero) const
			{
				Base::SetPredication(pBuffer, alignedBufferOffset, opEqualZero);
			}

			void SetMarker(uint32_t metaData, const void* pData, uint32_t size) const
			{
				Base::SetMarker(metaData, pData, size);
			}

			void BeginEvent(uint32_t metaData, const void* pData, uint32_t size) const
			{
				Base::BeginEvent(metaData, pData, size);
			}

			void EndEvent() { Base::EndEvent(); }

			// For resize window
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

			DescriptorTableLib* GetDescriptorTableLib() const;

			const XUSG::PipelineLayout& GetGraphicsPipelineLayout() const;
			const XUSG::PipelineLayout& GetComputePipelineLayout() const;

			uint32_t GetGraphicsConstantParamIndex(Shader::Stage stage) const;
			uint32_t GetComputeConstantParamIndex() const;

			void* GetNativeHandle() { return Base::GetNativeHandle(); }
			void* GetDeviceNativeHandle() const { return Base::GetDeviceNativeHandle(); }
			void* GetBackend() { return Base::GetBackend(); }

			Device* GetDevice() const { return Base::GetDevice(); }
			XUSG::CommandList* AsCommandList();

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
				Descriptor DepthStencilView;
				ClearFlag ClearFlags;
				float Depth;
				uint8_t Stencil;
				std::vector<RectRange> Rects;
			};

			struct ClearRTV
			{
				Descriptor RenderTargetView;
				float ColorRGBA[4];
				std::vector<RectRange> Rects;
			};

			struct ClearUAVUint
			{
				const ResourceView* pUAV;
				DescriptorTable UAVTable;
				uint32_t Values[4];
				std::vector<RectRange> Rects;
			};

			struct ClearUAVFloat
			{
				const ResourceView* pUAV;
				DescriptorTable UAVTable;
				float Values[4];
				std::vector<RectRange> Rects;
			};

			struct SetDescriptorTable
			{
				Shader::Stage Stage;
				DescriptorType Type;
				DescriptorTable Table;
				uint32_t Space;
			};

			struct Set32BitConstant
			{
				Shader::Stage Stage;
				uint32_t SrcData;
				uint32_t Offset;
			};

			struct Set32BitConstants
			{
				Shader::Stage Stage;
				uint32_t Num32BitValues;
				const void* pSrcData;
				uint32_t Offset;
			};

			static const uint8_t CbvSrvUavTypes = 3;

			virtual bool adopt(XUSG::CommandList* pCommandList) = 0;

			bool init(XUSG::CommandList* pCommandList, uint32_t samplerHeapSize, uint32_t cbvSrvUavHeapSize);
			bool createGraphicsPipelineLayouts(
				const uint32_t maxSamplers[Shader::Stage::NUM_GRAPHICS],
				const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_GRAPHICS],
				const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_GRAPHICS],
				const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_GRAPHICS],
				const uint32_t maxCbvSpaces[Shader::Stage::NUM_GRAPHICS],
				const uint32_t maxSrvSpaces[Shader::Stage::NUM_GRAPHICS],
				const uint32_t maxUavSpaces[Shader::Stage::NUM_GRAPHICS],
				const uint32_t max32BitConstants[Shader::Stage::NUM_GRAPHICS],
				const uint32_t constantSlots[Shader::Stage::NUM_GRAPHICS],
				const uint32_t constantSpaces[Shader::Stage::NUM_GRAPHICS],
				uint32_t slotExt, uint32_t spaceExt);
			bool createComputePipelineLayouts(uint32_t maxSamplers, const uint32_t* pMaxCbvsEachSpace,
				const uint32_t* pMaxSrvsEachSpace, const uint32_t* pMaxUavsEachSpace,
				uint32_t maxCbvSpaces, uint32_t maxSrvSpaces, uint32_t maxUavSpaces,
				uint32_t max32BitConstants, uint32_t constantSlot, uint32_t constantSpace,
				uint32_t slotExt, uint32_t spaceExt);
			bool createShaders();
			bool createBlitPipelineLayout();
			void recordSpaceStarts(
				const uint32_t cbvStartSpaces[Shader::Stage::NUM_STAGE],
				const uint32_t maxCbvSpaces[Shader::Stage::NUM_STAGE],
				const uint32_t srvStartSpaces[Shader::Stage::NUM_STAGE],
				const uint32_t maxSrvSpaces[Shader::Stage::NUM_STAGE],
				const uint32_t uavStartSpaces[Shader::Stage::NUM_STAGE],
				const uint32_t maxUavSpaces[Shader::Stage::NUM_STAGE],
				const uint32_t samplerStartSpaces[Shader::Stage::NUM_STAGE]);
			uint32_t spaceStart(Shader::Stage stage, DescriptorType type) const;
			void flushClears(const Resource* pResource);
			void dispatchBlit(const Blob& shader, SamplerPreset sampler, const ResourceView& srv,
				const ResourceView& uav, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ);

			void predraw();
			void predispatch();
			void preexecuteIndirect(Resource* pArgumentBuffer, Resource* pCountBuffer);
			void clearDSVs();
			void clearRTVs();
			void clearUAVs();
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
			XUSG::PipelineLayout m_blitPipelineLayout;
			XUSG::Pipeline m_pipeline;

			Graphics::State::uptr m_graphicsState;
			Compute::State::uptr m_computeState;

			bool m_isGraphicsDirty;
			bool m_isComputeDirty;

			std::vector<Descriptor> m_descriptors;
			std::vector<Util::DescriptorTable::uptr> m_cbvSrvUavTables[Shader::Stage::NUM_STAGE][CbvSrvUavTypes];
			Util::DescriptorTable::uptr m_samplerTables[Shader::Stage::NUM_STAGE];

			std::vector<ResourceBarrier> m_barriers;
			std::vector<ClearDSV> m_clearDSVs;
			std::vector<ClearRTV> m_clearRTVs;
			std::vector<ClearUAVUint> m_clearUAVsUint;
			std::vector<ClearUAVFloat> m_clearUAVsFloat;
			std::vector<ResourceView> m_clearUAVs;

			std::vector<uint32_t> m_graphicsSpaceToParamIndexMap[Shader::Stage::NUM_GRAPHICS][CbvSrvUavTypes];
			std::vector<uint32_t> m_computeSpaceToParamIndexMap[CbvSrvUavTypes];
			uint32_t m_spaceStarts[Shader::Stage::NUM_STAGE][static_cast<uint32_t>(DescriptorType::SAMPLER) + 1];

			std::vector<SetDescriptorTable> m_graphicsSetDescriptorTables;
			std::vector<Set32BitConstant> m_graphicsSetSingle32BitConstants;
			std::vector<Set32BitConstants> m_graphicsSet32BitConstants;

			uint32_t m_graphicsConstantParamIndices[Shader::Stage::NUM_GRAPHICS];
			uint32_t m_computeConstantParamIndex;

			Blob m_shaders[NUM_SHADER];
		};

		extern template class CommandList_Impl<XUSG::CommandList_DX12>;
#ifndef XUSG_NO_VULKAN
		extern template class CommandList_Impl<XUSG::CommandList_VK>;
#endif

		class CommandList_DX12 :
			public virtual CommandList_Impl<XUSG::CommandList_DX12>
		{
		public:
			using CommandList_Impl<XUSG::CommandList_DX12>::CommandList_Impl;

		protected:
			bool adopt(XUSG::CommandList* pCommandList) override;
		};

#ifndef XUSG_NO_VULKAN
		class CommandList_VK :
			public virtual CommandList_Impl<XUSG::CommandList_VK>
		{
		public:
			using CommandList_Impl<XUSG::CommandList_VK>::CommandList_Impl;

		protected:
			bool adopt(XUSG::CommandList* pCommandList) override;
		};
#endif
	}
}
