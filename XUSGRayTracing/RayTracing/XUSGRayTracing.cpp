//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSG_DX12.h"
#include "Core/XUSGCommand_DX12.h"
#include "Core/XUSGPipelineLayout_DX12.h"
#include "XUSGRayTracing.h"
#include "XUSGRayTracing_DX12.h"
#include "XUSGAccelerationStructure_DX12.h"
#include "XUSGShaderTable_DX12.h"
#include "XUSGRayTracingCommand_DX12.h"
#include "XUSGRayTracingPipelineLayout_DX12.h"
#include "XUSGRayTracingState_DX12.h"

using namespace std;
using namespace XUSG;
using namespace XUSG::RayTracing;

#if ENABLE_DXR_FALLBACK
uint32_t g_numUAVs;
#endif
uint32_t g_frameCount = 1;

#if ENABLE_DXR_FALLBACK
uint32_t AccelerationStructure::GetUAVCount()
{
	return g_numUAVs;
}

void AccelerationStructure::SetUAVCount(uint32_t numUAVs)
{
	g_numUAVs = numUAVs;
}
#endif

void AccelerationStructure::SetFrameCount(uint32_t frameCount)
{
	g_frameCount = frameCount;
}

bool AccelerationStructure::AllocateUAVBuffer(const Device* pDevice, Resource* pResource,
	size_t byteWidth, ResourceState dstState, XUSG::API api)
{
	return AccelerationStructure_DX12::AllocateUAVBuffer(pDevice, pResource, byteWidth, dstState);
}

bool AccelerationStructure::AllocateUploadBuffer(const Device* pDevice, Resource* pResource,
	size_t byteWidth, void* pData, XUSG::API api)
{
	return AccelerationStructure_DX12::AllocateUploadBuffer(pDevice, pResource, byteWidth, pData);
}

void BottomLevelAS::SetTriangleGeometries(GeometryBuffer& geometries, uint32_t numGeometries,
	Format vertexFormat, const VertexBufferView* pVBs, const IndexBufferView* pIBs,
	const GeometryFlag* geometryFlags, const ResourceView* pTransforms, XUSG::API api)
{
	BottomLevelAS_DX12::SetTriangleGeometries(geometries, numGeometries,
		vertexFormat, pVBs, pIBs, geometryFlags, pTransforms);
}

void BottomLevelAS::SetAABBGeometries(GeometryBuffer& geometries, uint32_t numGeometries,
	const VertexBufferView* pVBs, const GeometryFlag* geometryFlags, XUSG::API api)
{
	BottomLevelAS_DX12::SetAABBGeometries(geometries, numGeometries, pVBs, geometryFlags);
}

RayTracing::Device::uptr RayTracing::Device::MakeUnique(XUSG::API api)
{
	return make_unique<RayTracing::Device_DX12>();
}

RayTracing::Device::sptr RayTracing::Device::MakeShared(XUSG::API api)
{
	return make_shared<RayTracing::Device_DX12>();
}

BottomLevelAS::uptr BottomLevelAS::MakeUnique(XUSG::API api)
{
	return make_unique<BottomLevelAS_DX12>();
}

BottomLevelAS::sptr BottomLevelAS::MakeShared(XUSG::API api)
{
	return make_shared<BottomLevelAS_DX12>();
}

void TopLevelAS::SetInstances(const Device* pDevice, Resource* pInstances,
	uint32_t numInstances, const BottomLevelAS* const* pBottomLevelASs,
	float* const* transforms, XUSG::API api)
{
	TopLevelAS_DX12::SetInstances(pDevice, pInstances, numInstances, pBottomLevelASs, transforms);
}

TopLevelAS::uptr TopLevelAS::MakeUnique(XUSG::API api)
{
	return make_unique<TopLevelAS_DX12>();
}

TopLevelAS::sptr TopLevelAS::MakeShared(XUSG::API api)
{
	return make_shared<TopLevelAS_DX12>();
}

uint32_t ShaderRecord::GetShaderIDSize(const Device* pDevice, XUSG::API api)
{
	return ShaderRecord_DX12::GetShaderIDSize(pDevice);
}

ShaderRecord::uptr ShaderRecord::MakeUnique(const Device* pDevice, const Pipeline& pipeline, const void* shader,
	const void* pLocalDescriptorArgs, uint32_t localDescriptorArgSize, XUSG::API api)
{
	return make_unique<ShaderRecord_DX12>(pDevice, pipeline, shader, pLocalDescriptorArgs, localDescriptorArgSize);
}

ShaderRecord::sptr ShaderRecord::MakeShared(const Device* pDevice, const Pipeline& pipeline, const void* shader,
	const void* pLocalDescriptorArgs, uint32_t localDescriptorArgSize, XUSG::API api)
{
	return make_shared<ShaderRecord_DX12>(pDevice, pipeline, shader, pLocalDescriptorArgs, localDescriptorArgSize);
}

ShaderRecord::uptr ShaderRecord::MakeUnique(void* pShaderID, uint32_t shaderIDSize,
	const void* pLocalDescriptorArgs, uint32_t localDescriptorArgSize, XUSG::API api)
{
	return make_unique<ShaderRecord_DX12>(pShaderID, shaderIDSize, pLocalDescriptorArgs, localDescriptorArgSize);
}

ShaderRecord::sptr ShaderRecord::MakeShared(void* pShaderID, uint32_t shaderIDSize,
	const void* pLocalDescriptorArgs, uint32_t localDescriptorArgSize, XUSG::API api)
{
	return make_shared<ShaderRecord_DX12>(pShaderID, shaderIDSize, pLocalDescriptorArgs, localDescriptorArgSize);
}

ShaderTable::uptr ShaderTable::MakeUnique(XUSG::API api)
{
	return make_unique<ShaderTable_DX12>();
}

ShaderTable::sptr ShaderTable::MakeShared(XUSG::API api)
{
	return make_shared<ShaderTable_DX12>();
}

RayTracing::CommandList::uptr RayTracing::CommandList::MakeUnique(XUSG::API api)
{
	return make_unique<RayTracing::CommandList_DX12>();
}

RayTracing::CommandList::sptr RayTracing::CommandList::MakeShared(XUSG::API api)
{
	return make_shared<RayTracing::CommandList_DX12>();
}

#if ENABLE_DXR_FALLBACK
RayTracing::CommandList::uptr RayTracing::CommandList::MakeUnique(XUSG::CommandList* pCommandList, const RayTracing::Device* pDevice, XUSG::API api)
{
	return make_unique<RayTracing::CommandList_DX12>(pCommandList, pDevice);
}

RayTracing::CommandList::sptr RayTracing::CommandList::MakeShared(XUSG::CommandList* pCommandList, const RayTracing::Device* pDevice, XUSG::API api)
{
	return make_shared<RayTracing::CommandList_DX12>(pCommandList, pDevice);
}
#else
RayTracing::CommandList::uptr RayTracing::CommandList::MakeUnique(XUSG::CommandList* pCommandList, XUSG::API api)
{
	return make_unique<RayTracing::CommandList_DX12>(pCommandList);
}

RayTracing::CommandList::sptr RayTracing::CommandList::MakeShared(XUSG::CommandList* pCommandList, XUSG::API api)
{
	return make_shared<RayTracing::CommandList_DX12>(pCommandList);
}
#endif

RayTracing::PipelineLayout::uptr RayTracing::PipelineLayout::MakeUnique(XUSG::API api)
{
	return make_unique<RayTracing::PipelineLayout_DX12>();
}

RayTracing::PipelineLayout::sptr RayTracing::PipelineLayout::MakeShared(XUSG::API api)
{
	return make_shared<RayTracing::PipelineLayout_DX12>();
}

RayTracing::State::uptr RayTracing::State::MakeUnique(XUSG::API api)
{
	return make_unique<RayTracing::State_DX12>();
}

RayTracing::State::sptr RayTracing::State::MakeShared(XUSG::API api)
{
	return make_shared<RayTracing::State_DX12>();
}

RayTracing::PipelineCache::uptr RayTracing::PipelineCache::MakeUnique(XUSG::API api)
{
	return make_unique<RayTracing::PipelineCache_DX12>();
}

RayTracing::PipelineCache::sptr RayTracing::PipelineCache::MakeShared(XUSG::API api)
{
	return make_shared<RayTracing::PipelineCache_DX12>();
}

RayTracing::PipelineCache::uptr RayTracing::PipelineCache::MakeUnique(const Device* pDevice, XUSG::API api)
{
	return make_unique<RayTracing::PipelineCache_DX12>(pDevice);
}

RayTracing::PipelineCache::sptr RayTracing::PipelineCache::MakeShared(const Device* pDevice, XUSG::API api)
{
	return make_shared<RayTracing::PipelineCache_DX12>(pDevice);
}
