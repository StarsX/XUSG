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
	m_commandList = dynamic_cast<XUSG::CommandList_DX12&>(commandList).GetGraphicsCommandList();
	CreateInterface();
}

CommandList_DX12::~CommandList_DX12()
{
}

bool CommandList_DX12::CreateInterface()
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

void CommandList_DX12::Barrier(uint32_t numBarrierGroups, BarrierGroup* pBarrierGroups)
{
	if (numBarrierGroups)
	{
		assert(pBarrierGroups);
		if (m_barrierGroups.size() < numBarrierGroups) m_barrierGroups.resize(numBarrierGroups);
		if (m_barrierGroupBarrierStarts.size() < numBarrierGroups) m_barrierGroupBarrierStarts.resize(numBarrierGroups);
		m_globalBarriers.clear();
		m_textureBarriers.clear();
		m_bufferBarriers.clear();

		for (auto i = 0u; i < numBarrierGroups; ++i)
		{
			const auto& barrierGroup = pBarrierGroups[i];
			if (barrierGroup.NumBarriers)
			{
				assert(barrierGroup.pBarriers);
				auto& dxBarrierGroup = m_barrierGroups[i];
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
						textureBarrier.pResource = static_cast<ID3D12Resource*>(barrier.pResource->GetHandle());
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
						bufferBarrier.pResource = static_cast<ID3D12Resource*>(barrier.pResource->GetHandle());
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
		}

		m_commandListA->Barrier(numBarrierGroups, m_barrierGroups.data());
	}
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
			bufferBarrier.pResource = static_cast<ID3D12Resource*>(barrier.pResource->GetHandle());
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
			textureBarrier.pResource = static_cast<ID3D12Resource*>(barrier.pResource->GetHandle());
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

void CommandList_DX12::ResolveSubresourceRegion(const Resource* pDstResource, uint32_t dstSubresource, uint32_t dstX, uint32_t dstY,
	const Resource* pSrcResource, uint32_t srcSubresource, const RectRange& srcRect, Format format, ResolveMode resolveMode) const
{
	assert(pDstResource);
	assert(pSrcResource);

	D3D12_RECT rect = { srcRect.Left, srcRect.Top, srcRect.Right, srcRect.Bottom };

	m_commandListU->ResolveSubresourceRegion(static_cast<ID3D12Resource*>(pDstResource->GetHandle()),
		dstSubresource, dstX, dstY, static_cast<ID3D12Resource*>(pSrcResource->GetHandle()), srcSubresource,
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

void CommandList_DX12::RSSetShadingRateImage(const Resource* pShadingRateImage)  const
{
	m_commandListU->RSSetShadingRateImage(pShadingRateImage ?
		static_cast<ID3D12Resource*>(pShadingRateImage->GetHandle()) : nullptr);
}

void CommandList_DX12::DispatchMesh(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)  const
{
	m_commandListU->DispatchMesh(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}

void CommandList_DX12::SetStateObject(const Pipeline& stateObject)
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

XUSG::com_ptr<ID3D12GraphicsCommandList6>& CommandList_DX12::GetGraphicsCommandList()
{
	return m_commandListU;
}

void CommandList_DX12::createAgilityInterface()
{
	if (m_commandListA == nullptr)
	{
		const auto hr = m_commandList->QueryInterface(IID_PPV_ARGS(&m_commandListA));

		if (FAILED(hr)) OutputDebugString(L"Couldn't get DirectX agility interface for the command list.\n");
	}
}

//--------------------------------------------------------------------------------------
// Pipeline layout
//--------------------------------------------------------------------------------------

PipelineLayoutLib_DX12::PipelineLayoutLib_DX12() :
	XUSG::PipelineLayoutLib_DX12()
{
}

PipelineLayoutLib_DX12::PipelineLayoutLib_DX12(const Device* pDevice) :
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
	if (layoutIter == m_rootSignatures.end())
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

bool SamplerFeedBack_DX12::Create(const Device* pDevice, const Texture* pTarget, Format format,
	uint32_t mipRegionWidth, uint32_t mipRegionHeight, uint32_t mipRegionDepth,
	ResourceFlag resourceFlags, bool isCubeMap, MemoryFlag memoryFlags, const wchar_t* name,
	uint16_t srvComponentMapping, TextureLayout textureLayout, uint32_t maxThreads)
{
	assert(format == Format::MIN_MIP_OPAQUE || format == Format::MIP_REGION_USED_OPAQUE);
	XUSG_N_RETURN(Initialize(pDevice, format), false);

	const auto hasSRV = (resourceFlags & ResourceFlag::DENY_SHADER_RESOURCE) == ResourceFlag::NONE;

	uint16_t arraySize;
	uint8_t numMips;

	if (pTarget)
	{
		// Get paired properties
		arraySize = pTarget->GetArraySize();
		numMips = pTarget->GetNumMips();

		XUSG_N_RETURN(CreateResource(pTarget, format, mipRegionWidth, mipRegionHeight, mipRegionDepth,
			resourceFlags, isCubeMap, memoryFlags, ResourceState::COMMON, textureLayout, maxThreads), false);
	}
	else arraySize = numMips = 1;

	SetName(name);

	// Allocate CBV SRV UAV heap
	uint32_t numDescriptors = 0;
	if (hasSRV) numDescriptors += max<uint8_t>(numMips, 1);
	if (pTarget) ++numDescriptors;
	const auto uavHeapStart = AllocateCbvSrvUavHeap(numDescriptors);
	auto descriptorIdx = 0u;

	// Create SRVs
	if (hasSRV)
		XUSG_N_RETURN(createSRVs(descriptorIdx, arraySize, m_format, numMips, false, isCubeMap, srvComponentMapping), false);

	// Create UAV
	if (pTarget)
	{
		m_uavs.resize(1);
		XUSG_X_RETURN(m_uavs[0], CreateUAV(uavHeapStart, descriptorIdx, pTarget), false);
	}

	return true;
}

bool SamplerFeedBack_DX12::Create(const Device* pDevice, const Heap* pHeap, uint64_t heapOffset,
	const Texture* pTarget, Format format, uint32_t mipRegionWidth, uint32_t mipRegionHeight,
	uint32_t mipRegionDepth, ResourceFlag resourceFlags, bool isCubeMap, const wchar_t* name,
	uint16_t srvComponentMapping, TextureLayout textureLayout, uint32_t maxThreads)
{
	assert(format == Format::MIN_MIP_OPAQUE || format == Format::MIP_REGION_USED_OPAQUE);
	XUSG_N_RETURN(Initialize(pDevice, format), false);

	const auto hasSRV = (resourceFlags & ResourceFlag::DENY_SHADER_RESOURCE) == ResourceFlag::NONE;

	uint16_t arraySize;
	uint8_t numMips;

	if (pTarget)
	{
		// Get paired properties
		arraySize = pTarget->GetArraySize();
		numMips = pTarget->GetNumMips();

		XUSG_N_RETURN(CreateResource(pHeap, heapOffset, pTarget, format, mipRegionWidth, mipRegionHeight,
			mipRegionDepth, resourceFlags, isCubeMap, ResourceState::COMMON, textureLayout, maxThreads), false);
	}
	else arraySize = numMips = 1;

	SetName(name);

	// Allocate CBV SRV UAV heap
	uint32_t numDescriptors = 0;
	if (hasSRV) numDescriptors += max<uint8_t>(numMips, 1);
	if (pTarget) ++numDescriptors;
	const auto uavHeapStart = AllocateCbvSrvUavHeap(numDescriptors);
	auto descriptorIdx = 0u;

	// Create SRVs
	if (hasSRV)
		XUSG_N_RETURN(createSRVs(descriptorIdx, arraySize, m_format, numMips, false, isCubeMap, srvComponentMapping), false);

	// Create UAV
	if (pTarget)
	{
		m_uavs.resize(1);
		XUSG_X_RETURN(m_uavs[0], CreateUAV(uavHeapStart, descriptorIdx, pTarget), false);
	}

	return true;
}

bool SamplerFeedBack_DX12::CreateResource(const Texture* pTarget, Format format,
	uint32_t mipRegionWidth, uint32_t mipRegionHeight, uint32_t mipRegionDepth,
	ResourceFlag resourceFlags, bool isCubeMap, MemoryFlag memoryFlags,
	ResourceState initialResourceState, TextureLayout textureLayout,
	uint32_t maxThreads)
{
	V_RETURN(m_device->QueryInterface(IID_PPV_ARGS(&m_deviceU)), cerr, false);

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

	V_RETURN(m_deviceU->CreateCommittedResource2(&heapProperties, GetDX12HeapFlags(memoryFlags), &desc,
		GetDX12ResourceStates(m_states[0][0]), nullptr, nullptr, IID_PPV_ARGS(&m_resource)), clog, false);

	return true;
}

bool SamplerFeedBack_DX12::CreateResource(const Heap* pHeap, uint64_t heapOffset, const Texture* pTarget,
	Format format, uint32_t mipRegionWidth, uint32_t mipRegionHeight, uint32_t mipRegionDepth,
	ResourceFlag resourceFlags, bool isCubeMap, ResourceState initialResourceState,
	TextureLayout textureLayout, uint32_t maxThreads)
{
	V_RETURN(m_device->QueryInterface(IID_PPV_ARGS(&m_deviceU)), cerr, false);

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
	V_RETURN(m_deviceU->CreatePlacedResource2(static_cast<ID3D12Heap*>(pHeap->GetHandle()), heapOffset, &desc,
		GetDX12BarrierLayout(GetBarrierLayout(m_states[0][0])), nullptr, 0, nullptr, IID_PPV_ARGS(&m_resource)),
		clog, false);

	return true;
}

XUSG::Descriptor SamplerFeedBack_DX12::CreateUAV(const Descriptor& uavHeapStart, uint32_t descriptorIdx, const Resource* pTarget)
{
	// Create an unordered access view
	assert(pTarget);
	const auto stride = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	const auto descriptor = uavHeapStart + stride * descriptorIdx;
	m_deviceU->CreateSamplerFeedbackUnorderedAccessView(static_cast<ID3D12Resource*>(pTarget->GetHandle()),
		m_resource.get(), { descriptor });

	return descriptor;
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
		D3D12_BARRIER_SYNC_SPLIT
	};

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

	return modes[static_cast<uint32_t>(mode)];
}

D3D12_VIEW_INSTANCING_FLAGS XUSG::Ultimate::GetDX12ViewInstanceFlag(ViewInstanceFlag viewInstanceFlag)
{
	static const D3D12_VIEW_INSTANCING_FLAGS viewInstanceFlags[] =
	{
		D3D12_VIEW_INSTANCING_FLAG_ENABLE_VIEW_INSTANCE_MASKING
	};

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
