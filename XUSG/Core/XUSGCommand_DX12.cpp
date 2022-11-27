//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSG_DX12.h"
#include "XUSGCommand_DX12.h"
#include "XUSGResource_DX12.h"
#include "XUSGEnum_DX12.h"

using namespace std;
using namespace XUSG;

CommandAllocator_DX12::CommandAllocator_DX12()
{
}

CommandAllocator_DX12::~CommandAllocator_DX12()
{
}

bool CommandAllocator_DX12::Create(const Device* pDevice, CommandListType type, const wchar_t* name)
{
	const auto pDxDevice = static_cast<ID3D12Device*>(pDevice->GetHandle());

	assert(pDxDevice);
	V_RETURN(pDxDevice->CreateCommandAllocator(GetDX12CommandListType(type),
		IID_PPV_ARGS(&m_commandAllocator)), cerr, false);
	if (name) m_commandAllocator->SetName(name);

	m_pDevice = pDevice;

	return true;
}

bool CommandAllocator_DX12::Reset()
{
	V_RETURN(m_commandAllocator->Reset(), cerr, false);

	return true;
}

void CommandAllocator_DX12::Create(void* pHandle, const wchar_t* name)
{
	m_commandAllocator = pHandle;
	if (name) m_commandAllocator->SetName(name);
}

void* CommandAllocator_DX12::GetHandle() const
{
	return m_commandAllocator.get();
}

void* CommandAllocator_DX12::GetDeviceHandle() const
{
	com_ptr<ID3D12Device> device;
	V_RETURN(m_commandAllocator->GetDevice(IID_PPV_ARGS(&device)), cerr, nullptr);

	return device.get();
}

const Device* CommandAllocator_DX12::GetDevice() const
{
	return m_pDevice;
}

//--------------------------------------------------------------------------------------

CommandList_DX12::CommandList_DX12()
{
}

CommandList_DX12::~CommandList_DX12()
{
}

bool CommandList_DX12::Create(const Device* pDevice, uint32_t nodeMask, CommandListType type,
	const CommandAllocator* pCommandAllocator, const Pipeline& pipeline, const wchar_t* name)
{
	const auto pDxDevice = static_cast<ID3D12Device*>(pDevice->GetHandle());
	const auto pDxCommandAllocator = static_cast<ID3D12CommandAllocator*>(pCommandAllocator->GetHandle());

	assert(pDxDevice);
	V_RETURN(pDxDevice->CreateCommandList(nodeMask, GetDX12CommandListType(type), pDxCommandAllocator,
		static_cast<ID3D12PipelineState*>(pipeline), IID_PPV_ARGS(&m_commandList)), cerr, false);
	if (name) m_commandList->SetName(name);

	m_pDevice = pDevice;

	return true;
}

bool CommandList_DX12::Close() const
{
	V_RETURN(m_commandList->Close(), cerr, false);

	return true;
}

bool CommandList_DX12::Reset(const CommandAllocator* pCommandAllocator, const Pipeline& initialState) const
{
	assert(pCommandAllocator);
	V_RETURN(m_commandList->Reset(static_cast<ID3D12CommandAllocator*>(pCommandAllocator->GetHandle()),
		static_cast<ID3D12PipelineState*>(initialState)), cerr, false);

	return true;
}

void CommandList_DX12::ClearState(const Pipeline& initialState) const
{
	m_commandList->ClearState(static_cast<ID3D12PipelineState*>(initialState));
}

void CommandList_DX12::Draw(uint32_t vertexCountPerInstance, uint32_t instanceCount,
	uint32_t startVertexLocation, uint32_t startInstanceLocation) const
{
	m_commandList->DrawInstanced(vertexCountPerInstance, instanceCount,
		startVertexLocation, startInstanceLocation);
}

void CommandList_DX12::DrawIndexed(uint32_t indexCountPerInstance, uint32_t instanceCount,
	uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation) const
{
	m_commandList->DrawIndexedInstanced(indexCountPerInstance, instanceCount,
		startIndexLocation, baseVertexLocation, startInstanceLocation);
}

void CommandList_DX12::Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) const
{
	m_commandList->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}

void CommandList_DX12::CopyBufferRegion(const Resource* pDstBuffer, uint64_t dstOffset,
	const Resource* pSrcBuffer, uint64_t srcOffset, uint64_t numBytes) const
{
	assert(pDstBuffer);
	assert(pSrcBuffer);
	m_commandList->CopyBufferRegion(static_cast<ID3D12Resource*>(pDstBuffer->GetHandle()), dstOffset,
		static_cast<ID3D12Resource*>(pSrcBuffer->GetHandle()), srcOffset, numBytes);
}

void CommandList_DX12::CopyTextureRegion(const TextureCopyLocation& dst,
	uint32_t dstX, uint32_t dstY, uint32_t dstZ, const TextureCopyLocation& src,
	const BoxRange* pSrcBox) const
{
	D3D12_BOX srcBox;
	if (pSrcBox)
	{
		srcBox.left = pSrcBox->Left;
		srcBox.top = pSrcBox->Top;
		srcBox.front = pSrcBox->Front;
		srcBox.right = pSrcBox->Right;
		srcBox.bottom = pSrcBox->Bottom;
		srcBox.back = pSrcBox->Back;
	};

	assert(dst.pResource);
	assert(src.pResource);
	const CD3DX12_TEXTURE_COPY_LOCATION dstCopy(static_cast<ID3D12Resource*>(dst.pResource->GetHandle()), dst.SubresourceIndex);
	const CD3DX12_TEXTURE_COPY_LOCATION srcCopy(static_cast<ID3D12Resource*>(src.pResource->GetHandle()), src.SubresourceIndex);
	m_commandList->CopyTextureRegion(&dstCopy, dstX, dstY, dstZ, &srcCopy, pSrcBox ? &srcBox : nullptr);
}

void CommandList_DX12::CopyResource(const Resource* pDstResource, const Resource* pSrcResource) const
{
	assert(pDstResource);
	assert(pSrcResource);
	m_commandList->CopyResource(static_cast<ID3D12Resource*>(pDstResource->GetHandle()),
		static_cast<ID3D12Resource*>(pSrcResource->GetHandle()));
}

void CommandList_DX12::CopyTiles(const Resource* pTiledResource, const TiledResourceCoord* pTileRegionStartCoord,
	const TileRegionSize* pTileRegionSize, const Resource* pBuffer, uint64_t bufferStartOffsetInBytes,
	TileCopyFlag flags) const
{
	assert(pTiledResource);
	assert(pTileRegionStartCoord);
	assert(pTileRegionSize);
	assert(pBuffer);

	const D3D12_TILED_RESOURCE_COORDINATE tileRegionStartCoord =
	{
		pTileRegionStartCoord->X,
		pTileRegionStartCoord->Y,
		pTileRegionStartCoord->Z,
		pTileRegionStartCoord->Subresource
	};

	const D3D12_TILE_REGION_SIZE tileRegionSize =
	{
		pTileRegionSize->NumTiles,
		pTileRegionSize->UseBox,
		pTileRegionSize->Width,
		pTileRegionSize->Height,
		pTileRegionSize->Depth
	};

	m_commandList->CopyTiles(static_cast<ID3D12Resource*>(pTiledResource->GetHandle()), &tileRegionStartCoord,
		&tileRegionSize, static_cast<ID3D12Resource*>(pBuffer->GetHandle()), bufferStartOffsetInBytes,
		GetDX12TileCopyFlags(flags));
}

void CommandList_DX12::ResolveSubresource(const Resource* pDstResource, uint32_t dstSubresource,
	const Resource* pSrcResource, uint32_t srcSubresource, Format format) const
{
	assert(pDstResource);
	assert(pSrcResource);
	m_commandList->ResolveSubresource(static_cast<ID3D12Resource*>(pDstResource->GetHandle()), dstSubresource,
		static_cast<ID3D12Resource*>(pSrcResource->GetHandle()), srcSubresource, GetDXGIFormat(format));
}

void CommandList_DX12::IASetPrimitiveTopology(PrimitiveTopology primitiveTopology) const
{
	m_commandList->IASetPrimitiveTopology(GetDX12PrimitiveTopology(primitiveTopology));
}

void CommandList_DX12::RSSetViewports(uint32_t numViewports, const Viewport* pViewports) const
{
	assert(numViewports == 0 || pViewports);
	assert(numViewports <= D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE);

	D3D12_VIEWPORT viewports[D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
	for (uint8_t i = 0; i < numViewports; ++i)
	{
		viewports[i].TopLeftX = pViewports[i].TopLeftX;
		viewports[i].TopLeftY = pViewports[i].TopLeftY;
		viewports[i].Width = pViewports[i].Width;
		viewports[i].Height = pViewports[i].Height;
		viewports[i].MinDepth = pViewports[i].MinDepth;
		viewports[i].MaxDepth = pViewports[i].MaxDepth;
	}

	m_commandList->RSSetViewports(numViewports, pViewports ? viewports : nullptr);
}

void CommandList_DX12::RSSetScissorRects(uint32_t numRects, const RectRange* pRects) const
{
	assert(numRects == 0 || pRects);
	assert(numRects <= D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE);

	D3D12_RECT rects[D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
	for (uint8_t i = 0; i < numRects; ++i)
	{
		rects[i].left = pRects[i].Left;
		rects[i].top = pRects[i].Top;
		rects[i].right = pRects[i].Right;
		rects[i].bottom = pRects[i].Bottom;
	}

	m_commandList->RSSetScissorRects(numRects, pRects ? rects : nullptr);
}

void CommandList_DX12::OMSetBlendFactor(const float blendFactor[4]) const
{
	m_commandList->OMSetBlendFactor(blendFactor);
}

void CommandList_DX12::OMSetStencilRef(uint32_t stencilRef) const
{
	m_commandList->OMSetStencilRef(stencilRef);
}

void CommandList_DX12::SetPipelineState(const Pipeline& pipelineState) const
{
	m_commandList->SetPipelineState(static_cast<ID3D12PipelineState*>(pipelineState));
}

void CommandList_DX12::Barrier(uint32_t numBarriers, const ResourceBarrier* pBarriers)
{
	if (numBarriers > 0)
	{
		assert(pBarriers);
		if (m_barriers.size() < numBarriers) m_barriers.resize(numBarriers);
		for (auto i = 0u; i < numBarriers; ++i)
		{
			const auto& barrier = pBarriers[i];
			const auto pResourceAfter = barrier.pResourceAfter ? static_cast<ID3D12Resource*>(barrier.pResourceAfter->GetHandle()) : nullptr;

			if (barrier.pResource)
			{
				const auto pResource = static_cast<ID3D12Resource*>(barrier.pResource->GetHandle());

				if (barrier.StateBefore == barrier.StateAfter)
					m_barriers[i] = barrier.StateAfter == ResourceState::UNORDERED_ACCESS ?
						CD3DX12_RESOURCE_BARRIER::UAV(pResource) : CD3DX12_RESOURCE_BARRIER::Aliasing(pResource, pResourceAfter);
				else m_barriers[i] = CD3DX12_RESOURCE_BARRIER::Transition(pResource, GetDX12ResourceStates(barrier.StateBefore),
					GetDX12ResourceStates(barrier.StateAfter), barrier.Subresource, GetDX12BarrierFlags(barrier.Flags));
			}
			else m_barriers[i] = barrier.StateBefore == ResourceState::UNORDERED_ACCESS ?
				CD3DX12_RESOURCE_BARRIER::UAV(nullptr) : CD3DX12_RESOURCE_BARRIER::Aliasing(nullptr, pResourceAfter);
		}

		m_commandList->ResourceBarrier(numBarriers, m_barriers.data());
	}
}

void CommandList_DX12::ExecuteBundle(const CommandList* pCommandList) const
{
	assert(pCommandList);
	m_commandList->ExecuteBundle(static_cast<ID3D12GraphicsCommandList*>(pCommandList->GetHandle()));
}

void CommandList_DX12::SetDescriptorHeaps(uint32_t numDescriptorHeaps, const DescriptorHeap* pDescriptorHeaps) const
{
	assert(numDescriptorHeaps == 0 || pDescriptorHeaps);
	m_commandList->SetDescriptorHeaps(numDescriptorHeaps, pDescriptorHeaps ?
		reinterpret_cast<ID3D12DescriptorHeap* const*>(pDescriptorHeaps) : nullptr);
}

void CommandList_DX12::SetComputePipelineLayout(const PipelineLayout& pipelineLayout) const
{
	m_commandList->SetComputeRootSignature(static_cast<ID3D12RootSignature*>(pipelineLayout));
}

void CommandList_DX12::SetGraphicsPipelineLayout(const PipelineLayout& pipelineLayout) const
{
	m_commandList->SetGraphicsRootSignature(static_cast<ID3D12RootSignature*>(pipelineLayout));
}

void CommandList_DX12::SetComputeDescriptorTable(uint32_t index, const DescriptorTable& descriptorTable) const
{
	m_commandList->SetComputeRootDescriptorTable(index, { *descriptorTable });
}

void CommandList_DX12::SetGraphicsDescriptorTable(uint32_t index, const DescriptorTable& descriptorTable) const
{
	m_commandList->SetGraphicsRootDescriptorTable(index, { *descriptorTable });
}

void CommandList_DX12::SetCompute32BitConstant(uint32_t index, uint32_t srcData, uint32_t destOffsetIn32BitValues) const
{
	m_commandList->SetComputeRoot32BitConstant(index, srcData, destOffsetIn32BitValues);
}

void CommandList_DX12::SetGraphics32BitConstant(uint32_t index, uint32_t srcData, uint32_t destOffsetIn32BitValues) const
{
	m_commandList->SetGraphicsRoot32BitConstant(index, srcData, destOffsetIn32BitValues);
}

void CommandList_DX12::SetCompute32BitConstants(uint32_t index, uint32_t num32BitValuesToSet,
	const void* pSrcData, uint32_t destOffsetIn32BitValues) const
{
	m_commandList->SetComputeRoot32BitConstants(index, num32BitValuesToSet, pSrcData, destOffsetIn32BitValues);
}

void CommandList_DX12::SetGraphics32BitConstants(uint32_t index, uint32_t num32BitValuesToSet,
	const void* pSrcData, uint32_t destOffsetIn32BitValues) const
{
	m_commandList->SetGraphicsRoot32BitConstants(index, num32BitValuesToSet, pSrcData, destOffsetIn32BitValues);
}

void CommandList_DX12::SetComputeRootConstantBufferView(uint32_t index, const Resource* pResource, int offset) const
{
	m_commandList->SetComputeRootConstantBufferView(index, pResource ? pResource->GetVirtualAddress(offset) : 0);
}

void CommandList_DX12::SetGraphicsRootConstantBufferView(uint32_t index, const Resource* pResource, int offset) const
{
	m_commandList->SetGraphicsRootConstantBufferView(index, pResource ? pResource->GetVirtualAddress(offset) : 0);
}

void CommandList_DX12::SetComputeRootShaderResourceView(uint32_t index, const Resource* pResource, int offset) const
{
	m_commandList->SetComputeRootShaderResourceView(index, pResource ? pResource->GetVirtualAddress(offset) : 0);
}

void CommandList_DX12::SetGraphicsRootShaderResourceView(uint32_t index, const Resource* pResource, int offset) const
{
	m_commandList->SetGraphicsRootShaderResourceView(index, pResource ? pResource->GetVirtualAddress(offset) : 0);
}

void CommandList_DX12::SetComputeRootUnorderedAccessView(uint32_t index, const Resource* pResource, int offset) const
{
	m_commandList->SetComputeRootUnorderedAccessView(index, pResource ? pResource->GetVirtualAddress(offset) : 0);
}

void CommandList_DX12::SetGraphicsRootUnorderedAccessView(uint32_t index, const Resource* pResource, int offset) const
{
	m_commandList->SetGraphicsRootUnorderedAccessView(index, pResource ? pResource->GetVirtualAddress(offset) : 0);
}

void CommandList_DX12::SetComputeRootConstantBufferView(uint32_t index, uint64_t address) const
{
	m_commandList->SetComputeRootConstantBufferView(index, address);
}

void CommandList_DX12::SetGraphicsRootConstantBufferView(uint32_t index, uint64_t address) const
{
	m_commandList->SetGraphicsRootConstantBufferView(index, address);
}

void CommandList_DX12::SetComputeRootShaderResourceView(uint32_t index, uint64_t address) const
{
	m_commandList->SetComputeRootShaderResourceView(index, address);
}

void CommandList_DX12::SetGraphicsRootShaderResourceView(uint32_t index, uint64_t address) const
{
	m_commandList->SetGraphicsRootShaderResourceView(index, address);
}

void CommandList_DX12::SetComputeRootUnorderedAccessView(uint32_t index, uint64_t address) const
{
	m_commandList->SetComputeRootUnorderedAccessView(index, address);
}

void CommandList_DX12::SetGraphicsRootUnorderedAccessView(uint32_t index, uint64_t address) const
{
	m_commandList->SetGraphicsRootUnorderedAccessView(index, address);
}

void CommandList_DX12::IASetIndexBuffer(const IndexBufferView& view) const
{
	D3D12_INDEX_BUFFER_VIEW ibv = { view.BufferLocation, view.SizeInBytes, GetDXGIFormat(view.Format) };
	m_commandList->IASetIndexBuffer(&ibv);
}

void CommandList_DX12::IASetVertexBuffers(uint32_t startSlot, uint32_t numViews, const VertexBufferView* pViews) const
{
	assert(numViews == 0 || pViews);
	assert(numViews <= D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT);

	D3D12_VERTEX_BUFFER_VIEW views[D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
	for (uint8_t i = 0; i < numViews; ++i)
	{
		views[i].BufferLocation = pViews[i].BufferLocation;
		views[i].SizeInBytes = pViews[i].SizeInBytes;
		views[i].StrideInBytes = pViews[i].StrideInBytes;
	}

	m_commandList->IASetVertexBuffers(startSlot, numViews, pViews ? views : nullptr);
}

void CommandList_DX12::SOSetTargets(uint32_t startSlot, uint32_t numViews, const StreamOutBufferView* pViews) const
{
	assert(numViews == 0 || pViews);
	assert(numViews <= 4);

	D3D12_STREAM_OUTPUT_BUFFER_VIEW views[4];
	for (uint8_t i = 0; i < numViews; ++i)
	{
		views[i].BufferLocation = pViews[i].BufferLocation;
		views[i].SizeInBytes = pViews[i].SizeInBytes;
		views[i].BufferFilledSizeLocation = pViews[i].BufferFilledSizeLocation;
	}

	m_commandList->SOSetTargets(startSlot, numViews, pViews ? views : nullptr);
}

void CommandList_DX12::OMSetFramebuffer(const Framebuffer& framebuffer) const
{
	assert(framebuffer.NumRenderTargetDescriptors < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT);

	D3D12_CPU_DESCRIPTOR_HANDLE renderTargetViews[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
	for (uint8_t i = 0; i < framebuffer.NumRenderTargetDescriptors; ++i)
		renderTargetViews[i].ptr = framebuffer.RenderTargetViews.get()[i];

	const D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView = { framebuffer.DepthStencilView };

	m_commandList->OMSetRenderTargets(framebuffer.NumRenderTargetDescriptors, renderTargetViews, true,
		framebuffer.DepthStencilView ? &depthStencilView : nullptr);
}

void CommandList_DX12::OMSetRenderTargets(uint32_t numRenderTargetDescriptors, const Descriptor* pRenderTargetViews,
	const Descriptor* pDepthStencilView, bool rtsSingleHandleToDescriptorRange) const
{
	assert(numRenderTargetDescriptors == 0 || pRenderTargetViews);
	assert(numRenderTargetDescriptors < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT);

	D3D12_CPU_DESCRIPTOR_HANDLE renderTargetViews[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
	for (uint8_t i = 0; i < numRenderTargetDescriptors; ++i)
		renderTargetViews[i].ptr = pRenderTargetViews[i];

	CD3DX12_CPU_DESCRIPTOR_HANDLE depthStencilView(D3D12_DEFAULT);
	depthStencilView.ptr = pDepthStencilView ? *pDepthStencilView : depthStencilView.ptr;

	m_commandList->OMSetRenderTargets(numRenderTargetDescriptors, pRenderTargetViews ?
		renderTargetViews : nullptr, rtsSingleHandleToDescriptorRange,
		pDepthStencilView ? &depthStencilView : nullptr);
}

void CommandList_DX12::ClearDepthStencilView(const Framebuffer& framebuffer, ClearFlag clearFlags, float depth,
	uint8_t stencil, uint32_t numRects, const RectRange* pRects)
{
	ClearDepthStencilView(framebuffer.DepthStencilView, clearFlags, depth, stencil, numRects, pRects);
}

void CommandList_DX12::ClearDepthStencilView(const Descriptor& depthStencilView, ClearFlag clearFlags, float depth,
	uint8_t stencil, uint32_t numRects, const RectRange* pRects)
{
	assert(numRects == 0 || pRects);

	if (m_rects.size() < numRects) m_rects.resize(numRects);
	for (auto i = 0u; i < numRects; ++i)
	{
		m_rects[i].left = pRects[i].Left;
		m_rects[i].top = pRects[i].Top;
		m_rects[i].right = pRects[i].Right;
		m_rects[i].bottom = pRects[i].Bottom;
	}

	m_commandList->ClearDepthStencilView({ depthStencilView }, GetDX12ClearFlags(clearFlags),
		depth, stencil, numRects, pRects ? m_rects.data() : nullptr);
}

void CommandList_DX12::ClearRenderTargetView(const Descriptor& renderTargetView, const float colorRGBA[4],
	uint32_t numRects, const RectRange* pRects)
{
	assert(numRects == 0 || pRects);

	if (m_rects.size() < numRects) m_rects.resize(numRects);
	for (auto i = 0u; i < numRects; ++i)
	{
		m_rects[i].left = pRects[i].Left;
		m_rects[i].top = pRects[i].Top;
		m_rects[i].right = pRects[i].Right;
		m_rects[i].bottom = pRects[i].Bottom;
	}

	m_commandList->ClearRenderTargetView({ renderTargetView }, colorRGBA,
		numRects, pRects ? m_rects.data() : nullptr);
}

void CommandList_DX12::ClearUnorderedAccessViewUint(const DescriptorTable& descriptorTable, const Descriptor& descriptor,
	const Resource* pResource, const uint32_t values[4], uint32_t numRects, const RectRange* pRects)
{
	assert(pResource);
	assert(numRects == 0 || pRects);

	if (m_rects.size() < numRects) m_rects.resize(numRects);
	for (auto i = 0u; i < numRects; ++i)
	{
		m_rects[i].left = pRects[i].Left;
		m_rects[i].top = pRects[i].Top;
		m_rects[i].right = pRects[i].Right;
		m_rects[i].bottom = pRects[i].Bottom;
	}

	m_commandList->ClearUnorderedAccessViewUint({ *descriptorTable }, { descriptor },
		static_cast<ID3D12Resource*>(pResource->GetHandle()), values,
		numRects, numRects ? m_rects.data() : nullptr);
}

void CommandList_DX12::ClearUnorderedAccessViewFloat(const DescriptorTable& descriptorTable, const Descriptor& descriptor,
	const Resource* pResource, const float values[4], uint32_t numRects, const RectRange* pRects)
{
	assert(pResource);
	assert(numRects == 0 || pRects);

	if (m_rects.size() < numRects) m_rects.resize(numRects);
	for (auto i = 0u; i < numRects; ++i)
	{
		m_rects[i].left = pRects[i].Left;
		m_rects[i].top = pRects[i].Top;
		m_rects[i].right = pRects[i].Right;
		m_rects[i].bottom = pRects[i].Bottom;
	}

	m_commandList->ClearUnorderedAccessViewFloat({ *descriptorTable }, { descriptor },
		static_cast<ID3D12Resource*>(pResource->GetHandle()), values,
		numRects, numRects ? m_rects.data() : nullptr);
}

void CommandList_DX12::DiscardResource(const Resource* pResource, uint32_t numRects, const RectRange* pRects,
	uint32_t firstSubresource, uint32_t numSubresources)
{
	assert(pResource);
	assert(numRects == 0 || pRects);

	if (m_rects.size() < numRects) m_rects.resize(numRects);
	for (auto i = 0u; i < numRects; ++i)
	{
		m_rects[i].left = pRects[i].Left;
		m_rects[i].top = pRects[i].Top;
		m_rects[i].right = pRects[i].Right;
		m_rects[i].bottom = pRects[i].Bottom;
	}

	const D3D12_DISCARD_REGION region =
	{ numRects, numRects ? m_rects.data() : nullptr, firstSubresource, numSubresources };
	m_commandList->DiscardResource(static_cast<ID3D12Resource*>(pResource->GetHandle()), &region);
}

void CommandList_DX12::BeginQuery(const QueryHeap& queryHeap, QueryType type, uint32_t index) const
{
	m_commandList->BeginQuery(static_cast<ID3D12QueryHeap*>(queryHeap), GetDX12QueryType(type), index);
}

void CommandList_DX12::EndQuery(const QueryHeap& queryHeap, QueryType type, uint32_t index) const
{
	m_commandList->EndQuery(static_cast<ID3D12QueryHeap*>(queryHeap), GetDX12QueryType(type), index);
}

void CommandList_DX12::ResolveQueryData(const QueryHeap& queryHeap, QueryType type, uint32_t startIndex,
	uint32_t numQueries, const Resource* pDstBuffer, uint64_t alignedDstBufferOffset) const
{
	assert(pDstBuffer);
	m_commandList->ResolveQueryData(static_cast<ID3D12QueryHeap*>(queryHeap), GetDX12QueryType(type),
		startIndex, numQueries, static_cast<ID3D12Resource*>(pDstBuffer->GetHandle()), alignedDstBufferOffset);
}

void CommandList_DX12::SetPredication(const Resource* pBuffer, uint64_t alignedBufferOffset, bool opEqualZero) const
{
	assert(pBuffer);
	m_commandList->SetPredication(static_cast<ID3D12Resource*>(pBuffer->GetHandle()), alignedBufferOffset,
		opEqualZero ? D3D12_PREDICATION_OP_EQUAL_ZERO : D3D12_PREDICATION_OP_NOT_EQUAL_ZERO);
}

void CommandList_DX12::SetMarker(uint32_t metaData, const void* pData, uint32_t size) const
{
	m_commandList->SetMarker(metaData, pData, size);
}

void CommandList_DX12::BeginEvent(uint32_t metaData, const void* pData, uint32_t size) const
{
	m_commandList->BeginEvent(metaData, pData, size);
}

void CommandList_DX12::EndEvent()
{
	m_commandList->EndEvent();
}

void CommandList_DX12::ExecuteIndirect(const CommandLayout* pCommandlayout, uint32_t maxCommandCount,
	const Resource* pArgumentBuffer, uint64_t argumentBufferOffset,
	const Resource* pCountBuffer, uint64_t countBufferOffset)
{
	assert(pCommandlayout);
	assert(pArgumentBuffer);
	m_commandList->ExecuteIndirect(static_cast<ID3D12CommandSignature*>(pCommandlayout->GetHandle()),
		maxCommandCount, static_cast<ID3D12Resource*>(pArgumentBuffer->GetHandle()), argumentBufferOffset,
		pCountBuffer ? static_cast<ID3D12Resource*>(pCountBuffer->GetHandle()) : nullptr,
		countBufferOffset);
}

void CommandList_DX12::Create(void* pHandle, const wchar_t* name)
{
	m_commandList = pHandle;
	if (name) m_commandList->SetName(name);
}

void* CommandList_DX12::GetHandle() const
{
	return m_commandList.get();
}

void* CommandList_DX12::GetDeviceHandle() const
{
	com_ptr<ID3D12Device> device;
	V_RETURN(m_commandList->GetDevice(IID_PPV_ARGS(&device)), cerr, nullptr);

	return device.get();
}

const Device* CommandList_DX12::GetDevice() const
{
	return m_pDevice;
}

com_ptr<ID3D12GraphicsCommandList>& CommandList_DX12::GetGraphicsCommandList()
{
	return m_commandList;
}

//--------------------------------------------------------------------------------------

CommandQueue_DX12::CommandQueue_DX12()
{
}

CommandQueue_DX12::~CommandQueue_DX12()
{
}

bool CommandQueue_DX12::Create(const Device* pDevice, CommandListType type,
	CommandQueueFlag flags, int32_t priority, uint32_t nodeMask, const wchar_t* name)
{
	D3D12_COMMAND_QUEUE_DESC queueDesc;
	queueDesc.Type = GetDX12CommandListType(type);
	queueDesc.Priority = priority;
	queueDesc.Flags = GetDX12CommandQueueFlags(flags);
	queueDesc.NodeMask = nodeMask;

	const auto pDxDevice = static_cast<ID3D12Device*>(pDevice->GetHandle());

	assert(pDxDevice);
	V_RETURN(pDxDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)), cerr, false);
	if (name) m_commandQueue->SetName(name);

	m_pDevice = pDevice;

	return true;
}

bool CommandQueue_DX12::SubmitCommandLists(uint32_t numCommandLists, const CommandList* const* ppCommandLists,
	const Semaphore* pWaits, uint32_t numWaits, const Semaphore* pSignals, uint32_t numSignals)
{
	for (auto i = 0u; i < numWaits; ++i)
		XUSG_N_RETURN(Wait(pWaits[i].Fence.get(), pWaits[i].Value), false);
	ExecuteCommandLists(numCommandLists, ppCommandLists);
	for (auto i = 0u; i < numSignals; ++i)
		XUSG_N_RETURN(Signal(pSignals[i].Fence.get(), pSignals[i].Value), false);

	return true;
}

bool CommandQueue_DX12::SubmitCommandList(const CommandList* pCommandList,
	const Semaphore* pWaits, uint32_t numWaits, const Semaphore* pSignals, uint32_t numSignals)
{
	for (auto i = 0u; i < numWaits; ++i)
		XUSG_N_RETURN(Wait(pWaits[i].Fence.get(), pWaits[i].Value), false);
	ExecuteCommandList(pCommandList);
	for (auto i = 0u; i < numSignals; ++i)
		XUSG_N_RETURN(Signal(pSignals[i].Fence.get(), pSignals[i].Value), false);

	return true;
}

void CommandQueue_DX12::ExecuteCommandLists(uint32_t numCommandLists, const CommandList* const* ppCommandLists)
{
	if (m_pCommandLists.size() < numCommandLists) m_pCommandLists.resize(numCommandLists);
	for (auto i = 0u; i < numCommandLists; ++i)
		m_pCommandLists[i] = static_cast<ID3D12CommandList*>(ppCommandLists[i]->GetHandle());

	m_commandQueue->ExecuteCommandLists(numCommandLists, m_pCommandLists.data());
}

void CommandQueue_DX12::ExecuteCommandList(const CommandList* pCommandList)
{
	const auto pDxCommandList = static_cast<ID3D12CommandList*>(pCommandList->GetHandle());

	m_commandQueue->ExecuteCommandLists(1, &pDxCommandList);
}

bool CommandQueue_DX12::Wait(const Fence* pFence, uint64_t value)
{
	V_RETURN(m_commandQueue->Wait(static_cast<ID3D12Fence*>(pFence->GetHandle()), value), cerr, false);

	return true;
}

bool CommandQueue_DX12::Signal(const Fence* pFence, uint64_t value)
{
	V_RETURN(m_commandQueue->Signal(static_cast<ID3D12Fence*>(pFence->GetHandle()), value), cerr, false);

	return true;
}

void CommandQueue_DX12::Create(void* pHandle, const wchar_t* name)
{
	m_commandQueue = pHandle;
	if (name) m_commandQueue->SetName(name);
}

void* CommandQueue_DX12::GetHandle() const
{
	return m_commandQueue.get();
}

void* CommandQueue_DX12::GetDeviceHandle() const
{
	com_ptr<ID3D12Device> device;
	V_RETURN(m_commandQueue->GetDevice(IID_PPV_ARGS(&device)), cerr, nullptr);

	return device.get();
}

const Device* CommandQueue_DX12::GetDevice() const
{
	return m_pDevice;
}

com_ptr<ID3D12CommandQueue>& CommandQueue_DX12::GetCommandQueue()
{
	return m_commandQueue;
}
