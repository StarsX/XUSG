//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSGCommand_DX12.h"
#include "XUSG-EZ_DX12.h"

#include "CSBlit2D.h"
#include "CSBlit3D.h"

using namespace std;
using namespace XUSG;

EZ::CommandList_DX12::CommandList_DX12() :
	XUSG::CommandList_DX12(),
	m_graphicsPipelineLib(nullptr),
	m_computePipelineLib(nullptr),
	m_pipelineLayoutLib(nullptr),
	m_descriptorTableLib(nullptr),
	m_pInputLayout(nullptr),
	m_pipelineLayouts(),
	m_pipeline(nullptr),
	m_graphicsState(nullptr),
	m_computeState(nullptr),
	m_isGraphicsDirty(false),
	m_isComputeDirty(false),
	m_descriptors(0),
	m_cbvSrvUavTables(),
	m_samplerTables(),
	m_barriers(0),
	m_clearDSVs(0),
	m_clearRTVs(0),
	m_graphicsSpaceToParamIndexMap(),
	m_computeSpaceToParamIndexMap(),
	m_graphicsConstantParamIndices(),
	m_computeConstantParamIndex(0),
	m_shaders()
{
	m_shaderLib = ShaderLib::MakeUnique();
}

EZ::CommandList_DX12::CommandList_DX12(XUSG::CommandList* pCommandList, uint32_t samplerHeapSize, uint32_t cbvSrvUavHeapSize,
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

bool EZ::CommandList_DX12::Create(XUSG::CommandList* pCommandList,
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
	XUSG::CommandList_DX12::Create(pHandle, name);

	return Create(this, samplerHeapSize, cbvSrvUavHeapSize, maxSamplers,
		pMaxCbvsEachSpace, pMaxSrvsEachSpace, pMaxUavsEachSpace,
		maxCbvSpaces, maxSrvSpaces, maxUavSpaces, max32BitConstants,
		constantSlots, constantSpaces, slotExt, spaceExt);
}

bool EZ::CommandList_DX12::Close(RenderTarget* pBackBuffer)
{
	if (pBackBuffer)
	{
		ResourceBarrier barrier;
		const auto numBarriers = pBackBuffer->SetBarrier(&barrier, ResourceState::PRESENT);
		XUSG::CommandList_DX12::Barrier(numBarriers, &barrier);
	}

	return XUSG::CommandList_DX12::Close();
}

bool EZ::CommandList_DX12::Reset(const CommandAllocator* pAllocator, const Pipeline& initialState)
{
	XUSG_N_RETURN(XUSG::CommandList_DX12::Reset(pAllocator, initialState), false);

	// Set Descriptor heaps
	const DescriptorHeap descriptorHeaps[] =
	{
		m_descriptorTableLib->GetDescriptorHeap(CBV_SRV_UAV_HEAP),
		m_descriptorTableLib->GetDescriptorHeap(SAMPLER_HEAP)
	};
	XUSG::CommandList_DX12::SetDescriptorHeaps(static_cast<uint32_t>(size(descriptorHeaps)), descriptorHeaps);

	// Set pipeline layouts
	XUSG::CommandList_DX12::SetGraphicsPipelineLayout(m_pipelineLayouts[GRAPHICS]);
	XUSG::CommandList_DX12::SetComputePipelineLayout(m_pipelineLayouts[COMPUTE]);
	m_pipeline = initialState;
	m_pInputLayout = nullptr;

	m_barriers.clear();
	m_clearDSVs.clear();

	return true;
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

void EZ::CommandList_DX12::DrawIndirect(const CommandLayout* pCommandlayout, uint32_t maxCommandCount,
	Resource* pArgumentBuffer, uint64_t argumentBufferOffset, Resource* pCountBuffer, uint64_t countBufferOffset)
{
	preexecuteIndirect(pArgumentBuffer, pCountBuffer);
	predraw();

	XUSG::CommandList_DX12::ExecuteIndirect(pCommandlayout, maxCommandCount,
		pArgumentBuffer, argumentBufferOffset, pCountBuffer, countBufferOffset);
}

void EZ::CommandList_DX12::Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
{
	predispatch();
	XUSG::CommandList_DX12::Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}

void EZ::CommandList_DX12::DispatchIndirect(const CommandLayout* pCommandlayout, uint32_t maxCommandCount,
	Resource* pArgumentBuffer, uint64_t argumentBufferOffset, Resource* pCountBuffer, uint64_t countBufferOffset)
{
	preexecuteIndirect(pArgumentBuffer, pCountBuffer);
	predispatch();

	XUSG::CommandList_DX12::ExecuteIndirect(pCommandlayout, maxCommandCount,
		pArgumentBuffer, argumentBufferOffset, pCountBuffer, countBufferOffset);
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
	assert(pDstResource);
	assert(pSrcResource);

	// Generate barriers for each resource
	ResourceBarrier barriers[2];
	auto numBarriers = pDstResource->SetBarrier(barriers, ResourceState::RESOLVE_DEST, 0, dstSubresource);
	numBarriers = pSrcResource->SetBarrier(barriers, ResourceState::RESOLVE_SOURCE, numBarriers, srcSubresource);
	XUSG::CommandList_DX12::Barrier(numBarriers, barriers);

	XUSG::CommandList_DX12::ResolveSubresource(pDstResource, dstSubresource, pSrcResource, srcSubresource, format);
}

void EZ::CommandList_DX12::IASetInputLayout(const InputLayout* pLayout)
{
	assert(m_graphicsState);
	m_graphicsState->IASetInputLayout(pLayout);
	m_isGraphicsDirty = true;
}

void EZ::CommandList_DX12::IASetIndexBufferStripCutValue(IBStripCutValue ibStripCutValue)
{
	assert(m_graphicsState);
	m_graphicsState->IASetIndexBufferStripCutValue(ibStripCutValue);
	m_isGraphicsDirty = true;
}

void EZ::CommandList_DX12::RSSetState(const Graphics::Rasterizer* pRasterizer)
{
	assert(m_graphicsState);
	m_graphicsState->RSSetState(pRasterizer);
	m_isGraphicsDirty = true;
}

void EZ::CommandList_DX12::RSSetState(Graphics::RasterizerPreset preset)
{
	assert(m_graphicsState);
	m_graphicsState->RSSetState(preset, m_graphicsPipelineLib.get());
	m_isGraphicsDirty = true;
}

void EZ::CommandList_DX12::OMSetBlendState(const Graphics::Blend* pBlend, uint32_t sampleMask)
{
	assert(m_graphicsState);
	m_graphicsState->OMSetBlendState(pBlend, sampleMask);
	m_isGraphicsDirty = true;
}

void EZ::CommandList_DX12::OMSetBlendState(Graphics::BlendPreset preset, uint8_t numColorRTs, uint32_t sampleMask)
{
	assert(m_graphicsState);
	m_graphicsState->OMSetBlendState(preset, m_graphicsPipelineLib.get(), numColorRTs, sampleMask);
	m_isGraphicsDirty = true;
}

void EZ::CommandList_DX12::OMSetSample(uint8_t count, uint8_t quality)
{
	assert(m_graphicsState);
	m_graphicsState->OMSetSample(count, quality);
	m_isGraphicsDirty = true;
}

void EZ::CommandList_DX12::DSSetState(const Graphics::DepthStencil* pDepthStencil)
{
	assert(m_graphicsState);
	m_graphicsState->DSSetState(pDepthStencil);
	m_isGraphicsDirty = true;
}

void EZ::CommandList_DX12::DSSetState(Graphics::DepthStencilPreset preset)
{
	assert(m_graphicsState);
	m_graphicsState->DSSetState(preset, m_graphicsPipelineLib.get());
	m_isGraphicsDirty = true;
}

void EZ::CommandList_DX12::SetGraphicsShader(Shader::Stage stage, const Blob& shader)
{
	assert(stage < Shader::Stage::NUM_GRAPHICS);
	assert(m_graphicsState);
	m_graphicsState->SetShader(stage, shader);
	m_isGraphicsDirty = true;
}

void EZ::CommandList_DX12::SetGraphicsNodeMask(uint32_t nodeMask)
{
	assert(m_graphicsState);
	m_graphicsState->SetNodeMask(nodeMask);
	m_isGraphicsDirty = true;
}

void EZ::CommandList_DX12::SetComputeShader(const Blob& shader)
{
	assert(m_computeState);
	m_computeState->SetShader(shader);
	m_isComputeDirty = true;
}

void EZ::CommandList_DX12::SetComputeNodeMask(uint32_t nodeMask)
{
	assert(m_computeState);
	m_computeState->SetNodeMask(nodeMask);
	m_isComputeDirty = true;
}

void EZ::CommandList_DX12::SetGraphicsPipelineState(const Pipeline& pipelineState, const Graphics::State* pState)
{
	assert(pipelineState || pState);
	Pipeline pipeline;

	if (pState)
	{
		pipeline = pState->GetPipeline(m_graphicsPipelineLib.get());
		if (pipelineState && pipeline != pipelineState)
		{
			pipeline = pipelineState;
			m_graphicsPipelineLib->SetPipeline(pState, pipeline);
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

void EZ::CommandList_DX12::SetComputePipelineState(const Pipeline& pipelineState, const Compute::State* pState)
{
	assert(pipelineState || pState);
	Pipeline pipeline;

	if (pState)
	{
		pipeline = pState->GetPipeline(m_computePipelineLib.get());
		if (pipelineState && pipeline != pipelineState)
		{
			pipeline = pipelineState;
			m_computePipelineLib->SetPipeline(pState, pipeline);
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
		m_isComputeDirty = false;
	}
}

void EZ::CommandList_DX12::SetSamplers(Shader::Stage stage, uint32_t startBinding, uint32_t numSamplers, const Sampler* const* ppSamplers)
{
	auto& descriptorTable = m_samplerTables[stage];
	if (!descriptorTable) descriptorTable = Util::DescriptorTable::MakeUnique(API::DIRECTX_12);
	descriptorTable->SetSamplers(startBinding, numSamplers, ppSamplers);
}

void EZ::CommandList_DX12::SetSamplerStates(Shader::Stage stage, uint32_t startBinding, uint32_t numSamplers, const SamplerPreset* pSamplerPresets)
{
	auto& descriptorTable = m_samplerTables[stage];
	if (!descriptorTable) descriptorTable = Util::DescriptorTable::MakeUnique(API::DIRECTX_12);
	descriptorTable->SetSamplers(startBinding, numSamplers, pSamplerPresets, m_descriptorTableLib.get());
}

void EZ::CommandList_DX12::SetResources(Shader::Stage stage, DescriptorType descriptorType, uint32_t startBinding,
	uint32_t numResources, const ResourceView* pResourceViews, uint32_t space)
{
	// Handle the remaining clear-views commands
	if (descriptorType == DescriptorType::SRV)
	{
		auto needClearDSVs = false;
		for (const auto& args : m_clearDSVs)
		{
			for (auto i = 0u; i < numResources; ++i)
			{
				if (pResourceViews[i].pResource == args.pResource)
				{
					needClearDSVs = true;
					break;
				}
			}
		}
		
		auto needClearRTVs = false;
		for (const auto& args : m_clearRTVs)
		{
			for (auto i = 0u; i < numResources; ++i)
			{
				if (pResourceViews[i].pResource == args.pResource)
				{
					needClearRTVs = true;
					break;
				}
			}
		}

		if (needClearDSVs || needClearRTVs)
		{
			// Set barrier command
			XUSG::CommandList_DX12::Barrier(static_cast<uint32_t>(m_barriers.size()), m_barriers.data());
			m_barriers.clear();

			if (needClearDSVs) clearDSVs();
			if (needClearRTVs) clearRTVs();
		}
	}

	// Map descriptor table
	auto& descriptorTables = m_cbvSrvUavTables[stage][static_cast<uint32_t>(descriptorType)];
	assert(space < descriptorTables.size());
	auto& descriptorTable = descriptorTables[space];

	// Set descriptors to the descriptor table
	if (!descriptorTable) descriptorTable = Util::DescriptorTable::MakeUnique(API::DIRECTX_12);
	if (m_descriptors.size() < numResources) m_descriptors.resize(numResources);
	for (auto i = 0u; i < numResources; ++i) m_descriptors[i] = pResourceViews[i].View;
	descriptorTable->SetDescriptors(startBinding, numResources, m_descriptors.data());

	// Set barriers
	if (descriptorType == DescriptorType::SRV || descriptorType == DescriptorType::UAV)
		setBarriers(numResources, pResourceViews);
}

void EZ::CommandList_DX12::SetGraphicsDescriptorTable(Shader::Stage stage, DescriptorType descriptorType,
	const DescriptorTable& descriptorTable, uint32_t space)
{
	XUSG::CommandList_DX12::SetGraphicsDescriptorTable(m_graphicsSpaceToParamIndexMap[stage][static_cast<uint32_t>(descriptorType)][space], descriptorTable);
}

void EZ::CommandList_DX12::SetComputeDescriptorTable(DescriptorType descriptorType, const DescriptorTable& descriptorTable, uint32_t space)
{
	XUSG::CommandList_DX12::SetComputeDescriptorTable(m_computeSpaceToParamIndexMap[static_cast<uint32_t>(descriptorType)][space], descriptorTable);
}

void EZ::CommandList_DX12::SetGraphics32BitConstant(Shader::Stage stage, uint32_t srcData, uint32_t destOffsetIn32BitValues) const
{
	assert(stage < Shader::Stage::NUM_GRAPHICS);
	XUSG::CommandList_DX12::SetGraphics32BitConstant(m_graphicsConstantParamIndices[stage], srcData, destOffsetIn32BitValues);
}

void EZ::CommandList_DX12::SetCompute32BitConstant(uint32_t srcData, uint32_t destOffsetIn32BitValues) const
{
	XUSG::CommandList_DX12::SetCompute32BitConstant(m_computeConstantParamIndex, srcData, destOffsetIn32BitValues);
}

void EZ::CommandList_DX12::SetGraphics32BitConstants(Shader::Stage stage, uint32_t num32BitValuesToSet,
	const void* pSrcData, uint32_t destOffsetIn32BitValues) const
{
	assert(stage < Shader::Stage::NUM_GRAPHICS);
	XUSG::CommandList_DX12::SetGraphics32BitConstants(m_graphicsConstantParamIndices[stage],
		num32BitValuesToSet, pSrcData, destOffsetIn32BitValues);
}

void EZ::CommandList_DX12::SetCompute32BitConstants(uint32_t num32BitValuesToSet, const void* pSrcData, uint32_t destOffsetIn32BitValues) const
{
	XUSG::CommandList_DX12::SetCompute32BitConstants(m_computeConstantParamIndex, num32BitValuesToSet, pSrcData, destOffsetIn32BitValues);
}

void EZ::CommandList_DX12::IASetPrimitiveTopology(PrimitiveTopology primitiveTopology)
{
	assert(m_graphicsState);

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

	m_isGraphicsDirty = true;

	XUSG::CommandList_DX12::IASetPrimitiveTopology(primitiveTopology);
}

void EZ::CommandList_DX12::IASetIndexBuffer(const IndexBufferView& view)
{
	// Set barriers if the index buffers is not at the read states
	const auto startIdx = m_barriers.size();
	m_barriers.resize(startIdx + 1);
	const auto numBarriers = view.pResource->SetBarrier(&m_barriers[startIdx], view.DstState);

	// Shrink the size of barrier list
	if (!numBarriers) m_barriers.resize(startIdx);

	XUSG::CommandList_DX12::IASetIndexBuffer(*view.pView);
}

void EZ::CommandList_DX12::IASetVertexBuffers(uint32_t startSlot, uint32_t numViews, const VertexBufferView* pViews)
{
	vector<XUSG::VertexBufferView> views(numViews);

	// Set barriers if the index buffers is not at the read states
	const auto startIdx = m_barriers.size();
	m_barriers.resize(startIdx + numViews);
	auto numBarriers = 0u;
	for (auto i = 0u; i < numViews; ++i)
	{
		auto& view = pViews[i];
		const auto numBarriers = view.pResource->SetBarrier(&m_barriers[startIdx], view.DstState);
		views[i] = *view.pView;
	}

	// Shrink the size of barrier list
	if (numBarriers < numViews) m_barriers.resize(startIdx + numBarriers);

	XUSG::CommandList_DX12::IASetVertexBuffers(startSlot, numViews, views.data());
}

void EZ::CommandList_DX12::SOSetTargets(uint32_t startSlot, uint32_t numViews, const StreamOutBufferView* pViews, Resource* const* ppResources)
{
	vector<ResourceView> resourceViews(numViews);
	for (auto i = 0u; i < numViews; ++i)
	{
		auto& resourceView = resourceViews[i];
		resourceView.pResource = ppResources[i];
		resourceView.Subresources = { XUSG_BARRIER_ALL_SUBRESOURCES };
		resourceView.DstState = ResourceState::STREAM_OUT;
	}

	setBarriers(numViews, resourceViews.data());

	XUSG::CommandList_DX12::SOSetTargets(startSlot, numViews, pViews);
}

void EZ::CommandList_DX12::OMSetRenderTargets(uint32_t numRenderTargets,
	const ResourceView* pRenderTargetViews, const ResourceView* pDepthStencilView)
{
	// Set RTV barriers
	setBarriers(numRenderTargets, pRenderTargetViews);

	// Set DSV barriers
	if (pDepthStencilView)
	{
		setBarriers(1, pDepthStencilView);
		m_graphicsState->OMSetDSVFormat(dynamic_cast<Texture*>(pDepthStencilView->pResource)->GetFormat());
	}
	else m_graphicsState->OMSetDSVFormat(Format::UNKNOWN);

	m_graphicsState->OMSetNumRenderTargets(numRenderTargets);

	Descriptor pRTVs[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
	for (auto i = 0u; i < numRenderTargets; ++i)
	{
		pRTVs[i] = pRenderTargetViews[i].View;
		m_graphicsState->OMSetRTVFormat(i, dynamic_cast<Texture*>(pRenderTargetViews[i].pResource)->GetFormat());
	}

	for (auto i = numRenderTargets; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		m_graphicsState->OMSetRTVFormat(i, Format::UNKNOWN);

	m_isGraphicsDirty = true;

	XUSG::CommandList_DX12::OMSetRenderTargets(numRenderTargets, pRTVs,
		pDepthStencilView ? &pDepthStencilView->View : nullptr);
}

void EZ::CommandList_DX12::ClearDepthStencilView(const ResourceView& depthStencilView, ClearFlag clearFlags,
	float depth, uint8_t stencil, uint32_t numRects, const RectRange* pRects)
{
	setBarriers(1, &depthStencilView);
	m_clearDSVs.emplace_back(ClearDSV{ depthStencilView.pResource, depthStencilView.View, clearFlags, depth, stencil });

	auto& clearView = m_clearDSVs.back();
	clearView.Rects.resize(numRects);
	for (auto i = 0u; i < numRects; ++i) clearView.Rects[i] = pRects[i];
}

void EZ::CommandList_DX12::ClearRenderTargetView(const ResourceView& renderTargetView,
	const float colorRGBA[4], uint32_t numRects, const RectRange* pRects)
{
	setBarriers(1, &renderTargetView);
	m_clearRTVs.emplace_back(ClearRTV{ renderTargetView.pResource, renderTargetView.View,
		colorRGBA[0], colorRGBA[1], colorRGBA[2], colorRGBA[3] });

	auto& clearView = m_clearRTVs.back();
	clearView.Rects.resize(numRects);
	for (auto i = 0u; i < numRects; ++i) clearView.Rects[i] = pRects[i];
}

void EZ::CommandList_DX12::ClearUnorderedAccessViewUint(const ResourceView& unorderedAccessView,
	const uint32_t values[4], uint32_t numRects, const RectRange* pRects)
{
	setBarriers(1, &unorderedAccessView);

	const auto uavTable = Util::DescriptorTable::MakeUnique(API::DIRECTX_12);
	uavTable->SetDescriptors(0, 1, &unorderedAccessView.View);
	const auto descriptorTable = uavTable->GetCbvSrvUavTable(m_descriptorTableLib.get());

	m_clearUAVsUint.emplace_back(ClearUAVUint{ unorderedAccessView.pResource, unorderedAccessView.View,
		descriptorTable, values[0], values[1], values[2], values[3] });

	auto& clearView = m_clearUAVsUint.back();
	clearView.Rects.resize(numRects);
	for (auto i = 0u; i < numRects; ++i) clearView.Rects[i] = pRects[i];
}

void EZ::CommandList_DX12::ClearUnorderedAccessViewFloat(const ResourceView& unorderedAccessView,
	const float values[4], uint32_t numRects, const RectRange* pRects)
{
	setBarriers(1, &unorderedAccessView);

	const auto uavTable = Util::DescriptorTable::MakeUnique(API::DIRECTX_12);
	uavTable->SetDescriptors(0, 1, &unorderedAccessView.View);
	const auto descriptorTable = uavTable->GetCbvSrvUavTable(m_descriptorTableLib.get());

	m_clearUAVsFloat.emplace_back(ClearUAVFloat{ unorderedAccessView.pResource, unorderedAccessView.View,
		descriptorTable, values[0], values[1], values[2], values[3] });

	auto& clearView = m_clearUAVsFloat.back();
	clearView.Rects.resize(numRects);
	for (auto i = 0u; i < numRects; ++i) clearView.Rects[i] = pRects[i];
}

void EZ::CommandList_DX12::Resize()
{
	m_descriptorTableLib->ResetDescriptorHeap(CBV_SRV_UAV_HEAP);
}

void EZ::CommandList_DX12::Blit(Texture* pDstResource, Texture* pSrcResource, SamplerPreset sampler,
	const Blob& customShader, uint32_t dstMip, uint32_t srcMip)
{
	// Set pipeline state
	SetComputeShader(customShader ? customShader : m_shaders[CS_BLIT_2D]);

	// Set sampler
	SetSamplerStates(Shader::Stage::CS, 0, 1, &sampler);

	// Set UAV
	assert(pDstResource);
	assert(dstMip < pDstResource->GetNumMips());
	const auto uav = EZ::GetUAV(pDstResource, dstMip);
	SetResources(Shader::Stage::CS, DescriptorType::UAV, 0, 1, &uav);

	// Set SRV
	assert(pSrcResource);
	assert(srcMip < pDstResource->GetNumMips());
	const auto srv = pSrcResource->GetNumMips() > 1 ? EZ::GetSRV(pSrcResource, srcMip, true) : EZ::GetSRV(pSrcResource);
	SetResources(Shader::Stage::CS, DescriptorType::SRV, 0, 1, &srv);

	// Dispatch grid
	const uint32_t width = static_cast<uint32_t>(pDstResource->GetWidth());
	const uint32_t height = pDstResource->GetHeight();
	const auto threadsX = (max)(width >> dstMip, 1u);
	const auto threadsY = (max)(height >> dstMip, 1u);
	Dispatch(XUSG_DIV_UP(threadsX, 8), XUSG_DIV_UP(threadsY, 8), 1);
}

void EZ::CommandList_DX12::Blit(Texture3D* pDstResource, Texture* pSrcResource, SamplerPreset sampler,
	const Blob& customShader, uint32_t dstMip, uint32_t srcMip)
{
	// Set pipeline state
	SetComputeShader(customShader ? customShader : m_shaders[CS_BLIT_3D]);

	// Set sampler
	SetSamplerStates(Shader::Stage::CS, 0, 1, &sampler);

	// Set UAV
	assert(pDstResource);
	assert(dstMip < pDstResource->GetNumMips());
	const auto uav = EZ::GetUAV(pDstResource, dstMip);
	SetResources(Shader::Stage::CS, DescriptorType::UAV, 0, 1, &uav);

	// Set SRV
	assert(pSrcResource);
	assert(srcMip < pDstResource->GetNumMips());
	const auto srv = pSrcResource->GetNumMips() > 1 ? EZ::GetSRV(pSrcResource, srcMip, true) : EZ::GetSRV(pSrcResource);
	SetResources(Shader::Stage::CS, DescriptorType::SRV, 0, 1, &srv);

	// Dispatch grid
	const uint32_t width = static_cast<uint32_t>(pDstResource->GetWidth());
	const uint32_t height = pDstResource->GetHeight();
	const uint32_t depth = pDstResource->GetDepth();
	const auto threadsX = (max)(width >> dstMip, 1u);
	const auto threadsY = (max)(height >> dstMip, 1u);
	const auto threadsZ = (max)(depth >> dstMip, 1u);
	Dispatch(XUSG_DIV_UP(threadsX, 4), XUSG_DIV_UP(threadsY, 4), XUSG_DIV_UP(threadsZ, 4));
}

void EZ::CommandList_DX12::GenerateMips(Texture* pResource, SamplerPreset sampler, const Blob& customShader)
{
	// Set pipeline state
	SetComputeShader(customShader ? customShader : m_shaders[CS_BLIT_2D]);

	// Set sampler
	SetSamplerStates(Shader::Stage::CS, 0, 1, &sampler);

	assert(pResource);
	const uint32_t width = static_cast<uint32_t>(pResource->GetWidth());
	const uint32_t height = pResource->GetHeight();
	const uint8_t numMips = pResource->GetNumMips();
	for (uint8_t i = 1; i < numMips; ++i)
	{
		// Set UAV
		const auto uav = EZ::GetUAV(pResource, i);
		SetResources(Shader::Stage::CS, DescriptorType::UAV, 0, 1, &uav);

		// Set SRV
		const auto srv = EZ::GetSRV(pResource, i - 1, true);
		SetResources(Shader::Stage::CS, DescriptorType::SRV, 0, 1, &srv);

		// Dispatch grid
		const auto threadsX = (max)(width >> i, 1u);
		const auto threadsY = (max)(height >> i, 1u);
		Dispatch(XUSG_DIV_UP(threadsX, 8), XUSG_DIV_UP(threadsY, 8), 1);
	}
}

void EZ::CommandList_DX12::GenerateMips(Texture3D* pResource, SamplerPreset sampler, const Blob& customShader)
{
	// Set pipeline state
	SetComputeShader(customShader ? customShader : m_shaders[CS_BLIT_3D]);

	// Set sampler
	SetSamplerStates(Shader::Stage::CS, 0, 1, &sampler);

	assert(pResource);
	const uint32_t width = static_cast<uint32_t>(pResource->GetWidth());
	const uint32_t height = pResource->GetHeight();
	const uint32_t depth = pResource->GetDepth();
	const uint8_t numMips = pResource->GetNumMips();
	for (uint8_t i = 1; i < numMips; ++i)
	{
		// Set UAV
		const auto uav = EZ::GetUAV(pResource, i);
		SetResources(Shader::Stage::CS, DescriptorType::UAV, 0, 1, &uav);

		// Set SRV
		const auto srv = EZ::GetSRV(pResource, i - 1, true);
		SetResources(Shader::Stage::CS, DescriptorType::SRV, 0, 1, &srv);

		// Dispatch grid
		const auto threadsX = (max)(width >> i, 1u);
		const auto threadsY = (max)(height >> i, 1u);
		const auto threadsZ = (max)(depth >> i, 1u);
		Dispatch(XUSG_DIV_UP(threadsX, 4), XUSG_DIV_UP(threadsY, 4), XUSG_DIV_UP(threadsZ, 4));
	}
}

const Graphics::Blend* EZ::CommandList_DX12::GetBlend(Graphics::BlendPreset preset, uint8_t numColorRTs)
{
	return m_graphicsPipelineLib->GetBlend(preset, numColorRTs);
}

const Graphics::Rasterizer* EZ::CommandList_DX12::GetRasterizer(Graphics::RasterizerPreset preset)
{
	return m_graphicsPipelineLib->GetRasterizer(preset);
}

const Graphics::DepthStencil* EZ::CommandList_DX12::GetDepthStencil(Graphics::DepthStencilPreset preset)
{
	return m_graphicsPipelineLib->GetDepthStencil(preset);
}

DescriptorTableLib* EZ::CommandList_DX12::GetDescriptorTableLib() const
{
	return m_descriptorTableLib.get();
}

const PipelineLayout& EZ::CommandList_DX12::GetGraphicsPipelineLayout() const
{
	return m_pipelineLayouts[GRAPHICS];
}

const XUSG::PipelineLayout& EZ::CommandList_DX12::GetComputePipelineLayout() const
{
	return m_pipelineLayouts[COMPUTE];
}

uint32_t EZ::CommandList_DX12::GetGraphicsConstantParamIndex(Shader::Stage stage) const
{
	return m_graphicsConstantParamIndices[stage];
}

uint32_t EZ::CommandList_DX12::GetComputeConstantParamIndex() const
{
	return m_computeConstantParamIndex;
}

bool EZ::CommandList_DX12::init(XUSG::CommandList* pCommandList, uint32_t samplerHeapSize, uint32_t cbvSrvUavHeapSize)
{
	m_pDevice = pCommandList->GetDevice();
	m_commandList = dynamic_cast<XUSG::CommandList_DX12*>(pCommandList)->GetGraphicsCommandList();

	m_graphicsPipelineLib = Graphics::PipelineLib::MakeUnique(m_pDevice, API::DIRECTX_12);
	m_computePipelineLib = Compute::PipelineLib::MakeUnique(m_pDevice, API::DIRECTX_12);
	m_pipelineLayoutLib = PipelineLayoutLib::MakeUnique(m_pDevice, API::DIRECTX_12);
	m_descriptorTableLib = DescriptorTableLib::MakeUnique(m_pDevice, L"EZDescriptorTableLib", API::DIRECTX_12);
	m_graphicsState = Graphics::State::MakeUnique(API::DIRECTX_12);
	m_computeState = Compute::State::MakeUnique(API::DIRECTX_12);

	// Create internal shaders
	XUSG_N_RETURN(createShaders(), false);

	// Allocate descriptor heaps
	XUSG_N_RETURN(m_descriptorTableLib->AllocateDescriptorHeap(SAMPLER_HEAP, samplerHeapSize), false);
	XUSG_N_RETURN(m_descriptorTableLib->AllocateDescriptorHeap(CBV_SRV_UAV_HEAP, cbvSrvUavHeapSize), false);

	return true;
}

bool EZ::CommandList_DX12::createGraphicsPipelineLayouts(
	const uint32_t maxSamplers[Shader::Stage::NUM_GRAPHICS],
	const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_GRAPHICS],
	const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_GRAPHICS],
	const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_GRAPHICS],
	const uint32_t maxCbvSpaces[Shader::Stage::NUM_GRAPHICS],
	const uint32_t maxSrvSpaces[Shader::Stage::NUM_GRAPHICS],
	const uint32_t maxUavSpaces[Shader::Stage::NUM_GRAPHICS],
	const uint32_t max32BitConstants[Shader::Stage::NUM_GRAPHICS],
	const uint32_t constantSlots[Shader::Stage::NUM_GRAPHICS],
	const uint32_t constantSpaces[Shader::Stage::NUM_GRAPHICS],
	uint32_t slotExt, uint32_t spaceExt)
{
	// Create common graphics pipeline layout
	auto paramIndex = 0u;
	const auto pipelineLayout = Util::PipelineLayout::MakeUnique(API::DIRECTX_12);

	// Handle all samplers to take up the first N root params with fixed param indices 
	for (uint8_t i = 0; i < Shader::Stage::NUM_GRAPHICS; ++i)
	{
		const Shader::Stage stage = static_cast<Shader::Stage>(i);
		const auto stageMaxSamplers = maxSamplers ? maxSamplers[stage] : (i == Shader::Stage::PS ? 16 : 0);

		if (stageMaxSamplers > 0)
		{
			pipelineLayout->SetRange(paramIndex, DescriptorType::SAMPLER, stageMaxSamplers, 0, 0, DescriptorFlag::DESCRIPTORS_VOLATILE);
			pipelineLayout->SetShaderStage(paramIndex++, stage);
		}
	}

	// Then, handle the CBVs, SRVs, and UAVs
	for (uint8_t i = 0; i < Shader::Stage::NUM_GRAPHICS; ++i)
	{
		const Shader::Stage stage = static_cast<Shader::Stage>(i);
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

		auto& spaceToParamIndexMap = m_graphicsSpaceToParamIndexMap[i];
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
			m_graphicsConstantParamIndices[stage] = paramIndex;
			pipelineLayout->SetConstants(paramIndex++, num32BitValues, constantSlot, constantSpace, stage);
		}
	}

	pipelineLayout->SetRange(paramIndex++, DescriptorType::UAV, 1, slotExt, spaceExt);

	XUSG_X_RETURN(m_pipelineLayouts[GRAPHICS], pipelineLayout->GetPipelineLayout(m_pipelineLayoutLib.get(),
		PipelineLayoutFlag::ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, L"EZGraphicsLayout"), false);

	return true;
}

bool EZ::CommandList_DX12::createComputePipelineLayouts(uint32_t maxSamplers,
	const uint32_t* pMaxCbvsEachSpace, const uint32_t* pMaxSrvsEachSpace, const uint32_t* pMaxUavsEachSpace,
	uint32_t maxCbvSpaces, uint32_t maxSrvSpaces, uint32_t maxUavSpaces,
	uint32_t max32BitConstants, uint32_t constantSlot, uint32_t constantSpace,
	uint32_t slotExt, uint32_t spaceExt)
{
	// Create common compute pipeline layout
	auto paramIndex = 0u;
	const auto pipelineLayout = Util::PipelineLayout::MakeUnique(API::DIRECTX_12);
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

	pipelineLayout->SetRange(paramIndex++, DescriptorType::SAMPLER, maxSamplers, 0, 0, DescriptorFlag::DESCRIPTORS_VOLATILE);

	for (auto s = 0u; s < maxSpaces; ++s)
	{
		if (s < maxCbvSpaces)
		{
			auto maxDescriptors = pMaxCbvsEachSpace ? pMaxCbvsEachSpace[s] : 14;
			auto slot = 0;
			if (s == constantSpace && max32BitConstants > 0)
			{
				maxDescriptors = constantSlot ? (min)(maxDescriptors, constantSlot) : maxDescriptors - 1;
				slot = constantSlot ? 0 : 1;
			}
			m_computeSpaceToParamIndexMap[static_cast<uint32_t>(DescriptorType::CBV)][s] = paramIndex;
			pipelineLayout->SetRange(paramIndex++, DescriptorType::CBV, maxDescriptors, slot, s, DescriptorFlag::DESCRIPTORS_VOLATILE);
		}

		if (s < maxSrvSpaces)
		{
			const auto maxDescriptors = pMaxSrvsEachSpace ? pMaxSrvsEachSpace[s] : 32;
			m_computeSpaceToParamIndexMap[static_cast<uint32_t>(DescriptorType::SRV)][s] = paramIndex;
			pipelineLayout->SetRange(paramIndex++, DescriptorType::SRV, maxDescriptors, 0, s, DescriptorFlag::DESCRIPTORS_VOLATILE);
		}

		if (s < maxUavSpaces)
		{
			const auto maxDescriptors = pMaxUavsEachSpace ? pMaxUavsEachSpace[s] : 16;
			m_computeSpaceToParamIndexMap[static_cast<uint32_t>(DescriptorType::UAV)][s] = paramIndex;
			pipelineLayout->SetRange(paramIndex++, DescriptorType::UAV, maxDescriptors, 0, s, DescriptorFlag::DESCRIPTORS_VOLATILE);
		}
	}

	if (max32BitConstants > 0)
	{
		m_computeConstantParamIndex = paramIndex;
		pipelineLayout->SetConstants(paramIndex++, max32BitConstants, constantSlot, constantSpace);
	}

	pipelineLayout->SetRange(paramIndex++, DescriptorType::UAV, 1, slotExt, spaceExt);

	XUSG_X_RETURN(m_pipelineLayouts[COMPUTE], pipelineLayout->GetPipelineLayout(m_pipelineLayoutLib.get(),
		PipelineLayoutFlag::NONE, L"EZComputeLayout"), false);

	return true;
}

bool EZ::CommandList_DX12::createShaders()
{
	auto vsIndex = 0u;
	auto psIndex = 0u;
	auto csIndex = 0u;

	XUSG_N_RETURN(m_shaderLib->CreateShader(Shader::Stage::CS, csIndex, CSBlit2D, sizeof(CSBlit2D)), false);
	m_shaders[CS_BLIT_2D] = m_shaderLib->GetShader(Shader::Stage::CS, csIndex++);

	XUSG_N_RETURN(m_shaderLib->CreateShader(Shader::Stage::CS, csIndex, CSBlit3D, sizeof(CSBlit3D)), false);
	m_shaders[CS_BLIT_3D] = m_shaderLib->GetShader(Shader::Stage::CS, csIndex++);

	return true;
}

void EZ::CommandList_DX12::predraw()
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
	for (uint8_t i = 0; i < Shader::Stage::NUM_GRAPHICS; ++i)
	{
		// Create and set sampler table
		auto& samplerTable = m_samplerTables[i];
		if (samplerTable)
		{
			const auto descriptorTable = samplerTable->GetSamplerTable(m_descriptorTableLib.get());
			if (descriptorTable) XUSG::CommandList_DX12::SetGraphicsDescriptorTable(i, descriptorTable);
			samplerTable.reset();
		}

		// Create and set CBV/SRV/UAV tables
		for (uint8_t t = 0; t < CbvSrvUavTypes; ++t)
		{
			auto& cbvSrvUavTables = m_cbvSrvUavTables[i][t];
			const auto numSpaces = static_cast<uint32_t>(cbvSrvUavTables.size());
			for (auto s = 0u; s < numSpaces; ++s)
			{
				auto& cbvSrvUavTable = cbvSrvUavTables[s];
				if (cbvSrvUavTable)
				{
					const auto descriptorTable = cbvSrvUavTable->GetCbvSrvUavTable(m_descriptorTableLib.get());
					if (descriptorTable) XUSG::CommandList_DX12::SetGraphicsDescriptorTable(
						m_graphicsSpaceToParamIndexMap[i][t][s], descriptorTable);
					cbvSrvUavTable.reset();
				}
			}
		}
	}

	// Create pipeline for dynamic states
	assert(m_graphicsState);
	if (m_isGraphicsDirty)
	{
		m_graphicsState->SetPipelineLayout(m_pipelineLayouts[GRAPHICS]);
		const auto pipeline = m_graphicsState->GetPipeline(m_graphicsPipelineLib.get());
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

void EZ::CommandList_DX12::predispatch()
{
	// Set barrier command
	XUSG::CommandList_DX12::Barrier(static_cast<uint32_t>(m_barriers.size()), m_barriers.data());
	m_barriers.clear();

	// Clear UAVs
	clearUAVsUint();
	clearUAVsFloat();

	// Create and set sampler table
	auto& samplerTable = m_samplerTables[Shader::Stage::CS];
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
	assert(m_computeState);
	if (m_isComputeDirty)
	{
		m_computeState->SetPipelineLayout(m_pipelineLayouts[COMPUTE]);
		const auto pipeline = m_computeState->GetPipeline(m_computePipelineLib.get());
		if (pipeline)
		{
			if (m_pipeline != pipeline)
			{
				XUSG::CommandList_DX12::SetPipelineState(pipeline);
				m_pipeline = pipeline;
			}
			m_isComputeDirty = false;
		}
	}
}

void EZ::CommandList_DX12::preexecuteIndirect(Resource* pArgumentBuffer, Resource* pCountBuffer)
{
	assert(pArgumentBuffer);
	auto numBarriers = static_cast<uint32_t>(m_barriers.size());
	m_barriers.resize(numBarriers + 2);
	numBarriers = pArgumentBuffer->SetBarrier(m_barriers.data(), ResourceState::INDIRECT_ARGUMENT, numBarriers);
	if (pCountBuffer) numBarriers = pArgumentBuffer->SetBarrier(m_barriers.data(), ResourceState::INDIRECT_ARGUMENT, numBarriers);

	// Shrink the size of barrier list
	if (numBarriers < m_barriers.size()) m_barriers.resize(numBarriers);
}

void EZ::CommandList_DX12::clearDSVs()
{
	for (const auto& args : m_clearDSVs)
		XUSG::CommandList_DX12::ClearDepthStencilView(args.DepthStencilView, args.ClearFlags,
			args.Depth, args.Stencil, static_cast<uint32_t>(args.Rects.size()), args.Rects.data());
	m_clearDSVs.clear();
}

void EZ::CommandList_DX12::clearRTVs()
{
	for (const auto& args : m_clearRTVs)
		XUSG::CommandList_DX12::ClearRenderTargetView(args.RenderTargetView,
			args.ColorRGBA, static_cast<uint32_t>(args.Rects.size()), args.Rects.data());
	m_clearRTVs.clear();
}

void EZ::CommandList_DX12::clearUAVsUint()
{
	for (const auto& args : m_clearUAVsUint)
		XUSG::CommandList_DX12::ClearUnorderedAccessViewUint(args.UAVTable, args.UnorderedAccessView,
			args.pResource, args.Values, static_cast<uint32_t>(args.Rects.size()), args.Rects.data());
	m_clearUAVsUint.clear();
}

void EZ::CommandList_DX12::clearUAVsFloat()
{
	for (const auto& args : m_clearUAVsFloat)
		XUSG::CommandList_DX12::ClearUnorderedAccessViewFloat(args.UAVTable, args.UnorderedAccessView,
			args.pResource, args.Values, static_cast<uint32_t>(args.Rects.size()), args.Rects.data());
	m_clearUAVsFloat.clear();
}

void EZ::CommandList_DX12::setBarriers(uint32_t numResources, const ResourceView* pResourceViews)
{
	// Estimate barrier count
	auto numBarriersEst = 0u;
	for (auto i = 0u; i < numResources; ++i)
	{
		numBarriersEst += static_cast<uint32_t>(pResourceViews[i].Subresources.size());
		numBarriersEst = pResourceViews[i].pCounter ? numBarriersEst + 1 : numBarriersEst;
	}

	if (numBarriersEst > 0)
	{
		// Generate barriers for each resource
		const auto startIdx = m_barriers.size();
		m_barriers.resize(startIdx + numBarriersEst);
		auto numBarriers = 0u;
		for (auto i = 0u; i < numResources; ++i)
			numBarriers = generateBarriers(&m_barriers[startIdx], pResourceViews[i], numBarriers);

		// Shrink the size of barrier list
		if (numBarriers < numBarriersEst) m_barriers.resize(startIdx + numBarriers);
	}
}

uint32_t EZ::CommandList_DX12::generateBarriers(ResourceBarrier* pBarriers,
	const ResourceView& resrouceView, uint32_t numBarriers, BarrierFlag flags)
{
	assert(resrouceView.pResource || resrouceView.Subresources.empty());
	for (const auto& subresource : resrouceView.Subresources)
		numBarriers = resrouceView.pResource->SetBarrier(pBarriers, resrouceView.DstState, numBarriers, subresource, flags);

	if (resrouceView.pCounter)
	{
		assert(resrouceView.DstState == ResourceState::UNORDERED_ACCESS);
		numBarriers = resrouceView.pCounter->SetBarrier(pBarriers, ResourceState::UNORDERED_ACCESS,
			numBarriers, XUSG_BARRIER_ALL_SUBRESOURCES, flags);
	}

	return numBarriers;
}
