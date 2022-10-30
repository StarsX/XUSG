//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSGCommand_DX12.h"
#include "Core/XUSGResource_DX12.h"
#include "Ultimate/XUSGUltimate_DX12.h"
#include "RayTracing/XUSGRayTracingCommand_DX12.h"
#include "RayTracing/XUSGRayTracingState_DX12.h"
#include "XUSG-EZ_DX12.h"
#include "XUSGUltimate-EZ_DX12.h"
#include "XUSGRayTracing-EZ_DX12.h"

using namespace std;
using namespace XUSG::RayTracing;

EZ::CommandList_DXR::CommandList_DXR() :
	Ultimate::CommandList_DX12(),
	Ultimate::EZ::CommandList_DX12(),
	m_RayTracingPipelineLib(nullptr),
	m_scratchSize(0),
	m_scratches(0),
	m_isRTStateDirty(false),
	m_rayTracingState(nullptr),
	m_asUavCount(0),
	m_tlasBindingToParamIndexMap(0)
{
}

EZ::CommandList_DXR::CommandList_DXR(RayTracing::CommandList* pCommandList,
	uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
	const uint32_t maxSamplers[Shader::Stage::NUM_STAGE],
	const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_STAGE],
	const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_STAGE],
	const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_STAGE],
	const uint32_t maxCbvSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t maxSrvSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t maxUavSpaces[Shader::Stage::NUM_STAGE],
	uint32_t maxTLASSrvs, uint32_t spaceTLAS) :
	CommandList_DXR()
{
	Create(pCommandList, samplerPoolSize, cbvSrvUavPoolSize, maxSamplers,
		pMaxCbvsEachSpace, pMaxSrvsEachSpace, pMaxUavsEachSpace,
		maxCbvSpaces, maxSrvSpaces, maxUavSpaces, maxTLASSrvs, spaceTLAS);
}

EZ::CommandList_DXR::~CommandList_DXR()
{
}

bool EZ::CommandList_DXR::Create(RayTracing::CommandList* pCommandList,
	uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
	const uint32_t maxSamplers[Shader::Stage::NUM_STAGE],
	const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_STAGE],
	const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_STAGE],
	const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_STAGE],
	const uint32_t maxCbvSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t maxSrvSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t maxUavSpaces[Shader::Stage::NUM_STAGE],
	uint32_t maxTLASSrvs, uint32_t spaceTLAS)
{
	XUSG_N_RETURN(Ultimate::EZ::CommandList_DX12::init(pCommandList, samplerPoolSize, cbvSrvUavPoolSize), false);

	m_pDeviceRT = pCommandList->GetRTDevice();

	const auto pDxDevice = static_cast<ID3D12RaytracingFallbackDevice*>(m_pDeviceRT->GetRTHandle());
	XUSG_N_RETURN(pDxDevice, false);
	pDxDevice->QueryRaytracingCommandList(m_commandList.get(), IID_PPV_ARGS(&m_commandListRT));

	m_RayTracingPipelineLib = PipelineLib::MakeUnique(m_pDeviceRT, API::DIRECTX_12);
	m_rayTracingState = State::MakeUnique(API::DIRECTX_12);

	// Create common pipeline layouts
	XUSG_N_RETURN(createGraphicsPipelineLayouts(maxSamplers, pMaxCbvsEachSpace, pMaxSrvsEachSpace,
		pMaxUavsEachSpace, maxCbvSpaces, maxSrvSpaces, maxUavSpaces), false);

	XUSG_N_RETURN(createComputePipelineLayouts(maxSamplers ? maxSamplers[Shader::Stage::CS] : 16,
		pMaxCbvsEachSpace ? pMaxCbvsEachSpace[Shader::Stage::CS] : nullptr,
		pMaxSrvsEachSpace ? pMaxSrvsEachSpace[Shader::Stage::CS] : nullptr,
		pMaxUavsEachSpace ? pMaxUavsEachSpace[Shader::Stage::CS] : nullptr,
		maxCbvSpaces ? maxCbvSpaces[Shader::Stage::CS] : 1,
		maxSrvSpaces ? maxSrvSpaces[Shader::Stage::CS] : 1,
		maxUavSpaces ? maxUavSpaces[Shader::Stage::CS] : 1,
		maxTLASSrvs, spaceTLAS), false);

	D3D12_FEATURE_DATA_D3D12_OPTIONS7 featureSupportData = {};
	const auto hr = static_cast<ID3D12Device*>(m_pDevice->GetHandle())->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7,
		&featureSupportData, sizeof(featureSupportData));

	if (m_commandListU && SUCCEEDED(hr) && featureSupportData.MeshShaderTier != D3D12_MESH_SHADER_TIER_NOT_SUPPORTED)
		XUSG_N_RETURN(createMeshShaderPipelineLayouts(maxSamplers, pMaxCbvsEachSpace, pMaxSrvsEachSpace,
			pMaxUavsEachSpace, maxCbvSpaces, maxSrvSpaces, maxUavSpaces), false);

	return true;
}

bool EZ::CommandList_DXR::Create(const RayTracing::Device* pDevice, void* pHandle,
	uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
	const uint32_t maxSamplers[Shader::Stage::NUM_STAGE],
	const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_STAGE],
	const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_STAGE],
	const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_STAGE],
	const uint32_t maxCbvSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t maxSrvSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t maxUavSpaces[Shader::Stage::NUM_STAGE],
	uint32_t maxTLASSrvs, uint32_t spaceTLAS, const wchar_t* name)
{
	m_pDeviceRT = pDevice;
	RayTracing::CommandList_DX12::Create(pHandle, name);

	return Create(this, samplerPoolSize, cbvSrvUavPoolSize, maxSamplers,
		pMaxCbvsEachSpace, pMaxSrvsEachSpace, pMaxUavsEachSpace,
		maxCbvSpaces, maxSrvSpaces, maxUavSpaces, maxTLASSrvs, spaceTLAS);
}

bool EZ::CommandList_DXR::Reset(const CommandAllocator* pAllocator, const Pipeline& initialState)
{
	if (!m_scratches.empty())
	{
		auto scratch = move(m_scratches.back());
		m_scratches.resize(1);
		m_scratches[0] = move(scratch);
	}

	m_uploaders.clear();

	return XUSG::EZ::CommandList_DX12::Reset(pAllocator, initialState);
}

bool EZ::CommandList_DXR::PreBuildBLAS(BottomLevelAS* pBLAS, uint32_t numGeometries,
	const GeometryBuffer& geometries, BuildFlag flags)
{
	// Set barrier command
	XUSG::CommandList_DX12::Barrier(static_cast<uint32_t>(m_barriers.size()), m_barriers.data());
	m_barriers.clear();

	assert(pBLAS);
	XUSG_N_RETURN(pBLAS->PreBuild(m_pDeviceRT, numGeometries, geometries, m_asUavCount++, flags), false);

	const auto descriptorTable = Util::DescriptorTable::MakeUnique(API::DIRECTX_12);
	descriptorTable->SetDescriptors(0, 1, &pBLAS->GetResult()->GetUAV());
	const auto uavTable = descriptorTable->GetCbvSrvUavTable(m_descriptorTableLib.get());
	XUSG_N_RETURN(uavTable, false);

	m_scratchSize = (max)(m_scratchSize, pBLAS->GetScratchDataMaxSize());

	return true;
}

bool EZ::CommandList_DXR::PreBuildTLAS(TopLevelAS* pTLAS, uint32_t numInstances, BuildFlag flags)
{
	// Set barrier command
	XUSG::CommandList_DX12::Barrier(static_cast<uint32_t>(m_barriers.size()), m_barriers.data());
	m_barriers.clear();

	assert(pTLAS);
	XUSG_N_RETURN(pTLAS->PreBuild(m_pDeviceRT, numInstances, m_asUavCount++, flags), false);

	const auto descriptorTable = Util::DescriptorTable::MakeUnique(API::DIRECTX_12);
	descriptorTable->SetDescriptors(0, 1, &pTLAS->GetResult()->GetUAV());
	const auto uavTable = descriptorTable->GetCbvSrvUavTable(m_descriptorTableLib.get());
	XUSG_N_RETURN(uavTable, false);

	m_scratchSize = (max)(m_scratchSize, pTLAS->GetScratchDataMaxSize());

	return true;
}

void EZ::CommandList_DXR::SetTriangleGeometries(GeometryBuffer& geometries, uint32_t numGeometries,
	Format vertexFormat, XUSG::EZ::VertexBufferView* pVBs, XUSG::EZ::IndexBufferView* pIBs,
	const GeometryFlag* pGeometryFlags, const ResourceView* pTransforms)
{
	vector<VertexBufferView> vbvs(numGeometries);
	vector<IndexBufferView> ibvs;

	// Set barriers if the vertex buffers or index buffers are not at the read states
	const auto startIdx = m_barriers.size();
	m_barriers.resize(startIdx + numGeometries * 2);
	auto numBarriers = 0u;
	for (auto i = 0u; i < numGeometries; ++i)
	{
		const auto& vb = pVBs[i];
		const auto pBarrier = &m_barriers[startIdx];
		if (vb.pResource->GetResourceState() == ResourceState::COMMON)
			vb.pResource->SetBarrier(pBarrier, ResourceState::COPY_DEST, numBarriers);
		numBarriers = vb.pResource->SetBarrier(pBarrier, vb.DstState, numBarriers);
		vbvs[i] = *vb.pView;
	}

	if (pIBs)
	{
		ibvs.resize(numGeometries);
		for (auto i = 0u; i < numGeometries; ++i)
		{
			const auto& ib = pIBs[i];
			const auto pBarrier = &m_barriers[startIdx];
			if (ib.pResource->GetResourceState() == ResourceState::COMMON)
				ib.pResource->SetBarrier(pBarrier, ResourceState::COPY_DEST, numBarriers);
			numBarriers = ib.pResource->SetBarrier(pBarrier, ib.DstState, numBarriers);
			ibvs[i] = *ib.pView;
		}
	}

	// Shrink the size of barrier list
	if (numBarriers < numGeometries * 2) m_barriers.resize(startIdx + numBarriers);

	// Set geometries
	BottomLevelAS::SetTriangleGeometries(geometries, numGeometries, vertexFormat,
		vbvs.data(), pIBs ? ibvs.data() : nullptr, pGeometryFlags, pTransforms);
}

void EZ::CommandList_DXR::SetAABBGeometries(GeometryBuffer& geometries, uint32_t numGeometries,
	XUSG::EZ::VertexBufferView* pVBs, const GeometryFlag* pGeometryFlags)
{
	vector<VertexBufferView> vbvs(numGeometries);

	// Set barriers if the vertex buffers or index buffers are not at the read states
	const auto startIdx = m_barriers.size();
	m_barriers.resize(startIdx + numGeometries);
	auto numBarriers = 0u;
	for (auto i = 0u; i < numGeometries; ++i)
	{
		const auto& vb = pVBs[i];
		const auto pBarrier = &m_barriers[startIdx];
		if (vb.pResource->GetResourceState() == ResourceState::COMMON)
			vb.pResource->SetBarrier(pBarrier, ResourceState::COPY_DEST, numBarriers);
		numBarriers = vb.pResource->SetBarrier(pBarrier, vb.DstState, numBarriers);
		vbvs[i] = *vb.pView;
	}

	// Shrink the size of barrier list
	if (numBarriers < numGeometries) m_barriers.resize(startIdx + numBarriers);

	// Set geometries
	BottomLevelAS::SetAABBGeometries(geometries, numGeometries, vbvs.data(), pGeometryFlags);
}

void EZ::CommandList_DXR::BuildBLAS(BottomLevelAS* pBLAS, bool update)
{
	assert(pBLAS);

	const auto pScratch = needScratch(m_scratchSize);
	const auto descriptorPool = m_descriptorTableLib->GetDescriptorPool(CBV_SRV_UAV_POOL);

	pBLAS->Build(this, pScratch, descriptorPool, update);
}

void EZ::CommandList_DXR::BuildTLAS(TopLevelAS* pTLAS, const Resource* pInstanceDescs, bool update)
{
	assert(pTLAS);

	const auto pScratch = needScratch(m_scratchSize);
	const auto descriptorPool = m_descriptorTableLib->GetDescriptorPool(CBV_SRV_UAV_POOL);

	pTLAS->Build(this, pScratch, pInstanceDescs, descriptorPool, update);
}

void EZ::CommandList_DXR::SetTopLevelAccelerationStructure(uint32_t binding, const TopLevelAS* pTopLevelAS) const
{
	RayTracing::CommandList_DX12::SetTopLevelAccelerationStructure(m_tlasBindingToParamIndexMap[binding], pTopLevelAS);
}

void EZ::CommandList_DXR::RTSetShaderLibrary(uint32_t index, const Blob& shaderLib, uint32_t numShaders, const void** pShaders)
{
	assert(m_rayTracingState);
	m_rayTracingState->SetShaderLibrary(index, shaderLib, numShaders, pShaders);
	m_isRTStateDirty = true;
}

void EZ::CommandList_DXR::RTSetShaderConfig(uint32_t maxPayloadSize, uint32_t maxAttributeSize)
{
	assert(m_rayTracingState);
	m_rayTracingState->SetShaderConfig(maxPayloadSize, maxAttributeSize);
	m_isRTStateDirty = true;
}

void EZ::CommandList_DXR::RTSetHitGroup(uint32_t index, const void* pHitGroup, const void* pClosestHitShader,
	const void* pAnyHitShader, const void* pIntersectionShader, HitGroupType type)
{
	assert(m_rayTracingState);
	m_rayTracingState->SetHitGroup(index, pHitGroup, pClosestHitShader, pAnyHitShader, pIntersectionShader, type);
	m_isRTStateDirty = true;
}

void EZ::CommandList_DXR::RTSetMaxRecursionDepth(uint32_t depth)
{
	assert(m_rayTracingState);
	m_rayTracingState->SetMaxRecursionDepth(depth);
	m_isRTStateDirty = true;
}

void EZ::CommandList_DXR::DispatchRays(uint32_t width, uint32_t height, uint32_t depth,
	const void* pRayGenShader, const void* pMissShader)
{
	CShaderTablePtr pRayGen = nullptr;
	CShaderTablePtr pHitGroup = nullptr;
	CShaderTablePtr pMiss = nullptr;

	predispatchRays(pRayGen, pHitGroup, pMiss, pRayGenShader, pMissShader);
	RayTracing::CommandList_DX12::DispatchRays(width, height, depth, pHitGroup, pMiss, pRayGen);
}

void EZ::CommandList_DXR::DispatchRaysIndirect(const CommandLayout* pCommandlayout, uint32_t maxCommandCount,
	const void* pRayGenShader, const void* pMissShader, Resource* pArgumentBuffer, uint64_t argumentBufferOffset,
	Resource* pCountBuffer, uint64_t countBufferOffset)
{
	CShaderTablePtr pRayGen = nullptr;
	CShaderTablePtr pHitGroup = nullptr;
	CShaderTablePtr pMiss = nullptr;

	preexecuteIndirect(pArgumentBuffer, pCountBuffer);
	predispatchRays(pRayGen, pHitGroup, pMiss, pRayGenShader, pMissShader);

	// Populate arg-buffer header with shader tables
	string key(sizeof(ArgumentBufferVA), '\0');
	auto& argumentBufferVA = reinterpret_cast<ArgumentBufferVA&>(key[0]);
	argumentBufferVA = { pArgumentBuffer, argumentBufferOffset };
	const auto iter = m_argumentBufferVAs.find(key);
	if (iter == m_argumentBufferVAs.cend())
	{
		// Need to copy buffer
		m_uploaders.emplace_back();
		auto& uploader = m_uploaders.back();

		uploader = Buffer::MakeUnique();
		const auto result = uploader->Create(m_pDevice, sizeof(DX12DispatchRaysDescHeader),
			ResourceFlag::DENY_SHADER_RESOURCE, MemoryType::UPLOAD, 0, nullptr,
			0, nullptr, MemoryFlag::NONE);
		assert(result);

		const auto pDispatchDescHeader = static_cast<DX12DispatchRaysDescHeader*>(uploader->Map());
		const auto dxHitGroup = pHitGroup ? static_cast<ID3D12Resource*>(pHitGroup->GetResource()->GetHandle()) : nullptr;
		const auto dxMiss = pMiss ? static_cast<ID3D12Resource*>(pMiss->GetResource()->GetHandle()) : nullptr;
		const auto dxRayGen = pRayGen ? static_cast<ID3D12Resource*>(pRayGen->GetResource()->GetHandle()) : nullptr;
		pDispatchDescHeader->HitGroupTable.StartAddress = dxHitGroup ? dxHitGroup->GetGPUVirtualAddress() : 0;
		pDispatchDescHeader->HitGroupTable.SizeInBytes = pHitGroup ? pHitGroup->GetResource()->GetWidth() : 0;
		pDispatchDescHeader->HitGroupTable.StrideInBytes = pHitGroup ? pHitGroup->GetShaderRecordSize() : 0;
		pDispatchDescHeader->MissShaderTable.StartAddress = dxMiss ? dxMiss->GetGPUVirtualAddress() : 0;
		pDispatchDescHeader->MissShaderTable.SizeInBytes = pMiss ? pMiss->GetResource()->GetWidth() : 0;
		pDispatchDescHeader->MissShaderTable.StrideInBytes = pMiss ? pMiss->GetShaderRecordSize() : 0;
		pDispatchDescHeader->RayGenerationShaderRecord.StartAddress = dxRayGen ? dxRayGen->GetGPUVirtualAddress() : 0;
		pDispatchDescHeader->RayGenerationShaderRecord.SizeInBytes = pRayGen ? pRayGen->GetResource()->GetWidth() : 0;
		uploader->Unmap();

		CopyBufferRegion(pArgumentBuffer, argumentBufferOffset, uploader.get(), 0, sizeof(DX12DispatchRaysDescHeader));
	}
	
	XUSG::CommandList_DX12::ExecuteIndirect(pCommandlayout, maxCommandCount,
		pArgumentBuffer, argumentBufferOffset, pCountBuffer, countBufferOffset);
}

bool EZ::CommandList_DXR::createComputePipelineLayouts(uint32_t maxSamplers, const uint32_t* pMaxCbvsEachSpace,
	const uint32_t* pMaxSrvsEachSpace, const uint32_t* pMaxUavsEachSpace,
	uint32_t maxCbvSpaces, uint32_t maxSrvSpaces, uint32_t maxUavSpaces,
	uint32_t maxTLASSrvs, uint32_t spaceTLAS)
{
	// Create common compute pipeline layout with ray tracing
	auto paramIndex = 0u;
	const auto pipelineLayout = PipelineLayout::MakeUnique(API::DIRECTX_12);
	const auto maxSpaces = (max)(maxCbvSpaces, (max)(maxSrvSpaces, maxUavSpaces));

	auto& descriptorTables = m_cbvSrvUavTables[Shader::Stage::CS];
	if (descriptorTables[static_cast<uint32_t>(DescriptorType::CBV)].empty())
		descriptorTables[static_cast<uint32_t>(DescriptorType::CBV)].resize(maxCbvSpaces);
	if (descriptorTables[static_cast<uint32_t>(DescriptorType::SRV)].empty())
		descriptorTables[static_cast<uint32_t>(DescriptorType::SRV)].resize(maxSrvSpaces);
	if (descriptorTables[static_cast<uint32_t>(DescriptorType::UAV)].empty())
		descriptorTables[static_cast<uint32_t>(DescriptorType::UAV)].resize(maxUavSpaces);

	m_computeSpaceToParamIndexMap[static_cast<uint32_t>(DescriptorType::CBV)].resize(maxCbvSpaces);
	m_computeSpaceToParamIndexMap[static_cast<uint32_t>(DescriptorType::SRV)].resize(maxSrvSpaces);
	m_computeSpaceToParamIndexMap[static_cast<uint32_t>(DescriptorType::UAV)].resize(maxUavSpaces);
	m_tlasBindingToParamIndexMap.resize(maxTLASSrvs);

	pipelineLayout->SetRange(paramIndex++, DescriptorType::SAMPLER, maxSamplers, 0, 0, DescriptorFlag::DATA_STATIC);

	for (auto s = 0u; s < maxSpaces; ++s)
	{
		if (s < maxCbvSpaces)
		{
			const auto maxDescriptors = pMaxCbvsEachSpace ? pMaxCbvsEachSpace[s] : 12;
			m_computeSpaceToParamIndexMap[static_cast<uint32_t>(DescriptorType::CBV)][s] = paramIndex;
			pipelineLayout->SetRange(paramIndex++, DescriptorType::CBV, maxDescriptors, 0, s, DescriptorFlag::DATA_STATIC);
		}

		if (s < maxSrvSpaces && !(s == spaceTLAS && maxTLASSrvs > 0))
		{
			const auto maxDescriptors = pMaxSrvsEachSpace ? pMaxSrvsEachSpace[s] : 32;
			m_computeSpaceToParamIndexMap[static_cast<uint32_t>(DescriptorType::SRV)][s] = paramIndex;
			pipelineLayout->SetRange(paramIndex++, DescriptorType::SRV, maxDescriptors, 0, s);
		}

		if (s < maxUavSpaces)
		{
			const auto maxDescriptors = pMaxUavsEachSpace ? pMaxUavsEachSpace[s] : 16;
			m_computeSpaceToParamIndexMap[static_cast<uint32_t>(DescriptorType::UAV)][s] = paramIndex;
			pipelineLayout->SetRange(paramIndex++, DescriptorType::UAV, maxDescriptors, 0, s);
		}
	}

	for (auto slot = 0u; slot < maxTLASSrvs; ++slot)
	{
		m_tlasBindingToParamIndexMap[slot] = paramIndex;
		pipelineLayout->SetRootSRV(paramIndex++, slot, spaceTLAS, DescriptorFlag::DATA_STATIC);
	}

	XUSG_X_RETURN(m_pipelineLayouts[COMPUTE], pipelineLayout->GetPipelineLayout(m_pDeviceRT, m_pipelineLayoutLib.get(),
		PipelineLayoutFlag::NONE, L"RayTracingEZComputeLayout"), false);

	return true;
}

void EZ::CommandList_DXR::predispatchRays(CShaderTablePtr& pRayGen, CShaderTablePtr& pHitGroup, CShaderTablePtr& pMiss,
	const void* pRayGenShader, const void* pMissShader)
{
	// Set barrier command
	XUSG::CommandList_DX12::Barrier(static_cast<uint32_t>(m_barriers.size()), m_barriers.data());
	m_barriers.clear();

	// Clear UAVs
	clearUAVsUint();
	clearUAVsFloat();

	// Create and set sampler table
	auto& samplerTable = m_samplerTables[COMPUTE];
	if (samplerTable)
	{
		const auto descriptorTable = samplerTable->GetSamplerTable(m_descriptorTableLib.get());
		if (descriptorTable) XUSG::CommandList_DX12::SetComputeDescriptorTable(0, descriptorTable);
		samplerTable.reset();
	}

	// Create and set CBV/SRV/UAV tables
	for (uint8_t t = 0; t < CbvSrvUavTypes; ++t)
	{
		auto& cbvSrvUavTables = m_cbvSrvUavTables[Shader::Stage::CS][t];
		const auto numSpaces = static_cast<uint32_t>(cbvSrvUavTables.size());
		for (auto s = 0u; s < numSpaces; ++s)
		{
			auto& cbvSrvUavTable = cbvSrvUavTables[s];
			if (cbvSrvUavTable)
			{
				const auto descriptorTable = cbvSrvUavTable->GetCbvSrvUavTable(m_descriptorTableLib.get());
				if (descriptorTable) XUSG::CommandList_DX12::SetComputeDescriptorTable(
					m_computeSpaceToParamIndexMap[t][s], descriptorTable);
				cbvSrvUavTable.reset();
			}
		}
	}

	// Create pipeline for dynamic states
	assert(m_rayTracingState);
	if (m_isRTStateDirty)
	{
		m_rayTracingState->SetGlobalPipelineLayout(m_pipelineLayouts[COMPUTE]);
		const auto pipeline = m_rayTracingState->GetPipeline(m_RayTracingPipelineLib.get());
		if (pipeline)
		{
			if (m_pipeline != pipeline)
			{
				XUSG::RayTracing::CommandList_DX12::SetRayTracingPipeline(pipeline);
				m_pipeline = pipeline;

				// Get hit-group table; the hit-group has been bound on the pipeline
				const auto numHitGroups = m_rayTracingState->GetNumHitGroups();
				string key(sizeof(void*) * numHitGroups, '\0');
				const auto pShaderIDs = reinterpret_cast<const void**>(&key[0]);
				for (auto i = 0u; i < numHitGroups; ++i) // Set hit-group shader IDs into the key
				{
					const void* hitGroup = m_rayTracingState->GetHitGroup(i);
					pShaderIDs[i] = ShaderRecord::GetShaderID(pipeline, hitGroup, API::DIRECTX_12);
				}
				pHitGroup = getShaderTable(key, m_hitGroupTables, numHitGroups);
			}
			// Different from other types of state objects, m_rayTracingState cannot be reused
			// for the next different pipeline, so we need to remake it
			m_rayTracingState = State::MakeUnique(API::DIRECTX_12);
			m_isRTStateDirty = false;
		}
	}

	// Get ray-generation shader table; the ray-generation shader is independent of the pipeline
	{
		string key(sizeof(void*), '\0');
		auto& shaderID = reinterpret_cast<const void*&>(key[0]);
		shaderID = ShaderRecord::GetShaderID(m_pipeline, pRayGenShader, API::DIRECTX_12);
		pRayGen = getShaderTable(key, m_rayGenTables, 1);
	}

	{
		string key(sizeof(void*), '\0');
		auto& shaderID = reinterpret_cast<const void*&>(key[0]);
		shaderID = ShaderRecord::GetShaderID(m_pipeline, pMissShader, API::DIRECTX_12);
		pMiss = getShaderTable(key, m_missTables, 1);
	}
}

XUSG::Resource* EZ::CommandList_DXR::needScratch(uint32_t size)
{
	if (m_scratches.empty() || !m_scratches.back() || m_scratches.back()->GetWidth() < size)
	{
		m_scratches.emplace_back(Resource::MakeUnique(API::DIRECTX_12));
		AccelerationStructure::AllocateUAVBuffer(m_pDeviceRT, m_scratches.back().get(), size);
	}

	return m_scratches.back().get();
}

const ShaderTable* EZ::CommandList_DXR::getShaderTable(const string& key,
	unordered_map<string, ShaderTable::uptr>& shaderTables,
	uint32_t numShaderIDs)
{
	const auto shaderTablePair = shaderTables.find(key);

	if (shaderTablePair == shaderTables.cend())
	{
		const auto shaderIDSize = ShaderRecord::GetShaderIDSize(m_pDeviceRT, API::DIRECTX_12);

		auto& shaderTable = shaderTables[key];
		shaderTable = ShaderTable::MakeUnique(API::DIRECTX_12);
		shaderTable->Create(m_pDeviceRT, numShaderIDs, shaderIDSize);

		const auto pShaderIDs = reinterpret_cast<void* const*>(&key[0]);
		for (auto i = 0u; i < numShaderIDs; ++i)
			shaderTable->AddShaderRecord(ShaderRecord::MakeUnique(pShaderIDs[i], shaderIDSize).get());

		return shaderTable.get();
	}

	return shaderTablePair->second.get();
}
