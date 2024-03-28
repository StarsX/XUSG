//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSG_DX12.h"
#include "Core/XUSGCommand_DX12.h"
#include "Core/XUSGResource_DX12.h"
#include "Core/XUSGEnum_DX12.h"
#include "XUSGUltimate_DX12.h"

using namespace std;
using namespace XUSG::Ultimate;

#define APPEND_FLAG(type, dx12Type, flags, flag, none) (static_cast<bool>(flags & type::flag) ? dx12Type##_##flag : dx12Type##_##none)
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

void CommandList_DX12::SetProgram(ProgramType type, ProgramIdentifier identifier, WorkGraphFlag flags,
	uint64_t backingMemoryAddress, uint64_t backingMemoryByteSize, uint64_t localRootArgTableAddress,
	uint64_t localRootArgTableByteSize, uint64_t localRootArgTableByteStride) const
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

	m_commandListU->SetProgram(&desc);
}

void CommandList_DX12::DispatchGraph(uint32_t numNodeInputs, const NodeCPUInput* pNodeInputs, uint64_t nodeInputByteStride) const
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
		desc.Mode = D3D12_DISPATCH_MODE_MULTI_NODE_CPU_INPUT;
		desc.MultiNodeCPUInput.NumNodeInputs = numNodeInputs;
		desc.MultiNodeCPUInput.NodeInputStrideInBytes = nodeInputByteStride ? nodeInputByteStride : sizeof(D3D12_NODE_CPU_INPUT);
		for (auto i = 0u; i < numNodeInputs; ++i)
			populateNodeInput(desc.MultiNodeCPUInput.pNodeInputs[i], pNodeInputs[i]);
	}

	m_commandListU->DispatchGraph(&desc);
}

void CommandList_DX12::DispatchGraph(uint64_t nodeGPUInputAddress, bool isMultiNodes) const
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

	m_commandListU->DispatchGraph(&desc);
}

XUSG::com_ptr<ID3D12GraphicsCommandList10>& CommandList_DX12::GetGraphicsCommandList()
{
	return m_commandListU;
}

XUSG::ProgramIdentifier XUSG::Ultimate::GetProgramIdentifierFromDX12(const XUSG::Pipeline& stateObject, const wchar_t* programName)
{
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

//--------------------------------------------------------------------------------------
// DX12 enum transfer functions
//--------------------------------------------------------------------------------------

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
