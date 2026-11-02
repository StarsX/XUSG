//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSG_DX12.h"
#include "Core/XUSGResource_DX12.h"
#include "XUSGRayTracing_DX12.h"
#include "XUSGCluster_DX12.h"

using namespace std;
using namespace XUSG;
using namespace XUSG::RayTracing;

static_assert(sizeof(RTASPrebuildInfo) == sizeof(D3D12_RTAS_OPERATION_PREBUILD_INFO),
	"RTASPrebuildInfo must match D3D12_RTAS_OPERATION_PREBUILD_INFO");
static_assert(sizeof(RTASClusterLimits) == sizeof(D3D12_RTAS_CLUSTER_LIMITS),
	"RTASClusterLimits must match D3D12_RTAS_CLUSTER_LIMITS");
static_assert(sizeof(RTASClusterTrianglesInputs) == sizeof(D3D12_RTAS_CLUSTER_TRIANGLES_INPUTS_DESC),
	"RTASClusterTrianglesInputs must match D3D12_RTAS_CLUSTER_TRIANGLES_INPUTS_DESC");
static_assert(sizeof(RTASClusterTemplateTrianglesInputs) == sizeof(D3D12_RTAS_CLUSTER_TEMPLATE_TRIANGLES_INPUTS_DESC),
	"RTASClusterTemplateTrianglesInputs must match D3D12_RTAS_CLUSTER_TEMPLATE_TRIANGLES_INPUTS_DESC");
static_assert(sizeof(RTASInstantiateClusterTemplateInputs) == sizeof(D3D12_RTAS_INSTANTIATE_CLUSTER_TEMPLATE_INPUTS_DESC),
	"RTASInstantiateClusterTemplateInputs must match D3D12_RTAS_INSTANTIATE_CLUSTER_TEMPLATE_INPUTS_DESC");
static_assert(sizeof(RTASClasInputs) == sizeof(D3D12_RTAS_CLAS_INPUTS_DESC),
	"RTASClasInputs must match D3D12_RTAS_CLAS_INPUTS_DESC");
static_assert(sizeof(RTASClusterMoves) == sizeof(D3D12_RTAS_CLUSTER_MOVES_DESC),
	"RTASClusterMoves must match D3D12_RTAS_CLUSTER_MOVES_DESC");
static_assert(sizeof(RTASBatchedOperationData) == sizeof(D3D12_RTAS_BATCHED_OPERATION_DATA),
	"RTASBatchedOperationData must match D3D12_RTAS_BATCHED_OPERATION_DATA");
static_assert(sizeof(RTASPartitionedTLASInputs) == sizeof(D3D12_RTAS_PARTITIONED_TLAS_INPUTS_DESC),
	"RTASPartitionedTLASInputs must match D3D12_RTAS_PARTITIONED_TLAS_INPUTS_DESC");
static_assert(sizeof(RTASPartitionedTLASOperationData) == sizeof(D3D12_RTAS_PARTITIONED_TLAS_OPERATION_DATA),
	"RTASPartitionedTLASOperationData must match D3D12_RTAS_PARTITIONED_TLAS_OPERATION_DATA");
static_assert(sizeof(RTASOperationInputs) == sizeof(D3D12_RTAS_OPERATION_INPUTS),
	"RTASOperationInputs must match D3D12_RTAS_OPERATION_INPUTS");
static_assert(sizeof(RTASOperationDesc) == sizeof(D3D12_RTAS_OPERATION_DESC),
	"RTASOperationDesc must match D3D12_RTAS_OPERATION_DESC");
static_assert(sizeof(GpuVirtualAddressAndStride) == sizeof(D3D12_GPU_VIRTUAL_ADDRESS_AND_STRIDE),
	"GpuVirtualAddressAndStride must match D3D12_GPU_VIRTUAL_ADDRESS_AND_STRIDE");
static_assert(sizeof(RTASBuildClasFromTrianglesArgs) == sizeof(D3D12_RTAS_OPERATION_BUILD_CLAS_FROM_TRIANGLES_ARGS),
	"RTASBuildClasFromTrianglesArgs must match D3D12_RTAS_OPERATION_BUILD_CLAS_FROM_TRIANGLES_ARGS");
static_assert(offsetof(RTASBuildClasFromTrianglesArgs, ClusterID) == offsetof(D3D12_RTAS_OPERATION_BUILD_CLAS_FROM_TRIANGLES_ARGS, ClusterID),
	"CLAS argument ClusterID offset must match D3D12");
static_assert(offsetof(RTASBuildClasFromTrianglesArgs, VertexBuffer) == offsetof(D3D12_RTAS_OPERATION_BUILD_CLAS_FROM_TRIANGLES_ARGS, VertexBuffer),
	"CLAS argument VertexBuffer offset must match D3D12");
static_assert(sizeof(RTASBuildBlasFromClasArgs) == sizeof(D3D12_RTAS_OPERATION_BUILD_BLAS_FROM_CLAS_ARGS),
	"RTASBuildBlasFromClasArgs must match D3D12_RTAS_OPERATION_BUILD_BLAS_FROM_CLAS_ARGS");
static_assert(sizeof(RTASMoveClusterObjectsArgs) == sizeof(D3D12_RTAS_OPERATION_MOVE_CLUSTER_OBJECTS_ARGS),
	"RTASMoveClusterObjectsArgs must match D3D12_RTAS_OPERATION_MOVE_CLUSTER_OBJECTS_ARGS");
static_assert(sizeof(RTASBuildClusterTemplatesFromTrianglesArgs) == sizeof(D3D12_RTAS_OPERATION_BUILD_CLUSTER_TEMPLATES_FROM_TRIANGLES_ARGS),
	"RTASBuildClusterTemplatesFromTrianglesArgs must match D3D12_RTAS_OPERATION_BUILD_CLUSTER_TEMPLATES_FROM_TRIANGLES_ARGS");
static_assert(sizeof(RTASInstantiateClusterTemplatesArgs) == sizeof(D3D12_RTAS_OPERATION_INSTANTIATE_CLUSTER_TEMPLATES_ARGS),
	"RTASInstantiateClusterTemplatesArgs must match D3D12_RTAS_OPERATION_INSTANTIATE_CLUSTER_TEMPLATES_ARGS");
static_assert(sizeof(RTASPartitionedTLASOperation) == sizeof(D3D12_RTAS_PARTITIONED_TLAS_OPERATION),
	"RTASPartitionedTLASOperation must match D3D12_RTAS_PARTITIONED_TLAS_OPERATION");
static_assert(sizeof(RTASPartitionedTLASWriteInstanceArgs) == sizeof(D3D12_RTAS_PARTITIONED_TLAS_OPERATION_WRITE_INSTANCE_ARGS),
	"RTASPartitionedTLASWriteInstanceArgs must match D3D12_RTAS_PARTITIONED_TLAS_OPERATION_WRITE_INSTANCE_ARGS");
static_assert(sizeof(RTASPartitionedTLASUpdateInstanceArgs) == sizeof(D3D12_RTAS_PARTITIONED_TLAS_OPERATION_UPDATE_INSTANCE_ARGS),
	"RTASPartitionedTLASUpdateInstanceArgs must match D3D12_RTAS_PARTITIONED_TLAS_OPERATION_UPDATE_INSTANCE_ARGS");
static_assert(sizeof(RTASPartitionedTLASTranslatePartitionArgs) == sizeof(D3D12_RTAS_PARTITIONED_TLAS_OPERATION_TRANSLATE_PARTITION_ARGS),
	"RTASPartitionedTLASTranslatePartitionArgs must match D3D12_RTAS_PARTITIONED_TLAS_OPERATION_TRANSLATE_PARTITION_ARGS");
static_assert(static_cast<uint32_t>(RayTracing::PipelineFlag::ALLOW_CLUSTERED_GEOMETRY) == D3D12_RAYTRACING_PIPELINE_FLAG_ALLOW_CLUSTERED_GEOMETRY,
	"ALLOW_CLUSTERED_GEOMETRY must match D3D12_RAYTRACING_PIPELINE_FLAG_ALLOW_CLUSTERED_GEOMETRY");
static_assert(offsetof(RTASOperationInputs, Type) == offsetof(D3D12_RTAS_OPERATION_INPUTS, Type),
	"RTASOperationInputs::Type offset must match D3D12");
static_assert(offsetof(RTASOperationInputs, pClusterTrianglesDesc) == offsetof(D3D12_RTAS_OPERATION_INPUTS, pClusterTrianglesDesc),
	"RTASOperationInputs::pClusterTrianglesDesc offset must match D3D12");

namespace
{
	bool allocateNativeRTAS(RayTracing::Device* pDevice, Buffer::sptr& resource, uintptr_t& byteOffset, size_t byteWidth,
		uint32_t numSRVs, MemoryFlag memoryFlags, const wchar_t* name, uint32_t maxThreads)
	{
		const auto pDxDevice = static_cast<ID3D12RaytracingFallbackDevice*>(pDevice->GetRTHandle());
		assert(pDxDevice);
		assert(pDxDevice->UsingRaytracingDriver());
		assert(pDevice->HasClustersAndPTLAS());

		resource = Buffer::MakeShared();
		byteOffset = 0;

		return resource->Create(pDevice, byteWidth, ResourceFlag::ACCELERATION_STRUCTURE, MemoryType::DEFAULT,
			numSRVs, nullptr, 0, nullptr, memoryFlags, name, nullptr, maxThreads);
	}

	void executeBatched(RayTracing::CommandList* pCommandList, const RTASOperationInputs& inputs, uint64_t result,
		uint64_t scratch, uint64_t indirectArgumentAddress, uint64_t indirectArgumentStride)
	{
		RTASBatchedOperationData batched = {};
		batched.AddressResolutionFlags = RTASAddressResolutionFlag::NONE;
		batched.BatchResultData = result;
		batched.BatchScratchData = scratch;
		batched.IndirectArgumentArray.StartAddress = indirectArgumentAddress;
		batched.IndirectArgumentArray.StrideInBytes = indirectArgumentStride;

		RTASOperationDesc desc = {};
		desc.Inputs = inputs;
		desc.pBatchedOperationData = &batched;
		pCommandList->ExecuteIndirectRTASOperations(1, &desc);
	}
}

ClusterLevelAS_DX12::ClusterLevelAS_DX12() :
	m_byteOffset(0),
	m_type(RTASOperationType::BUILD_CLAS_FROM_TRIANGLES)
{
	memset(&m_prebuildInfo, 0, sizeof(m_prebuildInfo));
	memset(&m_triangles, 0, sizeof(m_triangles));
	memset(&m_instantiate, 0, sizeof(m_instantiate));
}

ClusterLevelAS_DX12::~ClusterLevelAS_DX12()
{
}

void ClusterLevelAS_DX12::SetTriangles(const RTASClusterTrianglesInputs& desc)
{
	m_type = RTASOperationType::BUILD_CLAS_FROM_TRIANGLES;
	m_triangles = desc;
}

void ClusterLevelAS_DX12::SetTemplateInstantiation(const RTASInstantiateClusterTemplateInputs& desc)
{
	m_type = RTASOperationType::INSTANTIATE_CLUSTER_TEMPLATES;
	m_instantiate = desc;
}

bool ClusterLevelAS_DX12::Prebuild(Device* pDevice)
{
	assert(pDevice);

	RTASOperationInputs inputs = {};
	inputs.Type = m_type;
	if (m_type == RTASOperationType::INSTANTIATE_CLUSTER_TEMPLATES)
		inputs.pInstantiateClusterTemplateDesc = &m_instantiate;
	else
		inputs.pClusterTrianglesDesc = &m_triangles;

	pDevice->GetRTASOperationPrebuildInfo(inputs, m_prebuildInfo);

	return m_prebuildInfo.ResultDataMaxByteSize || m_prebuildInfo.ScratchDataByteSize;
}

bool ClusterLevelAS_DX12::Allocate(Device* pDevice, size_t byteWidth, MemoryFlag memoryFlags,
	const wchar_t* name, uint32_t maxThreads)
{
	byteWidth = byteWidth ? byteWidth : m_prebuildInfo.ResultDataMaxByteSize;

	return allocateNativeRTAS(pDevice, m_resource, m_byteOffset, byteWidth, 0, memoryFlags, name, maxThreads);
}

void ClusterLevelAS_DX12::Build(CommandList* pCommandList, const Resource* pScratch,
	uint64_t indirectArgumentAddress, uint64_t indirectArgumentStride)
{
	assert(pCommandList);
	assert(pScratch);
	assert(m_resource);

	RTASOperationInputs inputs = {};
	inputs.Type = m_type;
	if (m_type == RTASOperationType::INSTANTIATE_CLUSTER_TEMPLATES)
	{
		inputs.pInstantiateClusterTemplateDesc = &m_instantiate;
		if (!indirectArgumentStride) indirectArgumentStride = sizeof(RTASInstantiateClusterTemplatesArgs);
	}
	else
	{
		inputs.pClusterTrianglesDesc = &m_triangles;
		if (!indirectArgumentStride) indirectArgumentStride = sizeof(RTASBuildClasFromTrianglesArgs);
	}

	executeBatched(pCommandList, inputs, GetVirtualAddress(), pScratch->GetVirtualAddress(),
		indirectArgumentAddress, indirectArgumentStride);
}

uint32_t ClusterLevelAS_DX12::SetBarrier(ResourceBarrier* pBarriers, uint32_t numBarriers)
{
	return AccelerationStructure::SetBarrier(pBarriers, m_resource.get(), numBarriers);
}

const RTASPrebuildInfo& ClusterLevelAS_DX12::GetPrebuildInfo() const
{
	return m_prebuildInfo;
}

size_t ClusterLevelAS_DX12::GetResultDataMaxByteSize(bool isAligned) const
{
	return isAligned ? Align(m_prebuildInfo.ResultDataMaxByteSize) : m_prebuildInfo.ResultDataMaxByteSize;
}

size_t ClusterLevelAS_DX12::GetScratchDataByteSize(bool isAligned) const
{
	return isAligned ? Align(m_prebuildInfo.ScratchDataByteSize) : m_prebuildInfo.ScratchDataByteSize;
}

uint64_t ClusterLevelAS_DX12::GetVirtualAddress() const
{
	return m_resource->GetVirtualAddress() + m_byteOffset;
}

void* ClusterLevelAS_DX12::GetBackend()
{
	return this;
}

size_t ClusterLevelAS_DX12::Align(size_t byteSize)
{
	return XUSG::Align(byteSize, D3D12_RAYTRACING_CLAS_BYTE_ALIGNMENT);
}

ClusterTemplate_DX12::ClusterTemplate_DX12() :
	m_byteOffset(0)
{
	memset(&m_prebuildInfo, 0, sizeof(m_prebuildInfo));
	memset(&m_triangles, 0, sizeof(m_triangles));
}

ClusterTemplate_DX12::~ClusterTemplate_DX12()
{
}

void ClusterTemplate_DX12::SetTriangles(const RTASClusterTemplateTrianglesInputs& desc)
{
	m_triangles = desc;
}

bool ClusterTemplate_DX12::Prebuild(Device* pDevice)
{
	assert(pDevice);

	RTASOperationInputs inputs = {};
	inputs.Type = RTASOperationType::BUILD_CLUSTER_TEMPLATES_FROM_TRIANGLES;
	inputs.pClusterTemplateTrianglesDesc = &m_triangles;
	pDevice->GetRTASOperationPrebuildInfo(inputs, m_prebuildInfo);

	return m_prebuildInfo.ResultDataMaxByteSize || m_prebuildInfo.ScratchDataByteSize;
}

bool ClusterTemplate_DX12::Allocate(Device* pDevice, size_t byteWidth, MemoryFlag memoryFlags,
	const wchar_t* name, uint32_t maxThreads)
{
	byteWidth = byteWidth ? byteWidth : m_prebuildInfo.ResultDataMaxByteSize;

	return allocateNativeRTAS(pDevice, m_resource, m_byteOffset, byteWidth, 0, memoryFlags, name, maxThreads);
}

void ClusterTemplate_DX12::Build(CommandList* pCommandList, const Resource* pScratch,
	uint64_t indirectArgumentAddress, uint64_t indirectArgumentStride)
{
	assert(pCommandList);
	assert(pScratch);
	assert(m_resource);

	if (!indirectArgumentStride) indirectArgumentStride = sizeof(RTASBuildClusterTemplatesFromTrianglesArgs);

	RTASOperationInputs inputs = {};
	inputs.Type = RTASOperationType::BUILD_CLUSTER_TEMPLATES_FROM_TRIANGLES;
	inputs.pClusterTemplateTrianglesDesc = &m_triangles;
	executeBatched(pCommandList, inputs, GetVirtualAddress(), pScratch->GetVirtualAddress(),
		indirectArgumentAddress, indirectArgumentStride);
}

void ClusterTemplate_DX12::Instantiate(CommandList* pCommandList, ClusterLevelAS* pDestination,
	const Resource* pScratch, uint64_t indirectArgumentAddress, uint64_t indirectArgumentStride)
{
	assert(pDestination);

	pDestination->Build(pCommandList, pScratch, indirectArgumentAddress, indirectArgumentStride);
}

uint32_t ClusterTemplate_DX12::SetBarrier(ResourceBarrier* pBarriers, uint32_t numBarriers)
{
	return AccelerationStructure::SetBarrier(pBarriers, m_resource.get(), numBarriers);
}

const RTASPrebuildInfo& ClusterTemplate_DX12::GetPrebuildInfo() const
{
	return m_prebuildInfo;
}

size_t ClusterTemplate_DX12::GetResultDataMaxByteSize(bool isAligned) const
{
	return isAligned ? Align(m_prebuildInfo.ResultDataMaxByteSize) : m_prebuildInfo.ResultDataMaxByteSize;
}

size_t ClusterTemplate_DX12::GetScratchDataByteSize(bool isAligned) const
{
	return isAligned ? Align(m_prebuildInfo.ScratchDataByteSize) : m_prebuildInfo.ScratchDataByteSize;
}

uint64_t ClusterTemplate_DX12::GetVirtualAddress() const
{
	return m_resource->GetVirtualAddress() + m_byteOffset;
}

void* ClusterTemplate_DX12::GetBackend()
{
	return this;
}

size_t ClusterTemplate_DX12::Align(size_t byteSize)
{
	return XUSG::Align(byteSize, D3D12_RAYTRACING_CLUSTER_TEMPLATE_BYTE_ALIGNMENT);
}

ClusterBottomLevelAS_DX12::ClusterBottomLevelAS_DX12() :
	m_byteOffset(0)
{
	memset(&m_prebuildInfo, 0, sizeof(m_prebuildInfo));
	memset(&m_clusters, 0, sizeof(m_clusters));
}

ClusterBottomLevelAS_DX12::~ClusterBottomLevelAS_DX12()
{
}

void ClusterBottomLevelAS_DX12::SetClusters(const RTASClasInputs& desc)
{
	m_clusters = desc;
}

bool ClusterBottomLevelAS_DX12::Prebuild(Device* pDevice)
{
	assert(pDevice);

	RTASOperationInputs inputs = {};
	inputs.Type = RTASOperationType::BUILD_BLAS_FROM_CLAS;
	inputs.pClasDesc = &m_clusters;
	pDevice->GetRTASOperationPrebuildInfo(inputs, m_prebuildInfo);

	return m_prebuildInfo.ResultDataMaxByteSize || m_prebuildInfo.ScratchDataByteSize;
}

bool ClusterBottomLevelAS_DX12::Allocate(Device* pDevice, size_t byteWidth, MemoryFlag memoryFlags,
	const wchar_t* name, uint32_t maxThreads)
{
	byteWidth = byteWidth ? byteWidth : m_prebuildInfo.ResultDataMaxByteSize;

	return allocateNativeRTAS(pDevice, m_resource, m_byteOffset, byteWidth, 0, memoryFlags, name, maxThreads);
}

void ClusterBottomLevelAS_DX12::Build(CommandList* pCommandList, const Resource* pScratch,
	uint64_t indirectArgumentAddress, uint64_t indirectArgumentStride)
{
	assert(pCommandList);
	assert(pScratch);
	assert(m_resource);

	if (!indirectArgumentStride) indirectArgumentStride = sizeof(RTASBuildBlasFromClasArgs);

	RTASOperationInputs inputs = {};
	inputs.Type = RTASOperationType::BUILD_BLAS_FROM_CLAS;
	inputs.pClasDesc = &m_clusters;
	executeBatched(pCommandList, inputs, GetVirtualAddress(), pScratch->GetVirtualAddress(),
		indirectArgumentAddress, indirectArgumentStride);
}

uint32_t ClusterBottomLevelAS_DX12::SetBarrier(ResourceBarrier* pBarriers, uint32_t numBarriers)
{
	return AccelerationStructure::SetBarrier(pBarriers, m_resource.get(), numBarriers);
}

const RTASPrebuildInfo& ClusterBottomLevelAS_DX12::GetPrebuildInfo() const
{
	return m_prebuildInfo;
}

size_t ClusterBottomLevelAS_DX12::GetResultDataMaxByteSize(bool isAligned) const
{
	return isAligned ? Align(m_prebuildInfo.ResultDataMaxByteSize) : m_prebuildInfo.ResultDataMaxByteSize;
}

size_t ClusterBottomLevelAS_DX12::GetScratchDataByteSize(bool isAligned) const
{
	return isAligned ? Align(m_prebuildInfo.ScratchDataByteSize) : m_prebuildInfo.ScratchDataByteSize;
}

uint64_t ClusterBottomLevelAS_DX12::GetVirtualAddress() const
{
	return m_resource->GetVirtualAddress() + m_byteOffset;
}

void* ClusterBottomLevelAS_DX12::GetBackend()
{
	return this;
}

size_t ClusterBottomLevelAS_DX12::Align(size_t byteSize)
{
	return XUSG::Align(byteSize, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
}

PartitionedTLAS_DX12::PartitionedTLAS_DX12() :
	m_byteOffset(0)
{
	memset(&m_prebuildInfo, 0, sizeof(m_prebuildInfo));
	memset(&m_partitions, 0, sizeof(m_partitions));
}

PartitionedTLAS_DX12::~PartitionedTLAS_DX12()
{
}

void PartitionedTLAS_DX12::SetPartitions(const RTASPartitionedTLASInputs& desc)
{
	m_partitions = desc;
}

bool PartitionedTLAS_DX12::Prebuild(Device* pDevice)
{
	assert(pDevice);

	RTASOperationInputs inputs = {};
	inputs.Type = RTASOperationType::PARTITIONED_TLAS;
	inputs.pPartitionedTLASInputsDesc = &m_partitions;
	pDevice->GetRTASOperationPrebuildInfo(inputs, m_prebuildInfo);

	return m_prebuildInfo.ResultDataMaxByteSize || m_prebuildInfo.ScratchDataByteSize;
}

bool PartitionedTLAS_DX12::Allocate(Device* pDevice, size_t byteWidth, MemoryFlag memoryFlags,
	const wchar_t* name, uint32_t maxThreads)
{
	byteWidth = byteWidth ? byteWidth : m_prebuildInfo.ResultDataMaxByteSize;

	return allocateNativeRTAS(pDevice, m_resource, m_byteOffset, byteWidth, 1, memoryFlags, name, maxThreads);
}

void PartitionedTLAS_DX12::Build(CommandList* pCommandList, const Resource* pScratch,
	uint64_t indirectOps, uint64_t indirectOpCount)
{
	build(pCommandList, pScratch, indirectOps, indirectOpCount, 0);
}

void PartitionedTLAS_DX12::Update(CommandList* pCommandList, const Resource* pScratch,
	uint64_t indirectOps, uint64_t indirectOpCount)
{
	build(pCommandList, pScratch, indirectOps, indirectOpCount, GetVirtualAddress());
}

uint32_t PartitionedTLAS_DX12::SetBarrier(ResourceBarrier* pBarriers, uint32_t numBarriers)
{
	return AccelerationStructure::SetBarrier(pBarriers, m_resource.get(), numBarriers);
}

const RTASPrebuildInfo& PartitionedTLAS_DX12::GetPrebuildInfo() const
{
	return m_prebuildInfo;
}

size_t PartitionedTLAS_DX12::GetResultDataMaxByteSize(bool isAligned) const
{
	return isAligned ? Align(m_prebuildInfo.ResultDataMaxByteSize) : m_prebuildInfo.ResultDataMaxByteSize;
}

size_t PartitionedTLAS_DX12::GetScratchDataByteSize(bool isAligned) const
{
	return isAligned ? Align(m_prebuildInfo.ScratchDataByteSize) : m_prebuildInfo.ScratchDataByteSize;
}

uint64_t PartitionedTLAS_DX12::GetVirtualAddress() const
{
	return m_resource->GetVirtualAddress() + m_byteOffset;
}

const Descriptor& PartitionedTLAS_DX12::GetSRV() const
{
	return m_resource->GetSRV();
}

void* PartitionedTLAS_DX12::GetBackend()
{
	return this;
}

size_t PartitionedTLAS_DX12::Align(size_t byteSize)
{
	return XUSG::Align(byteSize, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
}

void PartitionedTLAS_DX12::build(CommandList* pCommandList, const Resource* pScratch,
	uint64_t indirectOps, uint64_t indirectOpCount, uint64_t source)
{
	assert(pCommandList);
	assert(pScratch);
	assert(m_resource);

	RTASOperationInputs inputs = {};
	inputs.Type = RTASOperationType::PARTITIONED_TLAS;
	inputs.pPartitionedTLASInputsDesc = &m_partitions;

	RTASPartitionedTLASOperationData data = {};
	data.SourceAccelerationStructureData = source;
	data.DestAccelerationStructureData = GetVirtualAddress();
	data.ScratchAccelerationStructureData = pScratch->GetVirtualAddress();
	data.IndirectPartitionedTlasOpCount = indirectOpCount;
	data.IndirectPartitionedTlasOps = indirectOps;

	RTASOperationDesc desc = {};
	desc.Inputs = inputs;
	desc.pPartitionedTlasOperationData = &data;
	pCommandList->ExecuteIndirectRTASOperations(1, &desc);
}
