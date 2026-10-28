//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSGCommand_DX12.h"
#include "Core/XUSGPipelineLayout_DX12.h"
#include "Core/XUSGResource_DX12.h"
#include "Ultimate/XUSGUltimate_DX12.h"
#include "RayTracing/XUSGRayTracingCommand_DX12.h"
#include "RayTracing/XUSGRayTracingState_DX12.h"
#ifndef XUSG_NO_VULKAN
#include "Core/XUSGCommand_VK.h"
#include "Ultimate/XUSGUltimate_VK.h"
#include "RayTracing/XUSGRayTracingCommand_VK.h"
#endif
#include "XUSG-EZ_Impl.h"
#include "XUSGUltimate-EZ_Impl.h"
#include "XUSGRayTracing-EZ_Impl.h"

using namespace std;
using namespace XUSG;
using namespace XUSG::RayTracing;

template<typename CoreList, typename UltimateList, typename RTList>
XUSG::RayTracing::EZ::CommandList_Impl<CoreList, UltimateList, RTList>::CommandList_Impl() :
	UltimateList(),
	Ultimate::EZ::CommandList_Impl<CoreList, UltimateList>(),
	m_rayTracingPipelineLib(nullptr),
	m_rayTracingState(nullptr),
	m_scratchSize(0),
	m_isRTStateDirty(false),
	m_scratches(0),
	m_tlasBindingToParamIndexMap(0),
	m_pHitGroup(nullptr)
{
}

template<typename CoreList, typename UltimateList, typename RTList>
XUSG::RayTracing::EZ::CommandList_Impl<CoreList, UltimateList, RTList>::CommandList_Impl(RayTracing::CommandList* pCommandList,
	uint32_t samplerHeapSize, uint32_t cbvSrvUavHeapSize) :
	CommandList_Impl()
{
	Create(pCommandList, samplerHeapSize, cbvSrvUavHeapSize);
}

template<typename CoreList, typename UltimateList, typename RTList>
XUSG::RayTracing::EZ::CommandList_Impl<CoreList, UltimateList, RTList>::~CommandList_Impl()
{
}

template<typename CoreList, typename UltimateList, typename RTList>
bool XUSG::RayTracing::EZ::CommandList_Impl<CoreList, UltimateList, RTList>::Create(RayTracing::CommandList* pCommandList, uint32_t samplerHeapSize, uint32_t cbvSrvUavHeapSize)
{
	XUSG_N_RETURN((Ultimate::EZ::CommandList_Impl<CoreList, UltimateList>::init(pCommandList, samplerHeapSize, cbvSrvUavHeapSize)), false);
	XUSG_N_RETURN(bindRayTracing(pCommandList), false);

	m_rayTracingPipelineLib = PipelineLib::MakeUnique(m_pDeviceRT);
	m_rayTracingState = State::MakeUnique();

	return true;
}

template<typename CoreList, typename UltimateList, typename RTList>
bool XUSG::RayTracing::EZ::CommandList_Impl<CoreList, UltimateList, RTList>::Create(RayTracing::Device* pDevice, void* pNativeHandle,
	uint32_t samplerHeapSize, uint32_t cbvSrvUavHeapSize)
{
	m_pDeviceRT = pDevice;
	RTList::Create(pNativeHandle, pDevice);

	return Create(this, samplerHeapSize, cbvSrvUavHeapSize);
}

template<typename CoreList, typename UltimateList, typename RTList>
bool XUSG::RayTracing::EZ::CommandList_Impl<CoreList, UltimateList, RTList>::CreatePipelineLayouts(
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
	uint32_t maxTLASSrvs, uint32_t spaceTLAS,
	uint32_t slotExt, uint32_t spaceExt)
{
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
		maxCbvSpaces ? maxCbvSpaces[Shader::Stage::CS] : 1,
		maxSrvSpaces ? maxSrvSpaces[Shader::Stage::CS] : 1,
		maxUavSpaces ? maxUavSpaces[Shader::Stage::CS] : 1,
		max32BitConstants ? max32BitConstants[Shader::Stage::CS] : 0,
		constantSlots ? constantSlots[Shader::Stage::CS] : 0,
		constantSpaces ? constantSpaces[Shader::Stage::CS] :
		spaceStart(Shader::Stage::CS, DescriptorType::CBV) + cMaxCbvSpaces - 1,
		maxTLASSrvs, spaceTLAS, slotExt, spaceExt), false);

	if (supportsMeshShader())
		XUSG_N_RETURN(createMeshShaderPipelineLayouts(maxSamplers, pMaxCbvsEachSpace, pMaxSrvsEachSpace,
			pMaxUavsEachSpace, maxCbvSpaces, maxSrvSpaces, maxUavSpaces, max32BitConstants,
			constantSlots, constantSpaces, slotExt, spaceExt), false);

	return true;
}

template<typename CoreList, typename UltimateList, typename RTList>
bool XUSG::RayTracing::EZ::CommandList_Impl<CoreList, UltimateList, RTList>::Reset(uint8_t allocatorIndex, const Pipeline& initialState)
{
	if (!m_scratches.empty())
	{
		auto scratch = move(m_scratches.back());
		m_scratches.resize(1);
		m_scratches[0] = move(scratch);
	}

	m_uploaders.clear();

	return XUSG::EZ::CommandList_Impl<CoreList>::Reset(allocatorIndex, initialState);
}

template<typename CoreList, typename UltimateList, typename RTList>
bool XUSG::RayTracing::EZ::CommandList_Impl<CoreList, UltimateList, RTList>::PrebuildBLAS(BottomLevelAS* pBLAS, uint32_t numGeometries,
	const GeometryBuffer& geometries, BuildFlag flags)
{
	assert(pBLAS);
	XUSG_N_RETURN(pBLAS->Prebuild(m_pDeviceRT, numGeometries, geometries, flags), false);

	m_scratchSize = (max)(m_scratchSize, pBLAS->GetScratchDataByteSize());
	if ((flags & BuildFlag::ALLOW_UPDATE) == BuildFlag::ALLOW_UPDATE)
		m_scratchSize = (max)(m_scratchSize, pBLAS->GetUpdateScratchDataByteSize());

	return true;
}

template<typename CoreList, typename UltimateList, typename RTList>
bool XUSG::RayTracing::EZ::CommandList_Impl<CoreList, UltimateList, RTList>::PrebuildTLAS(TopLevelAS* pTLAS, uint32_t numInstances, BuildFlag flags)
{
	assert(pTLAS);
	XUSG_N_RETURN(pTLAS->Prebuild(m_pDeviceRT, numInstances, flags), false);

	m_scratchSize = (max)(m_scratchSize, pTLAS->GetScratchDataByteSize());
	if ((flags & BuildFlag::ALLOW_UPDATE) == BuildFlag::ALLOW_UPDATE)
		m_scratchSize = (max)(m_scratchSize, pTLAS->GetUpdateScratchDataByteSize());

	return true;
}

template<typename CoreList, typename UltimateList, typename RTList>
bool XUSG::RayTracing::EZ::CommandList_Impl<CoreList, UltimateList, RTList>::PrebuildOmmArray(OpacityMicromapArray* pOmmArray, uint32_t numOpacityMicromaps,
	const GeometryBuffer& ommArrayDescs, BuildFlag flags)
{
	assert(pOmmArray);
	XUSG_N_RETURN(pOmmArray->Prebuild(m_pDeviceRT, numOpacityMicromaps, ommArrayDescs, flags), false);

	m_scratchSize = (max)(m_scratchSize, pOmmArray->GetScratchDataByteSize());
	if ((flags & BuildFlag::ALLOW_UPDATE) == BuildFlag::ALLOW_UPDATE)
		m_scratchSize = (max)(m_scratchSize, pOmmArray->GetUpdateScratchDataByteSize());

	return true;
}

template<typename CoreList, typename UltimateList, typename RTList>
bool XUSG::RayTracing::EZ::CommandList_Impl<CoreList, UltimateList, RTList>::AllocateAccelerationStructure(AccelerationStructure* pAccelerationStructure, size_t byteWidth)
{
	assert(pAccelerationStructure);
	return pAccelerationStructure->Allocate(m_pDeviceRT, m_descriptorTableLib.get(), byteWidth);
}

template<typename CoreList, typename UltimateList, typename RTList>
void XUSG::RayTracing::EZ::CommandList_Impl<CoreList, UltimateList, RTList>::SetTriangleGeometry(BottomLevelAS::GeometryDesc& geometry, Format vertexFormat,
	const XUSG::EZ::VertexBufferView& vbv, const XUSG::EZ::IndexBufferView* pIbv, GeometryFlag flags,
	const ResourceView* pTransform)
{
	// Set barriers if the vertex buffers or index buffers are not at the read states
	const auto startIdx = m_barriers.size();
	m_barriers.resize(startIdx + 2);
	auto numBarriers = 0u;

	const auto pBarrier = &m_barriers[startIdx];
	assert(vbv.pResource);
	if (vbv.pResource->GetResourceState() == ResourceState::COMMON)
		vbv.pResource->SetBarrier(pBarrier, ResourceState::COPY_DEST, numBarriers);
	numBarriers = vbv.pResource->SetBarrier(pBarrier, vbv.DstState, numBarriers);

	if (pIbv)
	{
		const auto pBarrier = &m_barriers[startIdx];
		if (pIbv->pResource->GetResourceState() == ResourceState::COMMON)
			pIbv->pResource->SetBarrier(pBarrier, ResourceState::COPY_DEST, numBarriers);
		numBarriers = pIbv->pResource->SetBarrier(pBarrier, pIbv->DstState, numBarriers);
	}

	// Shrink the size of barrier list
	if (numBarriers < 2) m_barriers.resize(startIdx + numBarriers);

	// Set geometry
	assert(vbv.pView);
	BottomLevelAS::SetTriangleGeometry(geometry, vertexFormat, *vbv.pView, pIbv ? pIbv->pView : nullptr, flags, pTransform);
}

template<typename CoreList, typename UltimateList, typename RTList>
void XUSG::RayTracing::EZ::CommandList_Impl<CoreList, UltimateList, RTList>::SetAABBGeometry(BottomLevelAS::GeometryDesc& geometry, const XUSG::EZ::VertexBufferView& vbv, GeometryFlag flags)
{
	// Set barriers if the vertex buffers or index buffers are not at the read states
	const auto startIdx = m_barriers.size();
	m_barriers.resize(startIdx + 1);
	auto numBarriers = 0u;

	const auto pBarrier = &m_barriers[startIdx];
	assert(vbv.pResource);
	if (vbv.pResource->GetResourceState() == ResourceState::COMMON)
		vbv.pResource->SetBarrier(pBarrier, ResourceState::COPY_DEST, numBarriers);
	numBarriers = vbv.pResource->SetBarrier(pBarrier, vbv.DstState, numBarriers);

	// Shrink the size of barrier list
	if (numBarriers < 1) m_barriers.resize(startIdx + numBarriers);

	// Set geometry
	assert(vbv.pView);
	BottomLevelAS::SetAABBGeometry(geometry, *vbv.pView, flags);
}

template<typename CoreList, typename UltimateList, typename RTList>
void XUSG::RayTracing::EZ::CommandList_Impl<CoreList, UltimateList, RTList>::SetOMMGeometry(BottomLevelAS::GeometryDesc& geometry, const BottomLevelAS::TriangleGeometry* pTriangles,
	const BottomLevelAS::OMMLinkage* pOmmLinkage, Buffer* pOpacityMicromapIndexBuffer, GeometryFlag flags)
{
	if (pOmmLinkage)
	{
		// Set barriers if the OMM linkage buffers are not at the read states
		const auto startIdx = m_barriers.size();
		m_barriers.resize(startIdx + 1);
		auto numBarriers = 0u;

		const auto pBarrier = &m_barriers[startIdx];
		assert(pOpacityMicromapIndexBuffer);
		if (pOpacityMicromapIndexBuffer->GetResourceState() == ResourceState::COMMON)
			pOpacityMicromapIndexBuffer->SetBarrier(pBarrier, ResourceState::COPY_DEST, numBarriers);
		numBarriers = pOpacityMicromapIndexBuffer->SetBarrier(pBarrier, ResourceState::NON_PIXEL_SHADER_RESOURCE, numBarriers);

		// Shrink the size of barrier list
		if (numBarriers < 1) m_barriers.resize(startIdx + numBarriers);
	}

	// Set geometry
	BottomLevelAS::SetOMMGeometry(geometry, pTriangles, pOmmLinkage, flags);
}

template<typename CoreList, typename UltimateList, typename RTList>
void XUSG::RayTracing::EZ::CommandList_Impl<CoreList, UltimateList, RTList>::SetOmmArray(GeometryBuffer& ommArrayDescs, uint32_t numOpacityMicromaps,
	const OpacityMicromapArray::Desc* pOmmArrayDescs)
{
	if (pOmmArrayDescs)
	{
		// Set barriers if the OMM input buffers are not at the read states
		const auto startIdx = m_barriers.size();
		m_barriers.resize(startIdx + numOpacityMicromaps * 2);
		auto numBarriers = 0u;

		for (auto i = 0u; i < numOpacityMicromaps; ++i)
		{
			const auto& pInput = pOmmArrayDescs[i].pInputBuffer;
			const auto pBarrier = &m_barriers[startIdx];
			assert(pInput);
			if (pInput->GetResourceState() == ResourceState::COMMON)
				pInput->SetBarrier(pBarrier, ResourceState::COPY_DEST, numBarriers);
			numBarriers = pInput->SetBarrier(pBarrier, ResourceState::NON_PIXEL_SHADER_RESOURCE, numBarriers);
		}

		for (auto i = 0u; i < numOpacityMicromaps; ++i)
		{
			const auto& pPerOmmDescs = pOmmArrayDescs[i].pPerOmmDescs;
			const auto pBarrier = &m_barriers[startIdx];
			assert(pPerOmmDescs);
			if (pPerOmmDescs->GetResourceState() != ResourceState::COMMON &&
				pPerOmmDescs->GetResourceState() != ResourceState::GENERIC_READ_RESOURCE)
				numBarriers = pPerOmmDescs->SetBarrier(pBarrier, ResourceState::NON_PIXEL_SHADER_RESOURCE, numBarriers);
		}

		// Shrink the size of barrier list
		if (numBarriers < numOpacityMicromaps * 2) m_barriers.resize(startIdx + numBarriers);
	}

	// Set OMM array
	OpacityMicromapArray::SetOmmArray(ommArrayDescs, numOpacityMicromaps, pOmmArrayDescs);
}

template<typename CoreList, typename UltimateList, typename RTList>
void XUSG::RayTracing::EZ::CommandList_Impl<CoreList, UltimateList, RTList>::SetBLASDestination(BottomLevelAS* pBLAS, const Buffer::sptr destBuffer,
	uintptr_t byteOffset, uint32_t uavIndex)
{
	pBLAS->SetDestination(m_pDeviceRT, destBuffer, byteOffset, uavIndex, m_descriptorTableLib.get());
}

template<typename CoreList, typename UltimateList, typename RTList>
void XUSG::RayTracing::EZ::CommandList_Impl<CoreList, UltimateList, RTList>::SetTLASDestination(TopLevelAS* pTLAS, const Buffer::sptr destBuffer,
	uintptr_t byteOffset, uint32_t uavIndex, uint32_t srvIndex)
{
	pTLAS->SetDestination(m_pDeviceRT, destBuffer, byteOffset, uavIndex, srvIndex, m_descriptorTableLib.get());
}

template<typename CoreList, typename UltimateList, typename RTList>
void XUSG::RayTracing::EZ::CommandList_Impl<CoreList, UltimateList, RTList>::SetOmmArrayDestination(OpacityMicromapArray* pOmmArray,
	const Buffer::sptr destBuffer, uintptr_t byteOffset) const
{
	pOmmArray->SetDestination(m_pDeviceRT, destBuffer, byteOffset);
}

template<typename CoreList, typename UltimateList, typename RTList>
void XUSG::RayTracing::EZ::CommandList_Impl<CoreList, UltimateList, RTList>::BuildBLAS(BottomLevelAS* pBLAS, BottomLevelAS* pSource,
	uint8_t numPostbuildInfoDescs, const PostbuildInfoType* pPostbuildInfoTypes)
{
	assert(pBLAS);
	const auto pScratch = needScratch(m_scratchSize);

	// Set barrier command
	CoreList::Barrier(static_cast<uint32_t>(m_barriers.size()), m_barriers.data());
	m_barriers.clear();

	pBLAS->Build(this, pScratch, pSource, numPostbuildInfoDescs, pPostbuildInfoTypes);

	ResourceBarrier barriers[2];
	auto numBarriers = pBLAS->SetBarrier(barriers);
	numBarriers = pScratch->SetBarrier(barriers, ResourceState::UNORDERED_ACCESS, numBarriers);
	CoreList::Barrier(numBarriers, barriers);
}

template<typename CoreList, typename UltimateList, typename RTList>
void XUSG::RayTracing::EZ::CommandList_Impl<CoreList, UltimateList, RTList>::BuildTLAS(TopLevelAS* pTLAS, Resource* pInstanceDescs, TopLevelAS* pSource,
	uint8_t numPostbuildInfoDescs, const PostbuildInfoType* pPostbuildInfoTypes)
{
	assert(pTLAS);
	const auto pScratch = needScratch(m_scratchSize);
	const auto descriptorHeap = m_descriptorTableLib->GetDescriptorHeap(CBV_SRV_UAV_HEAP);

	// Set barriers if the instance descs buffer is not at the read states
	const auto startIdx = m_barriers.size();
	m_barriers.resize(startIdx + 1);
	auto numBarriers = 0u;
	if (pInstanceDescs->GetResourceState() != ResourceState::COMMON &&
		pInstanceDescs->GetResourceState() != ResourceState::GENERIC_READ_RESOURCE)
		numBarriers = pInstanceDescs->SetBarrier(&m_barriers[startIdx], ResourceState::NON_PIXEL_SHADER_RESOURCE, numBarriers);

	// Shrink the size of barrier list
	if (numBarriers < 1) m_barriers.resize(startIdx + numBarriers);

	// Set barrier command
	CoreList::Barrier(static_cast<uint32_t>(m_barriers.size()), m_barriers.data());
	m_barriers.clear();

	pTLAS->Build(this, pScratch, pInstanceDescs, descriptorHeap, pSource, numPostbuildInfoDescs, pPostbuildInfoTypes);

	ResourceBarrier barriers[2];
	numBarriers = pTLAS->SetBarrier(barriers);
	numBarriers = pScratch->SetBarrier(barriers, ResourceState::UNORDERED_ACCESS, numBarriers);
	CoreList::Barrier(numBarriers, barriers);
}

template<typename CoreList, typename UltimateList, typename RTList>
void XUSG::RayTracing::EZ::CommandList_Impl<CoreList, UltimateList, RTList>::BuildOmmArray(OpacityMicromapArray* pOmmArray, OpacityMicromapArray* pSource,
	uint8_t numPostbuildInfoDescs, const PostbuildInfoType* pPostbuildInfoTypes)
{
	assert(pOmmArray);
	const auto pScratch = needScratch(m_scratchSize);

	// Set barrier command
	CoreList::Barrier(static_cast<uint32_t>(m_barriers.size()), m_barriers.data());
	m_barriers.clear();

	pOmmArray->Build(this, pScratch, pSource, numPostbuildInfoDescs, pPostbuildInfoTypes);

	ResourceBarrier barriers[2];
	auto numBarriers = pOmmArray->SetBarrier(barriers);
	numBarriers = pScratch->SetBarrier(barriers, ResourceState::UNORDERED_ACCESS, numBarriers);
	CoreList::Barrier(numBarriers, barriers);
}

template<typename CoreList, typename UltimateList, typename RTList>
void XUSG::RayTracing::EZ::CommandList_Impl<CoreList, UltimateList, RTList>::CopyRaytracingAccelerationStructure(AccelerationStructure* pDst,
	AccelerationStructure* pSrc, CopyMode mode)
{
	RTList::CopyRaytracingAccelerationStructure(pDst, pSrc, mode);

	ResourceBarrier barrier;
	CoreList::Barrier(pDst->SetBarrier(&barrier), &barrier);
}

template<typename CoreList, typename UltimateList, typename RTList>
void XUSG::RayTracing::EZ::CommandList_Impl<CoreList, UltimateList, RTList>::SetTopLevelAccelerationStructure(uint32_t binding, const TopLevelAS* pTopLevelAS) const
{
	RTList::SetTopLevelAccelerationStructure(m_tlasBindingToParamIndexMap[binding], pTopLevelAS);
}

template<typename CoreList, typename UltimateList, typename RTList>
void XUSG::RayTracing::EZ::CommandList_Impl<CoreList, UltimateList, RTList>::RTSetShaderLibrary(uint32_t index, const Blob& shaderLib, uint32_t numShaders, const wchar_t** pShaderNames)
{
	assert(m_rayTracingState);
	m_rayTracingState->SetShaderLibrary(index, shaderLib, numShaders, pShaderNames);
	m_isRTStateDirty = true;
}

template<typename CoreList, typename UltimateList, typename RTList>
void XUSG::RayTracing::EZ::CommandList_Impl<CoreList, UltimateList, RTList>::RTSetShaderConfig(uint32_t maxPayloadSize, uint32_t maxAttributeSize)
{
	assert(m_rayTracingState);
	m_rayTracingState->SetShaderConfig(maxPayloadSize, maxAttributeSize);
	m_isRTStateDirty = true;
}

template<typename CoreList, typename UltimateList, typename RTList>
void XUSG::RayTracing::EZ::CommandList_Impl<CoreList, UltimateList, RTList>::RTSetHitGroup(uint32_t index, const wchar_t* hitGroupName, const wchar_t* closestHitShaderName,
	const wchar_t* anyHitShaderName, const wchar_t* intersectionShaderName, HitGroupType type)
{
	assert(m_rayTracingState);
	m_rayTracingState->SetHitGroup(index, hitGroupName, closestHitShaderName, anyHitShaderName, intersectionShaderName, type);
	m_isRTStateDirty = true;
}

template<typename CoreList, typename UltimateList, typename RTList>
void XUSG::RayTracing::EZ::CommandList_Impl<CoreList, UltimateList, RTList>::RTSetMaxRecursionDepth(uint32_t depth)
{
	assert(m_rayTracingState);
	m_rayTracingState->SetMaxRecursionDepth(depth);
	m_isRTStateDirty = true;
}

template<typename CoreList, typename UltimateList, typename RTList>
void XUSG::RayTracing::EZ::CommandList_Impl<CoreList, UltimateList, RTList>::RTSetPipelineState(const Pipeline& pipelineState, RayTracing::State* pState)
{
	assert(pipelineState || pState);
	Pipeline pipeline;

	if (pState)
	{
		pipeline = pState->GetPipeline(m_rayTracingPipelineLib.get());
		if (pipelineState && pipeline != pipelineState)
		{
			pipeline = pipelineState;
			m_rayTracingPipelineLib->SetPipeline(pState, pipeline);
		}
	}
	else pipeline = pipelineState;

	if (pipeline)
	{
		if (m_pipeline != pipeline)
		{
			CoreList::SetPipelineState(pipeline);
			m_pipeline = pipeline;
		}
		m_isComputeDirty = false;
	}
}

template<typename CoreList, typename UltimateList, typename RTList>
void XUSG::RayTracing::EZ::CommandList_Impl<CoreList, UltimateList, RTList>::DispatchRays(uint32_t width, uint32_t height, uint32_t depth,
	const wchar_t* rayGenShaderName, const wchar_t* const* pMissShaderNames, uint32_t numMissShaders,
	const wchar_t* const* pCallableShaderNames, uint32_t numCallableShaders)
{
	CShaderTablePtr pRayGen = nullptr;
	CShaderTablePtr pHitGroup = nullptr;
	CShaderTablePtr pMiss = nullptr;
	CShaderTablePtr pCallable = nullptr;

	predispatchRays(pRayGen, pHitGroup, pMiss, pCallable, rayGenShaderName, pMissShaderNames, numMissShaders,
		pCallableShaderNames, numCallableShaders);
	RTList::DispatchRays(width, height, depth, pRayGen, pHitGroup, pMiss, pCallable);
}

template<typename CoreList, typename UltimateList, typename RTList>
void XUSG::RayTracing::EZ::CommandList_Impl<CoreList, UltimateList, RTList>::DispatchRaysIndirect(CommandLayout* pCommandlayout, uint32_t maxCommandCount,
	const wchar_t* rayGenShaderName, const wchar_t* const* pMissShaderNames, uint32_t numMissShaders,
	const wchar_t* const* pCallableShaderNames, uint32_t numCallableShaders,
	Resource* pArgumentBuffer, uint64_t argumentBufferOffset, Resource* pCountBuffer, uint64_t countBufferOffset)
{
	CShaderTablePtr pRayGen = nullptr;
	CShaderTablePtr pHitGroup = nullptr;
	CShaderTablePtr pMiss = nullptr;
	CShaderTablePtr pCallable = nullptr;

	preexecuteIndirect(pArgumentBuffer, pCountBuffer);
	predispatchRays(pRayGen, pHitGroup, pMiss, pCallable, rayGenShaderName, pMissShaderNames, numMissShaders,
		pCallableShaderNames, numCallableShaders);

	// Populate arg-buffer header with shader tables
	string key(sizeof(ArgumentBufferVA), '\0');
	auto& argumentBufferVA = reinterpret_cast<ArgumentBufferVA&>(key[0]);
	argumentBufferVA = { pArgumentBuffer, argumentBufferOffset };
	const auto iter = m_argumentBufferVAs.find(key);
	if (iter == m_argumentBufferVAs.cend())
	{
		// Need to copy buffer
		m_uploaders.emplace_back();
		auto& uploader = m_uploaders.back();

		uploader = Buffer::MakeUnique();
		const auto result = uploader->Create(m_pDevice, sizeof(DX12DispatchRaysDescHeader),
			ResourceFlag::DENY_SHADER_RESOURCE, MemoryType::UPLOAD, 0, nullptr,
			0, nullptr, MemoryFlag::NONE);
		assert(result);

		const auto pDispatchDescHeader = static_cast<DX12DispatchRaysDescHeader*>(uploader->Map());
		pDispatchDescHeader->RayGenerationShaderRecord.StartAddress = pRayGen ? pRayGen->GetVirtualAddress() : 0;
		pDispatchDescHeader->RayGenerationShaderRecord.SizeInBytes = pRayGen ? pRayGen->GetByteSize() : 0;
		pDispatchDescHeader->HitGroupTable.StartAddress = pHitGroup ? pHitGroup->GetVirtualAddress() : 0;
		pDispatchDescHeader->HitGroupTable.SizeInBytes = pHitGroup ? pHitGroup->GetByteSize() : 0;
		pDispatchDescHeader->HitGroupTable.StrideInBytes = pHitGroup ? pHitGroup->GetByteStride() : 0;
		pDispatchDescHeader->MissShaderTable.StartAddress = pMiss ? pMiss->GetVirtualAddress() : 0;
		pDispatchDescHeader->MissShaderTable.SizeInBytes = pMiss ? pMiss->GetByteSize() : 0;
		pDispatchDescHeader->MissShaderTable.StrideInBytes = pMiss ? pMiss->GetByteStride() : 0;
		pDispatchDescHeader->CallableShaderTable.StartAddress = pCallable ? pCallable->GetVirtualAddress() : 0;
		pDispatchDescHeader->CallableShaderTable.SizeInBytes = pCallable ? pCallable->GetByteSize() : 0;
		pDispatchDescHeader->CallableShaderTable.StrideInBytes = pCallable ? pCallable->GetByteStride() : 0;
		uploader->Unmap();

		CopyBufferRegion(pArgumentBuffer, argumentBufferOffset, uploader.get(), 0, sizeof(DX12DispatchRaysDescHeader));
	}
	
	CoreList::ExecuteIndirect(pCommandlayout, maxCommandCount,
		pArgumentBuffer, argumentBufferOffset, pCountBuffer, countBufferOffset);
}

template<typename CoreList, typename UltimateList, typename RTList>
size_t XUSG::RayTracing::EZ::CommandList_Impl<CoreList, UltimateList, RTList>::GetDispatchRaysArgReservedOffset() const
{
	return sizeof(DX12DispatchRaysDescHeader);
}

template<typename CoreList, typename UltimateList, typename RTList>
RayTracing::Device* XUSG::RayTracing::EZ::CommandList_Impl<CoreList, UltimateList, RTList>::GetRTDevice() const
{
	return RTList::GetRTDevice();
}

template<typename CoreList, typename UltimateList, typename RTList>
RayTracing::CommandList* XUSG::RayTracing::EZ::CommandList_Impl<CoreList, UltimateList, RTList>::AsRTCommandList()
{
	return static_cast<RayTracing::CommandList*>(static_cast<RTList*>(this));
}

template<typename CoreList, typename UltimateList, typename RTList>
bool XUSG::RayTracing::EZ::CommandList_Impl<CoreList, UltimateList, RTList>::createComputePipelineLayouts(uint32_t maxSamplers, const uint32_t* pMaxCbvsEachSpace,
	const uint32_t* pMaxSrvsEachSpace, const uint32_t* pMaxUavsEachSpace,
	uint32_t maxCbvSpaces, uint32_t maxSrvSpaces, uint32_t maxUavSpaces,
	uint32_t max32BitConstants, uint32_t constantSlot, uint32_t constantSpace,
	uint32_t maxTLASSrvs, uint32_t spaceTLAS, uint32_t slotExt, uint32_t spaceExt)
{
	// Create common compute pipeline layout with ray tracing
	auto paramIndex = 0u;
	const auto pipelineLayout = PipelineLayout::MakeUnique();
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
	m_tlasBindingToParamIndexMap.resize(maxTLASSrvs);

	const auto cbvSpaceStart = spaceStart(Shader::Stage::CS, DescriptorType::CBV);
	const auto srvSpaceStart = spaceStart(Shader::Stage::CS, DescriptorType::SRV);
	const auto uavSpaceStart = spaceStart(Shader::Stage::CS, DescriptorType::UAV);
	pipelineLayout->SetRange(paramIndex++, DescriptorType::SAMPLER, maxSamplers, 0,
		spaceStart(Shader::Stage::CS, DescriptorType::SAMPLER), DescriptorFlag::DESCRIPTORS_VOLATILE);

	for (auto s = 0u; s < maxSpaces; ++s)
	{
		if (s < maxCbvSpaces)
		{
			auto maxDescriptors = pMaxCbvsEachSpace ? pMaxCbvsEachSpace[s] : 12;
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

		if (s < maxSrvSpaces && !(srvSpaceStart + s == spaceTLAS && maxTLASSrvs > 0))
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

	for (auto slot = 0u; slot < maxTLASSrvs; ++slot)
	{
		m_tlasBindingToParamIndexMap[slot] = paramIndex;
		pipelineLayout->SetRootSRV(paramIndex++, slot, spaceTLAS, DescriptorFlag::DATA_STATIC);
	}

	pipelineLayout->SetRange(paramIndex++, DescriptorType::UAV, 1, slotExt, spaceExt);

	XUSG_X_RETURN(m_pipelineLayouts[COMPUTE], pipelineLayout->GetPipelineLayout(m_pDeviceRT, m_pipelineLayoutLib.get(),
		PipelineLayoutFlag::NONE, L"RayTracingEZComputeLayout"), false);

	return true;
}

template<typename CoreList, typename UltimateList, typename RTList>
void XUSG::RayTracing::EZ::CommandList_Impl<CoreList, UltimateList, RTList>::predispatchRays(CShaderTablePtr& pRayGen, CShaderTablePtr& pHitGroup, CShaderTablePtr& pMiss, CShaderTablePtr& pCallable,
	const wchar_t* rayGenShaderName, const wchar_t* const* pMissShaderNames, uint32_t numMissShaders,
	const wchar_t* const* pCallableShaderNames, uint32_t numCallableShaders)
{
	// Set barrier command
	CoreList::Barrier(static_cast<uint32_t>(m_barriers.size()), m_barriers.data());
	m_barriers.clear();

	// Clear UAVs
	clearUAVs();

	// Create pipeline for dynamic states
	assert(m_rayTracingState);
	if (m_isRTStateDirty)
	{
		m_rayTracingState->SetGlobalPipelineLayout(m_pipelineLayouts[COMPUTE]);
		const auto pipeline = m_rayTracingState->GetPipeline(m_rayTracingPipelineLib.get());
		if (pipeline)
		{
			if (m_pipeline != pipeline)
			{
				RTList::SetRayTracingPipeline(pipeline);
				m_pipeline = pipeline;
			}

			// Get hit-group table; the hit-group has been bound on the pipeline
			const auto numHitGroups = m_rayTracingState->GetNumHitGroups();
			const auto numRecords = numHitGroups ? numHitGroups : 1u;
			string key(sizeof(void*) * numRecords, '\0');
			const auto pShaderIDs = reinterpret_cast<const void**>(&key[0]);
			for (auto i = 0u; i < numHitGroups; ++i) // Set hit-group shader IDs into the key
			{
				const auto hitGroupName = m_rayTracingState->GetHitGroupName(i);
				if (hitGroupName)
					pShaderIDs[i] = ShaderRecord::GetShaderIdentifier(pipeline, hitGroupName);
			}
			m_pHitGroup = getShaderTable(key, m_hitGroupTables, numRecords);
			// Different from other types of state objects, m_rayTracingState cannot be reused
			// for the next different pipeline, so we need to remake it
			m_rayTracingState = State::MakeUnique();
			m_isRTStateDirty = false;
		}
	}

	pHitGroup = m_pHitGroup;

	// Get ray-generation shader table; the ray-generation shader is independent of the pipeline
	if (rayGenShaderName)
	{
		string key(sizeof(void*), '\0');
		auto& shaderID = reinterpret_cast<const void*&>(key[0]);
		shaderID = ShaderRecord::GetShaderIdentifier(m_pipeline, rayGenShaderName);
		pRayGen = getShaderTable(key, m_rayGenTables, 1);
	}

	// Get miss shader tables; the miss shaders are independent of the pipeline
	if (numMissShaders)
	{
		string key(sizeof(void*) * numMissShaders, '\0');
		const auto pShaderIDs = reinterpret_cast<const void**>(&key[0]);
		for (auto i = 0u; i < numMissShaders; ++i) // Set miss shader IDs into the key
		{
			if (pMissShaderNames && pMissShaderNames[i])
				pShaderIDs[i] = ShaderRecord::GetShaderIdentifier(m_pipeline, pMissShaderNames[i]);
		}
		pMiss = getShaderTable(key, m_missTables, numMissShaders);
	}

	// Get callable shader tables; the callable shaders are independent of the pipeline
	if (numCallableShaders)
	{
		string key(sizeof(void*) * numCallableShaders, '\0');
		const auto pShaderIDs = reinterpret_cast<const void**>(&key[0]);
		for (auto i = 0u; i < numCallableShaders; ++i) // Set callable shader IDs into the key
		{
			if (pCallableShaderNames && pCallableShaderNames[i])
				pShaderIDs[i] = ShaderRecord::GetShaderIdentifier(m_pipeline, pCallableShaderNames[i]);
		}
		pCallable = getShaderTable(key, m_callableTables, numCallableShaders);
	}

	// Create and set sampler table
	auto& samplerTable = m_samplerTables[COMPUTE];
	if (samplerTable)
	{
		const auto descriptorTable = samplerTable->GetSamplerTable(m_descriptorTableLib.get());
		if (descriptorTable) CoreList::SetComputeDescriptorTable(0, descriptorTable);
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
				if (descriptorTable) CoreList::SetComputeDescriptorTable(
					m_computeSpaceToParamIndexMap[t][s], descriptorTable);
				cbvSrvUavTable.reset();
			}
		}
	}
}

template<typename CoreList, typename UltimateList, typename RTList>
XUSG::Resource* XUSG::RayTracing::EZ::CommandList_Impl<CoreList, UltimateList, RTList>::needScratch(size_t size)
{
	if (m_scratches.empty() || !m_scratches.back() || m_scratches.back()->GetWidth() < size)
	{
		m_scratches.emplace_back(Buffer::MakeUnique());
		AccelerationStructure::AllocateUAVBuffer(m_pDeviceRT, m_scratches.back().get(), size);
	}

	return m_scratches.back().get();
}

template<typename CoreList, typename UltimateList, typename RTList>
const ShaderTable* XUSG::RayTracing::EZ::CommandList_Impl<CoreList, UltimateList, RTList>::getShaderTable(const string& key,
	unordered_map<string, ShaderTable::uptr>& shaderTables,
	uint32_t numShaderIDs)
{
	const auto shaderTablePair = shaderTables.find(key);

	if (shaderTablePair == shaderTables.cend())
	{
		const auto shaderIDSize = ShaderRecord::GetShaderIdentifierSize(m_pDeviceRT);

		auto& shaderTable = shaderTables[key];
		shaderTable = ShaderTable::MakeUnique();
		shaderTable->Create(m_pDeviceRT, numShaderIDs, shaderIDSize);

		const auto pShaderIDs = reinterpret_cast<void* const*>(&key[0]);
		for (auto i = 0u; i < numShaderIDs; ++i)
			shaderTable->AddShaderRecord(ShaderRecord::MakeUnique(pShaderIDs[i], shaderIDSize).get());

		return shaderTable.get();
	}

	return shaderTablePair->second.get();
}

bool XUSG::RayTracing::EZ::CommandList_DX12::bindRayTracing(RayTracing::CommandList* pCommandList)
{
	m_pDeviceRT = pCommandList->GetRTDevice();
	const auto pDxDevice = static_cast<ID3D12RaytracingFallbackDevice*>(m_pDeviceRT ? m_pDeviceRT->GetRTHandle() : nullptr);
	if (!pDxDevice) return false;
	pDxDevice->QueryRaytracingCommandList(m_commandList.get(), IID_PPV_ARGS(&m_commandListRT));

	return true;
}

bool XUSG::RayTracing::EZ::CommandList_DX12::supportsMeshShader() const
{
	if (!m_commandListU) return false;

	D3D12_FEATURE_DATA_D3D12_OPTIONS7 featureSupportData = {};
	const auto hr = static_cast<ID3D12Device*>(m_pDevice->GetNativeHandle())->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7,
		&featureSupportData, sizeof(featureSupportData));

	return SUCCEEDED(hr) && featureSupportData.MeshShaderTier != D3D12_MESH_SHADER_TIER_NOT_SUPPORTED;
}

#ifndef XUSG_NO_VULKAN
bool XUSG::RayTracing::EZ::CommandList_VK::bindRayTracing(RayTracing::CommandList* pCommandList)
{
	if (pCommandList != static_cast<RayTracing::CommandList*>(this))
	{
		const auto pRT = AsVK(pCommandList);
		if (pRT) AttachRayTracing(*pRT);
	}

	return createInterface();
}

bool XUSG::RayTracing::EZ::CommandList_VK::supportsMeshShader() const
{
	return true;
}
#endif

template class XUSG::RayTracing::EZ::CommandList_Impl<XUSG::CommandList_DX12, Ultimate::CommandList_DX12, RayTracing::CommandList_DX12>;
#ifndef XUSG_NO_VULKAN
template class XUSG::RayTracing::EZ::CommandList_Impl<XUSG::CommandList_VK, Ultimate::CommandList_VK, RayTracing::CommandList_VK>;
#endif
