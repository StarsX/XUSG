//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSGCommand_DX12.h"
#include "RayTracing/XUSGRayTracingCommand_DX12.h"
#include "RayTracing/XUSGRayTracingState_DX12.h"
#include "XUSG-EZ_DX12.h"
#include "XUSGRayTracing-EZ_DX12.h"

using namespace std;
using namespace XUSG::RayTracing;

EZ::CommandList_DXR::CommandList_DXR() :
	XUSG::CommandList_DX12(),
	XUSG::EZ::CommandList_DX12(),
	m_RayTracingPipelineCache(nullptr),
	m_scratchSize(0),
	m_scratches(0),
	m_isRTStateDirty(false),
	m_rayTracingState(nullptr),
	m_asUavCount(0),
	m_tlasBindingToParamIndexMap(0)
{
}

EZ::CommandList_DXR::~CommandList_DXR()
{
}

EZ::CommandList_DXR::CommandList_DXR(RayTracing::CommandList* pCommandList, uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
	uint32_t maxSamplers, const uint32_t* pMaxCbvsEachSpace, const uint32_t* pMaxSrvsEachSpace, const uint32_t* pMaxUavsEachSpace,
	uint32_t maxCbvSpaces, uint32_t maxSrvSpaces, uint32_t maxUavSpaces, uint32_t maxTLASSrvs, uint32_t spaceTLAS) :
	CommandList_DXR()
{
	Create(pCommandList, samplerPoolSize, cbvSrvUavPoolSize, maxSamplers,
		pMaxCbvsEachSpace, pMaxSrvsEachSpace, pMaxUavsEachSpace,
		maxCbvSpaces, maxSrvSpaces, maxUavSpaces, maxTLASSrvs, spaceTLAS);
}

bool EZ::CommandList_DXR::Create(RayTracing::CommandList* pCommandList, uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
	uint32_t maxSamplers, const uint32_t* pMaxCbvsEachSpace, const uint32_t* pMaxSrvsEachSpace,
	const uint32_t* pMaxUavsEachSpace, uint32_t maxCbvSpaces, uint32_t maxSrvSpaces, uint32_t maxUavSpaces,
	uint32_t maxTLASSrvs, uint32_t spaceTLAS)
{
	m_pDeviceRT = pCommandList->GetRTDevice();
	m_pDevice = m_pDeviceRT;
	m_commandList = dynamic_cast<XUSG::CommandList_DX12*>(pCommandList)->GetGraphicsCommandList();

	const auto pDxDevice = static_cast<ID3D12RaytracingFallbackDevice*>(m_pDeviceRT->GetRTHandle());
	XUSG_N_RETURN(pDxDevice, false);
	pDxDevice->QueryRaytracingCommandList(m_commandList.get(), IID_PPV_ARGS(&m_commandListRT));

	m_graphicsPipelineCache = Graphics::PipelineCache::MakeUnique(m_pDevice, API::DIRECTX_12);
	m_computePipelineCache = Compute::PipelineCache::MakeUnique(m_pDevice, API::DIRECTX_12);
	m_RayTracingPipelineCache = PipelineCache::MakeUnique(m_pDeviceRT, API::DIRECTX_12);
	m_pipelineLayoutCache = PipelineLayoutCache::MakeUnique(m_pDevice, API::DIRECTX_12);
	m_descriptorTableCache = DescriptorTableCache::MakeUnique(m_pDevice, L"EZDescirptorTableCache", API::DIRECTX_12);
	m_rayTracingState = State::MakeUnique(API::DIRECTX_12);

	// Allocate descriptor pools
	XUSG_N_RETURN(m_descriptorTableCache->AllocateDescriptorPool(
		DescriptorPoolType::SAMPLER_POOL, samplerPoolSize), false);
	XUSG_N_RETURN(m_descriptorTableCache->AllocateDescriptorPool(
		DescriptorPoolType::CBV_SRV_UAV_POOL, cbvSrvUavPoolSize), false);

	// Create common pipeline layouts
	XUSG_N_RETURN(createPipelineLayouts(maxSamplers, pMaxCbvsEachSpace, pMaxSrvsEachSpace, pMaxUavsEachSpace,
		maxCbvSpaces, maxSrvSpaces, maxUavSpaces, maxTLASSrvs, spaceTLAS), false);

	return true;
}

bool EZ::CommandList_DXR::Create(const RayTracing::Device* pDevice, void* pHandle, uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
	uint32_t maxSamplers, const uint32_t* pMaxCbvsEachSpace, const uint32_t* pMaxSrvsEachSpace, const uint32_t* pMaxUavsEachSpace,
	uint32_t maxCbvSpaces, uint32_t maxSrvSpaces, uint32_t maxUavSpaces, uint32_t maxTLASSrvs, uint32_t spaceTLAS, const wchar_t* name)
{
	m_pDeviceRT = pDevice;
	RayTracing::CommandList_DX12::Create(pHandle, name);

	return Create(this, samplerPoolSize, cbvSrvUavPoolSize, maxSamplers,
		pMaxCbvsEachSpace, pMaxSrvsEachSpace, pMaxUavsEachSpace,
		maxCbvSpaces, maxSrvSpaces, maxUavSpaces, maxTLASSrvs, spaceTLAS);
}

bool EZ::CommandList_DXR::Close()
{
	if (!m_scratches.empty())
	{
		auto scratch = move(m_scratches.back());
		m_scratches.resize(1);
		m_scratches[0] = move(scratch);
	}

	return XUSG::EZ::CommandList_DX12::Close();
}

bool EZ::CommandList_DXR::CloseForPresent(RenderTarget* pBackBuffer)
{
	ResourceBarrier barrier;
	const auto numBarriers = pBackBuffer->SetBarrier(&barrier, ResourceState::PRESENT);
	XUSG::CommandList_DX12::Barrier(numBarriers, &barrier);

	return Close();
}

bool EZ::CommandList_DXR::PreBuildBLAS(BottomLevelAS* pBLAS, uint32_t numGeometries,
	const GeometryBuffer& geometries, BuildFlag flags)
{
	assert(pBLAS);
	XUSG_N_RETURN(pBLAS->PreBuild(m_pDeviceRT, numGeometries, geometries, m_asUavCount++, flags), false);

	const auto descriptorTable = Util::DescriptorTable::MakeUnique(API::DIRECTX_12);
	descriptorTable->SetDescriptors(0, 1, &pBLAS->GetResult()->GetUAV());
	const auto uavTable = descriptorTable->GetCbvSrvUavTable(m_descriptorTableCache.get());
	XUSG_N_RETURN(uavTable, false);

	m_scratchSize = (max)(m_scratchSize, pBLAS->GetScratchDataMaxSize());

	return true;
}

bool EZ::CommandList_DXR::PreBuildTLAS(TopLevelAS* pTLAS, uint32_t numInstances, BuildFlag flags)
{
	assert(pTLAS);
	XUSG_N_RETURN(pTLAS->PreBuild(m_pDeviceRT, numInstances, m_asUavCount++, flags), false);

	const auto descriptorTable = Util::DescriptorTable::MakeUnique(API::DIRECTX_12);
	descriptorTable->SetDescriptors(0, 1, &pTLAS->GetResult()->GetUAV());
	const auto uavTable = descriptorTable->GetCbvSrvUavTable(m_descriptorTableCache.get());
	XUSG_N_RETURN(uavTable, false);

	m_scratchSize = (max)(m_scratchSize, pTLAS->GetScratchDataMaxSize());

	return true;
}

void EZ::CommandList_DXR::BuildBLAS(BottomLevelAS* pBLAS, bool update)
{
	assert(pBLAS);

	const auto pScratch = needScratch(m_scratchSize);
	const auto descriptorPool = m_descriptorTableCache->GetDescriptorPool(CBV_SRV_UAV_POOL);

	pBLAS->Build(this, pScratch, descriptorPool, update);
}

void EZ::CommandList_DXR::BuildTLAS(TopLevelAS* pTLAS, const Resource* pInstanceDescs, bool update)
{
	assert(pTLAS);

	const auto pScratch = needScratch(m_scratchSize);
	const auto descriptorPool = m_descriptorTableCache->GetDescriptorPool(CBV_SRV_UAV_POOL);

	pTLAS->Build(this, pScratch, pInstanceDescs, descriptorPool, update);
}

void EZ::CommandList_DXR::SetTopLevelAccelerationStructure(uint32_t binding, const TopLevelAS* pTopLevelAS) const
{
	RayTracing::CommandList_DX12::SetTopLevelAccelerationStructure(m_tlasBindingToParamIndexMap[binding], pTopLevelAS);
}

void EZ::CommandList_DXR::RTSetShaderLibrary(Blob shaderLib)
{
	assert(m_rayTracingState);
	m_rayTracingState->SetShaderLibrary(shaderLib);
	m_isRTStateDirty = true;
}

void EZ::CommandList_DXR::RTSetShaderConfig(uint32_t maxPayloadSize, uint32_t maxAttributeSize)
{
	assert(m_rayTracingState);
	m_rayTracingState->SetShaderConfig(maxPayloadSize, maxAttributeSize);
	m_isRTStateDirty = true;
}

void EZ::CommandList_DXR::RTSetHitGroup(uint32_t index, const void* hitGroup, const void* closestHitShader,
	const void* anyHitShader, const void* intersectionShader, HitGroupType type)
{
	assert(m_rayTracingState);
	m_rayTracingState->SetHitGroup(index, hitGroup, closestHitShader, anyHitShader, intersectionShader, type);
	m_isRTStateDirty = true;
}

void EZ::CommandList_DXR::RTSetMaxRecursionDepth(uint32_t depth)
{
	assert(m_rayTracingState);
	m_rayTracingState->SetMaxRecursionDepth(depth);
	m_isRTStateDirty = true;
}

void EZ::CommandList_DXR::DispatchRays(uint32_t width, uint32_t height, uint32_t depth,
	const void* rayGenShader, const void* missShader)
{
	//TODO: fix raytracing shadertables.
	// Create and set sampler table
	auto& samplerTable = m_samplerTables[COMPUTE];
	if (samplerTable)
	{
		const auto descriptorTable = samplerTable->GetSamplerTable(m_descriptorTableCache.get());
		if (descriptorTable) XUSG::CommandList_DX12::SetComputeDescriptorTable(0, descriptorTable);
		samplerTable.reset();
	}

	// Create and set CBV/SRV/UAV tables
	for (uint8_t t = 0; t < CbvSrvUavTypes; ++t)
	{
		auto& cbvSrvUavTables = m_computeCbvSrvUavTables[t];
		const auto numSpaces = static_cast<uint32_t>(cbvSrvUavTables.size());
		for (auto s = 0u; s < numSpaces; ++s)
		{
			auto& cbvSrvUavTable = cbvSrvUavTables[s];
			if (cbvSrvUavTable)
			{
				const auto descriptorTable = cbvSrvUavTable->GetCbvSrvUavTable(m_descriptorTableCache.get());
				if (descriptorTable) XUSG::CommandList_DX12::SetComputeDescriptorTable(
					m_computeSpaceToParamIndexMap[t][s], descriptorTable);
				cbvSrvUavTable.reset();
			}
		}
	}

	// Set barrier command
	XUSG::CommandList_DX12::Barrier(static_cast<uint32_t>(m_barriers.size()), m_barriers.data());
	m_barriers.clear();

	const ShaderTable* pHitGroup = nullptr;

	// Create pipeline for dynamic states
	assert(m_rayTracingState);
	if (m_isRTStateDirty)
	{
		m_rayTracingState->SetGlobalPipelineLayout(m_pipelineLayouts[COMPUTE]);
		const auto pipeline = m_rayTracingState->GetPipeline(m_RayTracingPipelineCache.get());
		if (pipeline)
		{
			if (m_pipeline != pipeline)
			{
				XUSG::RayTracing::CommandList_DX12::SetRayTracingPipeline(pipeline);
				m_pipeline = pipeline;

				// Get hit-group table; the hit-group has been bound on the pipeline
				const auto numHitGroups = getNumHitGroupsFromState(m_rayTracingState.get());
				string key(sizeof(void*) * numHitGroups, '\0');
				const auto pShaderIDs = reinterpret_cast<const void**>(&key[0]);
				for (auto i = 0u; i < numHitGroups; ++i) // Set hit-group shader IDs into the key
				{
					const void* hitGroup = getHitGroupFromState(i, m_rayTracingState.get());
					pShaderIDs[i] = ShaderRecord::GetShaderID(pipeline, hitGroup, API::DIRECTX_12);
				}
				pHitGroup = getShaderTable(key, m_hitGroupTables, numHitGroups);
			}
			m_isRTStateDirty = false;
		}
	}

	// Get ray-generation shader table; the ray-generation shader is independent of the pipeline
	const ShaderTable* pRayGen = nullptr;
	{
		string key(sizeof(void*), '\0');
		auto& shaderID = reinterpret_cast<const void*&>(key[0]);
		shaderID = ShaderRecord::GetShaderID(m_pipeline, rayGenShader, API::DIRECTX_12);
		pRayGen = getShaderTable(key, m_rayGenTables, 1);
	}

	const ShaderTable* pMiss = nullptr;
	{
		string key(sizeof(void*), '\0');
		auto& shaderID = reinterpret_cast<const void*&>(key[0]);
		shaderID = ShaderRecord::GetShaderID(m_pipeline, missShader, API::DIRECTX_12);
		pMiss = getShaderTable(key, m_missTables, 1);
	}

	RayTracing::CommandList_DX12::DispatchRays(width, height, depth, pHitGroup, pMiss, pRayGen);
}

bool EZ::CommandList_DXR::createPipelineLayouts(uint32_t maxSamplers, const uint32_t* pMaxCbvsEachSpace,
	const uint32_t* pMaxSrvsEachSpace, const uint32_t* pMaxUavsEachSpace,
	uint32_t maxCbvSpaces, uint32_t maxSrvSpaces, uint32_t maxUavSpaces, uint32_t maxTLASSrvs, uint32_t spaceTLAS)
{
	const auto maxSpaces = (max)(maxCbvSpaces, (max)(maxSrvSpaces, maxUavSpaces));
	{
		// Create common graphics pipeline layout
		auto paramIndex = 0u;
		const auto pipelineLayout = Util::PipelineLayout::MakeUnique(API::DIRECTX_12);
		const auto maxSpaces = (max)(maxCbvSpaces, (max)(maxSrvSpaces, maxUavSpaces));
		pipelineLayout->SetRange(paramIndex++, DescriptorType::SAMPLER, maxSamplers, 0, 0, DescriptorFlag::DATA_STATIC);

		for (uint8_t i = 0; i < Shader::Stage::NUM_GRAPHICS; ++i)
		{
			auto& descriptorTables = m_graphicsCbvSrvUavTables[i];
			descriptorTables[static_cast<uint32_t>(DescriptorType::CBV)].resize(maxCbvSpaces);
			descriptorTables[static_cast<uint32_t>(DescriptorType::SRV)].resize(maxSrvSpaces);
			descriptorTables[static_cast<uint32_t>(DescriptorType::UAV)].resize(maxUavSpaces);

			auto& spaceToParamIndexMap = m_graphicsSpaceToParamIndexMap[i];
			spaceToParamIndexMap[static_cast<uint32_t>(DescriptorType::CBV)].resize(maxCbvSpaces);
			spaceToParamIndexMap[static_cast<uint32_t>(DescriptorType::SRV)].resize(maxSrvSpaces);
			spaceToParamIndexMap[static_cast<uint32_t>(DescriptorType::UAV)].resize(maxUavSpaces);
			for (auto s = 0u; s < maxSpaces; ++s)
			{
				const auto stage = static_cast<Shader::Stage>(i);

				if (s < maxCbvSpaces)
				{
					const auto maxDescriptors = pMaxCbvsEachSpace ? pMaxCbvsEachSpace[s] : 14;
					spaceToParamIndexMap[static_cast<uint32_t>(DescriptorType::CBV)][s] = paramIndex;
					pipelineLayout->SetRange(paramIndex, DescriptorType::CBV, maxDescriptors, 0, s, DescriptorFlag::DATA_STATIC);
					pipelineLayout->SetShaderStage(paramIndex++, stage);
				}

				if (s < maxSrvSpaces)
				{
					const auto maxDescriptors = pMaxSrvsEachSpace ? pMaxSrvsEachSpace[s] : 32;
					spaceToParamIndexMap[static_cast<uint32_t>(DescriptorType::SRV)][s] = paramIndex;
					pipelineLayout->SetRange(paramIndex, DescriptorType::SRV, maxDescriptors, 0, s);
					pipelineLayout->SetShaderStage(paramIndex++, stage);
				}

				if (s < maxUavSpaces)
				{
					const auto maxDescriptors = pMaxUavsEachSpace ? pMaxUavsEachSpace[s] : 16;
					spaceToParamIndexMap[static_cast<uint32_t>(DescriptorType::UAV)][s] = paramIndex;
					pipelineLayout->SetRange(paramIndex, DescriptorType::UAV, maxDescriptors, 0, s);
					pipelineLayout->SetShaderStage(paramIndex++, stage);
				}
			}
		}

		XUSG_X_RETURN(m_pipelineLayouts[GRAPHICS], pipelineLayout->GetPipelineLayout(m_pipelineLayoutCache.get(),
			PipelineLayoutFlag::ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, L"EZGraphicsLayout"), false);
	}

	{
		// Create common compute pipeline layout
		auto paramIndex = 0u;
		const auto pipelineLayout = PipelineLayout::MakeUnique(API::DIRECTX_12);
		pipelineLayout->SetRange(paramIndex++, DescriptorType::SAMPLER, maxSamplers, 0, 0, DescriptorFlag::DATA_STATIC);

		auto& descriptorTables = m_computeCbvSrvUavTables;
		descriptorTables[static_cast<uint32_t>(DescriptorType::CBV)].resize(maxCbvSpaces);
		descriptorTables[static_cast<uint32_t>(DescriptorType::SRV)].resize(maxSrvSpaces);
		descriptorTables[static_cast<uint32_t>(DescriptorType::UAV)].resize(maxUavSpaces);

		m_computeSpaceToParamIndexMap[static_cast<uint32_t>(DescriptorType::CBV)].resize(maxCbvSpaces);
		m_computeSpaceToParamIndexMap[static_cast<uint32_t>(DescriptorType::SRV)].resize(maxSrvSpaces);
		m_computeSpaceToParamIndexMap[static_cast<uint32_t>(DescriptorType::UAV)].resize(maxUavSpaces);
		m_tlasBindingToParamIndexMap.resize(maxTLASSrvs);

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

		XUSG_X_RETURN(m_pipelineLayouts[COMPUTE], pipelineLayout->GetPipelineLayout(m_pDeviceRT, m_pipelineLayoutCache.get(),
			PipelineLayoutFlag::NONE, L"EZComputeLayout"), false);
	}

	return true;
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

const void* EZ::CommandList_DXR::getHitGroupFromState(uint32_t index, const State* pState)
{
	assert(index < getNumHitGroupsFromState(pState));

	const auto& key = m_rayTracingState->GetKey();
	const auto pHitGroups = reinterpret_cast<const State_DX12::KeyHitGroup*>(&key[sizeof(State_DX12::KeyHeader)]);

	return pHitGroups[index].HitGroup;
}

uint32_t EZ::CommandList_DXR::getNumHitGroupsFromState(const State* pState)
{
	const auto& key = m_rayTracingState->GetKey();
	const auto pKeyHeader = reinterpret_cast<const State_DX12::KeyHeader*>(&key[0]);

	return pKeyHeader->NumHitGroups;
}

const ShaderTable* EZ::CommandList_DXR::getShaderTable(const string& key,
	unordered_map<string, ShaderTable::uptr>& shaderTables,
	uint32_t numShaderIDs)
{
	const auto shaderTablePair = shaderTables.find(key);

	if (shaderTablePair == shaderTables.end())
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
