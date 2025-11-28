//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "DXFrameworkHelper.h"
#include "Core/XUSG_DX12.h"
#include "Core/XUSGResource_DX12.h"
#include "XUSGShaderTable_DX12.h"

using namespace std;
using namespace XUSG;
using namespace XUSG::RayTracing;

ShaderRecord_DX12::ShaderRecord_DX12(const void* pShaderIdentifier, uint32_t shaderIdentifierSize,
	const void* pLocalDescriptorArgs, uint32_t localDescriptorArgSize) :
	m_shaderIdentifier(pShaderIdentifier, shaderIdentifierSize),
	m_localDescriptorArgs(pLocalDescriptorArgs, localDescriptorArgSize)
{
}

ShaderRecord_DX12::ShaderRecord_DX12(const Device* pDevice, const Pipeline& pipeline,
	const wchar_t* shaderName, const void* pLocalDescriptorArgs, uint32_t localDescriptorArgSize) :
	ShaderRecord_DX12(GetShaderIdentifier(pipeline, shaderName), GetShaderIdentifierSize(pDevice),
		pLocalDescriptorArgs, localDescriptorArgSize)
{
}

ShaderRecord_DX12::~ShaderRecord_DX12()
{
}

void ShaderRecord_DX12::CopyTo(void* dest) const
{
	const auto byteDest = static_cast<uint8_t*>(dest);
	memcpy(dest, m_shaderIdentifier.Ptr, m_shaderIdentifier.Size);

	if (m_localDescriptorArgs.Ptr)
		memcpy(byteDest + m_shaderIdentifier.Size, m_localDescriptorArgs.Ptr, m_localDescriptorArgs.Size);
}

const void* XUSG::RayTracing::ShaderRecord_DX12::GetShaderIdentifier(const Pipeline& pipeline, const wchar_t* shaderName)
{
	const auto pPipeline = static_cast<ID3D12RaytracingFallbackStateObject*>(pipeline);

	return pPipeline->GetShaderIdentifier(shaderName);
}

uint32_t ShaderRecord_DX12::GetShaderIdentifierSize(const Device* pDevice)
{
	const auto pDxDevice = static_cast<ID3D12RaytracingFallbackDevice*>(pDevice->GetRTHandle());
	const auto shaderIdentifierSize = pDxDevice->UsingRaytracingDriver() ?
		D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES : pDxDevice->GetShaderIdentifierSize();

	return shaderIdentifierSize;
}

size_t ShaderRecord_DX12::Align(uint32_t byteSize)
{
	return XUSG::Align(byteSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
}

//--------------------------------------------------------------------------------------

ShaderTable_DX12::ShaderTable_DX12() :
	m_resource(nullptr),
	m_byteOffset(0),
	m_byteSize(0),
	m_byteStride(0),
	m_mappedData(nullptr)
{
}

ShaderTable_DX12::~ShaderTable_DX12()
{
}

bool ShaderTable_DX12::Create(const XUSG::Device* pDevice, uint32_t numShaderRecords,
	uint32_t shaderRecordSize, MemoryFlag memoryFlags, const wchar_t* name)
{
	m_byteStride = ShaderRecord_DX12::Align(shaderRecordSize);

	m_resource = Buffer::MakeShared(API::DIRECTX_12);
	const auto byteWidth = Align(m_byteStride * numShaderRecords);
	XUSG_N_RETURN(m_resource->CreateResource(pDevice, byteWidth, ResourceFlag::NONE, MemoryType::UPLOAD,
		memoryFlags, ResourceState::GENERIC_READ_RESOURCE), false);
	m_resource->SetName(name);

	m_byteOffset = 0;
	Reset();

	m_mappedData = static_cast<uint8_t*>(m_resource->Map());

	return true;
}

void ShaderTable_DX12::Create(Buffer::sptr resource, uint32_t shaderRecordSize, uintptr_t byteOffset)
{
	m_byteStride = ShaderRecord_DX12::Align(shaderRecordSize);

	m_resource = resource;
	m_byteOffset = byteOffset;
	Reset();

	m_mappedData = static_cast<uint8_t*>(m_resource->Map()) + byteOffset;
}

void ShaderTable_DX12::AddShaderRecord(const ShaderRecord* pShaderRecord)
{
	pShaderRecord->CopyTo(m_mappedData + m_byteSize);
	m_byteSize += m_byteStride;
}

void ShaderTable_DX12::Reset()
{
	m_byteSize = 0;
}

uint64_t ShaderTable_DX12::GetVirtualAddress() const
{
	return m_resource->GetVirtualAddress() + m_byteOffset;
}

size_t ShaderTable_DX12::GetByteSize() const
{
	return m_byteSize;
}

size_t ShaderTable_DX12::GetByteStride() const
{
	return m_byteStride;
}

size_t ShaderTable_DX12::Align(size_t byteSize)
{
	return XUSG::Align(byteSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
}
