//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSGCommand_DX12.h"
#include "Core/XUSG_DX12.h"
#ifndef XUSG_NO_VULKAN
#include "Core/XUSGCommand_VK.h"
#endif
#include "XUSG-EZ_Impl.h"
#include "Advanced/XUSGAdvanced.h"

#include "CSBlit2D.h"
#include "CSBlit3D.h"
#ifndef XUSG_NO_VULKAN
#include "CSBlit2DSpv.h"
#include "CSBlit3DSpv.h"
#endif

using namespace std;
using namespace XUSG;

template<typename Base>
EZ::CommandList_Impl<Base>::CommandList_Impl() :
	Base(),
	m_graphicsPipelineLib(nullptr),
	m_computePipelineLib(nullptr),
	m_pipelineLayoutLib(nullptr),
	m_descriptorTableLib(nullptr),
	m_pInputLayout(nullptr),
	m_pipelineLayouts(),
	m_blitPipelineLayout(nullptr),
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
	m_spaceStarts(),
	m_graphicsSetDescriptorTables(),
	m_graphicsConstantParamIndices(),
	m_computeConstantParamIndex(0),
	m_shaders()
{
	m_shaderLib = ShaderLib::MakeUnique();
}

template<typename Base>
EZ::CommandList_Impl<Base>::CommandList_Impl(XUSG::CommandList* pCommandList, uint32_t samplerHeapSize, uint32_t cbvSrvUavHeapSize,
	const uint32_t samplerStartSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t maxSamplers[Shader::Stage::NUM_STAGE],
	const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_STAGE],
	const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_STAGE],
	const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_STAGE],
	const uint32_t cbvStartSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t maxCbvSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t srvStartSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t maxSrvSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t uavStartSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t maxUavSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t max32BitConstants[Shader::Stage::NUM_STAGE],
	const uint32_t constantSlots[Shader::Stage::NUM_STAGE],
	const uint32_t constantSpaces[Shader::Stage::NUM_STAGE],
	uint32_t slotExt, uint32_t spaceExt) :
	CommandList_Impl()
{
	Create(pCommandList, samplerHeapSize, cbvSrvUavHeapSize, samplerStartSpaces, maxSamplers,
		pMaxCbvsEachSpace, pMaxSrvsEachSpace, pMaxUavsEachSpace,
		cbvStartSpaces, maxCbvSpaces, srvStartSpaces, maxSrvSpaces, uavStartSpaces, maxUavSpaces,
		max32BitConstants,
		constantSlots, constantSpaces, slotExt, spaceExt);
}

template<typename Base>
EZ::CommandList_Impl<Base>::~CommandList_Impl()
{
}

template<typename Base>
bool EZ::CommandList_Impl<Base>::Create(XUSG::CommandList* pCommandList,
	uint32_t samplerHeapSize, uint32_t cbvSrvUavHeapSize,
	const uint32_t samplerStartSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t maxSamplers[Shader::Stage::NUM_STAGE],
	const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_STAGE],
	const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_STAGE],
	const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_STAGE],
	const uint32_t cbvStartSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t maxCbvSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t srvStartSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t maxSrvSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t uavStartSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t maxUavSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t max32BitConstants[Shader::Stage::NUM_STAGE],
	const uint32_t constantSlots[Shader::Stage::NUM_STAGE],
	const uint32_t constantSpaces[Shader::Stage::NUM_STAGE],
	uint32_t slotExt, uint32_t spaceExt)
{
	XUSG_N_RETURN(init(pCommandList, samplerHeapSize, cbvSrvUavHeapSize), false);
	recordSpaceStarts(cbvStartSpaces, maxCbvSpaces, srvStartSpaces, maxSrvSpaces,
		uavStartSpaces, maxUavSpaces, samplerStartSpaces);

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
		constantSpaces ? constantSpaces[Shader::Stage::CS] :
		spaceStart(Shader::Stage::CS, DescriptorType::CBV) + cMaxCbvSpaces - 1,
		slotExt, spaceExt), false);

	return true;
}

template<typename Base>
bool EZ::CommandList_Impl<Base>::Create(Device* pDevice, void* pNativeHandle,
	uint32_t samplerHeapSize, uint32_t cbvSrvUavHeapSize,
	const uint32_t samplerStartSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t maxSamplers[Shader::Stage::NUM_STAGE],
	const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_STAGE],
	const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_STAGE],
	const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_STAGE],
	const uint32_t cbvStartSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t maxCbvSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t srvStartSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t maxSrvSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t uavStartSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t maxUavSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t max32BitConstants[Shader::Stage::NUM_STAGE],
	const uint32_t constantSlots[Shader::Stage::NUM_STAGE],
	const uint32_t constantSpaces[Shader::Stage::NUM_STAGE],
	uint32_t slotExt, uint32_t spaceExt)
{
	Base::Create(pNativeHandle, pDevice);

	return Create(this, samplerHeapSize, cbvSrvUavHeapSize, samplerStartSpaces, maxSamplers,
		pMaxCbvsEachSpace, pMaxSrvsEachSpace, pMaxUavsEachSpace,
		cbvStartSpaces, maxCbvSpaces, srvStartSpaces, maxSrvSpaces, uavStartSpaces, maxUavSpaces,
		max32BitConstants,
		constantSlots, constantSpaces, slotExt, spaceExt);
}

template<typename Base>
bool EZ::CommandList_Impl<Base>::Close(RenderTarget* pBackBuffer)
{
	if (pBackBuffer)
	{
		ResourceBarrier barrier;
		const auto numBarriers = pBackBuffer->SetBarrier(&barrier, ResourceState::PRESENT);
		Base::Barrier(numBarriers, &barrier);
	}

	return Base::Close();
}

template<typename Base>
bool EZ::CommandList_Impl<Base>::Reset(uint8_t allocatorIndex, const Pipeline& initialState)
{
	XUSG_N_RETURN(Base::Reset(allocatorIndex, initialState), false);

	// Set Descriptor heaps
	const DescriptorHeap descriptorHeaps[] =
	{
		m_descriptorTableLib->GetDescriptorHeap(CBV_SRV_UAV_HEAP),
		m_descriptorTableLib->GetDescriptorHeap(SAMPLER_HEAP)
	};
	Base::SetDescriptorHeaps(static_cast<uint32_t>(size(descriptorHeaps)), descriptorHeaps);

	// Set pipeline layout
	Base::SetComputePipelineLayout(m_pipelineLayouts[COMPUTE]);
	m_pipeline = initialState;
	m_pInputLayout = nullptr;

	m_barriers.clear();
	m_clearDSVs.clear();

	return true;
}

template<typename Base>
void EZ::CommandList_Impl<Base>::Draw(uint32_t vertexCountPerInstance, uint32_t instanceCount,
	uint32_t startVertexLocation, uint32_t startInstanceLocation)
{
	predraw();
	Base::Draw(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
}

template<typename Base>
void EZ::CommandList_Impl<Base>::DrawIndexed(uint32_t indexCountPerInstance, uint32_t instanceCount,
	uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation)
{
	predraw();
	Base::DrawIndexed(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
}

template<typename Base>
void EZ::CommandList_Impl<Base>::DrawIndirect(CommandLayout* pCommandlayout, uint32_t maxCommandCount,
	Resource* pArgumentBuffer, uint64_t argumentBufferOffset, Resource* pCountBuffer, uint64_t countBufferOffset)
{
	preexecuteIndirect(pArgumentBuffer, pCountBuffer);
	predraw();

	Base::ExecuteIndirect(pCommandlayout, maxCommandCount,
		pArgumentBuffer, argumentBufferOffset, pCountBuffer, countBufferOffset);
}

template<typename Base>
void EZ::CommandList_Impl<Base>::Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
{
	predispatch();
	Base::Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}

template<typename Base>
void EZ::CommandList_Impl<Base>::DispatchIndirect(CommandLayout* pCommandlayout, uint32_t maxCommandCount,
	Resource* pArgumentBuffer, uint64_t argumentBufferOffset, Resource* pCountBuffer, uint64_t countBufferOffset)
{
	preexecuteIndirect(pArgumentBuffer, pCountBuffer);
	predispatch();

	Base::ExecuteIndirect(pCommandlayout, maxCommandCount,
		pArgumentBuffer, argumentBufferOffset, pCountBuffer, countBufferOffset);
}

template<typename Base>
void EZ::CommandList_Impl<Base>::CopyBufferRegion(Resource* pDstBuffer, uint64_t dstOffset,
	Resource* pSrcBuffer, uint64_t srcOffset, uint64_t numBytes)
{
	// Generate barriers for each resource
	ResourceBarrier barriers[2];
	auto numBarriers = pDstBuffer->SetBarrier(barriers, ResourceState::COPY_DEST);
	numBarriers = pSrcBuffer->SetBarrier(barriers, ResourceState::COPY_SOURCE, numBarriers);
	Base::Barrier(numBarriers, barriers);

	Base::CopyBufferRegion(pDstBuffer, dstOffset, pSrcBuffer, srcOffset, numBytes);
}

template<typename Base>
void EZ::CommandList_Impl<Base>::CopyTextureRegion(TextureCopyLocation& dst, uint32_t dstX, uint32_t dstY, uint32_t dstZ,
	TextureCopyLocation& src, const BoxRange* pSrcBox)
{
	// Generate barriers for each resource
	ResourceBarrier barriers[2];
	auto numBarriers = dst.pResource->SetBarrier(barriers, ResourceState::COPY_DEST, 0, dst.SubresourceIndex);
	numBarriers = src.pResource->SetBarrier(barriers, ResourceState::COPY_SOURCE, numBarriers, src.SubresourceIndex);
	Base::Barrier(numBarriers, barriers);

	Base::CopyTextureRegion(dst, dstX, dstY, dstZ, src, pSrcBox);
}

template<typename Base>
void EZ::CommandList_Impl<Base>::CopyResource(Resource* pDstResource, Resource* pSrcResource)
{
	// Generate barriers for each resource
	ResourceBarrier barriers[2];
	auto numBarriers = pDstResource->SetBarrier(barriers, ResourceState::COPY_DEST);
	numBarriers = pSrcResource->SetBarrier(barriers, ResourceState::COPY_SOURCE, numBarriers);
	Base::Barrier(numBarriers, barriers);

	Base::CopyResource(pDstResource, pSrcResource);
}

template<typename Base>
void EZ::CommandList_Impl<Base>::CopyTiles(Resource* pTiledResource, const TiledResourceCoord* pTileRegionStartCoord,
	const TileRegionSize& tileRegionSize, Resource* pBuffer, uint64_t bufferStartOffsetInBytes, TileCopyFlag flags)
{
	ResourceBarrier barrier;
	const auto numBarriers = pTiledResource->SetBarrier(&barrier, ResourceState::COPY_DEST);
	Base::Barrier(numBarriers, &barrier);

	Base::CopyTiles(pTiledResource, pTileRegionStartCoord,
		tileRegionSize, pBuffer, bufferStartOffsetInBytes, flags);
}

template<typename Base>
void EZ::CommandList_Impl<Base>::ResolveSubresource(Resource* pDstResource, uint32_t dstSubresource,
	Resource* pSrcResource, uint32_t srcSubresource, Format format)
{
	assert(pDstResource);
	assert(pSrcResource);

	// Generate barriers for each resource
	ResourceBarrier barriers[2];
	auto numBarriers = pDstResource->SetBarrier(barriers, ResourceState::RESOLVE_DEST, 0, dstSubresource);
	numBarriers = pSrcResource->SetBarrier(barriers, ResourceState::RESOLVE_SOURCE, numBarriers, srcSubresource);
	Base::Barrier(numBarriers, barriers);

	Base::ResolveSubresource(pDstResource, dstSubresource, pSrcResource, srcSubresource, format);
}

template<typename Base>
void EZ::CommandList_Impl<Base>::IASetInputLayout(const InputLayout* pLayout)
{
	assert(m_graphicsState);
	m_graphicsState->IASetInputLayout(pLayout);
	m_isGraphicsDirty = true;
}

template<typename Base>
void EZ::CommandList_Impl<Base>::IASetIndexBufferStripCutValue(IBStripCutValue ibStripCutValue)
{
	assert(m_graphicsState);
	m_graphicsState->IASetIndexBufferStripCutValue(ibStripCutValue);
	m_isGraphicsDirty = true;
}

template<typename Base>
void EZ::CommandList_Impl<Base>::RSSetState(const Graphics::Rasterizer* pRasterizer)
{
	assert(m_graphicsState);
	m_graphicsState->RSSetState(pRasterizer);
	m_isGraphicsDirty = true;
}

template<typename Base>
void EZ::CommandList_Impl<Base>::RSSetState(Graphics::RasterizerPreset preset)
{
	assert(m_graphicsState);
	m_graphicsState->RSSetState(preset, m_graphicsPipelineLib.get());
	m_isGraphicsDirty = true;
}

template<typename Base>
void EZ::CommandList_Impl<Base>::OMSetBlendState(const Graphics::Blend* pBlend, uint32_t sampleMask)
{
	assert(m_graphicsState);
	m_graphicsState->OMSetBlendState(pBlend, sampleMask);
	m_isGraphicsDirty = true;
}

template<typename Base>
void EZ::CommandList_Impl<Base>::OMSetBlendState(Graphics::BlendPreset preset, uint8_t numColorRTs, uint32_t sampleMask)
{
	assert(m_graphicsState);
	m_graphicsState->OMSetBlendState(preset, m_graphicsPipelineLib.get(), numColorRTs, sampleMask);
	m_isGraphicsDirty = true;
}

template<typename Base>
void EZ::CommandList_Impl<Base>::OMSetSample(uint8_t count, uint8_t quality)
{
	assert(m_graphicsState);
	m_graphicsState->OMSetSample(count, quality);
	m_isGraphicsDirty = true;
}

template<typename Base>
void EZ::CommandList_Impl<Base>::DSSetState(const Graphics::DepthStencil* pDepthStencil)
{
	assert(m_graphicsState);
	m_graphicsState->DSSetState(pDepthStencil);
	m_isGraphicsDirty = true;
}

template<typename Base>
void EZ::CommandList_Impl<Base>::DSSetState(Graphics::DepthStencilPreset preset)
{
	assert(m_graphicsState);
	m_graphicsState->DSSetState(preset, m_graphicsPipelineLib.get());
	m_isGraphicsDirty = true;
}

template<typename Base>
void EZ::CommandList_Impl<Base>::SetGraphicsShader(Shader::Stage stage, const Blob& shader)
{
	assert(stage < Shader::Stage::NUM_GRAPHICS);
	assert(m_graphicsState);
	m_graphicsState->SetShader(stage, shader);
	m_isGraphicsDirty = true;
}

template<typename Base>
void EZ::CommandList_Impl<Base>::SetGraphicsNodeMask(uint32_t nodeMask)
{
	assert(m_graphicsState);
	m_graphicsState->SetNodeMask(nodeMask);
	m_isGraphicsDirty = true;
}

template<typename Base>
void EZ::CommandList_Impl<Base>::SetComputeShader(const Blob& shader)
{
	assert(m_computeState);
	m_computeState->SetShader(shader);
	m_isComputeDirty = true;
}

template<typename Base>
void EZ::CommandList_Impl<Base>::SetComputeNodeMask(uint32_t nodeMask)
{
	assert(m_computeState);
	m_computeState->SetNodeMask(nodeMask);
	m_isComputeDirty = true;
}

template<typename Base>
void EZ::CommandList_Impl<Base>::SetGraphicsPipelineState(const Pipeline& pipelineState, const Graphics::State* pState)
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
			Base::SetPipelineState(pipeline);
			m_pipeline = pipeline;
		}
		m_isGraphicsDirty = false;
	}
}

template<typename Base>
void EZ::CommandList_Impl<Base>::SetComputePipelineState(const Pipeline& pipelineState, const Compute::State* pState)
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
			Base::SetPipelineState(pipeline);
			m_pipeline = pipeline;
		}
		m_isComputeDirty = false;
	}
}

template<typename Base>
void EZ::CommandList_Impl<Base>::SetSamplers(Shader::Stage stage, uint32_t startBinding, uint32_t numSamplers, const Sampler* const* ppSamplers)
{
	auto& descriptorTable = m_samplerTables[stage];
	if (!descriptorTable) descriptorTable = Util::DescriptorTable::MakeUnique();
	descriptorTable->SetSamplers(startBinding, numSamplers, ppSamplers);
}

template<typename Base>
void EZ::CommandList_Impl<Base>::SetSamplerStates(Shader::Stage stage, uint32_t startBinding, uint32_t numSamplers, const SamplerPreset* pSamplerPresets)
{
	auto& descriptorTable = m_samplerTables[stage];
	if (!descriptorTable) descriptorTable = Util::DescriptorTable::MakeUnique();
	descriptorTable->SetSamplers(startBinding, numSamplers, pSamplerPresets, m_descriptorTableLib.get());
}

template<typename Base>
void EZ::CommandList_Impl<Base>::SetResources(Shader::Stage stage, DescriptorType descriptorType, uint32_t startBinding,
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
				if (pResourceViews[i].View->pResource == args.DepthStencilView->pResource)
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
				if (pResourceViews[i].View->pResource == args.RenderTargetView->pResource)
				{
					needClearRTVs = true;
					break;
				}
			}
		}

		if (needClearDSVs || needClearRTVs)
		{
			// Set barrier command
			Base::Barrier(static_cast<uint32_t>(m_barriers.size()), m_barriers.data());
			m_barriers.clear();

			if (needClearDSVs) clearDSVs();
			if (needClearRTVs) clearRTVs();
		}
	}

	// Map descriptor table. space is the register space; tables are indexed from the stage start.
	auto& descriptorTables = m_cbvSrvUavTables[stage][static_cast<uint32_t>(descriptorType)];
	const auto spaceIndex = space - spaceStart(stage, descriptorType);
	assert(spaceIndex < descriptorTables.size());
	auto& descriptorTable = descriptorTables[spaceIndex];

	// Set descriptors to the descriptor table
	if (!descriptorTable) descriptorTable = Util::DescriptorTable::MakeUnique();
	if (m_descriptors.size() < numResources) m_descriptors.resize(numResources);
	for (auto i = 0u; i < numResources; ++i) m_descriptors[i] = pResourceViews[i].View;
	descriptorTable->SetDescriptors(startBinding, numResources, m_descriptors.data());

	// Set barriers
	if (descriptorType == DescriptorType::SRV || descriptorType == DescriptorType::UAV)
		setBarriers(numResources, pResourceViews);
}

template<typename Base>
void EZ::CommandList_Impl<Base>::SetGraphicsDescriptorTable(Shader::Stage stage, DescriptorType descriptorType,
	const DescriptorTable& descriptorTable, uint32_t space)
{
	// Defer setting the descriptor table after pipeline layout set
	m_graphicsSetDescriptorTables.emplace_back(SetDescriptorTable{ stage, descriptorType, descriptorTable, space });
}

template<typename Base>
void EZ::CommandList_Impl<Base>::SetComputeDescriptorTable(DescriptorType descriptorType, const DescriptorTable& descriptorTable, uint32_t space)
{
	const auto spaceIndex = space - spaceStart(Shader::Stage::CS, descriptorType);
	Base::SetComputeDescriptorTable(m_computeSpaceToParamIndexMap[static_cast<uint32_t>(descriptorType)][spaceIndex], descriptorTable);
}

template<typename Base>
void EZ::CommandList_Impl<Base>::SetGraphics32BitConstant(Shader::Stage stage, uint32_t srcData, uint32_t destOffsetIn32BitValues)
{
	// Defer setting the constant after pipeline layout set
	assert(stage < Shader::Stage::NUM_GRAPHICS);
	m_graphicsSetSingle32BitConstants.emplace_back(Set32BitConstant{ stage, srcData, destOffsetIn32BitValues });
}

template<typename Base>
void EZ::CommandList_Impl<Base>::SetCompute32BitConstant(uint32_t srcData, uint32_t destOffsetIn32BitValues)
{
	Base::SetCompute32BitConstant(m_computeConstantParamIndex, srcData, destOffsetIn32BitValues);
}

template<typename Base>
void EZ::CommandList_Impl<Base>::SetGraphics32BitConstants(Shader::Stage stage, uint32_t num32BitValuesToSet,
	const void* pSrcData, uint32_t destOffsetIn32BitValues)
{
	// Defer setting the constants after pipeline layout set
	assert(stage < Shader::Stage::NUM_GRAPHICS);
	m_graphicsSet32BitConstants.emplace_back(Set32BitConstants{ stage, num32BitValuesToSet, pSrcData, destOffsetIn32BitValues });
}

template<typename Base>
void EZ::CommandList_Impl<Base>::SetCompute32BitConstants(uint32_t num32BitValuesToSet, const void* pSrcData, uint32_t destOffsetIn32BitValues)
{
	Base::SetCompute32BitConstants(m_computeConstantParamIndex, num32BitValuesToSet, pSrcData, destOffsetIn32BitValues);
}

template<typename Base>
void EZ::CommandList_Impl<Base>::IASetPrimitiveTopology(PrimitiveTopology primitiveTopology)
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

	Base::IASetPrimitiveTopology(primitiveTopology);
}

template<typename Base>
void EZ::CommandList_Impl<Base>::IASetIndexBuffer(const IndexBufferView& view)
{
	// Set barriers if the index buffers is not at the read states
	const auto startIdx = m_barriers.size();
	m_barriers.resize(startIdx + 1);
	const auto numBarriers = view.pResource->SetBarrier(&m_barriers[startIdx], view.DstState);

	// Shrink the size of barrier list
	if (!numBarriers) m_barriers.resize(startIdx);

	Base::IASetIndexBuffer(*view.pView);
}

template<typename Base>
void EZ::CommandList_Impl<Base>::IASetVertexBuffers(uint32_t startSlot, uint32_t numViews, const VertexBufferView* pViews)
{
	vector<XUSG::VertexBufferView> views(numViews);

	// Set barriers if the vertex buffers is not at the read states
	const auto startIdx = m_barriers.size();
	m_barriers.resize(startIdx + numViews);
	auto numBarriers = 0u;
	for (auto i = 0u; i < numViews; ++i)
	{
		auto& view = pViews[i];
		numBarriers = view.pResource->SetBarrier(&m_barriers[startIdx], view.DstState, numBarriers);
		views[i] = *view.pView;
	}

	// Shrink the size of barrier list
	if (numBarriers < numViews) m_barriers.resize(startIdx + numBarriers);

	Base::IASetVertexBuffers(startSlot, numViews, views.data());
}

template<typename Base>
void EZ::CommandList_Impl<Base>::SOSetTargets(uint32_t startSlot, uint32_t numViews,
	const StreamOutBufferView* pViews, Resource* const* ppResources)
{
	// Set barriers if the stream-out buffers is not at the stream-out states
	const auto startIdx = m_barriers.size();
	m_barriers.resize(startIdx + numViews);
	auto numBarriers = 0u;
	for (auto i = 0u; i < numViews; ++i)
		numBarriers = ppResources[i]->SetBarrier(&m_barriers[startIdx], ResourceState::STREAM_OUT, numBarriers);

	// Shrink the size of barrier list
	if (numBarriers < numViews) m_barriers.resize(startIdx + numBarriers);

	Base::SOSetTargets(startSlot, numViews, pViews);
}

template<typename Base>
void EZ::CommandList_Impl<Base>::OMSetRenderTargets(uint32_t numRenderTargets,
	const ResourceView* pRenderTargetViews, const ResourceView* pDepthStencilView)
{
	// Set RTV barriers
	setBarriers(numRenderTargets, pRenderTargetViews);

	// Set DSV barriers
	if (pDepthStencilView)
	{
		setBarriers(1, pDepthStencilView);
		const auto pTexture = dynamic_cast<const Texture*>(pDepthStencilView->View->pResource);
		m_graphicsState->OMSetDSVFormat(pTexture ? pTexture->GetFormat() : Format::UNKNOWN);
	}
	else m_graphicsState->OMSetDSVFormat(Format::UNKNOWN);

	m_graphicsState->OMSetNumRenderTargets(numRenderTargets);

	Descriptor pRTVs[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
	for (auto i = 0u; i < numRenderTargets; ++i)
	{
		pRTVs[i] = pRenderTargetViews[i].View;
		const auto pTexture = dynamic_cast<const Texture*>(pRenderTargetViews[i].View->pResource);
		m_graphicsState->OMSetRTVFormat(i, pTexture ? pTexture->GetFormat() : Format::UNKNOWN);
	}

	for (auto i = numRenderTargets; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		m_graphicsState->OMSetRTVFormat(i, Format::UNKNOWN);

	m_isGraphicsDirty = true;

	Base::OMSetRenderTargets(numRenderTargets, pRTVs,
		pDepthStencilView ? &pDepthStencilView->View : nullptr);
}

template<typename Base>
void EZ::CommandList_Impl<Base>::ClearDepthStencilView(const ResourceView& depthStencilView, ClearFlag clearFlags,
	float depth, uint8_t stencil, uint32_t numRects, const RectRange* pRects)
{
	setBarriers(1, &depthStencilView);
	m_clearDSVs.emplace_back(ClearDSV{ depthStencilView.View, clearFlags, depth, stencil });

	auto& clearView = m_clearDSVs.back();
	clearView.Rects.resize(numRects);
	for (auto i = 0u; i < numRects; ++i) clearView.Rects[i] = pRects[i];
}

template<typename Base>
void EZ::CommandList_Impl<Base>::ClearRenderTargetView(const ResourceView& renderTargetView,
	const float colorRGBA[4], uint32_t numRects, const RectRange* pRects)
{
	setBarriers(1, &renderTargetView);
	m_clearRTVs.emplace_back(ClearRTV{ renderTargetView.View, colorRGBA[0], colorRGBA[1], colorRGBA[2], colorRGBA[3] });

	auto& clearView = m_clearRTVs.back();
	clearView.Rects.resize(numRects);
	for (auto i = 0u; i < numRects; ++i) clearView.Rects[i] = pRects[i];
}

template<typename Base>
void EZ::CommandList_Impl<Base>::ClearUnorderedAccessViewUint(const ResourceView& unorderedAccessView,
	const uint32_t values[4], uint32_t numRects, const RectRange* pRects)
{
	setBarriers(1, &unorderedAccessView);

	const auto uavTable = Util::DescriptorTable::MakeUnique();
	uavTable->SetDescriptors(0, 1, &unorderedAccessView.View);
	const auto descriptorTable = uavTable->GetCbvSrvUavTable(m_descriptorTableLib.get());

	m_clearUAVs.emplace_back(unorderedAccessView);
	m_clearUAVsUint.emplace_back(ClearUAVUint{ &m_clearUAVs.back(),
		descriptorTable, values[0], values[1], values[2], values[3] });

	auto& clearView = m_clearUAVsUint.back();
	clearView.Rects.resize(numRects);
	for (auto i = 0u; i < numRects; ++i) clearView.Rects[i] = pRects[i];
}

template<typename Base>
void EZ::CommandList_Impl<Base>::ClearUnorderedAccessViewFloat(const ResourceView& unorderedAccessView,
	const float values[4], uint32_t numRects, const RectRange* pRects)
{
	setBarriers(1, &unorderedAccessView);

	const auto uavTable = Util::DescriptorTable::MakeUnique();
	uavTable->SetDescriptors(0, 1, &unorderedAccessView.View);
	const auto descriptorTable = uavTable->GetCbvSrvUavTable(m_descriptorTableLib.get());

	m_clearUAVs.emplace_back(unorderedAccessView);
	m_clearUAVsFloat.emplace_back(ClearUAVFloat{ &m_clearUAVs.back(),
		descriptorTable, values[0], values[1], values[2], values[3] });

	auto& clearView = m_clearUAVsFloat.back();
	clearView.Rects.resize(numRects);
	for (auto i = 0u; i < numRects; ++i) clearView.Rects[i] = pRects[i];
}

template<typename Base>
void EZ::CommandList_Impl<Base>::Resize()
{
	m_descriptorTableLib->ResetDescriptorHeap(CBV_SRV_UAV_HEAP);
}

template<typename Base>
void EZ::CommandList_Impl<Base>::Blit(Texture* pDstResource, Texture* pSrcResource, SamplerPreset sampler,
	const Blob& customShader, uint32_t dstMip, uint32_t srcMip)
{
	assert(pDstResource);
	assert(dstMip < pDstResource->GetNumMips());
	assert(pSrcResource);
	assert(srcMip < pDstResource->GetNumMips());
	const auto uav = EZ::GetUAV(pDstResource, dstMip);
	const auto srv = pSrcResource->GetNumMips() > 1 ? EZ::GetSRV(pSrcResource, srcMip, true) : EZ::GetSRV(pSrcResource);

	const uint32_t width = static_cast<uint32_t>(pDstResource->GetWidth());
	const uint32_t height = pDstResource->GetHeight();
	const auto threadsX = (max)(width >> dstMip, 1u);
	const auto threadsY = (max)(height >> dstMip, 1u);
	dispatchBlit(customShader ? customShader : m_shaders[CS_BLIT_2D], sampler, srv, uav,
		XUSG_DIV_UP(threadsX, 8), XUSG_DIV_UP(threadsY, 8), 1);
}

template<typename Base>
void EZ::CommandList_Impl<Base>::Blit(Texture3D* pDstResource, Texture* pSrcResource, SamplerPreset sampler,
	const Blob& customShader, uint32_t dstMip, uint32_t srcMip)
{
	assert(pDstResource);
	assert(dstMip < pDstResource->GetNumMips());
	assert(pSrcResource);
	assert(srcMip < pDstResource->GetNumMips());
	const auto uav = EZ::GetUAV(pDstResource, dstMip);
	const auto srv = pSrcResource->GetNumMips() > 1 ? EZ::GetSRV(pSrcResource, srcMip, true) : EZ::GetSRV(pSrcResource);

	const uint32_t width = static_cast<uint32_t>(pDstResource->GetWidth());
	const uint32_t height = pDstResource->GetHeight();
	const uint32_t depth = pDstResource->GetDepth();
	const auto threadsX = (max)(width >> dstMip, 1u);
	const auto threadsY = (max)(height >> dstMip, 1u);
	const auto threadsZ = (max)(depth >> dstMip, 1u);
	dispatchBlit(customShader ? customShader : m_shaders[CS_BLIT_3D], sampler, srv, uav,
		XUSG_DIV_UP(threadsX, 4), XUSG_DIV_UP(threadsY, 4), XUSG_DIV_UP(threadsZ, 4));
}

template<typename Base>
void EZ::CommandList_Impl<Base>::GenerateMips(Texture* pResource, SamplerPreset sampler, const Blob& customShader)
{
	assert(pResource);
	const uint32_t width = static_cast<uint32_t>(pResource->GetWidth());
	const uint32_t height = pResource->GetHeight();
	const uint8_t numMips = pResource->GetNumMips();
	const auto& shader = customShader ? customShader : m_shaders[CS_BLIT_2D];
	for (uint8_t i = 1; i < numMips; ++i)
	{
		const auto uav = EZ::GetUAV(pResource, i);
		const auto srv = EZ::GetSRV(pResource, i - 1, true);
		const auto threadsX = (max)(width >> i, 1u);
		const auto threadsY = (max)(height >> i, 1u);
		dispatchBlit(shader, sampler, srv, uav, XUSG_DIV_UP(threadsX, 8), XUSG_DIV_UP(threadsY, 8), 1);
	}
}

template<typename Base>
void EZ::CommandList_Impl<Base>::GenerateMips(Texture3D* pResource, SamplerPreset sampler, const Blob& customShader)
{
	assert(pResource);
	const uint32_t width = static_cast<uint32_t>(pResource->GetWidth());
	const uint32_t height = pResource->GetHeight();
	const uint32_t depth = pResource->GetDepth();
	const uint8_t numMips = pResource->GetNumMips();
	const auto& shader = customShader ? customShader : m_shaders[CS_BLIT_3D];
	for (uint8_t i = 1; i < numMips; ++i)
	{
		const auto uav = EZ::GetUAV(pResource, i);
		const auto srv = EZ::GetSRV(pResource, i - 1, true);
		const auto threadsX = (max)(width >> i, 1u);
		const auto threadsY = (max)(height >> i, 1u);
		const auto threadsZ = (max)(depth >> i, 1u);
		dispatchBlit(shader, sampler, srv, uav,
			XUSG_DIV_UP(threadsX, 4), XUSG_DIV_UP(threadsY, 4), XUSG_DIV_UP(threadsZ, 4));
	}
}

template<typename Base>
const Graphics::Blend* EZ::CommandList_Impl<Base>::GetBlend(Graphics::BlendPreset preset, uint8_t numColorRTs)
{
	return m_graphicsPipelineLib->GetBlend(preset, numColorRTs);
}

template<typename Base>
const Graphics::Rasterizer* EZ::CommandList_Impl<Base>::GetRasterizer(Graphics::RasterizerPreset preset)
{
	return m_graphicsPipelineLib->GetRasterizer(preset);
}

template<typename Base>
const Graphics::DepthStencil* EZ::CommandList_Impl<Base>::GetDepthStencil(Graphics::DepthStencilPreset preset)
{
	return m_graphicsPipelineLib->GetDepthStencil(preset);
}

template<typename Base>
DescriptorTableLib* EZ::CommandList_Impl<Base>::GetDescriptorTableLib() const
{
	return m_descriptorTableLib.get();
}

template<typename Base>
const PipelineLayout& EZ::CommandList_Impl<Base>::GetGraphicsPipelineLayout() const
{
	return m_pipelineLayouts[GRAPHICS];
}

template<typename Base>
const PipelineLayout& EZ::CommandList_Impl<Base>::GetComputePipelineLayout() const
{
	return m_pipelineLayouts[COMPUTE];
}

template<typename Base>
uint32_t EZ::CommandList_Impl<Base>::GetGraphicsConstantParamIndex(Shader::Stage stage) const
{
	return m_graphicsConstantParamIndices[stage];
}

template<typename Base>
uint32_t EZ::CommandList_Impl<Base>::GetComputeConstantParamIndex() const
{
	return m_computeConstantParamIndex;
}

template<typename Base>
CommandList* EZ::CommandList_Impl<Base>::AsCommandList()
{
	const auto pCommandList = dynamic_cast<XUSG::CommandList*>(this);
	assert(pCommandList);

	return pCommandList;
}

template<typename Base>
bool EZ::CommandList_Impl<Base>::init(XUSG::CommandList* pCommandList, uint32_t samplerHeapSize, uint32_t cbvSrvUavHeapSize)
{
	XUSG_N_RETURN(adopt(pCommandList), false);

	const auto pDevice = pCommandList->GetDevice();
	m_graphicsPipelineLib = Graphics::PipelineLib::MakeUnique(pDevice);
	m_computePipelineLib = Compute::PipelineLib::MakeUnique(pDevice);
	m_pipelineLayoutLib = PipelineLayoutLib::MakeUnique(pDevice);
	m_descriptorTableLib = DescriptorTableLib::MakeUnique(pDevice, L"EZDescriptorTableLib");
	m_graphicsState = Graphics::State::MakeUnique();
	m_computeState = Compute::State::MakeUnique();

	// Create internal shaders
	XUSG_N_RETURN(createShaders(), false);

	// Allocate descriptor heaps
	XUSG_N_RETURN(m_descriptorTableLib->AllocateDescriptorHeap(SAMPLER_HEAP, samplerHeapSize), false);
	XUSG_N_RETURN(m_descriptorTableLib->AllocateDescriptorHeap(CBV_SRV_UAV_HEAP, cbvSrvUavHeapSize), false);

	return true;
}

template<typename Base>
bool EZ::CommandList_Impl<Base>::createGraphicsPipelineLayouts(
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
	const auto pipelineLayout = Util::PipelineLayout::MakeUnique();

	// Handle all samplers to take up the first N root params with fixed param indices 
	for (uint8_t i = 0; i < Shader::Stage::NUM_GRAPHICS; ++i)
	{
		const Shader::Stage stage = static_cast<Shader::Stage>(i);
		const auto stageMaxSamplers = maxSamplers ? maxSamplers[stage] : (i == Shader::Stage::PS ? 16 : 0);

		if (stageMaxSamplers > 0)
		{
			pipelineLayout->SetRange(paramIndex, DescriptorType::SAMPLER, stageMaxSamplers, 0,
				spaceStart(stage, DescriptorType::SAMPLER), DescriptorFlag::DESCRIPTORS_VOLATILE);
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
		const auto constantSpace = constantSpaces ? constantSpaces[stage] : spaceStart(stage, DescriptorType::CBV);
		const auto constantSlot = constantSlots ? constantSlots[stage] : 0;
		const auto cbvSpaceStart = spaceStart(stage, DescriptorType::CBV);
		const auto srvSpaceStart = spaceStart(stage, DescriptorType::SRV);
		const auto uavSpaceStart = spaceStart(stage, DescriptorType::UAV);

		for (auto s = 0u; s < maxSpaces; ++s)
		{
			if (s < stageMaxCbvSpaces)
			{
				auto maxDescriptors = pMaxCbvsEachSpace && pMaxCbvsEachSpace[stage] ? pMaxCbvsEachSpace[stage][s] : 14;
				auto slot = 0;
				if (cbvSpaceStart + s == constantSpace && num32BitValues > 0)
				{
					maxDescriptors = constantSlot ? (min)(maxDescriptors, constantSlot) : maxDescriptors - 1;
					slot = constantSlot ? 0 : 1;
				}
				spaceToParamIndexMap[static_cast<uint32_t>(DescriptorType::CBV)][s] = paramIndex;
				pipelineLayout->SetRange(paramIndex, DescriptorType::CBV, maxDescriptors, slot, cbvSpaceStart + s,
					DescriptorFlag::DESCRIPTORS_VOLATILE);
				pipelineLayout->SetShaderStage(paramIndex++, stage);
			}

			if (s < stageMaxSrvSpaces)
			{
				const auto maxDescriptors = pMaxSrvsEachSpace && pMaxSrvsEachSpace[stage] ? pMaxSrvsEachSpace[stage][s] : 32;
				spaceToParamIndexMap[static_cast<uint32_t>(DescriptorType::SRV)][s] = paramIndex;
				pipelineLayout->SetRange(paramIndex, DescriptorType::SRV, maxDescriptors, 0, srvSpaceStart + s,
					DescriptorFlag::DESCRIPTORS_VOLATILE);
				pipelineLayout->SetShaderStage(paramIndex++, stage);
			}

			if (s < stageMaxUavSpaces)
			{
				const auto maxDescriptors = pMaxUavsEachSpace && pMaxUavsEachSpace[stage] ? pMaxUavsEachSpace[stage][s] : 16;
				spaceToParamIndexMap[static_cast<uint32_t>(DescriptorType::UAV)][s] = paramIndex;
				pipelineLayout->SetRange(paramIndex, DescriptorType::UAV, maxDescriptors, 0, uavSpaceStart + s,
					DescriptorFlag::DESCRIPTORS_VOLATILE);
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

template<typename Base>
bool EZ::CommandList_Impl<Base>::createComputePipelineLayouts(uint32_t maxSamplers,
	const uint32_t* pMaxCbvsEachSpace, const uint32_t* pMaxSrvsEachSpace, const uint32_t* pMaxUavsEachSpace,
	uint32_t maxCbvSpaces, uint32_t maxSrvSpaces, uint32_t maxUavSpaces,
	uint32_t max32BitConstants, uint32_t constantSlot, uint32_t constantSpace,
	uint32_t slotExt, uint32_t spaceExt)
{
	// Create common compute pipeline layout
	auto paramIndex = 0u;
	const auto pipelineLayout = Util::PipelineLayout::MakeUnique();
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

	const auto cbvSpaceStart = spaceStart(Shader::Stage::CS, DescriptorType::CBV);
	const auto srvSpaceStart = spaceStart(Shader::Stage::CS, DescriptorType::SRV);
	const auto uavSpaceStart = spaceStart(Shader::Stage::CS, DescriptorType::UAV);
	pipelineLayout->SetRange(paramIndex++, DescriptorType::SAMPLER, maxSamplers, 0,
		spaceStart(Shader::Stage::CS, DescriptorType::SAMPLER), DescriptorFlag::DESCRIPTORS_VOLATILE);

	for (auto s = 0u; s < maxSpaces; ++s)
	{
		if (s < maxCbvSpaces)
		{
			auto maxDescriptors = pMaxCbvsEachSpace ? pMaxCbvsEachSpace[s] : 14;
			auto slot = 0;
			if (cbvSpaceStart + s == constantSpace && max32BitConstants > 0)
			{
				maxDescriptors = constantSlot ? (min)(maxDescriptors, constantSlot) : maxDescriptors - 1;
				slot = constantSlot ? 0 : 1;
			}
			m_computeSpaceToParamIndexMap[static_cast<uint32_t>(DescriptorType::CBV)][s] = paramIndex;
			pipelineLayout->SetRange(paramIndex++, DescriptorType::CBV, maxDescriptors, slot, cbvSpaceStart + s,
				DescriptorFlag::DESCRIPTORS_VOLATILE);
		}

		if (s < maxSrvSpaces)
		{
			const auto maxDescriptors = pMaxSrvsEachSpace ? pMaxSrvsEachSpace[s] : 32;
			m_computeSpaceToParamIndexMap[static_cast<uint32_t>(DescriptorType::SRV)][s] = paramIndex;
			pipelineLayout->SetRange(paramIndex++, DescriptorType::SRV, maxDescriptors, 0, srvSpaceStart + s,
				DescriptorFlag::DESCRIPTORS_VOLATILE);
		}

		if (s < maxUavSpaces)
		{
			const auto maxDescriptors = pMaxUavsEachSpace ? pMaxUavsEachSpace[s] : 16;
			m_computeSpaceToParamIndexMap[static_cast<uint32_t>(DescriptorType::UAV)][s] = paramIndex;
			pipelineLayout->SetRange(paramIndex++, DescriptorType::UAV, maxDescriptors, 0, uavSpaceStart + s,
				DescriptorFlag::DESCRIPTORS_VOLATILE);
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

template<typename Base>
bool EZ::CommandList_Impl<Base>::createShaders()
{
	auto csIndex = 0u;

#ifndef XUSG_NO_VULKAN
	XUSG_N_RETURN(m_shaderLib->CreateShader(Shader::Stage::CS, csIndex, CSBlit2D, sizeof(CSBlit2D),
		CSBlit2DSpv, sizeof(CSBlit2DSpv)), false);
#else
	XUSG_N_RETURN(m_shaderLib->CreateShader(Shader::Stage::CS, csIndex, CSBlit2D, sizeof(CSBlit2D)), false);
#endif
	m_shaders[CS_BLIT_2D] = m_shaderLib->GetShader(Shader::Stage::CS, csIndex++);

#ifndef XUSG_NO_VULKAN
	XUSG_N_RETURN(m_shaderLib->CreateShader(Shader::Stage::CS, csIndex, CSBlit3D, sizeof(CSBlit3D),
		CSBlit3DSpv, sizeof(CSBlit3DSpv)), false);
#else
	XUSG_N_RETURN(m_shaderLib->CreateShader(Shader::Stage::CS, csIndex, CSBlit3D, sizeof(CSBlit3D)), false);
#endif
	m_shaders[CS_BLIT_3D] = m_shaderLib->GetShader(Shader::Stage::CS, csIndex++);

	return createBlitPipelineLayout();
}

template<typename Base>
bool EZ::CommandList_Impl<Base>::createBlitPipelineLayout()
{
	const auto vulkan = Device::GetApi() == API::VULKAN;
	const auto samplerSpace = vulkan ? XUSG_VK_SPACE_SAMPLER : 0u;
	const auto srvSpace = vulkan ? XUSG_VK_SPACE_SRV : 0u;
	const auto uavSpace = vulkan ? XUSG_VK_SPACE_UAV : 0u;

	auto paramIndex = 0u;
	const auto pipelineLayout = Util::PipelineLayout::MakeUnique();
	pipelineLayout->SetRange(paramIndex, DescriptorType::SAMPLER, 1, 0, samplerSpace, DescriptorFlag::DESCRIPTORS_VOLATILE);
	pipelineLayout->SetShaderStage(paramIndex++, Shader::Stage::CS);
	pipelineLayout->SetRange(paramIndex, DescriptorType::SRV, 1, 0, srvSpace, DescriptorFlag::DESCRIPTORS_VOLATILE);
	pipelineLayout->SetShaderStage(paramIndex++, Shader::Stage::CS);
	pipelineLayout->SetRange(paramIndex, DescriptorType::UAV, 1, 0, uavSpace, DescriptorFlag::DESCRIPTORS_VOLATILE);
	pipelineLayout->SetShaderStage(paramIndex++, Shader::Stage::CS);

	XUSG_X_RETURN(m_blitPipelineLayout, pipelineLayout->GetPipelineLayout(m_pipelineLayoutLib.get(),
		PipelineLayoutFlag::NONE, L"EZBlitLayout"), false);

	return true;
}

template<typename Base>
void EZ::CommandList_Impl<Base>::recordSpaceStarts(
	const uint32_t cbvStartSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t maxCbvSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t srvStartSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t maxSrvSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t uavStartSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t maxUavSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t samplerStartSpaces[Shader::Stage::NUM_STAGE])
{
	const auto vulkan = Device::GetApi() == API::VULKAN;
	for (uint8_t stage = 0; stage < Shader::Stage::NUM_STAGE; ++stage)
	{
		const auto maxCbv = maxCbvSpaces ? maxCbvSpaces[stage] : 1u;
		const auto maxSrv = maxSrvSpaces ? maxSrvSpaces[stage] : 1u;
		const auto maxUav = maxUavSpaces ? maxUavSpaces[stage] : 1u;
		const auto cbv = cbvStartSpaces ? cbvStartSpaces[stage] : 0u;
		const auto srv = srvStartSpaces ? srvStartSpaces[stage] : (vulkan ? cbv + maxCbv : 0u);
		const auto uav = uavStartSpaces ? uavStartSpaces[stage] : (vulkan ? srv + maxSrv : 0u);
		const auto sampler = samplerStartSpaces ? samplerStartSpaces[stage] : (vulkan ? uav + maxUav : 0u);

		m_spaceStarts[stage][static_cast<uint32_t>(DescriptorType::CBV)] = cbv;
		m_spaceStarts[stage][static_cast<uint32_t>(DescriptorType::SRV)] = srv;
		m_spaceStarts[stage][static_cast<uint32_t>(DescriptorType::UAV)] = uav;
		m_spaceStarts[stage][static_cast<uint32_t>(DescriptorType::SAMPLER)] = sampler;
	}
}

template<typename Base>
uint32_t EZ::CommandList_Impl<Base>::spaceStart(Shader::Stage stage, DescriptorType type) const
{
	return m_spaceStarts[stage][static_cast<uint32_t>(type)];
}

template<typename Base>
void EZ::CommandList_Impl<Base>::flushClears(const Resource* pResource)
{
	auto needClearDSVs = false;
	for (const auto& args : m_clearDSVs)
	{
		if (args.DepthStencilView->pResource == pResource)
		{
			needClearDSVs = true;
			break;
		}
	}

	auto needClearRTVs = false;
	for (const auto& args : m_clearRTVs)
	{
		if (args.RenderTargetView->pResource == pResource)
		{
			needClearRTVs = true;
			break;
		}
	}

	if (needClearDSVs || needClearRTVs)
	{
		Base::Barrier(static_cast<uint32_t>(m_barriers.size()), m_barriers.data());
		m_barriers.clear();

		if (needClearDSVs) clearDSVs();
		if (needClearRTVs) clearRTVs();
	}
}

template<typename Base>
void EZ::CommandList_Impl<Base>::dispatchBlit(const Blob& shader, SamplerPreset sampler, const ResourceView& srv,
	const ResourceView& uav, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
{
	flushClears(srv.View->pResource);
	setBarriers(1, &srv);
	setBarriers(1, &uav);
	Base::Barrier(static_cast<uint32_t>(m_barriers.size()), m_barriers.data());
	m_barriers.clear();

	const auto samplerTable = Util::DescriptorTable::MakeUnique();
	samplerTable->SetSamplers(0, 1, &sampler, m_descriptorTableLib.get());
	const auto srvTable = Util::DescriptorTable::MakeUnique();
	srvTable->SetDescriptors(0, 1, &srv.View);
	const auto uavTable = Util::DescriptorTable::MakeUnique();
	uavTable->SetDescriptors(0, 1, &uav.View);

	Base::SetComputePipelineLayout(m_blitPipelineLayout);
	assert(m_computeState);
	m_computeState->SetPipelineLayout(m_blitPipelineLayout);
	m_computeState->SetShader(shader);
	const auto pipeline = m_computeState->GetPipeline(m_computePipelineLib.get());
	if (pipeline)
	{
		Base::SetPipelineState(pipeline);
		m_pipeline = pipeline;
	}

	Base::SetComputeDescriptorTable(0, samplerTable->GetSamplerTable(m_descriptorTableLib.get()));
	Base::SetComputeDescriptorTable(1, srvTable->GetCbvSrvUavTable(m_descriptorTableLib.get()));
	Base::SetComputeDescriptorTable(2, uavTable->GetCbvSrvUavTable(m_descriptorTableLib.get()));
	Base::Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);

	Base::SetComputePipelineLayout(m_pipelineLayouts[COMPUTE]);
	m_computeState->SetPipelineLayout(m_pipelineLayouts[COMPUTE]);
	m_isComputeDirty = true;
	m_pipeline = nullptr;
}

template<typename Base>
void EZ::CommandList_Impl<Base>::predraw()
{
	// Set barrier command
	Base::Barrier(static_cast<uint32_t>(m_barriers.size()), m_barriers.data());
	m_barriers.clear();

	// Clear DSVs, RTVs, and UAVs
	clearDSVs();
	clearRTVs();
	clearUAVs();

	// Create pipeline for dynamic states
	assert(m_graphicsState);
	if (m_isGraphicsDirty)
	{
		// Set pipeline layout
		Base::SetGraphicsPipelineLayout(m_pipelineLayouts[GRAPHICS]);

		m_graphicsState->SetPipelineLayout(m_pipelineLayouts[GRAPHICS]);
		const auto pipeline = m_graphicsState->GetPipeline(m_graphicsPipelineLib.get());
		if (pipeline)
		{
			if (m_pipeline != pipeline)
			{
				Base::SetPipelineState(pipeline);
				m_pipeline = pipeline;
			}
			m_isGraphicsDirty = false;
		}
	}

	// Set descriptor tables
	for (uint8_t i = 0; i < Shader::Stage::NUM_GRAPHICS; ++i)
	{
		// Create and set sampler table
		auto& samplerTable = m_samplerTables[i];
		if (samplerTable)
		{
			const auto descriptorTable = samplerTable->GetSamplerTable(m_descriptorTableLib.get());
			if (descriptorTable) Base::SetGraphicsDescriptorTable(i, descriptorTable);
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
					if (descriptorTable) Base::SetGraphicsDescriptorTable(
						m_graphicsSpaceToParamIndexMap[i][t][s], descriptorTable);
					cbvSrvUavTable.reset();
				}
			}
		}
	}

	for (const auto& set : m_graphicsSetDescriptorTables)
	{
		const auto spaceIndex = set.Space - spaceStart(set.Stage, set.Type);
		const auto& paramIdx = m_graphicsSpaceToParamIndexMap[set.Stage][static_cast<uint32_t>(set.Type)][spaceIndex];
		Base::SetGraphicsDescriptorTable(paramIdx, set.Table);
	}
	m_graphicsSetDescriptorTables.clear();

	for (const auto& set : m_graphicsSetSingle32BitConstants)
	{
		const auto& paramIdx = m_graphicsConstantParamIndices[set.Stage];
		Base::SetGraphics32BitConstant(paramIdx, set.SrcData, set.Offset);
	}
	m_graphicsSetSingle32BitConstants.clear();

	for (const auto& set : m_graphicsSet32BitConstants)
	{
		const auto& paramIdx = m_graphicsConstantParamIndices[set.Stage];
		Base::SetGraphics32BitConstants(paramIdx, set.Num32BitValues, set.pSrcData, set.Offset);
	}
	m_graphicsSet32BitConstants.clear();
}

template<typename Base>
void EZ::CommandList_Impl<Base>::predispatch()
{
	// Set barrier command
	Base::Barrier(static_cast<uint32_t>(m_barriers.size()), m_barriers.data());
	m_barriers.clear();

	// Clear UAVs
	clearUAVs();

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
				Base::SetPipelineState(pipeline);
				m_pipeline = pipeline;
			}
			m_isComputeDirty = false;
		}
	}

	// Create and set sampler table
	auto& samplerTable = m_samplerTables[Shader::Stage::CS];
	if (samplerTable)
	{
		const auto descriptorTable = samplerTable->GetSamplerTable(m_descriptorTableLib.get());
		if (descriptorTable) Base::SetComputeDescriptorTable(0, descriptorTable);
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
				if (descriptorTable) Base::SetComputeDescriptorTable(
					m_computeSpaceToParamIndexMap[t][s], descriptorTable);
				cbvSrvUavTable.reset();
			}
		}
	}
}

template<typename Base>
void EZ::CommandList_Impl<Base>::preexecuteIndirect(Resource* pArgumentBuffer, Resource* pCountBuffer)
{
	assert(pArgumentBuffer);
	auto numBarriers = static_cast<uint32_t>(m_barriers.size());
	m_barriers.resize(numBarriers + 2);
	numBarriers = pArgumentBuffer->SetBarrier(m_barriers.data(), ResourceState::INDIRECT_ARGUMENT, numBarriers);
	if (pCountBuffer) numBarriers = pArgumentBuffer->SetBarrier(m_barriers.data(), ResourceState::INDIRECT_ARGUMENT, numBarriers);

	// Shrink the size of barrier list
	if (numBarriers < m_barriers.size()) m_barriers.resize(numBarriers);
}

template<typename Base>
void EZ::CommandList_Impl<Base>::clearDSVs()
{
	for (const auto& args : m_clearDSVs)
		Base::ClearDepthStencilView(args.DepthStencilView, args.ClearFlags,
			args.Depth, args.Stencil, static_cast<uint32_t>(args.Rects.size()), args.Rects.data());
	m_clearDSVs.clear();
}

template<typename Base>
void EZ::CommandList_Impl<Base>::clearRTVs()
{
	for (const auto& args : m_clearRTVs)
		Base::ClearRenderTargetView(args.RenderTargetView,
			args.ColorRGBA, static_cast<uint32_t>(args.Rects.size()), args.Rects.data());
	m_clearRTVs.clear();
}

template<typename Base>
void EZ::CommandList_Impl<Base>::clearUAVs()
{
	for (const auto& args : m_clearUAVsUint)
		Base::ClearUnorderedAccessViewUint(args.UAVTable, args.pUAV->View,
			args.Values, static_cast<uint32_t>(args.Rects.size()), args.Rects.data());

	for (const auto& args : m_clearUAVsFloat)
		Base::ClearUnorderedAccessViewFloat(args.UAVTable, args.pUAV->View,
			args.Values, static_cast<uint32_t>(args.Rects.size()), args.Rects.data());

	// Set barrier and barrier command
	if (!m_clearUAVs.empty())
	{
		setBarriers(static_cast<uint32_t>(m_clearUAVs.size()), m_clearUAVs.data());
		Base::Barrier(static_cast<uint32_t>(m_barriers.size()), m_barriers.data());
		m_barriers.clear();
	}

	m_clearUAVsUint.clear();
	m_clearUAVsFloat.clear();
	m_clearUAVs.clear();
}

template<typename Base>
void EZ::CommandList_Impl<Base>::setBarriers(uint32_t numResources, const ResourceView* pResourceViews)
{
	// Estimate barrier count
	auto numBarriersEst = 0u;
	for (auto i = 0u; i < numResources; ++i)
	{
		numBarriersEst += static_cast<uint32_t>(pResourceViews[i].Subresources.size());
		numBarriersEst = pResourceViews[i].View->pCounterResource ? numBarriersEst + 1 : numBarriersEst;
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

template<typename Base>
uint32_t EZ::CommandList_Impl<Base>::generateBarriers(ResourceBarrier* pBarriers,
	const ResourceView& resrouceView, uint32_t numBarriers, BarrierFlag flags)
{
	assert(resrouceView.View->pResource || resrouceView.Subresources.empty());
	for (const auto& subresource : resrouceView.Subresources)
		numBarriers = resrouceView.View->pResource->SetBarrier(pBarriers,
			resrouceView.DstState, numBarriers, subresource, flags);

	if (resrouceView.View->pCounterResource)
	{
		assert(resrouceView.DstState == ResourceState::UNORDERED_ACCESS);
		numBarriers = resrouceView.View->pCounterResource->SetBarrier(pBarriers,
			ResourceState::UNORDERED_ACCESS, numBarriers, XUSG_BARRIER_ALL_SUBRESOURCES, flags);
	}

	return numBarriers;
}

bool EZ::CommandList_DX12::adopt(XUSG::CommandList* pCommandList)
{
	const auto pCommandListDX12 = AsDX12(pCommandList);
	m_pDevice = pCommandList->GetDevice();
	m_commandList = pCommandListDX12->GetGraphicsCommandList();
	m_rtvHeap = pCommandListDX12->GetRtvHeap();
	m_dsvHeap = pCommandListDX12->GetDsvHeap();
	m_uavHeap = pCommandListDX12->GetUavHeap();

	const auto pDxDevice = static_cast<ID3D12Device*>(m_pDevice->GetNativeHandle());
	m_rtvStride = pDxDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_dsvStride = pDxDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	return true;
}

#ifndef XUSG_NO_VULKAN
bool EZ::CommandList_VK::adopt(XUSG::CommandList* pCommandList)
{
	if (pCommandList != static_cast<XUSG::CommandList*>(this))
		Attach(*AsVK(pCommandList));

	return m_pDevice != nullptr;
}
#endif

template class EZ::CommandList_Impl<XUSG::CommandList_DX12>;
#ifndef XUSG_NO_VULKAN
template class EZ::CommandList_Impl<XUSG::CommandList_VK>;
#endif
