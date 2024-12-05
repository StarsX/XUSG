//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSGCommand_DX12.h"
#include "Core/XUSGPipelineLayout_DX12.h"
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
	m_pipelineLayout(nullptr),
	m_meshShaderState(nullptr),
	m_meshShaderConstantParamIndices()
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
	const uint32_t max32BitConstants[Shader::Stage::NUM_STAGE],
	const uint32_t constantSlots[Shader::Stage::NUM_STAGE],
	const uint32_t constantSpaces[Shader::Stage::NUM_STAGE],
	uint32_t slotExt, uint32_t spaceExt) :
	CommandList_DX12()
{
	Create(pCommandList, samplerHeapSize, cbvSrvUavHeapSize, maxSamplers,
		pMaxCbvsEachSpace, pMaxSrvsEachSpace, pMaxUavsEachSpace,
		maxCbvSpaces, maxSrvSpaces, maxUavSpaces, max32BitConstants,
		constantSlots, constantSpaces, slotExt, spaceExt);
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
	const uint32_t max32BitConstants[Shader::Stage::NUM_STAGE],
	const uint32_t constantSlots[Shader::Stage::NUM_STAGE],
	const uint32_t constantSpaces[Shader::Stage::NUM_STAGE],
	uint32_t slotExt, uint32_t spaceExt)
{
	XUSG_N_RETURN(init(pCommandList, samplerHeapSize, cbvSrvUavHeapSize), false);

	// Create common pipeline layouts
	XUSG_N_RETURN(createGraphicsPipelineLayouts(maxSamplers, pMaxCbvsEachSpace, pMaxSrvsEachSpace,
		pMaxUavsEachSpace, maxCbvSpaces, maxSrvSpaces, maxUavSpaces, max32BitConstants,
		constantSlots, constantSpaces, slotExt, spaceExt), false);

	const auto cMaxCbvSpaces = maxCbvSpaces ? maxCbvSpaces[Shader::Stage::CS] : 1;
	XUSG_N_RETURN(createComputePipelineLayouts(maxSamplers ? maxSamplers[Shader::Stage::CS] : 16,
		pMaxCbvsEachSpace ? pMaxCbvsEachSpace[Shader::Stage::CS] : nullptr,
		pMaxSrvsEachSpace ? pMaxSrvsEachSpace[Shader::Stage::CS] : nullptr,
		pMaxUavsEachSpace ? pMaxUavsEachSpace[Shader::Stage::CS] : nullptr,
		cMaxCbvSpaces,
		maxSrvSpaces ? maxSrvSpaces[Shader::Stage::CS] : 1,
		maxUavSpaces ? maxUavSpaces[Shader::Stage::CS] : 1,
		max32BitConstants ? max32BitConstants[Shader::Stage::CS] : 0,
		constantSlots ? constantSlots[Shader::Stage::CS] : 0,
		constantSpaces ? constantSpaces[Shader::Stage::CS] : cMaxCbvSpaces - 1,
		slotExt, spaceExt), false);

	XUSG_N_RETURN(createMeshShaderPipelineLayouts(maxSamplers, pMaxCbvsEachSpace, pMaxSrvsEachSpace,
		pMaxUavsEachSpace, maxCbvSpaces, maxSrvSpaces, maxUavSpaces, max32BitConstants,
		constantSlots, constantSpaces, slotExt, spaceExt), false);

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
	const uint32_t max32BitConstants[Shader::Stage::NUM_STAGE],
	const uint32_t constantSlots[Shader::Stage::NUM_STAGE],
	const uint32_t constantSpaces[Shader::Stage::NUM_STAGE],
	uint32_t slotExt, uint32_t spaceExt, const wchar_t* name)
{
	m_pDevice = pDevice;
	Ultimate::CommandList_DX12::Create(pHandle, name);

	return Create(this, samplerHeapSize, cbvSrvUavHeapSize, maxSamplers,
		pMaxCbvsEachSpace, pMaxSrvsEachSpace, pMaxUavsEachSpace,
		maxCbvSpaces, maxSrvSpaces, maxUavSpaces, max32BitConstants,
		constantSlots, constantSpaces, slotExt, spaceExt);
}

void EZ::CommandList_DX12::ResolveSubresourceRegion(Resource* pDstResource, uint32_t dstSubresource,
	uint32_t dstX, uint32_t dstY, Resource* pSrcResource, uint32_t srcSubresource,
	const RectRange& srcRect, Format format, ResolveMode resolveMode)
{
	assert(pDstResource);
	assert(pSrcResource);

	// Generate barriers for each resource
	XUSG::ResourceBarrier barriers[2];
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

void EZ::CommandList_DX12::SetGraphicsPipelineState(const Pipeline& pipelineState, const State* pState)
{
	assert(pipelineState || pState);
	Pipeline pipeline;

	if (pState)
	{
		pipeline = pState->GetPipeline(m_meshShaderPipelineLib.get());
		if (pipelineState && pipeline != pipelineState)
		{
			pipeline = pipelineState;
			m_meshShaderPipelineLib->SetPipeline(pState, pipeline);
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
		m_isGraphicsDirty = false;
	}
}

void EZ::CommandList_DX12::OMSetBlendState(const Graphics::Blend* pBlend, uint32_t sampleMask)
{
	assert(m_meshShaderState);
	m_meshShaderState->OMSetBlendState(pBlend, sampleMask);
	XUSG::EZ::CommandList_DX12::OMSetBlendState(pBlend, sampleMask);
}

void EZ::CommandList_DX12::OMSetBlendState(Graphics::BlendPreset preset, uint8_t numColorRTs, uint32_t sampleMask)
{
	assert(m_meshShaderState);
	m_meshShaderState->OMSetBlendState(preset, m_meshShaderPipelineLib.get(), numColorRTs, sampleMask);
	XUSG::EZ::CommandList_DX12::OMSetBlendState(preset, numColorRTs, sampleMask);
}

void EZ::CommandList_DX12::OMSetSample(uint8_t count, uint8_t quality)
{
	assert(m_meshShaderState);
	m_meshShaderState->OMSetSample(count, quality);
	XUSG::EZ::CommandList_DX12::OMSetSample(count, quality);
}

void EZ::CommandList_DX12::RSSetState(const Graphics::Rasterizer* pRasterizer)
{
	assert(m_meshShaderState);
	m_meshShaderState->RSSetState(pRasterizer);
	XUSG::EZ::CommandList_DX12::RSSetState(pRasterizer);
}

void EZ::CommandList_DX12::RSSetState(Graphics::RasterizerPreset preset)
{
	assert(m_meshShaderState);
	m_meshShaderState->RSSetState(preset, m_meshShaderPipelineLib.get());
	XUSG::EZ::CommandList_DX12::RSSetState(preset);
}

void EZ::CommandList_DX12::DSSetState(const Graphics::DepthStencil* pDepthStencil)
{
	assert(m_meshShaderState);
	m_meshShaderState->DSSetState(pDepthStencil);
	XUSG::EZ::CommandList_DX12::DSSetState(pDepthStencil);
}

void EZ::CommandList_DX12::DSSetState(Graphics::DepthStencilPreset preset)
{
	assert(m_meshShaderState);
	m_meshShaderState->DSSetState(preset, m_meshShaderPipelineLib.get());
	XUSG::EZ::CommandList_DX12::DSSetState(preset);
}

void EZ::CommandList_DX12::SetGraphicsShader(Shader::Stage stage, const Blob& shader)
{
	if (stage == Shader::Stage::MS || stage == Shader::Stage::AS)
	{
		assert(m_meshShaderState);
		m_meshShaderState->SetShader(stage, shader);
		m_isGraphicsDirty = true;
	}
	else if (stage == Shader::Stage::PS)
	{
		assert(m_meshShaderState);
		m_meshShaderState->SetShader(stage, shader);
		XUSG::EZ::CommandList_DX12::SetGraphicsShader(stage, shader);
	}
	else XUSG::EZ::CommandList_DX12::SetGraphicsShader(stage, shader);
}

void EZ::CommandList_DX12::SetMeshGraphicsShader(Shader::Stage stage, const Blob& shader)
{
	assert(m_meshShaderState);
	assert(stage == Shader::Stage::PS || stage == Shader::Stage::MS || stage == Shader::Stage::AS);
	m_meshShaderState->SetShader(stage, shader);
	m_isGraphicsDirty = true;
}

void EZ::CommandList_DX12::SetMeshGraphics32BitConstant(Shader::Stage stage, uint32_t srcData, uint32_t destOffsetIn32BitValues) const
{
	const auto stageIdx = getShaderStageIndex(stage);
	XUSG::CommandList_DX12::SetGraphics32BitConstant(m_meshShaderConstantParamIndices[stageIdx], srcData, destOffsetIn32BitValues);
}

void EZ::CommandList_DX12::SetMeshGraphics32BitConstants(Shader::Stage stage, uint32_t num32BitValuesToSet,
	const void* pSrcData, uint32_t destOffsetIn32BitValues) const
{
	const auto stageIdx = getShaderStageIndex(stage);
	XUSG::CommandList_DX12::SetGraphics32BitConstants(m_meshShaderConstantParamIndices[stageIdx],
		num32BitValuesToSet, pSrcData, destOffsetIn32BitValues);
}

void EZ::CommandList_DX12::SetGraphicsNodeMask(uint32_t nodeMask)
{
	assert(m_meshShaderState);
	m_meshShaderState->SetNodeMask(nodeMask);
	XUSG::EZ::CommandList_DX12::SetGraphicsNodeMask(nodeMask);
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

void EZ::CommandList_DX12::OMSetRenderTargets(uint32_t numRenderTargets,
	const XUSG::EZ::ResourceView* pRenderTargetViews,
	const XUSG::EZ::ResourceView* pDepthStencilView)
{
	// Set RTV barriers
	setBarriers(numRenderTargets, pRenderTargetViews);

	// Set DSV barriers
	if (pDepthStencilView)
	{
		setBarriers(1, pDepthStencilView);
		m_graphicsState->OMSetDSVFormat(dynamic_cast<Texture*>(pDepthStencilView->pResource)->GetFormat());
		m_meshShaderState->OMSetDSVFormat(dynamic_cast<Texture*>(pDepthStencilView->pResource)->GetFormat());
	}
	else m_graphicsState->OMSetDSVFormat(Format::UNKNOWN);

	m_graphicsState->OMSetNumRenderTargets(numRenderTargets);

	Descriptor pRTVs[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
	for (auto i = 0u; i < numRenderTargets; ++i)
	{
		pRTVs[i] = pRenderTargetViews[i].View;
		m_graphicsState->OMSetRTVFormat(i, dynamic_cast<Texture*>(pRenderTargetViews[i].pResource)->GetFormat());
		m_meshShaderState->OMSetRTVFormat(i, dynamic_cast<Texture*>(pRenderTargetViews[i].pResource)->GetFormat());
	}

	for (auto i = numRenderTargets; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
	{
		m_graphicsState->OMSetRTVFormat(i, Format::UNKNOWN);
		m_meshShaderState->OMSetRTVFormat(i, Format::UNKNOWN);
	}

	m_isGraphicsDirty = true;

	XUSG::CommandList_DX12::OMSetRenderTargets(numRenderTargets, pRTVs,
		pDepthStencilView ? &pDepthStencilView->View : nullptr);
}

const XUSG::PipelineLayout& EZ::CommandList_DX12::GetMSPipelineLayout() const
{
	return m_pipelineLayout;
}

uint32_t EZ::CommandList_DX12::GetMSConstantParamIndex(Shader::Stage stage) const
{
	return m_meshShaderConstantParamIndices[getShaderStageIndex(stage)];
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
	const uint32_t max32BitConstants[Shader::Stage::NUM_STAGE],
	const uint32_t constantSlots[Shader::Stage::NUM_STAGE],
	const uint32_t constantSpaces[Shader::Stage::NUM_STAGE],
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

		const auto num32BitValues = max32BitConstants ? max32BitConstants[stage] : 0;
		const auto constantSpace = constantSpaces ? constantSpaces[stage] : 0;
		const auto constantSlot = constantSlots ? constantSlots[stage] : 0;

		for (auto s = 0u; s < maxSpaces; ++s)
		{
			if (s < stageMaxCbvSpaces)
			{
				auto maxDescriptors = pMaxCbvsEachSpace && pMaxCbvsEachSpace[stage] ? pMaxCbvsEachSpace[stage][s] : 14;
				auto slot = 0;
				if (s == constantSpace && num32BitValues > 0)
				{
					maxDescriptors = constantSlot ? (min)(maxDescriptors, constantSlot) : maxDescriptors - 1;
					slot = constantSlot ? 0 : 1;
				}
				spaceToParamIndexMap[static_cast<uint32_t>(DescriptorType::CBV)][s] = paramIndex;
				pipelineLayout->SetRange(paramIndex, DescriptorType::CBV, maxDescriptors, slot, s, DescriptorFlag::DESCRIPTORS_VOLATILE);
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

		if (num32BitValues > 0)
		{
			m_meshShaderConstantParamIndices[i] = paramIndex;
			pipelineLayout->SetConstants(paramIndex++, num32BitValues, constantSlot, constantSpace, stage);
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

	// Set pipeline layout
	XUSG::CommandList_DX12::SetGraphicsPipelineLayout(m_pipelineLayout);

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
						m_meshShaderSpaceToParamIndexMap[i][t][s], descriptorTable);
					cbvSrvUavTable.reset();
				}
			}
		}
	}

	// Create pipeline for dynamic states
	assert(m_meshShaderState);
	if (m_isGraphicsDirty)
	{
		m_meshShaderState->SetPipelineLayout(m_pipelineLayout);
		const auto pipeline = m_meshShaderState->GetPipeline(m_meshShaderPipelineLib.get());
		if (pipeline)
		{
			if (m_pipeline != pipeline)
			{
				XUSG::CommandList_DX12::SetPipelineState(pipeline);
				m_pipeline = pipeline;
			}
			m_isGraphicsDirty = false;
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

EZ::CommandList_DX12::StageIndex EZ::CommandList_DX12::getShaderStageIndex(Shader::Stage stage) const
{
	switch (stage)
	{
	case Shader::Stage::PS:
		return PS;
	case Shader::Stage::MS:
		return MS;
	case Shader::Stage::AS:
		return AS;
	default:
		assert(!"Invalid shader stage");
		return PS;
	}
}
