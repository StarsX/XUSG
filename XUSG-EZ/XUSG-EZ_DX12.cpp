//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSGCommand_DX12.h"
#include "XUSG-EZ_DX12.h"

using namespace std;
using namespace XUSG;

EZ::CommandList_DX12::CommandList_DX12() :
	XUSG::CommandList_DX12(),
	m_graphicsCbvSrvUavTables(),
	m_computeCbvSrvUavTables(),
	m_graphicsSpaceToParamIndexMap(),
	m_computeSpaceToParamIndexMap()
{
}

EZ::CommandList_DX12::~CommandList_DX12()
{
}

EZ::CommandList_DX12::CommandList_DX12(const Device* pDevice, XUSG::CommandList* pCommandList, uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
	uint32_t maxSamplers, const uint32_t* pMaxCbvsEachSpace, const uint32_t* pMaxSrvsEachSpace, const uint32_t* pMaxUavsEachSpace,
	uint32_t maxCbvSpaces, uint32_t maxSrvSpaces, uint32_t maxUavSpaces) :
	CommandList_DX12()
{
	Create(pDevice, pCommandList, samplerPoolSize, cbvSrvUavPoolSize, maxSamplers,
		pMaxCbvsEachSpace, pMaxSrvsEachSpace, pMaxUavsEachSpace,
		maxCbvSpaces, maxSrvSpaces, maxUavSpaces);
}

bool EZ::CommandList_DX12::Create(const Device* pDevice, XUSG::CommandList* pCommandList, uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
	uint32_t maxSamplers, const uint32_t* pMaxCbvsEachSpace, const uint32_t* pMaxSrvsEachSpace, const uint32_t* pMaxUavsEachSpace,
	uint32_t maxCbvSpaces, uint32_t maxSrvSpaces, uint32_t maxUavSpaces)
{
	m_commandList = dynamic_cast<XUSG::CommandList_DX12*>(pCommandList)->GetGraphicsCommandList();

	m_graphicsPipelineCache = Graphics::PipelineCache::MakeUnique(pDevice);
	m_computePipelineCache = Compute::PipelineCache::MakeUnique(pDevice);
	m_pipelineLayoutCache = PipelineLayoutCache::MakeUnique(pDevice);
	m_descriptorTableCache = DescriptorTableCache::MakeUnique(pDevice);

	// Allocate descriptor pools
	m_descriptorTableCache->AllocateDescriptorPool(DescriptorPoolType::SAMPLER_POOL, samplerPoolSize);
	m_descriptorTableCache->AllocateDescriptorPool(DescriptorPoolType::CBV_SRV_UAV_POOL, cbvSrvUavPoolSize);

	// Create common pipeline layouts
	createPipelineLayouts(maxSamplers, pMaxCbvsEachSpace, pMaxSrvsEachSpace, pMaxUavsEachSpace, maxCbvSpaces, maxSrvSpaces, maxUavSpaces);

	return true;
}

bool EZ::CommandList_DX12::Reset(const CommandAllocator* pAllocator, const Pipeline& initialState) const
{
	const auto ret = Reset(pAllocator, initialState);

	// Set Descriptor pools
	const DescriptorPool descriptorPools[] =
	{
		m_descriptorTableCache->GetDescriptorPool(CBV_SRV_UAV_POOL),
		m_descriptorTableCache->GetDescriptorPool(SAMPLER_POOL)
	};
	XUSG::CommandList_DX12::SetDescriptorPools(static_cast<uint32_t>(size(descriptorPools)), descriptorPools);

	// Set pipeline layouts
	XUSG::CommandList_DX12::SetGraphicsPipelineLayout(m_pipelineLayouts[GRAPHICS]);
	XUSG::CommandList_DX12::SetComputePipelineLayout(m_pipelineLayouts[COMPUTE]);

	return ret;
}

void EZ::CommandList_DX12::Draw(uint32_t vertexCountPerInstance, uint32_t instanceCount,
		uint32_t startVertexLocation, uint32_t startInstanceLocation)
{
	predraw();
	XUSG::CommandList_DX12::Draw(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
}

void EZ::CommandList_DX12::DrawIndexed(uint32_t indexCountPerInstance, uint32_t instanceCount,
	uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation)
{
	predraw();
	XUSG::CommandList_DX12::DrawIndexed(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
}

void EZ::CommandList_DX12::Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
{
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

	// Set barriers
	XUSG::CommandList_DX12::Barrier(static_cast<uint32_t>(m_barriers.size()), m_barriers.data());
	m_barriers.clear();

	// Create pipeline for dynamic states
	if (m_computeState)
	{
		const auto pipeline = m_computeState->GetPipeline(m_computePipelineCache.get());
		if (pipeline) XUSG::CommandList_DX12::SetPipelineState(pipeline);
	}

	XUSG::CommandList_DX12::Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}

void EZ::CommandList_DX12::IASetPrimitiveTopology(PrimitiveTopology primitiveTopology)
{
	if (m_graphicsState)
	{
		if (primitiveTopology == PrimitiveTopology::POINTLIST)
			m_graphicsState->IASetPrimitiveTopologyType(PrimitiveTopologyType::POINT);
		else if (primitiveTopology == PrimitiveTopology::LINELIST || primitiveTopology == PrimitiveTopology::LINESTRIP ||
			primitiveTopology == PrimitiveTopology::LINELIST_ADJ || primitiveTopology == PrimitiveTopology::LINESTRIP_ADJ)
			m_graphicsState->IASetPrimitiveTopologyType(PrimitiveTopologyType::LINE);
		else if (primitiveTopology == PrimitiveTopology::TRIANGLELIST || primitiveTopology == PrimitiveTopology::TRIANGLESTRIP ||
			primitiveTopology == PrimitiveTopology::TRIANGLELIST_ADJ || primitiveTopology == PrimitiveTopology::TRIANGLESTRIP_ADJ)
			m_graphicsState->IASetPrimitiveTopologyType(PrimitiveTopologyType::TRIANGLE);
		else if (primitiveTopology >= PrimitiveTopology::CONTROL_POINT1_PATCHLIST && primitiveTopology <= PrimitiveTopology::CONTROL_POINT32_PATCHLIST)
			m_graphicsState->IASetPrimitiveTopologyType(PrimitiveTopologyType::PATCH);
		else
		{
			assert(primitiveTopology == PrimitiveTopology::UNDEFINED);
			m_graphicsState->IASetPrimitiveTopologyType(PrimitiveTopologyType::UNDEFINED);
		}
	}

	XUSG::CommandList_DX12::IASetPrimitiveTopology(primitiveTopology);
}

void EZ::CommandList_DX12::SetPipelineState(const Pipeline& pipelineState)
{
	XUSG::CommandList_DX12::SetPipelineState(pipelineState);
	m_graphicsState.reset();
	m_computeState.reset();
}

void EZ::CommandList_DX12::SetGraphicsSamplerStates(uint32_t startBinding, uint32_t numSamplers, const SamplerPreset* pSamplerStates)
{
	auto& descriptorTable = m_samplerTables[GRAPHICS];
	if (!descriptorTable) descriptorTable = Util::DescriptorTable::MakeUnique();
	descriptorTable->SetSamplers(startBinding, numSamplers, pSamplerStates, m_descriptorTableCache.get());
}

void EZ::CommandList_DX12::SetGraphicsResources(Shader::Stage stage, DescriptorType descriptorType,
	uint32_t startBinding, uint32_t numResources, const ResourceView* pResourceViews, uint32_t space)
{
	auto& descriptorTables = m_graphicsCbvSrvUavTables[stage][static_cast<uint32_t>(descriptorType)];
	assert(space < descriptorTables.size());
	auto& descriptorTable = descriptorTables[space];

	// Set descriptors to the descriptor table
	if (!descriptorTable) descriptorTable = Util::DescriptorTable::MakeUnique();
	vector<Descriptor> descriptors(numResources);
	for (auto i = 0u; i < numResources; ++i) descriptors[i] = pResourceViews[i].view;
	descriptorTable->SetDescriptors(startBinding, numResources, descriptors.data());

	// Set barriers
	if (descriptorType == DescriptorType::SRV || descriptorType == DescriptorType::UAV)
	{
		ResourceState dstState;
		if (descriptorType == DescriptorType::UAV) dstState = ResourceState::UNORDERED_ACCESS;
		else if (stage == Shader::Stage::PS) dstState = ResourceState::PIXEL_SHADER_RESOURCE;
		else dstState = ResourceState::NON_PIXEL_SHADER_RESOURCE;

		vector<ResourceBarrier> barriers(numResources);
		auto numBarriers = 0u;
		// [TODO] Per subresource
		for (auto i = 0u; i < numResources; ++i)
			numBarriers = pResourceViews[i].pResource->SetBarrier(barriers.data(), dstState, numBarriers);

		const auto i = m_barriers.size();
		m_barriers.resize(m_barriers.size() + numBarriers);
		memcpy(&m_barriers[i], barriers.data(), sizeof(ResourceBarrier) * numBarriers);
	}
}

void EZ::CommandList_DX12::SetComputeSamplerStates(uint32_t startBinding, uint32_t numSamplers, const SamplerPreset* pSamplerStates)
{
	auto& descriptorTable = m_samplerTables[COMPUTE];
	if (!descriptorTable) descriptorTable = Util::DescriptorTable::MakeUnique();
	descriptorTable->SetSamplers(startBinding, numSamplers, pSamplerStates, m_descriptorTableCache.get());
}

void EZ::CommandList_DX12::SetComputeResources(DescriptorType descriptorType, uint32_t startBinding,
	uint32_t numResources, const ResourceView* pResourceViews, uint32_t space)
{
	auto& descriptorTables = m_computeCbvSrvUavTables[static_cast<uint32_t>(descriptorType)];
	assert(space < descriptorTables.size());
	auto& descriptorTable = descriptorTables[space];

	// Set descriptors to the descriptor table
	if (!descriptorTable) descriptorTable = Util::DescriptorTable::MakeUnique();
	vector<Descriptor> descriptors(numResources);
	for (auto i = 0u; i < numResources; ++i) descriptors[i] = pResourceViews[i].view;
	descriptorTable->SetDescriptors(startBinding, numResources, descriptors.data());

	// Set barriers
	if (descriptorType == DescriptorType::SRV || descriptorType == DescriptorType::UAV)
	{
		const auto dstState = descriptorType == DescriptorType::UAV ?
			ResourceState::UNORDERED_ACCESS : ResourceState::NON_PIXEL_SHADER_RESOURCE;

		vector<ResourceBarrier> barriers(numResources);
		auto numBarriers = 0u;
		// [TODO] Per subresource
		for (auto i = 0u; i < numResources; ++i)
			numBarriers = pResourceViews[i].pResource->SetBarrier(barriers.data(), dstState, numBarriers);

		const auto i = m_barriers.size();
		m_barriers.resize(m_barriers.size() + numBarriers);
		memcpy(&m_barriers[i], barriers.data(), sizeof(ResourceBarrier) * numBarriers);
	}
}

void EZ::CommandList_DX12::OMSetRenderTargets(uint32_t numRenderTargets,
	const ResourceView* pRenderTargetViews, const ResourceView* pDepthStencilView)
{
	vector<ResourceBarrier> barriers(numRenderTargets + (pDepthStencilView ? 1 : 0));
	auto numBarriers = 0u;
	// [TODO] Per subresource
	for (auto i = 0u; i < numRenderTargets; ++i)
		numBarriers = pRenderTargetViews[i].pResource->SetBarrier(barriers.data(), ResourceState::RENDER_TARGET, numBarriers);
	if (pDepthStencilView) numBarriers = pDepthStencilView->pResource->SetBarrier(barriers.data(), ResourceState::DEPTH_WRITE, numBarriers);

	const auto i = m_barriers.size();
	m_barriers.resize(m_barriers.size() + numBarriers);
	memcpy(&m_barriers[i], barriers.data(), sizeof(ResourceBarrier) * numBarriers);

	vector<Descriptor> rtvs(numRenderTargets);
	for (auto i = 0u; i < numRenderTargets; ++i) rtvs[i] = pRenderTargetViews[i].view;
	XUSG::CommandList_DX12::OMSetRenderTargets(numRenderTargets, rtvs.data(),
		pDepthStencilView ? &pDepthStencilView->view : nullptr);
}

void EZ::CommandList_DX12::ResetDescriptorPool(DescriptorPoolType type)
{
	m_descriptorTableCache->ResetDescriptorPool(type, 0);
}

void EZ::CommandList_DX12::Resize()
{
	ResetDescriptorPool(DescriptorPoolType::CBV_SRV_UAV_POOL);
}

void EZ::CommandList_DX12::Create(const Device* pDevice, void* pHandle, uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
	uint32_t maxSamplers, const uint32_t* pMaxCbvsEachSpace, const uint32_t* pMaxSrvsEachSpace, const uint32_t* pMaxUavsEachSpace,
	uint32_t maxCbvSpaces, uint32_t maxSrvSpaces, uint32_t maxUavSpaces, const wchar_t* name)
{
	XUSG::CommandList_DX12::Create(pHandle, name);
	Create(pDevice, this, samplerPoolSize, cbvSrvUavPoolSize, maxSamplers,
		pMaxCbvsEachSpace, pMaxSrvsEachSpace, pMaxUavsEachSpace,
		maxCbvSpaces, maxSrvSpaces, maxUavSpaces);
}

Graphics::State* EZ::CommandList_DX12::GetGraphicsPipelineState()
{
	if (!m_graphicsState) m_graphicsState = Graphics::State::MakeUnique();

	return m_graphicsState.get();
}

Compute::State* EZ::CommandList_DX12::GetComputePipelineState()
{
	if (!m_computeState) m_computeState = Compute::State::MakeUnique();

	return m_computeState.get();
}

bool EZ::CommandList_DX12::createPipelineLayouts(uint32_t maxSamplers,
	const uint32_t* pMaxCbvsEachSpace, const uint32_t* pMaxSrvsEachSpace, const uint32_t* pMaxUavsEachSpace,
	uint32_t maxCbvSpaces, uint32_t maxSrvSpaces, uint32_t maxUavSpaces)
{
	const auto maxSpaces = (max)(maxCbvSpaces, (max)(maxSrvSpaces, maxUavSpaces));
	{
		// Create common graphics pipeline layout
		auto paramIndex = 0u;
		const auto pipelineLayout = Util::PipelineLayout::MakeUnique();
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
					const auto maxDescriptors = pMaxCbvsEachSpace ? pMaxCbvsEachSpace[s] : 32;
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
		const auto pipelineLayout = Util::PipelineLayout::MakeUnique();
		pipelineLayout->SetRange(paramIndex++, DescriptorType::SAMPLER, maxSamplers, 0, 0, DescriptorFlag::DATA_STATIC);

		m_computeSpaceToParamIndexMap[static_cast<uint32_t>(DescriptorType::CBV)].resize(maxCbvSpaces);
		m_computeSpaceToParamIndexMap[static_cast<uint32_t>(DescriptorType::SRV)].resize(maxSrvSpaces);
		m_computeSpaceToParamIndexMap[static_cast<uint32_t>(DescriptorType::UAV)].resize(maxUavSpaces);
		for (auto s = 0u; s < maxSpaces; ++s)
		{
			if (s < maxCbvSpaces)
			{
				const auto maxDescriptors = pMaxCbvsEachSpace ? pMaxCbvsEachSpace[s] : 32;
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

		X_RETURN(m_pipelineLayouts[COMPUTE], pipelineLayout->GetPipelineLayout(m_pipelineLayoutCache.get(),
			PipelineLayoutFlag::NONE, L"EZComputeLayout"), false);
	}

	return true;
}

void EZ::CommandList_DX12::predraw()
{
	// Create and set sampler table
	auto& samplerTable = m_samplerTables[GRAPHICS];
	if (samplerTable)
	{
		const auto descriptorTable = samplerTable->GetSamplerTable(m_descriptorTableCache.get());
		if (descriptorTable) XUSG::CommandList_DX12::SetGraphicsDescriptorTable(0, descriptorTable);
		samplerTable.reset();
	}

	// Create and set CBV/SRV/UAV tables
	for (uint8_t i = 0; i < Shader::Stage::NUM_GRAPHICS; ++i)
	{
		for (uint8_t t = 0; t < CbvSrvUavTypes; ++t)
		{
			auto& cbvSrvUavTables = m_graphicsCbvSrvUavTables[i][t];
			const auto numSpaces = static_cast<uint32_t>(cbvSrvUavTables.size());
			for (auto s = 0u; s < numSpaces; ++s)
			{
				auto& cbvSrvUavTable = cbvSrvUavTables[s];
				if (cbvSrvUavTable)
				{
					const auto descriptorTable = cbvSrvUavTable->GetCbvSrvUavTable(m_descriptorTableCache.get());
					if (descriptorTable) XUSG::CommandList_DX12::SetGraphicsDescriptorTable(
						m_graphicsSpaceToParamIndexMap[i][t][s], descriptorTable);
					cbvSrvUavTable.reset();
				}
			}
		}
	}

	// Set barriers
	XUSG::CommandList_DX12::Barrier(static_cast<uint32_t>(m_barriers.size()), m_barriers.data());
	m_barriers.clear();

	// Create pipeline for dynamic states
	if (m_graphicsState)
	{
		const auto pipeline = m_graphicsState->GetPipeline(m_graphicsPipelineCache.get());
		if (pipeline) XUSG::CommandList_DX12::SetPipelineState(pipeline);
	}
}
