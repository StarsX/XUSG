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

#if ENABLE_DXR_FALLBACK
extern uint32_t g_numUAVs;
#endif
extern uint32_t g_frameCount;

//--------------------------------------------------------------------------------------
// Acceleration structure
//--------------------------------------------------------------------------------------

AccelerationStructure_DX12::AccelerationStructure_DX12() :
	m_currentFrame(0)
{
}

AccelerationStructure_DX12::~AccelerationStructure_DX12()
{
}

RawBuffer::sptr AccelerationStructure_DX12::GetResult() const
{
	return m_results[m_currentFrame];
}

uint32_t AccelerationStructure_DX12::GetResultDataMaxSize() const
{
	return static_cast<uint32_t>(m_prebuildInfo.ResultDataMaxSizeInBytes);
}

uint32_t AccelerationStructure_DX12::GetScratchDataMaxSize() const
{
	return static_cast<uint32_t>(m_prebuildInfo.ScratchDataSizeInBytes);
}

uint32_t AccelerationStructure_DX12::GetUpdateScratchDataSize() const
{
	return static_cast<uint32_t>(m_prebuildInfo.UpdateScratchDataSizeInBytes);
}

#if ENABLE_DXR_FALLBACK
const WRAPPED_GPU_POINTER& AccelerationStructure_DX12::GetResultPointer() const
{
	return m_pointers[m_currentFrame];
}
#endif

bool AccelerationStructure_DX12::AllocateUAVBuffer(const XUSG::Device* pDevice, Resource* pResource,
	size_t byteWidth, ResourceState dstState)
{
	const auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	const auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(byteWidth, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	const auto pDxDevice = static_cast<ID3D12Device*>(pDevice->GetHandle());
	auto& dxResource = dynamic_cast<Resource_DX12*>(pResource)->GetResource();

	assert(pDxDevice);
	V_RETURN(pDxDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc,
		static_cast<D3D12_RESOURCE_STATES>(dstState), nullptr, IID_PPV_ARGS(&dxResource)), cerr, false);

	return true;
}

bool AccelerationStructure_DX12::AllocateUploadBuffer(const XUSG::Device* pDevice, Resource* pResource,
	size_t byteWidth, void* pData)
{
	const auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	const auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(byteWidth);

	const auto pDxDevice = static_cast<ID3D12Device*>(pDevice->GetHandle());
	auto& dxResource = dynamic_cast<Resource_DX12*>(pResource)->GetResource();

	assert(pDxDevice);
	V_RETURN(pDxDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE,
		&bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&dxResource)), cerr, false);

	void* pMappedData;
	dxResource->Map(0, nullptr, &pMappedData);
	memcpy(pMappedData, pData, byteWidth);
	dxResource->Unmap(0, nullptr);

	return true;
}

bool AccelerationStructure_DX12::preBuild(const Device* pDevice, uint32_t descriptorIndex, uint32_t numSRVs)
{
	const auto& inputs = m_buildDesc.Inputs;
	assert(pDevice->GetHandle());

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo;
#if ENABLE_DXR_FALLBACK
	const auto pDxDevice = static_cast<ID3D12RaytracingFallbackDevice*>(pDevice->GetRTHandle());
	pDxDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo, g_numUAVs);
#else // DirectX Raytracing
	const auto pDxDevice = static_cast<ID3D12Device5*>(pDevice->GetRTHandle());
	pDxDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);
#endif

	m_prebuildInfo.ResultDataMaxSizeInBytes = prebuildInfo.ResultDataMaxSizeInBytes;
	m_prebuildInfo.ScratchDataSizeInBytes = prebuildInfo.ScratchDataSizeInBytes;
	m_prebuildInfo.UpdateScratchDataSizeInBytes = prebuildInfo.UpdateScratchDataSizeInBytes;

	N_RETURN(m_prebuildInfo.ResultDataMaxSizeInBytes > 0, false);

	// Allocate resources for acceleration structures.
	// Acceleration structures can only be placed in resources that are created in the default heap (or custom heap equivalent). 
	// Default heap is OK since the application doesn't need CPU read/write access to them. 
	// The resources that will contain acceleration structures must be created in the state D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, 
	// and must have resource flag D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS. The ALLOW_UNORDERED_ACCESS requirement simply acknowledges both: 
	//  - the system will be doing this type of access in its implementation of acceleration structure builds behind the scenes.
	//  - from the app point of view, synchronization of writes/reads to acceleration structures is accomplished using UAV barriers.
	const auto bufferCount = (inputs.Flags & D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE)
		== D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE ? g_frameCount : 1;

	m_results.resize(bufferCount);
#if ENABLE_DXR_FALLBACK
	const auto resourceFlags = pDxDevice->UsingRaytracingDriver() ? ResourceFlag::ACCELERATION_STRUCTURE : ResourceFlag::ALLOW_UNORDERED_ACCESS;
#else // DirectX Raytracing
	const auto resourceFlags = ResourceFlag::ACCELERATION_STRUCTURE;
#endif
	for (auto& result : m_results)
	{
		result = RawBuffer::MakeShared();
		N_RETURN(result->Create(pDevice, GetResultDataMaxSize(), resourceFlags, MemoryType::DEFAULT, numSRVs), false);
	}

#if ENABLE_DXR_FALLBACK
	// The Fallback Layer interface uses WRAPPED_GPU_POINTER to encapsulate the underlying pointer
	// which will either be an emulated GPU pointer for the compute - based path or a GPU_VIRTUAL_ADDRESS for the DXR path.
	m_pointers.resize(bufferCount);
	for (auto i = 0u; i < bufferCount; ++i)
		m_pointers[i] = pDxDevice->GetWrappedPointerSimple(descriptorIndex,
			static_cast<ID3D12Resource*>(m_results[i]->GetHandle())->GetGPUVirtualAddress());
#endif

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

bool BottomLevelAS_DX12::PreBuild(const Device* pDevice, uint32_t numDescs,
	const GeometryBuffer& geometries, uint32_t descriptorIndex, BuildFlag flags)
{
	m_buildDesc = {};
	auto& inputs = m_buildDesc.Inputs;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = GetDXRBuildFlags(flags);
	inputs.NumDescs = numDescs;
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	inputs.pGeometryDescs = reinterpret_cast<const D3D12_RAYTRACING_GEOMETRY_DESC*>(geometries.data());

	// Get required sizes for an acceleration structure.
	return preBuild(pDevice, descriptorIndex);
}

void BottomLevelAS_DX12::Build(const CommandList* pCommandList, const Resource* pScratch,
	const DescriptorPool& descriptorPool, bool update)
{
	// Complete Acceleration Structure desc
	{
		const auto pResultBuffer = static_cast<ID3D12Resource*>(m_results[m_currentFrame]->GetHandle());
		if (update && (m_buildDesc.Inputs.Flags & D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE)
			== D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE)
		{
			m_buildDesc.SourceAccelerationStructureData = pResultBuffer->GetGPUVirtualAddress();
			m_buildDesc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
			m_currentFrame = (m_currentFrame + 1) % g_frameCount;
		}

		m_buildDesc.DestAccelerationStructureData = pResultBuffer->GetGPUVirtualAddress();
		m_buildDesc.ScratchAccelerationStructureData = dynamic_cast<const Resource_DX12*>(pScratch)->GetGPUVirtualAddress();
	}

	// Build acceleration structure.
	pCommandList->BuildRaytracingAccelerationStructure(&m_buildDesc, 0, nullptr, descriptorPool);

	// Resource barrier
	const ResourceBarrier barrier =
	{
		m_results[m_currentFrame].get(),
		ResourceState::UNORDERED_ACCESS,
		ResourceState::UNORDERED_ACCESS,
		BARRIER_ALL_SUBRESOURCES
	};
	pCommandList->Barrier(1, &barrier);
}

#if !ENABLE_DXR_FALLBACK
void BottomLevelAS_DX12::Build(XUSG::CommandList* pCommandList, const Resource* pScratch,
	const DescriptorPool& descriptorPool, bool update)
{
	const auto commandList = CommandList::MakeUnique(pCommandList);
	Build(commandList.get(), pScratch, descriptorPool, update);
}
#endif

void BottomLevelAS_DX12::SetTriangleGeometries(GeometryBuffer& geometries, uint32_t numGeometries,
	Format vertexFormat, const VertexBufferView* pVBs, const IndexBufferView* pIBs,
	const GeometryFlag* pGeometryFlags, const ResourceView* pTransforms)
{
	geometries.resize(sizeof(D3D12_RAYTRACING_GEOMETRY_DESC) * numGeometries);
	for (auto i = 0u; i < numGeometries; ++i)
	{
		auto& geometryDesc = reinterpret_cast<D3D12_RAYTRACING_GEOMETRY_DESC*>(geometries.data())[i];

		auto strideIB = 0u;
		if (pIBs)
		{
			assert(pIBs[i].Format == Format::R32_UINT || pIBs[i].Format == Format::R16_UINT);
			strideIB = pIBs[i].Format == Format::R32_UINT ? sizeof(uint32_t) : sizeof(uint16_t);
		}

		geometryDesc = {};
		geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		geometryDesc.Triangles.Transform3x4 = pTransforms ?
			dynamic_cast<const Resource_DX12*>(pTransforms[i].pResource)->GetGPUVirtualAddress() + pTransforms[i].Offset : 0;
		geometryDesc.Triangles.IndexFormat = pIBs ? GetDXGIFormat(pIBs[i].Format) : DXGI_FORMAT_UNKNOWN;
		geometryDesc.Triangles.VertexFormat = GetDXGIFormat(vertexFormat);
		geometryDesc.Triangles.IndexCount = pIBs ? pIBs[i].SizeInBytes / strideIB : 0;
		geometryDesc.Triangles.VertexCount = pVBs ? pVBs[i].SizeInBytes / pVBs[i].StrideInBytes : 0;
		geometryDesc.Triangles.IndexBuffer = pIBs ? pIBs[i].BufferLocation : 0;
		geometryDesc.Triangles.VertexBuffer.StartAddress = pVBs ? pVBs[i].BufferLocation : 0;
		geometryDesc.Triangles.VertexBuffer.StrideInBytes = pVBs ? pVBs[i].StrideInBytes : 0;

		// Mark the geometry as opaque. 
		// PERFORMANCE TIP: mark geometry as opaque whenever applicable as it can enable important ray processing optimizations.
		// Note: When rays encounter opaque geometry an any hit shader will not be executed whether it is present or not.
		geometryDesc.Flags = pGeometryFlags ? GetDXRGeometryFlags(pGeometryFlags[i]) : D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
	}
}

void BottomLevelAS_DX12::SetAABBGeometries(GeometryBuffer& geometries, uint32_t numGeometries,
	const VertexBufferView* pVBs, const GeometryFlag* pGeometryFlags)
{
	geometries.resize(sizeof(D3D12_RAYTRACING_GEOMETRY_DESC) * numGeometries);
	for (auto i = 0u; i < numGeometries; ++i)
	{
		auto& geometryDesc = reinterpret_cast<D3D12_RAYTRACING_GEOMETRY_DESC*>(geometries.data())[i];

		geometryDesc = {};
		geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
		geometryDesc.AABBs.AABBCount = pVBs ? pVBs[i].SizeInBytes / pVBs[i].StrideInBytes : 0;
		geometryDesc.AABBs.AABBs.StartAddress = pVBs ? pVBs[i].BufferLocation : 0;
		geometryDesc.AABBs.AABBs.StrideInBytes = pVBs ? pVBs[i].StrideInBytes : sizeof(D3D12_RAYTRACING_AABB);

		// Mark the geometry as opaque. 
		// PERFORMANCE TIP: mark geometry as opaque whenever applicable as it can enable important ray processing optimizations.
		// Note: When rays encounter opaque geometry an any hit shader will not be executed whether it is present or not.
		geometryDesc.Flags = pGeometryFlags ? GetDXRGeometryFlags(pGeometryFlags[i]) : D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
	}
}

//--------------------------------------------------------------------------------------
// Top level acceleration structure
//--------------------------------------------------------------------------------------

TopLevelAS_DX12::TopLevelAS_DX12() :
	AccelerationStructure_DX12()
{
}

TopLevelAS_DX12::~TopLevelAS_DX12()
{
}

bool TopLevelAS_DX12::PreBuild(const Device* pDevice, uint32_t numDescs,
	uint32_t descriptorIndex, BuildFlag flags)
{
	m_buildDesc = {};
	auto& inputs = m_buildDesc.Inputs;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = GetDXRBuildFlags(flags);
	inputs.NumDescs = numDescs;
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

	// Get required sizes for an acceleration structure.
	return preBuild(pDevice, descriptorIndex, 1);
}

void TopLevelAS_DX12::Build(const CommandList* pCommandList, const Resource* pScratch,
	const Resource* pInstanceDescs, const DescriptorPool& descriptorPool, bool update)
{
	// Complete Acceleration Structure desc
	{
		const auto pResultBuffer = static_cast<ID3D12Resource*>(m_results[m_currentFrame]->GetHandle());
		if (update && (m_buildDesc.Inputs.Flags & D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE)
			== D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE)
		{
			m_buildDesc.SourceAccelerationStructureData = pResultBuffer->GetGPUVirtualAddress();
			m_buildDesc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
			m_currentFrame = (m_currentFrame + 1) % g_frameCount;
		}

		m_buildDesc.DestAccelerationStructureData = pResultBuffer->GetGPUVirtualAddress();
		m_buildDesc.Inputs.InstanceDescs = dynamic_cast<const Resource_DX12*>(pInstanceDescs)->GetGPUVirtualAddress();
		m_buildDesc.ScratchAccelerationStructureData = dynamic_cast<const Resource_DX12*>(pScratch)->GetGPUVirtualAddress();
	}

	// Build acceleration structure.
	pCommandList->BuildRaytracingAccelerationStructure(&m_buildDesc, 0, nullptr, descriptorPool);
}

#if !ENABLE_DXR_FALLBACK
void TopLevelAS_DX12::Build(XUSG::CommandList* pCommandList, const Resource* pScratch,
	const Resource* pInstanceDescs, const DescriptorPool& descriptorPool, bool update)
{
	const auto commandList = CommandList::MakeUnique(pCommandList);
	Build(commandList.get(), pScratch, pInstanceDescs, descriptorPool, update);
}
#endif

void TopLevelAS_DX12::SetInstances(const RayTracing::Device* pDevice, Resource* pInstances,
	uint32_t numInstances, const BottomLevelAS* const* ppBottomLevelASs, float* const* transforms)
{
	const auto pDxInstances = static_cast<ID3D12Resource*>(pInstances->GetHandle());

#if ENABLE_DXR_FALLBACK
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
		memcpy(instanceDescs[i].Transform, transforms[i], sizeof(instanceDescs[i].Transform));
		instanceDescs[i].InstanceMask = 1;
		instanceDescs[i].AccelerationStructure = ppBottomLevelASs[i]->GetResultPointer();
	}

	if (pDxInstances)
	{
		void* pMappedData;
		pDxInstances->Map(0, nullptr, &pMappedData);
		memcpy(pMappedData, instanceDescs.data(), sizeof(D3D12_RAYTRACING_FALLBACK_INSTANCE_DESC) * numInstances);
		pDxInstances->Unmap(0, nullptr);
	}
	else AccelerationStructure_DX12::AllocateUploadBuffer(pDevice, pInstances,
		sizeof(D3D12_RAYTRACING_FALLBACK_INSTANCE_DESC) * numInstances, instanceDescs.data());
#else // DirectX Raytracing
	vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs(numInstances);
	for (auto i = 0u; i < numInstances; ++i)
	{
		const auto pResource = static_cast<ID3D12Resource*>(ppBottomLevelASs[i]->GetResult()->GetHandle());
		memcpy(instanceDescs[i].Transform, transforms[i], sizeof(instanceDescs[i].Transform));
		instanceDescs[i].InstanceMask = 1;
		instanceDescs[i].AccelerationStructure = pResource->GetGPUVirtualAddress();
	}

	if (pDxInstances)
	{
		void* pMappedData;
		pDxInstances->Map(0, nullptr, &pMappedData);
		memcpy(pMappedData, instanceDescs.data(), sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * numInstances);
		pDxInstances->Unmap(0, nullptr);
	}
	else AccelerationStructure_DX12::AllocateUploadBuffer(pDevice, pInstances,
		sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * numInstances, instanceDescs.data());
#endif
}
