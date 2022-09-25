//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSGCommand_DX12.h"
#include "Core/XUSGResource_DX12.h"
#include "Ultimate/XUSGUltimate_DX12.h"
#include "XUSG-EZ_DX12.h"
#include "XUSGUltimate-EZ_DX12.h"

using namespace std;
using namespace XUSG::Ultimate;

EZ::CommandList_DX12::CommandList_DX12() :
	XUSG::CommandList_DX12(),
	XUSG::EZ::CommandList_DX12(),
	m_meshShaderPipelineCache(nullptr),
	m_isMSStateDirty(false),
	m_meshShaderState(nullptr)
{
}

EZ::CommandList_DX12::CommandList_DX12(Ultimate::CommandList* pCommandList, uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
	uint32_t maxSamplers, const uint32_t* pMaxCbvsEachSpace, const uint32_t* pMaxSrvsEachSpace, const uint32_t* pMaxUavsEachSpace,
	uint32_t maxCbvSpaces, uint32_t maxSrvSpaces, uint32_t maxUavSpaces) :
	CommandList_DX12()
{
	Create(pCommandList, samplerPoolSize, cbvSrvUavPoolSize, maxSamplers,
		pMaxCbvsEachSpace, pMaxSrvsEachSpace, pMaxUavsEachSpace,
		maxCbvSpaces, maxSrvSpaces, maxUavSpaces);
}

EZ::CommandList_DX12::~CommandList_DX12()
{
}

bool EZ::CommandList_DX12::Create(Ultimate::CommandList* pCommandList, uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
	uint32_t maxSamplers, const uint32_t* pMaxCbvsEachSpace, const uint32_t* pMaxSrvsEachSpace, const uint32_t* pMaxUavsEachSpace,
	uint32_t maxCbvSpaces, uint32_t maxSrvSpaces, uint32_t maxUavSpaces)
{
	XUSG_N_RETURN(init(pCommandList, samplerPoolSize, cbvSrvUavPoolSize), false);

	// Create common pipeline layouts
	XUSG_N_RETURN(createGraphicsPipelineLayouts(maxSamplers, pMaxCbvsEachSpace, pMaxSrvsEachSpace,
		pMaxUavsEachSpace, maxCbvSpaces, maxSrvSpaces, maxUavSpaces), false);
	XUSG_N_RETURN(createComputePipelineLayouts(maxSamplers, pMaxCbvsEachSpace, pMaxSrvsEachSpace,
		pMaxUavsEachSpace, maxCbvSpaces, maxSrvSpaces, maxUavSpaces), false);
	XUSG_N_RETURN(createMeshShaderPipelineLayouts(maxSamplers, pMaxCbvsEachSpace, pMaxSrvsEachSpace,
		pMaxUavsEachSpace, maxCbvSpaces, maxSrvSpaces, maxUavSpaces), false);

	return true;
}

bool EZ::CommandList_DX12::Create(const Device* pDevice, void* pHandle, uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
	uint32_t maxSamplers, const uint32_t* pMaxCbvsEachSpace, const uint32_t* pMaxSrvsEachSpace, const uint32_t* pMaxUavsEachSpace,
	uint32_t maxCbvSpaces, uint32_t maxSrvSpaces, uint32_t maxUavSpaces, const wchar_t* name)
{
	m_pDevice = pDevice;
	Ultimate::CommandList_DX12::Create(pHandle, name);

	return Create(this, samplerPoolSize, cbvSrvUavPoolSize, maxSamplers,
		pMaxCbvsEachSpace, pMaxSrvsEachSpace, pMaxUavsEachSpace,
		maxCbvSpaces, maxSrvSpaces, maxUavSpaces);
}

void EZ::CommandList_DX12::ResolveSubresourceRegion(Resource* pDstResource, uint32_t dstSubresource,
	uint32_t dstX, uint32_t dstY, Resource* pSrcResource, uint32_t srcSubresource,
	const RectRange& srcRect, Format format, ResolveMode resolveMode)
{
	assert(pDstResource);
	assert(pSrcResource);

	// Generate barriers for each resource
	ResourceBarrier barriers[2];
	auto numBarriers = pDstResource->SetBarrier(barriers, ResourceState::RESOLVE_DEST, 0, dstSubresource);
	numBarriers = pSrcResource->SetBarrier(barriers, ResourceState::RESOLVE_SOURCE, numBarriers, srcSubresource);
	XUSG::CommandList_DX12::Barrier(numBarriers, barriers);

	Ultimate::CommandList_DX12::ResolveSubresourceRegion(pDstResource, dstSubresource,
		dstX, dstY, pSrcResource, srcSubresource, srcRect, format, resolveMode);
}

void EZ::CommandList_DX12::RSSetShadingRateImage(Resource* pShadingRateImage)
{
	// Set a barrier
	if (pShadingRateImage)
	{
		const auto numBarriersEst = 1u;
		const auto startIdx = m_barriers.size();
		m_barriers.resize(startIdx + numBarriersEst);
		const auto numBarriers = pShadingRateImage->SetBarrier(&m_barriers[startIdx], ResourceState::SHADING_RATE_SOURCE);

		// Shrink the size of barrier list
		if (numBarriers < numBarriersEst) m_barriers.resize(startIdx + numBarriers);
	}

	// Set the shading-rate image
	Ultimate::CommandList_DX12::RSSetShadingRateImage(pShadingRateImage);
}

void EZ::CommandList_DX12::SetPipelineState(const Pipeline& pipelineState)
{
	XUSG::CommandList_DX12::SetPipelineState(pipelineState);
	m_isGraphicsDirty = false;
	m_isComputeDirty = false;
	m_isMSStateDirty = false;
}

void EZ::CommandList_DX12::MSSetBlendState(MeshShader::BlendPreset preset, uint8_t numColorRTs, uint32_t sampleMask)
{
	assert(m_meshShaderState);
	m_meshShaderState->OMSetBlendState(preset, m_meshShaderPipelineCache.get(), numColorRTs, sampleMask);
	m_isMSStateDirty = true;
}

void EZ::CommandList_DX12::MSSetSample(uint8_t count, uint8_t quality)
{
	assert(m_meshShaderState);
	m_meshShaderState->OMSetSample(count, quality);
	m_isMSStateDirty = true;
}

void EZ::CommandList_DX12::MSSetRasterizerState(MeshShader::RasterizerPreset preset)
{
	assert(m_meshShaderState);
	m_meshShaderState->RSSetState(preset, m_meshShaderPipelineCache.get());
	m_isMSStateDirty = true;
}

void EZ::CommandList_DX12::MSSetDepthStencilState(MeshShader::DepthStencilPreset preset)
{
	assert(m_meshShaderState);
	m_meshShaderState->DSSetState(preset, m_meshShaderPipelineCache.get());
	m_isMSStateDirty = true;
}

void EZ::CommandList_DX12::MSSetShader(Shader::Stage stage, const Blob& shader)
{
	assert(stage == Shader::Stage::PS || stage == Shader::Stage::MS || stage == Shader::Stage::AS);
	assert(m_meshShaderState);
	m_meshShaderState->SetShader(stage, shader);
	m_isMSStateDirty = true;
}

void EZ::CommandList_DX12::MSSetNodeMask(uint32_t nodeMask)
{
	assert(m_meshShaderState);
	m_meshShaderState->SetNodeMask(nodeMask);
	m_isMSStateDirty = true;
}

void EZ::CommandList_DX12::DispatchMesh(uint32_t ThreadGroupCountX, uint32_t ThreadGroupCountY, uint32_t ThreadGroupCountZ)
{
	predispatchMesh();
	Ultimate::CommandList_DX12::DispatchMesh(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
}

void EZ::CommandList_DX12::DispatchMeshIndirect(const CommandLayout* pCommandlayout, uint32_t maxCommandCount,
	Resource* pArgumentBuffer, uint64_t argumentBufferOffset, Resource* pCountBuffer, uint64_t countBufferOffset)
{
	preexecuteIndirect(pArgumentBuffer, pCountBuffer);
	predispatchMesh();

	XUSG::CommandList_DX12::ExecuteIndirect(pCommandlayout, maxCommandCount,
		pArgumentBuffer, argumentBufferOffset, pCountBuffer, countBufferOffset);
}

bool EZ::CommandList_DX12::init(Ultimate::CommandList* pCommandList, uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize)
{
	XUSG_N_RETURN(XUSG::EZ::CommandList_DX12::init(pCommandList, samplerPoolSize, cbvSrvUavPoolSize), false);

	m_commandListU = dynamic_cast<Ultimate::CommandList_DX12*>(pCommandList)->GetGraphicsCommandList();

	if (m_commandListU)
	{
		m_meshShaderPipelineCache = MeshShader::PipelineCache::MakeUnique(m_pDevice, API::DIRECTX_12);
		m_meshShaderState = MeshShader::State::MakeUnique(API::DIRECTX_12);
	}

	return true;
}

bool EZ::CommandList_DX12::createMeshShaderPipelineLayouts(uint32_t maxSamplers,
	const uint32_t* pMaxCbvsEachSpace, const uint32_t* pMaxSrvsEachSpace, const uint32_t* pMaxUavsEachSpace,
	uint32_t maxCbvSpaces, uint32_t maxSrvSpaces, uint32_t maxUavSpaces)
{
	// Create common mesh-shader pipeline layout
	auto paramIndex = 0u;
	const auto pipelineLayout = Util::PipelineLayout::MakeUnique(API::DIRECTX_12);
	const auto maxSpaces = (max)(maxCbvSpaces, (max)(maxSrvSpaces, maxUavSpaces));

	for (uint8_t i = 0; i < NUM_STAGE; ++i)
	{
		pipelineLayout->SetRange(paramIndex, DescriptorType::SAMPLER, maxSamplers, 0, 0, DescriptorFlag::DATA_STATIC);
		pipelineLayout->SetShaderStage(paramIndex++, getShaderStage(i));
	}

	for (uint8_t i = 0; i < NUM_STAGE; ++i)
	{
		auto& descriptorTables = m_cbvSrvUavTables[i];
		if (descriptorTables[static_cast<uint32_t>(DescriptorType::CBV)].empty())
			descriptorTables[static_cast<uint32_t>(DescriptorType::CBV)].resize(maxCbvSpaces);
		if (descriptorTables[static_cast<uint32_t>(DescriptorType::SRV)].empty())
			descriptorTables[static_cast<uint32_t>(DescriptorType::SRV)].resize(maxSrvSpaces);
		if (descriptorTables[static_cast<uint32_t>(DescriptorType::UAV)].empty())
			descriptorTables[static_cast<uint32_t>(DescriptorType::UAV)].resize(maxUavSpaces);

		auto& spaceToParamIndexMap = m_meshShaderSpaceToParamIndexMap[i];
		spaceToParamIndexMap[static_cast<uint32_t>(DescriptorType::CBV)].resize(maxCbvSpaces);
		spaceToParamIndexMap[static_cast<uint32_t>(DescriptorType::SRV)].resize(maxSrvSpaces);
		spaceToParamIndexMap[static_cast<uint32_t>(DescriptorType::UAV)].resize(maxUavSpaces);
		for (auto s = 0u; s < maxSpaces; ++s)
		{
			const auto& stage = getShaderStage(i);

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

	XUSG_X_RETURN(m_pipelineLayout, pipelineLayout->GetPipelineLayout(m_pipelineLayoutCache.get(),
		PipelineLayoutFlag::NONE, L"EZMeshShaderLayout"), false);

	return true;
}

void EZ::CommandList_DX12::predispatchMesh()
{
	// Set barrier command
	XUSG::CommandList_DX12::Barrier(static_cast<uint32_t>(m_barriers.size()), m_barriers.data());
	m_barriers.clear();

	// Clear DSVs, RTVs, and UAVs
	clearDSVs();
	clearRTVs();
	clearUAVsUint();
	clearUAVsFloat();

	// Set descriptor tables
	for (uint8_t i = 0; i < NUM_STAGE; ++i)
	{
		const auto& stage = getShaderStage(i);

		// Create and set sampler table
		auto& samplerTable = m_samplerTables[stage];
		if (samplerTable)
		{
			const auto descriptorTable = samplerTable->GetSamplerTable(m_descriptorTableCache.get());
			if (descriptorTable) XUSG::CommandList_DX12::SetGraphicsDescriptorTable(i, descriptorTable);
			samplerTable.reset();
		}

		// Create and set CBV/SRV/UAV tables
		for (uint8_t t = 0; t < CbvSrvUavTypes; ++t)
		{
			auto& cbvSrvUavTables = m_cbvSrvUavTables[stage][t];
			const auto numSpaces = static_cast<uint32_t>(cbvSrvUavTables.size());
			for (auto s = 0u; s < numSpaces; ++s)
			{
				auto& cbvSrvUavTable = cbvSrvUavTables[s];
				if (cbvSrvUavTable)
				{
					const auto descriptorTable = cbvSrvUavTable->GetCbvSrvUavTable(m_descriptorTableCache.get());
					if (descriptorTable) XUSG::CommandList_DX12::SetGraphicsDescriptorTable(
						m_meshShaderSpaceToParamIndexMap[stage][t][s], descriptorTable);
					cbvSrvUavTable.reset();
				}
			}
		}
	}

	// Create pipeline for dynamic states
	assert(m_meshShaderState);
	if (m_isMSStateDirty)
	{
		m_meshShaderState->SetPipelineLayout(m_pipelineLayouts[GRAPHICS]);
		const auto pipeline = m_meshShaderState->GetPipeline(m_meshShaderPipelineCache.get());
		if (pipeline)
		{
			if (m_pipeline != pipeline)
			{
				XUSG::CommandList_DX12::SetPipelineState(pipeline);
				m_pipeline = pipeline;
			}
			m_isMSStateDirty = false;
		}
	}
}

const XUSG::Shader::Stage& EZ::CommandList_DX12::getShaderStage(uint8_t index) const
{
	static const Shader::Stage stages[] =
	{
		Shader::Stage::PS,
		Shader::Stage::MS,
		Shader::Stage::AS
	};
	assert(stages[PS] == Shader::Stage::PS);
	assert(stages[MS] == Shader::Stage::MS);
	assert(stages[AS] == Shader::Stage::AS);

	return stages[index];
}
