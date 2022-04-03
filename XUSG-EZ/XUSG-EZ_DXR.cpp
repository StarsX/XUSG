//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSGCommand_DX12.h"
#include "XUSG-EZ_DXR.h"

using namespace std;
using namespace XUSG;
using namespace XUSG::EZ::RayTracing;

CommandList_DXR::CommandList_DXR() :
	XUSG::EZ::CommandList_DX12()
{
}

CommandList_DXR::~CommandList_DXR()
{
}

CommandList_DXR::CommandList_DXR(XUSG::RayTracing::CommandList* pCommandList, uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
	uint32_t maxSamplers, const uint32_t* pMaxCbvsEachSpace, const uint32_t* pMaxSrvsEachSpace, const uint32_t* pMaxUavsEachSpace,
	uint32_t maxCbvSpaces, uint32_t maxSrvSpaces, uint32_t maxUavSpaces) :
	XUSG::EZ::CommandList_DX12(pCommandList, samplerPoolSize, cbvSrvUavPoolSize, maxSamplers, pMaxCbvsEachSpace, pMaxSrvsEachSpace, pMaxUavsEachSpace,
		maxCbvSpaces, maxSrvSpaces, maxUavSpaces)
{
}

bool CommandList_DXR::Create(XUSG::RayTracing::CommandList* pCommandList, uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
	uint32_t maxSamplers, const uint32_t* pMaxCbvsEachSpace, const uint32_t* pMaxSrvsEachSpace,
	const uint32_t* pMaxUavsEachSpace, uint32_t maxCbvSpaces, uint32_t maxSrvSpaces, uint32_t maxUavSpaces)
{
	const auto pDevice = pCommandList->GetRTDevice();
	m_commandList = dynamic_cast<XUSG::CommandList_DX12*>(pCommandList)->GetGraphicsCommandList();

	m_graphicsPipelineCache = Graphics::PipelineCache::MakeUnique(pDevice, API::DIRECTX_12);
	m_computePipelineCache = Compute::PipelineCache::MakeUnique(pDevice, API::DIRECTX_12);
	m_pipelineLayoutCache = PipelineLayoutCache::MakeUnique(pDevice, API::DIRECTX_12);
	m_descriptorTableCache = DescriptorTableCache::MakeUnique(pDevice, L"EZDescirptorTableCache", API::DIRECTX_12);

	// Allocate descriptor pools
	m_descriptorTableCache->AllocateDescriptorPool(DescriptorPoolType::SAMPLER_POOL, samplerPoolSize);
	m_descriptorTableCache->AllocateDescriptorPool(DescriptorPoolType::CBV_SRV_UAV_POOL, cbvSrvUavPoolSize);

	// Create common pipeline layouts
	createPipelineLayouts(pCommandList, maxSamplers, pMaxCbvsEachSpace, pMaxSrvsEachSpace, pMaxUavsEachSpace, maxCbvSpaces, maxSrvSpaces, maxUavSpaces);

	return true;
}

bool CommandList_DXR::createPipelineLayouts(XUSG::RayTracing::CommandList* pCommandList, uint32_t maxSamplers, const uint32_t* pMaxCbvsEachSpace,
	const uint32_t* pMaxSrvsEachSpace, const uint32_t* pMaxUavsEachSpace,
	uint32_t maxCbvSpaces, uint32_t maxSrvSpaces, uint32_t maxUavSpaces)
{
	const auto maxSpaces = (max)(maxCbvSpaces, (max)(maxSrvSpaces, maxUavSpaces));
	{
		// Create common graphics pipeline layout
		auto paramIndex = 0u;
		const auto pipelineLayout = XUSG::RayTracing::PipelineLayout::MakeUnique(API::DIRECTX_12);
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

		X_RETURN(m_pipelineLayouts[GRAPHICS], pipelineLayout->GetPipelineLayout(pCommandList->GetRTDevice(), m_pipelineLayoutCache.get(),
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
		for (auto s = 0u; s < maxSpaces; ++s)
		{
			if (s < maxCbvSpaces)
			{
				const auto maxDescriptors = pMaxCbvsEachSpace ? pMaxCbvsEachSpace[s] : 14;
				m_computeSpaceToParamIndexMap[static_cast<uint32_t>(DescriptorType::CBV)][s] = paramIndex;
				pipelineLayout->SetRange(paramIndex++, DescriptorType::CBV, maxDescriptors, 0, s, DescriptorFlag::DATA_STATIC);
			}

			if (s < maxSrvSpaces)
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

		X_RETURN(m_pipelineLayouts[COMPUTE], pipelineLayout->GetPipelineLayout(pCommandList->GetRTDevice(), m_pipelineLayoutCache.get(),
			PipelineLayoutFlag::NONE, L"EZComputeLayout"), false);
	}

	return true;
}