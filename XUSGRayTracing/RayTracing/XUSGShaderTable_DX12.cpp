//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "DXFrameworkHelper.h"
#include "XUSGShaderTable_DX12.h"

using namespace std;
using namespace XUSG;
using namespace XUSG::RayTracing;

ShaderRecord_DX12::ShaderRecord_DX12(const RayTracing::Device& device, const RayTracing::Pipeline& pipeline,
	const void* shader, const void* pLocalDescriptorArgs, uint32_t localDescriptorArgSize) :
	m_localDescriptorArgs(pLocalDescriptorArgs, localDescriptorArgSize)
{
#if ENABLE_DXR_FALLBACK
	if (device.RaytracingAPI == RayTracing::API::FallbackLayer)
		m_shaderID.Ptr = pipeline.Fallback->GetShaderIdentifier(reinterpret_cast<const wchar_t*>(shader));
	else // DirectX Raytracing
#endif
	{
		ComPtr<ID3D12StateObjectProperties> stateObjectProperties;
		ThrowIfFailed(pipeline.Native.As(&stateObjectProperties));
		m_shaderID.Ptr = stateObjectProperties->GetShaderIdentifier(reinterpret_cast<const wchar_t*>(shader));
	}

	m_shaderID.Size = GetShaderIDSize(device);
}

ShaderRecord_DX12::ShaderRecord_DX12(void* pShaderID, uint32_t shaderIDSize,
	const void* pLocalDescriptorArgs, uint32_t localDescriptorArgSize) :
	m_shaderID(pShaderID, shaderIDSize),
	m_localDescriptorArgs(pLocalDescriptorArgs, localDescriptorArgSize)
{
}

ShaderRecord_DX12::~ShaderRecord_DX12()
{
}

void ShaderRecord_DX12::CopyTo(void* dest) const
{
	const auto byteDest = static_cast<uint8_t*>(dest);
	memcpy(dest, m_shaderID.Ptr, m_shaderID.Size);

	if (m_localDescriptorArgs.Ptr)
		memcpy(byteDest + m_shaderID.Size, m_localDescriptorArgs.Ptr, m_localDescriptorArgs.Size);
}

uint32_t ShaderRecord_DX12::GetShaderIDSize(const RayTracing::Device& device)
{
#if ENABLE_DXR_FALLBACK
	const auto shaderIDSize = device.RaytracingAPI == RayTracing::API::FallbackLayer ?
		device.Fallback->GetShaderIdentifierSize() : D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
#else
	const auto shaderIDSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
#endif

	return shaderIDSize;
}

//--------------------------------------------------------------------------------------

ShaderTable_DX12::ShaderTable_DX12() :
	m_resource(nullptr),
	m_mappedShaderRecords(nullptr)
{
}

ShaderTable_DX12::~ShaderTable_DX12()
{
	if (m_resource) Unmap();
}

bool ShaderTable_DX12::Create(const RayTracing::Device& device, uint32_t numShaderRecords,
	uint32_t shaderRecordSize, const wchar_t* name)
{
	if (m_resource) Unmap();

	m_shaderRecordSize = (shaderRecordSize + D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT - 1) &
		~(D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT - 1);
	//m_shaderRecords.reserve(numShaderRecords);

	const auto bufferWidth = numShaderRecords * m_shaderRecordSize;
	N_RETURN(allocate(device, bufferWidth, name), false);
	N_RETURN(Map(), false);

	return true;
}

bool ShaderTable_DX12::AddShaderRecord(const ShaderRecord& shaderRecord)
{
	//if (m_shaderRecords.size() >= m_shaderRecords.capacity()) return false;
	//m_shaderRecords.push_back(shaderRecord);
	shaderRecord.CopyTo(m_mappedShaderRecords);
	reinterpret_cast<uint8_t*&>(m_mappedShaderRecords) += m_shaderRecordSize;

	return true;
}

void* ShaderTable_DX12::Map()
{
	if (m_mappedShaderRecords == nullptr)
	{
		// Map and initialize the constant buffer. We don't unmap this until the
		// app closes. Keeping things mapped for the lifetime of the resource is okay.
		CD3DX12_RANGE readRange(0, 0);	// We do not intend to read from this resource on the CPU.
		V_RETURN(m_resource->Map(0, &readRange, &m_mappedShaderRecords), cerr, nullptr);
	}

	return m_mappedShaderRecords;
}

void ShaderTable_DX12::Unmap()
{
	if (m_mappedShaderRecords)
	{
		m_resource->Unmap(0, nullptr);
		m_mappedShaderRecords = nullptr;
	}
}

void ShaderTable_DX12::Reset()
{
	Unmap();
	Map();
}

const Resource& ShaderTable_DX12::GetResource() const
{
	return m_resource;
}

uint32_t ShaderTable_DX12::GetShaderRecordSize() const
{
	return m_shaderRecordSize;
}

bool ShaderTable_DX12::allocate(const RayTracing::Device& device, uint32_t byteWidth, const wchar_t* name)
{
	const auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	const auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(byteWidth);

	V_RETURN(device.Common->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE,
		&bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&m_resource)), cerr, false);
	m_resource->SetName(name);

	return true;
}
