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
#ifndef XUSG_NO_VULKAN
#include "Core/XUSG_VK.h"
#include "XUSGRayTracing_VK.h"
#include "XUSGAccelerationStructure_VK.h"
#include "XUSGShaderTable_VK.h"
#include "XUSGRayTracingCommand_VK.h"
#include "XUSGRayTracingPipelineLayout_VK.h"
#include "XUSGRayTracingState_VK.h"
#endif

using namespace std;
using namespace XUSG;
using namespace XUSG::RayTracing;

#ifdef XUSG_NO_VULKAN
#define XUSG_IF_VK(...)
#else
#define XUSG_IF_VK(...) if (XUSG::Device::GetApi() == API::VULKAN) { __VA_ARGS__ }
#endif

bool AccelerationStructure::AllocateDestBuffer(Device* pDevice, Buffer* pDestBuffer, size_t byteWidth,
	uint32_t numSRVs, const uintptr_t* firstSrvElements, uint32_t numUAVs, const uintptr_t* firstUavElements,
	MemoryFlag memoryFlags, const wchar_t* name, uint32_t maxThreads)
{
	XUSG::Device::LockApi();
	XUSG_IF_VK(return AccelerationStructure_VK::AllocateDestBuffer(pDevice, pDestBuffer,
		byteWidth, numSRVs, firstSrvElements, numUAVs, firstUavElements,
		memoryFlags, name, maxThreads);)

	return AccelerationStructure_DX12::AllocateDestBuffer(pDevice, pDestBuffer,
		byteWidth, numSRVs, firstSrvElements, numUAVs, firstUavElements,
		memoryFlags, name, maxThreads);
}

bool AccelerationStructure::AllocateUAVBuffer(Device* pDevice, Buffer* pBuffer, size_t byteWidth,
	ResourceState dstState, MemoryFlag memoryFlags, const wchar_t* name, uint32_t maxThreads)
{
	XUSG_N_RETURN(pBuffer->CreateResource(pDevice, byteWidth, ResourceFlag::ALLOW_UNORDERED_ACCESS,
		MemoryType::DEFAULT, memoryFlags, dstState, 0, nullptr, maxThreads), false);
	pBuffer->SetName(name);

	return true;
}

bool AccelerationStructure::AllocateUploadBuffer(Device* pDevice, Buffer* pBuffer,
	size_t byteWidth, void* pData, MemoryFlag memoryFlags, const wchar_t* name)
{
	XUSG_N_RETURN(pBuffer->CreateResource(pDevice, byteWidth, ResourceFlag::ACCELERATION_STRUCTURE_BUILD_INPUT,
		MemoryType::UPLOAD, memoryFlags, ResourceState::GENERIC_READ_RESOURCE), false);
	pBuffer->SetName(name);

	void* pMappedData = pBuffer->Map();
	memcpy(pMappedData, pData, byteWidth);
	pBuffer->Unmap();

	return true;
}

uint32_t AccelerationStructure::SetBarrier(ResourceBarrier* pBarriers, Resource* pResource, uint32_t numBarriers)
{
	pBarriers[numBarriers++] = { pResource, ResourceState::RAYTRACING_ACCELERATION_STRUCTURE,
		ResourceState::RAYTRACING_ACCELERATION_STRUCTURE };

	return numBarriers;
}

size_t AccelerationStructure::Align(size_t byteSize)
{
	XUSG::Device::LockApi();
	XUSG_IF_VK(return AccelerationStructure_VK::Align(byteSize);)

	return AccelerationStructure_DX12::Align(byteSize);
}

void BottomLevelAS::SetTriangleGeometry(TriangleGeometry& triangles, Format vertexFormat, const VertexBufferView& vbv,
	const IndexBufferView* pIbv, GeometryFlag flags, const ResourceView* pTransform)
{
	auto strideIB = 0u;
	if (pIbv)
	{
		assert(pIbv->Format == Format::R32_UINT || pIbv->Format == Format::R16_UINT);
		strideIB = pIbv->Format == Format::R32_UINT ? sizeof(uint32_t) : sizeof(uint16_t);
	}

	triangles.Transform = pTransform ? pTransform->pResource->GetVirtualAddress() + pTransform->Offset : 0;
	triangles.IndexFormat = pIbv ? pIbv->Format : Format::UNKNOWN;
	triangles.VertexFormat = vertexFormat;
	triangles.IndexCount = pIbv ? pIbv->ByteSize / strideIB : 0;
	triangles.VertexCount = vbv.ByteSize / vbv.ByteStride;
	triangles.IndexBuffer = pIbv && pIbv->pResource ? pIbv->pResource->GetVirtualAddress() + pIbv->ByteOffset : 0;
	triangles.VertexBufferStart = vbv.pResource ? vbv.pResource->GetVirtualAddress() + vbv.ByteOffset : 0;
	triangles.VertexBufferStride = vbv.ByteStride;
}

void BottomLevelAS::SetTriangleGeometry(GeometryDesc& geometry, Format vertexFormat, const VertexBufferView& vbv,
	const IndexBufferView* pIbv, GeometryFlag flags, const ResourceView* pTransform)
{
	geometry.Type = GeometryType::TRIANGLES;
	geometry.Flags = flags;
	SetTriangleGeometry(geometry.Triangles, vertexFormat, vbv, pIbv, flags, pTransform);
}

void BottomLevelAS::SetAABBGeometry(GeometryDesc& geometry, const VertexBufferView& vbv, GeometryFlag flags)
{
	geometry.Type = GeometryType::PROCEDURAL_PRIMITIVE_AABBS;
	geometry.Flags = flags;
	geometry.AABBs.AABBCount = vbv.ByteSize / vbv.ByteStride;
	geometry.AABBs.AABBsStart = vbv.pResource ? vbv.pResource->GetVirtualAddress() + vbv.ByteOffset : 0;
	geometry.AABBs.AABBsStride = vbv.ByteStride;
}

void BottomLevelAS::SetOMMGeometry(GeometryDesc& geometry, const TriangleGeometry* pTriangles,
	const OMMLinkage* pOmmLinkage, GeometryFlag flags)
{
	geometry.Type = GeometryType::OMM_TRIANGLES;
	geometry.Flags = flags;
	geometry.OmmTriangles.pTriangles = pTriangles ? pTriangles : nullptr;
	geometry.OmmTriangles.pOmmLinkage = pOmmLinkage ? pOmmLinkage : nullptr;
}

void BottomLevelAS::SetGeometries(GeometryBuffer& geometries,
	uint32_t numGeometries, GeometryDesc* pGeometries)
{
	XUSG::Device::LockApi();
	XUSG_IF_VK(BottomLevelAS_VK::SetGeometries(geometries, numGeometries, pGeometries); return;)

	BottomLevelAS_DX12::SetGeometries(geometries, numGeometries, pGeometries);
}

size_t BottomLevelAS::AlignTransform(size_t byteSize)
{
	XUSG::Device::LockApi();
	XUSG_IF_VK(return BottomLevelAS_VK::AlignTransform(byteSize);)

	return BottomLevelAS_DX12::AlignTransform(byteSize);
}

size_t BottomLevelAS::AlignAABB(size_t byteSize)
{
	XUSG::Device::LockApi();
	XUSG_IF_VK(return BottomLevelAS_VK::AlignAABB(byteSize);)

	return BottomLevelAS_DX12::AlignAABB(byteSize);
}

RayTracing::Device::uptr RayTracing::Device::MakeUnique()
{
	XUSG::Device::LockApi();
	XUSG_IF_VK(return make_unique<RayTracing::Device_VK>();)

	return make_unique<RayTracing::Device_DX12>();
}

RayTracing::Device::sptr RayTracing::Device::MakeShared()
{
	XUSG::Device::LockApi();
	XUSG_IF_VK(return make_shared<RayTracing::Device_VK>();)

	return make_shared<RayTracing::Device_DX12>();
}

BottomLevelAS::uptr BottomLevelAS::MakeUnique()
{
	XUSG::Device::LockApi();
	XUSG_IF_VK(return make_unique<BottomLevelAS_VK>();)

	return make_unique<BottomLevelAS_DX12>();
}

BottomLevelAS::sptr BottomLevelAS::MakeShared()
{
	XUSG::Device::LockApi();
	XUSG_IF_VK(return make_shared<BottomLevelAS_VK>();)

	return make_shared<BottomLevelAS_DX12>();
}

void TopLevelAS::SetInstance(InstanceDesc& instanceDesc, const BottomLevelAS* pBottomLevelAS, const float* transform,
	InstanceFlag flags, uint32_t instanceID, uint8_t instanceMask, uint32_t instanceContributionToHitGroupIndex)
{
	instanceDesc.pTransform = transform;
	instanceDesc.InstanceID = instanceID;
	instanceDesc.InstanceMask = instanceMask;
	instanceDesc.InstanceContributionToHitGroupIndex = instanceContributionToHitGroupIndex;
	instanceDesc.Flags = static_cast<uint8_t>(flags);
	instanceDesc.pBottomLevelAS = pBottomLevelAS;
}

void TopLevelAS::SetInstances(Device* pDevice, Buffer* pInstances, uint32_t numInstances,
	const InstanceDesc* pInstanceDescs, MemoryFlag memoryFlags, const wchar_t* instanceName)
{
	XUSG::Device::LockApi();
	XUSG_IF_VK(TopLevelAS_VK::SetInstances(pDevice, pInstances, numInstances, pInstanceDescs, memoryFlags, instanceName); return;)

	TopLevelAS_DX12::SetInstances(pDevice, pInstances, numInstances, pInstanceDescs, memoryFlags, instanceName);
}

size_t TopLevelAS::AlignInstanceDesc(size_t byteSize)
{
	XUSG::Device::LockApi();
	XUSG_IF_VK(return TopLevelAS_VK::AlignInstanceDesc(byteSize);)

	return TopLevelAS_DX12::AlignInstanceDesc(byteSize);
}

TopLevelAS::uptr TopLevelAS::MakeUnique()
{
	XUSG::Device::LockApi();
	XUSG_IF_VK(return make_unique<TopLevelAS_VK>();)

	return make_unique<TopLevelAS_DX12>();
}

TopLevelAS::sptr TopLevelAS::MakeShared()
{
	XUSG::Device::LockApi();
	XUSG_IF_VK(return make_shared<TopLevelAS_VK>();)

	return make_shared<TopLevelAS_DX12>();
}

void OpacityMicromapArray::SetOmmArray(GeometryBuffer& ommArrayDescs, uint32_t numOpacityMicromaps,
	const Desc* pOmmArrayDescs)
{
	XUSG::Device::LockApi();
	XUSG_IF_VK(OpacityMicromapArray_VK::SetOmmArray(ommArrayDescs, numOpacityMicromaps, pOmmArrayDescs); return;)

	OpacityMicromapArray_DX12::SetOmmArray(ommArrayDescs, numOpacityMicromaps, pOmmArrayDescs);
}

void OpacityMicromapArray::SetOmmDescs(Device* pDevice, Buffer* pOMMDescsBuffer, uint32_t numOMMDescs,
	const OpacityMicromapDesc* pOMMDescs, MemoryFlag memoryFlags, const wchar_t* ommName)
{
	XUSG::Device::LockApi();
	XUSG_IF_VK(OpacityMicromapArray_VK::SetOmmDescs(pDevice, pOMMDescsBuffer, numOMMDescs, pOMMDescs, memoryFlags, ommName); return;)

	OpacityMicromapArray_DX12::SetOmmDescs(pDevice, pOMMDescsBuffer, numOMMDescs, pOMMDescs, memoryFlags, ommName);
}

size_t OpacityMicromapArray::AlignOmmInput(size_t byteSize)
{
	XUSG::Device::LockApi();
	XUSG_IF_VK(return OpacityMicromapArray_VK::AlignOmmInput(byteSize);)

	return OpacityMicromapArray_DX12::AlignOmmInput(byteSize);
}

size_t OpacityMicromapArray::AlignOmmDesc(size_t byteSize)
{
	XUSG::Device::LockApi();
	XUSG_IF_VK(return OpacityMicromapArray_VK::AlignOmmDesc(byteSize);)

	return OpacityMicromapArray_DX12::AlignOmmDesc(byteSize);
}

OpacityMicromapArray::uptr OpacityMicromapArray::MakeUnique()
{
	XUSG::Device::LockApi();
	XUSG_IF_VK(return make_unique<OpacityMicromapArray_VK>();)

	return make_unique<OpacityMicromapArray_DX12>();
}

OpacityMicromapArray::sptr OpacityMicromapArray::MakeShared()
{
	XUSG::Device::LockApi();
	XUSG_IF_VK(return make_shared<OpacityMicromapArray_VK>();)

	return make_shared<OpacityMicromapArray_DX12>();
}

const void* ShaderRecord::GetShaderIdentifier(const Pipeline& pipeline, const wchar_t* shaderName)
{
	XUSG::Device::LockApi();
	XUSG_IF_VK(return ShaderRecord_VK::GetShaderIdentifier(pipeline, shaderName);)

	return ShaderRecord_DX12::GetShaderIdentifier(pipeline, shaderName);
}

uint32_t ShaderRecord::GetShaderIdentifierSize(Device* pDevice)
{
	XUSG::Device::LockApi();
	XUSG_IF_VK(return ShaderRecord_VK::GetShaderIdentifierSize(pDevice);)

	return ShaderRecord_DX12::GetShaderIdentifierSize(pDevice);
}

ShaderRecord::uptr ShaderRecord::MakeUnique(void* pShaderIdentifier, uint32_t shaderIdentifierSize,
	const void* pLocalDescriptorArgs, uint32_t localDescriptorArgSize)
{
	XUSG::Device::LockApi();
	XUSG_IF_VK(return make_unique<ShaderRecord_VK>(pShaderIdentifier, shaderIdentifierSize, pLocalDescriptorArgs, localDescriptorArgSize);)

	return make_unique<ShaderRecord_DX12>(pShaderIdentifier, shaderIdentifierSize, pLocalDescriptorArgs, localDescriptorArgSize);
}

ShaderRecord::sptr ShaderRecord::MakeShared(void* pShaderIdentifier, uint32_t shaderIdentifierSize,
	const void* pLocalDescriptorArgs, uint32_t localDescriptorArgSize)
{
	XUSG::Device::LockApi();
	XUSG_IF_VK(return make_shared<ShaderRecord_VK>(pShaderIdentifier, shaderIdentifierSize, pLocalDescriptorArgs, localDescriptorArgSize);)

	return make_shared<ShaderRecord_DX12>(pShaderIdentifier, shaderIdentifierSize, pLocalDescriptorArgs, localDescriptorArgSize);
}

ShaderRecord::uptr ShaderRecord::MakeUnique(Device* pDevice, const Pipeline& pipeline, const wchar_t* shaderName,
	const void* pLocalDescriptorArgs, uint32_t localDescriptorArgSize)
{
	XUSG::Device::LockApi();
	XUSG_IF_VK(return make_unique<ShaderRecord_VK>(pDevice, pipeline, shaderName, pLocalDescriptorArgs, localDescriptorArgSize);)

	return make_unique<ShaderRecord_DX12>(pDevice, pipeline, shaderName, pLocalDescriptorArgs, localDescriptorArgSize);
}

ShaderRecord::sptr ShaderRecord::MakeShared(Device* pDevice, const Pipeline& pipeline, const wchar_t* shaderName,
	const void* pLocalDescriptorArgs, uint32_t localDescriptorArgSize)
{
	XUSG::Device::LockApi();
	XUSG_IF_VK(return make_shared<ShaderRecord_VK>(pDevice, pipeline, shaderName, pLocalDescriptorArgs, localDescriptorArgSize);)

	return make_shared<ShaderRecord_DX12>(pDevice, pipeline, shaderName, pLocalDescriptorArgs, localDescriptorArgSize);
}

size_t ShaderRecord::Align(uint32_t byteSize)
{
	XUSG::Device::LockApi();
	XUSG_IF_VK(return ShaderRecord_VK::Align(byteSize);)

	return ShaderRecord_DX12::Align(byteSize);
}

ShaderTable::uptr ShaderTable::MakeUnique()
{
	XUSG::Device::LockApi();
	XUSG_IF_VK(return make_unique<ShaderTable_VK>();)

	return make_unique<ShaderTable_DX12>();
}

ShaderTable::sptr ShaderTable::MakeShared()
{
	XUSG::Device::LockApi();
	XUSG_IF_VK(return make_shared<ShaderTable_VK>();)

	return make_shared<ShaderTable_DX12>();
}

size_t ShaderTable::Align(size_t byteSize)
{
	XUSG::Device::LockApi();
	XUSG_IF_VK(return ShaderTable_VK::Align(byteSize);)

	return ShaderTable_DX12::Align(byteSize);
}

RayTracing::CommandList::uptr RayTracing::CommandList::MakeUnique()
{
	XUSG::Device::LockApi();
	XUSG_IF_VK(return make_unique<RayTracing::CommandList_VK>();)

	return make_unique<RayTracing::CommandList_DX12>();
}

RayTracing::CommandList::sptr RayTracing::CommandList::MakeShared()
{
	XUSG::Device::LockApi();
	XUSG_IF_VK(return make_shared<RayTracing::CommandList_VK>();)

	return make_shared<RayTracing::CommandList_DX12>();
}

RayTracing::CommandList::uptr RayTracing::CommandList::MakeUnique(XUSG::CommandList* pCommandList, RayTracing::Device* pDevice)
{
	XUSG::Device::LockApi();
	XUSG_IF_VK(return make_unique<RayTracing::CommandList_VK>(pCommandList, pDevice);)

	return make_unique<RayTracing::CommandList_DX12>(pCommandList, pDevice);
}

RayTracing::CommandList::sptr RayTracing::CommandList::MakeShared(XUSG::CommandList* pCommandList, RayTracing::Device* pDevice)
{
	XUSG::Device::LockApi();
	XUSG_IF_VK(return make_shared<RayTracing::CommandList_VK>(pCommandList, pDevice);)

	return make_shared<RayTracing::CommandList_DX12>(pCommandList, pDevice);
}

RayTracing::PipelineLayout::uptr RayTracing::PipelineLayout::MakeUnique()
{
	XUSG::Device::LockApi();
	XUSG_IF_VK(return make_unique<RayTracing::PipelineLayout_VK>();)

	return make_unique<RayTracing::PipelineLayout_DX12>();
}

RayTracing::PipelineLayout::sptr RayTracing::PipelineLayout::MakeShared()
{
	XUSG::Device::LockApi();
	XUSG_IF_VK(return make_shared<RayTracing::PipelineLayout_VK>();)

	return make_shared<RayTracing::PipelineLayout_DX12>();
}

RayTracing::State::uptr RayTracing::State::MakeUnique()
{
	XUSG::Device::LockApi();
	XUSG_IF_VK(return make_unique<RayTracing::State_VK>();)

	return make_unique<RayTracing::State_DX12>();
}

RayTracing::State::sptr RayTracing::State::MakeShared()
{
	XUSG::Device::LockApi();
	XUSG_IF_VK(return make_shared<RayTracing::State_VK>();)

	return make_shared<RayTracing::State_DX12>();
}

RayTracing::PipelineLib::uptr RayTracing::PipelineLib::MakeUnique()
{
	XUSG::Device::LockApi();
	XUSG_IF_VK(return make_unique<RayTracing::PipelineLib_VK>();)

	return make_unique<RayTracing::PipelineLib_DX12>();
}

RayTracing::PipelineLib::sptr RayTracing::PipelineLib::MakeShared()
{
	XUSG::Device::LockApi();
	XUSG_IF_VK(return make_shared<RayTracing::PipelineLib_VK>();)

	return make_shared<RayTracing::PipelineLib_DX12>();
}

RayTracing::PipelineLib::uptr RayTracing::PipelineLib::MakeUnique(Device* pDevice)
{
	XUSG::Device::LockApi();
	XUSG_IF_VK(return make_unique<RayTracing::PipelineLib_VK>(pDevice);)

	return make_unique<RayTracing::PipelineLib_DX12>(pDevice);
}

RayTracing::PipelineLib::sptr RayTracing::PipelineLib::MakeShared(Device* pDevice)
	{
	XUSG::Device::LockApi();
	XUSG_IF_VK(return make_shared<RayTracing::PipelineLib_VK>(pDevice);)

	return make_shared<RayTracing::PipelineLib_DX12>(pDevice);
}
