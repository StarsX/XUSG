//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSG_DX12.h"
#include "Core/XUSGCommand_DX12.h"
#include "Core/XUSGResource_DX12.h"
#include "Core/XUSGPipelineLayout_DX12.h"
#include "Ultimate/XUSGUltimate_DX12.h"
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

bool AccelerationStructure::AllocateDestBuffer(const Device* pDevice, Buffer* pDestBuffer, size_t byteWidth,
	uint32_t numSRVs, const uintptr_t* firstSrvElements, uint32_t numUAVs, const uintptr_t* firstUavElements,
	MemoryFlag memoryFlags, const wchar_t* name, uint32_t maxThreads, API api)
{
	return AccelerationStructure_DX12::AllocateDestBuffer(pDevice, pDestBuffer,
		byteWidth, numSRVs, firstSrvElements, numUAVs, firstUavElements,
		memoryFlags, name, maxThreads);
}

bool AccelerationStructure::AllocateUAVBuffer(const Device* pDevice, Buffer* pBuffer, size_t byteWidth,
	ResourceState dstState, MemoryFlag memoryFlags, const wchar_t* name, uint32_t maxThreads)
{
	XUSG_N_RETURN(pBuffer->Initialize(pDevice, Format::R32_TYPELESS), false);
	XUSG_N_RETURN(pBuffer->CreateResource(byteWidth, ResourceFlag::ALLOW_UNORDERED_ACCESS,
		MemoryType::DEFAULT, memoryFlags, dstState, 0, nullptr, maxThreads), false);
	pBuffer->SetName(name);

	return true;
}

bool AccelerationStructure::AllocateUploadBuffer(const Device* pDevice, Buffer* pBuffer,
	size_t byteWidth, void* pData, MemoryFlag memoryFlags, const wchar_t* name)
{
	XUSG_N_RETURN(pBuffer->Initialize(pDevice, Format::R32_TYPELESS), false);
	XUSG_N_RETURN(pBuffer->CreateResource(byteWidth, ResourceFlag::NONE, MemoryType::UPLOAD,
		memoryFlags, ResourceState::GENERIC_READ_RESOURCE), false);
	pBuffer->SetName(name);

	void* pMappedData = pBuffer->Map();
	memcpy(pMappedData, pData, byteWidth);
	pBuffer->Unmap();

	return true;
}

uint32_t AccelerationStructure::SetBarrier(ResourceBarrier* pBarriers, Resource* pResource, uint32_t numBarriers)
{
	pBarriers[numBarriers++] = { pResource, ResourceState::UNORDERED_ACCESS, ResourceState::UNORDERED_ACCESS };

	return numBarriers;
}

size_t AccelerationStructure::Align(size_t byteSize, API api)
{
	return AccelerationStructure_DX12::Align(byteSize);
}

void BottomLevelAS::SetTriangleGeometries(GeometryBuffer& geometries, uint32_t numGeometries,
	Format vertexFormat, const VertexBufferView* pVBs, const IndexBufferView* pIBs,
	const GeometryFlag* geometryFlags, const ResourceView* pTransforms, API api)
{
	BottomLevelAS_DX12::SetTriangleGeometries(geometries, numGeometries,
		vertexFormat, pVBs, pIBs, geometryFlags, pTransforms);
}

void BottomLevelAS::SetAABBGeometries(GeometryBuffer& geometries, uint32_t numGeometries,
	const VertexBufferView* pVBs, const GeometryFlag* geometryFlags, API api)
{
	BottomLevelAS_DX12::SetAABBGeometries(geometries, numGeometries, pVBs, geometryFlags);
}

RayTracing::Device::uptr RayTracing::Device::MakeUnique(API api)
{
	return make_unique<RayTracing::Device_DX12>();
}

RayTracing::Device::sptr RayTracing::Device::MakeShared(API api)
{
	return make_shared<RayTracing::Device_DX12>();
}

BottomLevelAS::uptr BottomLevelAS::MakeUnique(API api)
{
	return make_unique<BottomLevelAS_DX12>();
}

BottomLevelAS::sptr BottomLevelAS::MakeShared(API api)
{
	return make_shared<BottomLevelAS_DX12>();
}

void TopLevelAS::SetInstances(const Device* pDevice, Buffer* pInstances, uint32_t numInstances,
	const BottomLevelAS* const* ppBottomLevelASs, const float* const* transforms,
	MemoryFlag memoryFlags, const wchar_t* instanceName, API api)
{
	assert(numInstances == 0 || (ppBottomLevelASs && transforms));

	vector<InstanceDesc> instanceDescs(numInstances);
	for (auto i = 0u; i < numInstances; ++i)
	{
		instanceDescs[i].pTransform = transforms[i];
		instanceDescs[i].InstanceMask = 1;
		instanceDescs[i].pBottomLevelAS = ppBottomLevelASs[i];
	}

	SetInstances(pDevice, pInstances, numInstances, instanceDescs.data(), memoryFlags, instanceName, api);
}

void TopLevelAS::SetInstances(const Device* pDevice, Buffer* pInstances, uint32_t numInstances,
	const InstanceDesc* pInstanceDescs, MemoryFlag memoryFlags, const wchar_t* instanceName, API api)
{
	TopLevelAS_DX12::SetInstances(pDevice, pInstances, numInstances, pInstanceDescs, memoryFlags, instanceName);
}

TopLevelAS::uptr TopLevelAS::MakeUnique(API api)
{
	return make_unique<TopLevelAS_DX12>();
}

TopLevelAS::sptr TopLevelAS::MakeShared(API api)
{
	return make_shared<TopLevelAS_DX12>();
}

const void* ShaderRecord::GetShaderIdentifier(const Pipeline& pipeline, const wchar_t* shaderName, API api)
{
	return ShaderRecord_DX12::GetShaderIdentifier(pipeline, shaderName);
}

uint32_t ShaderRecord::GetShaderIdentifierSize(const Device* pDevice, API api)
{
	return ShaderRecord_DX12::GetShaderIdentifierSize(pDevice);
}

ShaderRecord::uptr ShaderRecord::MakeUnique(void* pShaderIdentifier, uint32_t shaderIdentifierSize,
	const void* pLocalDescriptorArgs, uint32_t localDescriptorArgSize, API api)
{
	return make_unique<ShaderRecord_DX12>(pShaderIdentifier, shaderIdentifierSize, pLocalDescriptorArgs, localDescriptorArgSize);
}

ShaderRecord::sptr ShaderRecord::MakeShared(void* pShaderIdentifier, uint32_t shaderIdentifierSize,
	const void* pLocalDescriptorArgs, uint32_t localDescriptorArgSize, API api)
{
	return make_shared<ShaderRecord_DX12>(pShaderIdentifier, shaderIdentifierSize, pLocalDescriptorArgs, localDescriptorArgSize);
}

ShaderRecord::uptr ShaderRecord::MakeUnique(const Device* pDevice, const Pipeline& pipeline, const wchar_t* shaderName,
	const void* pLocalDescriptorArgs, uint32_t localDescriptorArgSize, API api)
{
	return make_unique<ShaderRecord_DX12>(pDevice, pipeline, shaderName, pLocalDescriptorArgs, localDescriptorArgSize);
}

ShaderRecord::sptr ShaderRecord::MakeShared(const Device* pDevice, const Pipeline& pipeline, const wchar_t* shaderName,
	const void* pLocalDescriptorArgs, uint32_t localDescriptorArgSize, API api)
{
	return make_shared<ShaderRecord_DX12>(pDevice, pipeline, shaderName, pLocalDescriptorArgs, localDescriptorArgSize);
}

size_t ShaderRecord::Align(uint32_t byteSize, API api)
{
	return ShaderRecord_DX12::Align(byteSize);
}

ShaderTable::uptr ShaderTable::MakeUnique(API api)
{
	return make_unique<ShaderTable_DX12>();
}

ShaderTable::sptr ShaderTable::MakeShared(API api)
{
	return make_shared<ShaderTable_DX12>();
}

size_t ShaderTable::Align(size_t byteSize, API api)
{
	return ShaderTable_DX12::Align(byteSize);
}

RayTracing::CommandList::uptr RayTracing::CommandList::MakeUnique(API api)
{
	return make_unique<RayTracing::CommandList_DX12>();
}

RayTracing::CommandList::sptr RayTracing::CommandList::MakeShared(API api)
{
	return make_shared<RayTracing::CommandList_DX12>();
}

RayTracing::CommandList::uptr RayTracing::CommandList::MakeUnique(XUSG::CommandList* pCommandList, const RayTracing::Device* pDevice, API api)
{
	return make_unique<RayTracing::CommandList_DX12>(pCommandList, pDevice);
}

RayTracing::CommandList::sptr RayTracing::CommandList::MakeShared(XUSG::CommandList* pCommandList, const RayTracing::Device* pDevice, API api)
{
	return make_shared<RayTracing::CommandList_DX12>(pCommandList, pDevice);
}

RayTracing::PipelineLayout::uptr RayTracing::PipelineLayout::MakeUnique(API api)
{
	return make_unique<RayTracing::PipelineLayout_DX12>();
}

RayTracing::PipelineLayout::sptr RayTracing::PipelineLayout::MakeShared(API api)
{
	return make_shared<RayTracing::PipelineLayout_DX12>();
}

RayTracing::State::uptr RayTracing::State::MakeUnique(API api)
{
	return make_unique<RayTracing::State_DX12>();
}

RayTracing::State::sptr RayTracing::State::MakeShared(API api)
{
	return make_shared<RayTracing::State_DX12>();
}

RayTracing::PipelineLib::uptr RayTracing::PipelineLib::MakeUnique(API api)
{
	return make_unique<RayTracing::PipelineLib_DX12>();
}

RayTracing::PipelineLib::sptr RayTracing::PipelineLib::MakeShared(API api)
{
	return make_shared<RayTracing::PipelineLib_DX12>();
}

RayTracing::PipelineLib::uptr RayTracing::PipelineLib::MakeUnique(const Device* pDevice, API api)
{
	return make_unique<RayTracing::PipelineLib_DX12>(pDevice);
}

RayTracing::PipelineLib::sptr RayTracing::PipelineLib::MakeShared(const Device* pDevice, API api)
{
	return make_shared<RayTracing::PipelineLib_DX12>(pDevice);
}
