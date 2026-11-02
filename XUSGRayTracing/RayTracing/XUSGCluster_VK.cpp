//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGCluster_VK.h"
#include "XUSGAccelerationStructure_VK.h"

using namespace std;
using namespace XUSG;
using namespace XUSG::RayTracing;

ClusterLevelAS_VK::ClusterLevelAS_VK() :
	m_byteOffset(0)
{
	memset(&m_prebuildInfo, 0, sizeof(m_prebuildInfo));
}

ClusterLevelAS_VK::~ClusterLevelAS_VK()
{
}

void ClusterLevelAS_VK::SetTriangles(const RTASClusterTrianglesInputs& desc)
{
	assert(!"ClusterLevelAS::SetTriangles is not implemented on Vulkan");
	(void)desc;
}

void ClusterLevelAS_VK::SetTemplateInstantiation(const RTASInstantiateClusterTemplateInputs& desc)
{
	assert(!"ClusterLevelAS::SetTemplateInstantiation is not implemented on Vulkan");
	(void)desc;
}

bool ClusterLevelAS_VK::Prebuild(Device* pDevice)
{
	assert(!"ClusterLevelAS::Prebuild is not implemented on Vulkan");
	(void)pDevice;

	return false;
}

bool ClusterLevelAS_VK::Allocate(Device* pDevice, size_t byteWidth, MemoryFlag memoryFlags,
	const wchar_t* name, uint32_t maxThreads)
{
	assert(!"ClusterLevelAS::Allocate is not implemented on Vulkan");
	(void)pDevice;
	(void)byteWidth;
	(void)memoryFlags;
	(void)name;
	(void)maxThreads;

	return false;
}

void ClusterLevelAS_VK::Build(CommandList* pCommandList, const Resource* pScratch,
	uint64_t indirectArgumentAddress, uint64_t indirectArgumentStride)
{
	assert(!"ClusterLevelAS::Build is not implemented on Vulkan");
	(void)pCommandList;
	(void)pScratch;
	(void)indirectArgumentAddress;
	(void)indirectArgumentStride;
}

uint32_t ClusterLevelAS_VK::SetBarrier(ResourceBarrier* pBarriers, uint32_t numBarriers)
{
	assert(!"ClusterLevelAS::SetBarrier is not implemented on Vulkan");
	(void)pBarriers;

	return numBarriers;
}

const RTASPrebuildInfo& ClusterLevelAS_VK::GetPrebuildInfo() const
{
	assert(!"ClusterLevelAS::GetPrebuildInfo is not implemented on Vulkan");

	return m_prebuildInfo;
}

size_t ClusterLevelAS_VK::GetResultDataMaxByteSize(bool isAligned) const
{
	assert(!"ClusterLevelAS::GetResultDataMaxByteSize is not implemented on Vulkan");
	(void)isAligned;

	return 0;
}

size_t ClusterLevelAS_VK::GetScratchDataByteSize(bool isAligned) const
{
	assert(!"ClusterLevelAS::GetScratchDataByteSize is not implemented on Vulkan");
	(void)isAligned;

	return 0;
}

uint64_t ClusterLevelAS_VK::GetVirtualAddress() const
{
	assert(!"ClusterLevelAS::GetVirtualAddress is not implemented on Vulkan");

	return 0;
}

void* ClusterLevelAS_VK::GetBackend()
{
	return this;
}

size_t ClusterLevelAS_VK::Align(size_t byteSize)
{
	return XUSG::Align(byteSize, 128);
}

ClusterTemplate_VK::ClusterTemplate_VK() :
	m_byteOffset(0)
{
	memset(&m_prebuildInfo, 0, sizeof(m_prebuildInfo));
}

ClusterTemplate_VK::~ClusterTemplate_VK()
{
}

void ClusterTemplate_VK::SetTriangles(const RTASClusterTemplateTrianglesInputs& desc)
{
	assert(!"ClusterTemplate::SetTriangles is not implemented on Vulkan");
	(void)desc;
}

bool ClusterTemplate_VK::Prebuild(Device* pDevice)
{
	assert(!"ClusterTemplate::Prebuild is not implemented on Vulkan");
	(void)pDevice;

	return false;
}

bool ClusterTemplate_VK::Allocate(Device* pDevice, size_t byteWidth, MemoryFlag memoryFlags,
	const wchar_t* name, uint32_t maxThreads)
{
	assert(!"ClusterTemplate::Allocate is not implemented on Vulkan");
	(void)pDevice;
	(void)byteWidth;
	(void)memoryFlags;
	(void)name;
	(void)maxThreads;

	return false;
}

void ClusterTemplate_VK::Build(CommandList* pCommandList, const Resource* pScratch,
	uint64_t indirectArgumentAddress, uint64_t indirectArgumentStride)
{
	assert(!"ClusterTemplate::Build is not implemented on Vulkan");
	(void)pCommandList;
	(void)pScratch;
	(void)indirectArgumentAddress;
	(void)indirectArgumentStride;
}

void ClusterTemplate_VK::Instantiate(CommandList* pCommandList, ClusterLevelAS* pDestination,
	const Resource* pScratch, uint64_t indirectArgumentAddress, uint64_t indirectArgumentStride)
{
	assert(!"ClusterTemplate::Instantiate is not implemented on Vulkan");
	(void)pCommandList;
	(void)pDestination;
	(void)pScratch;
	(void)indirectArgumentAddress;
	(void)indirectArgumentStride;
}

uint32_t ClusterTemplate_VK::SetBarrier(ResourceBarrier* pBarriers, uint32_t numBarriers)
{
	assert(!"ClusterTemplate::SetBarrier is not implemented on Vulkan");
	(void)pBarriers;

	return numBarriers;
}

const RTASPrebuildInfo& ClusterTemplate_VK::GetPrebuildInfo() const
{
	assert(!"ClusterTemplate::GetPrebuildInfo is not implemented on Vulkan");

	return m_prebuildInfo;
}

size_t ClusterTemplate_VK::GetResultDataMaxByteSize(bool isAligned) const
{
	assert(!"ClusterTemplate::GetResultDataMaxByteSize is not implemented on Vulkan");
	(void)isAligned;

	return 0;
}

size_t ClusterTemplate_VK::GetScratchDataByteSize(bool isAligned) const
{
	assert(!"ClusterTemplate::GetScratchDataByteSize is not implemented on Vulkan");
	(void)isAligned;

	return 0;
}

uint64_t ClusterTemplate_VK::GetVirtualAddress() const
{
	assert(!"ClusterTemplate::GetVirtualAddress is not implemented on Vulkan");

	return 0;
}

void* ClusterTemplate_VK::GetBackend()
{
	return this;
}

size_t ClusterTemplate_VK::Align(size_t byteSize)
{
	return XUSG::Align(byteSize, 32);
}

ClusterBottomLevelAS_VK::ClusterBottomLevelAS_VK() :
	m_byteOffset(0)
{
	memset(&m_prebuildInfo, 0, sizeof(m_prebuildInfo));
}

ClusterBottomLevelAS_VK::~ClusterBottomLevelAS_VK()
{
}

void ClusterBottomLevelAS_VK::SetClusters(const RTASClasInputs& desc)
{
	assert(!"ClusterBottomLevelAS::SetClusters is not implemented on Vulkan");
	(void)desc;
}

bool ClusterBottomLevelAS_VK::Prebuild(Device* pDevice)
{
	assert(!"ClusterBottomLevelAS::Prebuild is not implemented on Vulkan");
	(void)pDevice;

	return false;
}

bool ClusterBottomLevelAS_VK::Allocate(Device* pDevice, size_t byteWidth, MemoryFlag memoryFlags,
	const wchar_t* name, uint32_t maxThreads)
{
	assert(!"ClusterBottomLevelAS::Allocate is not implemented on Vulkan");
	(void)pDevice;
	(void)byteWidth;
	(void)memoryFlags;
	(void)name;
	(void)maxThreads;

	return false;
}

void ClusterBottomLevelAS_VK::Build(CommandList* pCommandList, const Resource* pScratch,
	uint64_t indirectArgumentAddress, uint64_t indirectArgumentStride)
{
	assert(!"ClusterBottomLevelAS::Build is not implemented on Vulkan");
	(void)pCommandList;
	(void)pScratch;
	(void)indirectArgumentAddress;
	(void)indirectArgumentStride;
}

uint32_t ClusterBottomLevelAS_VK::SetBarrier(ResourceBarrier* pBarriers, uint32_t numBarriers)
{
	assert(!"ClusterBottomLevelAS::SetBarrier is not implemented on Vulkan");
	(void)pBarriers;

	return numBarriers;
}

const RTASPrebuildInfo& ClusterBottomLevelAS_VK::GetPrebuildInfo() const
{
	assert(!"ClusterBottomLevelAS::GetPrebuildInfo is not implemented on Vulkan");

	return m_prebuildInfo;
}

size_t ClusterBottomLevelAS_VK::GetResultDataMaxByteSize(bool isAligned) const
{
	assert(!"ClusterBottomLevelAS::GetResultDataMaxByteSize is not implemented on Vulkan");
	(void)isAligned;

	return 0;
}

size_t ClusterBottomLevelAS_VK::GetScratchDataByteSize(bool isAligned) const
{
	assert(!"ClusterBottomLevelAS::GetScratchDataByteSize is not implemented on Vulkan");
	(void)isAligned;

	return 0;
}

uint64_t ClusterBottomLevelAS_VK::GetVirtualAddress() const
{
	assert(!"ClusterBottomLevelAS::GetVirtualAddress is not implemented on Vulkan");

	return 0;
}

void* ClusterBottomLevelAS_VK::GetBackend()
{
	return this;
}

size_t ClusterBottomLevelAS_VK::Align(size_t byteSize)
{
	return AccelerationStructure_VK::Align(byteSize);
}

PartitionedTLAS_VK::PartitionedTLAS_VK() :
	m_byteOffset(0)
{
	memset(&m_prebuildInfo, 0, sizeof(m_prebuildInfo));
	m_srv = {};
}

PartitionedTLAS_VK::~PartitionedTLAS_VK()
{
}

void PartitionedTLAS_VK::SetPartitions(const RTASPartitionedTLASInputs& desc)
{
	assert(!"PartitionedTLAS::SetPartitions is not implemented on Vulkan");
	(void)desc;
}

bool PartitionedTLAS_VK::Prebuild(Device* pDevice)
{
	assert(!"PartitionedTLAS::Prebuild is not implemented on Vulkan");
	(void)pDevice;

	return false;
}

bool PartitionedTLAS_VK::Allocate(Device* pDevice, size_t byteWidth, MemoryFlag memoryFlags,
	const wchar_t* name, uint32_t maxThreads)
{
	assert(!"PartitionedTLAS::Allocate is not implemented on Vulkan");
	(void)pDevice;
	(void)byteWidth;
	(void)memoryFlags;
	(void)name;
	(void)maxThreads;

	return false;
}

void PartitionedTLAS_VK::Build(CommandList* pCommandList, const Resource* pScratch,
	uint64_t indirectOps, uint64_t indirectOpCount)
{
	assert(!"PartitionedTLAS::Build is not implemented on Vulkan");
	(void)pCommandList;
	(void)pScratch;
	(void)indirectOps;
	(void)indirectOpCount;
}

void PartitionedTLAS_VK::Update(CommandList* pCommandList, const Resource* pScratch,
	uint64_t indirectOps, uint64_t indirectOpCount)
{
	assert(!"PartitionedTLAS::Update is not implemented on Vulkan");
	(void)pCommandList;
	(void)pScratch;
	(void)indirectOps;
	(void)indirectOpCount;
}

uint32_t PartitionedTLAS_VK::SetBarrier(ResourceBarrier* pBarriers, uint32_t numBarriers)
{
	assert(!"PartitionedTLAS::SetBarrier is not implemented on Vulkan");
	(void)pBarriers;

	return numBarriers;
}

const RTASPrebuildInfo& PartitionedTLAS_VK::GetPrebuildInfo() const
{
	assert(!"PartitionedTLAS::GetPrebuildInfo is not implemented on Vulkan");

	return m_prebuildInfo;
}

size_t PartitionedTLAS_VK::GetResultDataMaxByteSize(bool isAligned) const
{
	assert(!"PartitionedTLAS::GetResultDataMaxByteSize is not implemented on Vulkan");
	(void)isAligned;

	return 0;
}

size_t PartitionedTLAS_VK::GetScratchDataByteSize(bool isAligned) const
{
	assert(!"PartitionedTLAS::GetScratchDataByteSize is not implemented on Vulkan");
	(void)isAligned;

	return 0;
}

uint64_t PartitionedTLAS_VK::GetVirtualAddress() const
{
	assert(!"PartitionedTLAS::GetVirtualAddress is not implemented on Vulkan");

	return 0;
}

const Descriptor& PartitionedTLAS_VK::GetSRV() const
{
	assert(!"PartitionedTLAS::GetSRV is not implemented on Vulkan");

	return m_srv;
}

void* PartitionedTLAS_VK::GetBackend()
{
	return this;
}

size_t PartitionedTLAS_VK::Align(size_t byteSize)
{
	return AccelerationStructure_VK::Align(byteSize);
}
