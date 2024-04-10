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
	m_meshShaderPipelineLib(nullptr),
	m_isMSStateDirty(false),
	m_meshShaderState(nullptr)
{
}

EZ::CommandList_DX12::CommandList_DX12(Ultimate::CommandList* pCommandList,
	uint32_t samplerHeapSize, uint32_t cbvSrvUavHeapSize,
	const uint32_t maxSamplers[Shader::Stage::NUM_STAGE],
	const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_STAGE],
	const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_STAGE],
	const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_STAGE],
	const uint32_t maxCbvSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t maxSrvSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t maxUavSpaces[Shader::Stage::NUM_STAGE],
	uint32_t slotExt, uint32_t spaceExt) :
	CommandList_DX12()
{
	Create(pCommandList, samplerHeapSize, cbvSrvUavHeapSize, maxSamplers,
		pMaxCbvsEachSpace, pMaxSrvsEachSpace, pMaxUavsEachSpace,
		maxCbvSpaces, maxSrvSpaces, maxUavSpaces, slotExt, spaceExt);
}

EZ::CommandList_DX12::~CommandList_DX12()
{
}

bool EZ::CommandList_DX12::Create(Ultimate::CommandList* pCommandList,
	uint32_t samplerHeapSize, uint32_t cbvSrvUavHeapSize,
	const uint32_t maxSamplers[Shader::Stage::NUM_STAGE],
	const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_STAGE],
	const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_STAGE],
	const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_STAGE],
	const uint32_t maxCbvSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t maxSrvSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t maxUavSpaces[Shader::Stage::NUM_STAGE],
	uint32_t slotExt, uint32_t spaceExt)
{
	XUSG_N_RETURN(init(pCommandList, samplerHeapSize, cbvSrvUavHeapSize), false);

	// Create common pipeline layouts
	XUSG_N_RETURN(createGraphicsPipelineLayouts(maxSamplers, pMaxCbvsEachSpace, pMaxSrvsEachSpace,
		pMaxUavsEachSpace, maxCbvSpaces, maxSrvSpaces, maxUavSpaces, slotExt, spaceExt), false);

	XUSG_N_RETURN(createComputePipelineLayouts(maxSamplers ? maxSamplers[Shader::Stage::CS] : 16,
		pMaxCbvsEachSpace ? pMaxCbvsEachSpace[Shader::Stage::CS] : nullptr,
		pMaxSrvsEachSpace ? pMaxSrvsEachSpace[Shader::Stage::CS] : nullptr,
		pMaxUavsEachSpace ? pMaxUavsEachSpace[Shader::Stage::CS] : nullptr,
		maxCbvSpaces ? maxCbvSpaces[Shader::Stage::CS] : 1,
		maxSrvSpaces ? maxSrvSpaces[Shader::Stage::CS] : 1,
		maxUavSpaces ? maxUavSpaces[Shader::Stage::CS] : 1,
		slotExt, spaceExt), false);

	XUSG_N_RETURN(createMeshShaderPipelineLayouts(maxSamplers, pMaxCbvsEachSpace, pMaxSrvsEachSpace,
		pMaxUavsEachSpace, maxCbvSpaces, maxSrvSpaces, maxUavSpaces, slotExt, spaceExt), false);

	return true;
}

bool EZ::CommandList_DX12::Create(const Device* pDevice, void* pHandle,
	uint32_t samplerHeapSize, uint32_t cbvSrvUavHeapSize,
	const uint32_t maxSamplers[Shader::Stage::NUM_STAGE],
	const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_STAGE],
	const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_STAGE],
	const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_STAGE],
	const uint32_t maxCbvSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t maxSrvSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t maxUavSpaces[Shader::Stage::NUM_STAGE],
	uint32_t slotExt, uint32_t spaceExt, const wchar_t* name)
{
	m_pDevice = pDevice;
	Ultimate::CommandList_DX12::Create(pHandle, name);

	return Create(this, samplerHeapSize, cbvSrvUavHeapSize, maxSamplers,
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

void EZ::CommandList_DX12::MSSetPipelineState(const Pipeline& pipelineState, const State* pState)
{
	assert(pipelineState || pState);
	Pipeline pipeline;

	if (pState)
	{
		pipeline = pState->GetPipeline(m_meshShaderPipelineLib.get());
		if (pipelineState && pipeline != pipelineState)
		{
			pipeline = pipelineState;
			m_meshShaderPipelineLib->SetPipeline(pState->GetKey(), pipeline);
		}
	}
	else pipeline = pipelineState;

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

void EZ::CommandList_DX12::MSSetBlendState(BlendPreset preset, uint8_t numColorRTs, uint32_t sampleMask)
{
	assert(m_meshShaderState);
	m_meshShaderState->OMSetBlendState(preset, m_meshShaderPipelineLib.get(), numColorRTs, sampleMask);
	m_isMSStateDirty = true;
}

void EZ::CommandList_DX12::MSSetSample(uint8_t count, uint8_t quality)
{
	assert(m_meshShaderState);
	m_meshShaderState->OMSetSample(count, quality);
	m_isMSStateDirty = true;
}

void EZ::CommandList_DX12::MSSetRasterizerState(RasterizerPreset preset)
{
	assert(m_meshShaderState);
	m_meshShaderState->RSSetState(preset, m_meshShaderPipelineLib.get());
	m_isMSStateDirty = true;
}

void EZ::CommandList_DX12::MSSetDepthStencilState(DepthStencilPreset preset)
{
	assert(m_meshShaderState);
	m_meshShaderState->DSSetState(preset, m_meshShaderPipelineLib.get());
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

const XUSG::PipelineLayout& EZ::CommandList_DX12::GetMSPipelineLayout() const
{
	return m_pipelineLayout;
}

bool EZ::CommandList_DX12::init(Ultimate::CommandList* pCommandList, uint32_t samplerHeapSize, uint32_t cbvSrvUavHeapSize)
{
	XUSG_N_RETURN(XUSG::EZ::CommandList_DX12::init(pCommandList, samplerHeapSize, cbvSrvUavHeapSize), false);

	m_commandListU = dynamic_cast<Ultimate::CommandList_DX12*>(pCommandList)->GetGraphicsCommandList();

	if (m_commandListU)
	{
		m_meshShaderPipelineLib = PipelineLib::MakeUnique(m_pDevice, API::DIRECTX_12);
		m_meshShaderState = State::MakeUnique(API::DIRECTX_12);
	}

	return true;
}

bool EZ::CommandList_DX12::createMeshShaderPipelineLayouts(
	const uint32_t maxSamplers[Shader::Stage::NUM_STAGE],
	const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_STAGE],
	const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_STAGE],
	const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_STAGE],
	const uint32_t maxCbvSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t maxSrvSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t maxUavSpaces[Shader::Stage::NUM_STAGE],
	uint32_t slotExt, uint32_t spaceExt)
{
	// Create common mesh-shader pipeline layout
	auto paramIndex = 0u;
	const auto pipelineLayout = Util::PipelineLayout::MakeUnique(API::DIRECTX_12);

	for (uint8_t i = 0; i < NUM_STAGE; ++i)
	{
		const auto& stage = getShaderStage(i);
		const auto stageMaxSamplers = maxSamplers ? maxSamplers[stage] : (i == PS ? 16 : 0);

		if (stageMaxSamplers > 0)
		{
			pipelineLayout->SetRange(paramIndex, DescriptorType::SAMPLER, stageMaxSamplers, 0, 0, DescriptorFlag::DESCRIPTORS_VOLATILE);
			pipelineLayout->SetShaderStage(paramIndex++, stage);
		}
	}

	for (uint8_t i = 0; i < NUM_STAGE; ++i)
	{
		const auto& stage = getShaderStage(i);
		const auto stageMaxCbvSpaces = maxCbvSpaces ? maxCbvSpaces[stage] : 1;
		const auto stageMaxSrvSpaces = maxSrvSpaces ? maxSrvSpaces[stage] : 1;
		const auto stageMaxUavSpaces = maxUavSpaces ? maxUavSpaces[stage] : 1;
		const auto maxSpaces = (max)(stageMaxCbvSpaces, (max)(stageMaxSrvSpaces, stageMaxUavSpaces));

		auto& descriptorTables = m_cbvSrvUavTables[stage];
		if (descriptorTables[static_cast<uint32_t>(DescriptorType::CBV)].empty())
			descriptorTables[static_cast<uint32_t>(DescriptorType::CBV)].resize(stageMaxCbvSpaces);
		if (descriptorTables[static_cast<uint32_t>(DescriptorType::SRV)].empty())
			descriptorTables[static_cast<uint32_t>(DescriptorType::SRV)].resize(stageMaxSrvSpaces);
		if (descriptorTables[static_cast<uint32_t>(DescriptorType::UAV)].empty())
			descriptorTables[static_cast<uint32_t>(DescriptorType::UAV)].resize(stageMaxUavSpaces);

		auto& spaceToParamIndexMap = m_meshShaderSpaceToParamIndexMap[i];
		spaceToParamIndexMap[static_cast<uint32_t>(DescriptorType::CBV)].resize(stageMaxCbvSpaces);
		spaceToParamIndexMap[static_cast<uint32_t>(DescriptorType::SRV)].resize(stageMaxSrvSpaces);
		spaceToParamIndexMap[static_cast<uint32_t>(DescriptorType::UAV)].resize(stageMaxUavSpaces);
		for (auto s = 0u; s < maxSpaces; ++s)
		{
			if (s < stageMaxCbvSpaces)
			{
				const auto maxDescriptors = pMaxCbvsEachSpace && pMaxCbvsEachSpace[stage] ? pMaxCbvsEachSpace[stage][s] : 14;
				spaceToParamIndexMap[static_cast<uint32_t>(DescriptorType::CBV)][s] = paramIndex;
				pipelineLayout->SetRange(paramIndex, DescriptorType::CBV, maxDescriptors, 0, s, DescriptorFlag::DESCRIPTORS_VOLATILE);
				pipelineLayout->SetShaderStage(paramIndex++, stage);
			}

			if (s < stageMaxSrvSpaces)
			{
				const auto maxDescriptors = pMaxSrvsEachSpace && pMaxSrvsEachSpace[stage] ? pMaxSrvsEachSpace[stage][s] : 32;
				spaceToParamIndexMap[static_cast<uint32_t>(DescriptorType::SRV)][s] = paramIndex;
				pipelineLayout->SetRange(paramIndex, DescriptorType::SRV, maxDescriptors, 0, s, DescriptorFlag::DESCRIPTORS_VOLATILE);
				pipelineLayout->SetShaderStage(paramIndex++, stage);
			}

			if (s < stageMaxUavSpaces)
			{
				const auto maxDescriptors = pMaxUavsEachSpace && pMaxUavsEachSpace[stage] ? pMaxUavsEachSpace[stage][s] : 16;
				spaceToParamIndexMap[static_cast<uint32_t>(DescriptorType::UAV)][s] = paramIndex;
				pipelineLayout->SetRange(paramIndex, DescriptorType::UAV, maxDescriptors, 0, s, DescriptorFlag::DESCRIPTORS_VOLATILE);
				pipelineLayout->SetShaderStage(paramIndex++, stage);
			}
		}
	}

	pipelineLayout->SetRange(paramIndex++, DescriptorType::UAV, 1, slotExt, spaceExt);

	XUSG_X_RETURN(m_pipelineLayout, pipelineLayout->GetPipelineLayout(m_pipelineLayoutLib.get(),
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
			const auto descriptorTable = samplerTable->GetSamplerTable(m_descriptorTableLib.get());
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
					const auto descriptorTable = cbvSrvUavTable->GetCbvSrvUavTable(m_descriptorTableLib.get());
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
		const auto pipeline = m_meshShaderState->GetPipeline(m_meshShaderPipelineLib.get());
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
