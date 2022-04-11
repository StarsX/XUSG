//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSGCommand_DX12.h"
#include "RayTracing/XUSGRayTracingCommand_DX12.h"
#include "XUSG-EZ_DX12.h"
#include "XUSG-EZ_DXR.h"
//#include "RayTracing/XUSGAccelerationStructure_DX12.h"
//#include "RayTracing/XUSGRayTracingState_DX12.h"

using namespace std;
using namespace XUSG;
using namespace XUSG::EZ::RayTracing;

CommandList_DXR::CommandList_DXR() :
	XUSG::CommandList_DX12(),
	EZ::CommandList_DX12(),
	m_asUavCount(0),
	m_paramIndex(0),
	m_scratchSize(0),
	m_isRTStateDirty(false)
{
}

CommandList_DXR::~CommandList_DXR()
{
}

CommandList_DXR::CommandList_DXR(XUSG::RayTracing::CommandList* pCommandList, uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
	uint32_t maxSamplers, const uint32_t* pMaxCbvsEachSpace, const uint32_t* pMaxSrvsEachSpace, const uint32_t* pMaxUavsEachSpace,
	uint32_t maxCbvSpaces, uint32_t maxSrvSpaces, uint32_t maxUavSpaces, uint32_t maxTLASSrvs, uint32_t spaceTLAS) :
	EZ::CommandList_DX12(pCommandList, samplerPoolSize, cbvSrvUavPoolSize, maxSamplers, pMaxCbvsEachSpace, pMaxSrvsEachSpace, pMaxUavsEachSpace,
		maxCbvSpaces, maxSrvSpaces, maxUavSpaces),
	m_paramIndex(0),
	m_asUavCount(0),
	m_scratchSize(0),
	m_isRTStateDirty(false)
{
}

bool CommandList_DXR::Create(XUSG::RayTracing::CommandList* pCommandList, uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
	uint32_t maxSamplers, const uint32_t* pMaxCbvsEachSpace, const uint32_t* pMaxSrvsEachSpace,
	const uint32_t* pMaxUavsEachSpace, uint32_t maxCbvSpaces, uint32_t maxSrvSpaces, uint32_t maxUavSpaces,
	uint32_t maxTLASSrvs, uint32_t spaceTLAS)
{
	m_pDeviceRT = pCommandList->GetRTDevice();
	m_pDevice = m_pDeviceRT;
	m_commandList = dynamic_cast<XUSG::CommandList_DX12*>(pCommandList)->GetGraphicsCommandList();

	m_graphicsPipelineCache = Graphics::PipelineCache::MakeUnique(m_pDevice, API::DIRECTX_12);
	m_computePipelineCache = Compute::PipelineCache::MakeUnique(m_pDevice, API::DIRECTX_12);
	m_RayTracingPipelineCache = XUSG::RayTracing::PipelineCache::MakeUnique(m_pDeviceRT, API::DIRECTX_12);
	m_pipelineLayoutCache = PipelineLayoutCache::MakeUnique(m_pDevice, API::DIRECTX_12);
	m_descriptorTableCache = DescriptorTableCache::MakeUnique(m_pDevice, L"EZDescirptorTableCache", API::DIRECTX_12);

	// Allocate descriptor pools
	m_descriptorTableCache->AllocateDescriptorPool(DescriptorPoolType::SAMPLER_POOL, samplerPoolSize);
	m_descriptorTableCache->AllocateDescriptorPool(DescriptorPoolType::CBV_SRV_UAV_POOL, cbvSrvUavPoolSize);

	// Create common pipeline layouts
	createPipelineLayouts(maxSamplers, pMaxCbvsEachSpace, pMaxSrvsEachSpace, pMaxUavsEachSpace, maxCbvSpaces, maxSrvSpaces, maxUavSpaces, maxTLASSrvs, spaceTLAS);

	return true;
}

bool CommandList_DXR::Close()
{
	if (!m_scratches.empty())
	{
		auto scratch = std::move(m_scratches.back());
		m_scratches.resize(1);
		m_scratches[0] = std::move(scratch);
	}

	return XUSG::EZ::CommandList_DX12::Close();
}

bool CommandList_DXR::PreBuildBLAS(XUSG::RayTracing::BottomLevelAS* pBLAS, uint32_t numGeometries, const XUSG::RayTracing::GeometryBuffer& geometries,
	XUSG::RayTracing::BuildFlag flags)
{
	assert(pBLAS);
	XUSG_N_RETURN(pBLAS->PreBuild(m_pDeviceRT, numGeometries, geometries, m_asUavCount++, flags), false);

	const auto descriptorTable = Util::DescriptorTable::MakeUnique();
	descriptorTable->SetDescriptors(0, 1, &pBLAS->GetResult()->GetUAV());
	const auto uavTable = descriptorTable->GetCbvSrvUavTable(m_descriptorTableCache.get());
	XUSG_N_RETURN(uavTable, false);

	m_scratchSize = (max)(m_scratchSize, pBLAS->GetScratchDataMaxSize());

	return true;
}

bool CommandList_DXR::PreBuildTLAS(XUSG::RayTracing::TopLevelAS* pTLAS, uint32_t numInstances, XUSG::RayTracing::BuildFlag flags)
{
	assert(pTLAS);
	XUSG_N_RETURN(pTLAS->PreBuild(m_pDeviceRT, numInstances, m_asUavCount++, flags), false);

	const auto descriptorTable = Util::DescriptorTable::MakeUnique();
	descriptorTable->SetDescriptors(0, 1, &pTLAS->GetResult()->GetUAV());
	const auto uavTable = descriptorTable->GetCbvSrvUavTable(m_descriptorTableCache.get());
	XUSG_N_RETURN(uavTable, false);

	m_scratchSize = (max)(m_scratchSize, pTLAS->GetScratchDataMaxSize());

	return true;
}

bool CommandList_DXR::BuildBLAS(XUSG::RayTracing::BottomLevelAS* pBLAS, bool update)
{
	assert(pBLAS);

	auto scratch = needScratch(m_scratchSize);
	const auto& descriptorPool = m_descriptorTableCache->GetDescriptorPool(CBV_SRV_UAV_POOL);
	pBLAS->Build(this, scratch, descriptorPool, update);
	return true;
}

bool CommandList_DXR::BuildTLAS(XUSG::RayTracing::TopLevelAS* pTLAS, const Resource* pInstanceDescs, bool update)
{
	assert(pTLAS);

	auto scratch = needScratch(m_scratchSize);
	const auto& descriptorPool = m_descriptorTableCache->GetDescriptorPool(CBV_SRV_UAV_POOL);
	pTLAS->Build(this, scratch, pInstanceDescs, descriptorPool, update);
	return true;
}

bool CommandList_DXR::Create(const XUSG::RayTracing::Device* pDevice, void* pHandle, uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize, uint32_t maxSamplers, const uint32_t* pMaxCbvsEachSpace, const uint32_t* pMaxSrvsEachSpace, const uint32_t* pMaxUavsEachSpace, uint32_t maxCbvSpaces, uint32_t maxSrvSpaces, uint32_t maxUavSpaces, const wchar_t* name)
{
	m_pDeviceRT = pDevice;
	XUSG::RayTracing::CommandList_DX12::Create(pHandle, name);
	return Create(this, samplerPoolSize, cbvSrvUavPoolSize, maxSamplers,
		pMaxCbvsEachSpace, pMaxSrvsEachSpace, pMaxUavsEachSpace,
		maxCbvSpaces, maxSrvSpaces, maxUavSpaces);
}

Resource* CommandList_DXR::needScratch(uint32_t size)
{
	if (m_scratches.empty() || !m_scratches.back() || m_scratches.back()->GetWidth() < size)
	{
		m_scratches.emplace_back(Resource::MakeUnique());
		XUSG::RayTracing::AccelerationStructure::AllocateUAVBuffer(m_pDeviceRT, m_scratches.back().get(), size);
	}

	return m_scratches.back().get();
}

bool CommandList_DXR::createPipelineLayouts(uint32_t maxSamplers, const uint32_t* pMaxCbvsEachSpace,
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
		const auto pipelineLayout = XUSG::RayTracing::PipelineLayout::MakeUnique(API::DIRECTX_12);
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
				const auto maxDescriptors = pMaxCbvsEachSpace ? pMaxCbvsEachSpace[s] : 14;
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

		m_paramIndex = paramIndex;
		XUSG_X_RETURN(m_pipelineLayouts[COMPUTE], pipelineLayout->GetPipelineLayout(m_pDeviceRT, m_pipelineLayoutCache.get(),
			PipelineLayoutFlag::NONE, L"EZComputeLayout"), false);
	}

	return true;
}

void CommandList_DXR::RTSetShaderLibrary(Blob shaderLib)
{
	if (!m_rayTracingState) m_rayTracingState = XUSG::RayTracing::State::MakeUnique(API::DIRECTX_12);
	m_rayTracingState->SetShaderLibrary(shaderLib);
	m_isRTStateDirty = true;
}

void CommandList_DXR::RTSetShaderConfig(uint32_t maxPayloadSize, uint32_t maxAttributeSize)
{
	if (!m_rayTracingState) m_rayTracingState = XUSG::RayTracing::State::MakeUnique(API::DIRECTX_12);
	m_rayTracingState->SetShaderConfig(maxPayloadSize, maxAttributeSize);
	m_isRTStateDirty = true;
}

void CommandList_DXR::RTSetHitGroup(uint32_t index, const void* hitGroup, const void* closestHitShader,
	const void* anyHitShader, const void* intersectionShader,
	XUSG::RayTracing::HitGroupType type)
{
	if (!m_rayTracingState) m_rayTracingState = XUSG::RayTracing::State::MakeUnique(API::DIRECTX_12);
	m_rayTracingState->SetHitGroup(index, hitGroup, closestHitShader, anyHitShader, intersectionShader, type);
	m_isRTStateDirty = true;
}

void CommandList_DXR::RTSetMaxRecursionDepth(uint32_t depth)
{
	if (!m_rayTracingState) m_rayTracingState = XUSG::RayTracing::State::MakeUnique(API::DIRECTX_12);
	m_rayTracingState->SetMaxRecursionDepth(depth);
	m_isRTStateDirty = true;
}

void CommandList_DXR::DispatchRays(uint32_t width, uint32_t height, uint32_t depth)
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

	// Create pipeline for dynamic states
	if (m_rayTracingState && m_isRTStateDirty)
	{
		m_rayTracingState->SetGlobalPipelineLayout(m_pipelineLayouts[COMPUTE]);
		const auto pipeline = m_rayTracingState->GetPipeline(m_RayTracingPipelineCache.get());
		if (pipeline)
		{
			if (m_pipeline != pipeline)
			{
				XUSG::RayTracing::CommandList_DX12::SetRayTracingPipeline(pipeline);
				m_pipeline = pipeline;
			}
			m_pipeline = pipeline;
			m_isRTStateDirty = false;
		}
	}

	//XUSG::RayTracing::CommandList::DispatchRays(width, height, depth, pHitGroup, pMiss, pRayGen);
}
