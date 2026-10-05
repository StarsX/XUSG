//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSG_VK.h"

namespace XUSG
{
	class Resource_VK;
	struct ResourceView_VK;

	class CommandAllocator_VK :
		public virtual CommandAllocator
	{
	public:
		CommandAllocator_VK();
		virtual ~CommandAllocator_VK();

		bool Create(const Device* pDevice, CommandListType type, const wchar_t* name = nullptr);
		bool Reset();

		void Create(void* pHandle, const wchar_t* name = nullptr);

		void* GetHandle() const;
		void* GetDeviceHandle() const;

		const Device* GetDevice() const;

		VkCommandPool GetPool() const;
		uint32_t GetFamily() const;

	protected:
		VkDevice		m_device;
		VkCommandPool	m_pool;
		uint32_t		m_family;

		const Device* m_pDevice;
	};

	class CommandList_VK :
		public virtual CommandList
	{
	public:
		CommandList_VK();
		virtual ~CommandList_VK();

		bool Create(const Device* pDevice, uint32_t nodeMask, CommandListType type,
			const CommandAllocator* pAllocator, const Pipeline& pipeline,
			const wchar_t* name = nullptr);
		bool Close() const;
		bool Reset(const CommandAllocator* pAllocator, const Pipeline& initialState) const;

		void ClearState(const Pipeline& initialState) const;
		void Draw(uint32_t vertexCountPerInstance, uint32_t instanceCount,
			uint32_t startVertexLocation, uint32_t startInstanceLocation) const;
		void DrawIndexed(uint32_t indexCountPerInstance, uint32_t instanceCount,
			uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation) const;
		void Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) const;
		void CopyBufferRegion(const Resource* pDstBuffer, uint64_t dstOffset,
			const Resource* pSrcBuffer, uint64_t srcOffset, uint64_t numBytes) const;
		void CopyTextureRegion(const TextureCopyLocation& dst, uint32_t dstX, uint32_t dstY, uint32_t dstZ,
			const TextureCopyLocation& src, const BoxRange* pSrcBox = nullptr) const;
		void CopyResource(const Resource* pDstResource, const Resource* pSrcResource) const;
		void CopyTiles(const Resource* pTiledResource, const TiledResourceCoord* pTileRegionStartCoord,
			const TileRegionSize& tileRegionSize, const Resource* pBuffer, uint64_t bufferStartOffsetInBytes,
			TileCopyFlag flags = TileCopyFlag::NONE) const;
		void ResolveSubresource(const Resource* pDstResource, uint32_t dstSubresource,
			const Resource* pSrcResource, uint32_t srcSubresource, Format format) const;
		void IASetPrimitiveTopology(PrimitiveTopology primitiveTopology) const;
		void RSSetViewports(uint32_t numViewports, const Viewport* pViewports) const;
		void RSSetScissorRects(uint32_t numRects, const RectRange* pRects) const;
		void OMSetBlendFactor(const float blendFactor[4]) const;
		void OMSetStencilRef(uint32_t stencilRef) const;
		void SetPipelineState(const Pipeline& pipelineState) const;
		void Barrier(uint32_t numBarriers, const ResourceBarrier* pBarriers);
		void ExecuteBundle(const CommandList* pCommandList) const;
		void SetDescriptorHeaps(uint32_t numDescriptorHeaps, const DescriptorHeap* pDescriptorHeaps);
		void SetComputePipelineLayout(const PipelineLayout& pipelineLayout) const;
		void SetGraphicsPipelineLayout(const PipelineLayout& pipelineLayout) const;
		void SetComputeDescriptorTable(uint32_t index, const DescriptorTable& descriptorTable) const;
		void SetGraphicsDescriptorTable(uint32_t index, const DescriptorTable& descriptorTable) const;
		void SetComputeDescriptorTable(uint32_t index, const DescriptorHeap& descriptorHeap, int32_t offset) const;
		void SetGraphicsDescriptorTable(uint32_t index, const DescriptorHeap& descriptorHeap, int32_t offset) const;
		void SetCompute32BitConstant(uint32_t index, uint32_t srcData, uint32_t destOffsetIn32BitValues = 0) const;
		void SetGraphics32BitConstant(uint32_t index, uint32_t srcData, uint32_t destOffsetIn32BitValues = 0) const;
		void SetCompute32BitConstants(uint32_t index, uint32_t num32BitValuesToSet,
			const void* pSrcData, uint32_t destOffsetIn32BitValues = 0) const;
		void SetGraphics32BitConstants(uint32_t index, uint32_t num32BitValuesToSet,
			const void* pSrcData, uint32_t destOffsetIn32BitValues = 0) const;
		void SetComputeRootConstantBufferView(uint32_t index, const Resource* pResource, int32_t offset = 0) const;
		void SetGraphicsRootConstantBufferView(uint32_t index, const Resource* pResource, int32_t offset = 0) const;
		void SetComputeRootShaderResourceView(uint32_t index, const Resource* pResource, int32_t offset = 0) const;
		void SetGraphicsRootShaderResourceView(uint32_t index, const Resource* pResource, int32_t offset = 0) const;
		void SetComputeRootUnorderedAccessView(uint32_t index, const Resource* pResource, int32_t offset = 0) const;
		void SetGraphicsRootUnorderedAccessView(uint32_t index, const Resource* pResource, int32_t offset = 0) const;
		void SetComputeRootConstantBufferView(uint32_t index, uint64_t address) const;
		void SetGraphicsRootConstantBufferView(uint32_t index, uint64_t address) const;
		void SetComputeRootShaderResourceView(uint32_t index, uint64_t address) const;
		void SetGraphicsRootShaderResourceView(uint32_t index, uint64_t address) const;
		void SetComputeRootUnorderedAccessView(uint32_t index, uint64_t address) const;
		void SetGraphicsRootUnorderedAccessView(uint32_t index, uint64_t address) const;
		void IASetIndexBuffer(const IndexBufferView& view) const;
		void IASetVertexBuffers(uint32_t startSlot, uint32_t numViews, const VertexBufferView* pViews) const;
		void SOSetTargets(uint32_t startSlot, uint32_t numViews, const StreamOutBufferView* pViews) const;
		void OMSetRenderTargets(uint32_t numRenderTargetDescriptors, const Descriptor* pRenderTargetViews,
			const Descriptor* pDepthStencilView) const;
		void ClearDepthStencilView(ClearFlag clearFlags, float depth, uint8_t stencil = 0,
			uint32_t numRects = 0, const RectRange* pRects = nullptr);
		void ClearDepthStencilView(const Descriptor& depthStencilView, ClearFlag clearFlags, float depth,
			uint8_t stencil = 0, uint32_t numRects = 0, const RectRange* pRects = nullptr);
		void ClearRenderTargetView(const float colorRGBA[4], uint8_t renderTargetViewIndex,
			uint32_t numRects = 0, const RectRange* pRects = nullptr);
		void ClearRenderTargetView(const Descriptor& renderTargetView, const float colorRGBA[4],
			uint32_t numRects = 0, const RectRange* pRects = nullptr);
		void ClearUnorderedAccessViewUint(const DescriptorTable& descriptorTable, const Descriptor& descriptor,
			const uint32_t values[4], uint32_t numRects, const RectRange* pRects);
		void ClearUnorderedAccessViewFloat(const DescriptorTable& descriptorTable, const Descriptor& descriptor,
			const float values[4], uint32_t numRects, const RectRange* pRects);
		void DiscardResource(Resource* pResource, uint32_t numRects, const RectRange* pRects,
			uint32_t firstSubresource, uint32_t numSubresources);
		void BeginQuery(const QueryHeap& queryHeap, QueryType type, uint32_t index) const;
		void EndQuery(const QueryHeap& queryHeap, QueryType type, uint32_t index) const;
		void ResolveQueryData(const QueryHeap& queryHeap, QueryType type, uint32_t startIndex,
			uint32_t numQueries, const Resource* pDstBuffer, uint64_t alignedDstBufferOffset) const;
		void SetPredication(const Resource* pBuffer, uint64_t alignedBufferOffset, bool opEqualZero) const;
		void SetMarker(uint32_t metaData, const void* pData, uint32_t size) const;
		void BeginEvent(uint32_t metaData, const void* pData, uint32_t size) const;
		void EndEvent();
		void ExecuteIndirect(const CommandLayout* pCommandlayout, uint32_t maxCommandCount,
			const Resource* pArgumentBuffer, uint64_t argumentBufferOffset,
			const Resource* pCountBuffer = nullptr, uint64_t countBufferOffset = 0);

		void Create(void* pHandle, const wchar_t* name = nullptr);
		void Attach(const CommandList_VK& source);

		void* GetHandle() const;
		void* GetDeviceHandle() const;

		const Device* GetDevice() const;
		VkCommandBuffer GetCommandBuffer() const;

		void CopyBufferToImage(const Resource* pDst, uint32_t dstSubresource, const Resource* pSrc,
			uint64_t srcOffset, uint32_t bufferRowLength, uint32_t bufferImageHeight) const;
		void CopyImageToBuffer(const Resource* pSrc, uint32_t mipLevel, uint32_t arrayLayer, const Resource* pDst,
			uint64_t dstOffset, uint32_t bufferRowLength, uint32_t bufferImageHeight, VkImageAspectFlags aspect) const;

	protected:
		struct RenderAttachment
		{
			VkImageView View;
			VkFormat Format;
			VkImageLayout Layout;
			Resource_VK* Resource;
			uint32_t BaseMip;
			uint32_t MipCount;
			uint32_t BaseLayer;
			uint32_t Width;
			uint32_t Height;
			uint32_t Layers;
		};

		void resetState() const;
		void endPredication() const;
		void beginPredication() const;
		void endRendering() const;
		void beginRendering() const;
		virtual const void* renderingPNext() const;
		void beginDraw() const;
		void bindHeap(DescriptorHeap heap) const;
		void pushData(uint32_t offset, const void* pData, uint32_t size) const;
		void setDescriptorTable(const PipelineLayoutObject* pLayout, uint32_t index, uint32_t descriptorIndex) const;
		void setConstants(const PipelineLayoutObject* pLayout, uint32_t index, uint32_t num32BitValues,
			const void* pData, uint32_t destOffsetIn32BitValues) const;
		void setRootAddress(const PipelineLayoutObject* pLayout, uint32_t index, uint64_t address) const;
		bool ensurePreprocess(VkDeviceSize size) const;
		void clearColor(uint32_t attachment, const float color[4], uint32_t numRects, const RectRange* pRects) const;
		void clearDepth(const RenderAttachment& attachment, ClearFlag clearFlags, float depth, uint8_t stencil,
			uint32_t numRects, const RectRange* pRects) const;
		void clearUnorderedAccessView(const Descriptor& descriptor, const VkClearColorValue& color, uint32_t numRects);
		void setAttachmentRange(RenderAttachment& attachment, const ResourceView_VK* pView, Resource_VK* pResource) const;

		struct ClearSubresource
		{
			uint32_t Subresource;
			ResourceState State;
		};

		void submitClearBarrier(Resource_VK* pResource, uint32_t regionCount, bool toCopy);

		VkDevice					m_device;
		VkCommandPool				m_pool;
		VkCommandBuffer				m_commandBuffer;
		bool						m_ownsCommandBuffer;
		const Device*				m_pDevice;
		const Device_VK*			m_pDeviceVK;
		CommandListType				m_type;

		PFN_vkCmdBeginRendering			m_beginRendering;
		PFN_vkCmdEndRendering			m_endRendering;
		PFN_vkCmdSetPrimitiveTopology	m_setPrimitiveTopology;
		PFN_vkCmdBindVertexBuffers2		m_bindVertexBuffers2;
		PFN_vkCmdBeginDebugUtilsLabelEXT	m_beginLabel;
		PFN_vkCmdEndDebugUtilsLabelEXT		m_endLabel;
		PFN_vkCmdInsertDebugUtilsLabelEXT	m_insertLabel;

		mutable VkBuffer			m_predicateBuffer;
		mutable VkDeviceSize		m_predicateOffset;
		mutable bool				m_predicateEqualZero;
		mutable bool				m_predicationActive;
		mutable bool				m_renderingBegun;
		mutable bool				m_hasDepth;
		mutable uint32_t			m_colorCount;
		mutable RenderAttachment	m_colors[8];
		mutable RenderAttachment	m_depth;
		mutable uint32_t			m_renderWidth;
		mutable uint32_t			m_renderHeight;

		mutable const PipelineLayoutObject* m_graphicsLayout;
		mutable const PipelineLayoutObject* m_computeLayout;
		mutable const Pipeline_VK*	m_pipeline;
		mutable DescriptorHeap		m_resourceHeap;
		mutable DescriptorHeap		m_samplerHeap;

		mutable VkBuffer			m_preprocessBuffer;
		mutable VkDeviceMemory		m_preprocessMemory;
		mutable VkDeviceAddress	m_preprocessAddress;
		mutable VkDeviceSize		m_preprocessSize;

		mutable std::vector<VkClearRect>				m_clearRects;
		mutable std::vector<VkImageCopy>				m_imageCopies;
		mutable std::vector<VkMemoryBarrier2>			m_globals;
		mutable std::vector<VkBufferMemoryBarrier2>		m_buffers;
		mutable std::vector<VkImageMemoryBarrier2>		m_images;
		mutable std::vector<ClearSubresource>			m_clearRegions;
	};

	class CommandQueue_VK :
		public virtual CommandQueue
	{
	public:
		CommandQueue_VK();
		virtual ~CommandQueue_VK();

		bool Create(const Device* pDevice, CommandListType type, CommandQueueFlag flags,
			int32_t priority = 0, uint32_t nodeMask = 0, const wchar_t* name = nullptr);

		bool SubmitCommandLists(uint32_t numCommandLists, const CommandList* const* ppCommandLists,
			const Semaphore* pWaits = nullptr, uint32_t numWaits = 0,
			const Semaphore* pSignals = nullptr, uint32_t numSignals = 0);
		bool SubmitCommandList(const CommandList* pCommandList,
			const Semaphore* pWaits = nullptr, uint32_t numWaits = 0,
			const Semaphore* pSignals = nullptr, uint32_t numSignals = 0);
		bool Wait(const Fence* pFence, uint64_t value);
		bool Signal(const Fence* pFence, uint64_t value);

		void ExecuteCommandLists(uint32_t numCommandLists, const CommandList* const* ppCommandLists);
		void ExecuteCommandList(const CommandList* pCommandList);
		void UpdateTileMappings(const Resource* pResource, uint32_t numResourceRegions,
			const TiledResourceCoord* pResourceRegionStartCoords, const TileRegionSize* pResourceRegionSizes,
			const Heap* pHeap, uint32_t numHeapRanges, const TileRangeFlag* pHeapRangeFlags,
			const uint32_t* pHeapRangeStartOffsets, const uint32_t* pHeapRangeTileCounts,
			TileMappingFlag flags = TileMappingFlag::NONE);
		void CopyTileMappings(const Resource* pDstResource, const TiledResourceCoord* pDstRegionStartCoord,
			const Resource* pSrcResource, const TiledResourceCoord* pSrcRegionStartCoordinate,
			const TileRegionSize& regionSize, TileMappingFlag flags = TileMappingFlag::NONE) const;

		void Create(void* pHandle, const wchar_t* name = nullptr);

		void* GetHandle() const;
		void* GetDeviceHandle() const;

		const Device* GetDevice() const;

	protected:
		bool submit(uint32_t numCommandLists, const CommandList* const* ppCommandLists,
			const Semaphore* pWaits, uint32_t numWaits, const Semaphore* pSignals, uint32_t numSignals);

		VkDevice		m_device;
		VkQueue			m_queue;
		const Device*	m_pDevice;
		uint32_t		m_family;
		uint32_t		m_queueIndex;
		bool			m_registered;
		bool			m_ownsSlot;

		std::vector<VkCommandBufferSubmitInfo> m_commandInfos;
		std::vector<VkSemaphoreSubmitInfo> m_waitInfos;
		std::vector<VkSemaphoreSubmitInfo> m_signalInfos;
	};
}
