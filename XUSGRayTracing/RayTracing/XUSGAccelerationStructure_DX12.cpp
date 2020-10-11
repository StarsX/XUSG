//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

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

bool AccelerationStructure_DX12::AllocateUAVBuffer(const RayTracing::Device& device, Resource& resource,
	size_t byteWidth, ResourceState dstState)
{
	const auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	const auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(byteWidth, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	V_RETURN(device.Common->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc,
		static_cast<D3D12_RESOURCE_STATES>(dstState), nullptr, IID_PPV_ARGS(&resource)), cerr, false);

	return true;
}

bool AccelerationStructure_DX12::AllocateUploadBuffer(const RayTracing::Device& device, Resource& resource,
	size_t byteWidth, void* pData)
{
	const auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	const auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(byteWidth);

	V_RETURN(device.Common->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE,
		&bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&resource)), cerr, false);

	void* pMappedData;
	resource->Map(0, nullptr, &pMappedData);
	memcpy(pMappedData, pData, byteWidth);
	resource->Unmap(0, nullptr);

	return true;
}

bool AccelerationStructure_DX12::preBuild(const RayTracing::Device& device, uint32_t descriptorIndex, uint32_t numSRVs)
{
	const auto& inputs = m_buildDesc.Inputs;

	m_prebuildInfo = {};
#if ENABLE_DXR_FALLBACK
	if (device.RaytracingAPI == API::FallbackLayer)
		device.Fallback->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &m_prebuildInfo, g_numUAVs);
	else // DirectX Raytracing
#endif
		device.Native->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &m_prebuildInfo);

	N_RETURN(m_prebuildInfo.ResultDataMaxSizeInBytes > 0, false);

	// Allocate resources for acceleration structures.
	// Acceleration structures can only be placed in resources that are created in the default heap (or custom heap equivalent). 
	// Default heap is OK since the application doesn’t need CPU read/write access to them. 
	// The resources that will contain acceleration structures must be created in the state D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, 
	// and must have resource flag D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS. The ALLOW_UNORDERED_ACCESS requirement simply acknowledges both: 
	//  - the system will be doing this type of access in its implementation of acceleration structure builds behind the scenes.
	//  - from the app point of view, synchronization of writes/reads to acceleration structures is accomplished using UAV barriers.
	const auto bufferCount = (inputs.Flags & D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE)
		== D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE ? g_frameCount : 1;

	m_results.resize(bufferCount);
#if ENABLE_DXR_FALLBACK
	const auto resourceFlags = device.RaytracingAPI == API::FallbackLayer ? ResourceFlag::ALLOW_UNORDERED_ACCESS : ResourceFlag::ACCELERATION_STRUCTURE;
#else
	const auto resourceFlags = ResourceFlag::ACCELERATION_STRUCTURE;
#endif
	for (auto& result : m_results)
	{
		result = RawBuffer::MakeShared();
		N_RETURN(result->Create(device.Common, GetResultDataMaxSize(),
			resourceFlags, MemoryType::DEFAULT, numSRVs), false);
	}

#if ENABLE_DXR_FALLBACK
	// The Fallback Layer interface uses WRAPPED_GPU_POINTER to encapsulate the underlying pointer
	// which will either be an emulated GPU pointer for the compute - based path or a GPU_VIRTUAL_ADDRESS for the DXR path.
	if (device.RaytracingAPI == API::FallbackLayer)
	{
		m_pointers.resize(bufferCount);
		for (auto i = 0u; i < bufferCount; ++i)
			m_pointers[i] = device.Fallback->GetWrappedPointerSimple(descriptorIndex,
				m_results[i]->GetResource()->GetGPUVirtualAddress());
	}
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

bool BottomLevelAS_DX12::PreBuild(const RayTracing::Device& device, uint32_t numDescs,
	const Geometry* pGeometries, uint32_t descriptorIndex, BuildFlags flags)
{
	m_buildDesc = {};
	auto& inputs = m_buildDesc.Inputs;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = static_cast<decltype(inputs.Flags)>(flags);
	inputs.NumDescs = numDescs;
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	inputs.pGeometryDescs = pGeometries;

	// Get required sizes for an acceleration structure.
	return preBuild(device, descriptorIndex);
}

void BottomLevelAS_DX12::Build(const RayTracing::CommandList* pCommandList, const Resource& scratch,
	const DescriptorPool& descriptorPool, bool update)
{
	// Complete Acceleration Structure desc
	{
		if (update && (m_buildDesc.Inputs.Flags & D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE)
			== D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE)
		{
			m_buildDesc.SourceAccelerationStructureData = m_results[m_currentFrame]->GetResource()->GetGPUVirtualAddress();
			m_buildDesc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
			m_currentFrame = (m_currentFrame + 1) % g_frameCount;
		}

		m_buildDesc.DestAccelerationStructureData = m_results[m_currentFrame]->GetResource()->GetGPUVirtualAddress();
		m_buildDesc.ScratchAccelerationStructureData = scratch->GetGPUVirtualAddress();
	}

	// Build acceleration structure.
	pCommandList->BuildRaytracingAccelerationStructure(&m_buildDesc, 0, nullptr, descriptorPool);

	// Resource barrier
	pCommandList->Barrier(1, &ResourceBarrier::UAV(m_results[m_currentFrame]->GetResource().get()));
}

#if !ENABLE_DXR_FALLBACK
void BottomLevelAS_DX12::Build(XUSG::CommandList* pCommandList, const Resource& scratch,
	const DescriptorPool& descriptorPool, bool update)
{
	const auto commandList = RayTracing::CommandList::MakeUnique(*pCommandList);
	Build(commandList.get(), scratch, descriptorPool, update);
}
#endif

void BottomLevelAS_DX12::SetTriangleGeometries(Geometry* pGeometries, uint32_t numGeometries,
	Format vertexFormat, const VertexBufferView* pVBs, const IndexBufferView* pIBs,
	const GeometryFlags* pGeometryFlags, const ResourceView* pTransforms)
{
	for (auto i = 0u; i < numGeometries; ++i)
	{
		auto& geometryDesc = pGeometries[i];
		
		auto strideIB = 0u;
		if (pIBs)
		{
			assert(pIBs[i].Format == DXGI_FORMAT_R32_UINT || pIBs[i].Format == DXGI_FORMAT_R16_UINT);
			strideIB = pIBs[i].Format == DXGI_FORMAT_R32_UINT ? sizeof(uint32_t) : sizeof(uint16_t);
		}

		geometryDesc = {};
		geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		geometryDesc.Triangles.Transform3x4 = pTransforms ? pTransforms[i].resource->GetGPUVirtualAddress() + pTransforms[i].offset: 0;
		geometryDesc.Triangles.IndexFormat = pIBs ? pIBs[i].Format : DXGI_FORMAT_UNKNOWN;
		geometryDesc.Triangles.VertexFormat = static_cast<decltype(geometryDesc.Triangles.VertexFormat)>(vertexFormat);
		geometryDesc.Triangles.IndexCount = pIBs ? pIBs[i].SizeInBytes / strideIB : 0;
		geometryDesc.Triangles.VertexCount = pVBs ? pVBs[i].SizeInBytes / pVBs[i].StrideInBytes : 0;
		geometryDesc.Triangles.IndexBuffer = pIBs ? pIBs[i].BufferLocation : 0;
		geometryDesc.Triangles.VertexBuffer.StartAddress = pVBs ? pVBs[i].BufferLocation : 0;
		geometryDesc.Triangles.VertexBuffer.StrideInBytes = pVBs ? pVBs[i].StrideInBytes : 0;

		// Mark the geometry as opaque. 
		// PERFORMANCE TIP: mark geometry as opaque whenever applicable as it can enable important ray processing optimizations.
		// Note: When rays encounter opaque geometry an any hit shader will not be executed whether it is present or not.
		geometryDesc.Flags = static_cast<decltype(geometryDesc.Flags)>(pGeometryFlags ? pGeometryFlags[i] : GeometryFlags::FULL_OPAQUE);
	}
}

void BottomLevelAS_DX12::SetAABBGeometries(Geometry* pGeometries, uint32_t numGeometries,
	const VertexBufferView* pVBs, const GeometryFlags* pGeometryFlags)
{
	for (auto i = 0u; i < numGeometries; ++i)
	{
		auto& geometryDesc = pGeometries[i];

		geometryDesc = {};
		geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
		geometryDesc.AABBs.AABBCount = pVBs ? pVBs[i].SizeInBytes / pVBs[i].StrideInBytes : 0;
		geometryDesc.AABBs.AABBs.StartAddress = pVBs ? pVBs[i].BufferLocation : 0;
		geometryDesc.AABBs.AABBs.StrideInBytes = pVBs ? pVBs[i].StrideInBytes : sizeof(D3D12_RAYTRACING_AABB);

		// Mark the geometry as opaque. 
		// PERFORMANCE TIP: mark geometry as opaque whenever applicable as it can enable important ray processing optimizations.
		// Note: When rays encounter opaque geometry an any hit shader will not be executed whether it is present or not.
		geometryDesc.Flags = static_cast<decltype(geometryDesc.Flags)>(pGeometryFlags ? pGeometryFlags[i] : GeometryFlags::FULL_OPAQUE);
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

bool TopLevelAS_DX12::PreBuild(const RayTracing::Device& device, uint32_t numDescs,
	uint32_t descriptorIndex, BuildFlags flags)
{
	m_buildDesc = {};
	auto& inputs = m_buildDesc.Inputs;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = static_cast<decltype(inputs.Flags)>(flags);
	inputs.NumDescs = numDescs;
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

	// Get required sizes for an acceleration structure.
	return preBuild(device, descriptorIndex, 1);
}

void TopLevelAS_DX12::Build(const RayTracing::CommandList* pCommandList, const Resource& scratch,
	const Resource& instanceDescs, const DescriptorPool& descriptorPool, bool update)
{
	// Complete Acceleration Structure desc
	{
		if (update && (m_buildDesc.Inputs.Flags & D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE)
			== D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE)
		{
			m_buildDesc.SourceAccelerationStructureData = m_results[m_currentFrame]->GetResource()->GetGPUVirtualAddress();
			m_buildDesc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
			m_currentFrame = (m_currentFrame + 1) % g_frameCount;
		}

		m_buildDesc.DestAccelerationStructureData = m_results[m_currentFrame]->GetResource()->GetGPUVirtualAddress();
		m_buildDesc.Inputs.InstanceDescs = instanceDescs->GetGPUVirtualAddress();
		m_buildDesc.ScratchAccelerationStructureData = scratch->GetGPUVirtualAddress();
	}

	// Build acceleration structure.
	pCommandList->BuildRaytracingAccelerationStructure(&m_buildDesc, 0, nullptr, descriptorPool);
}

#if !ENABLE_DXR_FALLBACK
void TopLevelAS_DX12::Build(XUSG::CommandList* pCommandList, const Resource& scratch,
	const Resource& instanceDescs, const DescriptorPool& descriptorPool, bool update)
{
	const auto commandList = RayTracing::CommandList::MakeUnique(*pCommandList);
	Build(commandList.get(), scratch, instanceDescs, descriptorPool, update);
}
#endif

void TopLevelAS_DX12::SetInstances(const RayTracing::Device& device, Resource& instances,
	uint32_t numInstances, const BottomLevelAS* const* ppBottomLevelASs, float* const* transforms)
{
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
	if (device.RaytracingAPI == API::FallbackLayer)
	{
		vector<D3D12_RAYTRACING_FALLBACK_INSTANCE_DESC> instanceDescs(numInstances);
		for (auto i = 0u; i < numInstances; ++i)
		{
			memcpy(instanceDescs[i].Transform, transforms[i], sizeof(instanceDescs[i].Transform));
			instanceDescs[i].InstanceMask = 1;
			instanceDescs[i].AccelerationStructure = ppBottomLevelASs[i]->GetResultPointer();
		}

		if (instances)
		{
			void* pMappedData;
			instances->Map(0, nullptr, &pMappedData);
			memcpy(pMappedData, instanceDescs.data(), sizeof(D3D12_RAYTRACING_FALLBACK_INSTANCE_DESC) * numInstances);
			instances->Unmap(0, nullptr);
		}
		else AccelerationStructure_DX12::AllocateUploadBuffer(device, instances, sizeof(D3D12_RAYTRACING_FALLBACK_INSTANCE_DESC) * numInstances, instanceDescs.data());
	}
	else // DirectX Raytracing
#endif
	{
		vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs(numInstances);
		for (auto i = 0u; i < numInstances; ++i)
		{
			memcpy(instanceDescs[i].Transform, transforms[i], sizeof(instanceDescs[i].Transform));
			instanceDescs[i].InstanceMask = 1;
			instanceDescs[i].AccelerationStructure = ppBottomLevelASs[i]->GetResult()->GetResource()->GetGPUVirtualAddress();
		}

		if (instances)
		{
			void* pMappedData;
			instances->Map(0, nullptr, &pMappedData);
			memcpy(pMappedData, instanceDescs.data(), sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * numInstances);
			instances->Unmap(0, nullptr);
		}
		else AccelerationStructure_DX12::AllocateUploadBuffer(device, instances, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * numInstances, instanceDescs.data());
	}
}
