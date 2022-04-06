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

EZ::CommandList_DX12::CommandList_DX12(XUSG::CommandList* pCommandList, uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
	uint32_t maxSamplers, const uint32_t* pMaxCbvsEachSpace, const uint32_t* pMaxSrvsEachSpace, const uint32_t* pMaxUavsEachSpace,
	uint32_t maxCbvSpaces, uint32_t maxSrvSpaces, uint32_t maxUavSpaces) :
	CommandList_DX12()
{
	Create(pCommandList, samplerPoolSize, cbvSrvUavPoolSize, maxSamplers,
		pMaxCbvsEachSpace, pMaxSrvsEachSpace, pMaxUavsEachSpace,
		maxCbvSpaces, maxSrvSpaces, maxUavSpaces);
}

bool EZ::CommandList_DX12::Create(XUSG::CommandList* pCommandList, uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
	uint32_t maxSamplers, const uint32_t* pMaxCbvsEachSpace, const uint32_t* pMaxSrvsEachSpace, const uint32_t* pMaxUavsEachSpace,
	uint32_t maxCbvSpaces, uint32_t maxSrvSpaces, uint32_t maxUavSpaces)
{
	const auto pDevice = pCommandList->GetDevice();
	m_commandList = dynamic_cast<XUSG::CommandList_DX12*>(pCommandList)->GetGraphicsCommandList();

	m_graphicsPipelineCache = Graphics::PipelineCache::MakeUnique(pDevice, API::DIRECTX_12);
	m_computePipelineCache = Compute::PipelineCache::MakeUnique(pDevice, API::DIRECTX_12);
	m_pipelineLayoutCache = PipelineLayoutCache::MakeUnique(pDevice, API::DIRECTX_12);
	m_descriptorTableCache = DescriptorTableCache::MakeUnique(pDevice, L"EZDescirptorTableCache", API::DIRECTX_12);

	// Allocate descriptor pools
	m_descriptorTableCache->AllocateDescriptorPool(DescriptorPoolType::SAMPLER_POOL, samplerPoolSize);
	m_descriptorTableCache->AllocateDescriptorPool(DescriptorPoolType::CBV_SRV_UAV_POOL, cbvSrvUavPoolSize);

	// Create common pipeline layouts
	createPipelineLayouts(maxSamplers, pMaxCbvsEachSpace, pMaxSrvsEachSpace, pMaxUavsEachSpace, maxCbvSpaces, maxSrvSpaces, maxUavSpaces);

	return true;
}

bool EZ::CommandList_DX12::Reset(const CommandAllocator* pAllocator, const Pipeline& initialState)
{
	const auto ret = XUSG::CommandList_DX12::Reset(pAllocator, initialState);

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
	m_pipeline = initialState;
	m_pInputLayout = nullptr;

	m_barriers.clear();
	m_clearDSVs.clear();

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

	// Set barrier command
	XUSG::CommandList_DX12::Barrier(static_cast<uint32_t>(m_barriers.size()), m_barriers.data());
	m_barriers.clear();

	// Create pipeline for dynamic states
	if (m_computeState)
	{
		m_computeState->SetPipelineLayout(m_pipelineLayouts[COMPUTE]);
		const auto pipeline = m_computeState->GetPipeline(m_computePipelineCache.get());
		if (pipeline && m_pipeline != pipeline) XUSG::CommandList_DX12::SetPipelineState(pipeline);
	}

	XUSG::CommandList_DX12::Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}

void EZ::CommandList_DX12::CopyBufferRegion(Resource* pDstBuffer, uint64_t dstOffset,
	Resource* pSrcBuffer, uint64_t srcOffset, uint64_t numBytes)
{
	// Generate barriers for each resource
	ResourceBarrier barriers[2];
	auto numBarriers = pDstBuffer->SetBarrier(barriers, ResourceState::COPY_DEST);
	numBarriers = pSrcBuffer->SetBarrier(barriers, ResourceState::COPY_SOURCE, numBarriers);
	XUSG::CommandList_DX12::Barrier(numBarriers, barriers);

	XUSG::CommandList_DX12::CopyBufferRegion(pDstBuffer, dstOffset, pSrcBuffer, srcOffset, numBytes);
}

void EZ::CommandList_DX12::CopyTextureRegion(const TextureCopyLocation& dst, uint32_t dstX,
	uint32_t dstY, uint32_t dstZ, const TextureCopyLocation& src, const BoxRange* pSrcBox)
{
	// Generate barriers for each resource
	ResourceBarrier barriers[2];
	auto numBarriers = const_cast<Resource*>(dst.pResource)->SetBarrier(
		barriers, ResourceState::COPY_DEST, 0, dst.SubresourceIndex);
	numBarriers = const_cast<Resource*>(src.pResource)->SetBarrier(
		barriers, ResourceState::COPY_SOURCE, numBarriers, src.SubresourceIndex);
	XUSG::CommandList_DX12::Barrier(numBarriers, barriers);

	XUSG::CommandList_DX12::CopyTextureRegion(dst, dstX, dstY, dstZ, src, pSrcBox);
}

void EZ::CommandList_DX12::CopyResource(Resource* pDstResource, Resource* pSrcResource)
{
	// Generate barriers for each resource
	ResourceBarrier barriers[2];
	auto numBarriers = pDstResource->SetBarrier(barriers, ResourceState::COPY_DEST);
	numBarriers = pSrcResource->SetBarrier(barriers, ResourceState::COPY_SOURCE, numBarriers);
	XUSG::CommandList_DX12::Barrier(numBarriers, barriers);

	XUSG::CommandList_DX12::CopyResource(pDstResource, pSrcResource);
}

void EZ::CommandList_DX12::CopyTiles(Resource* pTiledResource, const TiledResourceCoord* pTileRegionStartCoord,
	const TileRegionSize* pTileRegionSize, const Resource* pBuffer, uint64_t bufferStartOffsetInBytes, TileCopyFlag flags)
{
	ResourceBarrier barrier;
	const auto numBarriers = pTiledResource->SetBarrier(&barrier, ResourceState::COPY_DEST);
	XUSG::CommandList_DX12::Barrier(numBarriers, &barrier);

	XUSG::CommandList_DX12::CopyTiles(pTiledResource, pTileRegionStartCoord,
		pTileRegionSize, pBuffer, bufferStartOffsetInBytes, flags);
}

void EZ::CommandList_DX12::ResolveSubresource(Resource* pDstResource, uint32_t dstSubresource,
	Resource* pSrcResource, uint32_t srcSubresource, Format format)
{
	// Generate barriers for each resource
	ResourceBarrier barriers[2];
	auto numBarriers = pDstResource->SetBarrier(barriers, ResourceState::RESOLVE_DEST, 0, dstSubresource);
	numBarriers = pSrcResource->SetBarrier(barriers, ResourceState::RESOLVE_SOURCE, numBarriers, srcSubresource);
	XUSG::CommandList_DX12::Barrier(numBarriers, barriers);

	XUSG::CommandList_DX12::ResolveSubresource(pDstResource, dstSubresource, pSrcResource, srcSubresource, format);
}

void EZ::CommandList_DX12::RSSetState(Graphics::RasterizerPreset preset)
{
	if (!m_graphicsState) m_graphicsState = Graphics::State::MakeUnique(API::DIRECTX_12);
	m_graphicsState->RSSetState(preset, m_graphicsPipelineCache.get());
}

void EZ::CommandList_DX12::DSSetState(Graphics::DepthStencilPreset preset)
{
	if (!m_graphicsState) m_graphicsState = Graphics::State::MakeUnique(API::DIRECTX_12);
	m_graphicsState->DSSetState(preset, m_graphicsPipelineCache.get());
}

void EZ::CommandList_DX12::SetPipelineState(const Pipeline& pipelineState)
{
	XUSG::CommandList_DX12::SetPipelineState(pipelineState);
	m_graphicsState.reset();
	m_computeState.reset();
}

void EZ::CommandList_DX12::SetGraphicsSamplerStates(uint32_t startBinding, uint32_t numSamplers, const SamplerPreset* pSamplerPresets)
{
	auto& descriptorTable = m_samplerTables[GRAPHICS];
	if (!descriptorTable) descriptorTable = Util::DescriptorTable::MakeUnique(API::DIRECTX_12);
	descriptorTable->SetSamplers(startBinding, numSamplers, pSamplerPresets, m_descriptorTableCache.get());
}

void EZ::CommandList_DX12::SetGraphicsResources(Shader::Stage stage, DescriptorType descriptorType,
	uint32_t startBinding, uint32_t numResources, const ResourceView* pResourceViews, uint32_t space)
{
	auto& descriptorTables = m_graphicsCbvSrvUavTables[stage][static_cast<uint32_t>(descriptorType)];
	assert(space < descriptorTables.size());
	auto& descriptorTable = descriptorTables[space];

	// Set descriptors to the descriptor table
	if (!descriptorTable) descriptorTable = Util::DescriptorTable::MakeUnique(API::DIRECTX_12);
	if (m_descriptors.size() < numResources) m_descriptors.resize(numResources);
	for (auto i = 0u; i < numResources; ++i) m_descriptors[i] = pResourceViews[i].view;
	descriptorTable->SetDescriptors(startBinding, numResources, m_descriptors.data());

	// Set barriers
	if (descriptorType == DescriptorType::SRV || descriptorType == DescriptorType::UAV)
	{
		const auto dstState = descriptorType == DescriptorType::UAV ?
			ResourceState::UNORDERED_ACCESS : ResourceState::NON_PIXEL_SHADER_RESOURCE |
			ResourceState::PIXEL_SHADER_RESOURCE;
		setBarriers(numResources, pResourceViews, dstState);
	}
}

void EZ::CommandList_DX12::SetComputeSamplerStates(uint32_t startBinding, uint32_t numSamplers, const SamplerPreset* pSamplerPresets)
{
	auto& descriptorTable = m_samplerTables[COMPUTE];
	if (!descriptorTable) descriptorTable = Util::DescriptorTable::MakeUnique(API::DIRECTX_12);
	descriptorTable->SetSamplers(startBinding, numSamplers, pSamplerPresets, m_descriptorTableCache.get());
}

void EZ::CommandList_DX12::SetComputeResources(DescriptorType descriptorType, uint32_t startBinding,
	uint32_t numResources, const ResourceView* pResourceViews, uint32_t space)
{
	auto& descriptorTables = m_computeCbvSrvUavTables[static_cast<uint32_t>(descriptorType)];
	assert(space < descriptorTables.size());
	auto& descriptorTable = descriptorTables[space];

	// Set descriptors to the descriptor table
	if (!descriptorTable) descriptorTable = Util::DescriptorTable::MakeUnique(API::DIRECTX_12);
	if (m_descriptors.size() < numResources) m_descriptors.resize(numResources);
	for (auto i = 0u; i < numResources; ++i) m_descriptors[i] = pResourceViews[i].view;
	descriptorTable->SetDescriptors(startBinding, numResources, m_descriptors.data());

	// Set barriers
	if (descriptorType == DescriptorType::SRV || descriptorType == DescriptorType::UAV)
	{
		const auto dstState = descriptorType == DescriptorType::UAV ?
			ResourceState::UNORDERED_ACCESS : ResourceState::NON_PIXEL_SHADER_RESOURCE;
		setBarriers(numResources, pResourceViews, dstState);
	}
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

void EZ::CommandList_DX12::SOSetTargets(uint32_t startSlot, uint32_t numViews, const StreamOutBufferView* pViews, Resource* const* ppResources)
{
	vector<ResourceView> resourceViews(numViews);
	for (auto i = 0u; i < numViews; ++i)
	{
		auto& resourceView = resourceViews[i];
		resourceView.pResource = ppResources[i];
		resourceView.Subresources = { BARRIER_ALL_SUBRESOURCES };
	}

	setBarriers(numViews, resourceViews.data(), ResourceState::STREAM_OUT);

	XUSG::CommandList_DX12::SOSetTargets(startSlot, numViews, pViews);
}

void EZ::CommandList_DX12::OMSetRenderTargets(uint32_t numRenderTargets,
	const ResourceView* pRenderTargetViews, const ResourceView* pDepthStencilView)
{
	// Set RTV barriers
	setBarriers(numRenderTargets, pRenderTargetViews, ResourceState::RENDER_TARGET);

	// Set DSV barriers
	if (pDepthStencilView)
	{
		setBarriers(1, pDepthStencilView, ResourceState::DEPTH_WRITE);
		m_graphicsState->OMSetDSVFormat(dynamic_cast<Texture*>(pDepthStencilView->pResource)->GetFormat());
	}
	else m_graphicsState->OMSetDSVFormat(Format::UNKNOWN);

	m_graphicsState->OMSetNumRenderTargets(numRenderTargets);

	Descriptor pRTVs[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
	for (auto i = 0u; i < numRenderTargets; ++i)
	{
		pRTVs[i] = pRenderTargetViews[i].view;
		m_graphicsState->OMSetRTVFormat(i, dynamic_cast<Texture*>(pRenderTargetViews[i].pResource)->GetFormat());
	}

	for (auto i = numRenderTargets; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		m_graphicsState->OMSetRTVFormat(i, Format::UNKNOWN);

	XUSG::CommandList_DX12::OMSetRenderTargets(numRenderTargets, pRTVs,
		pDepthStencilView ? &pDepthStencilView->view : nullptr);
}

void EZ::CommandList_DX12::ClearDepthStencilView(ResourceView& depthStencilView, ClearFlag clearFlags,
	float depth, uint8_t stencil, uint32_t numRects, const RectRange* pRects)
{
	setBarriers(1, &depthStencilView, ResourceState::DEPTH_WRITE);
	m_clearDSVs.emplace_back(ClearDSV { depthStencilView.view, clearFlags, depth, stencil, numRects, pRects });
}

void EZ::CommandList_DX12::ClearRenderTargetView(ResourceView& renderTargetView,
	const float colorRGBA[4], uint32_t numRects, const RectRange* pRects)
{
	setBarriers(1, &renderTargetView, ResourceState::RENDER_TARGET);
	m_clearRTVs.emplace_back(ClearRTV{ renderTargetView.view, colorRGBA, numRects, pRects });
}

void EZ::CommandList_DX12::ClearUnorderedAccessViewUint(ResourceView& unorderedAccessView,
	const uint32_t values[4], uint32_t numRects, const RectRange* pRects)
{
	setBarriers(1, &unorderedAccessView, ResourceState::UNORDERED_ACCESS);

	const auto uavTable = Util::DescriptorTable::MakeUnique(API::DIRECTX_12);
	uavTable->SetDescriptors(0, 1, &unorderedAccessView.view);
	const auto descriptorTable = uavTable->GetCbvSrvUavTable(m_descriptorTableCache.get());

	XUSG::CommandList_DX12::ClearUnorderedAccessViewUint(descriptorTable, unorderedAccessView.view,
		unorderedAccessView.pResource, values, numRects, pRects);
}

void EZ::CommandList_DX12::ClearUnorderedAccessViewFloat(ResourceView& unorderedAccessView,
	const float values[4], uint32_t numRects, const RectRange* pRects)
{
	setBarriers(1, &unorderedAccessView, ResourceState::UNORDERED_ACCESS);

	const auto uavTable = Util::DescriptorTable::MakeUnique(API::DIRECTX_12);
	uavTable->SetDescriptors(0, 1, &unorderedAccessView.view);
	const auto descriptorTable = uavTable->GetCbvSrvUavTable(m_descriptorTableCache.get());

	XUSG::CommandList_DX12::ClearUnorderedAccessViewFloat(descriptorTable, unorderedAccessView.view,
		unorderedAccessView.pResource, values, numRects, pRects);
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
	if (!m_graphicsState) m_graphicsState = Graphics::State::MakeUnique(API::DIRECTX_12);

	return m_graphicsState.get();
}

Compute::State* EZ::CommandList_DX12::GetComputePipelineState()
{
	if (!m_computeState) m_computeState = Compute::State::MakeUnique(API::DIRECTX_12);

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
		const auto pipelineLayout = Util::PipelineLayout::MakeUnique(API::DIRECTX_12);
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

	// Set barrier command
	XUSG::CommandList_DX12::Barrier(static_cast<uint32_t>(m_barriers.size()), m_barriers.data());
	m_barriers.clear();

	// Clear DSVs
	for (const auto& args : m_clearDSVs)
		XUSG::CommandList_DX12::ClearDepthStencilView(args.DepthStencilView,
			args.ClearFlags, args.Depth, args.Stencil, args.NumRects, args.pRects);
	m_clearDSVs.clear();

	// Clear RTVs
	for (const auto& args : m_clearRTVs)
		XUSG::CommandList_DX12::ClearRenderTargetView(args.RenderTargetView,
			args.ColorRGBA, args.NumRects, args.pRects);
	m_clearRTVs.clear();

	// Create pipeline for dynamic states
	if (m_graphicsState)
	{
		m_graphicsState->SetPipelineLayout(m_pipelineLayouts[GRAPHICS]);
		const auto pipeline = m_graphicsState->GetPipeline(m_graphicsPipelineCache.get());
		if (pipeline && m_pipeline != pipeline) XUSG::CommandList_DX12::SetPipelineState(pipeline);
	}
}

void EZ::CommandList_DX12::setBarriers(uint32_t numResources, const ResourceView* pResourceViews, ResourceState dstState)
{
	// Estimate barrier count
	auto numBarriersEst = 0u;
	for (auto i = 0u; i < numResources; ++i)
		numBarriersEst += static_cast<uint32_t>(pResourceViews[i].Subresources.size());

	// Generate barriers for each resource
	const auto startIdx = m_barriers.size();
	m_barriers.resize(startIdx + numBarriersEst);
	auto numBarriers = 0u;
	for (auto i = 0u; i < numResources; ++i)
		numBarriers = generateBarriers(&m_barriers[startIdx], pResourceViews[i], dstState, numBarriers);

	// Shrink the size of barrier list
	if (numBarriers < numBarriersEst) m_barriers.resize(startIdx + numBarriers);
}

uint32_t EZ::CommandList_DX12::generateBarriers(ResourceBarrier* pBarriers, const ResourceView& resrouceView,
	ResourceState dstState, uint32_t numBarriers, BarrierFlag flags)
{
	for (const auto& subresource : resrouceView.Subresources)
		numBarriers = resrouceView.pResource->SetBarrier(pBarriers, dstState, numBarriers, subresource, flags);

	return numBarriers;
}
