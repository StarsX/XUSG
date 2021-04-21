//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

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

bool CommandAllocator_DX12::Create(const Device& device, CommandListType type, const wchar_t* name)
{
	const auto pDevice = static_cast<ID3D12Device*>(device.GetHandle());

	assert(pDevice);
	V_RETURN(pDevice->CreateCommandAllocator(GetDX12CommandListType(type),
		IID_PPV_ARGS(&m_commandAllocator)), cerr, false);
	if (name) m_commandAllocator->SetName(name);

	return true;
}

bool CommandAllocator_DX12::Reset()
{
	V_RETURN(m_commandAllocator->Reset(), cerr, false);

	return true;
}

void* CommandAllocator_DX12::GetHandle() const
{
	return m_commandAllocator.get();
}

//--------------------------------------------------------------------------------------

CommandList_DX12::CommandList_DX12()
{
}

CommandList_DX12::~CommandList_DX12()
{
}

bool CommandList_DX12::Create(const Device& device, uint32_t nodeMask, CommandListType type,
	const CommandAllocator& commandAllocator, const Pipeline& pipeline, const wchar_t* name)
{
	const auto pDevice = static_cast<ID3D12Device*>(device.GetHandle());
	const auto pCommandAllocator = static_cast<ID3D12CommandAllocator*>(commandAllocator.GetHandle());

	assert(pDevice);
	V_RETURN(pDevice->CreateCommandList(nodeMask, GetDX12CommandListType(type), pCommandAllocator,
		static_cast<ID3D12PipelineState*>(pipeline), IID_PPV_ARGS(&m_commandList)), cerr, false);
	if (name) m_commandList->SetName(name);

	return true;
}

bool CommandList_DX12::Close() const
{
	V_RETURN(m_commandList->Close(), cerr, false);

	return true;
}

bool CommandList_DX12::Reset(const CommandAllocator& allocator, const Pipeline& initialState) const
{
	V_RETURN(m_commandList->Reset(static_cast<ID3D12CommandAllocator*>(allocator.GetHandle()),
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

void CommandList_DX12::CopyBufferRegion(const Resource& dstBuffer, uint64_t dstOffset,
	const Resource& srcBuffer, uint64_t srcOffset, uint64_t numBytes) const
{
	m_commandList->CopyBufferRegion(static_cast<ID3D12Resource*>(dstBuffer.GetHandle()), dstOffset,
		static_cast<ID3D12Resource*>(srcBuffer.GetHandle()), srcOffset, numBytes);
}

void CommandList_DX12::CopyTextureRegion(const TextureCopyLocation& dst,
	uint32_t dstX, uint32_t dstY, uint32_t dstZ, const TextureCopyLocation& src,
	const BoxRange* pSrcBox) const
{
	const CD3DX12_TEXTURE_COPY_LOCATION dstCopy(static_cast<ID3D12Resource*>(dst.pResource->GetHandle()), dst.SubresourceIndex);
	const CD3DX12_TEXTURE_COPY_LOCATION srcCopy(static_cast<ID3D12Resource*>(src.pResource->GetHandle()), src.SubresourceIndex);
	m_commandList->CopyTextureRegion(&dstCopy, dstX, dstY, dstZ, &srcCopy, reinterpret_cast<const D3D12_BOX*>(pSrcBox));
}

void CommandList_DX12::CopyResource(const Resource& dstResource, const Resource& srcResource) const
{
	m_commandList->CopyResource(static_cast<ID3D12Resource*>(dstResource.GetHandle()),
		static_cast<ID3D12Resource*>(srcResource.GetHandle()));
}

void CommandList_DX12::CopyTiles(const Resource& tiledResource, const TiledResourceCoord* pTileRegionStartCoord,
	const TileRegionSize* pTileRegionSize, const Resource& buffer, uint64_t bufferStartOffsetInBytes,
	TileCopyFlags flags) const
{
	m_commandList->CopyTiles(static_cast<ID3D12Resource*>(tiledResource.GetHandle()),
		reinterpret_cast<const D3D12_TILED_RESOURCE_COORDINATE*>(pTileRegionStartCoord),
		reinterpret_cast<const D3D12_TILE_REGION_SIZE*>(pTileRegionSize),
		static_cast<ID3D12Resource*>(buffer.GetHandle()), bufferStartOffsetInBytes, flags);
}

void CommandList_DX12::ResolveSubresource(const Resource& dstResource, uint32_t dstSubresource,
	const Resource& srcResource, uint32_t srcSubresource, Format format) const
{
	m_commandList->ResolveSubresource(static_cast<ID3D12Resource*>(dstResource.GetHandle()), dstSubresource,
		static_cast<ID3D12Resource*>(srcResource.GetHandle()), srcSubresource, GetDXGIFormat(format));
}

void CommandList_DX12::IASetPrimitiveTopology(PrimitiveTopology primitiveTopology) const
{
	m_commandList->IASetPrimitiveTopology(GetDX12PrimitiveTopology(primitiveTopology));
}

void CommandList_DX12::RSSetViewports(uint32_t numViewports, const Viewport* pViewports) const
{
	m_commandList->RSSetViewports(numViewports, reinterpret_cast<const D3D12_VIEWPORT*>(pViewports));
}

void CommandList_DX12::RSSetScissorRects(uint32_t numRects, const RectRange* pRects) const
{
	m_commandList->RSSetScissorRects(numRects, reinterpret_cast<const D3D12_RECT*>(pRects));
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

void CommandList_DX12::Barrier(uint32_t numBarriers, const ResourceBarrier* pBarriers) const
{
	if (numBarriers > 0)
	{
		vector<D3D12_RESOURCE_BARRIER> barriers(numBarriers);
		for (auto i = 0u; i < numBarriers; ++i)
		{
			const auto& barrier = pBarriers[i];
			const auto resource = static_cast<ID3D12Resource*>(barrier.pResource->GetHandle());
			barriers[i] = barrier.StateBefore == barrier.StateAfter && barrier.StateAfter == ResourceState::UNORDERED_ACCESS ?
				CD3DX12_RESOURCE_BARRIER::UAV(resource) :
				CD3DX12_RESOURCE_BARRIER::Transition(resource, GetDX12ResourceStates(barrier.StateBefore),
					GetDX12ResourceStates(barrier.StateAfter), barrier.Subresource, GetDX12BarrierFlags(barrier.Flags));
		}
		m_commandList->ResourceBarrier(numBarriers, barriers.data());
	}
}

void CommandList_DX12::ExecuteBundle(CommandList& commandList) const
{
	m_commandList->ExecuteBundle(dynamic_cast<CommandList_DX12&>(commandList).GetGraphicsCommandList().get());
}

void CommandList_DX12::SetDescriptorPools(uint32_t numDescriptorPools, const DescriptorPool* pDescriptorPools) const
{
	m_commandList->SetDescriptorHeaps(numDescriptorPools, reinterpret_cast<ID3D12DescriptorHeap* const*>(pDescriptorPools));
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

void CommandList_DX12::SetComputeRootConstantBufferView(uint32_t index, const Resource& resource, int offset) const
{
	m_commandList->SetComputeRootConstantBufferView(index, dynamic_cast<const Resource_DX12&>(resource).GetGPUVirtualAddress(offset));
}

void CommandList_DX12::SetGraphicsRootConstantBufferView(uint32_t index, const Resource& resource, int offset) const
{
	m_commandList->SetGraphicsRootConstantBufferView(index, dynamic_cast<const Resource_DX12&>(resource).GetGPUVirtualAddress(offset));
}

void CommandList_DX12::SetComputeRootShaderResourceView(uint32_t index, const Resource& resource, int offset) const
{
	m_commandList->SetComputeRootShaderResourceView(index, dynamic_cast<const Resource_DX12&>(resource).GetGPUVirtualAddress(offset));
}

void CommandList_DX12::SetGraphicsRootShaderResourceView(uint32_t index, const Resource& resource, int offset) const
{
	m_commandList->SetGraphicsRootShaderResourceView(index, dynamic_cast<const Resource_DX12&>(resource).GetGPUVirtualAddress(offset));
}

void CommandList_DX12::SetComputeRootUnorderedAccessView(uint32_t index, const Resource& resource, int offset) const
{
	m_commandList->SetComputeRootUnorderedAccessView(index, dynamic_cast<const Resource_DX12&>(resource).GetGPUVirtualAddress(offset));
}

void CommandList_DX12::SetGraphicsRootUnorderedAccessView(uint32_t index, const Resource& resource, int offset) const
{
	m_commandList->SetGraphicsRootUnorderedAccessView(index, dynamic_cast<const Resource_DX12&>(resource).GetGPUVirtualAddress(offset));
}

void CommandList_DX12::IASetIndexBuffer(const IndexBufferView& view) const
{
	D3D12_INDEX_BUFFER_VIEW ibv = { view.BufferLocation, view.SizeInBytes, GetDXGIFormat(view.ViewFormat) };
	m_commandList->IASetIndexBuffer(&ibv);
}

void CommandList_DX12::IASetVertexBuffers(uint32_t startSlot, uint32_t numViews, const VertexBufferView* pViews) const
{
	m_commandList->IASetVertexBuffers(startSlot, numViews, reinterpret_cast<const D3D12_VERTEX_BUFFER_VIEW*>(pViews));
}

void CommandList_DX12::SOSetTargets(uint32_t startSlot, uint32_t numViews, const StreamOutBufferView* pViews) const
{
	m_commandList->SOSetTargets(startSlot, numViews, reinterpret_cast<const D3D12_STREAM_OUTPUT_BUFFER_VIEW*>(pViews));
}

void CommandList_DX12::OMSetFramebuffer(const Framebuffer& framebuffer) const
{
	m_commandList->OMSetRenderTargets(framebuffer.NumRenderTargetDescriptors,
		reinterpret_cast<const D3D12_CPU_DESCRIPTOR_HANDLE*>(framebuffer.RenderTargetViews.get()), true,
		framebuffer.DepthStencilView ? reinterpret_cast<const D3D12_CPU_DESCRIPTOR_HANDLE*>(&framebuffer.DepthStencilView) : nullptr);
}

void CommandList_DX12::OMSetRenderTargets(uint32_t numRenderTargetDescriptors, const Descriptor* pRenderTargetViews,
	const Descriptor* pDepthStencilView, bool rtsSingleHandleToDescriptorRange) const
{
	m_commandList->OMSetRenderTargets(numRenderTargetDescriptors,
		reinterpret_cast<const D3D12_CPU_DESCRIPTOR_HANDLE*>(pRenderTargetViews), rtsSingleHandleToDescriptorRange,
		reinterpret_cast<const D3D12_CPU_DESCRIPTOR_HANDLE*>(pDepthStencilView));
}

void CommandList_DX12::ClearDepthStencilView(const Framebuffer& framebuffer, ClearFlag clearFlags, float depth,
	uint8_t stencil, uint32_t numRects, const RectRange* pRects) const
{
	ClearDepthStencilView(framebuffer.DepthStencilView, clearFlags, depth, stencil, numRects, pRects);
}

void CommandList_DX12::ClearDepthStencilView(const Descriptor& depthStencilView, ClearFlag clearFlags, float depth,
	uint8_t stencil, uint32_t numRects, const RectRange* pRects) const
{
	m_commandList->ClearDepthStencilView({ depthStencilView }, GetDX12ClearFlags(clearFlags),
		depth, stencil, numRects, reinterpret_cast<const D3D12_RECT*>(pRects));
}

void CommandList_DX12::ClearRenderTargetView(const Descriptor& renderTargetView, const float colorRGBA[4],
	uint32_t numRects, const RectRange* pRects) const
{
	m_commandList->ClearRenderTargetView({ renderTargetView }, colorRGBA, numRects, reinterpret_cast<const D3D12_RECT*>(pRects));
}

void CommandList_DX12::ClearUnorderedAccessViewUint(const DescriptorTable& descriptorTable, const Descriptor& descriptor,
	const Resource& resource, const uint32_t values[4], uint32_t numRects, const RectRange* pRects) const
{
	m_commandList->ClearUnorderedAccessViewUint({ *descriptorTable }, { descriptor },
		static_cast<ID3D12Resource*>(resource.GetHandle()), values,
		numRects, reinterpret_cast<const D3D12_RECT*>(pRects));
}

void CommandList_DX12::ClearUnorderedAccessViewFloat(const DescriptorTable& descriptorTable, const Descriptor& descriptor,
	const Resource& resource, const float values[4], uint32_t numRects, const RectRange* pRects) const
{
	m_commandList->ClearUnorderedAccessViewFloat({ *descriptorTable }, { descriptor },
		static_cast<ID3D12Resource*>(resource.GetHandle()), values,
		numRects, reinterpret_cast<const D3D12_RECT*>(pRects));
}

void CommandList_DX12::DiscardResource(const Resource& resource, uint32_t numRects, const RectRange* pRects,
	uint32_t firstSubresource, uint32_t numSubresources) const
{
	const D3D12_DISCARD_REGION region =
	{ numRects, reinterpret_cast<const D3D12_RECT*>(pRects), firstSubresource, numSubresources };
	m_commandList->DiscardResource(static_cast<ID3D12Resource*>(resource.GetHandle()), &region);
}

void CommandList_DX12::BeginQuery(const QueryPool& queryPool, QueryType type, uint32_t index) const
{
	m_commandList->BeginQuery(static_cast<ID3D12QueryHeap*>(queryPool), GetDX12QueryType(type), index);
}

void CommandList_DX12::EndQuery(const QueryPool& queryPool, QueryType type, uint32_t index) const
{
	m_commandList->EndQuery(static_cast<ID3D12QueryHeap*>(queryPool), GetDX12QueryType(type), index);
}

void CommandList_DX12::ResolveQueryData(const QueryPool& queryPool, QueryType type, uint32_t startIndex,
	uint32_t numQueries, const Resource& dstBuffer, uint64_t alignedDstBufferOffset) const
{
	m_commandList->ResolveQueryData(static_cast<ID3D12QueryHeap*>(queryPool), GetDX12QueryType(type),
		startIndex, numQueries, static_cast<ID3D12Resource*>(dstBuffer.GetHandle()), alignedDstBufferOffset);
}

void CommandList_DX12::SetPredication(const Resource& buffer, uint64_t alignedBufferOffset, bool opEqualZero) const
{
	m_commandList->SetPredication(static_cast<ID3D12Resource*>(buffer.GetHandle()), alignedBufferOffset,
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

void CommandList_DX12::ExecuteIndirect(const CommandLayout& commandlayout, uint32_t maxCommandCount,
	const Resource& argumentBuffer, uint64_t argumentBufferOffset,
	const Resource* pCountBuffer, uint64_t countBufferOffset)
{
	m_commandList->ExecuteIndirect(static_cast<ID3D12CommandSignature*>(commandlayout.GetHandle()),
		maxCommandCount, static_cast<ID3D12Resource*>(argumentBuffer.GetHandle()), argumentBufferOffset,
		pCountBuffer ? static_cast<ID3D12Resource*>(pCountBuffer->GetHandle()) : nullptr,
		countBufferOffset);
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

bool CommandQueue_DX12::Create(const Device& device, CommandListType type,
	CommandQueueFlag flags, int32_t priority, uint32_t nodeMask, const wchar_t* name)
{
	D3D12_COMMAND_QUEUE_DESC queueDesc;
	queueDesc.Type = GetDX12CommandListType(type);
	queueDesc.Priority = priority;
	queueDesc.Flags = GetDX12CommandQueueFlags(flags);
	queueDesc.NodeMask = nodeMask;

	const auto pDevice = static_cast<ID3D12Device*>(device.GetHandle());

	assert(pDevice);
	V_RETURN(pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)), cerr, false);
	if (name) m_commandQueue->SetName(name);

	return true;
}

bool CommandQueue_DX12::SubmitCommandLists(uint32_t numCommandLists, CommandList* const* ppCommandLists,
	const Semaphore* pWaits, uint32_t numWaits, const Semaphore* pSignals, uint32_t numSignals)
{
	for (auto i = 0u; i < numWaits; ++i)
		N_RETURN(Wait(*pWaits[i].Fence, pWaits[i].Value), false);
	ExecuteCommandLists(numCommandLists, ppCommandLists);
	for (auto i = 0u; i < numSignals; ++i)
		N_RETURN(Signal(*pSignals[i].Fence, pSignals[i].Value), false);

	return true;
}

bool CommandQueue_DX12::SubmitCommandList(CommandList* const pCommandList,
	const Semaphore* pWaits, uint32_t numWaits, const Semaphore* pSignals, uint32_t numSignals)
{
	for (auto i = 0u; i < numWaits; ++i)
		N_RETURN(Wait(*pWaits[i].Fence, pWaits[i].Value), false);
	ExecuteCommandList(pCommandList);
	for (auto i = 0u; i < numSignals; ++i)
		N_RETURN(Signal(*pSignals[i].Fence, pSignals[i].Value), false);

	return true;
}

void CommandQueue_DX12::ExecuteCommandLists(uint32_t numCommandLists, CommandList* const* ppCommandLists)
{
	vector<ID3D12CommandList*> commandLists(numCommandLists);
	for (auto i = 0u; i < numCommandLists; ++i)
		commandLists[i] = dynamic_cast<CommandList_DX12*>(ppCommandLists[i])->GetGraphicsCommandList().get();

	m_commandQueue->ExecuteCommandLists(numCommandLists, commandLists.data());
}

void CommandQueue_DX12::ExecuteCommandList(CommandList* const pCommandList)
{
	ID3D12CommandList* const ppCommandLists[] =
	{ dynamic_cast<CommandList_DX12*>(pCommandList)->GetGraphicsCommandList().get() };

	m_commandQueue->ExecuteCommandLists(1, ppCommandLists);
}

bool CommandQueue_DX12::Wait(const Fence& fence, uint64_t value)
{
	V_RETURN(m_commandQueue->Wait(static_cast<ID3D12Fence*>(fence.GetHandle()), value), cerr, false);

	return true;
}

bool CommandQueue_DX12::Signal(const Fence& fence, uint64_t value)
{
	V_RETURN(m_commandQueue->Signal(static_cast<ID3D12Fence*>(fence.GetHandle()), value), cerr, false);

	return true;
}

void* CommandQueue_DX12::GetHandle() const
{
	return m_commandQueue.get();
}

com_ptr<ID3D12CommandQueue>& CommandQueue_DX12::GetCommandQueue()
{
	return m_commandQueue;
}
