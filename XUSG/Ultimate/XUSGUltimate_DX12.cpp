//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSG_DX12.h"
#include "Core/XUSGCommand_DX12.h"
#include "Core/XUSGResource_DX12.h"
#include "Core/XUSGPipelineLayout_DX12.h"
#include "Core/XUSGEnum_DX12.h"
#include "XUSGUltimate_DX12.h"

using namespace std;
using namespace XUSG::Ultimate;

// Dense enumerator values and single-bit flag positions are the indices of these tables.
// Do not reorder or renumber the enums in XUSG.h.

#define APPEND_FLAG(type, dx12Type, flags, flag, none) ((flags & type::flag) == type::flag ? dx12Type##_##flag : dx12Type##_##none)
#define APPEND_BARRIER_SYNC(barrierSyncs, barrierSync) APPEND_FLAG(BarrierSync, D3D12_BARRIER_SYNC, barrierSyncs, barrierSync, NONE)
#define APPEND_BARRIER_ACCESS(barrierAccesses, barrierAccess) APPEND_FLAG(BarrierAccess, D3D12_BARRIER_ACCESS, barrierAccesses, barrierAccess, COMMON)
#define APPEND_TEXTURE_BARRIER_FLAG(flags, flag) APPEND_FLAG(TextureBarrierFlag, D3D12_TEXTURE_BARRIER_FLAG, flags, flag, NONE)
#define APPEND_VIEW_INSTANCE_FLAG(flags, flag) APPEND_FLAG(ViewInstanceFlag, D3D12_VIEW_INSTANCING_FLAG, flags, flag, NONE)
#define APPEND_WORK_GRAPH_FLAG(flags, flag) APPEND_FLAG(WorkGraphFlag, D3D12_SET_WORK_GRAPH_FLAG, flags, flag, NONE)

//--------------------------------------------------------------------------------------
// Command list
//--------------------------------------------------------------------------------------

CommandList_DX12::CommandList_DX12() :
	XUSG::CommandList_DX12()
{
	if (m_commandList)
		m_commandList->QueryInterface(IID_PPV_ARGS(&m_commandListU));
}

CommandList_DX12::CommandList_DX12(XUSG::CommandList& commandList) :
	XUSG::CommandList_DX12()
{
	const auto pCommandList = AsDX12(&commandList);
	assert(pCommandList);

	m_commandList = pCommandList->GetGraphicsCommandList();
	createInterface();
}

CommandList_DX12::~CommandList_DX12()
{
}

bool CommandList_DX12::Create(Device* pDevice, uint32_t nodeMask, CommandListType type,
	uint8_t numAllocators, CommandAllocator* const* ppAllocators, const Pipeline& pipeline, const wchar_t* name)
{
	XUSG_N_RETURN(XUSG::CommandList_DX12::Create(pDevice, nodeMask, type, numAllocators, ppAllocators, pipeline, name), false);

	return createInterface();
}

void CommandList_DX12::Create(void* pNativeHandle, Device* pDevice)
{
	XUSG::CommandList_DX12::Create(pNativeHandle, pDevice);
	createInterface();
}

bool CommandList_DX12::prepareBarrierGroups(uint32_t numBarrierGroups, BarrierGroup* pBarrierGroups)
{
	if (!numBarrierGroups) return false;
	assert(pBarrierGroups);
	if (m_barrierGroups.size() < numBarrierGroups) m_barrierGroups.resize(numBarrierGroups);
	if (m_barrierGroupBarrierStarts.size() < numBarrierGroups) m_barrierGroupBarrierStarts.resize(numBarrierGroups);
	m_globalBarriers.clear();
	m_textureBarriers.clear();
	m_bufferBarriers.clear();

	auto hasBarriers = false;
	for (auto i = 0u; i < numBarrierGroups; ++i)
	{
		const auto& barrierGroup = pBarrierGroups[i];
		auto& dxBarrierGroup = m_barrierGroups[i];
		dxBarrierGroup = {};
		if (!barrierGroup.NumBarriers) continue;
		assert(barrierGroup.pBarriers);
		hasBarriers = true;

		auto& barrierStart = m_barrierGroupBarrierStarts[i];
		dxBarrierGroup.Type = barrierGroup.pBarriers->pResource ?
			(barrierGroup.pBarriers->LayoutBefore == BarrierLayout::UNDEFINED &&
				barrierGroup.pBarriers->LayoutAfter == BarrierLayout::UNDEFINED ?
				D3D12_BARRIER_TYPE_BUFFER : D3D12_BARRIER_TYPE_TEXTURE) :
			D3D12_BARRIER_TYPE_GLOBAL;
		dxBarrierGroup.NumBarriers = barrierGroup.NumBarriers;

		switch (dxBarrierGroup.Type)
		{
		case D3D12_BARRIER_TYPE_GLOBAL:
		{
			barrierStart = static_cast<uint32_t>(m_globalBarriers.size());
			m_globalBarriers.resize(barrierStart + barrierGroup.NumBarriers);

			for (auto j = 0u; j < barrierGroup.NumBarriers; ++j)
			{
				const auto& barrier = barrierGroup.pBarriers[j];
				const auto type = barrier.pResource ?
					(barrier.LayoutBefore == BarrierLayout::UNDEFINED &&
						barrier.LayoutAfter == BarrierLayout::UNDEFINED ?
						D3D12_BARRIER_TYPE_BUFFER : D3D12_BARRIER_TYPE_TEXTURE) :
					D3D12_BARRIER_TYPE_GLOBAL;
				assert(type == D3D12_BARRIER_TYPE_GLOBAL);
				assert(!barrier.pResource);

				auto& globalBarrier = m_globalBarriers[barrierStart + j];
				globalBarrier.SyncBefore = GetDX12BarrierSyncs(barrier.SyncBefore);
				globalBarrier.SyncAfter = GetDX12BarrierSyncs(barrier.SyncAfter);
				globalBarrier.AccessBefore = GetDX12BarrierAccesses(barrier.AccessBefore);
				globalBarrier.AccessAfter = GetDX12BarrierAccesses(barrier.AccessAfter);
			}
			break;
		}
		case D3D12_BARRIER_TYPE_TEXTURE:
		{
			barrierStart = static_cast<uint32_t>(m_textureBarriers.size());
			m_textureBarriers.resize(barrierStart + barrierGroup.NumBarriers);

			for (auto j = 0u; j < barrierGroup.NumBarriers; ++j)
			{
				const auto& barrier = barrierGroup.pBarriers[j];
				const auto type = barrier.pResource ?
					(barrier.LayoutBefore == BarrierLayout::UNDEFINED &&
						barrier.LayoutAfter == BarrierLayout::UNDEFINED ?
						D3D12_BARRIER_TYPE_BUFFER : D3D12_BARRIER_TYPE_TEXTURE) :
					D3D12_BARRIER_TYPE_GLOBAL;
				assert(type == D3D12_BARRIER_TYPE_TEXTURE);
				assert(barrier.pResource);

				auto& textureBarrier = m_textureBarriers[barrierStart + j];
				textureBarrier.SyncBefore = GetDX12BarrierSyncs(barrier.SyncBefore);
				textureBarrier.SyncAfter = GetDX12BarrierSyncs(barrier.SyncAfter);
				textureBarrier.AccessBefore = GetDX12BarrierAccesses(barrier.AccessBefore);
				textureBarrier.AccessAfter = GetDX12BarrierAccesses(barrier.AccessAfter);
				textureBarrier.LayoutBefore = GetDX12BarrierLayout(barrier.LayoutBefore);
				textureBarrier.LayoutAfter = GetDX12BarrierLayout(barrier.LayoutAfter);
				textureBarrier.pResource = reinterpret_cast<ID3D12Resource*>(barrier.pResource->GetNativeHandle());
				textureBarrier.Subresources.IndexOrFirstMipLevel = barrier.IndexOrFirstMipLevel;
				textureBarrier.Subresources.NumMipLevels = barrier.NumMipLevels;
				textureBarrier.Subresources.FirstArraySlice = barrier.FirstArraySlice;
				textureBarrier.Subresources.NumArraySlices = barrier.NumArraySlices;
				textureBarrier.Subresources.FirstPlane = barrier.FirstPlane;
				textureBarrier.Subresources.NumPlanes = barrier.NumPlanes;
				textureBarrier.Flags = GetDX12TextureBarrierFlags(barrier.Flags);
			}
			break;
		}
		case D3D12_BARRIER_TYPE_BUFFER:
		{
			barrierStart = static_cast<uint32_t>(m_bufferBarriers.size());
			m_bufferBarriers.resize(barrierStart + barrierGroup.NumBarriers);

			for (auto j = 0u; j < barrierGroup.NumBarriers; ++j)
			{
				const auto& barrier = barrierGroup.pBarriers[j];
				const auto type = barrier.pResource ?
					(barrier.LayoutBefore == BarrierLayout::UNDEFINED &&
						barrier.LayoutAfter == BarrierLayout::UNDEFINED ?
						D3D12_BARRIER_TYPE_BUFFER : D3D12_BARRIER_TYPE_TEXTURE) :
					D3D12_BARRIER_TYPE_GLOBAL;
				assert(type == D3D12_BARRIER_TYPE_BUFFER);
				assert(barrier.pResource);

				auto& bufferBarrier = m_bufferBarriers[barrierStart + j];
				bufferBarrier.SyncBefore = GetDX12BarrierSyncs(barrier.SyncBefore);
				bufferBarrier.SyncAfter = GetDX12BarrierSyncs(barrier.SyncAfter);
				bufferBarrier.AccessBefore = GetDX12BarrierAccesses(barrier.AccessBefore);
				bufferBarrier.AccessAfter = GetDX12BarrierAccesses(barrier.AccessAfter);
				bufferBarrier.pResource = reinterpret_cast<ID3D12Resource*>(barrier.pResource->GetNativeHandle());
				bufferBarrier.Offset = barrier.Offset;
				bufferBarrier.Size = barrier.Size;
			}
			break;
		}
		}

		switch (dxBarrierGroup.Type)
		{
		case D3D12_BARRIER_TYPE_GLOBAL:
			dxBarrierGroup.pGlobalBarriers = &m_globalBarriers[barrierStart];
			break;
		case D3D12_BARRIER_TYPE_TEXTURE:
			dxBarrierGroup.pTextureBarriers = &m_textureBarriers[barrierStart];
			break;
		case D3D12_BARRIER_TYPE_BUFFER:
			dxBarrierGroup.pBufferBarriers = &m_bufferBarriers[barrierStart];
			break;
		}
	}

	return hasBarriers || numBarrierGroups > 0;
}

void CommandList_DX12::Barrier(uint32_t numBarrierGroups, BarrierGroup* pBarrierGroups)
{
	if (!prepareBarrierGroups(numBarrierGroups, pBarrierGroups)) return;

	createAgilityInterface();
	m_commandListA->Barrier(numBarrierGroups, m_barrierGroups.data());
}

void CommandList_DX12::SignalBarrier(uint32_t numBarrierGroups, BarrierGroup* pBarrierGroups,
	Fence* pFence, uint64_t fenceValue)
{
	assert(pFence);
	prepareBarrierGroups(numBarrierGroups, pBarrierGroups);
	createAgilityInterface();
	m_commandListA->SignalBarrier(numBarrierGroups, numBarrierGroups ? m_barrierGroups.data() : nullptr,
		reinterpret_cast<ID3D12Fence*>(pFence->GetNativeHandle()), fenceValue);
}

void CommandList_DX12::WaitBarrier(uint32_t numBarrierGroups, BarrierGroup* pBarrierGroups,
	uint32_t numFences, Fence* const* ppFences, const uint64_t* pFenceValues)
{
	assert(numFences);
	assert(ppFences);
	assert(pFenceValues);
	if (m_fences.size() < numFences) m_fences.resize(numFences);
	for (auto i = 0u; i < numFences; ++i)
	{
		assert(ppFences[i]);
		m_fences[i] = reinterpret_cast<ID3D12Fence*>(ppFences[i]->GetNativeHandle());
	}

	prepareBarrierGroups(numBarrierGroups, pBarrierGroups);
	createAgilityInterface();
	m_commandListA->WaitBarrier(numBarrierGroups, numBarrierGroups ? m_barrierGroups.data() : nullptr,
		numFences, m_fences.data(), pFenceValues);
}

void CommandList_DX12::Barrier(uint32_t numBufferBarriers, ResourceBarrier* pBufferBarriers,
	uint32_t numTextureBarriers, ResourceBarrier* pTextureBarriers,
	uint32_t numGlobalBarriers, ResourceBarrier* pGlobalBarriers)
{
	m_barrierGroups.clear();

	if (numBufferBarriers)
	{
		assert(pBufferBarriers);
		if (m_bufferBarriers.size() < numBufferBarriers) m_bufferBarriers.resize(numBufferBarriers);

		for (auto i = 0u; i < numBufferBarriers; ++i)
		{
			const auto& barrier = pBufferBarriers[i];
			auto& bufferBarrier = m_bufferBarriers[i];
			bufferBarrier.SyncBefore = GetDX12BarrierSyncs(barrier.SyncBefore);
			bufferBarrier.SyncAfter = GetDX12BarrierSyncs(barrier.SyncAfter);
			bufferBarrier.AccessBefore = GetDX12BarrierAccesses(barrier.AccessBefore);
			bufferBarrier.AccessAfter = GetDX12BarrierAccesses(barrier.AccessAfter);
			bufferBarrier.pResource = reinterpret_cast<ID3D12Resource*>(barrier.pResource->GetNativeHandle());
			bufferBarrier.Offset = barrier.Offset;
			bufferBarrier.Size = barrier.Size;
		}

		m_barrierGroups.emplace_back();
		auto& barrierGroup = m_barrierGroups.back();
		barrierGroup.Type = D3D12_BARRIER_TYPE_BUFFER;
		barrierGroup.NumBarriers = numBufferBarriers;
		barrierGroup.pBufferBarriers = m_bufferBarriers.data();
	}

	if (numTextureBarriers)
	{
		assert(pTextureBarriers);
		if (m_textureBarriers.size() < numTextureBarriers) m_textureBarriers.resize(numTextureBarriers);

		for (auto i = 0u; i < numTextureBarriers; ++i)
		{
			const auto& barrier = pTextureBarriers[i];
			auto& textureBarrier = m_textureBarriers[i];
			textureBarrier.SyncBefore = GetDX12BarrierSyncs(barrier.SyncBefore);
			textureBarrier.SyncAfter = GetDX12BarrierSyncs(barrier.SyncAfter);
			textureBarrier.AccessBefore = GetDX12BarrierAccesses(barrier.AccessBefore);
			textureBarrier.AccessAfter = GetDX12BarrierAccesses(barrier.AccessAfter);
			textureBarrier.LayoutBefore = GetDX12BarrierLayout(barrier.LayoutBefore);
			textureBarrier.LayoutAfter = GetDX12BarrierLayout(barrier.LayoutAfter);
			textureBarrier.pResource = reinterpret_cast<ID3D12Resource*>(barrier.pResource->GetNativeHandle());
			textureBarrier.Subresources.IndexOrFirstMipLevel = barrier.IndexOrFirstMipLevel;
			textureBarrier.Subresources.NumMipLevels = barrier.NumMipLevels;
			textureBarrier.Subresources.FirstArraySlice = barrier.FirstArraySlice;
			textureBarrier.Subresources.NumArraySlices = barrier.NumArraySlices;
			textureBarrier.Subresources.FirstPlane = barrier.FirstPlane;
			textureBarrier.Subresources.NumPlanes = barrier.NumPlanes;
			textureBarrier.Flags = GetDX12TextureBarrierFlags(barrier.Flags);
		}

		m_barrierGroups.emplace_back();
		auto& barrierGroup = m_barrierGroups.back();
		barrierGroup.Type = D3D12_BARRIER_TYPE_TEXTURE;
		barrierGroup.NumBarriers = numBufferBarriers;
		barrierGroup.pTextureBarriers = m_textureBarriers.data();
	}

	if (numGlobalBarriers)
	{
		assert(pGlobalBarriers);
		if (m_globalBarriers.size() < numGlobalBarriers) m_globalBarriers.resize(numGlobalBarriers);

		for (auto i = 0u; i < numGlobalBarriers; ++i)
		{
			const auto& barrier = pGlobalBarriers[i];
			auto& globalBarrier = m_globalBarriers[i];
			globalBarrier.SyncBefore = GetDX12BarrierSyncs(barrier.SyncBefore);
			globalBarrier.SyncAfter = GetDX12BarrierSyncs(barrier.SyncAfter);
			globalBarrier.AccessBefore = GetDX12BarrierAccesses(barrier.AccessBefore);
			globalBarrier.AccessAfter = GetDX12BarrierAccesses(barrier.AccessAfter);
		}

		m_barrierGroups.emplace_back();
		auto& barrierGroup = m_barrierGroups.back();
		barrierGroup.Type = D3D12_BARRIER_TYPE_GLOBAL;
		barrierGroup.NumBarriers = numBufferBarriers;
		barrierGroup.pGlobalBarriers = m_globalBarriers.data();
	}

	if (!m_barrierGroups.empty())
		m_commandListA->Barrier(static_cast<uint32_t>(m_barrierGroups.size()), m_barrierGroups.data());
}

void CommandList_DX12::SetSamplePositions(uint8_t numSamplesPerPixel, uint8_t numPixels, SamplePosition* pPositions) const
{
	assert(
		numSamplesPerPixel == 0 ||
		numSamplesPerPixel == 1 ||
		numSamplesPerPixel == 2 ||
		numSamplesPerPixel == 4 ||
		numSamplesPerPixel == 8 ||
		numSamplesPerPixel == 16);
	assert(numPixels == 0 || numPixels == 1 || numPixels == 4);

	const uint8_t numSamples = numSamplesPerPixel * numPixels;

	assert(numSamples == 0 || pPositions);
	assert(numSamples <= 16);

	D3D12_SAMPLE_POSITION positions[16];
	if (pPositions)
	{
		for (uint8_t i = 0; i < numSamples; ++i)
		{
			positions[i].X = pPositions[i].X;
			positions[i].Y = pPositions[i].Y;
		}
	}

	m_commandListU->SetSamplePositions(numSamplesPerPixel, numPixels, pPositions ? positions : nullptr);
}

void CommandList_DX12::ResolveSubresourceRegion(Resource* pDstResource, uint32_t dstSubresource, uint32_t dstX, uint32_t dstY,
	Resource* pSrcResource, uint32_t srcSubresource, const RectRange& srcRect, Format format, ResolveMode resolveMode) const
{
	assert(pDstResource);
	assert(pSrcResource);

	D3D12_RECT rect = { srcRect.Left, srcRect.Top, srcRect.Right, srcRect.Bottom };

	m_commandListU->ResolveSubresourceRegion(reinterpret_cast<ID3D12Resource*>(pDstResource->GetNativeHandle()),
		dstSubresource, dstX, dstY, reinterpret_cast<ID3D12Resource*>(pSrcResource->GetNativeHandle()), srcSubresource,
		&rect, GetDXGIFormat(format), GetDX12ResolveMode(resolveMode));
}

void CommandList_DX12::RSSetShadingRate(ShadingRate baseShadingRate, const ShadingRateCombiner* pCombiners) const
{
	D3D12_SHADING_RATE_COMBINER combiners[D3D12_RS_SET_SHADING_RATE_COMBINER_COUNT];
	if (pCombiners)
		for (uint8_t i = 0; i < D3D12_RS_SET_SHADING_RATE_COMBINER_COUNT; ++i)
			combiners[i] = GetDX12ShadingRateCombiner(pCombiners[i]);

	m_commandListU->RSSetShadingRate(D3D12_SHADING_RATE(baseShadingRate), pCombiners ? combiners : nullptr);
}

void CommandList_DX12::RSSetShadingRateImage(Resource* pShadingRateImage)  const
{
	m_commandListU->RSSetShadingRateImage(pShadingRateImage ?
		reinterpret_cast<ID3D12Resource*>(pShadingRateImage->GetNativeHandle()) : nullptr);
}

void CommandList_DX12::DispatchMesh(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)  const
{
	m_commandListU->DispatchMesh(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}

void CommandList_DX12::SetStateObject(const Pipeline& stateObject) const
{
	m_commandListU->SetPipelineState1(static_cast<ID3D12StateObject*>(stateObject));
}

void CommandList_DX12::SetProgram(ProgramType type, ProgramIdentifier identifier, WorkGraphFlag flags,
	uint64_t backingMemoryAddress, uint64_t backingMemoryByteSize, uint64_t localRootArgTableAddress,
	uint64_t localRootArgTableByteSize, uint64_t localRootArgTableByteStride)
{
	D3D12_SET_PROGRAM_DESC desc = {};

	switch (type)
	{
	case ProgramType::RAYTRACING_PIPELINE:
		desc.Type = D3D12_PROGRAM_TYPE_RAYTRACING_PIPELINE;
		desc.RaytracingPipeline.ProgramIdentifier.OpaqueData[0] = identifier.OpaqueData[0];
		desc.RaytracingPipeline.ProgramIdentifier.OpaqueData[1] = identifier.OpaqueData[1];
		desc.RaytracingPipeline.ProgramIdentifier.OpaqueData[2] = identifier.OpaqueData[2];
		desc.RaytracingPipeline.ProgramIdentifier.OpaqueData[3] = identifier.OpaqueData[3];
		break;
	case ProgramType::WORK_GRAPH:
		desc.Type = D3D12_PROGRAM_TYPE_WORK_GRAPH;
		desc.WorkGraph.ProgramIdentifier.OpaqueData[0] = identifier.OpaqueData[0];
		desc.WorkGraph.ProgramIdentifier.OpaqueData[1] = identifier.OpaqueData[1];
		desc.WorkGraph.ProgramIdentifier.OpaqueData[2] = identifier.OpaqueData[2];
		desc.WorkGraph.ProgramIdentifier.OpaqueData[3] = identifier.OpaqueData[3];
		desc.WorkGraph.Flags = GetDX12WorkGraphFlags(flags);
		desc.WorkGraph.BackingMemory.StartAddress = backingMemoryAddress;
		desc.WorkGraph.BackingMemory.SizeInBytes = backingMemoryByteSize;
		desc.WorkGraph.NodeLocalRootArgumentsTable.StartAddress = localRootArgTableAddress;
		desc.WorkGraph.NodeLocalRootArgumentsTable.SizeInBytes = localRootArgTableByteSize;
		desc.WorkGraph.NodeLocalRootArgumentsTable.StrideInBytes = localRootArgTableByteStride;
		break;
	case ProgramType::MLIR_PROGRAM:
		desc.Type = D3D12_PROGRAM_TYPE_MLIR_PROGRAM;
		desc.MlirProgram.ProgramIdentifier.OpaqueData[0] = identifier.OpaqueData[0];
		desc.MlirProgram.ProgramIdentifier.OpaqueData[1] = identifier.OpaqueData[1];
		desc.MlirProgram.ProgramIdentifier.OpaqueData[2] = identifier.OpaqueData[2];
		desc.MlirProgram.ProgramIdentifier.OpaqueData[3] = identifier.OpaqueData[3];
		desc.MlirProgram.Flags = D3D12_SET_MLIR_PROGRAM_FLAG_NONE;
		break;
	default:
		assert(type == ProgramType::GENERIC_PIPELINE);
		desc.Type = D3D12_PROGRAM_TYPE_GENERIC_PIPELINE;
		desc.GenericPipeline.ProgramIdentifier.OpaqueData[0] = identifier.OpaqueData[0];
		desc.GenericPipeline.ProgramIdentifier.OpaqueData[1] = identifier.OpaqueData[1];
		desc.GenericPipeline.ProgramIdentifier.OpaqueData[2] = identifier.OpaqueData[2];
		desc.GenericPipeline.ProgramIdentifier.OpaqueData[3] = identifier.OpaqueData[3];
	}

	createAgilityInterface();
	m_commandListA->SetProgram(&desc);
}

void CommandList_DX12::DispatchGraph(uint32_t numNodeInputs, const NodeCPUInput* pNodeInputs, uint64_t nodeInputByteStride)
{
	const auto populateNodeInput = [](D3D12_NODE_CPU_INPUT& nodeInput, const NodeCPUInput& nodeCPUInput)
	{
		nodeInput.EntrypointIndex = nodeCPUInput.EntrypointIndex;
		nodeInput.NumRecords = nodeCPUInput.NumRecords;
		nodeInput.pRecords = nodeCPUInput.pRecords;
		nodeInput.RecordStrideInBytes = nodeCPUInput.RecordByteStride;
	};

	D3D12_DISPATCH_GRAPH_DESC desc = {};
	if (numNodeInputs == 1)
	{
		assert(pNodeInputs);
		desc.Mode = D3D12_DISPATCH_MODE_NODE_CPU_INPUT;
		populateNodeInput(desc.NodeCPUInput, pNodeInputs[0]);
	}
	else
	{
		if (m_nodeInputs.size() < numNodeInputs) m_nodeInputs.resize(numNodeInputs);
		for (auto i = 0u; i < numNodeInputs; ++i) populateNodeInput(m_nodeInputs[i], pNodeInputs[i]);
		desc.Mode = D3D12_DISPATCH_MODE_MULTI_NODE_CPU_INPUT;
		desc.MultiNodeCPUInput.NumNodeInputs = numNodeInputs;
		desc.MultiNodeCPUInput.NodeInputStrideInBytes = nodeInputByteStride ? nodeInputByteStride : sizeof(D3D12_NODE_CPU_INPUT);
		desc.MultiNodeCPUInput.pNodeInputs = m_nodeInputs.data();
	}

	createAgilityInterface();
	m_commandListA->DispatchGraph(&desc);
}

void CommandList_DX12::DispatchGraph(uint64_t nodeGPUInputAddress, bool isMultiNodes)
{
	D3D12_DISPATCH_GRAPH_DESC desc = {};
	if (isMultiNodes)
	{
		desc.Mode = D3D12_DISPATCH_MODE_MULTI_NODE_GPU_INPUT;
		desc.MultiNodeGPUInput = nodeGPUInputAddress;
	}
	else
	{
		desc.Mode = D3D12_DISPATCH_MODE_NODE_GPU_INPUT;
		desc.NodeGPUInput = nodeGPUInputAddress;
	}

	createAgilityInterface();
	m_commandListA->DispatchGraph(&desc);
}

void CommandList_DX12::DispatchGraph(uint32_t numBindings, const ProgramBinding* pBindings)
{
	assert(numBindings);
	assert(pBindings);

	if (m_programBindings.size() < numBindings) m_programBindings.resize(numBindings);
	for (auto i = 0u; i < numBindings; ++i)
	{
		auto& dst = m_programBindings[i];
		const auto& src = pBindings[i];

		dst.BindPointIndex = src.BindPointIndex;
		dst.Flags = static_cast<D3D12_MLIR_PROGRAM_BINDING_FLAGS>(src.Flags);
		dst.Type = static_cast<D3D12_MLIR_PROGRAM_BINDING_TYPE>(src.Type);

		switch (src.Type)
		{
		case ProgramBindingType::GPU_DESCRIPTOR:
			dst.GpuDescriptorBinding.ptr = src.GpuDescriptor;
			break;
		case ProgramBindingType::CPU:
			dst.CpuBinding.pData = src.Cpu.pData;
			dst.CpuBinding.DataSizeInBytes = src.Cpu.DataByteSize;
			break;
		default:
			assert(src.Type == ProgramBindingType::GPU_VIRTUAL_ADDRESS);
			dst.GpuVirtualAddressBinding.StartAddress = src.GpuVirtualAddress.StartAddress;
			dst.GpuVirtualAddressBinding.SizeInBytes = src.GpuVirtualAddress.ByteSize;
		}
	}

	D3D12_DISPATCH_GRAPH_DESC desc = {};
	desc.Mode = D3D12_DISPATCH_MODE_MLIR_PROGRAM;
	desc.MlirProgramBindings.NumBindings = numBindings;
	desc.MlirProgramBindings.pBindings = m_programBindings.data();

	createAgilityInterface();
	m_commandListA->DispatchGraph(&desc);
}

static D3D12_LINEAR_ALGEBRA_MATRIX_CONVERSION_DEST_INFO GetDX12LinearAlgebraConversionDestInfo(const LinearAlgebraMatrixInfo& matrixInfo)
{
	D3D12_LINEAR_ALGEBRA_MATRIX_CONVERSION_DEST_INFO destInfo;
	destInfo.DestSize = matrixInfo.Size;
	destInfo.DestLayout = GetDX12LinearAlgebraMatrixLayout(matrixInfo.Layout);
	destInfo.DestStride = matrixInfo.Stride;
	destInfo.NumRows = matrixInfo.NumRows;
	destInfo.NumColumns = matrixInfo.NumColumns;
	destInfo.DestDataType = GetDX12LinearAlgebraDataType(matrixInfo.DataType);

	return destInfo;
}

MIDL_INTERFACE("536d9bb6-9eee-4c75-86e8-e29e29e08ed3")
ID3D12GraphicsCommandListPreview0 : public ID3D12GraphicsCommandList10
{
public:
	virtual void STDMETHODCALLTYPE SetWorkGraphMaximumGPUInputRecords(
		_In_  UINT MaxRecords,
		_In_  UINT MaxNodeInputs) = 0;

	virtual void STDMETHODCALLTYPE ConvertLinearAlgebraMatrix(
		_In_  const D3D12_LINEAR_ALGEBRA_MATRIX_CONVERSION_INFO* pDesc,
		_In_  UINT DescCount) = 0;
};

void CommandList_DX12::ConvertLinearAlgebraMatrix(const LinearAlgebraMatrixConversionInfo* pConversions, uint32_t numConversions)
{
	assert(pConversions);
	m_matrixConversions.resize(numConversions);
	for (auto i = 0u; i < numConversions; ++i)
	{
		const auto& conversion = pConversions[i];
		auto& desc = m_matrixConversions[i];

		desc.DestInfo = GetDX12LinearAlgebraConversionDestInfo(conversion.DstInfo);
		desc.SrcInfo.SrcSize = conversion.SrcSize;
		desc.SrcInfo.SrcDataType = GetDX12LinearAlgebraDataType(conversion.SrcDataType);
		desc.SrcInfo.SrcLayout = GetDX12LinearAlgebraMatrixLayout(conversion.SrcLayout);
		desc.SrcInfo.SrcStride = conversion.SrcStride;
		desc.DataDesc.DestVA = conversion.Dst;
		desc.DataDesc.SrcVA = conversion.Src;
	}

	com_ptr<ID3D12GraphicsCommandListPreview> commandList = nullptr;
	auto hr = m_commandList->QueryInterface(IID_PPV_ARGS(&commandList));

	if (FAILED(hr) || !commandList)
	{
		com_ptr<ID3D12GraphicsCommandListPreview0> commandList = nullptr;
		hr = m_commandList->QueryInterface(IID_PPV_ARGS(&commandList));

		if (FAILED(hr)) OutputDebugString(L"Couldn't get DirectX preview interface for the command list.\n");

		commandList->ConvertLinearAlgebraMatrix(m_matrixConversions.data(), numConversions);
	}
	else commandList->ConvertLinearAlgebraMatrix(m_matrixConversions.data(), numConversions);
}

void CommandList_DX12::CopyBufferRegions(uint32_t numRegions, Resource* const* ppDestBuffers, const uint64_t* pDestOffsets,
	Resource* const* ppSourceBuffers, const uint64_t* pSourceOffsets, const uint64_t* pSizes)
{
	assert(numRegions);
	assert(ppDestBuffers);
	assert(pDestOffsets);
	assert(ppSourceBuffers);
	assert(pSourceOffsets);
	assert(pSizes);

	if (m_dstResources.size() < numRegions) m_dstResources.resize(numRegions);
	if (m_srcResources.size() < numRegions) m_srcResources.resize(numRegions);
	for (auto i = 0u; i < numRegions; ++i)
	{
		assert(ppDestBuffers[i]);
		assert(ppSourceBuffers[i]);
		m_dstResources[i] = reinterpret_cast<ID3D12Resource*>(ppDestBuffers[i]->GetNativeHandle());
		m_srcResources[i] = reinterpret_cast<ID3D12Resource*>(ppSourceBuffers[i]->GetNativeHandle());
	}

	createAgilityInterface();
	m_commandListA->CopyBufferRegions(numRegions, m_dstResources.data(), pDestOffsets,
		m_srcResources.data(), pSourceOffsets, pSizes);
}

void CommandList_DX12::CopyTextureRegions(uint32_t numCopyDescs, const TextureCopyDesc* pCopyDescs)
{
	assert(numCopyDescs);
	assert(pCopyDescs);

	auto numRegions = 0u;
	for (auto i = 0u; i < numCopyDescs; ++i) numRegions += pCopyDescs[i].NumRegions;

	if (m_textureCopyDescs.size() < numCopyDescs) m_textureCopyDescs.resize(numCopyDescs);
	if (m_textureCopyRegions.size() < numRegions) m_textureCopyRegions.resize(numRegions);
	if (m_boxes.size() < numRegions) m_boxes.resize(numRegions);

	auto regionIndex = 0u;
	for (auto i = 0u; i < numCopyDescs; ++i)
	{
		const auto& src = pCopyDescs[i];
		auto& dst = m_textureCopyDescs[i];

		dst.Type = static_cast<D3D12_TEXTURE_COPY_DESC_TYPE>(src.Type);
		dst.NumRegions = src.NumRegions;
		dst.pCopyRegions = src.NumRegions ? &m_textureCopyRegions[regionIndex] : nullptr;

		for (auto j = 0u; j < src.NumRegions; ++j)
		{
			const auto& srcRegion = src.pCopyRegions[j];
			auto& dstRegion = m_textureCopyRegions[regionIndex];
			dstRegion.DestX = srcRegion.DestX;
			dstRegion.DestY = srcRegion.DestY;
			dstRegion.DestZ = srcRegion.DestZ;
			if (srcRegion.pSourceBox)
			{
				auto& box = m_boxes[regionIndex];
				box.left = srcRegion.pSourceBox->Left;
				box.top = srcRegion.pSourceBox->Top;
				box.front = srcRegion.pSourceBox->Front;
				box.right = srcRegion.pSourceBox->Right;
				box.bottom = srcRegion.pSourceBox->Bottom;
				box.back = srcRegion.pSourceBox->Back;
				dstRegion.pSourceBox = &box;
			}
			else dstRegion.pSourceBox = nullptr;
			++regionIndex;
		}

		if (src.Type == TextureCopyDescType::TEXTURE_TO_TEXTURE)
		{
			assert(src.TextureTextureCopy.pDest);
			assert(src.TextureTextureCopy.pSource);
			dst.TextureTextureCopy.pDest = reinterpret_cast<ID3D12Resource*>(src.TextureTextureCopy.pDest->GetNativeHandle());
			dst.TextureTextureCopy.DestSubresourceIndex = src.TextureTextureCopy.DestSubresourceIndex;
			dst.TextureTextureCopy.pSource = reinterpret_cast<ID3D12Resource*>(src.TextureTextureCopy.pSource->GetNativeHandle());
			dst.TextureTextureCopy.SourceSubresourceIndex = src.TextureTextureCopy.SourceSubresourceIndex;
		}
		else
		{
			assert(src.BufferTextureCopy.pBuffer);
			assert(src.BufferTextureCopy.pTexture);
			dst.BufferTextureCopy.pBuffer = reinterpret_cast<ID3D12Resource*>(src.BufferTextureCopy.pBuffer->GetNativeHandle());
			dst.BufferTextureCopy.pTexture = reinterpret_cast<ID3D12Resource*>(src.BufferTextureCopy.pTexture->GetNativeHandle());
			dst.BufferTextureCopy.SubresourceIndex = src.BufferTextureCopy.SubresourceIndex;
			dst.BufferTextureCopy.PlacedFootprint.Offset = src.BufferTextureCopy.PlacedFootprint.Offset;
			dst.BufferTextureCopy.PlacedFootprint.Footprint.Format = GetDXGIFormat(src.BufferTextureCopy.PlacedFootprint.Footprint.Format);
			dst.BufferTextureCopy.PlacedFootprint.Footprint.Width = src.BufferTextureCopy.PlacedFootprint.Footprint.Width;
			dst.BufferTextureCopy.PlacedFootprint.Footprint.Height = src.BufferTextureCopy.PlacedFootprint.Footprint.Height;
			dst.BufferTextureCopy.PlacedFootprint.Footprint.Depth = src.BufferTextureCopy.PlacedFootprint.Footprint.Depth;
			dst.BufferTextureCopy.PlacedFootprint.Footprint.RowPitch = src.BufferTextureCopy.PlacedFootprint.Footprint.RowPitch;
		}
	}

	createAgilityInterface();
	m_commandListA->CopyTextureRegions(numCopyDescs, m_textureCopyDescs.data());
}

void CommandList_DX12::CopyResources(uint32_t numSourceDestPairs, Resource* const* ppDestResources,
	Resource* const* ppSourceResources)
{
	assert(numSourceDestPairs);
	assert(ppDestResources);
	assert(ppSourceResources);

	if (m_dstResources.size() < numSourceDestPairs) m_dstResources.resize(numSourceDestPairs);
	if (m_srcResources.size() < numSourceDestPairs) m_srcResources.resize(numSourceDestPairs);
	for (auto i = 0u; i < numSourceDestPairs; ++i)
	{
		assert(ppDestResources[i]);
		assert(ppSourceResources[i]);
		m_dstResources[i] = reinterpret_cast<ID3D12Resource*>(ppDestResources[i]->GetNativeHandle());
		m_srcResources[i] = reinterpret_cast<ID3D12Resource*>(ppSourceResources[i]->GetNativeHandle());
	}

	createAgilityInterface();
	m_commandListA->CopyResources(numSourceDestPairs, m_dstResources.data(), m_srcResources.data());
}

void CommandList_DX12::CopyTilesAsync(Resource* pTiledResource, const TiledResourceCoord* pTileRegionStartCoord,
	const TileRegionSize& tileRegionSize, Resource* pBuffer, uint64_t bufferOffset, TileCopyFlag flags)
{
	assert(pTiledResource);
	assert(pTileRegionStartCoord);
	assert(pBuffer);

	const D3D12_TILED_RESOURCE_COORDINATE regionStartCoord =
	{
		pTileRegionStartCoord->X,
		pTileRegionStartCoord->Y,
		pTileRegionStartCoord->Z,
		pTileRegionStartCoord->Subresource
	};

	const D3D12_TILE_REGION_SIZE regionSize =
	{
		tileRegionSize.NumTiles,
		tileRegionSize.UseBox,
		tileRegionSize.Width,
		tileRegionSize.Height,
		tileRegionSize.Depth
	};

	createAgilityInterface();
	m_commandListA->CopyTilesAsync(reinterpret_cast<ID3D12Resource*>(pTiledResource->GetNativeHandle()),
		&regionStartCoord, &regionSize, reinterpret_cast<ID3D12Resource*>(pBuffer->GetNativeHandle()),
		bufferOffset, GetDX12TileCopyFlags(flags));
}

void CommandList_DX12::ResolveSubresourceRegionAsync(Resource* pDstResource, uint32_t dstSubresource,
	uint32_t dstX, uint32_t dstY, Resource* pSrcResource, uint32_t srcSubresource,
	const RectRange* pSrcRect, Format format, ResolveMode resolveMode)
{
	assert(pDstResource);
	assert(pSrcResource);

	D3D12_RECT rect;
	if (pSrcRect)
	{
		rect.left = pSrcRect->Left;
		rect.top = pSrcRect->Top;
		rect.right = pSrcRect->Right;
		rect.bottom = pSrcRect->Bottom;
	}

	createAgilityInterface();
	m_commandListA->ResolveSubresourceRegionAsync(
		reinterpret_cast<ID3D12Resource*>(pDstResource->GetNativeHandle()), dstSubresource, dstX, dstY,
		reinterpret_cast<ID3D12Resource*>(pSrcResource->GetNativeHandle()), srcSubresource,
		pSrcRect ? &rect : nullptr, GetDXGIFormat(format), GetDX12ResolveMode(resolveMode));
}

void CommandList_DX12::ResolveQueryDataAsync(const QueryHeap& queryHeap, QueryType type, uint32_t startIndex,
	uint32_t numQueries, Resource* pDestBuffer, uint64_t destOffset)
{
	assert(pDestBuffer);

	createAgilityInterface();
	m_commandListA->ResolveQueryDataAsync(reinterpret_cast<ID3D12QueryHeap*>(queryHeap),
		GetDX12QueryType(type), startIndex, numQueries,
		reinterpret_cast<ID3D12Resource*>(pDestBuffer->GetNativeHandle()), destOffset);
}

void CommandList_DX12::ClearBoundDepthStencilView(ClearFlag clearFlags, float depth, uint8_t stencil,
	uint32_t numRegions, const BoxRange* pRegions)
{
	if (numRegions)
	{
		assert(pRegions);
		if (m_boxes.size() < numRegions) m_boxes.resize(numRegions);
		for (auto i = 0u; i < numRegions; ++i)
		{
			m_boxes[i].left = pRegions[i].Left;
			m_boxes[i].top = pRegions[i].Top;
			m_boxes[i].front = pRegions[i].Front;
			m_boxes[i].right = pRegions[i].Right;
			m_boxes[i].bottom = pRegions[i].Bottom;
			m_boxes[i].back = pRegions[i].Back;
		}
	}

	createAgilityInterface();
	m_commandListA->ClearBoundDepthStencilView(GetDX12ClearFlags(clearFlags), depth, stencil,
		numRegions, numRegions ? m_boxes.data() : nullptr);
}

void CommandList_DX12::ClearBoundRenderTargetViews(uint32_t renderTargetMask, const ClearData* pClearValues,
	const uint32_t* pNumRegions, const BoxRange* const* ppRegions)
{
	assert(pClearValues);
	assert(renderTargetMask);
	assert((renderTargetMask & ~0xffu) == 0);

	auto highestSlot = 0u;
	for (auto i = 0u; i < 8; ++i)
		if (renderTargetMask & (1u << i)) highestSlot = i;

	const auto numSlots = highestSlot + 1;
	if (m_clearData.size() < numSlots) m_clearData.resize(numSlots);
	for (auto i = 0u; i < numSlots; ++i)
		if (renderTargetMask & (1u << i))
			memcpy(&m_clearData[i], &pClearValues[i], sizeof(D3D12_CLEAR_DATA));

	const UINT* pDX12NumRegions = nullptr;
	const D3D12_BOX* const* ppDX12Regions = nullptr;
	if (pNumRegions)
	{
		assert(ppRegions);
		auto totalRegions = 0u;
		for (auto i = 0u; i < numSlots; ++i)
			if (renderTargetMask & (1u << i)) totalRegions += pNumRegions[i];

		if (m_boxes.size() < totalRegions) m_boxes.resize(totalRegions);
		if (m_boxPointers.size() < numSlots) m_boxPointers.resize(numSlots);

		auto boxIndex = 0u;
		for (auto i = 0u; i < numSlots; ++i)
		{
			if ((renderTargetMask & (1u << i)) == 0)
			{
				m_boxPointers[i] = nullptr;
				continue;
			}

			if (pNumRegions[i] == 0 || !ppRegions[i])
			{
				m_boxPointers[i] = nullptr;
				continue;
			}

			m_boxPointers[i] = &m_boxes[boxIndex];
			for (auto j = 0u; j < pNumRegions[i]; ++j)
			{
				m_boxes[boxIndex].left = ppRegions[i][j].Left;
				m_boxes[boxIndex].top = ppRegions[i][j].Top;
				m_boxes[boxIndex].front = ppRegions[i][j].Front;
				m_boxes[boxIndex].right = ppRegions[i][j].Right;
				m_boxes[boxIndex].bottom = ppRegions[i][j].Bottom;
				m_boxes[boxIndex].back = ppRegions[i][j].Back;
				++boxIndex;
			}
		}

		pDX12NumRegions = pNumRegions;
		ppDX12Regions = m_boxPointers.data();
	}

	createAgilityInterface();
	m_commandListA->ClearBoundRenderTargetViews(renderTargetMask, m_clearData.data(),
		pDX12NumRegions, ppDX12Regions);
}

void CommandList_DX12::ClearTextureSubresources(uint32_t numClearDescs, const ClearTextureDesc* pClearDescs)
{
	assert(numClearDescs);
	assert(pClearDescs);

	if (m_clearTextureDescs.size() < numClearDescs) m_clearTextureDescs.resize(numClearDescs);
	if (m_boxes.size() < numClearDescs) m_boxes.resize(numClearDescs);

	for (auto i = 0u; i < numClearDescs; ++i)
	{
		const auto& src = pClearDescs[i];
		auto& dst = m_clearTextureDescs[i];

		assert(src.pTexture);
		dst.pTexture = reinterpret_cast<ID3D12Resource*>(src.pTexture->GetNativeHandle());
		dst.SubresourceIndex = src.SubresourceIndex;
		memcpy(&dst.ClearValue, &src.ClearValue, sizeof(D3D12_CLEAR_DATA));
		dst.Format = GetDXGIFormat(src.Format);
		if (src.pRegion)
		{
			m_boxes[i].left = src.pRegion->Left;
			m_boxes[i].top = src.pRegion->Top;
			m_boxes[i].front = src.pRegion->Front;
			m_boxes[i].right = src.pRegion->Right;
			m_boxes[i].bottom = src.pRegion->Bottom;
			m_boxes[i].back = src.pRegion->Back;
			dst.pRegion = &m_boxes[i];
		}
		else dst.pRegion = nullptr;
	}

	createAgilityInterface();
	m_commandListA->ClearTextureSubresources(numClearDescs, m_clearTextureDescs.data());
}

void CommandList_DX12::FillBuffers(uint32_t numFillDescs, const FillBufferDesc* pFillDescs)
{
	assert(numFillDescs);
	assert(pFillDescs);

	if (m_fillBufferDescs.size() < numFillDescs) m_fillBufferDescs.resize(numFillDescs);
	for (auto i = 0u; i < numFillDescs; ++i)
	{
		const auto& src = pFillDescs[i];
		auto& dst = m_fillBufferDescs[i];

		assert(src.pBuffer);
		dst.pBuffer = reinterpret_cast<ID3D12Resource*>(src.pBuffer->GetNativeHandle());
		dst.Offset = src.Offset;
		memcpy(&dst.FillValue, &src.FillValue, sizeof(D3D12_CLEAR_DATA));
		dst.Format = GetDXGIFormat(src.Format);
		dst.RawPatternSizeInBytes = src.RawPatternSizeInBytes;
		dst.RepeatCount = src.RepeatCount;
	}

	createAgilityInterface();
	m_commandListA->FillBuffers(numFillDescs, m_fillBufferDescs.data());
}

XUSG::com_ptr<ID3D12GraphicsCommandList6>& CommandList_DX12::GetGraphicsCommandList()
{
	return m_commandListU;
}

bool CommandList_DX12::createInterface()
{
	if (!m_commandList) return false;
	const auto hr = m_commandList->QueryInterface(IID_PPV_ARGS(&m_commandListU));

	if (FAILED(hr))
	{
		OutputDebugString(L"Couldn't get DirectX ultimate interface for the command list.\n");

		return false;
	}

	return true;
}

void CommandList_DX12::createAgilityInterface()
{
	if (m_commandListA == nullptr)
	{
		const auto hr = m_commandList->QueryInterface(IID_PPV_ARGS(&m_commandListA));

		if (FAILED(hr)) OutputDebugString(L"Couldn't get ID3D12GraphicsCommandList12 for the command list.\n");
	}

	assert(m_commandListA); // Call only when GetAsyncCommandsImpl != NOT_SUPPORTED (1.620+ / fallback).
}

//--------------------------------------------------------------------------------------
// Pipeline layout
//--------------------------------------------------------------------------------------

PipelineLayoutLib_DX12::PipelineLayoutLib_DX12() :
	XUSG::PipelineLayoutLib_DX12()
{
}

PipelineLayoutLib_DX12::PipelineLayoutLib_DX12(Device* pDevice) :
	XUSG::PipelineLayoutLib_DX12(pDevice)
{
}

PipelineLayoutLib_DX12::~PipelineLayoutLib_DX12()
{
}

XUSG::PipelineLayout PipelineLayoutLib_DX12::CreateRootSignatureFromLibSubobject(
	const Blob& blobLib, const wchar_t* name, uint32_t nodeMask)
{
	string key(sizeof(blobLib), 0);
	memcpy(&key[0], blobLib, sizeof(blobLib));

	return createRootSignatureFromLibSubobject(key, blobLib, name, nodeMask);
}

XUSG::PipelineLayout PipelineLayoutLib_DX12::GetRootSignatureFromLibSubobject(const Blob& blobLib,
	const wchar_t* name, bool create, uint32_t nodeMask)
{
	string key(sizeof(blobLib), 0);
	memcpy(&key[0], blobLib, sizeof(blobLib));

	return getRootSignatureFromLibSubobject(key, blobLib, name, create, nodeMask);
}

XUSG::PipelineLayout PipelineLayoutLib_DX12::createRootSignatureFromLibSubobject(const string& key,
	const Blob& blobLib, const wchar_t* name, uint32_t nodeMask)
{
	com_ptr<ID3D12RootSignature> rootSignature;
	com_ptr<ID3D12Device14> device = nullptr;
	const auto pBlob = static_cast<ID3DBlob*>(blobLib);
	V_RETURN(m_device->QueryInterface(IID_PPV_ARGS(&device)), cerr, nullptr);
	V_RETURN(device->CreateRootSignatureFromSubobjectInLibrary(nodeMask, pBlob->GetBufferPointer(),
		pBlob->GetBufferSize(), name, IID_PPV_ARGS(&rootSignature)), cerr, nullptr);
	rootSignature->SetName(name);
	m_rootSignatures[key] = rootSignature;

	return rootSignature.get();
}

XUSG::PipelineLayout PipelineLayoutLib_DX12::getRootSignatureFromLibSubobject(const string& key,
	const Blob& blobLib, const wchar_t* name, bool create, uint32_t nodeMask)
{
	const auto layoutIter = m_rootSignatures.find(key);

	// Create one, if it does not exist
	if (layoutIter == m_rootSignatures.cend())
	{
		if (create) return createRootSignatureFromLibSubobject(key, blobLib, name, nodeMask);
		else return nullptr;
	}

	return layoutIter->second.get();
}

//--------------------------------------------------------------------------------------
// Sampler feedback
//--------------------------------------------------------------------------------------

SamplerFeedBack_DX12::SamplerFeedBack_DX12() :
	XUSG::Texture_DX12()
{
}

SamplerFeedBack_DX12::~SamplerFeedBack_DX12()
{
}

bool SamplerFeedBack_DX12::Create(Device* pDevice, Texture* pTarget, Format format,
	uint32_t mipRegionWidth, uint32_t mipRegionHeight, uint32_t mipRegionDepth,
	ResourceFlag resourceFlags, bool isCubeMap, MemoryFlag memoryFlags, const wchar_t* name,
	uint16_t srvComponentMapping, const TextureLayoutDesc& textureLayout, uint32_t maxThreads)
{
	assert(format == Format::MIN_MIP_OPAQUE || format == Format::MIP_REGION_USED_OPAQUE);
	m_format = format;

	const auto hasSRV = (resourceFlags & ResourceFlag::DENY_SHADER_RESOURCE) == ResourceFlag::NONE;

	uint16_t arraySize;
	uint8_t numMips;

	if (pTarget)
	{
		// Get paired properties
		arraySize = pTarget->GetArraySize();
		numMips = pTarget->GetNumMips();

		XUSG_N_RETURN(CreateResource(pDevice, pTarget, format, mipRegionWidth, mipRegionHeight, mipRegionDepth,
			resourceFlags, isCubeMap, memoryFlags, ResourceState::COMMON, textureLayout, maxThreads), false);
	}
	else arraySize = numMips = 1;

	SetName(name);

	// Create SRVs
	if (hasSRV)
		XUSG_N_RETURN(createSRVs(arraySize, m_format, numMips, false, isCubeMap, srvComponentMapping), false);

	// Create UAV
	if (pTarget)
	{
		m_uavs.resize(1);
		XUSG_X_RETURN(m_uavs[0], CreateUAV(pTarget), false);
	}

	return true;
}

bool SamplerFeedBack_DX12::Create(Device* pDevice, Heap* pHeap, uint64_t heapOffset,
	Texture* pTarget, Format format, uint32_t mipRegionWidth, uint32_t mipRegionHeight,
	uint32_t mipRegionDepth, ResourceFlag resourceFlags, bool isCubeMap, const wchar_t* name,
	uint16_t srvComponentMapping, const TextureLayoutDesc& textureLayout, uint32_t maxThreads)
{
	assert(format == Format::MIN_MIP_OPAQUE || format == Format::MIP_REGION_USED_OPAQUE);
	m_format = format;

	const auto hasSRV = (resourceFlags & ResourceFlag::DENY_SHADER_RESOURCE) == ResourceFlag::NONE;

	uint16_t arraySize;
	uint8_t numMips;

	if (pTarget)
	{
		// Get paired properties
		arraySize = pTarget->GetArraySize();
		numMips = pTarget->GetNumMips();

		XUSG_N_RETURN(CreateResource(pDevice, pHeap, heapOffset, pTarget, format, mipRegionWidth, mipRegionHeight,
			mipRegionDepth, resourceFlags, isCubeMap, ResourceState::COMMON, textureLayout, maxThreads), false);
	}
	else arraySize = numMips = 1;

	SetName(name);

	// Create SRVs
	if (hasSRV)
		XUSG_N_RETURN(createSRVs(arraySize, m_format, numMips, false, isCubeMap, srvComponentMapping), false);

	// Create UAV
	if (pTarget)
	{
		m_uavs.resize(1);
		XUSG_X_RETURN(m_uavs[0], CreateUAV(pTarget), false);
	}

	return true;
}

bool SamplerFeedBack_DX12::CreateResource(Device* pDevice, Texture* pTarget, Format format, uint32_t mipRegionWidth,
	uint32_t mipRegionHeight, uint32_t mipRegionDepth, ResourceFlag resourceFlags, bool isCubeMap, MemoryFlag memoryFlags,
	ResourceState initialResourceState, const TextureLayoutDesc& textureLayout, uint32_t maxThreads)
{
	com_ptr<ID3D12Device10> device;
	XUSG_N_RETURN(initialize(pDevice), false);
	V_RETURN(m_device->QueryInterface(IID_PPV_ARGS(&device)), cerr, false);
	assert(textureLayout.Layout != TextureLayout::GUID && "GUID texture layout is not supported for sampler feedback");

	m_format = m_format != Format::UNKNOWN ? m_format : format;

	// Get paired properties
	const uint16_t arraySize = pTarget->GetArraySize();
	const uint8_t numMips = pTarget->GetNumMips();

	// Setup the texture description.
	assert(pTarget);
	assert(format == Format::MIN_MIP_OPAQUE || format == Format::MIP_REGION_USED_OPAQUE);
	const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
	const auto desc = CD3DX12_RESOURCE_DESC1::Tex2D(GetDXGIFormat(format),
		pTarget->GetWidth(), pTarget->GetHeight(), arraySize, numMips, 1, 0,
		GetDX12ResourceFlags(ResourceFlag::ALLOW_UNORDERED_ACCESS | resourceFlags),
		GetDX12TextureLayout(textureLayout), 0, mipRegionWidth, mipRegionHeight, mipRegionDepth);

	// Determine initial state
	assert(maxThreads);
	m_states.resize(maxThreads);
	for (auto& states : m_states)
		states.resize(arraySize * numMips, initialResourceState);

	V_RETURN(device->CreateCommittedResource2(&heapProperties, GetDX12HeapFlags(memoryFlags), &desc,
		GetDX12ResourceStates(m_states[0][0]), nullptr, nullptr, IID_PPV_ARGS(&m_resource)), clog, false);

	return true;
}

bool SamplerFeedBack_DX12::CreateResource(Device* pDevice, Heap* pHeap, uint64_t heapOffset, Texture* pTarget,
	Format format, uint32_t mipRegionWidth, uint32_t mipRegionHeight, uint32_t mipRegionDepth, ResourceFlag resourceFlags,
	bool isCubeMap, ResourceState initialResourceState, const TextureLayoutDesc& textureLayout, uint32_t maxThreads)
{
	com_ptr<ID3D12Device10> device;
	XUSG_N_RETURN(initialize(pDevice), false);
	V_RETURN(m_device->QueryInterface(IID_PPV_ARGS(&device)), cerr, false);
	assert(textureLayout.Layout != TextureLayout::GUID && "GUID texture layout is not supported for sampler feedback");

	m_format = m_format != Format::UNKNOWN ? m_format : format;

	// Get paired properties
	const uint16_t arraySize = pTarget->GetArraySize();
	const uint8_t numMips = pTarget->GetNumMips();

	// Setup the texture description.
	assert(pTarget);
	assert(format == Format::MIN_MIP_OPAQUE || format == Format::MIP_REGION_USED_OPAQUE);
	const auto desc = CD3DX12_RESOURCE_DESC1::Tex2D(GetDXGIFormat(format),
		pTarget->GetWidth(), pTarget->GetHeight(), arraySize, numMips, 1, 0,
		GetDX12ResourceFlags(ResourceFlag::ALLOW_UNORDERED_ACCESS | resourceFlags),
		GetDX12TextureLayout(textureLayout), 0, mipRegionWidth, mipRegionHeight, mipRegionDepth);

	// Determine initial state
	assert(maxThreads);
	m_states.resize(maxThreads);
	for (auto& states : m_states)
		states.resize(arraySize * numMips, initialResourceState);

	assert(pHeap);
	V_RETURN(device->CreatePlacedResource2(reinterpret_cast<ID3D12Heap*>(pHeap->GetNativeHandle()), heapOffset, &desc,
		GetDX12BarrierLayout(GetBarrierLayout(m_states[0][0])), nullptr, 0, nullptr, IID_PPV_ARGS(&m_resource)),
		clog, false);

	return true;
}

XUSG::Descriptor SamplerFeedBack_DX12::CreateUAV(Resource* pTarget)
{
	// Create an unordered access view
	assert(pTarget);
	auto& resourceView = m_resourceViews.emplace_back(make_shared<ResourceView>());
	*resourceView = {};
	resourceView->Type = ResourceViewType::SAMPLER_FEEDBACK;
	resourceView->pResource = this;
	resourceView->pSamplerFeedbackTarget = pTarget;

	return resourceView.get();
}

XUSG::ProgramIdentifier XUSG::Ultimate::GetDX12ProgramIdentifier(const XUSG::Pipeline& stateObject, const wchar_t* programName)
{
	assert(stateObject);
	assert(programName);

	using namespace XUSG;
	com_ptr<ID3D12StateObjectProperties1> properties;
	V_RETURN(static_cast<ID3D12StateObject*>(stateObject)->QueryInterface(IID_PPV_ARGS(&properties)), cerr, ProgramIdentifier{});

	const auto programId = properties->GetProgramIdentifier(programName);

	ProgramIdentifier identifier;
	identifier.OpaqueData[0] = programId.OpaqueData[0];
	identifier.OpaqueData[1] = programId.OpaqueData[1];
	identifier.OpaqueData[2] = programId.OpaqueData[2];
	identifier.OpaqueData[3] = programId.OpaqueData[3];

	return identifier;
}

namespace XUSG::Ultimate
{
	namespace
	{
		// Declared in d3d12.h but not provided by dxguid.lib.
		constexpr GUID c_clsidAsyncCommandsFallback =
		{ 0xef4992f0, 0x0fae, 0x424e, { 0xb4, 0x84, 0x7b, 0xcd, 0x8e, 0x56, 0x9e, 0x3e } };
		constexpr GUID c_clsidDeviceFactory =
		{ 0x114863bf, 0xc386, 0x4aee, { 0xb3, 0x9d, 0x8f, 0x0b, 0xbb, 0x06, 0x29, 0x55 } };

		// Prefer factory GetConfigurationInterface for factory-created devices; else D3D12GetInterface
		// for D3D12CreateDevice (see D3D12AsyncCommands.md / Fallback spec).
		void setAsyncCommandsFallbackLayer(bool enable, ID3D12DeviceFactory* pFactory = nullptr)
		{
			com_ptr<ID3D12AsyncCommandsFallback> fallback;
			const auto hr = pFactory ?
				pFactory->GetConfigurationInterface(c_clsidAsyncCommandsFallback, IID_PPV_ARGS(&fallback)) :
				D3D12GetInterface(c_clsidAsyncCommandsFallback, IID_PPV_ARGS(&fallback));
			if (SUCCEEDED(hr))
				fallback->SetFallbackLayerEnabled(enable ? TRUE : FALSE);
		}

		uint32_t createDeviceWithAsyncCommandsFallbackIfNeeded(IDXGIAdapter* pAdapter,
			D3D_FEATURE_LEVEL minFeatureLevel, ID3D12Device** ppDevice, const wchar_t* name)
		{
			assert(ppDevice);
			*ppDevice = nullptr;

			com_ptr<ID3D12DeviceFactory> factory;
			const auto hasFactory = SUCCEEDED(D3D12GetInterface(c_clsidDeviceFactory, IID_PPV_ARGS(&factory)));
			ID3D12DeviceFactory* const pFactory = hasFactory ? factory.get() : nullptr;

			auto createDevice = [&](bool disallowSingleton, ID3D12Device** ppOut) -> HRESULT
			{
				if (pFactory)
				{
					factory->SetFlags(disallowSingleton ?
						D3D12_DEVICE_FACTORY_FLAG_DISALLOW_STORING_NEW_DEVICE_AS_SINGLETON :
						D3D12_DEVICE_FACTORY_FLAG_NONE);

					return factory->CreateDevice(pAdapter, minFeatureLevel, IID_PPV_ARGS(ppOut));
				}

				return D3D12CreateDevice(pAdapter, minFeatureLevel, IID_PPV_ARGS(ppOut));
			};

			// Plain CreateDevice path matching Core when the async opt-in path cannot proceed.
			auto createCoreCompatibleDevice = [&](ID3D12Device** ppOut) -> HRESULT
			{
				setAsyncCommandsFallbackLayer(false, pFactory);
				auto hrCreate = createDevice(false, ppOut);
				if (FAILED(hrCreate) && pFactory)
					hrCreate = D3D12CreateDevice(pAdapter, minFeatureLevel, IID_PPV_ARGS(ppOut));

				return hrCreate;
			};

			auto finish = [&](com_ptr<ID3D12Device>& device, HRESULT hrFinish) -> HRESULT
			{
				if (FAILED(hrFinish) || !device) return hrFinish;
				if (name) device->SetName(name);
				*ppDevice = device.detach();

				return hrFinish;
			};

			// Probe with fallback off. DISALLOW_STORING avoids singleton conflict if we recreate
			// after changing the fallback setting (Fallback spec: Opting in).
			setAsyncCommandsFallbackLayer(false, pFactory);

			com_ptr<ID3D12Device> device;
			auto hr = createDevice(true, device.put());
			if (FAILED(hr))
			{
				// Factory/DISALLOW quirks must not break Core: fall back to plain create.
				hr = createCoreCompatibleDevice(device.put());

				return finish(device, hr);
			}

			D3D12_FEATURE_DATA_ASYNC_COMMANDS featureData = {};

			// Inbox Windows d3d12.dll / older Agility / EOL drivers: unknown feature ? E_INVALIDARG.
			// Treat as absent and do NOT enable the fallback layer.
			if (FAILED(device->CheckFeatureSupport(
				D3D12_FEATURE_ASYNC_COMMANDS, &featureData, sizeof(featureData))))
				return finish(device, hr);

			// Compare against enum values (spec: do not treat Impl as a boolean).
			if (featureData.Impl == D3D12_ASYNC_COMMANDS_IMPL_NATIVE ||
				featureData.Impl == D3D12_ASYNC_COMMANDS_IMPL_FALLBACK)
				return finish(device, hr);

			// Fallback requires Enhanced Barriers (Fallback spec). OPTIONS12 may also be unknown
			// on inbox runtimes ? FAILED here likewise means keep the probe device.
			D3D12_FEATURE_DATA_D3D12_OPTIONS12 options12 = {};
			if (FAILED(device->CheckFeatureSupport(
				D3D12_FEATURE_D3D12_OPTIONS12, &options12, sizeof(options12))) ||
				!options12.EnhancedBarriersSupported)
				return finish(device, hr);

			assert(featureData.Impl == D3D12_ASYNC_COMMANDS_IMPL_NOT_SUPPORTED);

			device = nullptr;
			setAsyncCommandsFallbackLayer(true, pFactory);

			hr = createDevice(false, device.put());
			if (FAILED(hr))
			{
				// Opt-in recreate failed: restore Core-compatible create so Device::Create still works.
				hr = createCoreCompatibleDevice(device.put());
			}

			return finish(device, hr);
		}
	}

	void EnableDX12AsyncCommandsFallback(bool enable)
	{
		setAsyncCommandsFallbackLayer(false);
		SetDX12DeviceCreateHook(enable ? createDeviceWithAsyncCommandsFallbackIfNeeded : nullptr);
	}
}

bool XUSG::Ultimate::GetDX12AsyncCommandsImpl(Device* pDevice, AsyncCommandsImpl* pImpl)
{
	assert(pDevice);
	assert(pImpl);

	com_ptr<ID3D12Device> device = pDevice->GetNativeHandle();
	assert(device);

	D3D12_FEATURE_DATA_ASYNC_COMMANDS featureData = {};
	if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ASYNC_COMMANDS, &featureData, sizeof(featureData))))
	{
		*pImpl = AsyncCommandsImpl::NOT_SUPPORTED;

		return false;
	}

	*pImpl = static_cast<AsyncCommandsImpl>(featureData.Impl);

	return true;
}

MIDL_INTERFACE("55ea41d3-6bf5-4332-bbf9-905e6b4e2930")
ID3D12DevicePreview0 : public IUnknown
{
public:
	virtual void STDMETHODCALLTYPE GetLinearAlgebraMatrixConversionDestinationInfo(
		_Inout_  D3D12_LINEAR_ALGEBRA_MATRIX_CONVERSION_DEST_INFO * pDesc) = 0;
};

void XUSG::Ultimate::GetDX12LinearAlgebraMatrixInfo(const Device* pDevice, LinearAlgebraMatrixInfo& matrixInfo)
{
	auto destInfo = GetDX12LinearAlgebraConversionDestInfo(matrixInfo);

	com_ptr<ID3D12DevicePreview> device = nullptr;
	auto hr = static_cast<ID3D12Device*>(pDevice->GetNativeHandle())->QueryInterface(IID_PPV_ARGS(&device));

	if (FAILED(hr) || !device)
	{
		com_ptr<ID3D12DevicePreview0> device = nullptr;
		hr = static_cast<ID3D12Device*>(pDevice->GetNativeHandle())->QueryInterface(IID_PPV_ARGS(&device));

		if (FAILED(hr)) OutputDebugString(L"Couldn't get DirectX preview interface for the device.\n");

		device->GetLinearAlgebraMatrixConversionDestinationInfo(&destInfo);
	}
	else device->GetLinearAlgebraMatrixConversionDestinationInfo(&destInfo);

	matrixInfo.Size = destInfo.DestSize;
}

//--------------------------------------------------------------------------------------
// DX12 enum transfer functions
//--------------------------------------------------------------------------------------

D3D12_BARRIER_SYNC XUSG::Ultimate::GetDX12BarrierSync(BarrierSync barrierSync)
{
	static const D3D12_BARRIER_SYNC barrierSyncs[] =
	{
		D3D12_BARRIER_SYNC_ALL,
		D3D12_BARRIER_SYNC_DRAW,
		D3D12_BARRIER_SYNC_INDEX_INPUT,
		D3D12_BARRIER_SYNC_VERTEX_SHADING,
		D3D12_BARRIER_SYNC_PIXEL_SHADING,
		D3D12_BARRIER_SYNC_DEPTH_STENCIL,
		D3D12_BARRIER_SYNC_RENDER_TARGET,
		D3D12_BARRIER_SYNC_COMPUTE_SHADING,
		D3D12_BARRIER_SYNC_RAYTRACING,
		D3D12_BARRIER_SYNC_COPY,
		D3D12_BARRIER_SYNC_RESOLVE,
		D3D12_BARRIER_SYNC_EXECUTE_INDIRECT,
		D3D12_BARRIER_SYNC_ALL_SHADING,
		D3D12_BARRIER_SYNC_NON_PIXEL_SHADING,
		D3D12_BARRIER_SYNC_EMIT_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO,
		D3D12_BARRIER_SYNC_CLEAR_UNORDERED_ACCESS_VIEW,
		D3D12_BARRIER_SYNC_VIDEO_DECODE,
		D3D12_BARRIER_SYNC_VIDEO_PROCESS,
		D3D12_BARRIER_SYNC_VIDEO_ENCODE,
		D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE,
		D3D12_BARRIER_SYNC_COPY_RAYTRACING_ACCELERATION_STRUCTURE,
		D3D12_BARRIER_SYNC_SPLIT,
		D3D12_BARRIER_SYNC_CONVERT_LINEAR_ALGEBRA_MATRIX
	};

	assert(barrierSyncs[Log2(static_cast<uint32_t>(BarrierSync::ALL))] == D3D12_BARRIER_SYNC_ALL);
	assert(barrierSyncs[Log2(static_cast<uint32_t>(BarrierSync::DRAW))] == D3D12_BARRIER_SYNC_DRAW);
	assert(barrierSyncs[Log2(static_cast<uint32_t>(BarrierSync::INDEX_INPUT))] == D3D12_BARRIER_SYNC_INDEX_INPUT);
	assert(barrierSyncs[Log2(static_cast<uint32_t>(BarrierSync::VERTEX_SHADING))] == D3D12_BARRIER_SYNC_VERTEX_SHADING);
	assert(barrierSyncs[Log2(static_cast<uint32_t>(BarrierSync::PIXEL_SHADING))] == D3D12_BARRIER_SYNC_PIXEL_SHADING);
	assert(barrierSyncs[Log2(static_cast<uint32_t>(BarrierSync::DEPTH_STENCIL))] == D3D12_BARRIER_SYNC_DEPTH_STENCIL);
	assert(barrierSyncs[Log2(static_cast<uint32_t>(BarrierSync::RENDER_TARGET))] == D3D12_BARRIER_SYNC_RENDER_TARGET);
	assert(barrierSyncs[Log2(static_cast<uint32_t>(BarrierSync::COMPUTE_SHADING))] == D3D12_BARRIER_SYNC_COMPUTE_SHADING);
	assert(barrierSyncs[Log2(static_cast<uint32_t>(BarrierSync::RAYTRACING))] == D3D12_BARRIER_SYNC_RAYTRACING);
	assert(barrierSyncs[Log2(static_cast<uint32_t>(BarrierSync::COPY))] == D3D12_BARRIER_SYNC_COPY);
	assert(barrierSyncs[Log2(static_cast<uint32_t>(BarrierSync::RESOLVE))] == D3D12_BARRIER_SYNC_RESOLVE);
	assert(barrierSyncs[Log2(static_cast<uint32_t>(BarrierSync::EXECUTE_INDIRECT))] == D3D12_BARRIER_SYNC_EXECUTE_INDIRECT);
	assert(barrierSyncs[Log2(static_cast<uint32_t>(BarrierSync::ALL_SHADING))] == D3D12_BARRIER_SYNC_ALL_SHADING);
	assert(barrierSyncs[Log2(static_cast<uint32_t>(BarrierSync::NON_PIXEL_SHADING))] == D3D12_BARRIER_SYNC_NON_PIXEL_SHADING);
	assert(barrierSyncs[Log2(static_cast<uint32_t>(BarrierSync::EMIT_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO))] == D3D12_BARRIER_SYNC_EMIT_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO);
	assert(barrierSyncs[Log2(static_cast<uint32_t>(BarrierSync::CLEAR_UNORDERED_ACCESS_VIEW))] == D3D12_BARRIER_SYNC_CLEAR_UNORDERED_ACCESS_VIEW);
	assert(barrierSyncs[Log2(static_cast<uint32_t>(BarrierSync::VIDEO_DECODE))] == D3D12_BARRIER_SYNC_VIDEO_DECODE);
	assert(barrierSyncs[Log2(static_cast<uint32_t>(BarrierSync::VIDEO_PROCESS))] == D3D12_BARRIER_SYNC_VIDEO_PROCESS);
	assert(barrierSyncs[Log2(static_cast<uint32_t>(BarrierSync::VIDEO_ENCODE))] == D3D12_BARRIER_SYNC_VIDEO_ENCODE);
	assert(barrierSyncs[Log2(static_cast<uint32_t>(BarrierSync::BUILD_RAYTRACING_ACCELERATION_STRUCTURE))] == D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE);
	assert(barrierSyncs[Log2(static_cast<uint32_t>(BarrierSync::COPY_RAYTRACING_ACCELERATION_STRUCTURE))] == D3D12_BARRIER_SYNC_COPY_RAYTRACING_ACCELERATION_STRUCTURE);
	assert(barrierSyncs[Log2(static_cast<uint32_t>(BarrierSync::SPLIT))] == D3D12_BARRIER_SYNC_SPLIT);
	assert(barrierSyncs[Log2(static_cast<uint32_t>(BarrierSync::CONVERT_LINEAR_ALGEBRA_MATRIX))] == D3D12_BARRIER_SYNC_CONVERT_LINEAR_ALGEBRA_MATRIX);
	assert(barrierSyncs[Log2(static_cast<uint32_t>(BarrierSync::PREDICATION))] == D3D12_BARRIER_SYNC_EXECUTE_INDIRECT);

	if (barrierSync == BarrierSync::NONE) return D3D12_BARRIER_SYNC_NONE;

	const auto index = Log2(static_cast<uint32_t>(barrierSync));

	return barrierSyncs[index];
}

D3D12_BARRIER_SYNC XUSG::Ultimate::GetDX12BarrierSyncs(BarrierSync barrierSync)
{
	auto barrierSyncs = D3D12_BARRIER_SYNC_NONE;
	barrierSyncs |= APPEND_BARRIER_SYNC(barrierSync, ALL);
	barrierSyncs |= APPEND_BARRIER_SYNC(barrierSync, DRAW);
	barrierSyncs |= APPEND_BARRIER_SYNC(barrierSync, INDEX_INPUT);
	barrierSyncs |= APPEND_BARRIER_SYNC(barrierSync, VERTEX_SHADING);
	barrierSyncs |= APPEND_BARRIER_SYNC(barrierSync, PIXEL_SHADING);
	barrierSyncs |= APPEND_BARRIER_SYNC(barrierSync, DEPTH_STENCIL);
	barrierSyncs |= APPEND_BARRIER_SYNC(barrierSync, RENDER_TARGET);
	barrierSyncs |= APPEND_BARRIER_SYNC(barrierSync, COMPUTE_SHADING);
	barrierSyncs |= APPEND_BARRIER_SYNC(barrierSync, RAYTRACING);
	barrierSyncs |= APPEND_BARRIER_SYNC(barrierSync, COPY);
	barrierSyncs |= APPEND_BARRIER_SYNC(barrierSync, RESOLVE);
	barrierSyncs |= APPEND_BARRIER_SYNC(barrierSync, EXECUTE_INDIRECT);
	barrierSyncs |= APPEND_BARRIER_SYNC(barrierSync, ALL_SHADING);
	barrierSyncs |= APPEND_BARRIER_SYNC(barrierSync, NON_PIXEL_SHADING);
	barrierSyncs |= APPEND_BARRIER_SYNC(barrierSync, EMIT_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO);
	barrierSyncs |= APPEND_BARRIER_SYNC(barrierSync, CLEAR_UNORDERED_ACCESS_VIEW);
	barrierSyncs |= APPEND_BARRIER_SYNC(barrierSync, VIDEO_DECODE);
	barrierSyncs |= APPEND_BARRIER_SYNC(barrierSync, VIDEO_PROCESS);
	barrierSyncs |= APPEND_BARRIER_SYNC(barrierSync, VIDEO_ENCODE);
	barrierSyncs |= APPEND_BARRIER_SYNC(barrierSync, BUILD_RAYTRACING_ACCELERATION_STRUCTURE);
	barrierSyncs |= APPEND_BARRIER_SYNC(barrierSync, COPY_RAYTRACING_ACCELERATION_STRUCTURE);
	barrierSyncs |= APPEND_BARRIER_SYNC(barrierSync, SPLIT);
	barrierSyncs |= APPEND_BARRIER_SYNC(barrierSync, CONVERT_LINEAR_ALGEBRA_MATRIX);

	return barrierSyncs;
}

D3D12_BARRIER_ACCESS XUSG::Ultimate::GetDX12BarrierAccess(BarrierAccess barrierAccess)
{
	static const D3D12_BARRIER_ACCESS barrierAccesses[] =
	{
		D3D12_BARRIER_ACCESS_VERTEX_BUFFER,
		D3D12_BARRIER_ACCESS_CONSTANT_BUFFER,
		D3D12_BARRIER_ACCESS_INDEX_BUFFER,
		D3D12_BARRIER_ACCESS_RENDER_TARGET,
		D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,
		D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE,
		D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ,
		D3D12_BARRIER_ACCESS_SHADER_RESOURCE,
		D3D12_BARRIER_ACCESS_STREAM_OUTPUT,
		D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT,
		D3D12_BARRIER_ACCESS_COPY_DEST,
		D3D12_BARRIER_ACCESS_COPY_SOURCE,
		D3D12_BARRIER_ACCESS_RESOLVE_DEST,
		D3D12_BARRIER_ACCESS_RESOLVE_SOURCE,
		D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ,
		D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE,
		D3D12_BARRIER_ACCESS_SHADING_RATE_SOURCE,
		D3D12_BARRIER_ACCESS_VIDEO_DECODE_READ,
		D3D12_BARRIER_ACCESS_VIDEO_DECODE_WRITE,
		D3D12_BARRIER_ACCESS_VIDEO_PROCESS_READ,
		D3D12_BARRIER_ACCESS_VIDEO_PROCESS_WRITE,
		D3D12_BARRIER_ACCESS_VIDEO_ENCODE_READ,
		D3D12_BARRIER_ACCESS_VIDEO_ENCODE_WRITE,
		D3D12_BARRIER_ACCESS_NO_ACCESS
	};

	assert(barrierAccesses[Log2(static_cast<uint32_t>(BarrierAccess::VERTEX_BUFFER))] == D3D12_BARRIER_ACCESS_VERTEX_BUFFER);
	assert(barrierAccesses[Log2(static_cast<uint32_t>(BarrierAccess::CONSTANT_BUFFER))] == D3D12_BARRIER_ACCESS_CONSTANT_BUFFER);
	assert(barrierAccesses[Log2(static_cast<uint32_t>(BarrierAccess::INDEX_BUFFER))] == D3D12_BARRIER_ACCESS_INDEX_BUFFER);
	assert(barrierAccesses[Log2(static_cast<uint32_t>(BarrierAccess::RENDER_TARGET))] == D3D12_BARRIER_ACCESS_RENDER_TARGET);
	assert(barrierAccesses[Log2(static_cast<uint32_t>(BarrierAccess::UNORDERED_ACCESS))] == D3D12_BARRIER_ACCESS_UNORDERED_ACCESS);
	assert(barrierAccesses[Log2(static_cast<uint32_t>(BarrierAccess::DEPTH_STENCIL_WRITE))] == D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE);
	assert(barrierAccesses[Log2(static_cast<uint32_t>(BarrierAccess::DEPTH_STENCIL_READ))] == D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ);
	assert(barrierAccesses[Log2(static_cast<uint32_t>(BarrierAccess::SHADER_RESOURCE))] == D3D12_BARRIER_ACCESS_SHADER_RESOURCE);
	assert(barrierAccesses[Log2(static_cast<uint32_t>(BarrierAccess::STREAM_OUTPUT))] == D3D12_BARRIER_ACCESS_STREAM_OUTPUT);
	assert(barrierAccesses[Log2(static_cast<uint32_t>(BarrierAccess::INDIRECT_ARGUMENT))] == D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT);
	assert(barrierAccesses[Log2(static_cast<uint32_t>(BarrierAccess::COPY_DEST))] == D3D12_BARRIER_ACCESS_COPY_DEST);
	assert(barrierAccesses[Log2(static_cast<uint32_t>(BarrierAccess::COPY_SOURCE))] == D3D12_BARRIER_ACCESS_COPY_SOURCE);
	assert(barrierAccesses[Log2(static_cast<uint32_t>(BarrierAccess::RESOLVE_DEST))] == D3D12_BARRIER_ACCESS_RESOLVE_DEST);
	assert(barrierAccesses[Log2(static_cast<uint32_t>(BarrierAccess::RESOLVE_SOURCE))] == D3D12_BARRIER_ACCESS_RESOLVE_SOURCE);
	assert(barrierAccesses[Log2(static_cast<uint32_t>(BarrierAccess::RAYTRACING_ACCELERATION_STRUCTURE_READ))] == D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ);
	assert(barrierAccesses[Log2(static_cast<uint32_t>(BarrierAccess::RAYTRACING_ACCELERATION_STRUCTURE_WRITE))] == D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE);
	assert(barrierAccesses[Log2(static_cast<uint32_t>(BarrierAccess::SHADING_RATE_SOURCE))] == D3D12_BARRIER_ACCESS_SHADING_RATE_SOURCE);
	assert(barrierAccesses[Log2(static_cast<uint32_t>(BarrierAccess::VIDEO_DECODE_READ))] == D3D12_BARRIER_ACCESS_VIDEO_DECODE_READ);
	assert(barrierAccesses[Log2(static_cast<uint32_t>(BarrierAccess::VIDEO_DECODE_WRITE))] == D3D12_BARRIER_ACCESS_VIDEO_DECODE_WRITE);
	assert(barrierAccesses[Log2(static_cast<uint32_t>(BarrierAccess::VIDEO_PROCESS_READ))] == D3D12_BARRIER_ACCESS_VIDEO_PROCESS_READ);
	assert(barrierAccesses[Log2(static_cast<uint32_t>(BarrierAccess::VIDEO_PROCESS_WRITE))] == D3D12_BARRIER_ACCESS_VIDEO_PROCESS_WRITE);
	assert(barrierAccesses[Log2(static_cast<uint32_t>(BarrierAccess::VIDEO_ENCODE_READ))] == D3D12_BARRIER_ACCESS_VIDEO_ENCODE_READ);
	assert(barrierAccesses[Log2(static_cast<uint32_t>(BarrierAccess::VIDEO_ENCODE_WRITE))] == D3D12_BARRIER_ACCESS_VIDEO_ENCODE_WRITE);
	assert(barrierAccesses[Log2(static_cast<uint32_t>(BarrierAccess::NO_ACCESS))] == D3D12_BARRIER_ACCESS_NO_ACCESS);
	assert(barrierAccesses[Log2(static_cast<uint32_t>(BarrierAccess::PREDICATION))] == D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT);

	if (barrierAccess == BarrierAccess::COMMON) return D3D12_BARRIER_ACCESS_COMMON;

	const auto index = Log2(static_cast<uint32_t>(barrierAccess));

	return barrierAccesses[index];
}

D3D12_BARRIER_ACCESS XUSG::Ultimate::GetDX12BarrierAccesses(BarrierAccess barrierAccess)
{
	auto barrierAccesses = D3D12_BARRIER_ACCESS_COMMON;
	barrierAccesses |= APPEND_BARRIER_ACCESS(barrierAccess, VERTEX_BUFFER);
	barrierAccesses |= APPEND_BARRIER_ACCESS(barrierAccess, CONSTANT_BUFFER);
	barrierAccesses |= APPEND_BARRIER_ACCESS(barrierAccess, INDEX_BUFFER);
	barrierAccesses |= APPEND_BARRIER_ACCESS(barrierAccess, RENDER_TARGET);
	barrierAccesses |= APPEND_BARRIER_ACCESS(barrierAccess, UNORDERED_ACCESS);
	barrierAccesses |= APPEND_BARRIER_ACCESS(barrierAccess, DEPTH_STENCIL_WRITE);
	barrierAccesses |= APPEND_BARRIER_ACCESS(barrierAccess, DEPTH_STENCIL_READ);
	barrierAccesses |= APPEND_BARRIER_ACCESS(barrierAccess, SHADER_RESOURCE);
	barrierAccesses |= APPEND_BARRIER_ACCESS(barrierAccess, STREAM_OUTPUT);
	barrierAccesses |= APPEND_BARRIER_ACCESS(barrierAccess, INDIRECT_ARGUMENT);
	barrierAccesses |= APPEND_BARRIER_ACCESS(barrierAccess, COPY_DEST);
	barrierAccesses |= APPEND_BARRIER_ACCESS(barrierAccess, COPY_SOURCE);
	barrierAccesses |= APPEND_BARRIER_ACCESS(barrierAccess, RESOLVE_DEST);
	barrierAccesses |= APPEND_BARRIER_ACCESS(barrierAccess, RESOLVE_SOURCE);
	barrierAccesses |= APPEND_BARRIER_ACCESS(barrierAccess, RAYTRACING_ACCELERATION_STRUCTURE_READ);
	barrierAccesses |= APPEND_BARRIER_ACCESS(barrierAccess, RAYTRACING_ACCELERATION_STRUCTURE_WRITE);
	barrierAccesses |= APPEND_BARRIER_ACCESS(barrierAccess, SHADING_RATE_SOURCE);
	barrierAccesses |= APPEND_BARRIER_ACCESS(barrierAccess, VIDEO_DECODE_READ);
	barrierAccesses |= APPEND_BARRIER_ACCESS(barrierAccess, VIDEO_DECODE_WRITE);
	barrierAccesses |= APPEND_BARRIER_ACCESS(barrierAccess, VIDEO_PROCESS_READ);
	barrierAccesses |= APPEND_BARRIER_ACCESS(barrierAccess, VIDEO_PROCESS_WRITE);
	barrierAccesses |= APPEND_BARRIER_ACCESS(barrierAccess, VIDEO_ENCODE_READ);
	barrierAccesses |= APPEND_BARRIER_ACCESS(barrierAccess, VIDEO_ENCODE_WRITE);
	barrierAccesses |= APPEND_BARRIER_ACCESS(barrierAccess, NO_ACCESS);

	return barrierAccesses;
}

D3D12_TEXTURE_BARRIER_FLAGS XUSG::Ultimate::GetDX12TextureBarrierFlag(TextureBarrierFlag textureBarrierFlag)
{
	static const D3D12_TEXTURE_BARRIER_FLAGS textureBarrierFlags[] =
	{
		D3D12_TEXTURE_BARRIER_FLAG_DISCARD
	};

	assert(textureBarrierFlags[Log2(static_cast<uint32_t>(TextureBarrierFlag::DISCARD))] == D3D12_TEXTURE_BARRIER_FLAG_DISCARD);

	if (textureBarrierFlag == TextureBarrierFlag::NONE) return D3D12_TEXTURE_BARRIER_FLAG_NONE;

	const auto index = Log2(static_cast<uint32_t>(textureBarrierFlag));

	return textureBarrierFlags[index];
}

D3D12_TEXTURE_BARRIER_FLAGS XUSG::Ultimate::GetDX12TextureBarrierFlags(TextureBarrierFlag textureBarrierFlags)
{
	auto flags = D3D12_TEXTURE_BARRIER_FLAG_NONE;
	flags |= APPEND_TEXTURE_BARRIER_FLAG(textureBarrierFlags, DISCARD);

	return flags;
}

D3D12_SHADING_RATE_COMBINER XUSG::Ultimate::GetDX12ShadingRateCombiner(ShadingRateCombiner combiner)
{
	static const D3D12_SHADING_RATE_COMBINER combiners[] =
	{
		D3D12_SHADING_RATE_COMBINER_PASSTHROUGH,
		D3D12_SHADING_RATE_COMBINER_OVERRIDE,
		D3D12_SHADING_RATE_COMBINER_MIN,
		D3D12_SHADING_RATE_COMBINER_MAX,
		D3D12_SHADING_RATE_COMBINER_SUM
	};

	assert(combiners[static_cast<uint32_t>(ShadingRateCombiner::COMBINER_PASSTHROUGH)] == D3D12_SHADING_RATE_COMBINER_PASSTHROUGH);
	assert(combiners[static_cast<uint32_t>(ShadingRateCombiner::COMBINER_OVERRIDE)] == D3D12_SHADING_RATE_COMBINER_OVERRIDE);
	assert(combiners[static_cast<uint32_t>(ShadingRateCombiner::COMBINER_MIN)] == D3D12_SHADING_RATE_COMBINER_MIN);
	assert(combiners[static_cast<uint32_t>(ShadingRateCombiner::COMBINER_MAX)] == D3D12_SHADING_RATE_COMBINER_MAX);
	assert(combiners[static_cast<uint32_t>(ShadingRateCombiner::COMBINER_SUM)] == D3D12_SHADING_RATE_COMBINER_SUM);

	return combiners[static_cast<uint32_t>(combiner)];
}

D3D12_RESOLVE_MODE XUSG::Ultimate::GetDX12ResolveMode(ResolveMode mode)
{
	static const D3D12_RESOLVE_MODE modes[] =
	{
		D3D12_RESOLVE_MODE_DECOMPRESS,
		D3D12_RESOLVE_MODE_MIN,
		D3D12_RESOLVE_MODE_MAX,
		D3D12_RESOLVE_MODE_AVERAGE,
		D3D12_RESOLVE_MODE_ENCODE_SAMPLER_FEEDBACK,
		D3D12_RESOLVE_MODE_DECODE_SAMPLER_FEEDBACK
	};

	assert(modes[static_cast<uint32_t>(ResolveMode::DECOMPRESS)] == D3D12_RESOLVE_MODE_DECOMPRESS);
	assert(modes[static_cast<uint32_t>(ResolveMode::MIN)] == D3D12_RESOLVE_MODE_MIN);
	assert(modes[static_cast<uint32_t>(ResolveMode::MAX)] == D3D12_RESOLVE_MODE_MAX);
	assert(modes[static_cast<uint32_t>(ResolveMode::AVERAGE)] == D3D12_RESOLVE_MODE_AVERAGE);
	assert(modes[static_cast<uint32_t>(ResolveMode::ENCODE_SAMPLER_FEEDBACK)] == D3D12_RESOLVE_MODE_ENCODE_SAMPLER_FEEDBACK);
	assert(modes[static_cast<uint32_t>(ResolveMode::DECODE_SAMPLER_FEEDBACK)] == D3D12_RESOLVE_MODE_DECODE_SAMPLER_FEEDBACK);

	return modes[static_cast<uint32_t>(mode)];
}

D3D12_VIEW_INSTANCING_FLAGS XUSG::Ultimate::GetDX12ViewInstanceFlag(ViewInstanceFlag viewInstanceFlag)
{
	static const D3D12_VIEW_INSTANCING_FLAGS viewInstanceFlags[] =
	{
		D3D12_VIEW_INSTANCING_FLAG_ENABLE_VIEW_INSTANCE_MASKING
	};

	assert(viewInstanceFlags[Log2(static_cast<uint32_t>(ViewInstanceFlag::ENABLE_VIEW_INSTANCE_MASKING))] == D3D12_VIEW_INSTANCING_FLAG_ENABLE_VIEW_INSTANCE_MASKING);

	if (viewInstanceFlag == ViewInstanceFlag::NONE) return D3D12_VIEW_INSTANCING_FLAG_NONE;

	const auto index = Log2(static_cast<uint32_t>(viewInstanceFlag));

	return viewInstanceFlags[index];
}

D3D12_VIEW_INSTANCING_FLAGS XUSG::Ultimate::GetDX12ViewInstanceFlags(ViewInstanceFlag viewInstanceFlags)
{
	auto flags = D3D12_VIEW_INSTANCING_FLAG_NONE;
	flags |= APPEND_VIEW_INSTANCE_FLAG(viewInstanceFlags, ENABLE_VIEW_INSTANCE_MASKING);

	return flags;
}

D3D12_SET_WORK_GRAPH_FLAGS XUSG::Ultimate::GetDX12WorkGraphFlag(WorkGraphFlag workGraphFlag)
{
	static const D3D12_SET_WORK_GRAPH_FLAGS workGraphFlags[] =
	{
		D3D12_SET_WORK_GRAPH_FLAG_INITIALIZE
	};

	assert(workGraphFlags[Log2(static_cast<uint32_t>(WorkGraphFlag::INITIALIZE))] == D3D12_SET_WORK_GRAPH_FLAG_INITIALIZE);

	if (workGraphFlag == WorkGraphFlag::NONE) return D3D12_SET_WORK_GRAPH_FLAG_NONE;

	const auto index = Log2(static_cast<uint32_t>(workGraphFlag));

	return workGraphFlags[index];
}

D3D12_SET_WORK_GRAPH_FLAGS XUSG::Ultimate::GetDX12WorkGraphFlags(WorkGraphFlag workGraphFlags)
{
	auto flags = D3D12_SET_WORK_GRAPH_FLAG_NONE;
	flags |= APPEND_WORK_GRAPH_FLAG(workGraphFlags, INITIALIZE);

	return flags;
}

D3D12_LINEAR_ALGEBRA_MATRIX_LAYOUT XUSG::Ultimate::GetDX12LinearAlgebraMatrixLayout(LinearAlgebraMatrixLayout layout)
{
	static const D3D12_LINEAR_ALGEBRA_MATRIX_LAYOUT layouts[] =
	{
		D3D12_LINEAR_ALGEBRA_MATRIX_LAYOUT_ROW_MAJOR,
		D3D12_LINEAR_ALGEBRA_MATRIX_LAYOUT_COLUMN_MAJOR,
		D3D12_LINEAR_ALGEBRA_MATRIX_LAYOUT_MUL_OPTIMAL,
		D3D12_LINEAR_ALGEBRA_MATRIX_LAYOUT_OUTER_PRODUCT_OPTIMAL
	};

	assert(layouts[static_cast<uint32_t>(LinearAlgebraMatrixLayout::ROW_MAJOR)] == D3D12_LINEAR_ALGEBRA_MATRIX_LAYOUT_ROW_MAJOR);
	assert(layouts[static_cast<uint32_t>(LinearAlgebraMatrixLayout::COLUMN_MAJOR)] == D3D12_LINEAR_ALGEBRA_MATRIX_LAYOUT_COLUMN_MAJOR);
	assert(layouts[static_cast<uint32_t>(LinearAlgebraMatrixLayout::MUL_OPTIMAL)] == D3D12_LINEAR_ALGEBRA_MATRIX_LAYOUT_MUL_OPTIMAL);
	assert(layouts[static_cast<uint32_t>(LinearAlgebraMatrixLayout::OUTER_PRODUCT_OPTIMAL)] == D3D12_LINEAR_ALGEBRA_MATRIX_LAYOUT_OUTER_PRODUCT_OPTIMAL);

	return layouts[static_cast<uint32_t>(layout)];
}

D3D12_LINEAR_ALGEBRA_DATATYPE XUSG::Ultimate::GetDX12LinearAlgebraDataType(LinearAlgebraDataType dataType)
{
	static const D3D12_LINEAR_ALGEBRA_DATATYPE dataTypes[] =
	{
		D3D12_LINEAR_ALGEBRA_DATATYPE_SINT16,
		D3D12_LINEAR_ALGEBRA_DATATYPE_UINT16,
		D3D12_LINEAR_ALGEBRA_DATATYPE_SINT32,
		D3D12_LINEAR_ALGEBRA_DATATYPE_UINT32,
		D3D12_LINEAR_ALGEBRA_DATATYPE_FLOAT16,
		D3D12_LINEAR_ALGEBRA_DATATYPE_FLOAT32,
		D3D12_LINEAR_ALGEBRA_DATATYPE_UINT8,
		D3D12_LINEAR_ALGEBRA_DATATYPE_SINT8,
		D3D12_LINEAR_ALGEBRA_DATATYPE_FLOAT8_E4M3FN,
		D3D12_LINEAR_ALGEBRA_DATATYPE_FLOAT8_E5M2
	};

	assert(dataTypes[static_cast<uint32_t>(LinearAlgebraDataType::SINT16)] == D3D12_LINEAR_ALGEBRA_DATATYPE_SINT16);
	assert(dataTypes[static_cast<uint32_t>(LinearAlgebraDataType::UINT16)] == D3D12_LINEAR_ALGEBRA_DATATYPE_UINT16);
	assert(dataTypes[static_cast<uint32_t>(LinearAlgebraDataType::SINT32)] == D3D12_LINEAR_ALGEBRA_DATATYPE_SINT32);
	assert(dataTypes[static_cast<uint32_t>(LinearAlgebraDataType::UINT32)] == D3D12_LINEAR_ALGEBRA_DATATYPE_UINT32);
	assert(dataTypes[static_cast<uint32_t>(LinearAlgebraDataType::FLOAT16)] == D3D12_LINEAR_ALGEBRA_DATATYPE_FLOAT16);
	assert(dataTypes[static_cast<uint32_t>(LinearAlgebraDataType::FLOAT32)] == D3D12_LINEAR_ALGEBRA_DATATYPE_FLOAT32);
	assert(dataTypes[static_cast<uint32_t>(LinearAlgebraDataType::UINT8)] == D3D12_LINEAR_ALGEBRA_DATATYPE_UINT8);
	assert(dataTypes[static_cast<uint32_t>(LinearAlgebraDataType::SINT8)] == D3D12_LINEAR_ALGEBRA_DATATYPE_SINT8);
	assert(dataTypes[static_cast<uint32_t>(LinearAlgebraDataType::FLOAT8_E4M3FN)] == D3D12_LINEAR_ALGEBRA_DATATYPE_FLOAT8_E4M3FN);
	assert(dataTypes[static_cast<uint32_t>(LinearAlgebraDataType::FLOAT8_E5M2)] == D3D12_LINEAR_ALGEBRA_DATATYPE_FLOAT8_E5M2);

	return dataTypes[static_cast<uint32_t>(dataType)];
}
