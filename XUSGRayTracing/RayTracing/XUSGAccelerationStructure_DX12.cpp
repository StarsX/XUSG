//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSG_DX12.h"
#include "Core/XUSGResource_DX12.h"
#include "Core/XUSGEnum_DX12.h"
#include "XUSGRayTracing_DX12.h"
#include "XUSGAccelerationStructure_DX12.h"

using namespace std;
using namespace XUSG;
using namespace XUSG::RayTracing;

extern uint32_t g_numUAVs;

//--------------------------------------------------------------------------------------
// Acceleration structure
//--------------------------------------------------------------------------------------

AccelerationStructure_DX12::AccelerationStructure_DX12() :
	m_buildDesc(),
	m_prebuildInfo(),
	m_pointer(),
	m_byteOffset(0)
{
}

AccelerationStructure_DX12::~AccelerationStructure_DX12()
{
}

void AccelerationStructure_DX12::SetDestination(const Device* pDevice, const Buffer::sptr destBuffer,
	uintptr_t byteOffset, uint32_t uavIndex, DescriptorTableLib* pDescriptorTableLib)
{
	m_resource = destBuffer;
	m_byteOffset = byteOffset;

	if (pDescriptorTableLib)
	{
		uint32_t descriptorIndex;
		if (destBuffer->GetResourceState() != ResourceState::RAYTRACING_ACCELERATION_STRUCTURE)
		{
			const auto descriptorTable = Util::DescriptorTable::MakeUnique(API::DIRECTX_12);
			descriptorTable->SetDescriptors(0, 1, &destBuffer->GetUAV(uavIndex));
			descriptorIndex = descriptorTable->GetCbvSrvUavTableIndex(pDescriptorTableLib);
			g_numUAVs = (max)(descriptorIndex + 1, g_numUAVs);
		}
		else descriptorIndex = 0;

		// The Fallback Layer interface uses WRAPPED_GPU_POINTER to encapsulate the underlying pointer
		// which will either be an emulated GPU pointer for the compute - based path or a GPU_VIRTUAL_ADDRESS for the DXR path.
		const auto pDxDevice = static_cast<ID3D12RaytracingFallbackDevice*>(pDevice->GetRTHandle());
		m_pointer = pDxDevice->GetWrappedPointerSimple(descriptorIndex, m_resource->GetVirtualAddress() + byteOffset);
	}
}

uint32_t AccelerationStructure_DX12::SetBarrier(ResourceBarrier* pBarriers, uint32_t numBarriers)
{
	return AccelerationStructure::SetBarrier(pBarriers, m_resource.get(), numBarriers);
}

const PrebuildInfo& AccelerationStructure_DX12::GetPrebuildInfo() const
{
	return m_prebuildInfo;
}

Buffer* AccelerationStructure_DX12::GetPostbuildInfo() const
{
	return m_postbuildInfoRB.get();
}

size_t AccelerationStructure_DX12::GetResultDataMaxByteSize(bool isAligned) const
{
	const auto resultDataMaxByteSize = static_cast<size_t>(m_prebuildInfo.ResultDataMaxByteSize);

	return isAligned ? Align(resultDataMaxByteSize) : resultDataMaxByteSize;
}

size_t AccelerationStructure_DX12::GetScratchDataByteSize(bool isAligned) const
{
	const auto scratchDataByteSize = static_cast<size_t>(m_prebuildInfo.ScratchDataByteSize);

	return isAligned ? Align(scratchDataByteSize) : scratchDataByteSize;
}

size_t AccelerationStructure_DX12::GetUpdateScratchDataByteSize(bool isAligned) const
{
	const auto updateScratchDataByteSize = static_cast<size_t>(m_prebuildInfo.UpdateScratchDataByteSize);

	return isAligned ? Align(updateScratchDataByteSize) : updateScratchDataByteSize;
}

size_t AccelerationStructure_DX12::GetCompactedByteSize(bool isAligned) const
{
	using T = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE_DESC;
	const auto pPostbuildInfo = static_cast<const T*>(m_postbuildInfoRB->Map(nullptr));
	const auto compactedByteSize = static_cast<size_t>(pPostbuildInfo->CompactedSizeInBytes);
	m_postbuildInfoRB->Unmap();

	return isAligned ? Align(compactedByteSize) : compactedByteSize;
}

uint64_t AccelerationStructure_DX12::GetVirtualAddress() const
{
	return m_resource->GetVirtualAddress() + m_byteOffset;
}

uint64_t AccelerationStructure_DX12::GetResourcePointer() const
{
	return m_pointer.GpuVA;
}

bool AccelerationStructure_DX12::AllocateDestBuffer(const Device* pDevice, Buffer* pResource, size_t byteWidth,
	uint32_t numSRVs, const uintptr_t* firstSrvElements, uint32_t numUAVs, const uintptr_t* firstUavElements,
	MemoryFlag memoryFlags, const wchar_t* name, uint32_t maxThreads)
{
	// Allocate resources for acceleration structures.
	// Acceleration structures can only be placed in resources that are created in the default heap (or custom heap equivalent). 
	// Default heap is OK since the application doesn't need CPU read/write access to them. 
	// The resources that will contain acceleration structures must be created in the state D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, 
	// and must have resource flag D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS. The ALLOW_UNORDERED_ACCESS requirement simply acknowledges both: 
	//  - the system will be doing this type of access in its implementation of acceleration structure builds behind the scenes.
	//  - from the app point of view, synchronization of writes/reads to acceleration structures is accomplished using UAV barriers.
	const auto pDxDevice = static_cast<ID3D12RaytracingFallbackDevice*>(pDevice->GetRTHandle());
	const auto resourceFlags = pDxDevice->UsingRaytracingDriver() ? ResourceFlag::ACCELERATION_STRUCTURE : ResourceFlag::ALLOW_UNORDERED_ACCESS;

	return pResource->Create(pDevice, byteWidth, resourceFlags, MemoryType::DEFAULT,
		numSRVs, firstSrvElements, numUAVs, firstUavElements, memoryFlags, name,
		nullptr, maxThreads);
}

size_t AccelerationStructure_DX12::Align(size_t byteSize)
{
	return XUSG::Align(byteSize, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
}

bool AccelerationStructure_DX12::prebuild(const Device* pDevice)
{
	const auto& inputs = m_buildDesc.Inputs;
	assert(pDevice->GetHandle());

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo;
	const auto pDxDevice = static_cast<ID3D12RaytracingFallbackDevice*>(pDevice->GetRTHandle());
	pDxDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);

	m_prebuildInfo.ResultDataMaxByteSize = prebuildInfo.ResultDataMaxSizeInBytes;
	m_prebuildInfo.ScratchDataByteSize = prebuildInfo.ScratchDataSizeInBytes;
	m_prebuildInfo.UpdateScratchDataByteSize = prebuildInfo.UpdateScratchDataSizeInBytes;

	return m_prebuildInfo.ResultDataMaxByteSize;
}

bool AccelerationStructure_DX12::allocate(const Device* pDevice, size_t byteWidth,
	DescriptorTableLib* pDescriptorTableLib, uint32_t numSRVs, MemoryFlag memoryFlags,
	const wchar_t* name, uint32_t maxThreads)
{
	m_resource = Buffer::MakeShared(API::DIRECTX_12);
	byteWidth = byteWidth ? byteWidth : GetResultDataMaxByteSize(false);
	XUSG_N_RETURN(AllocateDestBuffer(pDevice, m_resource.get(), byteWidth, numSRVs,
		nullptr, 1, nullptr, memoryFlags, name, maxThreads), false);
	m_byteOffset = 0;

	if (pDescriptorTableLib)
	{
		uint32_t descriptorIndex;
		if (m_resource->GetResourceState() != ResourceState::RAYTRACING_ACCELERATION_STRUCTURE)
		{
			const auto descriptorTable = Util::DescriptorTable::MakeUnique(API::DIRECTX_12);
			descriptorTable->SetDescriptors(0, 1, &m_resource->GetUAV());
			descriptorIndex = descriptorTable->GetCbvSrvUavTableIndex(pDescriptorTableLib);
			g_numUAVs = (max)(descriptorIndex + 1, g_numUAVs);
		}
		else descriptorIndex = 0;

		// The Fallback Layer interface uses WRAPPED_GPU_POINTER to encapsulate the underlying pointer
		// which will either be an emulated GPU pointer for the compute - based path or a GPU_VIRTUAL_ADDRESS for the DXR path.
		const auto pDxDevice = static_cast<ID3D12RaytracingFallbackDevice*>(pDevice->GetRTHandle());
		m_pointer = pDxDevice->GetWrappedPointerSimple(descriptorIndex, m_resource->GetVirtualAddress());
	}

	return true;
}

//--------------------------------------------------------------------------------------
// Bottom level acceleration structure
//--------------------------------------------------------------------------------------

BottomLevelAS_DX12::BottomLevelAS_DX12() :
	AccelerationStructure_DX12()
{
}

BottomLevelAS_DX12::~BottomLevelAS_DX12()
{
}

bool BottomLevelAS_DX12::Prebuild(const Device* pDevice, uint32_t numGeometries,
	const GeometryBuffer& geometries, BuildFlag flags)
{
	m_buildDesc = {};
	auto& inputs = m_buildDesc.Inputs;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = GetDXRBuildFlags(flags);
	inputs.NumDescs = numGeometries;
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	inputs.pGeometryDescs = reinterpret_cast<const D3D12_RAYTRACING_GEOMETRY_DESC*>(geometries.data());

	// Get required sizes for an acceleration structure.
	return prebuild(pDevice);
}

bool BottomLevelAS_DX12::Allocate(const Device* pDevice, DescriptorTableLib* pDescriptorTableLib,
	size_t byteWidth, MemoryFlag memoryFlags, const wchar_t* name, uint32_t maxThreads)
{
	return allocate(pDevice, byteWidth, pDescriptorTableLib, 0, memoryFlags, name, maxThreads);
}

void BottomLevelAS_DX12::Build(CommandList* pCommandList, const Resource* pScratch, const BottomLevelAS* pSource,
	uint8_t numPostbuildInfoDescs, const PostbuildInfoType* pPostbuildInfoTypes)
{
	// Complete Acceleration Structure desc
	{
		if (pSource && (m_buildDesc.Inputs.Flags & D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE)
			== D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE)
		{
			m_buildDesc.SourceAccelerationStructureData = pSource->GetVirtualAddress();
			m_buildDesc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
		}

		m_buildDesc.DestAccelerationStructureData = GetVirtualAddress();
		m_buildDesc.ScratchAccelerationStructureData = pScratch->GetVirtualAddress();
	}

	if (numPostbuildInfoDescs)
	{
		m_postbuildInfoRB = Buffer::MakeUnique();
		m_postbuildInfo = Buffer::MakeUnique();
		m_postbuildInfo->Create(pCommandList->GetDevice(), sizeof(uint64_t) * numPostbuildInfoDescs,
			ResourceFlag::DENY_SHADER_RESOURCE | ResourceFlag::ALLOW_UNORDERED_ACCESS);

		ResourceBarrier barrier;
		const auto numBarriers = m_postbuildInfo->SetBarrier(&barrier, ResourceState::UNORDERED_ACCESS);
		static_cast<XUSG::CommandList*>(pCommandList)->Barrier(numBarriers, &barrier);
	}

	vector<PostbuildInfo> postbuildInfoDescs(numPostbuildInfoDescs);
	for (uint8_t i = 0; i < numPostbuildInfoDescs; ++i)
	{
		postbuildInfoDescs[i].DestBuffer = m_postbuildInfo->GetVirtualAddress() + sizeof(uint64_t) * i;
		postbuildInfoDescs[i].InfoType = pPostbuildInfoTypes[i];
	}

	// Build acceleration structure.
	pCommandList->BuildRaytracingAccelerationStructure(&m_buildDesc, numPostbuildInfoDescs,
		numPostbuildInfoDescs ? postbuildInfoDescs.data() : nullptr);

	if (m_postbuildInfo) m_postbuildInfo->ReadBack(pCommandList, m_postbuildInfoRB.get());
}

static D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC GetDX12RayTracingGeometryTrianglesDesc(const BottomLevelAS::TriangleGeometry& triangles)
{
	D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC trianglesDesc;
	trianglesDesc.Transform3x4 = triangles.Transform;
	trianglesDesc.IndexFormat = GetDXGIFormat(triangles.IndexFormat);
	trianglesDesc.VertexFormat = GetDXGIFormat(triangles.VertexFormat);
	trianglesDesc.IndexCount = triangles.IndexCount;
	trianglesDesc.VertexCount = triangles.VertexCount;
	trianglesDesc.IndexBuffer = triangles.IndexBuffer;
	trianglesDesc.VertexBuffer.StartAddress = triangles.VertexBufferStart;
	trianglesDesc.VertexBuffer.StrideInBytes = triangles.VertexBufferStride;

	return trianglesDesc;
}

void BottomLevelAS_DX12::SetGeometries(GeometryBuffer& geometries, uint32_t numGeometries, GeometryDesc* pGeometries)
{
	assert(pGeometries);
	const auto geometriesSize = sizeof(D3D12_RAYTRACING_GEOMETRY_DESC) * numGeometries;
	auto bufferSize = geometriesSize;
	size_t ommTriGeometriesSize = 0;
	for (auto i = 0u; i < numGeometries; ++i)
	{
		if (pGeometries[i].Type == GeometryType::OMM_TRIANGLES)
		{
			if (pGeometries[i].OmmTriangles.pTriangles) ommTriGeometriesSize += sizeof(D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC);
			if (pGeometries[i].OmmTriangles.pOmmLinkage) bufferSize += sizeof(D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC);
		}
	}
	bufferSize += ommTriGeometriesSize;

	geometries.resize(bufferSize);
	auto pTriangles = reinterpret_cast<D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC*>(&geometries[geometriesSize]);
	auto pOmmLinkage = reinterpret_cast<D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC*>(&geometries[geometriesSize + ommTriGeometriesSize]);
	for (auto i = 0u; i < numGeometries; ++i)
	{
		const auto& geometry = pGeometries[i];
		auto& geometryDesc = reinterpret_cast<D3D12_RAYTRACING_GEOMETRY_DESC*>(geometries.data())[i];
		geometryDesc.Flags = GetDXRGeometryFlags(geometry.Flags);

		switch (geometry.Type)
		{
		case GeometryType::TRIANGLES:
			geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
			geometryDesc.Triangles = GetDX12RayTracingGeometryTrianglesDesc(geometry.Triangles);
			break;
		case GeometryType::PROCEDURAL_PRIMITIVE_AABBS:
			geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
			geometryDesc.AABBs.AABBCount = geometry.AABBs.AABBCount;
			geometryDesc.AABBs.AABBs.StartAddress = geometry.AABBs.AABBsStart;
			geometryDesc.AABBs.AABBs.StrideInBytes = geometry.AABBs.AABBsStride;
		case GeometryType::OMM_TRIANGLES:
		{
			geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES;

			if (geometry.OmmTriangles.pTriangles)
				*pTriangles = GetDX12RayTracingGeometryTrianglesDesc(*geometry.OmmTriangles.pTriangles);

			if (geometry.OmmTriangles.pOmmLinkage)
			{
				pOmmLinkage->OpacityMicromapIndexBuffer.StartAddress = geometry.OmmTriangles.pOmmLinkage->OpacityMicromapIndexBufferStart;
				pOmmLinkage->OpacityMicromapIndexBuffer.StrideInBytes = geometry.OmmTriangles.pOmmLinkage->OpacityMicromapIndexBufferStride;
				pOmmLinkage->OpacityMicromapIndexFormat = GetDXGIFormat(geometry.OmmTriangles.pOmmLinkage->OpacityMicromapIndexFormat);
				pOmmLinkage->OpacityMicromapBaseLocation = geometry.OmmTriangles.pOmmLinkage->OpacityMicromapBaseLocation;
				pOmmLinkage->OpacityMicromapArray = geometry.OmmTriangles.pOmmLinkage->pOpacityMicromapArray->GetVirtualAddress() + geometry.OmmTriangles.pOmmLinkage->OpacityMicromapArrayOffset;
			}

			geometryDesc.OmmTriangles.pTriangles = pGeometries[i].OmmTriangles.pTriangles ? pTriangles++ : nullptr;
			geometryDesc.OmmTriangles.pOmmLinkage = pGeometries[i].OmmTriangles.pOmmLinkage ? pOmmLinkage++ : nullptr;
			break;
		}
		default:
			assert(!"Unsupported geometry type.");
		}
	}
}

size_t BottomLevelAS_DX12::AlignTransform(size_t byteSize)
{
	return XUSG::Align(byteSize, D3D12_RAYTRACING_TRANSFORM3X4_BYTE_ALIGNMENT);
}

size_t BottomLevelAS_DX12::AlignAABB(size_t byteSize)
{
	return XUSG::Align(byteSize, D3D12_RAYTRACING_AABB_BYTE_ALIGNMENT);
}

//--------------------------------------------------------------------------------------
// Top level acceleration structure
//--------------------------------------------------------------------------------------

TopLevelAS_DX12::TopLevelAS_DX12() :
	AccelerationStructure_DX12(),
	m_srvIndex(0)
{
}

TopLevelAS_DX12::~TopLevelAS_DX12()
{
}

bool TopLevelAS_DX12::Prebuild(const Device* pDevice, uint32_t numInstances, BuildFlag flags)
{
	m_buildDesc = {};
	auto& inputs = m_buildDesc.Inputs;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = GetDXRBuildFlags(flags);
	inputs.NumDescs = numInstances;
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

	// Get required sizes for an acceleration structure.
	return prebuild(pDevice);
}

bool TopLevelAS_DX12::Allocate(const Device* pDevice, DescriptorTableLib* pDescriptorTableLib,
	size_t byteWidth, MemoryFlag memoryFlags, const wchar_t* name, uint32_t maxThreads)
{
	m_srvIndex = 0;

	return allocate(pDevice, byteWidth, pDescriptorTableLib, 1, memoryFlags, name, maxThreads);
}

void TopLevelAS_DX12::SetDestination(const Device* pDevice, const Buffer::sptr destBuffer, uintptr_t byteOffset,
	uint32_t uavIndex, uint32_t srvIndex, DescriptorTableLib* pDescriptorTableLib)
{
	m_srvIndex = srvIndex;
	AccelerationStructure_DX12::SetDestination(pDevice, destBuffer, byteOffset, uavIndex, pDescriptorTableLib);
}

void TopLevelAS_DX12::Build(CommandList* pCommandList, const Resource* pScratch,
	const Resource* pInstanceDescs, const DescriptorHeap& descriptorHeap,
	const TopLevelAS* pSource, uint8_t numPostbuildInfoDescs,
	const PostbuildInfoType* pPostbuildInfoTypes)
{
	// Complete Acceleration Structure desc
	{
		if (pSource && (m_buildDesc.Inputs.Flags & D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE)
			== D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE)
		{
			m_buildDesc.SourceAccelerationStructureData = pSource->GetVirtualAddress();
			m_buildDesc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
		}

		m_buildDesc.DestAccelerationStructureData = GetVirtualAddress();
		m_buildDesc.Inputs.InstanceDescs = pInstanceDescs->GetVirtualAddress();
		m_buildDesc.ScratchAccelerationStructureData = pScratch->GetVirtualAddress();
	}

	if (numPostbuildInfoDescs)
	{
		m_postbuildInfoRB = Buffer::MakeShared();
		m_postbuildInfo = Buffer::MakeUnique();
		m_postbuildInfo->Create(pCommandList->GetDevice(), sizeof(uint64_t) * numPostbuildInfoDescs,
			ResourceFlag::DENY_SHADER_RESOURCE | ResourceFlag::ALLOW_UNORDERED_ACCESS);

		ResourceBarrier barrier;
		const auto numBarriers = m_postbuildInfo->SetBarrier(&barrier, ResourceState::UNORDERED_ACCESS);
		static_cast<XUSG::CommandList*>(pCommandList)->Barrier(numBarriers, &barrier);
	}

	vector<PostbuildInfo> postbuildInfoDescs(numPostbuildInfoDescs);
	for (uint8_t i = 0; i < numPostbuildInfoDescs; ++i)
	{
		postbuildInfoDescs[i].DestBuffer = m_postbuildInfo->GetVirtualAddress() + sizeof(uint64_t) * i;
		postbuildInfoDescs[i].InfoType = pPostbuildInfoTypes[i];
	}

	// Build acceleration structure.
	pCommandList->BuildRaytracingAccelerationStructure(&m_buildDesc, numPostbuildInfoDescs,
		numPostbuildInfoDescs ? postbuildInfoDescs.data() : nullptr, &descriptorHeap);

	if (m_postbuildInfo) m_postbuildInfo->ReadBack(pCommandList, m_postbuildInfoRB.get());
}

const Descriptor& TopLevelAS_DX12::GetSRV() const
{
	return m_resource->GetSRV(m_srvIndex);
}

void TopLevelAS_DX12::SetInstances(const Device* pDevice, Buffer* pInstances, uint32_t numInstances,
	const InstanceDesc* pInstanceDescs, MemoryFlag memoryFlags, const wchar_t* instanceName)
{
	assert(numInstances == 0 || pInstanceDescs);

	// Note on Emulated GPU pointers (AKA Wrapped pointers) requirement in Fallback Layer:
	// The primary point of divergence between the DXR API and the compute-based Fallback layer is the handling of GPU pointers. 
	// DXR fundamentally requires that GPUs be able to dynamically read from arbitrary addresses in GPU memory. 
	// The existing Direct Compute API today is more rigid than DXR and requires apps to explicitly inform the GPU what blocks of memory it will access with SRVs/UAVs.
	// In order to handle the requirements of DXR, the Fallback Layer uses the concept of Emulated GPU pointers, 
	// which requires apps to create views around all memory they will access for raytracing, 
	// but retains the DXR-like flexibility of only needing to bind the top level acceleration structure at DispatchRays.
	//
	// The Fallback Layer interface uses WRAPPED_GPU_POINTER to encapsulate the underlying pointer
	// which will either be an emulated GPU pointer for the compute - based path or a GPU_VIRTUAL_ADDRESS for the DXR path.
	vector<D3D12_RAYTRACING_FALLBACK_INSTANCE_DESC> instanceDescs(numInstances);
	for (auto i = 0u; i < numInstances; ++i)
	{
		const auto& instanceDesc = pInstanceDescs[i];
		const auto& pBottomLevelAS = pInstanceDescs[i].pBottomLevelAS;
		const auto bottomLevelASPtr = pBottomLevelAS ? pBottomLevelAS->GetResourcePointer() : 0;
		memcpy(instanceDescs[i].Transform, instanceDesc.pTransform, sizeof(instanceDescs[i].Transform));
		instanceDescs[i].InstanceID = instanceDesc.InstanceID;
		instanceDescs[i].InstanceMask = instanceDesc.InstanceMask;
		instanceDescs[i].InstanceContributionToHitGroupIndex = instanceDesc.InstanceContributionToHitGroupIndex;
		instanceDescs[i].Flags = GetDXRInstanceFlags(static_cast<InstanceFlag>(instanceDesc.Flags));
		instanceDescs[i].AccelerationStructure = reinterpret_cast<const WRAPPED_GPU_POINTER&>(bottomLevelASPtr);
	}

	if (pInstances->GetHandle())
	{
		void* pMappedData = pInstances->Map();
		memcpy(pMappedData, instanceDescs.data(), sizeof(D3D12_RAYTRACING_FALLBACK_INSTANCE_DESC) * numInstances);
		pInstances->Unmap();
	}
	else AccelerationStructure_DX12::AllocateUploadBuffer(pDevice, pInstances,
		sizeof(D3D12_RAYTRACING_FALLBACK_INSTANCE_DESC) * numInstances,
		instanceDescs.data(), memoryFlags, instanceName);
}

size_t TopLevelAS_DX12::AlignInstanceDesc(size_t byteSize)
{
	return XUSG::Align(byteSize, D3D12_RAYTRACING_INSTANCE_DESCS_BYTE_ALIGNMENT);
}

//--------------------------------------------------------------------------------------
// Opacity micromap array
//--------------------------------------------------------------------------------------

OpacityMicromapArray_DX12::OpacityMicromapArray_DX12() :
	AccelerationStructure_DX12()
{
}

OpacityMicromapArray_DX12::~OpacityMicromapArray_DX12()
{
}

bool OpacityMicromapArray_DX12::Prebuild(const Device* pDevice, uint32_t numOpacityMicromaps,
	const GeometryBuffer& ommArrayDescs, BuildFlag flags)
{
	m_buildDesc = {};
	auto& inputs = m_buildDesc.Inputs;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = GetDXRBuildFlags(flags);
	inputs.NumDescs = numOpacityMicromaps;
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_ARRAY;
	inputs.pOpacityMicromapArrayDesc = reinterpret_cast<const D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC*>(ommArrayDescs.data());

	// Get required sizes for an acceleration structure.
	return prebuild(pDevice);
}

bool OpacityMicromapArray_DX12::Allocate(const Device* pDevice, DescriptorTableLib* pDescriptorTableLib,
	size_t byteWidth, MemoryFlag memoryFlags, const wchar_t* name, uint32_t maxThreads)
{
	return allocate(pDevice, byteWidth, pDescriptorTableLib, 0, memoryFlags, name, maxThreads);
}

void OpacityMicromapArray_DX12::SetDestination(const Device* pDevice, const Buffer::sptr destBuffer, uintptr_t byteOffset)
{
	AccelerationStructure_DX12::SetDestination(pDevice, destBuffer, byteOffset, 0, nullptr);
}

void OpacityMicromapArray_DX12::Build(CommandList* pCommandList, const Resource* pScratch, const OpacityMicromapArray* pSource,
	uint8_t numPostbuildInfoDescs, const PostbuildInfoType* pPostbuildInfoTypes)
{
	// Complete Acceleration Structure desc
	{
		if (pSource && (m_buildDesc.Inputs.Flags & D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_OMM_UPDATE)
			== D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_OMM_UPDATE)
		{
			m_buildDesc.SourceAccelerationStructureData = pSource->GetVirtualAddress();
			m_buildDesc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
		}

		m_buildDesc.DestAccelerationStructureData = GetVirtualAddress();
		m_buildDesc.ScratchAccelerationStructureData = pScratch->GetVirtualAddress();
	}

	if (numPostbuildInfoDescs)
	{
		m_postbuildInfoRB = Buffer::MakeUnique();
		m_postbuildInfo = Buffer::MakeUnique();
		m_postbuildInfo->Create(pCommandList->GetDevice(), sizeof(uint64_t) * numPostbuildInfoDescs,
			ResourceFlag::DENY_SHADER_RESOURCE | ResourceFlag::ALLOW_UNORDERED_ACCESS);

		ResourceBarrier barrier;
		const auto numBarriers = m_postbuildInfo->SetBarrier(&barrier, ResourceState::UNORDERED_ACCESS);
		static_cast<XUSG::CommandList*>(pCommandList)->Barrier(numBarriers, &barrier);
	}

	vector<PostbuildInfo> postbuildInfoDescs(numPostbuildInfoDescs);
	for (uint8_t i = 0; i < numPostbuildInfoDescs; ++i)
	{
		postbuildInfoDescs[i].DestBuffer = m_postbuildInfo->GetVirtualAddress() + sizeof(uint64_t) * i;
		postbuildInfoDescs[i].InfoType = pPostbuildInfoTypes[i];
	}

	// Build acceleration structure.
	pCommandList->BuildRaytracingAccelerationStructure(&m_buildDesc, numPostbuildInfoDescs,
		numPostbuildInfoDescs ? postbuildInfoDescs.data() : nullptr);

	if (m_postbuildInfo) m_postbuildInfo->ReadBack(pCommandList, m_postbuildInfoRB.get());
}

void OpacityMicromapArray_DX12::SetOmmArray(GeometryBuffer& ommArrayDescs, uint32_t numOpacityMicromaps, const Desc* pOmmArrayDescs)
{
	const auto ommArraysSize = sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC) * numOpacityMicromaps;
	auto bufferSize = ommArraysSize;
	for (auto i = 0u; i < numOpacityMicromaps; ++i)
		bufferSize += sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_HISTOGRAM_ENTRY) * pOmmArrayDescs[i].NumOmmHistogramEntries;

	ommArrayDescs.resize(bufferSize);
	auto pOmmHistogram = reinterpret_cast<D3D12_RAYTRACING_OPACITY_MICROMAP_HISTOGRAM_ENTRY*>(&ommArrayDescs[ommArraysSize]);
	for (auto i = 0u; i < numOpacityMicromaps; ++i)
	{
		auto& ommArrayDesc = reinterpret_cast<D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC*>(ommArrayDescs.data())[i];

		ommArrayDesc = {};
		ommArrayDesc.NumOmmHistogramEntries = pOmmArrayDescs[i].NumOmmHistogramEntries;
		for (auto j = 0u; j < ommArrayDesc.NumOmmHistogramEntries; ++j)
		{
			assert(pOmmArrayDescs[i].pOmmHistogram);
			pOmmHistogram->Count = pOmmArrayDescs[i].pOmmHistogram->Count;
			pOmmHistogram->SubdivisionLevel = pOmmArrayDescs[i].pOmmHistogram->SubdivisionLevel;
			pOmmHistogram->Format = static_cast<D3D12_RAYTRACING_OPACITY_MICROMAP_FORMAT>(pOmmArrayDescs[i].pOmmHistogram->Format);
		}
		ommArrayDesc.pOmmHistogram = pOmmHistogram++;
		ommArrayDesc.InputBuffer = pOmmArrayDescs[i].pInputBuffer->GetVirtualAddress() + pOmmArrayDescs[i].InputOffset;
		ommArrayDesc.PerOmmDescs.StartAddress = pOmmArrayDescs[i].pPerOmmDescs->GetVirtualAddress() + pOmmArrayDescs[i].PerOmmDescOffset;
		ommArrayDesc.PerOmmDescs.StrideInBytes = pOmmArrayDescs[i].PerOmmDescStride;
	}
}

void OpacityMicromapArray_DX12::SetOmmDescs(const Device* pDevice, Buffer* pOmmDescsBuffer, uint32_t numOmmDescs,
	const OpacityMicromapDesc* pOmmDescs, MemoryFlag memoryFlags, const wchar_t* ommName)
{
	assert(numOmmDescs == 0 || pOmmDescs);

	vector<D3D12_RAYTRACING_OPACITY_MICROMAP_DESC> ommDescs(numOmmDescs);
	for (auto i = 0u; i < numOmmDescs; ++i)
	{
		const auto& ommDesc = pOmmDescs[i];
		ommDescs[i].ByteOffset = ommDesc.ByteOffset;
		ommDescs[i].SubdivisionLevel = ommDesc.SubdivisionLevel;
		ommDescs[i].Format = static_cast<D3D12_RAYTRACING_OPACITY_MICROMAP_FORMAT>(ommDesc.Format);
	}

	if (pOmmDescsBuffer->GetHandle())
	{
		void* pMappedData = pOmmDescsBuffer->Map();
		memcpy(pMappedData, ommDescs.data(), sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_DESC) * numOmmDescs);
		pOmmDescsBuffer->Unmap();
	}
	else AccelerationStructure_DX12::AllocateUploadBuffer(pDevice, pOmmDescsBuffer,
		sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_DESC) * numOmmDescs,
		ommDescs.data(), memoryFlags, ommName);
}

size_t OpacityMicromapArray_DX12::AlignOmmInput(size_t byteSize)
{
	return XUSG::Align(byteSize, 128);
}

size_t XUSG::RayTracing::OpacityMicromapArray_DX12::AlignOmmDesc(size_t byteSize)
{
	return XUSG::Align(byteSize, 4);
}
