//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSG.h"

namespace XUSG
{
	class CommandAllocator_DX12 :
		public virtual CommandAllocator
	{
	public:
		CommandAllocator_DX12();
		virtual ~CommandAllocator_DX12();

		bool Create(const Device* pDevice, CommandListType type, const wchar_t* name = nullptr);
		bool Reset();

		void* GetHandle() const;

	protected:
		com_ptr<ID3D12CommandAllocator> m_commandAllocator;
	};

	class CommandList_DX12 :
		public virtual CommandList
	{
	public:
		CommandList_DX12();
		~CommandList_DX12();

		bool Create(const Device* pDevice, uint32_t nodeMask, CommandListType type,
			const CommandAllocator* pAllocator, const Pipeline& pipeline,
			const wchar_t* name = nullptr);
		bool Close() const;
		bool Reset(const CommandAllocator* pAllocator,
			const Pipeline& initialState) const;

		void ClearState(const Pipeline& initialState) const;
		void Draw(
			uint32_t vertexCountPerInstance,
			uint32_t instanceCount,
			uint32_t startVertexLocation,
			uint32_t startInstanceLocation) const;
		void DrawIndexed(
			uint32_t indexCountPerInstance,
			uint32_t instanceCount,
			uint32_t startIndexLocation,
			int32_t baseVertexLocation,
			uint32_t startInstanceLocation) const;
		void Dispatch(
			uint32_t threadGroupCountX,
			uint32_t threadGroupCountY,
			uint32_t threadGroupCountZ) const;
		void CopyBufferRegion(const Resource* pDstBuffer, uint64_t dstOffset,
			const Resource* pSrcBuffer, uint64_t srcOffset, uint64_t numBytes) const;
		void CopyTextureRegion(const TextureCopyLocation& dst,
			uint32_t dstX, uint32_t dstY, uint32_t dstZ,
			const TextureCopyLocation& src, const BoxRange* pSrcBox = nullptr) const;
		void CopyResource(const Resource* pDstResource, const Resource* pSrcResource) const;
		void CopyTiles(const Resource* pTiledResource, const TiledResourceCoord* pTileRegionStartCoord,
			const TileRegionSize* pTileRegionSize, const Resource* pBuffer, uint64_t bufferStartOffsetInBytes,
			TileCopyFlag flags) const;
		void ResolveSubresource(const Resource* pDstResource, uint32_t dstSubresource,
			const Resource* pSrcResource, uint32_t srcSubresource, Format format) const;
		void IASetPrimitiveTopology(PrimitiveTopology primitiveTopology) const;
		void RSSetViewports(uint32_t numViewports, const Viewport* pViewports) const;
		void RSSetScissorRects(uint32_t numRects, const RectRange* pRects) const;
		void OMSetBlendFactor(const float blendFactor[4]) const;
		void OMSetStencilRef(uint32_t stencilRef) const;
		void SetPipelineState(const Pipeline& pipelineState) const;
		void Barrier(uint32_t numBarriers, const ResourceBarrier* pBarriers) const;
		void ExecuteBundle(CommandList& commandList) const;
		void SetDescriptorPools(uint32_t numDescriptorPools, const DescriptorPool* pDescriptorPools) const;
		void SetComputePipelineLayout(const PipelineLayout& pipelineLayout) const;
		void SetGraphicsPipelineLayout(const PipelineLayout& pipelineLayout) const;
		void SetComputeDescriptorTable(uint32_t index, const DescriptorTable& descriptorTable) const;
		void SetGraphicsDescriptorTable(uint32_t index, const DescriptorTable& descriptorTable) const;
		void SetCompute32BitConstant(uint32_t index, uint32_t srcData, uint32_t destOffsetIn32BitValues = 0) const;
		void SetGraphics32BitConstant(uint32_t index, uint32_t srcData, uint32_t destOffsetIn32BitValues = 0) const;
		void SetCompute32BitConstants(uint32_t index, uint32_t num32BitValuesToSet,
			const void* pSrcData, uint32_t destOffsetIn32BitValues = 0) const;
		void SetGraphics32BitConstants(uint32_t index, uint32_t num32BitValuesToSet,
			const void* pSrcData, uint32_t destOffsetIn32BitValues = 0) const;
		void SetComputeRootConstantBufferView(uint32_t index, const Resource* pResource, int offset = 0) const;
		void SetGraphicsRootConstantBufferView(uint32_t index, const Resource* pResource, int offset = 0) const;
		void SetComputeRootShaderResourceView(uint32_t index, const Resource* pResource, int offset = 0) const;
		void SetGraphicsRootShaderResourceView(uint32_t index, const Resource* pResource, int offset = 0) const;
		void SetComputeRootUnorderedAccessView(uint32_t index, const Resource* pResource, int offset = 0) const;
		void SetGraphicsRootUnorderedAccessView(uint32_t index, const Resource* pResource, int offset = 0) const;
		void IASetIndexBuffer(const IndexBufferView& view) const;
		void IASetVertexBuffers(uint32_t startSlot, uint32_t numViews, const VertexBufferView* pViews) const;
		void SOSetTargets(uint32_t startSlot, uint32_t numViews, const StreamOutBufferView* pViews) const;
		void OMSetFramebuffer(const Framebuffer& framebuffer) const;
		void OMSetRenderTargets(
			uint32_t numRenderTargetDescriptors,
			const Descriptor* pRenderTargetViews,
			const Descriptor* pDepthStencilView = nullptr,
			bool rtsSingleHandleToDescriptorRange = false) const;
		void ClearDepthStencilView(const Framebuffer& framebuffer, ClearFlag clearFlags,
			float depth, uint8_t stencil = 0, uint32_t numRects = 0, const RectRange * pRects = nullptr) const;
		void ClearDepthStencilView(const Descriptor& depthStencilView, ClearFlag clearFlags,
			float depth, uint8_t stencil = 0, uint32_t numRects = 0, const RectRange* pRects = nullptr) const;
		void ClearRenderTargetView(const Descriptor& renderTargetView, const float colorRGBA[4],
			uint32_t numRects = 0, const RectRange* pRects = nullptr) const;
		void ClearUnorderedAccessViewUint(const DescriptorTable& descriptorTable,
			const Descriptor& descriptor, const Resource* pResource, const uint32_t values[4],
			uint32_t numRects = 0, const RectRange* pRects = nullptr) const;
		void ClearUnorderedAccessViewFloat(const DescriptorTable& descriptorTable,
			const Descriptor& descriptor, const Resource* pResource, const float values[4],
			uint32_t numRects = 0, const RectRange* pRects = nullptr) const;
		void DiscardResource(const Resource* pResource, uint32_t numRects, const RectRange* pRects,
			uint32_t firstSubresource, uint32_t numSubresources) const;
		void BeginQuery(const QueryPool& queryPool, QueryType type, uint32_t index) const;
		void EndQuery(const QueryPool& queryPool, QueryType type, uint32_t index) const;
		void ResolveQueryData(const QueryPool& queryPool, QueryType type, uint32_t startIndex,
			uint32_t numQueries, const Resource* pDstBuffer, uint64_t alignedDstBufferOffset) const;
		void SetPredication(const Resource* pBuffer, uint64_t alignedBufferOffset, bool opEqualZero)const;
		void SetMarker(uint32_t metaData, const void* pData, uint32_t size) const;
		void BeginEvent(uint32_t metaData, const void* pData, uint32_t size) const;
		void EndEvent();
		void ExecuteIndirect(const CommandLayout& commandlayout, uint32_t maxCommandCount,
			const Resource* pArgumentBuffer, uint64_t argumentBufferOffset = 0,
			const Resource* pCountBuffer = nullptr, uint64_t countBufferOffset = 0);

		com_ptr<ID3D12GraphicsCommandList>& GetGraphicsCommandList();

	protected:
		com_ptr<ID3D12GraphicsCommandList> m_commandList;
	};

	class CommandQueue_DX12 :
		public virtual CommandQueue
	{
	public:
		CommandQueue_DX12();
		virtual ~CommandQueue_DX12();

		bool Create(const Device* pDevice, CommandListType type, CommandQueueFlag flags,
			int32_t priority = 0, uint32_t nodeMask = 0, const wchar_t* name = nullptr);

		bool SubmitCommandLists(uint32_t numCommandLists, CommandList* const* ppCommandLists,
			const Semaphore* pWaits = nullptr, uint32_t numWaits = 0,
			const Semaphore* pSignals = nullptr, uint32_t numSignals = 0);
		bool SubmitCommandList(CommandList* const pCommandList,
			const Semaphore* pWaits = nullptr, uint32_t numWaits = 0,
			const Semaphore* pSignals = nullptr, uint32_t numSignals = 0);
		bool Wait(const Fence* pFence, uint64_t value);
		bool Signal(const Fence* pFence, uint64_t value);

		void ExecuteCommandLists(uint32_t numCommandLists, CommandList* const* ppCommandLists);
		void ExecuteCommandList(CommandList* const pCommandList);

		void* GetHandle() const;

		com_ptr<ID3D12CommandQueue>& GetCommandQueue();

	protected:

		com_ptr<ID3D12CommandQueue> m_commandQueue;
	};
}
