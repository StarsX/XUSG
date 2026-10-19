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

		bool Create(Device* pDevice, CommandListType type, const wchar_t* name = nullptr);
		bool Reset();

		void Create(uint64_t nativeHandle, Device* pDevice);

		uint64_t GetNativeHandle() const;
		void* GetDeviceNativeHandle() const;

		Device* GetDevice() const;

		VkCommandPool GetPool() const;
		uint32_t GetFamily() const;

	protected:
		Device_VK*		m_pDevice;
		VkCommandPool	m_pool;
		uint32_t		m_family;
	};

	class CommandList_VK :
		public virtual CommandList
	{
	public:
		CommandList_VK();
		virtual ~CommandList_VK();

		bool Create(Device* pDevice, uint32_t nodeMask, CommandListType type, uint8_t numAllocators,
			CommandAllocator* const* ppAllocators, const Pipeline& pipeline, const wchar_t* name = nullptr);
		bool Close() const;
		bool Reset(uint8_t allocatorIndex, const Pipeline& initialState) const;

		void ClearState(const Pipeline& initialState) const;
		void Draw(uint32_t vertexCountPerInstance, uint32_t instanceCount,
			uint32_t startVertexLocation, uint32_t startInstanceLocation) const;
		void DrawIndexed(uint32_t indexCountPerInstance, uint32_t instanceCount,
			uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation) const;
		void Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) const;
		void CopyBufferRegion(Resource* pDstBuffer, uint64_t dstOffset,
			Resource* pSrcBuffer, uint64_t srcOffset, uint64_t numBytes) const;
		void CopyTextureRegion(const TextureCopyLocation& dst, uint32_t dstX, uint32_t dstY, uint32_t dstZ,
			const TextureCopyLocation& src, const BoxRange* pSrcBox = nullptr) const;
		void CopyResource(Resource* pDstResource, Resource* pSrcResource) const;
		void CopyTiles(Resource* pTiledResource, const TiledResourceCoord* pTileRegionStartCoord,
			const TileRegionSize& tileRegionSize, Resource* pBuffer, uint64_t bufferStartOffsetInBytes,
			TileCopyFlag flags = TileCopyFlag::NONE) const;
		void ResolveSubresource(Resource* pDstResource, uint32_t dstSubresource,
			Resource* pSrcResource, uint32_t srcSubresource, Format format) const;
		void IASetPrimitiveTopology(PrimitiveTopology primitiveTopology) const;
		void RSSetViewports(uint32_t numViewports, const Viewport* pViewports) const;
		void RSSetScissorRects(uint32_t numRects, const RectRange* pRects) const;
		void OMSetBlendFactor(const float blendFactor[4]) const;
		void OMSetStencilRef(uint32_t stencilRef) const;
		void SetPipelineState(const Pipeline& pipelineState) const;
		void Barrier(uint32_t numBarriers, const ResourceBarrier* pBarriers) const;
		void ExecuteBundle(CommandList* pCommandList) const;
		void SetDescriptorHeaps(uint32_t numDescriptorHeaps, const DescriptorHeap* pDescriptorHeaps) const;
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
			uint32_t numRects = 0, const RectRange* pRects = nullptr) const;
		void ClearDepthStencilView(const Descriptor& depthStencilView, ClearFlag clearFlags, float depth,
			uint8_t stencil = 0, uint32_t numRects = 0, const RectRange* pRects = nullptr) const;
		void ClearRenderTargetView(const float colorRGBA[4], uint8_t renderTargetViewIndex,
			uint32_t numRects = 0, const RectRange* pRects = nullptr) const;
		void ClearRenderTargetView(const Descriptor& renderTargetView, const float colorRGBA[4],
			uint32_t numRects = 0, const RectRange* pRects = nullptr) const;
		void ClearUnorderedAccessViewUint(const DescriptorTable& descriptorTable, const Descriptor& descriptor,
			const uint32_t values[4], uint32_t numRects, const RectRange* pRects) const;
		void ClearUnorderedAccessViewFloat(const DescriptorTable& descriptorTable, const Descriptor& descriptor,
			const float values[4], uint32_t numRects, const RectRange* pRects) const;
		void DiscardResource(Resource* pResource, uint32_t numRects, const RectRange* pRects,
			uint32_t firstSubresource, uint32_t numSubresources) const;
		void BeginQuery(const QueryHeap& queryHeap, QueryType type, uint32_t index) const;
		void EndQuery(const QueryHeap& queryHeap, QueryType type, uint32_t index) const;
		void ResolveQueryData(const QueryHeap& queryHeap, QueryType type, uint32_t startIndex,
			uint32_t numQueries, Resource* pDstBuffer, uint64_t alignedDstBufferOffset) const;
		void SetPredication(Resource* pBuffer, uint64_t alignedBufferOffset, bool opEqualZero) const;
		void SetMarker(uint32_t metaData, const void* pData, uint32_t size) const;
		void BeginEvent(uint32_t metaData, const void* pData, uint32_t size) const;
		void EndEvent() const;
		void ExecuteIndirect(CommandLayout* pCommandlayout, uint32_t maxCommandCount,
			Resource* pArgumentBuffer, uint64_t argumentBufferOffset = 0,
			Resource* pCountBuffer = nullptr, uint64_t countBufferOffset = 0) const;

		void Create(void* pNativeHandle, Device* pDevice);
		void Attach(const CommandList_VK& source);

		void* GetNativeHandle();
		void* GetDeviceNativeHandle() const;
		void* GetBackend() override;

		Device* GetDevice() const;
		VkCommandBuffer GetCommandBuffer() const;

		void CopyBufferToImage(Resource* pDst, uint32_t dstSubresource, Resource* pSrc,
			uint64_t srcOffset, uint32_t bufferRowLength, uint32_t bufferImageHeight) const;
		void CopyImageToBuffer(Resource* pSrc, uint32_t mipLevel, uint32_t arrayLayer, Resource* pDst,
			uint64_t dstOffset, uint32_t bufferRowLength, uint32_t bufferImageHeight, VkImageAspectFlags aspect) const;

	protected:
		struct RenderAttachment
		{
			VkRenderingAttachmentInfo Rendering;
			Resource_VK* Resource;
			uint32_t Width;
			uint32_t Height;
			uint32_t BaseMip;
			uint32_t MipCount;
			uint32_t BaseLayer;
			uint32_t Layers;
			uint32_t Subresource;
			uint8_t SingleSubresource;
			uint8_t HasStencil;
			uint8_t ReadOnly;
			uint8_t PendingClear;
		};

		void resetState() const;
		void endPredication() const;
		void beginPredication() const;
		void endRendering() const;
		bool sameRenderTargets(uint32_t numRenderTargetDescriptors, const Descriptor* pRenderTargetViews,
			const Descriptor* pDepthStencilView) const;
		bool hasPendingClear() const;
		void beginRendering() const;
		void beginTransformFeedback() const;
		void endTransformFeedback() const;
		bool ensureNullTransformFeedback() const;
		void recordOutside() const;
		virtual const void* renderingPNext() const;
		void bindHeap(DescriptorHeap heap) const;
		void pushData(uint32_t offset, const void* pData, uint32_t size) const;
		void writePush(bool graphics, uint32_t rootIndex, uint32_t offset, const void* pData, uint32_t size) const;
		void flushPush(bool graphics) const;
		void setDescriptorTable(const PipelineLayoutObject* pLayout, uint32_t index, uint32_t descriptorIndex) const;
		void setConstants(const PipelineLayoutObject* pLayout, uint32_t index, uint32_t num32BitValues,
			const void* pData, uint32_t destOffsetIn32BitValues) const;
		void setRootAddress(const PipelineLayoutObject* pLayout, uint32_t index, uint64_t address) const;
		bool ensurePreprocess(VkDeviceSize size) const;
		void clearColor(uint32_t attachment, const float color[4], uint32_t numRects, const RectRange* pRects) const;
		void clearOutside(const RenderAttachment& attachment, bool depth, ClearFlag flags, float depthValue,
			uint8_t stencil, const float color[4], uint32_t numRects, const RectRange* pRects) const;
		void flushDeferredClears() const;
		bool imageBound(VkImage image) const;
		void prepareBarriersOutsidePass(uint32_t numBarriers, const ResourceBarrier* pBarriers) const;
		void clearDepth(const RenderAttachment& attachment, ClearFlag clearFlags, float depth, uint8_t stencil,
			uint32_t numRects, const RectRange* pRects) const;
		void clearUnorderedAccessView(const Descriptor& descriptor, const VkClearColorValue& color, uint32_t numRects) const;
		void loadAttachment(RenderAttachment& attachment, const ResourceView_VK* pView) const;
		VkAttachmentLoadOp attachmentLoadOp(const RenderAttachment& attachment) const;

		struct ClearSubresource
		{
			uint32_t Subresource;
			ResourceState State;
		};

		void submitClearBarrier(Resource_VK* pResource, uint32_t regionCount, bool toCopy) const;
		void submitBarrierBatch(VkMemoryBarrier2& memory, std::vector<VkImageMemoryBarrier2>& images) const;
		void pipelineBarrier(const VkDependencyInfo& dependency) const;
		void ensureVertexBindingStride() const;
		void transitionUndefinedAttachment(const RenderAttachment& attachment, bool depth) const;

		mutable VkCommandBuffer		m_commandBuffer;
		bool						m_ownsCommandBuffer;

		struct PoolBuffer
		{
			VkCommandPool Pool;
			VkCommandBuffer Buffer;
		};
		mutable std::vector<PoolBuffer> m_poolBuffers;
		Device_VK*		m_pDevice;
		CommandListType	m_type;

		PFN_vkCmdBeginRendering				m_beginRendering;
		PFN_vkCmdEndRendering				m_endRendering;
		PFN_vkCmdSetPrimitiveTopology		m_setPrimitiveTopology;
		PFN_vkCmdBindVertexBuffers2			m_bindVertexBuffers2;
		mutable bool						m_vertexStrideBound;
		PFN_vkCmdBindIndexBuffer2			m_bindIndexBuffer2;
		PFN_vkCmdBeginDebugUtilsLabelEXT	m_beginLabel;
		PFN_vkCmdEndDebugUtilsLabelEXT		m_endLabel;
		PFN_vkCmdInsertDebugUtilsLabelEXT	m_insertLabel;

		mutable VkBuffer			m_predicateBuffer;
		mutable VkDeviceSize		m_predicateOffset;
		mutable bool				m_predicateEqualZero;
		mutable bool				m_predicationActive;
		mutable bool				m_renderingBegun;
		static constexpr uint32_t	MaxRenderTargetCount = 8;
		static constexpr uint32_t	MaxStreamOutCount = 4;

		mutable bool				m_hasDepth;
		mutable uint32_t			m_colorCount;
		mutable RenderAttachment	m_colors[MaxRenderTargetCount];
		mutable RenderAttachment	m_depth;
		mutable uint32_t			m_renderWidth;
		mutable uint32_t			m_renderHeight;

		mutable const PipelineLayoutObject* m_graphicsLayout;
		mutable const PipelineLayoutObject* m_computeLayout;
		mutable const Pipeline_VK*	m_pipeline;
		mutable DescriptorHeap		m_resourceHeap;
		mutable DescriptorHeap		m_samplerHeap;

		// D3D12 root-signature push budget is 256 bytes. Dirty is tracked per root
		// parameter; flush merges adjacent byte ranges and splits on gaps.
		// writePush skips unchanged staged bytes only while the layout is unchanged;
		// after a layout change (or CB reset) resync forces the next binds to upload.
		static constexpr uint32_t	MaxPushDataBytes = 256;
		static constexpr uint32_t	MaxRootParameters = 64;
		mutable uint8_t				m_graphicsPushData[MaxPushDataBytes];
		mutable uint8_t				m_computePushData[MaxPushDataBytes];
		mutable uint64_t			m_graphicsPushDirty;
		mutable uint64_t			m_computePushDirty;
		mutable bool				m_graphicsPushResync;
		mutable bool				m_computePushResync;

		mutable VkBuffer			m_preprocessBuffer;
		mutable VkDeviceMemory		m_preprocessMemory;
		mutable VkDeviceAddress		m_preprocessAddress;
		mutable VkDeviceSize		m_preprocessSize;
		mutable VkIndirectCommandsLayoutEXT	m_preprocessLayout;
		mutable VkPipeline			m_preprocessPipeline;
		mutable uint32_t			m_preprocessSequences;
		mutable VkDeviceSize		m_preprocessRequired;

		struct DeferredClear
		{
			RenderAttachment Attachment;
			bool Depth;
			ClearFlag Flags;
			float DepthValue;
			uint8_t Stencil;
			float Color[4];
		};

		mutable std::vector<VkClearRect>			m_clearRects;
		mutable std::vector<VkImageCopy>			m_imageCopies;
		mutable std::vector<VkBufferImageCopy>		m_tileCopies;
		mutable std::vector<VkBufferMemoryBarrier2>	m_buffers;
		mutable std::vector<VkImageMemoryBarrier2>	m_images;
		mutable std::vector<VkPipelineStageFlags>	m_legacySrcStages;
		mutable std::vector<VkPipelineStageFlags>	m_legacyDstStages;
		mutable std::vector<VkMemoryBarrier>		m_legacyMemories;
		mutable std::vector<VkBufferMemoryBarrier>	m_legacyBuffers;
		mutable std::vector<VkImageMemoryBarrier>	m_legacyImages;
		mutable std::vector<ClearSubresource>		m_clearRegions;
		mutable std::vector<DeferredClear>			m_deferredClears;

		mutable VkBuffer		m_soBuffers[MaxStreamOutCount];
		mutable VkDeviceSize	m_soOffsets[MaxStreamOutCount];
		mutable VkDeviceSize	m_soSizes[MaxStreamOutCount];
		mutable VkBuffer		m_soCounters[MaxStreamOutCount];
		mutable VkDeviceSize	m_soCounterOffsets[MaxStreamOutCount];
		mutable uint32_t		m_xfbActive;
		mutable VkBuffer		m_nullXfb;
		mutable VkDeviceMemory	m_nullXfbMemory;
	};

	class CommandQueue_VK :
		public virtual CommandQueue
	{
	public:
		CommandQueue_VK();
		virtual ~CommandQueue_VK();

		bool Create(Device* pDevice, CommandListType type, CommandQueueFlag flags,
			int32_t priority = 0, uint32_t nodeMask = 0, const wchar_t* name = nullptr);

		bool SubmitCommandLists(uint32_t numCommandLists, CommandList* const* ppCommandLists,
			const Semaphore* pWaits = nullptr, uint32_t numWaits = 0,
			const Semaphore* pSignals = nullptr, uint32_t numSignals = 0) const;
		bool SubmitCommandList(CommandList* pCommandList,
			const Semaphore* pWaits = nullptr, uint32_t numWaits = 0,
			const Semaphore* pSignals = nullptr, uint32_t numSignals = 0) const;
		bool Wait(Fence* pFence, uint64_t value) const;
		bool Signal(Fence* pFence, uint64_t value) const;

		void ExecuteCommandLists(uint32_t numCommandLists, CommandList* const* ppCommandLists) const;
		void ExecuteCommandList(CommandList* pCommandList) const;
		void UpdateTileMappings(Resource* pResource, uint32_t numResourceRegions,
			const TiledResourceCoord* pResourceRegionStartCoords, const TileRegionSize* pResourceRegionSizes,
			Heap* pHeap, uint32_t numHeapRanges, const TileRangeFlag* pHeapRangeFlags,
			const uint32_t* pHeapRangeStartOffsets, const uint32_t* pHeapRangeTileCounts,
			TileMappingFlag flags = TileMappingFlag::NONE) const;
		void CopyTileMappings(Resource* pDstResource, const TiledResourceCoord* pDstRegionStartCoord,
			Resource* pSrcResource, const TiledResourceCoord* pSrcRegionStartCoordinate,
			const TileRegionSize& regionSize, TileMappingFlag flags = TileMappingFlag::NONE) const;

		void Create(void* pNativeHandle, Device* pDevice);

		void* GetNativeHandle();
		void* GetDeviceNativeHandle() const;

		Device* GetDevice() const;

	protected:
		bool submit(uint32_t numCommandLists, CommandList* const* ppCommandLists,
			const Semaphore* pWaits, uint32_t numWaits, const Semaphore* pSignals, uint32_t numSignals) const;
		bool flushDeferred(const Semaphore* pWaits, uint32_t numWaits,
			const Semaphore* pSignals, uint32_t numSignals) const;
		bool consumeSparseWait(VkSemaphoreSubmitInfo& wait) const;
		VkQueue sparseQueue() const;

		Device_VK*		m_pDevice;
		VkQueue			m_queue;
		uint32_t		m_family;
		uint32_t		m_queueIndex;
		bool			m_registered;
		bool			m_ownsSlot;
		bool			m_sparseQueue;
		mutable uint64_t m_sparseWaited;

		// ExecuteCommandList defers until Signal (or explicit Submit) so
		// Execute -> Present -> Signal collapses to one queue submit + PresentKHR.
		mutable std::vector<CommandList*> m_deferredLists;

		mutable std::vector<VkCommandBufferSubmitInfo> m_commandInfos;
		mutable std::vector<VkSemaphoreSubmitInfo> m_waitInfos;
		mutable std::vector<VkSemaphoreSubmitInfo> m_signalInfos;
		mutable std::vector<VkSemaphore> m_waitSemaphores;
		mutable std::vector<VkPipelineStageFlags> m_waitStages;
		mutable std::vector<uint64_t> m_waitValues;
		mutable std::vector<VkCommandBuffer> m_submitBuffers;
		mutable std::vector<VkSemaphore> m_signalSemaphores;
		mutable std::vector<uint64_t> m_signalValues;
	};

	inline CommandList_VK* AsVK(CommandList* p)
	{
		assert(!p || p->GetBackend());
		return static_cast<CommandList_VK*>(p ? p->GetBackend() : nullptr);
	}

	inline const CommandList_VK* AsVK(const CommandList* p)
	{
		assert(!p || const_cast<CommandList*>(p)->GetBackend());
		return static_cast<const CommandList_VK*>(p ? const_cast<CommandList*>(p)->GetBackend() : nullptr);
	}
}
