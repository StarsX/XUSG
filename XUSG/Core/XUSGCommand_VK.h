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
			const Resource* pSrcBuffer, uint64_t srcOffset, uint64_t numBytes) const;
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
		void Barrier(uint32_t numBarriers, const ResourceBarrier* pBarriers);
		void ExecuteBundle(CommandList* pCommandList) const;
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
		void EndEvent() const;
		void ExecuteIndirect(CommandLayout* pCommandlayout, uint32_t maxCommandCount,
			const Resource* pArgumentBuffer, uint64_t argumentBufferOffset = 0,
			const Resource* pCountBuffer = nullptr, uint64_t countBufferOffset = 0) const;

		void Create(void* pNativeHandle, Device* pDevice);
		void Attach(const CommandList_VK& source);

		void* GetNativeHandle();
		void* GetDeviceNativeHandle() const;
		void* GetBackend();

		// Ultimate::CommandList_VK* when this object is the Ultimate wrap; nullptr for core-only lists.
		// Distinct from GetBackend(), which always returns the XUSG::CommandList_VK* subobject.
		virtual void* GetUltimateBackend();

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
		void submitBarrierBatch(VkMemoryBarrier2& memory, std::vector<VkBufferMemoryBarrier2>& buffers,
			std::vector<VkImageMemoryBarrier2>& images) const;
		void pipelineBarrier(const VkDependencyInfo& dependency) const;
		void ensureVertexBindingStride() const;
		void transitionUndefinedAttachment(const RenderAttachment& attachment, bool depth) const;

		struct PoolBuffer
		{
			VkCommandPool Pool;
			VkCommandBuffer Buffer;
		};

		struct DeferredClear
		{
			RenderAttachment Attachment;
			bool Depth;
			ClearFlag Flags;
			float DepthValue;
			uint8_t Stencil;
			float Color[4];
		};

		static constexpr uint32_t	MaxRenderTargetCount = 8;
		static constexpr uint32_t	MaxStreamOutCount = 4;
		// D3D12 root-signature push budget is 256 bytes. Dirty is tracked per root
		// parameter; flush merges adjacent byte ranges and splits on gaps.
		// writePush skips unchanged staged bytes only while the layout is unchanged;
		// after a layout change (or CB reset) resync forces the next binds to upload.
		static constexpr uint32_t	MaxPushDataBytes = 256;
		static constexpr uint32_t	MaxRootParameters = 64;

		// Shared by every wrapper. The last release frees the Vulkan objects.
		struct State
		{
			~State();

			VkCommandBuffer			m_commandBuffer = VK_NULL_HANDLE;
			bool					m_ownsCommandBuffer = false;
			std::vector<PoolBuffer>	m_poolBuffers;
			Device_VK*				m_pDevice = nullptr;
			CommandListType			m_type = CommandListType::DIRECT;

			PFN_vkCmdBeginRendering				m_beginRendering = nullptr;
			PFN_vkCmdEndRendering				m_endRendering = nullptr;
			PFN_vkCmdSetPrimitiveTopology		m_setPrimitiveTopology = nullptr;
			PFN_vkCmdBindVertexBuffers2			m_bindVertexBuffers2 = nullptr;
			bool								m_vertexStrideBound = false;
			PFN_vkCmdBindIndexBuffer2			m_bindIndexBuffer2 = nullptr;
			PFN_vkCmdBeginDebugUtilsLabelEXT	m_beginLabel = nullptr;
			PFN_vkCmdEndDebugUtilsLabelEXT		m_endLabel = nullptr;
			PFN_vkCmdInsertDebugUtilsLabelEXT	m_insertLabel = nullptr;

			VkBuffer			m_predicateBuffer = VK_NULL_HANDLE;
			VkDeviceSize		m_predicateOffset = 0;
			bool				m_predicateEqualZero = false;
			bool				m_predicationActive = false;
			bool				m_renderingBegun = false;

			bool				m_hasDepth = false;
			uint32_t			m_colorCount = 0;
			RenderAttachment	m_colors[MaxRenderTargetCount] = {};
			RenderAttachment	m_depth = {};
			uint32_t			m_renderWidth = 1;
			uint32_t			m_renderHeight = 1;

			const PipelineLayoutObject*	m_graphicsLayout = nullptr;
			const PipelineLayoutObject*	m_computeLayout = nullptr;
			const Pipeline_VK*			m_pipeline = nullptr;
			DescriptorHeap				m_resourceHeap = nullptr;
			DescriptorHeap				m_samplerHeap = nullptr;

			uint8_t		m_graphicsPushData[MaxPushDataBytes] = {};
			uint8_t		m_computePushData[MaxPushDataBytes] = {};
			uint64_t	m_graphicsPushDirty = 0;
			uint64_t	m_computePushDirty = 0;
			bool		m_graphicsPushResync = true;
			bool		m_computePushResync = true;

			VkBuffer					m_preprocessBuffer = VK_NULL_HANDLE;
			VkDeviceMemory				m_preprocessMemory = VK_NULL_HANDLE;
			VkDeviceAddress				m_preprocessAddress = 0;
			VkDeviceSize				m_preprocessSize = 0;
			VkIndirectCommandsLayoutEXT	m_preprocessLayout = VK_NULL_HANDLE;
			VkPipeline					m_preprocessPipeline = VK_NULL_HANDLE;
			uint32_t					m_preprocessSequences = 0;
			VkDeviceSize				m_preprocessRequired = 0;

			std::vector<VkClearRect>			m_clearRects;
			std::vector<VkImageCopy>			m_imageCopies;
			std::vector<VkBufferImageCopy>		m_tileCopies;
			std::vector<VkBufferMemoryBarrier2>	m_buffers;
			std::vector<VkImageMemoryBarrier2>	m_images;
			std::vector<VkPipelineStageFlags>	m_legacySrcStages;
			std::vector<VkPipelineStageFlags>	m_legacyDstStages;
			std::vector<VkMemoryBarrier>		m_legacyMemories;
			std::vector<VkBufferMemoryBarrier>	m_legacyBuffers;
			std::vector<VkImageMemoryBarrier>	m_legacyImages;
			std::vector<ClearSubresource>		m_clearRegions;
			std::vector<DeferredClear>			m_deferredClears;

			VkBuffer		m_soBuffers[MaxStreamOutCount] = {};
			VkDeviceSize	m_soOffsets[MaxStreamOutCount] = {};
			VkDeviceSize	m_soSizes[MaxStreamOutCount] = {};
			VkBuffer		m_soCounters[MaxStreamOutCount] = {};
			VkDeviceSize	m_soCounterOffsets[MaxStreamOutCount] = {};
			uint32_t		m_xfbActive = 0;
			VkBuffer		m_nullXfb = VK_NULL_HANDLE;
			VkDeviceMemory	m_nullXfbMemory = VK_NULL_HANDLE;
		};

		Device_VK*				m_pDevice;
		std::shared_ptr<State>	m_state;
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
			const Semaphore* pSignals = nullptr, uint32_t numSignals = 0);
		bool SubmitCommandList(CommandList* pCommandList,
			const Semaphore* pWaits = nullptr, uint32_t numWaits = 0,
			const Semaphore* pSignals = nullptr, uint32_t numSignals = 0);
		bool Wait(Fence* pFence, uint64_t value);
		bool Signal(Fence* pFence, uint64_t value);

		void ExecuteCommandLists(uint32_t numCommandLists, CommandList* const* ppCommandLists);
		void ExecuteCommandList(CommandList* pCommandList);
		void UpdateTileMappings(Resource* pResource, uint32_t numResourceRegions,
			const TiledResourceCoord* pResourceRegionStartCoords, const TileRegionSize* pResourceRegionSizes,
			Heap* pHeap, uint32_t numHeapRanges, const TileRangeFlag* pHeapRangeFlags,
			const uint32_t* pHeapRangeStartOffsets, const uint32_t* pHeapRangeTileCounts,
			TileMappingFlag flags = TileMappingFlag::NONE);
		void CopyTileMappings(Resource* pDstResource, const TiledResourceCoord* pDstRegionStartCoord,
			Resource* pSrcResource, const TiledResourceCoord* pSrcRegionStartCoordinate,
			const TileRegionSize& regionSize, TileMappingFlag flags = TileMappingFlag::NONE) const;

		void Create(void* pNativeHandle, Device* pDevice);

		void* GetNativeHandle() const;
		void* GetDeviceNativeHandle() const;

		Device* GetDevice() const;

	protected:
		bool submit(uint32_t numCommandLists, CommandList* const* ppCommandLists,
			const Semaphore* pWaits, uint32_t numWaits, const Semaphore* pSignals, uint32_t numSignals);
		bool flushDeferred(const Semaphore* pWaits, uint32_t numWaits,
			const Semaphore* pSignals, uint32_t numSignals);
		bool consumeSparseWait(VkSemaphoreSubmitInfo& wait);
		VkQueue sparseQueue() const;

		Device_VK*		m_pDevice;
		VkQueue			m_queue;
		uint32_t		m_family;
		uint32_t		m_queueIndex;
		bool			m_registered;
		bool			m_ownsSlot;
		bool			m_sparseQueue;
		uint64_t		m_sparseWaited;

		// ExecuteCommandList defers until Signal (or explicit Submit) so
		// Execute -> Present -> Signal collapses to one queue submit + PresentKHR.
		std::vector<CommandList*> m_deferredLists;

		std::vector<VkCommandBufferSubmitInfo> m_commandInfos;
		std::vector<VkSemaphoreSubmitInfo> m_waitInfos;
		std::vector<VkSemaphoreSubmitInfo> m_signalInfos;
		std::vector<VkSemaphore> m_waitSemaphores;
		std::vector<VkPipelineStageFlags> m_waitStages;
		std::vector<uint64_t> m_waitValues;
		std::vector<VkCommandBuffer> m_submitBuffers;
		std::vector<VkSemaphore> m_signalSemaphores;
		std::vector<uint64_t> m_signalValues;
	};

	CommandList_VK* AsVK(CommandList* p);
}
