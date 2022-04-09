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

ShaderRecord_DX12::ShaderRecord_DX12(const Device* pDevice, const Pipeline& pipeline,
	const void* shader, const void* pLocalDescriptorArgs, uint32_t localDescriptorArgSize) :
	m_localDescriptorArgs(pLocalDescriptorArgs, localDescriptorArgSize)
{
#if XUSG_ENABLE_DXR_FALLBACK
	m_shaderID.Ptr = static_cast<ID3D12RaytracingFallbackStateObject*>(pipeline)->GetShaderIdentifier(reinterpret_cast<const wchar_t*>(shader));
#else // DirectX Raytracing
	com_ptr<ID3D12StateObjectProperties> stateObjectProperties;
	ThrowIfFailed(static_cast<ID3D12StateObject*>(pipeline)->QueryInterface(IID_PPV_ARGS(&stateObjectProperties)));
	m_shaderID.Ptr = stateObjectProperties->GetShaderIdentifier(reinterpret_cast<const wchar_t*>(shader));
#endif

	m_shaderID.Size = GetShaderIDSize(pDevice);
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

uint32_t ShaderRecord_DX12::GetShaderIDSize(const Device* pDevice)
{
#if XUSG_ENABLE_DXR_FALLBACK
	const auto pDxDevice = static_cast<ID3D12RaytracingFallbackDevice*>(pDevice->GetRTHandle());
	const auto shaderIDSize = pDxDevice->UsingRaytracingDriver() ?
		D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES : pDxDevice->GetShaderIdentifierSize();
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

bool ShaderTable_DX12::Create(const XUSG::Device* pDevice, uint32_t numShaderRecords,
	uint32_t shaderRecordSize, const wchar_t* name)
{
	if (m_resource) Unmap();

	m_shaderRecordSize = (shaderRecordSize + D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT - 1) &
		~(D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT - 1);
	//m_shaderRecords.reserve(numShaderRecords);

	const auto bufferWidth = numShaderRecords * m_shaderRecordSize;
	XUSG_N_RETURN(allocate(pDevice, bufferWidth, name), false);
	XUSG_N_RETURN(Map(), false);

	return true;
}

bool ShaderTable_DX12::AddShaderRecord(const ShaderRecord* pShaderRecord)
{
	//if (m_shaderRecords.size() >= m_shaderRecords.capacity()) return false;
	//m_shaderRecords.push_back(shaderRecord);
	pShaderRecord->CopyTo(m_mappedShaderRecords);
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
		V_RETURN(static_cast<ID3D12Resource*>(m_resource->GetHandle())->Map(0, &readRange, &m_mappedShaderRecords), cerr, nullptr);
	}

	return m_mappedShaderRecords;
}

void ShaderTable_DX12::Unmap()
{
	if (m_mappedShaderRecords)
	{
		static_cast<ID3D12Resource*>(m_resource->GetHandle())->Unmap(0, nullptr);
		m_mappedShaderRecords = nullptr;
	}
}

void ShaderTable_DX12::Reset()
{
	Unmap();
	Map();
}

const Resource* ShaderTable_DX12::GetResource() const
{
	return m_resource.get();
}

uint32_t ShaderTable_DX12::GetShaderRecordSize() const
{
	return m_shaderRecordSize;
}

bool ShaderTable_DX12::allocate(const XUSG::Device* pDevice, uint32_t byteWidth, const wchar_t* name)
{
	const auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	const auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(byteWidth);

	m_resource = Resource::MakeUnique(API::DIRECTX_12);
	const auto pDxDevice = static_cast<ID3D12Device*>(pDevice->GetHandle());
	auto& resource = dynamic_cast<Resource_DX12*>(m_resource.get())->GetResource();

	assert(pDxDevice);
	V_RETURN(pDxDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE,
		&bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&resource)), cerr, false);
	resource->SetName(name);

	return true;
}
