//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSGCommand_DX12.h"
#include "XUSG-EZ_DXR.h"
#include "RayTracing/XUSGAccelerationStructure_DX12.h"

using namespace std;
using namespace XUSG;
using namespace XUSG::EZ::RayTracing;

CommandList_DXR::CommandList_DXR() :
	XUSG::CommandList_DX12(),
	EZ::CommandList_DX12(),
	m_asUavCount(0),
	m_paramIndex(0),
	m_scratchSize(0)
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
	m_scratchSize(0)
{
}

bool CommandList_DXR::Create(XUSG::RayTracing::CommandList* pCommandList, uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
	uint32_t maxSamplers, const uint32_t* pMaxCbvsEachSpace, const uint32_t* pMaxSrvsEachSpace,
	const uint32_t* pMaxUavsEachSpace, uint32_t maxCbvSpaces, uint32_t maxSrvSpaces, uint32_t maxUavSpaces,
	uint32_t maxTLASSrvs, uint32_t spaceTLAS)
{
	m_pDevice = pCommandList->GetDevice();
	m_pDeviceRT = pCommandList->GetRTDevice();
	m_commandList = dynamic_cast<XUSG::CommandList_DX12*>(pCommandList)->GetGraphicsCommandList();

	m_graphicsPipelineCache = Graphics::PipelineCache::MakeUnique(m_pDevice, API::DIRECTX_12);
	m_computePipelineCache = Compute::PipelineCache::MakeUnique(m_pDevice, API::DIRECTX_12);
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

	N_RETURN(pBLAS->PreBuild(m_pDeviceRT, numGeometries, geometries, m_asUavCount++, flags), false);

	Descriptor descriptor = pBLAS->GetResult()->GetUAV();
	const auto descriptorTable = Util::DescriptorTable::MakeUnique();
	descriptorTable->SetDescriptors(0, 1, &descriptor);
	const auto asTable = descriptorTable->GetCbvSrvUavTable(m_descriptorTableCache.get());
	N_RETURN(asTable, false);
	m_scratchSize = (max)(m_scratchSize, pBLAS->GetScratchDataMaxSize());

	return true;
}

bool CommandList_DXR::PreBuildTLAS(XUSG::RayTracing::TopLevelAS* pTLAS, uint32_t numGeometries, XUSG::RayTracing::BuildFlag flags)
{
	assert(pTLAS);

	N_RETURN(pTLAS->PreBuild(m_pDeviceRT, numGeometries, m_asUavCount++, flags), false);

	Descriptor descriptor = pTLAS->GetResult()->GetUAV();
	const auto descriptorTable = Util::DescriptorTable::MakeUnique();
	descriptorTable->SetDescriptors(0, 1, &descriptor);
	const auto asTable = descriptorTable->GetCbvSrvUavTable(m_descriptorTableCache.get());
	N_RETURN(asTable, false);
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

XUSG::Resource* CommandList_DXR::needScratch(uint32_t size)
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

		X_RETURN(m_pipelineLayouts[GRAPHICS], pipelineLayout->GetPipelineLayout(m_pipelineLayoutCache.get(),
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
		X_RETURN(m_pipelineLayouts[COMPUTE], pipelineLayout->GetPipelineLayout(m_pDeviceRT, m_pipelineLayoutCache.get(),
			PipelineLayoutFlag::NONE, L"EZComputeLayout"), false);
	}

	return true;
}
