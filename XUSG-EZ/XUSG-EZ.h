//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Core/XUSG.h"

namespace XUSG
{
	namespace EZ
	{
		// Resource views
		struct ResourceView
		{
			Resource* pResource;
			Descriptor View;
			std::vector<uint32_t> Subresources;
			ResourceState DstState;
			Resource* pCounter;
		};

		struct VertexBufferView
		{
			VertexBuffer* pResource;
			const XUSG::VertexBufferView* pView;
			ResourceState DstState;
		};

		struct IndexBufferView
		{
			IndexBuffer* pResource;
			const XUSG::IndexBufferView* pView;
			ResourceState DstState;
		};

		static const auto SHADER_RESOURCE_STATE = ResourceState::NON_PIXEL_SHADER_RESOURCE | ResourceState::PIXEL_SHADER_RESOURCE;

		XUSG_INTERFACE void CalcSubresources(std::vector<uint32_t>& subresources, const Texture* pResource, uint8_t mipSlice, uint8_t planeSlice = 0);

		XUSG_INTERFACE uint32_t CalcSubresource(const Texture* pResource, uint8_t mipSlice, uint32_t arraySlice, uint8_t planeSlice = 0);
		XUSG_INTERFACE uint32_t CalcSubresource(const Texture3D* pResource, uint8_t mipSlice, uint8_t planeSlice = 0);

		// Resource view generation helpers coupled for XUSG resources
		XUSG_INTERFACE ResourceView GetCBV(ConstantBuffer* pResource, uint32_t index = 0);
		XUSG_INTERFACE ResourceView GetSRV(Buffer* pResource, uint32_t index = 0,
			ResourceState dstState = SHADER_RESOURCE_STATE);
		XUSG_INTERFACE ResourceView GetSRV(VertexBuffer* pResource, uint32_t index = 0,
			ResourceState dstState = SHADER_RESOURCE_STATE | ResourceState::VERTEX_AND_CONSTANT_BUFFER);
		XUSG_INTERFACE ResourceView GetSRV(IndexBuffer* pResource, uint32_t index = 0,
			ResourceState dstState = SHADER_RESOURCE_STATE | ResourceState::INDEX_BUFFER);
		XUSG_INTERFACE ResourceView GetSRV(Texture* pResource, uint32_t index = 0,
			ResourceState dstState = SHADER_RESOURCE_STATE);
		XUSG_INTERFACE ResourceView GetSRV(Texture3D* pResource,
			ResourceState dstState = SHADER_RESOURCE_STATE);
		XUSG_INTERFACE ResourceView GetSRVLevel(Texture* pResource, uint8_t level,
			ResourceState dstState = SHADER_RESOURCE_STATE);
		XUSG_INTERFACE ResourceView GetSRVLevel(Texture3D* pResource, uint8_t level,
			ResourceState dstState = SHADER_RESOURCE_STATE);
		XUSG_INTERFACE ResourceView GetUAV(StructuredBuffer* pResource, uint8_t index = 0);
		XUSG_INTERFACE ResourceView GetUAV(Buffer* pResource, uint8_t index = 0);
		XUSG_INTERFACE ResourceView GetUAV(Texture* pResource, uint8_t index = 0);
		XUSG_INTERFACE ResourceView GetUAV(Texture3D* pResource, uint8_t index = 0);
		XUSG_INTERFACE ResourceView GetPackedUAV(Texture* pResource, uint8_t index = 0);
		XUSG_INTERFACE ResourceView GetPackedUAV(Texture3D* pResource, uint8_t index = 0);
		XUSG_INTERFACE ResourceView GetPackedUAV(TypedBuffer* pResource, uint8_t index = 0);
		XUSG_INTERFACE ResourceView GetRTV(RenderTarget* pResource, uint32_t slice = 0, uint8_t mipLevel = 0);
		XUSG_INTERFACE ResourceView GetArrayRTV(RenderTarget* pResource, uint8_t mipLevel = 0);
		XUSG_INTERFACE ResourceView GetDSV(DepthStencil* pResource, uint32_t slice = 0, uint8_t mipLevel = 0);
		XUSG_INTERFACE ResourceView GetArrayDSV(DepthStencil* pResource, uint8_t mipLevel = 0);
		XUSG_INTERFACE ResourceView GetReadOnlyDSV(DepthStencil* pResource, uint32_t slice = 0,
			uint8_t mipLevel = 0, ResourceState dstSrvState = SHADER_RESOURCE_STATE);
		XUSG_INTERFACE ResourceView GetReadOnlyArrayDSV(DepthStencil* pResource,
			uint8_t mipLevel = 0, ResourceState dstSrvState = SHADER_RESOURCE_STATE);
		XUSG_INTERFACE ResourceView GetStencilSRV(DepthStencil* pResource,
			ResourceState dstSrvState = SHADER_RESOURCE_STATE);

		XUSG_INTERFACE VertexBufferView GetVBV(VertexBuffer* pResource, uint32_t index = 0,
			ResourceState dstSrvState = SHADER_RESOURCE_STATE);

		XUSG_INTERFACE IndexBufferView GetIBV(IndexBuffer* pResource, uint32_t index = 0,
			ResourceState dstSrvState = SHADER_RESOURCE_STATE);

		//--------------------------------------------------------------------------------------
		// Command list
		//--------------------------------------------------------------------------------------
		class XUSG_INTERFACE CommandList
		{
		public:
			//CommandList();
			virtual ~CommandList() {}

			// By default maxSamplers[stage] = 16, maxCbvsEachSpace[stage] = 14, maxSrvsEachSpace[stage] = 32, and maxUavsEachSpace[stage] = 16
			virtual bool Create(XUSG::CommandList* pCommandList,
				uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
				const uint32_t maxSamplers[Shader::Stage::NUM_STAGE] = nullptr,
				const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
				const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
				const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
				const uint32_t maxCbvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
				const uint32_t maxSrvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
				const uint32_t maxUavSpaces[Shader::Stage::NUM_STAGE] = nullptr) = 0;
			virtual bool Create(const Device* pDevice, void* pHandle,
				uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
				const uint32_t maxSamplers[Shader::Stage::NUM_STAGE] = nullptr,
				const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
				const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
				const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
				const uint32_t maxCbvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
				const uint32_t maxSrvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
				const uint32_t maxUavSpaces[Shader::Stage::NUM_STAGE] = nullptr,
				const wchar_t* name = nullptr) = 0;
			virtual bool Close(RenderTarget* pBackBuffer = nullptr) = 0;
			virtual bool Reset(const CommandAllocator* pAllocator, const Pipeline& initialState) = 0;

			virtual void ClearState(const Pipeline& initialState) const = 0;
			virtual void Draw(
				uint32_t vertexCountPerInstance,
				uint32_t instanceCount,
				uint32_t startVertexLocation,
				uint32_t startInstanceLocation) = 0;
			virtual void DrawIndexed(
				uint32_t indexCountPerInstance,
				uint32_t instanceCount,
				uint32_t startIndexLocation,
				int32_t baseVertexLocation,
				uint32_t startInstanceLocation) = 0;
			virtual void DrawIndirect(const CommandLayout* pCommandlayout,
				uint32_t maxCommandCount,
				Resource* pArgumentBuffer,
				uint64_t argumentBufferOffset = 0,
				Resource* pCountBuffer = nullptr,
				uint64_t countBufferOffset = 0) = 0;
			virtual void Dispatch(
				uint32_t threadGroupCountX,
				uint32_t threadGroupCountY,
				uint32_t threadGroupCountZ) = 0;
			virtual void DispatchIndirect(const CommandLayout* pCommandlayout,
				uint32_t maxCommandCount,
				Resource* pArgumentBuffer,
				uint64_t argumentBufferOffset = 0,
				Resource* pCountBuffer = nullptr,
				uint64_t countBufferOffset = 0) = 0;
			virtual void CopyBufferRegion(Resource* pDstBuffer, uint64_t dstOffset,
				Resource* pSrcBuffer, uint64_t srcOffset, uint64_t numBytes) = 0;
			virtual void CopyTextureRegion(const TextureCopyLocation& dst,
				uint32_t dstX, uint32_t dstY, uint32_t dstZ,
				const TextureCopyLocation& src, const BoxRange* pSrcBox = nullptr) = 0;
			virtual void CopyResource(Resource* pDstResource, Resource* pSrcResource) = 0;
			virtual void CopyTiles(Resource* pTiledResource, const TiledResourceCoord* pTileRegionStartCoord,
				const TileRegionSize* pTileRegionSize, const Resource* pBuffer, uint64_t bufferStartOffsetInBytes,
				TileCopyFlag flags) = 0;
			virtual void ResolveSubresource(Resource* pDstResource, uint32_t dstSubresource,
				Resource* pSrcResource, uint32_t srcSubresource, Format format) = 0;
			virtual void IASetInputLayout(const InputLayout* pLayout) = 0;
			virtual void IASetIndexBufferStripCutValue(IBStripCutValue ibStripCutValue) = 0;
			virtual void RSSetState(const Graphics::Rasterizer* pRasterizer) = 0;
			virtual void RSSetState(Graphics::RasterizerPreset preset) = 0;
			virtual void RSSetViewports(uint32_t numViewports, const Viewport* pViewports) const = 0;
			virtual void RSSetScissorRects(uint32_t numRects, const RectRange* pRects) const = 0;
			virtual void OMSetBlendState(const Graphics::Blend* pBlend, uint32_t sampleMask = UINT_MAX) = 0;
			virtual void OMSetBlendState(Graphics::BlendPreset preset,
				uint8_t numColorRTs = 1, uint32_t sampleMask = UINT_MAX) = 0;
			virtual void OMSetBlendFactor(const float blendFactor[4]) const = 0;
			virtual void OMSetStencilRef(uint32_t stencilRef) const = 0;
			virtual void OMSetSample(uint8_t count, uint8_t quality = 0) = 0;
			virtual void DSSetState(const Graphics::DepthStencil* pDepthStencil) = 0;
			virtual void DSSetState(Graphics::DepthStencilPreset preset) = 0;
			virtual void SetGraphicsShader(Shader::Stage stage, const Blob& shader) = 0;
			virtual void SetGraphicsNodeMask(uint32_t nodeMask) = 0;
			virtual void SetComputeShader(const Blob& shader) = 0;
			virtual void SetComputeNodeMask(uint32_t nodeMask) = 0;
			virtual void SetPipelineState(const Pipeline& pipelineState) = 0;
			virtual void ExecuteBundle(const XUSG::CommandList* pCommandList) const = 0;
			virtual void SetSamplers(Shader::Stage stage, uint32_t startBinding,
				uint32_t numSamplers, const Sampler* const* pSamplers) = 0;
			virtual void SetSamplerStates(Shader::Stage stage, uint32_t startBinding,
				uint32_t numSamplers, const SamplerPreset* pSamplerPresets) = 0;
			virtual void SetResources(Shader::Stage stage, DescriptorType descriptorType, uint32_t startBinding,
				uint32_t numResources, const ResourceView* pResourceViews, uint32_t space = 0) = 0;
			virtual void IASetPrimitiveTopology(PrimitiveTopology primitiveTopology) = 0;
			virtual void IASetIndexBuffer(const IndexBufferView& view) = 0;
			virtual void IASetVertexBuffers(uint32_t startSlot, uint32_t numViews, const VertexBufferView* pViews) = 0;
			virtual void SOSetTargets(uint32_t startSlot, uint32_t numViews, const StreamOutBufferView* pViews, Resource* const* ppResources) = 0;
			virtual void OMSetRenderTargets(
				uint32_t numRenderTargets,
				const ResourceView* pRenderTargetViews,
				const ResourceView* pDepthStencilView = nullptr) = 0;
			virtual void ClearDepthStencilView(const ResourceView& depthStencilView, ClearFlag clearFlags,
				float depth, uint8_t stencil = 0, uint32_t numRects = 0, const RectRange* pRects = nullptr) = 0;
			virtual void ClearRenderTargetView(const ResourceView& renderTargetView, const float colorRGBA[4],
				uint32_t numRects = 0, const RectRange* pRects = nullptr) = 0;
			virtual void ClearUnorderedAccessViewUint(const ResourceView& unorderedAccessView, const uint32_t values[4],
				uint32_t numRects = 0, const RectRange* pRects = nullptr) = 0;
			virtual void ClearUnorderedAccessViewFloat(const ResourceView& unorderedAccessView, const float values[4],
				uint32_t numRects = 0, const RectRange* pRects = nullptr) = 0;
			virtual void DiscardResource(const Resource* pResource, uint32_t numRects, const RectRange* pRects,
				uint32_t firstSubresource, uint32_t numSubresources) = 0;
			virtual void BeginQuery(const QueryPool& queryPool, QueryType type, uint32_t index) const = 0;
			virtual void EndQuery(const QueryPool& queryPool, QueryType type, uint32_t index) const = 0;
			virtual void ResolveQueryData(const QueryPool& queryPool, QueryType type, uint32_t startIndex,
				uint32_t numQueries, const Resource* pDstBuffer, uint64_t alignedDstBufferOffset) const = 0;
			virtual void SetPredication(const Resource* pBuffer, uint64_t alignedBufferOffset, bool opEqualZero)const = 0;
			virtual void SetMarker(uint32_t metaData, const void* pData, uint32_t size) const = 0;
			virtual void BeginEvent(uint32_t metaData, const void* pData, uint32_t size) const = 0;
			virtual void EndEvent() = 0;

			virtual void ResetDescriptorPool(DescriptorPoolType type) = 0;
			virtual void Resize() = 0;

			virtual void Blit(Texture* pDstResource, Texture* pSrcResource, SamplerPreset sampler,
				const Blob& customShader = nullptr, uint32_t dstMip = 0, uint32_t srcMip = 0) = 0;
			virtual void GenerateMips(Texture* pResource, SamplerPreset sampler, const Blob& customShader = nullptr) = 0;

			virtual void* GetHandle() const = 0;
			virtual void* GetDeviceHandle() const = 0;

			virtual const Device* GetDevice() const = 0;
			virtual XUSG::CommandList* AsCommandList() = 0;

			using uptr = std::unique_ptr<CommandList>;
			using sptr = std::shared_ptr<CommandList>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
			static uptr MakeUnique(XUSG::CommandList* pCommandList,
				uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
				const uint32_t maxSamplers[Shader::Stage::NUM_STAGE] = nullptr,
				const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
				const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
				const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
				const uint32_t maxCbvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
				const uint32_t maxSrvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
				const uint32_t maxUavSpaces[Shader::Stage::NUM_STAGE] = nullptr,
				API api = API::DIRECTX_12);
			static sptr MakeShared(XUSG::CommandList* pCommandList,
				uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
				const uint32_t maxSamplers[Shader::Stage::NUM_STAGE] = nullptr,
				const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
				const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
				const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
				const uint32_t maxCbvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
				const uint32_t maxSrvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
				const uint32_t maxUavSpaces[Shader::Stage::NUM_STAGE] = nullptr,
				API api = API::DIRECTX_12);
		};
	}
}
