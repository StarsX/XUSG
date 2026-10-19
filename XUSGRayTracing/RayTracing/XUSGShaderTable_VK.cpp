//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSG_VK.h"
#include "XUSGShaderTable_VK.h"
#include "XUSGRayTracingState_VK.h"

using namespace std;
using namespace XUSG;
using namespace XUSG::RayTracing;

namespace
{
	uint32_t g_identifierSize = 32;
	uint32_t g_recordAlignment = 32;
	uint32_t g_tableAlignment = 64;
}

void XUSG::RayTracing::CacheShaderRecordAlignment(uint32_t identifierSize, uint32_t recordAlignment, uint32_t tableAlignment)
{
	if (identifierSize) g_identifierSize = identifierSize;
	if (recordAlignment) g_recordAlignment = recordAlignment;
	if (tableAlignment) g_tableAlignment = tableAlignment;
}

ShaderRecord_VK::ShaderRecord_VK(const void* pShaderIdentifier, uint32_t shaderIdentifierSize,
	const void* pLocalDescriptorArgs, uint32_t localDescriptorArgSize) :
	m_shaderIdentifier(pShaderIdentifier, shaderIdentifierSize),
	m_localDescriptorArgs(pLocalDescriptorArgs, localDescriptorArgSize)
{
}

ShaderRecord_VK::ShaderRecord_VK(Device* pDevice, const Pipeline& pipeline,
	const wchar_t* shaderName, const void* pLocalDescriptorArgs, uint32_t localDescriptorArgSize) :
	ShaderRecord_VK(GetShaderIdentifier(pipeline, shaderName), GetShaderIdentifierSize(pDevice),
		pLocalDescriptorArgs, localDescriptorArgSize)
{
}

ShaderRecord_VK::~ShaderRecord_VK()
{
}

void ShaderRecord_VK::CopyTo(void* dest) const
{
	const auto byteDest = static_cast<uint8_t*>(dest);
	memcpy(dest, m_shaderIdentifier.Ptr, m_shaderIdentifier.Size);

	if (m_localDescriptorArgs.Ptr)
		memcpy(byteDest + m_shaderIdentifier.Size, m_localDescriptorArgs.Ptr, m_localDescriptorArgs.Size);
}

const void* XUSG::RayTracing::ShaderRecord_VK::GetShaderIdentifier(const Pipeline& pipeline, const wchar_t* shaderName)
{
	const auto pPipeline = static_cast<const RayTracingPipeline_VK*>(pipeline);
	assert(pPipeline);
	assert(shaderName);
	if (!pPipeline || !shaderName || !pPipeline->HandleSize) return nullptr;

	const auto group = pPipeline->Groups.find(shaderName);
	assert(group != pPipeline->Groups.end());
	if (group == pPipeline->Groups.end()) return nullptr;

	return pPipeline->Handles.data() + static_cast<size_t>(group->second) * pPipeline->HandleSize;
}

uint32_t ShaderRecord_VK::GetShaderIdentifierSize(Device* pDevice)
{
	const auto pDeviceVK = AsVK(pDevice);
	assert(pDeviceVK);
	const auto size = pDeviceVK ? pDeviceVK->GetShaderGroupHandleSize() : g_identifierSize;
	const auto recordAlignment = pDeviceVK ? pDeviceVK->GetShaderGroupHandleAlignment() : g_recordAlignment;
	const auto tableAlignment = pDeviceVK ? pDeviceVK->GetShaderGroupBaseAlignment() : g_tableAlignment;
	CacheShaderRecordAlignment(size, recordAlignment, tableAlignment);

	return size ? size : g_identifierSize;
}

size_t ShaderRecord_VK::Align(uint32_t byteSize)
{
	return XUSG::Align(byteSize, g_recordAlignment ? g_recordAlignment : 32u);
}

//--------------------------------------------------------------------------------------

ShaderTable_VK::ShaderTable_VK() :
	m_resource(nullptr),
	m_byteOffset(0),
	m_byteSize(0),
	m_byteStride(0),
	m_mappedData(nullptr)
{
}

ShaderTable_VK::~ShaderTable_VK()
{
}

bool ShaderTable_VK::Create(XUSG::Device* pDevice, uint32_t numShaderRecords,
	uint32_t shaderRecordSize, MemoryFlag memoryFlags, const wchar_t* name)
{
	m_byteStride = ShaderRecord_VK::Align(shaderRecordSize);

	m_resource = Buffer::MakeShared();
	const auto byteWidth = Align(m_byteStride * numShaderRecords);
	XUSG_N_RETURN(m_resource->CreateResource(pDevice, byteWidth, ResourceFlag::SHADER_TABLE, MemoryType::UPLOAD,
		memoryFlags, ResourceState::GENERIC_READ_RESOURCE), false);
	m_resource->SetName(name);

	m_byteOffset = 0;
	Reset();

	m_mappedData = static_cast<uint8_t*>(m_resource->Map());

	return true;
}

void ShaderTable_VK::Create(Buffer::sptr resource, uint32_t shaderRecordSize, uintptr_t byteOffset)
{
	m_byteStride = ShaderRecord_VK::Align(shaderRecordSize);

	m_resource = resource;
	m_byteOffset = byteOffset;
	Reset();

	m_mappedData = static_cast<uint8_t*>(m_resource->Map()) + byteOffset;
}

void ShaderTable_VK::AddShaderRecord(const ShaderRecord* pShaderRecord)
{
	pShaderRecord->CopyTo(m_mappedData + m_byteSize);
	m_byteSize += m_byteStride;
}

void ShaderTable_VK::Reset()
{
	m_byteSize = 0;
}

uint64_t ShaderTable_VK::GetVirtualAddress() const
{
	return m_resource->GetVirtualAddress() + m_byteOffset;
}

size_t ShaderTable_VK::GetByteSize() const
{
	return m_byteSize;
}

size_t ShaderTable_VK::GetByteStride() const
{
	return m_byteStride;
}

size_t ShaderTable_VK::Align(size_t byteSize)
{
	return XUSG::Align(byteSize, g_tableAlignment ? g_tableAlignment : 64u);
}
