//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSG_VK.h"
#include "Core/XUSGEnum_VK.h"
#include "XUSGRayTracing_VK.h"
#include "XUSGAccelerationStructure_VK.h"

using namespace std;
using namespace XUSG;
using namespace XUSG::RayTracing;

#ifndef VK_ACCELERATION_STRUCTURE_SIZE_ALIGNMENT_KHR
#define VK_ACCELERATION_STRUCTURE_SIZE_ALIGNMENT_KHR 256
#endif

namespace
{
	struct BufferSpan
	{
		VkBuffer Buffer;
		VkDeviceAddress Address;
		VkDeviceSize Size;
	};

	struct AccelerationStructureRecord
	{
		VkAccelerationStructureKHR AccelerationStructure;
		VkDeviceAddress Address;
	};

	mutex g_bufferMutex;
	vector<BufferSpan> g_buffers;
	mutex g_asMutex;
	vector<AccelerationStructureRecord> g_accelerationStructures;
	vector<VkMicromapEXT> g_micromaps;

	struct PackedGeometry
	{
		VkAccelerationStructureGeometryKHR Geometry;
		uint32_t PrimitiveCount;
		uint32_t HasOpacityMicromap;
		Format OpacityMicromapIndexFormat;
		VkDeviceAddress OpacityMicromapIndexBuffer;
		VkDeviceSize OpacityMicromapIndexStride;
		uint32_t OpacityMicromapBaseTriangle;
		uint64_t OpacityMicromap;
	};

	struct PackedOmmUsage
	{
		uint32_t Count;
		uint32_t SubdivisionLevel;
		uint32_t Format;
	};

	struct PackedOmmArray
	{
		uint32_t NumUsages;
		uint32_t UsageOffset;
		VkDeviceAddress Input;
		VkDeviceAddress Triangles;
		VkDeviceSize TriangleStride;
	};

	VkIndexType IndexTypeFromFormat(Format format)
	{
		if (format == Format::R32_UINT) return VK_INDEX_TYPE_UINT32;
		if (format == Format::R16_UINT) return VK_INDEX_TYPE_UINT16;

		return VK_INDEX_TYPE_NONE_KHR;
	}

	VkAccelerationStructureGeometryKHR GetVKTriangles(const BottomLevelAS::TriangleGeometry& triangles, GeometryFlag flags, uint32_t& primitiveCount)
	{
		VkAccelerationStructureGeometryKHR geometry = {};
		geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		geometry.flags = GetVKGeometryFlags(flags);
		geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
		geometry.geometry.triangles.vertexFormat = GetVKFormat(triangles.VertexFormat);
		geometry.geometry.triangles.vertexData.deviceAddress = triangles.VertexBufferStart;
		geometry.geometry.triangles.vertexStride = triangles.VertexBufferStride;
		geometry.geometry.triangles.maxVertex = triangles.VertexCount ? triangles.VertexCount - 1 : 0;
		geometry.geometry.triangles.indexType = IndexTypeFromFormat(triangles.IndexFormat);
		geometry.geometry.triangles.indexData.deviceAddress = triangles.IndexBuffer;
		geometry.geometry.triangles.transformData.deviceAddress = triangles.Transform;

		const auto count = triangles.IndexCount ? triangles.IndexCount : triangles.VertexCount;
		primitiveCount = count / 3;

		return geometry;
	}

	VkOpacityMicromapFormatKHR GetVKOpacityMicromapFormat(OpacityMicromapFormat format)
	{
		switch (format)
		{
		case OpacityMicromapFormat::OC1_2_STATE:
			return VK_OPACITY_MICROMAP_FORMAT_2_STATE_KHR;
		case OpacityMicromapFormat::OC1_4_STATE:
			return VK_OPACITY_MICROMAP_FORMAT_4_STATE_KHR;
		default:
			assert(!"Unrecognized opacity micromap format");

			return VK_OPACITY_MICROMAP_FORMAT_2_STATE_KHR;
		}
	}

	VkIndexType OpacityMicromapIndexType(Format format, const XUSG::Device_VK* pDevice)
	{
		if (format == Format::UNKNOWN) return VK_INDEX_TYPE_NONE_KHR;
		if (format == Format::R32_UINT) return VK_INDEX_TYPE_UINT32;
		if (format == Format::R16_UINT) return VK_INDEX_TYPE_UINT16;
		if (format == Format::R8_UINT)
		{
			if (pDevice && pDevice->HasIndexTypeUint8()) return VK_INDEX_TYPE_UINT8;

			assert(!"R8_UINT opacity micromap indices require VK_KHR_index_type_uint8");

			return VK_INDEX_TYPE_UINT8;
		}

		assert(!"Unsupported opacity micromap index format");

		return VK_INDEX_TYPE_NONE_KHR;
	}

	bool RequireOpacityMicromap(const XUSG::Device_VK* pDevice)
	{
		assert(pDevice);
		if (!pDevice) return false;
		if (pDevice->HasMicromap()) return true;

		assert(!"Opacity micromap is not enabled");

		return false;
	}

	bool OpacityMicromapUsesExt(const XUSG::Device_VK* pDevice)
	{
		return pDevice && pDevice->HasMicromap() && !pDevice->HasOpacityMicromapKhr();
	}

	VkBuildAccelerationStructureFlagsKHR OpacityMicromapBuildFlags(BuildFlag buildFlags)
	{
		auto flags = VkBuildAccelerationStructureFlagsKHR(0);
		if (static_cast<bool>(buildFlags & BuildFlag::ALLOW_COMPACTION))
			flags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
		if (static_cast<bool>(buildFlags & BuildFlag::PREFER_FAST_TRACE))
			flags |= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		if (static_cast<bool>(buildFlags & BuildFlag::PREFER_FAST_BUILD))
			flags |= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;

		return flags;
	}

	VkBuildMicromapFlagsEXT OpacityMicromapExtBuildFlags(BuildFlag buildFlags)
	{
		auto flags = VkBuildMicromapFlagsEXT(0);
		if (static_cast<bool>(buildFlags & BuildFlag::ALLOW_COMPACTION))
			flags |= VK_BUILD_MICROMAP_ALLOW_COMPACTION_BIT_EXT;
		if (static_cast<bool>(buildFlags & BuildFlag::PREFER_FAST_TRACE))
			flags |= VK_BUILD_MICROMAP_PREFER_FAST_TRACE_BIT_EXT;
		if (static_cast<bool>(buildFlags & BuildFlag::PREFER_FAST_BUILD))
			flags |= VK_BUILD_MICROMAP_PREFER_FAST_BUILD_BIT_EXT;

		return flags;
	}

	void FillBuildRanges(BuildDesc_VK& desc)
	{
		desc.Ranges.resize(desc.PrimitiveCounts.size());
		for (auto i = 0u; i < desc.PrimitiveCounts.size(); ++i)
		{
			desc.Ranges[i] = {};
			desc.Ranges[i].primitiveCount = desc.PrimitiveCounts[i];
		}
		for (auto i = 0u; i < desc.Geometries.size(); ++i)
		{
			if (desc.Geometries[i].geometryType != VK_GEOMETRY_TYPE_TRIANGLES_KHR) continue;
			if (i < desc.OpacityMicromaps.size() &&
				desc.OpacityMicromaps[i].sType == VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_TRIANGLES_OPACITY_MICROMAP_KHR)
				desc.Geometries[i].geometry.triangles.pNext = &desc.OpacityMicromaps[i];
			else if (i < desc.OpacityMicromapsExt.size() &&
				desc.OpacityMicromapsExt[i].sType == VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_TRIANGLES_OPACITY_MICROMAP_EXT)
				desc.Geometries[i].geometry.triangles.pNext = &desc.OpacityMicromapsExt[i];
		}
		if (desc.Type == VK_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_KHR && !desc.Geometries.empty())
		{
			desc.Ranges.clear();
			desc.MicromapData.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_MICROMAP_DATA_KHR;
			desc.MicromapData.usageCountsCount = static_cast<uint32_t>(desc.Usages.size());
			desc.MicromapData.pUsageCounts = desc.Usages.empty() ? nullptr : desc.Usages.data();
			desc.MicromapData.ppUsageCounts = nullptr;
			desc.Geometries[0].pNext = &desc.MicromapData;
		}
		desc.Info.geometryCount = static_cast<uint32_t>(desc.Geometries.size());
		desc.Info.pGeometries = desc.Geometries.empty() ? nullptr : desc.Geometries.data();
		desc.Info.ppGeometries = nullptr;
	}
}

void XUSG::RayTracing::RegisterVKBuffer(VkBuffer buffer, VkDeviceAddress address, VkDeviceSize size)
{
	lock_guard<mutex> lock(g_bufferMutex);
	for (auto i = g_buffers.size(); i-- > 0;)
		if (g_buffers[i].Buffer == buffer) g_buffers.erase(g_buffers.begin() + i);
	if (buffer == VK_NULL_HANDLE || !address || !size) return;

	g_buffers.push_back({ buffer, address, size });
}

void XUSG::RayTracing::UnregisterVKBuffer(VkBuffer buffer)
{
	if (buffer == VK_NULL_HANDLE) return;

	lock_guard<mutex> lock(g_bufferMutex);
	for (auto i = g_buffers.size(); i-- > 0;)
		if (g_buffers[i].Buffer == buffer) g_buffers.erase(g_buffers.begin() + i);
}

bool XUSG::RayTracing::FindVKBuffer(VkDeviceAddress address, VkBuffer& buffer, VkDeviceSize& offset)
{
	lock_guard<mutex> lock(g_bufferMutex);
	for (const auto& span : g_buffers)
	{
		if (address < span.Address || address >= span.Address + span.Size) continue;

		buffer = span.Buffer;
		offset = address - span.Address;

		return true;
	}

	return false;
}

void XUSG::RayTracing::RegisterVKAccelerationStructure(VkAccelerationStructureKHR accelerationStructure, VkDeviceAddress address)
{
	if (accelerationStructure == VK_NULL_HANDLE) return;

	lock_guard<mutex> lock(g_asMutex);
	for (auto& record : g_accelerationStructures)
	{
		if (record.AccelerationStructure != accelerationStructure) continue;
		if (address) record.Address = address;

		return;
	}

	g_accelerationStructures.push_back({ accelerationStructure, address });
}

void XUSG::RayTracing::UnregisterVKAccelerationStructure(VkAccelerationStructureKHR accelerationStructure)
{
	if (accelerationStructure == VK_NULL_HANDLE) return;

	lock_guard<mutex> lock(g_asMutex);
	for (auto i = g_accelerationStructures.size(); i-- > 0;)
		if (g_accelerationStructures[i].AccelerationStructure == accelerationStructure)
			g_accelerationStructures.erase(g_accelerationStructures.begin() + i);
}

VkAccelerationStructureKHR XUSG::RayTracing::FindVKAccelerationStructure(uint64_t value)
{
	lock_guard<mutex> lock(g_asMutex);
	for (const auto& record : g_accelerationStructures)
		if (reinterpret_cast<uint64_t>(record.AccelerationStructure) == value || record.Address == value)
			return record.AccelerationStructure;

	return VK_NULL_HANDLE;
}

void XUSG::RayTracing::RegisterVKMicromap(VkMicromapEXT micromap)
{
	if (micromap == VK_NULL_HANDLE) return;

	lock_guard<mutex> lock(g_asMutex);
	for (const auto& record : g_micromaps)
		if (record == micromap) return;

	g_micromaps.push_back(micromap);
}

void XUSG::RayTracing::UnregisterVKMicromap(VkMicromapEXT micromap)
{
	if (micromap == VK_NULL_HANDLE) return;

	lock_guard<mutex> lock(g_asMutex);
	for (auto i = g_micromaps.size(); i-- > 0;)
		if (g_micromaps[i] == micromap) g_micromaps.erase(g_micromaps.begin() + i);
}

VkMicromapEXT XUSG::RayTracing::FindVKMicromap(uint64_t value)
{
	lock_guard<mutex> lock(g_asMutex);
	for (const auto micromap : g_micromaps)
		if (reinterpret_cast<uint64_t>(micromap) == value) return micromap;

	return VK_NULL_HANDLE;
}

//--------------------------------------------------------------------------------------
// Acceleration structure
//--------------------------------------------------------------------------------------

AccelerationStructure_VK::AccelerationStructure_VK() :
	m_buildDesc(),
	m_prebuildInfo(),
	m_pVkDevice(nullptr),
	m_byteOffset(0)
{
	m_buildDesc.Info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	m_buildDesc.AccelerationStructure = VK_NULL_HANDLE;
	m_buildDesc.Micromap = VK_NULL_HANDLE;
	m_buildDesc.Type = VK_ACCELERATION_STRUCTURE_TYPE_GENERIC_KHR;
	m_buildDesc.Flags = 0;
}

AccelerationStructure_VK::~AccelerationStructure_VK()
{
	if (m_postbuildInfo)
		UnregisterVKBuffer(reinterpret_cast<VkBuffer>(m_postbuildInfo->GetNativeHandle()));
	destroyAccelerationStructure();
}

void AccelerationStructure_VK::SetDestination(Device* pDevice, const Buffer::sptr destBuffer,
	uintptr_t byteOffset, uint32_t uavIndex, DescriptorTableLib* pDescriptorTableLib)
{
	(void)uavIndex;
	(void)pDescriptorTableLib;
	m_pVkDevice = AsVK(pDevice);
	m_resource = destBuffer;
	m_byteOffset = byteOffset;

	auto byteSize = GetResultDataMaxByteSize(false);
	if (!byteSize && m_resource && m_resource->GetWidth() > m_byteOffset)
		byteSize = static_cast<size_t>(m_resource->GetWidth() - m_byteOffset);
	createAccelerationStructure(byteSize);
}

uint32_t AccelerationStructure_VK::SetBarrier(ResourceBarrier* pBarriers, uint32_t numBarriers)
{
	return AccelerationStructure::SetBarrier(pBarriers, m_resource.get(), numBarriers);
}

const PrebuildInfo& AccelerationStructure_VK::GetPrebuildInfo() const
{
	return m_prebuildInfo;
}

Buffer* AccelerationStructure_VK::GetPostbuildInfo() const
{
	return m_postbuildInfoRB.get();
}

size_t AccelerationStructure_VK::GetResultDataMaxByteSize(bool isAligned) const
{
	const auto resultDataMaxByteSize = static_cast<size_t>(m_prebuildInfo.ResultDataMaxByteSize);

	return isAligned ? Align(resultDataMaxByteSize) : resultDataMaxByteSize;
}

size_t AccelerationStructure_VK::GetScratchDataByteSize(bool isAligned) const
{
	const auto scratchDataByteSize = static_cast<size_t>(m_prebuildInfo.ScratchDataByteSize);

	return isAligned ? Align(scratchDataByteSize) : scratchDataByteSize;
}

size_t AccelerationStructure_VK::GetUpdateScratchDataByteSize(bool isAligned) const
{
	const auto updateScratchDataByteSize = static_cast<size_t>(m_prebuildInfo.UpdateScratchDataByteSize);

	return isAligned ? Align(updateScratchDataByteSize) : updateScratchDataByteSize;
}

size_t AccelerationStructure_VK::GetCompactedByteSize(bool isAligned) const
{
	const auto pPostbuildInfo = static_cast<const uint64_t*>(m_postbuildInfoRB->Map(nullptr));
	const auto compactedByteSize = static_cast<size_t>(*pPostbuildInfo);
	m_postbuildInfoRB->Unmap();

	return isAligned ? Align(compactedByteSize) : compactedByteSize;
}

uint64_t AccelerationStructure_VK::GetVirtualAddress() const
{
	if (m_buildDesc.AccelerationStructure == VK_NULL_HANDLE || !m_pVkDevice) return 0;

	const auto& ext = m_pVkDevice->GetExtProcs();
	if (!ext.GetAccelerationStructureDeviceAddress) return 0;

	VkAccelerationStructureDeviceAddressInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	info.accelerationStructure = m_buildDesc.AccelerationStructure;
	const auto address = ext.GetAccelerationStructureDeviceAddress(m_pVkDevice->GetDevice(), &info);
	RegisterVKAccelerationStructure(m_buildDesc.AccelerationStructure, address);

	return address;
}

uint64_t AccelerationStructure_VK::GetResourcePointer() const
{
	return GetVirtualAddress();
}

VkAccelerationStructureKHR AccelerationStructure_VK::GetHandle() const
{
	return m_buildDesc.AccelerationStructure;
}

bool AccelerationStructure_VK::AllocateDestBuffer(Device* pDevice, Buffer* pResource, size_t byteWidth,
	uint32_t numSRVs, const uintptr_t* firstSrvElements, uint32_t numUAVs, const uintptr_t* firstUavElements,
	MemoryFlag memoryFlags, const wchar_t* name, uint32_t maxThreads)
{
	return pResource->Create(pDevice, byteWidth, ResourceFlag::ACCELERATION_STRUCTURE, MemoryType::DEFAULT,
		numSRVs, firstSrvElements, numUAVs, firstUavElements, memoryFlags, name,
		nullptr, maxThreads);
}

size_t AccelerationStructure_VK::Align(size_t byteSize)
{
	return XUSG::Align(byteSize, VK_ACCELERATION_STRUCTURE_SIZE_ALIGNMENT_KHR);
}

bool AccelerationStructure_VK::prebuild(Device* pDevice)
{
	m_pVkDevice = AsVK(pDevice);
	assert(m_pVkDevice);
	assert(m_pVkDevice->GetNativeHandle());
	const auto& ext = m_pVkDevice->GetExtProcs();
	assert(ext.GetAccelerationStructureBuildSizes);
	XUSG_N_RETURN(ext.GetAccelerationStructureBuildSizes, false);

	FillBuildRanges(m_buildDesc);
	const auto pPrimitiveCounts = m_buildDesc.Type == VK_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_KHR ||
		m_buildDesc.PrimitiveCounts.empty() ? nullptr : m_buildDesc.PrimitiveCounts.data();
	VkAccelerationStructureBuildSizesInfoKHR sizes = {};
	sizes.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	ext.GetAccelerationStructureBuildSizes(m_pVkDevice->GetDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&m_buildDesc.Info, pPrimitiveCounts, &sizes);

	m_prebuildInfo.ResultDataMaxByteSize = sizes.accelerationStructureSize;
	m_prebuildInfo.ScratchDataByteSize = sizes.buildScratchSize;
	m_prebuildInfo.UpdateScratchDataByteSize = sizes.updateScratchSize;

	return m_prebuildInfo.ResultDataMaxByteSize;
}

bool AccelerationStructure_VK::allocate(Device* pDevice, size_t byteWidth,
	DescriptorTableLib* pDescriptorTableLib, uint32_t numSRVs, MemoryFlag memoryFlags,
	const wchar_t* name, uint32_t maxThreads)
{
	(void)pDescriptorTableLib;
	m_pVkDevice = AsVK(pDevice);
	m_resource = Buffer::MakeShared();
	byteWidth = byteWidth ? byteWidth : GetResultDataMaxByteSize(false);
	XUSG_N_RETURN(AllocateDestBuffer(pDevice, m_resource.get(), byteWidth, numSRVs,
		nullptr, 1, nullptr, memoryFlags, name, maxThreads), false);
	m_byteOffset = 0;

	return createAccelerationStructure(byteWidth);
}

bool AccelerationStructure_VK::createAccelerationStructure(VkDeviceSize size)
{
	destroyAccelerationStructure();
	if (!m_pVkDevice || !m_resource || !size) return false;

	const auto& ext = m_pVkDevice->GetExtProcs();
	assert(ext.CreateAccelerationStructure);
	XUSG_N_RETURN(ext.CreateAccelerationStructure, false);

	VkAccelerationStructureCreateInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	info.buffer = reinterpret_cast<VkBuffer>(m_resource->GetNativeHandle());
	info.offset = m_byteOffset;
	info.size = size;
	info.type = m_buildDesc.Type;
	XUSG_VK_CHECK(ext.CreateAccelerationStructure(m_pVkDevice->GetDevice(), &info, nullptr, &m_buildDesc.AccelerationStructure), cerr, false);
	RegisterVKAccelerationStructure(m_buildDesc.AccelerationStructure, 0);

	return true;
}

void AccelerationStructure_VK::destroyAccelerationStructure()
{
	if (m_buildDesc.AccelerationStructure == VK_NULL_HANDLE) return;

	UnregisterVKAccelerationStructure(m_buildDesc.AccelerationStructure);
	if (m_pVkDevice)
	{
		const auto destroy = m_pVkDevice->GetExtProcs().DestroyAccelerationStructure;
		if (destroy) destroy(m_pVkDevice->GetDevice(), m_buildDesc.AccelerationStructure, nullptr);
	}
	m_buildDesc.AccelerationStructure = VK_NULL_HANDLE;
}

void AccelerationStructure_VK::registerPostbuildBuffer()
{
	if (!m_postbuildInfo) return;

	RegisterVKBuffer(reinterpret_cast<VkBuffer>(m_postbuildInfo->GetNativeHandle()),
		m_postbuildInfo->GetVirtualAddress(), m_postbuildInfo->GetWidth());
}

//--------------------------------------------------------------------------------------
// Bottom level acceleration structure
//--------------------------------------------------------------------------------------

BottomLevelAS_VK::BottomLevelAS_VK() :
	AccelerationStructure_VK()
{
}

BottomLevelAS_VK::~BottomLevelAS_VK()
{
}

bool BottomLevelAS_VK::Prebuild(Device* pDevice, uint32_t numGeometries,
	const GeometryBuffer& geometries, BuildFlag flags)
{
	const auto pPacked = reinterpret_cast<const PackedGeometry*>(geometries.data());
	const auto pVkDevice = AsVK(pDevice);
	m_buildDesc.Geometries.resize(numGeometries);
	m_buildDesc.PrimitiveCounts.resize(numGeometries);
	m_buildDesc.OpacityMicromaps.assign(numGeometries, {});
	m_buildDesc.OpacityMicromapsExt.assign(numGeometries, {});
	m_buildDesc.Usages.clear();
	m_buildDesc.MicromapData = {};
	m_buildDesc.Micromap = VK_NULL_HANDLE;
	const auto micromapExt = OpacityMicromapUsesExt(pVkDevice);
	auto hasOpacityMicromap = false;
	for (auto i = 0u; i < numGeometries; ++i)
	{
		m_buildDesc.Geometries[i] = pPacked[i].Geometry;
		m_buildDesc.Geometries[i].pNext = nullptr;
		if (m_buildDesc.Geometries[i].geometryType == VK_GEOMETRY_TYPE_TRIANGLES_KHR)
			m_buildDesc.Geometries[i].geometry.triangles.pNext = nullptr;
		m_buildDesc.PrimitiveCounts[i] = pPacked[i].PrimitiveCount;
		if (!pPacked[i].HasOpacityMicromap) continue;

		hasOpacityMicromap = true;
		if (micromapExt)
		{
			auto& opacityMicromap = m_buildDesc.OpacityMicromapsExt[i];
			opacityMicromap.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_TRIANGLES_OPACITY_MICROMAP_EXT;
			opacityMicromap.pNext = nullptr;
			opacityMicromap.indexType = OpacityMicromapIndexType(pPacked[i].OpacityMicromapIndexFormat, pVkDevice);
			opacityMicromap.indexBuffer.deviceAddress = pPacked[i].OpacityMicromapIndexBuffer;
			opacityMicromap.indexStride = pPacked[i].OpacityMicromapIndexStride;
			opacityMicromap.baseTriangle = pPacked[i].OpacityMicromapBaseTriangle;
			opacityMicromap.usageCountsCount = 0;
			opacityMicromap.pUsageCounts = nullptr;
			opacityMicromap.ppUsageCounts = nullptr;
			opacityMicromap.micromap = reinterpret_cast<VkMicromapEXT>(pPacked[i].OpacityMicromap);
		}
		else
		{
			auto& opacityMicromap = m_buildDesc.OpacityMicromaps[i];
			opacityMicromap.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_TRIANGLES_OPACITY_MICROMAP_KHR;
			opacityMicromap.pNext = nullptr;
			opacityMicromap.indexType = OpacityMicromapIndexType(pPacked[i].OpacityMicromapIndexFormat, pVkDevice);
			opacityMicromap.indexBuffer = pPacked[i].OpacityMicromapIndexBuffer;
			opacityMicromap.indexStride = pPacked[i].OpacityMicromapIndexStride;
			opacityMicromap.baseTriangle = pPacked[i].OpacityMicromapBaseTriangle;
			opacityMicromap.micromap = reinterpret_cast<VkAccelerationStructureKHR>(pPacked[i].OpacityMicromap);
		}
	}
	if (hasOpacityMicromap && !RequireOpacityMicromap(pVkDevice)) return false;

	m_buildDesc.Type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	m_buildDesc.Flags = GetVKBuildFlags(flags);
	m_buildDesc.Info = {};
	m_buildDesc.Info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	m_buildDesc.Info.type = m_buildDesc.Type;
	m_buildDesc.Info.flags = m_buildDesc.Flags;
	m_buildDesc.Info.mode = static_cast<bool>(flags & BuildFlag::PERFORM_UPDATE) ?
		VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;

	return prebuild(pDevice);
}

bool BottomLevelAS_VK::Allocate(Device* pDevice, DescriptorTableLib* pDescriptorTableLib,
	size_t byteWidth, MemoryFlag memoryFlags, const wchar_t* name, uint32_t maxThreads)
{
	return allocate(pDevice, byteWidth, pDescriptorTableLib, 0, memoryFlags, name, maxThreads);
}

void BottomLevelAS_VK::Build(CommandList* pCommandList, const Resource* pScratch, const BottomLevelAS* pSource,
	uint8_t numPostbuildInfoDescs, const PostbuildInfoType* pPostbuildInfoTypes)
{
	m_buildDesc.Info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	m_buildDesc.Info.srcAccelerationStructure = VK_NULL_HANDLE;
	if (pSource && (m_buildDesc.Flags & VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR))
	{
		const auto pSourceVK = dynamic_cast<const AccelerationStructure_VK*>(pSource);
		assert(pSourceVK);
		m_buildDesc.Info.srcAccelerationStructure = pSourceVK ? pSourceVK->GetHandle() : VK_NULL_HANDLE;
		m_buildDesc.Info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
	}

	m_buildDesc.Info.dstAccelerationStructure = GetHandle();
	m_buildDesc.Info.scratchData.deviceAddress = pScratch ? pScratch->GetVirtualAddress() : 0;
	FillBuildRanges(m_buildDesc);

	if (numPostbuildInfoDescs)
	{
		if (m_postbuildInfo) UnregisterVKBuffer(reinterpret_cast<VkBuffer>(m_postbuildInfo->GetNativeHandle()));
		m_postbuildInfoRB = Buffer::MakeUnique();
		m_postbuildInfo = Buffer::MakeUnique();
		m_postbuildInfo->Create(pCommandList->GetDevice(), sizeof(uint64_t) * numPostbuildInfoDescs,
			ResourceFlag::DENY_SHADER_RESOURCE | ResourceFlag::ALLOW_UNORDERED_ACCESS);
		registerPostbuildBuffer();

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

	pCommandList->BuildRaytracingAccelerationStructure(&m_buildDesc, numPostbuildInfoDescs,
		numPostbuildInfoDescs ? postbuildInfoDescs.data() : nullptr);

	if (m_postbuildInfo) m_postbuildInfo->ReadBack(pCommandList, m_postbuildInfoRB.get());
}

void BottomLevelAS_VK::SetGeometries(GeometryBuffer& geometries, uint32_t numGeometries, GeometryDesc* pGeometries)
{
	assert(pGeometries);
	geometries.resize(sizeof(PackedGeometry) * numGeometries);
	auto pPacked = reinterpret_cast<PackedGeometry*>(geometries.data());
	for (auto i = 0u; i < numGeometries; ++i)
	{
		const auto& geometry = pGeometries[i];
		auto& packed = pPacked[i];
		packed = {};

		switch (geometry.Type)
		{
		case GeometryType::TRIANGLES:
			packed.Geometry = GetVKTriangles(geometry.Triangles, geometry.Flags, packed.PrimitiveCount);
			break;
		case GeometryType::PROCEDURAL_PRIMITIVE_AABBS:
			packed.Geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
			packed.Geometry.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;
			packed.Geometry.flags = GetVKGeometryFlags(geometry.Flags);
			packed.Geometry.geometry.aabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
			packed.Geometry.geometry.aabbs.data.deviceAddress = geometry.AABBs.AABBsStart;
			packed.Geometry.geometry.aabbs.stride = geometry.AABBs.AABBsStride;
			packed.PrimitiveCount = geometry.AABBs.AABBCount;
			break;
		case GeometryType::OMM_TRIANGLES:
			assert(geometry.OmmTriangles.pTriangles);
			if (!geometry.OmmTriangles.pTriangles) break;

			packed.Geometry = GetVKTriangles(*geometry.OmmTriangles.pTriangles, geometry.Flags, packed.PrimitiveCount);
			if (!geometry.OmmTriangles.pOmmLinkage) break;

			{
				const auto& linkage = *geometry.OmmTriangles.pOmmLinkage;
				assert(linkage.pOpacityMicromapArray);
				packed.HasOpacityMicromap = 1;
				packed.OpacityMicromapIndexFormat = linkage.OpacityMicromapIndexFormat;
				packed.OpacityMicromapIndexBuffer = linkage.OpacityMicromapIndexBufferStart;
				packed.OpacityMicromapIndexStride = linkage.OpacityMicromapIndexBufferStride;
				packed.OpacityMicromapBaseTriangle = linkage.OpacityMicromapBaseLocation;
				if (linkage.pOpacityMicromapArray)
				{
					const auto pArrayVK = dynamic_cast<const OpacityMicromapArray_VK*>(linkage.pOpacityMicromapArray);
					assert(pArrayVK);
					if (pArrayVK && pArrayVK->GetMicromap() != VK_NULL_HANDLE)
						packed.OpacityMicromap = reinterpret_cast<uint64_t>(pArrayVK->GetMicromap());
					else if (pArrayVK) packed.OpacityMicromap = reinterpret_cast<uint64_t>(pArrayVK->GetHandle());
					if (!packed.OpacityMicromap)
					{
						const auto address = linkage.pOpacityMicromapArray->GetVirtualAddress() + linkage.OpacityMicromapArrayOffset;
						if (address) packed.OpacityMicromap = reinterpret_cast<uint64_t>(FindVKAccelerationStructure(address));
					}
				}
				assert(packed.OpacityMicromap);
			}
			break;
		default:
			assert(!"Unsupported geometry type.");
		}
	}
}

size_t BottomLevelAS_VK::AlignTransform(size_t byteSize)
{
	return XUSG::Align(byteSize, 16);
}

size_t BottomLevelAS_VK::AlignAABB(size_t byteSize)
{
	return XUSG::Align(byteSize, 8);
}

//--------------------------------------------------------------------------------------
// Top level acceleration structure
//--------------------------------------------------------------------------------------

TopLevelAS_VK::TopLevelAS_VK() :
	AccelerationStructure_VK(),
	m_srvIndex(0)
{
}

TopLevelAS_VK::~TopLevelAS_VK()
{
}

bool TopLevelAS_VK::Prebuild(Device* pDevice, uint32_t numInstances, BuildFlag flags)
{
	VkAccelerationStructureGeometryKHR geometry = {};
	geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	geometry.geometry.instances.arrayOfPointers = VK_FALSE;
	m_buildDesc.Geometries = { geometry };
	m_buildDesc.PrimitiveCounts = { numInstances };
	m_buildDesc.Type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	m_buildDesc.Flags = GetVKBuildFlags(flags);
	m_buildDesc.Info = {};
	m_buildDesc.Info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	m_buildDesc.Info.type = m_buildDesc.Type;
	m_buildDesc.Info.flags = m_buildDesc.Flags;
	m_buildDesc.Info.mode = static_cast<bool>(flags & BuildFlag::PERFORM_UPDATE) ?
		VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;

	return prebuild(pDevice);
}

bool TopLevelAS_VK::Allocate(Device* pDevice, DescriptorTableLib* pDescriptorTableLib,
	size_t byteWidth, MemoryFlag memoryFlags, const wchar_t* name, uint32_t maxThreads)
{
	m_srvIndex = 0;

	return allocate(pDevice, byteWidth, pDescriptorTableLib, 1, memoryFlags, name, maxThreads);
}

void TopLevelAS_VK::SetDestination(Device* pDevice, const Buffer::sptr destBuffer, uintptr_t byteOffset,
	uint32_t uavIndex, uint32_t srvIndex, DescriptorTableLib* pDescriptorTableLib)
{
	m_srvIndex = srvIndex;
	AccelerationStructure_VK::SetDestination(pDevice, destBuffer, byteOffset, uavIndex, pDescriptorTableLib);
}

void TopLevelAS_VK::Build(CommandList* pCommandList, const Resource* pScratch,
	const Resource* pInstanceDescs, const DescriptorHeap& descriptorHeap,
	const TopLevelAS* pSource, uint8_t numPostbuildInfoDescs,
	const PostbuildInfoType* pPostbuildInfoTypes)
{
	(void)descriptorHeap;
	m_buildDesc.Info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	m_buildDesc.Info.srcAccelerationStructure = VK_NULL_HANDLE;
	if (pSource && (m_buildDesc.Flags & VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR))
	{
		const auto pSourceVK = dynamic_cast<const AccelerationStructure_VK*>(pSource);
		assert(pSourceVK);
		m_buildDesc.Info.srcAccelerationStructure = pSourceVK ? pSourceVK->GetHandle() : VK_NULL_HANDLE;
		m_buildDesc.Info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
	}

	if (!m_buildDesc.Geometries.empty())
		m_buildDesc.Geometries[0].geometry.instances.data.deviceAddress = pInstanceDescs ? pInstanceDescs->GetVirtualAddress() : 0;
	m_buildDesc.Info.dstAccelerationStructure = GetHandle();
	m_buildDesc.Info.scratchData.deviceAddress = pScratch ? pScratch->GetVirtualAddress() : 0;
	FillBuildRanges(m_buildDesc);

	if (numPostbuildInfoDescs)
	{
		if (m_postbuildInfo) UnregisterVKBuffer(reinterpret_cast<VkBuffer>(m_postbuildInfo->GetNativeHandle()));
		m_postbuildInfoRB = Buffer::MakeShared();
		m_postbuildInfo = Buffer::MakeUnique();
		m_postbuildInfo->Create(pCommandList->GetDevice(), sizeof(uint64_t) * numPostbuildInfoDescs,
			ResourceFlag::DENY_SHADER_RESOURCE | ResourceFlag::ALLOW_UNORDERED_ACCESS);
		registerPostbuildBuffer();

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

	pCommandList->BuildRaytracingAccelerationStructure(&m_buildDesc, numPostbuildInfoDescs,
		numPostbuildInfoDescs ? postbuildInfoDescs.data() : nullptr);

	if (m_postbuildInfo) m_postbuildInfo->ReadBack(pCommandList, m_postbuildInfoRB.get());
}

const Descriptor& TopLevelAS_VK::GetSRV() const
{
	return m_resource->GetSRV(m_srvIndex);
}

void TopLevelAS_VK::SetInstances(Device* pDevice, Buffer* pInstances, uint32_t numInstances,
	const InstanceDesc* pInstanceDescs, MemoryFlag memoryFlags, const wchar_t* instanceName)
{
	assert(numInstances == 0 || pInstanceDescs);

	vector<VkAccelerationStructureInstanceKHR> instanceDescs(numInstances);
	for (auto i = 0u; i < numInstances; ++i)
	{
		const auto& instanceDesc = pInstanceDescs[i];
		const auto& pBottomLevelAS = instanceDesc.pBottomLevelAS;
		memcpy(instanceDescs[i].transform.matrix, instanceDesc.pTransform, sizeof(instanceDescs[i].transform.matrix));
		instanceDescs[i].instanceCustomIndex = instanceDesc.InstanceID;
		instanceDescs[i].mask = instanceDesc.InstanceMask;
		instanceDescs[i].instanceShaderBindingTableRecordOffset = instanceDesc.InstanceContributionToHitGroupIndex;
		instanceDescs[i].flags = GetVKInstanceFlags(static_cast<InstanceFlag>(instanceDesc.Flags));
		instanceDescs[i].accelerationStructureReference = pBottomLevelAS ? pBottomLevelAS->GetVirtualAddress() : 0;
	}

	if (pInstances->GetNativeHandle())
	{
		void* pMappedData = pInstances->Map();
		memcpy(pMappedData, instanceDescs.data(), sizeof(VkAccelerationStructureInstanceKHR) * numInstances);
		pInstances->Unmap();
	}
	else AccelerationStructure_VK::AllocateUploadBuffer(pDevice, pInstances,
		sizeof(VkAccelerationStructureInstanceKHR) * numInstances,
		instanceDescs.data(), memoryFlags, instanceName);
}

size_t TopLevelAS_VK::AlignInstanceDesc(size_t byteSize)
{
	return XUSG::Align(byteSize, 64);
}

//--------------------------------------------------------------------------------------
// Opacity micromap array
//--------------------------------------------------------------------------------------

OpacityMicromapArray_VK::OpacityMicromapArray_VK() :
	AccelerationStructure_VK(),
	m_buildFlags(BuildFlag::NONE),
	m_micromap(VK_NULL_HANDLE)
{
}

OpacityMicromapArray_VK::~OpacityMicromapArray_VK()
{
	destroyMicromap();
}

bool OpacityMicromapArray_VK::createMicromap(VkDeviceSize size)
{
	destroyMicromap();
	if (!m_pVkDevice || !m_resource || !size) return false;

	const auto& ext = m_pVkDevice->GetExtProcs();
	assert(ext.CreateMicromap);
	XUSG_N_RETURN(ext.CreateMicromap, false);

	VkMicromapCreateInfoEXT info = {};
	info.sType = VK_STRUCTURE_TYPE_MICROMAP_CREATE_INFO_EXT;
	info.buffer = reinterpret_cast<VkBuffer>(m_resource->GetNativeHandle());
	info.offset = m_byteOffset;
	info.size = size;
	info.type = VK_MICROMAP_TYPE_OPACITY_MICROMAP_EXT;
	XUSG_VK_CHECK(ext.CreateMicromap(m_pVkDevice->GetDevice(), &info, nullptr, &m_micromap), cerr, false);
	m_buildDesc.Micromap = m_micromap;
	RegisterVKMicromap(m_micromap);

	return true;
}

void OpacityMicromapArray_VK::destroyMicromap()
{
	if (m_micromap == VK_NULL_HANDLE) return;

	UnregisterVKMicromap(m_micromap);
	if (m_pVkDevice)
	{
		const auto destroy = m_pVkDevice->GetExtProcs().DestroyMicromap;
		if (destroy) destroy(m_pVkDevice->GetDevice(), m_micromap, nullptr);
	}
	m_micromap = VK_NULL_HANDLE;
	m_buildDesc.Micromap = VK_NULL_HANDLE;
}

VkMicromapEXT OpacityMicromapArray_VK::GetMicromap() const
{
	return m_micromap;
}

bool OpacityMicromapArray_VK::Prebuild(Device* pDevice, uint32_t numOpacityMicromaps,
	const GeometryBuffer& ommArrayDescs, BuildFlag flags)
{
	const auto pVkDevice = AsVK(pDevice);
	m_pVkDevice = pVkDevice;
	if (!RequireOpacityMicromap(pVkDevice)) return false;
	assert(numOpacityMicromaps == 1);
	if (numOpacityMicromaps != 1) return false;
	assert(ommArrayDescs.size() >= sizeof(PackedOmmArray));
	if (ommArrayDescs.size() < sizeof(PackedOmmArray)) return false;

	const auto& packed = *reinterpret_cast<const PackedOmmArray*>(ommArrayDescs.data());
	assert(packed.UsageOffset + sizeof(PackedOmmUsage) * packed.NumUsages <= ommArrayDescs.size());
	if (packed.UsageOffset + sizeof(PackedOmmUsage) * packed.NumUsages > ommArrayDescs.size()) return false;

	m_buildFlags = flags;
	const auto pUsages = reinterpret_cast<const PackedOmmUsage*>(ommArrayDescs.data() + packed.UsageOffset);
	if (OpacityMicromapUsesExt(pVkDevice))
	{
		m_buildDesc.UsagesExt.resize(packed.NumUsages);
		for (auto i = 0u; i < packed.NumUsages; ++i)
		{
			m_buildDesc.UsagesExt[i].count = pUsages[i].Count;
			m_buildDesc.UsagesExt[i].subdivisionLevel = pUsages[i].SubdivisionLevel;
			m_buildDesc.UsagesExt[i].format = static_cast<uint32_t>(GetVKOpacityMicromapFormat(
				static_cast<OpacityMicromapFormat>(pUsages[i].Format)));
		}

		m_buildDesc.MicromapInfo = {};
		m_buildDesc.MicromapInfo.sType = VK_STRUCTURE_TYPE_MICROMAP_BUILD_INFO_EXT;
		m_buildDesc.MicromapInfo.type = VK_MICROMAP_TYPE_OPACITY_MICROMAP_EXT;
		m_buildDesc.MicromapInfo.flags = OpacityMicromapExtBuildFlags(flags);
		m_buildDesc.MicromapInfo.mode = VK_BUILD_MICROMAP_MODE_BUILD_EXT;
		m_buildDesc.MicromapInfo.usageCountsCount = static_cast<uint32_t>(m_buildDesc.UsagesExt.size());
		m_buildDesc.MicromapInfo.pUsageCounts = m_buildDesc.UsagesExt.empty() ? nullptr : m_buildDesc.UsagesExt.data();
		m_buildDesc.MicromapInfo.ppUsageCounts = nullptr;
		m_buildDesc.MicromapInfo.data.deviceAddress = packed.Input;
		m_buildDesc.MicromapInfo.triangleArray.deviceAddress = packed.Triangles;
		m_buildDesc.MicromapInfo.triangleArrayStride = packed.TriangleStride;
		m_buildDesc.Type = VK_ACCELERATION_STRUCTURE_TYPE_GENERIC_KHR;
		m_buildDesc.Geometries.clear();
		m_buildDesc.PrimitiveCounts.clear();

		const auto& ext = pVkDevice->GetExtProcs();
		assert(ext.GetMicromapBuildSizes);
		XUSG_N_RETURN(ext.GetMicromapBuildSizes, false);

		VkMicromapBuildSizesInfoEXT sizes = {};
		sizes.sType = VK_STRUCTURE_TYPE_MICROMAP_BUILD_SIZES_INFO_EXT;
		ext.GetMicromapBuildSizes(pVkDevice->GetDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			&m_buildDesc.MicromapInfo, &sizes);
		m_prebuildInfo.ResultDataMaxByteSize = sizes.micromapSize;
		m_prebuildInfo.ScratchDataByteSize = sizes.buildScratchSize;
		m_prebuildInfo.UpdateScratchDataByteSize = 0;

		return m_prebuildInfo.ResultDataMaxByteSize;
	}

	m_buildDesc.MicromapData = {};
	m_buildDesc.MicromapData.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_MICROMAP_DATA_KHR;
	m_buildDesc.MicromapData.data = packed.Input;
	m_buildDesc.MicromapData.triangleArray = packed.Triangles;
	m_buildDesc.MicromapData.triangleArrayStride = packed.TriangleStride;
	m_buildDesc.OpacityMicromaps.clear();
	m_buildDesc.Micromap = VK_NULL_HANDLE;

	auto triangleCount = 0u;
	m_buildDesc.Usages.resize(packed.NumUsages);
	for (auto i = 0u; i < packed.NumUsages; ++i)
	{
		m_buildDesc.Usages[i].count = pUsages[i].Count;
		m_buildDesc.Usages[i].subdivisionLevel = pUsages[i].SubdivisionLevel;
		m_buildDesc.Usages[i].format = GetVKOpacityMicromapFormat(static_cast<OpacityMicromapFormat>(pUsages[i].Format));
		triangleCount += pUsages[i].Count;
	}

	VkAccelerationStructureGeometryKHR geometry = {};
	geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	geometry.geometryType = VK_GEOMETRY_TYPE_MICROMAP_KHR;
	m_buildDesc.Geometries = { geometry };
	m_buildDesc.PrimitiveCounts = { triangleCount };
	m_buildDesc.Type = VK_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_KHR;
	m_buildDesc.Flags = OpacityMicromapBuildFlags(flags);
	m_buildDesc.Info = {};
	m_buildDesc.Info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	m_buildDesc.Info.type = m_buildDesc.Type;
	m_buildDesc.Info.flags = m_buildDesc.Flags;
	m_buildDesc.Info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;

	return prebuild(pDevice);
}

bool OpacityMicromapArray_VK::Allocate(Device* pDevice, DescriptorTableLib* pDescriptorTableLib,
	size_t byteWidth, MemoryFlag memoryFlags, const wchar_t* name, uint32_t maxThreads)
{
	m_pVkDevice = AsVK(pDevice);
	if (!OpacityMicromapUsesExt(m_pVkDevice))
		return allocate(pDevice, byteWidth, pDescriptorTableLib, 0, memoryFlags, name, maxThreads);

	(void)pDescriptorTableLib;
	m_resource = Buffer::MakeShared();
	byteWidth = byteWidth ? byteWidth : GetResultDataMaxByteSize(false);
	XUSG_N_RETURN(AllocateDestBuffer(pDevice, m_resource.get(), byteWidth, 0,
		nullptr, 1, nullptr, memoryFlags, name, maxThreads), false);
	m_byteOffset = 0;

	return createMicromap(byteWidth);
}

void OpacityMicromapArray_VK::SetDestination(Device* pDevice, const Buffer::sptr destBuffer, uintptr_t byteOffset)
{
	m_pVkDevice = AsVK(pDevice);
	if (!OpacityMicromapUsesExt(m_pVkDevice))
	{
		AccelerationStructure_VK::SetDestination(pDevice, destBuffer, byteOffset, 0, nullptr);

		return;
	}

	m_resource = destBuffer;
	m_byteOffset = byteOffset;
	auto byteSize = GetResultDataMaxByteSize(false);
	if (!byteSize && m_resource && m_resource->GetWidth() > m_byteOffset)
		byteSize = static_cast<size_t>(m_resource->GetWidth() - m_byteOffset);
	createMicromap(byteSize);
}

void OpacityMicromapArray_VK::Build(CommandList* pCommandList, const Resource* pScratch, const OpacityMicromapArray* pSource,
	uint8_t numPostbuildInfoDescs, const PostbuildInfoType* pPostbuildInfoTypes)
{
	if (!RequireOpacityMicromap(m_pVkDevice)) return;

	if (pSource && static_cast<bool>(m_buildFlags & BuildFlag::ALLOW_OMM_UPDATE))
	{
		static auto asserted = false;
		if (!asserted)
		{
			asserted = true;
			assert(!"Opacity micromap updates are not supported");
		}
	}

	if (OpacityMicromapUsesExt(m_pVkDevice))
	{
		m_buildDesc.Micromap = m_micromap;
		m_buildDesc.MicromapInfo.mode = VK_BUILD_MICROMAP_MODE_BUILD_EXT;
		m_buildDesc.MicromapInfo.dstMicromap = m_micromap;
		m_buildDesc.MicromapInfo.usageCountsCount = static_cast<uint32_t>(m_buildDesc.UsagesExt.size());
		m_buildDesc.MicromapInfo.pUsageCounts = m_buildDesc.UsagesExt.empty() ? nullptr : m_buildDesc.UsagesExt.data();
		m_buildDesc.MicromapInfo.ppUsageCounts = nullptr;
		m_buildDesc.MicromapInfo.scratchData.deviceAddress = pScratch ? pScratch->GetVirtualAddress() : 0;
	}
	else
	{
		m_buildDesc.Info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		m_buildDesc.Info.srcAccelerationStructure = VK_NULL_HANDLE;
		m_buildDesc.Info.dstAccelerationStructure = GetHandle();
		m_buildDesc.Info.scratchData.deviceAddress = pScratch ? pScratch->GetVirtualAddress() : 0;
		FillBuildRanges(m_buildDesc);
	}

	if (numPostbuildInfoDescs)
	{
		if (m_postbuildInfo) UnregisterVKBuffer(reinterpret_cast<VkBuffer>(m_postbuildInfo->GetNativeHandle()));
		m_postbuildInfoRB = Buffer::MakeUnique();
		m_postbuildInfo = Buffer::MakeUnique();
		m_postbuildInfo->Create(pCommandList->GetDevice(), sizeof(uint64_t) * numPostbuildInfoDescs,
			ResourceFlag::DENY_SHADER_RESOURCE | ResourceFlag::ALLOW_UNORDERED_ACCESS);
		registerPostbuildBuffer();

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

	pCommandList->BuildRaytracingAccelerationStructure(&m_buildDesc, numPostbuildInfoDescs,
		numPostbuildInfoDescs ? postbuildInfoDescs.data() : nullptr);

	if (m_postbuildInfo) m_postbuildInfo->ReadBack(pCommandList, m_postbuildInfoRB.get());
}

void OpacityMicromapArray_VK::SetOmmArray(GeometryBuffer& ommArrayDescs, uint32_t numOpacityMicromaps, const Desc* pOmmArrayDescs)
{
	assert(numOpacityMicromaps == 0 || pOmmArrayDescs);
	assert(numOpacityMicromaps <= 1);
	if (numOpacityMicromaps != 1)
	{
		ommArrayDescs.clear();

		return;
	}

	const auto& desc = pOmmArrayDescs[0];
	assert(desc.pInputBuffer);
	assert(desc.pPerOmmDescs);
	assert(desc.NumOmmHistogramEntries == 0 || desc.pOmmHistogram);
	if (!desc.pInputBuffer || !desc.pPerOmmDescs || (desc.NumOmmHistogramEntries && !desc.pOmmHistogram))
	{
		ommArrayDescs.clear();

		return;
	}
	ommArrayDescs.resize(sizeof(PackedOmmArray) + sizeof(PackedOmmUsage) * desc.NumOmmHistogramEntries);
	auto& packed = *reinterpret_cast<PackedOmmArray*>(ommArrayDescs.data());
	packed.NumUsages = desc.NumOmmHistogramEntries;
	packed.UsageOffset = sizeof(PackedOmmArray);
	packed.Input = desc.pInputBuffer->GetVirtualAddress() + desc.InputOffset;
	packed.Triangles = desc.pPerOmmDescs->GetVirtualAddress() + desc.PerOmmDescOffset;
	packed.TriangleStride = desc.PerOmmDescStride;

	auto pUsages = reinterpret_cast<PackedOmmUsage*>(ommArrayDescs.data() + packed.UsageOffset);
	for (auto i = 0u; i < desc.NumOmmHistogramEntries; ++i)
	{
		pUsages[i].Count = desc.pOmmHistogram[i].Count;
		pUsages[i].SubdivisionLevel = desc.pOmmHistogram[i].SubdivisionLevel;
		pUsages[i].Format = static_cast<uint32_t>(desc.pOmmHistogram[i].Format);
	}
}

static_assert(sizeof(OpacityMicromapDesc) == sizeof(VkMicromapTriangleKHR),
	"OpacityMicromapDesc must match the Vulkan per-micromap record");

void OpacityMicromapArray_VK::SetOmmDescs(Device* pDevice, Buffer* pOmmDescsBuffer, uint32_t numOmmDescs,
	const OpacityMicromapDesc* pOmmDescs, MemoryFlag memoryFlags, const wchar_t* ommName)
{
	assert(numOmmDescs == 0 || pOmmDescs);

	vector<VkMicromapTriangleKHR> ommDescs(numOmmDescs);
	for (auto i = 0u; i < numOmmDescs; ++i)
	{
		ommDescs[i].dataOffset = pOmmDescs[i].ByteOffset;
		ommDescs[i].subdivisionLevel = static_cast<uint16_t>(pOmmDescs[i].SubdivisionLevel);
		ommDescs[i].format = static_cast<uint16_t>(GetVKOpacityMicromapFormat(pOmmDescs[i].Format));
	}

	if (pOmmDescsBuffer->GetNativeHandle())
	{
		void* pMappedData = pOmmDescsBuffer->Map();
		memcpy(pMappedData, ommDescs.data(), sizeof(VkMicromapTriangleKHR) * numOmmDescs);
		pOmmDescsBuffer->Unmap();
	}
	else AccelerationStructure_VK::AllocateUploadBuffer(pDevice, pOmmDescsBuffer,
		sizeof(VkMicromapTriangleKHR) * numOmmDescs,
		ommDescs.data(), memoryFlags, ommName);
}

size_t OpacityMicromapArray_VK::AlignOmmInput(size_t byteSize)
{
	return XUSG::Align(byteSize, 128);
}

size_t OpacityMicromapArray_VK::AlignOmmDesc(size_t byteSize)
{
	return XUSG::Align(byteSize, 4);
}
