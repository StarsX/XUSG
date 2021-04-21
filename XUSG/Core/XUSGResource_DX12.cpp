//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "DXFrameworkHelper.h"
#include "XUSG_DX12.h"
#include "XUSGResource_DX12.h"
#include "XUSGCommand_DX12.h"
#include "XUSGEnum_DX12.h"

using namespace std;
using namespace XUSG;

Format MapToPackedFormat(Format& format)
{
	auto formatUAV = Format::R32_UINT;

	switch (format)
	{
	case Format::R10G10B10A2_UNORM:
	case Format::R10G10B10A2_UINT:
		format = Format::R10G10B10A2_TYPELESS;
		break;
	case Format::R8G8B8A8_UNORM:
	case Format::R8G8B8A8_UNORM_SRGB:
	case Format::R8G8B8A8_UINT:
	case Format::R8G8B8A8_SNORM:
	case Format::R8G8B8A8_SINT:
		format = Format::R8G8B8A8_TYPELESS;
		break;
	case Format::B8G8R8A8_UNORM:
	case Format::B8G8R8A8_UNORM_SRGB:
		format = Format::B8G8R8A8_TYPELESS;
		break;
	case Format::B8G8R8X8_UNORM:
	case Format::B8G8R8X8_UNORM_SRGB:
		format = Format::B8G8R8X8_TYPELESS;
		break;
	case Format::R16G16_FLOAT:
	case Format::R16G16_UNORM:
	case Format::R16G16_UINT:
	case Format::R16G16_SNORM:
	case Format::R16G16_SINT:
		format = Format::R16G16_TYPELESS;
		break;
	default:
		formatUAV = format;
	}

	return formatUAV;
}

//--------------------------------------------------------------------------------------
// Resource base
//--------------------------------------------------------------------------------------

Resource_DX12::Resource_DX12() :
	m_device(nullptr),
	m_resource(nullptr),
	m_states()
{
}

Resource_DX12::~Resource_DX12()
{
}

uint32_t Resource_DX12::SetBarrier(ResourceBarrier* pBarriers, ResourceState dstState,
	uint32_t numBarriers, uint32_t subresource, BarrierFlag flags)
{
	const auto& state = m_states[subresource == BARRIER_ALL_SUBRESOURCES ? 0 : subresource];
	if (state != dstState || dstState == ResourceState::UNORDERED_ACCESS)
		pBarriers[numBarriers++] = Transition(dstState, subresource, flags);

	return numBarriers;
}

ResourceBarrier Resource_DX12::Transition(ResourceState dstState,
	uint32_t subresource, BarrierFlag flags)
{
	ResourceState srcState;
	if (subresource == BARRIER_ALL_SUBRESOURCES)
	{
		srcState = m_states[0];
		if (flags != BarrierFlag::BEGIN_ONLY)
			for (auto& state : m_states) state = dstState;
	}
	else
	{
		srcState = m_states[subresource];
		m_states[subresource] = flags == BarrierFlag::BEGIN_ONLY ? srcState : dstState;
	}

	return { this, srcState, dstState, subresource, flags };
}

ResourceState Resource_DX12::GetResourceState(uint32_t subresource) const
{
	return m_states[subresource];
}

uint32_t Resource_DX12::GetWidth() const
{
	return static_cast<uint32_t>(m_resource->GetDesc().Width);
}

uint64_t Resource_DX12::GetVirtualAddress(int offset) const
{
	return m_resource->GetGPUVirtualAddress() + offset;
}

void Resource_DX12::Create(void* pDeviceHandle, void* pResourceHandle, const wchar_t* name)
{
	m_device = static_cast<ID3D12Device*>(pDeviceHandle);
	m_resource = static_cast<ID3D12Resource*>(pResourceHandle);

	const auto desc = m_resource->GetDesc();
	m_states.resize(desc.MipLevels * desc.DepthOrArraySize, ResourceState::COMMON);

	if (name) m_resource->SetName((wstring(name) + L".Resource").c_str());
}

void* Resource_DX12::GetHandle() const
{
	return m_resource.get();
}

com_ptr<ID3D12Resource>& Resource_DX12::GetResource()
{
	return m_resource;
}

//--------------------------------------------------------------------------------------
// Constant buffer
//--------------------------------------------------------------------------------------

ConstantBuffer_DX12::ConstantBuffer_DX12() :
	Resource_DX12(),
	m_cbvHeaps(0),
	m_cbvs(0),
	m_cbvOffsets(0),
	m_pDataBegin(nullptr)
{
}

ConstantBuffer_DX12::~ConstantBuffer_DX12()
{
	if (m_resource) Unmap();
}

bool ConstantBuffer_DX12::Create(const Device* pDevice, size_t byteWidth, uint32_t numCBVs,
	const size_t* offsets, MemoryType memoryType, MemoryFlag memoryFlags, const wchar_t* name)
{
	m_device = static_cast<ID3D12Device*>(pDevice->GetHandle());
	M_RETURN(!m_device, cerr, "The device is NULL.", false);
	m_cbvHeaps.clear();

	// Instanced CBVs
	vector<size_t> offsetList;
	if (!offsets)
	{
		size_t numBytes = 0;
		// CB size is required to be D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT-byte aligned.
		auto cbvSize = byteWidth / numCBVs;
		cbvSize = CalculateConstantBufferByteSize(static_cast<uint32_t>(cbvSize));
		offsetList.resize(numCBVs);

		for (auto& offset : offsetList)
		{
			offset = numBytes;
			numBytes += cbvSize;
		}

		offsets = offsetList.data();
		byteWidth = cbvSize * numCBVs;
	}

	const CD3DX12_HEAP_PROPERTIES heapProperties(GetDX12HeapType(memoryType));
	const auto desc = CD3DX12_RESOURCE_DESC::Buffer(byteWidth, D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE);

	V_RETURN(m_device->CreateCommittedResource(&heapProperties, GetDX12HeapFlags(memoryFlags), &desc,
		memoryType == MemoryType::DEFAULT ? D3D12_RESOURCE_STATE_COMMON : D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(&m_resource)), clog, false);
	if (name) m_resource->SetName((wstring(name) + L".Resource").c_str());

	// Describe and create a constant buffer view.
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC desc;

		m_cbvs.resize(numCBVs);
		m_cbvOffsets.resize(numCBVs);
		const auto maxSize = static_cast<uint32_t>(byteWidth);
		for (auto i = 0u; i < numCBVs; ++i)
		{
			const auto& offset = offsets[i];
			desc.BufferLocation = m_resource->GetGPUVirtualAddress() + offset;
			desc.SizeInBytes = static_cast<uint32_t>((i + 1 >= numCBVs ? maxSize : offsets[i + 1]) - offset);

			m_cbvOffsets[i] = offset;

			// Create a constant buffer view
			m_cbvs[i] = allocateCbvHeap(name);
			m_device->CreateConstantBufferView(&desc, { m_cbvs[i] });
		}
	}

	return true;
}

bool ConstantBuffer_DX12::Upload(CommandList* pCommandList, Resource* pUploader, const void* pData,
	size_t size, uint32_t cbvIndex, ResourceState srcState, ResourceState dstState)
{
	const auto offset = m_cbvOffsets.empty() ? 0 : m_cbvOffsets[cbvIndex];
	D3D12_SUBRESOURCE_DATA subresourceData;
	subresourceData.pData = pData;
	subresourceData.RowPitch = static_cast<uint32_t>(size);
	subresourceData.SlicePitch = static_cast<uint32_t>(m_resource->GetDesc().Width);

	// Create the GPU upload buffer.
	assert(pUploader);
	auto& uploaderResource = dynamic_cast<Resource_DX12*>(pUploader)->GetResource();
	if (!uploaderResource)
	{
		const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
		const auto cbSize = static_cast<uint32_t>(size + offset);
		const auto desc = CD3DX12_RESOURCE_DESC::Buffer(CalculateConstantBufferByteSize(cbSize));

		V_RETURN(m_device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploaderResource)), clog, false);
		if (!m_name.empty()) uploaderResource->SetName((m_name + L".UploaderResource").c_str());
	}

	// Copy data to the intermediate upload heap and then schedule a copy 
	// from the upload heap to the buffer.
	if (srcState != ResourceState::COMMON)
	{
		const ResourceBarrier barrier = { this, srcState, dstState, BARRIER_ALL_SUBRESOURCES };
		pCommandList->Barrier(1, &barrier);
	}

	const auto pGraphicsCommandList = static_cast<ID3D12GraphicsCommandList*>(pCommandList->GetHandle());
	M_RETURN(UpdateSubresources(pGraphicsCommandList, m_resource.get(), uploaderResource.get(), offset, 0, 1, &subresourceData) <= 0,
		clog, "Failed to upload the resource.", false);

	if (dstState != ResourceState::COMMON)
	{
		const ResourceBarrier barrier = { this, ResourceState::COPY_DEST, dstState, BARRIER_ALL_SUBRESOURCES };
		pCommandList->Barrier(1, &barrier);
	}

	return true;
}

void* ConstantBuffer_DX12::Map(uint32_t cbvIndex)
{
	if (m_pDataBegin == nullptr)
	{
		// Map and initialize the constant buffer. We don't unmap this until the
		// app closes. Keeping things mapped for the lifetime of the resource is okay.
		CD3DX12_RANGE readRange(0, 0);	// We do not intend to read from this resource on the CPU.
		V_RETURN(m_resource->Map(0, &readRange, &m_pDataBegin), cerr, nullptr);
	}

	return &reinterpret_cast<uint8_t*>(m_pDataBegin)[m_cbvOffsets[cbvIndex]];
}

void ConstantBuffer_DX12::Unmap()
{
	if (m_pDataBegin)
	{
		m_resource->Unmap(0, nullptr);
		m_pDataBegin = nullptr;
	}
}

const Descriptor& ConstantBuffer_DX12::GetCBV(uint32_t index) const
{
	assert(m_cbvs.size() > index);
	return m_cbvs[index];
}

uint32_t ConstantBuffer_DX12::GetCBVOffset(uint32_t index) const
{
	return static_cast<uint32_t>(m_cbvOffsets[index]);
}

Descriptor ConstantBuffer_DX12::allocateCbvHeap(const wchar_t* name)
{
	m_cbvHeaps.emplace_back();
	auto& cbvPool = m_cbvHeaps.back();

	D3D12_DESCRIPTOR_HEAP_DESC desc = { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1 };
	V_RETURN(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&cbvPool)), cerr, 0);
	if (name) cbvPool->SetName((wstring(name) + L".CbvPool").c_str());

	return cbvPool->GetCPUDescriptorHandleForHeapStart().ptr;
}

//--------------------------------------------------------------------------------------
// Resource base
//--------------------------------------------------------------------------------------

ShaderResource_DX12::ShaderResource_DX12() :
	Resource_DX12(),
	m_format(Format::UNKNOWN),
	m_srvUavHeaps(0),
	m_srvs(0)
{
}

ShaderResource_DX12::~ShaderResource_DX12()
{
}

const Descriptor& ShaderResource_DX12::GetSRV(uint32_t index) const
{
	assert(m_srvs.size() > index);
	return m_srvs[index];
}

Format ShaderResource_DX12::GetFormat() const
{
	return m_format;
}

bool ShaderResource_DX12::setDevice(const Device* pDevice)
{
	m_device = static_cast<ID3D12Device*>(pDevice->GetHandle());
	M_RETURN(!m_device, cerr, "The device is NULL.", false);
	m_srvUavHeaps.clear();

	return true;
}

Descriptor ShaderResource_DX12::allocateSrvUavHeap()
{
	m_srvUavHeaps.emplace_back();
	auto& srvUavPool = m_srvUavHeaps.back();

	D3D12_DESCRIPTOR_HEAP_DESC desc = { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1 };
	V_RETURN(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&srvUavPool)), cerr, 0);
	if (!m_name.empty()) srvUavPool->SetName((m_name + L".SrvUavPool").c_str());

	return srvUavPool->GetCPUDescriptorHandleForHeapStart().ptr;
}

//--------------------------------------------------------------------------------------
// 2D Texture
//--------------------------------------------------------------------------------------

Texture2D_DX12::Texture2D_DX12() :
	ShaderResource_DX12(),
	m_uavs(0),
	m_packedUavs(0),
	m_srvLevels(0)
{
}

Texture2D_DX12::~Texture2D_DX12()
{
}

bool Texture2D_DX12::Create(const Device* pDevice, uint32_t width, uint32_t height, Format format,
	uint32_t arraySize, ResourceFlag resourceFlags, uint8_t numMips, uint8_t sampleCount,
	bool isCubeMap, MemoryFlag memoryFlags, const wchar_t* name)
{
	N_RETURN(setDevice(pDevice), false);

	if (name) m_name = name;

	const auto needPackedUAV = (resourceFlags & ResourceFlag::NEED_PACKED_UAV) == ResourceFlag::NEED_PACKED_UAV;
	const auto hasSRV = (resourceFlags & ResourceFlag::DENY_SHADER_RESOURCE) == ResourceFlag::NONE;
	const bool hasUAV = (resourceFlags & ResourceFlag::ALLOW_UNORDERED_ACCESS) == ResourceFlag::ALLOW_UNORDERED_ACCESS;

	// Map formats
	m_format = format;
	const auto formatUAV = needPackedUAV ? MapToPackedFormat(format) : format;

	// Setup the texture description.
	const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
	const auto desc = CD3DX12_RESOURCE_DESC::Tex2D(GetDXGIFormat(format),
		width, height, arraySize, numMips, sampleCount, 0, GetDX12ResourceFlags(resourceFlags));

	// Determine initial state
	m_states.resize(arraySize * numMips, ResourceState::COMMON);

	V_RETURN(m_device->CreateCommittedResource(&heapProperties, GetDX12HeapFlags(memoryFlags), &desc,
		GetDX12ResourceStates(m_states[0]), nullptr, IID_PPV_ARGS(&m_resource)), clog, false);
	if (!m_name.empty()) m_resource->SetName((m_name + L".Resource").c_str());

	// Create SRV
	if (hasSRV) N_RETURN(CreateSRVs(arraySize, m_format, numMips, sampleCount, isCubeMap), false);

	// Create UAVs
	if (hasUAV)
	{
		N_RETURN(CreateUAVs(arraySize, m_format, numMips), false);
		if (needPackedUAV) N_RETURN(CreateUAVs(arraySize, formatUAV, numMips, &m_packedUavs), false);
	}

	// Create SRV for each level
	if (hasSRV && hasUAV) N_RETURN(CreateSRVLevels(arraySize, numMips, m_format, sampleCount, isCubeMap), false);

	return true;
}

bool Texture2D_DX12::Upload(CommandList* pCommandList, Resource* pUploader,
	const SubresourceData* pSubresourceData, uint32_t numSubresources,
	ResourceState dstState, uint32_t firstSubresource)
{
	N_RETURN(pSubresourceData, false);
	vector<D3D12_SUBRESOURCE_DATA> subresourceData(numSubresources);
	for (auto i = 0u; i < numSubresources; ++i)
	{
		subresourceData[i].pData = pSubresourceData[i].pData;
		subresourceData[i].RowPitch = pSubresourceData[i].RowPitch;
		subresourceData[i].SlicePitch = pSubresourceData[i].SlicePitch;
	};

	// Create the GPU upload buffer.
	assert(pUploader);
	auto& uploaderResource = dynamic_cast<Resource_DX12*>(pUploader)->GetResource();
	if (!uploaderResource)
	{
		const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
		const auto uploadBufferSize = GetRequiredIntermediateSize(m_resource.get(),
			firstSubresource, numSubresources);
		const auto desc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

		V_RETURN(m_device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploaderResource)), clog, false);
		if (!m_name.empty()) uploaderResource->SetName((m_name + L".UploaderResource").c_str());
	}

	// Copy data to the intermediate upload heap and then schedule a copy 
	// from the upload heap to the Texture2D.
	const bool decay = m_resource->GetDesc().Flags & D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;
	ResourceBarrier barrier;
	auto numBarriers = SetBarrier(&barrier, ResourceState::COPY_DEST);
	if (m_states[0] != ResourceState::COMMON || !decay) pCommandList->Barrier(numBarriers, &barrier);

	const auto pGraphicsCommandList = static_cast<ID3D12GraphicsCommandList*>(pCommandList->GetHandle());
	M_RETURN(UpdateSubresources(pGraphicsCommandList, m_resource.get(), uploaderResource.get(), 0, firstSubresource,
		numSubresources, subresourceData.data()) <= 0,
		clog, "Failed to upload the resource.", false);

	numBarriers = SetBarrier(&barrier, dstState);
	if (dstState != ResourceState::COMMON || !decay) pCommandList->Barrier(numBarriers, &barrier);

	return true;
}

bool Texture2D_DX12::Upload(CommandList* pCommandList, Resource* pUploader,
	const void* pData, uint8_t stride, ResourceState dstState)
{
	const auto desc = m_resource->GetDesc();

	SubresourceData subresourceData;
	subresourceData.pData = pData;
	subresourceData.RowPitch = stride * static_cast<uint32_t>(desc.Width);
	subresourceData.SlicePitch = subresourceData.RowPitch * desc.Height;

	return Upload(pCommandList, pUploader, &subresourceData, 1, dstState);
}

bool Texture2D_DX12::CreateSRVs(uint32_t arraySize, Format format, uint8_t numMips,
	uint8_t sampleCount, bool isCubeMap)
{
	// Setup the description of the shader resource view.
	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	desc.Format = format != Format::UNKNOWN ? GetDXGIFormat(format) : m_resource->GetDesc().Format;
	desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	uint8_t mipLevel = 0;
	m_srvs.resize(sampleCount > 1 ? 1 : max<uint8_t>(numMips, 1));

	for (auto& descriptor : m_srvs)
	{
		if (isCubeMap)
		{
			assert(arraySize % 6 == 0);
			if (arraySize > 6)
			{
				desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
				desc.TextureCubeArray.MipLevels = numMips - mipLevel;
				desc.TextureCubeArray.MostDetailedMip = mipLevel++;
				desc.TextureCubeArray.NumCubes = arraySize / 6;
			}
			else
			{
				desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
				desc.TextureCube.MipLevels = numMips - mipLevel;
				desc.TextureCube.MostDetailedMip = mipLevel++;
			}
		}
		else if (arraySize > 1)
		{
			if (sampleCount > 1)
			{
				desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
				desc.Texture2DMSArray.ArraySize = arraySize;
			}
			else
			{
				desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
				desc.Texture2DArray.ArraySize = arraySize;
				desc.Texture2DArray.MipLevels = numMips - mipLevel;
				desc.Texture2DArray.MostDetailedMip = mipLevel++;
			}
		}
		else
		{
			if (sampleCount > 1)
				desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
			else
			{
				desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
				desc.Texture2D.MipLevels = numMips - mipLevel;
				desc.Texture2D.MostDetailedMip = mipLevel++;
			}
		}

		// Create a shader resource view
		X_RETURN(descriptor, allocateSrvUavHeap(), false);
		m_device->CreateShaderResourceView(m_resource.get(), &desc, { descriptor });
	}

	return true;
}

bool Texture2D_DX12::CreateSRVLevels(uint32_t arraySize, uint8_t numMips, Format format,
	uint8_t sampleCount, bool isCubeMap)
{
	if (numMips > 1)
	{
		// Setup the description of the shader resource view.
		D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
		desc.Format = format != Format::UNKNOWN ? GetDXGIFormat(format) : m_resource->GetDesc().Format;
		desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		uint8_t mipLevel = 0;
		m_srvLevels.resize(numMips);

		for (auto& descriptor : m_srvLevels)
		{
			// Setup the description of the shader resource view.
			if (isCubeMap)
			{
				assert(arraySize % 6 == 0);
				if (arraySize > 6)
				{
					desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
					desc.TextureCubeArray.MostDetailedMip = mipLevel++;
					desc.TextureCubeArray.MipLevels = 1;
					desc.TextureCubeArray.NumCubes = arraySize / 6;
				}
				else
				{
					desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
					desc.TextureCube.MostDetailedMip = mipLevel++;
					desc.TextureCube.MipLevels = 1;
				}
			}
			else if (arraySize > 1)
			{
				desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
				desc.Texture2DArray.ArraySize = arraySize;
				desc.Texture2DArray.MostDetailedMip = mipLevel++;
				desc.Texture2DArray.MipLevels = 1;
			}
			else
			{
				desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
				desc.Texture2D.MostDetailedMip = mipLevel++;
				desc.Texture2D.MipLevels = 1;
			}

			// Create a shader resource view
			X_RETURN(descriptor, allocateSrvUavHeap(), false);
			m_device->CreateShaderResourceView(m_resource.get(), &desc, { descriptor });
		}
	}

	return true;
}

bool Texture2D_DX12::CreateUAVs(uint32_t arraySize, Format format, uint8_t numMips, vector<Descriptor>* pUavs)
{
	// Setup the description of the unordered access view.
	D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
	desc.Format = format != Format::UNKNOWN ? GetDXGIFormat(format) : m_resource->GetDesc().Format;

	uint8_t mipLevel = 0;
	pUavs = pUavs ? pUavs : &m_uavs;
	pUavs->resize(max<uint8_t>(numMips, 1));

	for (auto& descriptor : *pUavs)
	{
		// Setup the description of the unordered access view.
		if (arraySize > 1)
		{
			desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
			desc.Texture2DArray.ArraySize = arraySize;
			desc.Texture2DArray.MipSlice = mipLevel++;
		}
		else
		{
			desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			desc.Texture2D.MipSlice = mipLevel++;
		}

		// Create an unordered access view
		X_RETURN(descriptor, allocateSrvUavHeap(), false);
		m_device->CreateUnorderedAccessView(m_resource.get(), nullptr, &desc, { descriptor });
	}

	return true;
}

uint32_t Texture2D_DX12::SetBarrier(ResourceBarrier* pBarriers, ResourceState dstState,
	uint32_t numBarriers, uint32_t subresource, BarrierFlag flags)
{
	return ShaderResource_DX12::SetBarrier(pBarriers, dstState, numBarriers, subresource, flags);
}

uint32_t Texture2D_DX12::SetBarrier(ResourceBarrier* pBarriers, uint8_t mipLevel, ResourceState dstState,
	uint32_t numBarriers, uint32_t slice, BarrierFlag flags)
{
	const auto desc = m_resource->GetDesc();
	const auto subresource = D3D12CalcSubresource(mipLevel, slice, 0, desc.MipLevels, desc.DepthOrArraySize);

	return SetBarrier(pBarriers, dstState, numBarriers, subresource, flags);
}

void Texture2D_DX12::Blit(const CommandList* pCommandList, uint32_t groupSizeX, uint32_t groupSizeY,
	uint32_t groupSizeZ, const DescriptorTable& uavSrvTable, uint32_t uavSrvSlot, uint8_t mipLevel,
	const DescriptorTable& srvTable, uint32_t srvSlot, const DescriptorTable& samplerTable,
	uint32_t samplerSlot, const Pipeline& pipeline)
{
	// Set pipeline layout and descriptor tables
	if (uavSrvTable) pCommandList->SetComputeDescriptorTable(uavSrvSlot, uavSrvTable);
	if (srvTable) pCommandList->SetComputeDescriptorTable(srvSlot, srvTable);
	if (samplerTable) pCommandList->SetComputeDescriptorTable(samplerSlot, samplerTable);

	// Set pipeline
	if (pipeline) pCommandList->SetPipelineState(pipeline);

	// Dispatch
	const auto desc = m_resource->GetDesc();
	const auto width = (max)(static_cast<uint32_t>(desc.Width >> mipLevel), 1u);
	const auto height = (max)(desc.Height >> mipLevel, 1u);
	pCommandList->Dispatch(DIV_UP(width, groupSizeX), DIV_UP(height, groupSizeY),
		DIV_UP(desc.DepthOrArraySize, groupSizeZ));
}

uint32_t Texture2D_DX12::Blit(const CommandList* pCommandList, ResourceBarrier* pBarriers,
	uint32_t groupSizeX, uint32_t groupSizeY, uint32_t groupSizeZ, uint8_t mipLevel,
	int8_t srcMipLevel, ResourceState srcState, const DescriptorTable& uavSrvTable,
	uint32_t uavSrvSlot, uint32_t numBarriers, const DescriptorTable& srvTable,
	uint32_t srvSlot, uint32_t baseSlice, uint32_t numSlices)
{
	const auto prevBarriers = numBarriers;
	const auto desc = m_resource->GetDesc();
	if (!numSlices) numSlices = desc.DepthOrArraySize - baseSlice;

	if (!mipLevel && srcMipLevel <= mipLevel)
		numBarriers = SetBarrier(pBarriers, ResourceState::UNORDERED_ACCESS, numBarriers);
	else for (auto i = 0u; i < numSlices; ++i)
	{
		const auto j = baseSlice + i;
		const auto subresource = D3D12CalcSubresource(mipLevel, j, 0, desc.MipLevels, desc.DepthOrArraySize);
		if (m_states[subresource] != ResourceState::UNORDERED_ACCESS)
		{
			pBarriers[numBarriers++] = { this, m_states[subresource], ResourceState::UNORDERED_ACCESS, subresource };
			m_states[subresource] = ResourceState::UNORDERED_ACCESS;
		}
		numBarriers = SetBarrier(pBarriers, srcMipLevel, srcState, numBarriers, j);
	}

	if (numBarriers > prevBarriers)
	{
		pCommandList->Barrier(numBarriers, pBarriers);
		numBarriers = 0;
	}

	Blit(pCommandList, groupSizeX, groupSizeY, groupSizeZ, uavSrvTable, uavSrvSlot, mipLevel, srvTable, srvSlot);

	return numBarriers;
}

uint32_t Texture2D_DX12::GenerateMips(const CommandList* pCommandList, ResourceBarrier* pBarriers, uint32_t groupSizeX,
	uint32_t groupSizeY, uint32_t groupSizeZ, ResourceState dstState, const PipelineLayout& pipelineLayout,
	const Pipeline& pipeline, const DescriptorTable* pUavSrvTables, uint32_t uavSrvSlot, const DescriptorTable& samplerTable,
	uint32_t samplerSlot, uint32_t numBarriers, const DescriptorTable* pSrvTables, uint32_t srvSlot, uint8_t baseMip,
	uint8_t numMips, uint32_t baseSlice, uint32_t numSlices)
{
	if (pipelineLayout) pCommandList->SetComputePipelineLayout(pipelineLayout);
	if (samplerTable) pCommandList->SetComputeDescriptorTable(samplerSlot, samplerTable);
	if (pipeline) pCommandList->SetPipelineState(pipeline);

	if (!(numMips || baseMip || numSlices || baseSlice))
		numBarriers = SetBarrier(pBarriers, ResourceState::UNORDERED_ACCESS, numBarriers);

	const auto desc = m_resource->GetDesc();
	if (!numSlices) numSlices = desc.DepthOrArraySize - baseSlice;
	if (!numMips) numMips = desc.MipLevels - baseMip;

	for (uint8_t i = 0u; i < numMips; ++i)
	{
		const auto j = baseMip + i;
		const auto prevBarriers = numBarriers;

		if (j > 0) for (auto k = 0u; k < numSlices; ++k)
		{
			const auto n = baseSlice + k;
			auto subresource = D3D12CalcSubresource(j, n, 0, desc.MipLevels, desc.DepthOrArraySize);
			if (m_states[subresource] != ResourceState::UNORDERED_ACCESS)
			{
				pBarriers[numBarriers++] = { this, m_states[subresource], ResourceState::UNORDERED_ACCESS, subresource };
				m_states[subresource] = ResourceState::UNORDERED_ACCESS;
			}

			subresource = D3D12CalcSubresource(j - 1, n, 0, desc.MipLevels, desc.DepthOrArraySize);
			if ((m_states[subresource] & dstState) != dstState)
				numBarriers = SetBarrier(pBarriers, dstState, numBarriers, subresource);
		}

		pCommandList->Barrier(numBarriers, pBarriers);
		numBarriers = 0;

		Blit(pCommandList, groupSizeX, groupSizeY, groupSizeZ, pUavSrvTables[i], uavSrvSlot, j,
			pSrvTables ? pSrvTables[i] : nullptr, srvSlot);
	}

	const auto m = baseMip + numMips - 1;
	for (auto i = 0u; i < numSlices; ++i)
		numBarriers = SetBarrier(pBarriers, m, dstState, numBarriers, baseSlice + i);

	return numBarriers;
}

const Descriptor& Texture2D_DX12::GetUAV(uint8_t index) const
{
	assert(m_uavs.size() > index);
	return m_uavs[index];
}

const Descriptor& Texture2D_DX12::GetPackedUAV(uint8_t index) const
{
	assert(m_packedUavs.size() > index);
	return m_packedUavs[index];
}

const Descriptor& Texture2D_DX12::GetSRVLevel(uint8_t level) const
{
	assert(m_srvLevels.size() > level);
	return m_srvLevels[level];
}

uint32_t Texture2D_DX12::GetHeight() const
{
	return m_resource->GetDesc().Height;
}

uint32_t Texture2D_DX12::GetArraySize() const
{
	return m_resource->GetDesc().DepthOrArraySize;
}

uint8_t Texture2D_DX12::GetNumMips() const
{
	return static_cast<uint8_t>(m_resource->GetDesc().MipLevels);
}

//--------------------------------------------------------------------------------------
// Render target
//--------------------------------------------------------------------------------------

RenderTarget_DX12::RenderTarget_DX12() :
	Texture2D_DX12(),
	m_rtvHeaps(0),
	m_rtvs(0)
{
}

RenderTarget_DX12::~RenderTarget_DX12()
{
}

bool RenderTarget_DX12::Create(const Device* pDevice, uint32_t width, uint32_t height, Format format,
	uint32_t arraySize, ResourceFlag resourceFlags, uint8_t numMips, uint8_t sampleCount,
	const float* pClearColor, bool isCubeMap, MemoryFlag memoryFlags, const wchar_t* name)
{
	N_RETURN(create(pDevice, width, height, arraySize, format, numMips, sampleCount,
		resourceFlags, pClearColor, isCubeMap, memoryFlags, name), false);

	// Setup the description of the render target view.
	D3D12_RENDER_TARGET_VIEW_DESC desc = {};
	desc.Format = GetDXGIFormat(m_format);

	numMips = max<uint8_t>(numMips, 1);
	m_rtvs.resize(arraySize);
	for (auto i = 0u; i < arraySize; ++i)
	{
		uint8_t mipLevel = 0;
		m_rtvs[i].resize(numMips);

		for (auto& descriptor : m_rtvs[i])
		{
			// Setup the description of the render target view.
			if (arraySize > 1)
			{
				if (sampleCount > 1)
				{
					desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
					desc.Texture2DMSArray.FirstArraySlice = i;
					desc.Texture2DMSArray.ArraySize = 1;
				}
				else
				{
					desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
					desc.Texture2DArray.FirstArraySlice = i;
					desc.Texture2DArray.ArraySize = 1;
					desc.Texture2DArray.MipSlice = mipLevel++;
				}
			}
			else
			{
				if (sampleCount > 1)
					desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
				else
				{
					desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
					desc.Texture2D.MipSlice = mipLevel++;
				}
			}

			// Create a render target view
			X_RETURN(descriptor, allocateRtvHeap(), false);
			m_device->CreateRenderTargetView(m_resource.get(), &desc, { descriptor });
		}
	}

	return true;
}

bool RenderTarget_DX12::CreateArray(const Device* pDevice, uint32_t width, uint32_t height,
	uint32_t arraySize, Format format, ResourceFlag resourceFlags, uint8_t numMips,
	uint8_t sampleCount, const float* pClearColor, bool isCubeMap,
	MemoryFlag memoryFlags, const wchar_t* name)
{
	N_RETURN(create(pDevice, width, height, arraySize, format, numMips, sampleCount,
		resourceFlags, pClearColor, isCubeMap, memoryFlags, name), false);

	// Setup the description of the render target view.
	D3D12_RENDER_TARGET_VIEW_DESC desc = {};
	desc.Format = GetDXGIFormat(m_format);

	m_rtvs.resize(1);
	m_rtvs[0].resize(max<uint8_t>(numMips, 1));

	uint8_t mipLevel = 0;
	for (auto& descriptor : m_rtvs[0])
	{
		// Setup the description of the render target view.
		if (sampleCount > 1)
		{
			desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
			desc.Texture2DMSArray.ArraySize = arraySize;
		}
		else
		{
			desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
			desc.Texture2DArray.ArraySize = arraySize;
			desc.Texture2DArray.MipSlice = mipLevel++;
		}

		// Create a render target view
		X_RETURN(descriptor, allocateRtvHeap(), false);
		m_device->CreateRenderTargetView(m_resource.get(), &desc, { descriptor });
	}

	return true;
}

bool RenderTarget_DX12::CreateFromSwapChain(const Device* pDevice, const SwapChain* pSwapChain, uint32_t bufferIndex)
{
	N_RETURN(setDevice(pDevice), false);
	m_rtvHeaps.clear();

	m_name = L"SwapChain[" + to_wstring(bufferIndex) + L"]";

	// Determine initial state
	m_states.resize(1);
	m_states[0] = ResourceState::PRESENT;

	// Get resource
	N_RETURN(pSwapChain->GetBuffer(bufferIndex, this), false);

	// Create RTV
	m_rtvs.resize(1);
	m_rtvs[0].resize(1);
	m_rtvs[0][0] = allocateRtvHeap();
	N_RETURN(m_rtvs[0][0], false);
	m_device->CreateRenderTargetView(m_resource.get(), nullptr, { m_rtvs[0][0] });

	switch (m_resource->GetDesc().Format)
	{
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
		m_format = Format::R16G16B16A16_FLOAT;
		break;
	case DXGI_FORMAT_B8G8R8A8_UNORM:
		m_format = Format::B8G8R8A8_UNORM;
		break;
	case DXGI_FORMAT_R8G8B8A8_UNORM:
		m_format = Format::R8G8B8A8_UNORM;
		break;
	default:
		assert("Unsupported swapchain format!");
	}

	return true;
}

void RenderTarget_DX12::Blit(const CommandList* pCommandList, const DescriptorTable& srcSrvTable,
	uint32_t srcSlot, uint8_t mipLevel, uint32_t baseSlice, uint32_t numSlices,
	const DescriptorTable& samplerTable, uint32_t samplerSlot, const Pipeline& pipeline,
	uint32_t offsetForSliceId, uint32_t cbSlot)
{
	// Set pipeline layout and descriptor tables
	if (srcSrvTable) pCommandList->SetGraphicsDescriptorTable(srcSlot, srcSrvTable);
	if (samplerTable) pCommandList->SetGraphicsDescriptorTable(samplerSlot, samplerTable);

	// Set pipeline
	if (pipeline) pCommandList->SetPipelineState(pipeline);

	// Set viewport
	const auto desc = m_resource->GetDesc();
	const auto width = (max)(static_cast<uint32_t>(desc.Width >> mipLevel), 1u);
	const auto height = (max)(desc.Height >> mipLevel, 1u);
	const Viewport viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
	const RectRange rect(0, 0, width, height);
	pCommandList->RSSetViewports(1, &viewport);
	pCommandList->RSSetScissorRects(1, &rect);

	// Draw quads
	if (numSlices == 0) numSlices = desc.DepthOrArraySize - baseSlice;
	pCommandList->IASetPrimitiveTopology(PrimitiveTopology::TRIANGLELIST);
	if (numSlices == 1)
	{
		// Set render target
		pCommandList->OMSetRenderTargets(1, &GetRTV(baseSlice, mipLevel));
		pCommandList->Draw(3, 1, 0, 0);
	}
	else for (auto i = 0u; i < numSlices; ++i)
	{
		// Set render target
		pCommandList->OMSetRenderTargets(1, &GetRTV(baseSlice + i, mipLevel));
		pCommandList->SetGraphics32BitConstant(cbSlot, i, offsetForSliceId);
		pCommandList->Draw(3, 1, 0, 0);
	}
}

uint32_t RenderTarget_DX12::Blit(const CommandList* pCommandList, ResourceBarrier* pBarriers, uint8_t mipLevel,
	int8_t srcMipLevel, ResourceState srcState, const DescriptorTable& srcSrvTable, uint32_t srcSlot,
	uint32_t numBarriers, uint32_t baseSlice, uint32_t numSlices,
	uint32_t offsetForSliceId, uint32_t cbSlot)
{
	const auto prevBarriers = numBarriers;
	if (!numSlices) numSlices = m_resource->GetDesc().DepthOrArraySize - baseSlice;

	if (!mipLevel && srcMipLevel <= mipLevel)
		numBarriers = SetBarrier(pBarriers, ResourceState::RENDER_TARGET, numBarriers);
	else for (auto i = 0u; i < numSlices; ++i)
	{
		const auto j = baseSlice + i;
		numBarriers = SetBarrier(pBarriers, mipLevel, ResourceState::RENDER_TARGET, numBarriers, j);
		numBarriers = SetBarrier(pBarriers, srcMipLevel, srcState, numBarriers, j);
	}

	if (numBarriers > prevBarriers)
	{
		pCommandList->Barrier(numBarriers, pBarriers);
		numBarriers = 0;
	}

	Blit(pCommandList, srcSrvTable, srcSlot, mipLevel, baseSlice,
		numSlices, nullptr, 1, nullptr, offsetForSliceId, cbSlot);

	return numBarriers;
}

uint32_t RenderTarget_DX12::GenerateMips(const CommandList* pCommandList, ResourceBarrier* pBarriers,
	ResourceState dstState, const PipelineLayout& pipelineLayout, const Pipeline& pipeline,
	const DescriptorTable* pSrcSrvTables, uint32_t srcSlot, const DescriptorTable& samplerTable,
	uint32_t samplerSlot, uint32_t numBarriers, uint8_t baseMip, uint8_t numMips,
	uint32_t baseSlice, uint32_t numSlices, uint32_t offsetForSliceId, uint32_t cbSlot)
{
	if (pipelineLayout) pCommandList->SetGraphicsPipelineLayout(pipelineLayout);
	if (samplerTable) pCommandList->SetGraphicsDescriptorTable(samplerSlot, samplerTable);
	if (pipeline) pCommandList->SetPipelineState(pipeline);

	if (!(numMips || baseMip || numSlices || baseSlice))
		numBarriers = SetBarrier(pBarriers, ResourceState::RENDER_TARGET, numBarriers);

	const auto& desc = m_resource->GetDesc();
	if (!numSlices) numSlices = desc.DepthOrArraySize - baseSlice;
	if (!numMips) numMips = desc.MipLevels - baseMip;

	for (uint8_t i = 0; i < numMips; ++i)
	{
		const auto j = baseMip + i;
		const auto prevBarriers = numBarriers;

		if (j > 0) for (auto k = 0u; k < numSlices; ++k)
		{
			const auto n = baseSlice + k;
			numBarriers = SetBarrier(pBarriers, j, ResourceState::RENDER_TARGET, numBarriers, n);

			const auto subresource = D3D12CalcSubresource(j - 1, n, 0, desc.MipLevels, desc.DepthOrArraySize);
			if ((m_states[subresource] & dstState) != dstState)
				numBarriers = SetBarrier(pBarriers, dstState, numBarriers, subresource);
		}

		pCommandList->Barrier(numBarriers, pBarriers);
		numBarriers = 0;

		if (numSlices > 1) pCommandList->SetGraphics32BitConstant(cbSlot, 0, offsetForSliceId);
		Blit(pCommandList, pSrcSrvTables[i], srcSlot, j, baseSlice, numSlices,
			nullptr, samplerSlot, nullptr, offsetForSliceId, cbSlot);
	}

	const auto m = baseMip + numMips - 1;
	for (auto i = 0u; i < numSlices; ++i)
		numBarriers = SetBarrier(pBarriers, m, dstState, numBarriers, baseSlice + i);

	return numBarriers;
}

const Descriptor& RenderTarget_DX12::GetRTV(uint32_t slice, uint8_t mipLevel) const
{
	assert(m_rtvs.size() > slice && m_rtvs[slice].size() > mipLevel);
	return m_rtvs[slice][mipLevel];
}

bool RenderTarget_DX12::create(const Device* pDevice, uint32_t width, uint32_t height,
	uint32_t arraySize, Format format, uint8_t numMips, uint8_t sampleCount,
	ResourceFlag resourceFlags, const float* pClearColor, bool isCubeMap,
	MemoryFlag memoryFlags, const wchar_t* name)
{
	N_RETURN(setDevice(pDevice), false);
	m_rtvHeaps.clear();

	if (name) m_name = name;

	const auto needPackedUAV = (resourceFlags & ResourceFlag::NEED_PACKED_UAV) == ResourceFlag::NEED_PACKED_UAV;
	const auto hasSRV = (resourceFlags & ResourceFlag::DENY_SHADER_RESOURCE) == ResourceFlag::NONE;
	const bool hasUAV = (resourceFlags & ResourceFlag::ALLOW_UNORDERED_ACCESS) == ResourceFlag::ALLOW_UNORDERED_ACCESS;

	// Map formats
	m_format = format;
	const auto formatUAV = needPackedUAV ? MapToPackedFormat(format) : format;

	// Setup the texture description.
	const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
	const auto desc = CD3DX12_RESOURCE_DESC::Tex2D(GetDXGIFormat(format),
		width, height, arraySize, numMips, sampleCount, 0,
		GetDX12ResourceFlags(ResourceFlag::ALLOW_RENDER_TARGET | resourceFlags));

	// Determine initial state
	m_states.resize(arraySize * numMips, ResourceState::COMMON);

	// Optimized clear value
	D3D12_CLEAR_VALUE clearValue = { GetDXGIFormat(m_format) };
	if (pClearColor) memcpy(clearValue.Color, pClearColor, sizeof(clearValue.Color));

	// Create the render target texture.
	V_RETURN(m_device->CreateCommittedResource(&heapProperties, GetDX12HeapFlags(memoryFlags), &desc,
		GetDX12ResourceStates(m_states[0]), &clearValue, IID_PPV_ARGS(&m_resource)), clog, false);
	if (!m_name.empty()) m_resource->SetName((m_name + L".Resource").c_str());

	// Create SRV
	if (hasSRV)
	{
		N_RETURN(CreateSRVs(arraySize, m_format, numMips, sampleCount, isCubeMap), false);
		N_RETURN(CreateSRVLevels(arraySize, numMips, m_format, sampleCount, isCubeMap), false);
	}

	// Create UAV
	if (hasUAV)
	{
		N_RETURN(CreateUAVs(arraySize, m_format, numMips), false);
		if (needPackedUAV) N_RETURN(CreateUAVs(arraySize, formatUAV, numMips, &m_packedUavs), false);
	}

	return true;
}

Descriptor RenderTarget_DX12::allocateRtvHeap()
{
	m_rtvHeaps.emplace_back();
	auto& rtvPool = m_rtvHeaps.back();

	D3D12_DESCRIPTOR_HEAP_DESC desc = { D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1 };
	V_RETURN(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&rtvPool)), cerr, 0);
	if (!m_name.empty()) rtvPool->SetName((m_name + L".RtvPool").c_str());

	return rtvPool->GetCPUDescriptorHandleForHeapStart().ptr;
}

//--------------------------------------------------------------------------------------
// Depth stencil
//--------------------------------------------------------------------------------------

DepthStencil_DX12::DepthStencil_DX12() :
	Texture2D_DX12(),
	m_dsvHeaps(),
	m_dsvs(0),
	m_readOnlyDsvs(0),
	m_stencilSrv(0)
{
}

DepthStencil_DX12::~DepthStencil_DX12()
{
}

bool DepthStencil_DX12::Create(const Device* pDevice, uint32_t width, uint32_t height, Format format,
	ResourceFlag resourceFlags, uint32_t arraySize, uint8_t numMips, uint8_t sampleCount,
	float clearDepth, uint8_t clearStencil, bool isCubeMap, MemoryFlag memoryFlags,
	const wchar_t* name)
{
	bool hasSRV;
	Format formatStencil;
	N_RETURN(create(pDevice, width, height, arraySize, numMips, sampleCount, format, resourceFlags,
		clearDepth, clearStencil, hasSRV, formatStencil, isCubeMap, memoryFlags, name), false);

	// Setup the description of the depth stencil view.
	D3D12_DEPTH_STENCIL_VIEW_DESC desc = {};
	desc.Format = GetDXGIFormat(m_format);

	numMips = max<uint8_t>(numMips, 1);
	m_dsvs.resize(arraySize);
	m_readOnlyDsvs.resize(arraySize);

	for (auto i = 0u; i < arraySize; ++i)
	{
		uint8_t mipLevel = 0;
		m_dsvs[i].resize(numMips);
		m_readOnlyDsvs[i].resize(numMips);

		for (uint8_t j = 0; j < numMips; ++j)
		{
			auto& dsv = m_dsvs[i][j];
			auto& readOnlyDsv = m_readOnlyDsvs[i][j];

			// Setup the description of the depth stencil view.
			if (arraySize > 1)
			{
				if (sampleCount > 1)
				{
					desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
					desc.Texture2DMSArray.FirstArraySlice = i;
					desc.Texture2DMSArray.ArraySize = 1;
				}
				else
				{
					desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
					desc.Texture2DArray.FirstArraySlice = i;
					desc.Texture2DArray.ArraySize = 1;
					desc.Texture2DArray.MipSlice = j;
				}
			}
			else
			{
				if (sampleCount > 1)
					desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
				else
				{
					desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
					desc.Texture2D.MipSlice = j;
				}
			}

			// Create a depth stencil view
			dsv = allocateDsvHeap();
			N_RETURN(dsv, false);
			m_device->CreateDepthStencilView(m_resource.get(), &desc, { dsv });

			// Read-only depth stencil
			if (hasSRV)
			{
				// Setup the description of the depth stencil view.
				desc.Flags = formatStencil != Format::UNKNOWN ? D3D12_DSV_FLAG_READ_ONLY_DEPTH |
					D3D12_DSV_FLAG_READ_ONLY_STENCIL : D3D12_DSV_FLAG_READ_ONLY_DEPTH;

				// Create a depth stencil view
				readOnlyDsv = allocateDsvHeap();
				N_RETURN(readOnlyDsv, false);
				m_device->CreateDepthStencilView(m_resource.get(), &desc, { readOnlyDsv });
			}
			else readOnlyDsv = dsv;
		}
	}

	return true;
}

bool DepthStencil_DX12::CreateArray(const Device* pDevice, uint32_t width, uint32_t height, uint32_t arraySize,
	Format format, ResourceFlag resourceFlags, uint8_t numMips, uint8_t sampleCount, float clearDepth,
	uint8_t clearStencil, bool isCubeMap, MemoryFlag memoryFlags, const wchar_t* name)
{
	bool hasSRV;
	Format formatStencil;
	N_RETURN(create(pDevice, width, height, arraySize, numMips, sampleCount, format, resourceFlags,
		clearDepth, clearStencil, hasSRV, formatStencil, isCubeMap, memoryFlags, name), false);

	// Setup the description of the depth stencil view.
	D3D12_DEPTH_STENCIL_VIEW_DESC desc = {};
	desc.Format = GetDXGIFormat(m_format);

	numMips = max<uint8_t>(numMips, 1);
	m_dsvs.resize(1);
	m_dsvs[0].resize(numMips);
	m_readOnlyDsvs.resize(1);
	m_readOnlyDsvs[0].resize(numMips);

	for (uint8_t i = 0; i < numMips; ++i)
	{
		auto& dsv = m_dsvs[0][i];
		auto& readOnlyDsv = m_readOnlyDsvs[0][i];

		// Setup the description of the depth stencil view.
		if (arraySize > 1)
		{
			if (sampleCount > 1)
			{
				desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
				desc.Texture2DMSArray.ArraySize = arraySize;
			}
			else
			{
				desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
				desc.Texture2DArray.ArraySize = arraySize;
				desc.Texture2DArray.MipSlice = i;
			}
		}
		else
		{
			if (sampleCount > 1)
				desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
			else
			{
				desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
				desc.Texture2D.MipSlice = i;
			}
		}

		// Create a depth stencil view
		dsv = allocateDsvHeap();
		N_RETURN(dsv, false);
		m_device->CreateDepthStencilView(m_resource.get(), &desc, { dsv });

		// Read-only depth stencil
		if (hasSRV)
		{
			// Setup the description of the depth stencil view.
			desc.Flags = formatStencil != Format::UNKNOWN ? D3D12_DSV_FLAG_READ_ONLY_DEPTH |
				D3D12_DSV_FLAG_READ_ONLY_STENCIL : D3D12_DSV_FLAG_READ_ONLY_DEPTH;

			// Create a depth stencil view
			readOnlyDsv = allocateDsvHeap();
			N_RETURN(readOnlyDsv, false);
			m_device->CreateDepthStencilView(m_resource.get(), &desc, { readOnlyDsv });
		}
		else readOnlyDsv = dsv;
	}

	return true;
}

const Descriptor& DepthStencil_DX12::GetDSV(uint32_t slice, uint8_t mipLevel) const
{
	assert(m_dsvs.size() > slice && m_dsvs[slice].size() > mipLevel);
	return m_dsvs[slice][mipLevel];
}

const Descriptor& DepthStencil_DX12::GetReadOnlyDSV(uint32_t slice, uint8_t mipLevel) const
{
	assert(m_readOnlyDsvs.size() > slice && m_readOnlyDsvs[slice].size() > mipLevel);
	return m_readOnlyDsvs[slice][mipLevel];
}

const Descriptor& DepthStencil_DX12::GetStencilSRV() const
{
	return m_stencilSrv;
}

uint32_t DepthStencil_DX12::GetArraySize() const
{
	return static_cast<uint32_t>(m_dsvs.size());
}

uint8_t DepthStencil_DX12::GetNumMips() const
{
	return static_cast<uint8_t>(m_dsvs.size());
}

bool DepthStencil_DX12::create(const Device* pDevice, uint32_t width, uint32_t height, uint32_t arraySize,
	uint8_t numMips, uint8_t sampleCount, Format format, ResourceFlag resourceFlags, float clearDepth,
	uint8_t clearStencil, bool& hasSRV, Format& formatStencil, bool isCubeMap, MemoryFlag memoryFlags,
	const wchar_t* name)
{
	N_RETURN(setDevice(pDevice), false);
	m_dsvHeaps.clear();

	if (name) m_name = name;

	hasSRV = (resourceFlags & ResourceFlag::DENY_SHADER_RESOURCE) == ResourceFlag::NONE;

	// Map formats
	m_format = format;
	auto formatDepth = Format::UNKNOWN;
	formatStencil = Format::UNKNOWN;

	if (hasSRV)
	{
		switch (format)
		{
		case Format::D24_UNORM_S8_UINT:
			format = Format::R24G8_TYPELESS;
			formatDepth = Format::R24_UNORM_X8_TYPELESS;
			formatStencil = Format::X24_TYPELESS_G8_UINT;
			break;
		case Format::D32_FLOAT_S8X24_UINT:
			format = Format::R32G8X24_TYPELESS;
			formatDepth = Format::R32_FLOAT_X8X24_TYPELESS;
			formatStencil = Format::X32_TYPELESS_G8X24_UINT;
			break;
		case Format::D16_UNORM:
			format = Format::R16_TYPELESS;
			formatDepth = Format::R16_UNORM;
			break;
		case Format::D32_FLOAT:
			format = Format::R32_TYPELESS;
			formatDepth = Format::R32_FLOAT;
			break;
		default:
			m_format = Format::D24_UNORM_S8_UINT;
			format = Format::R24G8_TYPELESS;
			formatDepth = Format::R24_UNORM_X8_TYPELESS;
		}
	}

	// Setup the render depth stencil description.
	{
		const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
		const auto desc = CD3DX12_RESOURCE_DESC::Tex2D(GetDXGIFormat(format),
			width, height, arraySize, numMips, sampleCount, 0,
			GetDX12ResourceFlags(ResourceFlag::ALLOW_DEPTH_STENCIL | resourceFlags));

		// Determine initial state
		m_states.resize(arraySize * numMips, ResourceState::DEPTH_WRITE);

		// Optimized clear value
		D3D12_CLEAR_VALUE clearValue = { GetDXGIFormat(m_format) };
		clearValue.DepthStencil.Depth = clearDepth;
		clearValue.DepthStencil.Stencil = clearStencil;

		// Create the depth stencil texture.
		V_RETURN(m_device->CreateCommittedResource(&heapProperties, GetDX12HeapFlags(memoryFlags), &desc,
			GetDX12ResourceStates(m_states[0]), &clearValue, IID_PPV_ARGS(&m_resource)), clog, false);
		if (!m_name.empty()) m_resource->SetName((m_name + L".Resource").c_str());
	}

	if (hasSRV)
	{
		// Create SRV
		if (hasSRV) N_RETURN(CreateSRVs(arraySize, formatDepth, numMips, sampleCount, isCubeMap), false);

		// Has stencil
		if (formatStencil != Format::UNKNOWN)
		{
			// Setup the description of the shader resource view.
			D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
			desc.Format = GetDXGIFormat(formatStencil);
			desc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(1, 4, 4, 4);

			if (isCubeMap)
			{
				assert(arraySize % 6 == 0);
				if (arraySize > 6)
				{
					desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
					desc.TextureCubeArray.MipLevels = numMips;
					desc.TextureCubeArray.NumCubes = arraySize / 6;
				}
				else
				{
					desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
					desc.TextureCube.MipLevels = numMips;
				}
			}
			else if (arraySize > 1)
			{
				if (sampleCount > 1)
				{
					desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
					desc.Texture2DMSArray.ArraySize = arraySize;
				}
				else
				{
					desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
					desc.Texture2DArray.ArraySize = arraySize;
					desc.Texture2DArray.MipLevels = numMips;
					desc.Texture2DArray.PlaneSlice = 1;
				}
			}
			else
			{
				if (sampleCount > 1)
					desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
				else
				{
					desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
					desc.Texture2D.MipLevels = numMips;
					desc.Texture2D.PlaneSlice = 1;
				}
			}

			// Create a shader resource view
			m_stencilSrv = allocateSrvUavHeap();
			N_RETURN(m_stencilSrv, false);
			m_device->CreateShaderResourceView(m_resource.get(), &desc, { m_stencilSrv });
		}
	}

	return true;
}

Descriptor DepthStencil_DX12::allocateDsvHeap()
{
	m_dsvHeaps.emplace_back();
	auto& dsvPool = m_dsvHeaps.back();

	D3D12_DESCRIPTOR_HEAP_DESC desc = { D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1 };
	V_RETURN(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&dsvPool)), cerr, 0);
	if (!m_name.empty()) dsvPool->SetName((m_name + L".DsvPool").c_str());

	return dsvPool->GetCPUDescriptorHandleForHeapStart().ptr;
}

//--------------------------------------------------------------------------------------
// 3D Texture
//--------------------------------------------------------------------------------------

Texture3D_DX12::Texture3D_DX12() :
	Texture2D_DX12()
{
}

Texture3D_DX12::~Texture3D_DX12()
{
}

bool Texture3D_DX12::Create(const Device* pDevice, uint32_t width, uint32_t height,
	uint32_t depth, Format format, ResourceFlag resourceFlags, uint8_t numMips,
	MemoryFlag memoryFlags, const wchar_t* name)
{
	N_RETURN(setDevice(pDevice), false);

	if (name) m_name = name;

	const auto needPackedUAV = (resourceFlags & ResourceFlag::NEED_PACKED_UAV) == ResourceFlag::NEED_PACKED_UAV;
	const auto hasSRV = (resourceFlags & ResourceFlag::DENY_SHADER_RESOURCE) == ResourceFlag::NONE;
	const bool hasUAV = (resourceFlags & ResourceFlag::ALLOW_UNORDERED_ACCESS) == ResourceFlag::ALLOW_UNORDERED_ACCESS;

	// Map formats
	m_format = format;
	const auto formatUAV = needPackedUAV ? MapToPackedFormat(format) : format;

	// Setup the texture description.
	const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
	const auto desc = CD3DX12_RESOURCE_DESC::Tex3D(GetDXGIFormat(format),
		width, height, depth, numMips, GetDX12ResourceFlags(resourceFlags));

	// Determine initial state
	m_states.resize(numMips, ResourceState::COMMON);

	V_RETURN(m_device->CreateCommittedResource(&heapProperties, GetDX12HeapFlags(memoryFlags), &desc,
		GetDX12ResourceStates(m_states[0]), nullptr, IID_PPV_ARGS(&m_resource)), clog, false);
	if (!m_name.empty()) m_resource->SetName((m_name + L".Resource").c_str());

	// Create SRV
	if (hasSRV) N_RETURN(CreateSRVs(m_format, numMips), false);

	// Create UAV
	if (hasUAV)
	{
		N_RETURN(CreateUAVs(m_format, numMips), false);
		if (needPackedUAV) N_RETURN(CreateUAVs(formatUAV, numMips, &m_packedUavs), false);
	}

	// Create SRV for each level
	if (hasSRV && hasUAV) N_RETURN(CreateSRVLevels(numMips, m_format), false);

	return true;
}

bool Texture3D_DX12::CreateSRVs(Format format, uint8_t numMips)
{
	// Setup the description of the shader resource view.
	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	desc.Format = format != Format::UNKNOWN ? GetDXGIFormat(format) : m_resource->GetDesc().Format;
	desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;

	uint8_t mipLevel = 0;
	m_srvs.resize(max<uint8_t>(numMips, 1));

	for (auto& descriptor : m_srvs)
	{
		// Setup the description of the shader resource view.
		desc.Texture3D.MipLevels = numMips - mipLevel;
		desc.Texture3D.MostDetailedMip = mipLevel++;

		// Create a shader resource view
		X_RETURN(descriptor, allocateSrvUavHeap(), false);
		m_device->CreateShaderResourceView(m_resource.get(), &desc, { descriptor });
	}

	return true;
}

bool Texture3D_DX12::CreateSRVLevels(uint8_t numMips, Format format)
{
	if (numMips > 1)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
		desc.Format = format != Format::UNKNOWN ? GetDXGIFormat(format) : m_resource->GetDesc().Format;
		desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;

		uint8_t mipLevel = 0;
		m_srvLevels.resize(numMips);

		for (auto& descriptor : m_srvLevels)
		{
			// Setup the description of the shader resource view.
			desc.Texture3D.MostDetailedMip = mipLevel++;
			desc.Texture3D.MipLevels = 1;

			// Create a shader resource view
			X_RETURN(descriptor, allocateSrvUavHeap(), false);
			m_device->CreateShaderResourceView(m_resource.get(), &desc, { descriptor });
		}
	}

	return true;
}

bool Texture3D_DX12::CreateUAVs(Format format, uint8_t numMips, vector<Descriptor>* pUavs)
{
	const auto txDesc = m_resource->GetDesc();

	// Setup the description of the unordered access view.
	D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
	desc.Format = format != Format::UNKNOWN ? GetDXGIFormat(format) : txDesc.Format;
	desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;

	uint8_t mipLevel = 0;
	pUavs = pUavs ? pUavs : &m_uavs;
	pUavs->resize(numMips);

	for (auto& descriptor : *pUavs)
	{
		// Setup the description of the unordered access view.
		desc.Texture3D.WSize = txDesc.DepthOrArraySize >> mipLevel;
		desc.Texture3D.MipSlice = mipLevel++;

		// Create an unordered access view
		X_RETURN(descriptor, allocateSrvUavHeap(), false);
		m_device->CreateUnorderedAccessView(m_resource.get(), nullptr, &desc, { descriptor });
	}

	return true;
}

uint32_t Texture3D_DX12::GetDepth() const
{
	return m_resource->GetDesc().DepthOrArraySize;
}

//--------------------------------------------------------------------------------------
// Raw buffer
//--------------------------------------------------------------------------------------

RawBuffer_DX12::RawBuffer_DX12() :
	ShaderResource_DX12(),
	m_uavs(0),
	m_srvOffsets(0),
	m_pDataBegin(nullptr)
{
}

RawBuffer_DX12::~RawBuffer_DX12()
{
	if (m_resource) Unmap();
}

bool RawBuffer_DX12::Create(const Device* pDevice, size_t byteWidth, ResourceFlag resourceFlags,
	MemoryType memoryType, uint32_t numSRVs, const uint32_t* firstSRVElements,
	uint32_t numUAVs, const uint32_t* firstUAVElements, MemoryFlag memoryFlags,
	const wchar_t* name)
{
	const auto hasSRV = (resourceFlags & ResourceFlag::DENY_SHADER_RESOURCE) == ResourceFlag::NONE;
	const bool hasUAV = (resourceFlags & ResourceFlag::ALLOW_UNORDERED_ACCESS) == ResourceFlag::ALLOW_UNORDERED_ACCESS;

	numSRVs = hasSRV ? numSRVs : 0;
	numUAVs = hasUAV ? numUAVs : 0;

	// Create buffer
	N_RETURN(create(pDevice, byteWidth, resourceFlags, memoryType, numSRVs, numUAVs, memoryFlags, name), false);

	// Create SRV
	if (numSRVs > 0) N_RETURN(CreateSRVs(byteWidth, firstSRVElements, numSRVs), false);

	// Create UAV
	if (numUAVs > 0) N_RETURN(CreateUAVs(byteWidth, firstUAVElements, numUAVs), false);

	return true;
}

bool RawBuffer_DX12::Upload(CommandList* pCommandList, Resource* pUploader, const void* pData,
	size_t size, uint32_t descriptorIndex, ResourceState dstState)
{
	const auto offset = m_srvOffsets.empty() ? 0 : m_srvOffsets[descriptorIndex];
	D3D12_SUBRESOURCE_DATA subresourceData;
	subresourceData.pData = pData;
	subresourceData.RowPitch = static_cast<uint32_t>(size);
	subresourceData.SlicePitch = static_cast<uint32_t>(m_resource->GetDesc().Width);

	// Create the GPU upload buffer.
	assert(pUploader);
	auto& uploaderResource = dynamic_cast<Resource_DX12*>(pUploader)->GetResource();
	if (!uploaderResource)
	{
		const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
		const auto desc = CD3DX12_RESOURCE_DESC::Buffer(offset + size);

		V_RETURN(m_device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploaderResource)), clog, false);
		if (!m_name.empty()) uploaderResource->SetName((m_name + L".UploaderResource").c_str());
	}

	// Copy data to the intermediate upload heap and then schedule a copy 
	// from the upload heap to the buffer.
	ResourceBarrier barrier;
	auto numBarriers = SetBarrier(&barrier, ResourceState::COPY_DEST);
	if (m_states[0] != ResourceState::COMMON) pCommandList->Barrier(numBarriers, &barrier);

	const auto pGraphicsCommandList = static_cast<ID3D12GraphicsCommandList*>(pCommandList->GetHandle());
	M_RETURN(UpdateSubresources(pGraphicsCommandList, m_resource.get(), uploaderResource.get(), offset, 0, 1, &subresourceData) <= 0,
		clog, "Failed to upload the resource.", false);

	numBarriers = SetBarrier(&barrier, dstState);
	if (dstState != ResourceState::COMMON) pCommandList->Barrier(numBarriers, &barrier);

	return true;
}

bool RawBuffer_DX12::CreateSRVs(size_t byteWidth, const uint32_t* firstElements,
	uint32_t numDescriptors)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	if (m_states[0] == ResourceState::RAYTRACING_ACCELERATION_STRUCTURE)
	{
		m_srvOffsets.resize(1);
		m_srvs.resize(1);

		m_srvOffsets[0] = firstElements ? firstElements[0] : 0;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		//desc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
		//desc.RaytracingAccelerationStructure.Location = m_resource->GetGPUVirtualAddress();
		//desc.RaytracingAccelerationStructure.Location += m_srvOffsets[0];
		// To compatible with the old d3d12.h in the compile time
		desc.ViewDimension = D3D12_SRV_DIMENSION(11);
		desc.Buffer.FirstElement = m_resource->GetGPUVirtualAddress();
		desc.Buffer.FirstElement += m_srvOffsets[0];

		// Create a shader resource view
		m_srvs[0] = allocateSrvUavHeap();
		N_RETURN(m_srvs[0], false);
		m_device->CreateShaderResourceView(nullptr, &desc, { m_srvs[0] });
	}
	else
	{
		desc.Format = DXGI_FORMAT_R32_TYPELESS;
		desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;

		const auto stride = sizeof(uint32_t);
		const auto numElements = static_cast<uint32_t>(byteWidth / stride);

		m_srvOffsets.resize(numDescriptors);
		m_srvs.resize(numDescriptors);
		for (auto i = 0u; i < numDescriptors; ++i)
		{
			const auto firstElement = firstElements ? firstElements[i] : 0;
			desc.Buffer.FirstElement = firstElement;
			desc.Buffer.NumElements = (!firstElements || i + 1 >= numDescriptors ?
				numElements : firstElements[i + 1]) - firstElement;

			m_srvOffsets[i] = stride * firstElement;

			// Create a shader resource view
			m_srvs[i] = allocateSrvUavHeap();
			N_RETURN(m_srvs[i], false);
			m_device->CreateShaderResourceView(m_resource.get(), &desc, { m_srvs[i] });
		}
	}

	return true;
}

bool RawBuffer_DX12::CreateUAVs(size_t byteWidth, const uint32_t* firstElements,
	uint32_t numDescriptors)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
	desc.Format = DXGI_FORMAT_R32_TYPELESS;
	desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;

	const uint32_t numElements = static_cast<uint32_t>(byteWidth / sizeof(uint32_t));

	m_uavs.resize(numDescriptors);
	for (auto i = 0u; i < numDescriptors; ++i)
	{
		const auto firstElement = firstElements ? firstElements[i] : 0;
		desc.Buffer.FirstElement = firstElement;
		desc.Buffer.NumElements = (!firstElements || i + 1 >= numDescriptors ?
			numElements : firstElements[i + 1]) - firstElement;

		// Create an unordered access view
		m_uavs[i] = allocateSrvUavHeap();
		N_RETURN(m_uavs[i], false);
		m_device->CreateUnorderedAccessView(m_resource.get(), nullptr, &desc, { m_uavs[i] });
	}

	return true;
}

const Descriptor& RawBuffer_DX12::GetUAV(uint32_t index) const
{
	assert(m_uavs.size() > index);
	return m_uavs[index];
}

void* RawBuffer_DX12::Map(uint32_t descriptorIndex, uintptr_t readBegin, uintptr_t readEnd)
{
	const Range range(readBegin, readEnd);

	return Map(&range, descriptorIndex);
}

void* RawBuffer_DX12::Map(const Range* pReadRange, uint32_t descriptorIndex)
{
	// Map and initialize the buffer.
	if (m_pDataBegin == nullptr)
	{
		D3D12_RANGE range;
		if (pReadRange)
		{
			range.Begin = pReadRange->Begin;
			range.End = pReadRange->End;
		}

		V_RETURN(m_resource->Map(0, pReadRange ? &range : nullptr, &m_pDataBegin), cerr, nullptr);
	}

	const auto offset = !descriptorIndex ? 0 : m_srvOffsets[descriptorIndex];

	return &reinterpret_cast<uint8_t*>(m_pDataBegin)[offset];
}

void RawBuffer_DX12::Unmap()
{
	if (m_pDataBegin)
	{
		m_resource->Unmap(0, nullptr);
		m_pDataBegin = nullptr;
	}
}

bool RawBuffer_DX12::create(const Device* pDevice, size_t byteWidth, ResourceFlag resourceFlags,
	MemoryType memoryType, uint32_t numSRVs, uint32_t numUAVs, MemoryFlag memoryFlags,
	const wchar_t* name)
{
	N_RETURN(setDevice(pDevice), false);

	if (name) m_name = name;

	const auto isRaytracingAS = (resourceFlags & ResourceFlag::ACCELERATION_STRUCTURE) == ResourceFlag::ACCELERATION_STRUCTURE;

	// Setup the buffer description.
	const CD3DX12_HEAP_PROPERTIES heapProperties(GetDX12HeapType(memoryType));
	const auto desc = CD3DX12_RESOURCE_DESC::Buffer(byteWidth, GetDX12ResourceFlags(resourceFlags));

	// Determine initial state
	m_states.resize(1);
	switch (memoryType)
	{
	case MemoryType::UPLOAD:
		for (auto& state : m_states)
			state = ResourceState::GENERAL_READ;
		break;
	case MemoryType::READBACK:
		for (auto& state : m_states)
			state = ResourceState::COPY_DEST;
		break;
	default:
		if (isRaytracingAS) for (auto& state : m_states)
			state = ResourceState::RAYTRACING_ACCELERATION_STRUCTURE;
		else for (auto& state : m_states)
			state = ResourceState::COMMON;
	}

	V_RETURN(m_device->CreateCommittedResource(&heapProperties, GetDX12HeapFlags(memoryFlags), &desc,
		GetDX12ResourceStates(m_states[0]), nullptr, IID_PPV_ARGS(&m_resource)), clog, false);
	if (!m_name.empty()) m_resource->SetName((m_name + L".Resource").c_str());

	return true;
}

//--------------------------------------------------------------------------------------
// Structured buffer
//--------------------------------------------------------------------------------------

StructuredBuffer_DX12::StructuredBuffer_DX12() :
	RawBuffer_DX12()
{
}

StructuredBuffer_DX12::~StructuredBuffer_DX12()
{
}

bool StructuredBuffer_DX12::Create(const Device* pDevice, uint32_t numElements, uint32_t stride,
	ResourceFlag resourceFlags, MemoryType memoryType, uint32_t numSRVs, const uint32_t* firstSRVElements,
	uint32_t numUAVs, const uint32_t* firstUAVElements, MemoryFlag memoryFlags, const wchar_t* name,
	const size_t* counterOffsetsInBytes)
{
	const auto hasSRV = (resourceFlags & ResourceFlag::DENY_SHADER_RESOURCE) == ResourceFlag::NONE;
	const bool hasUAV = (resourceFlags & ResourceFlag::ALLOW_UNORDERED_ACCESS) == ResourceFlag::ALLOW_UNORDERED_ACCESS;

	numSRVs = hasSRV ? numSRVs : 0;
	numUAVs = hasUAV ? numUAVs : 0;

	// Create buffer
	N_RETURN(create(pDevice, static_cast<size_t>(stride) * numElements, resourceFlags,
		memoryType, numSRVs, numUAVs, memoryFlags, name), false);

	// Create SRV
	if (numSRVs > 0) N_RETURN(CreateSRVs(numElements, stride, firstSRVElements, numSRVs), false);

	// Create UAV
	if (numUAVs > 0) N_RETURN(CreateUAVs(numElements, stride, firstUAVElements, numUAVs, counterOffsetsInBytes), false);

	return true;
}

bool StructuredBuffer_DX12::CreateSRVs(uint32_t numElements, uint32_t stride,
	const uint32_t* firstElements, uint32_t numDescriptors)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	desc.Format = m_resource->GetDesc().Format;
	desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	desc.Buffer.StructureByteStride = stride;

	m_srvOffsets.resize(numDescriptors);
	m_srvs.resize(numDescriptors);
	for (auto i = 0u; i < numDescriptors; ++i)
	{
		const auto firstElement = firstElements ? firstElements[i] : 0;
		desc.Buffer.FirstElement = firstElement;
		desc.Buffer.NumElements = (!firstElements || i + 1 >= numDescriptors ?
			numElements : firstElements[i + 1]) - firstElement;

		m_srvOffsets[i] = stride * firstElement;

		// Create a shader resource view
		m_srvs[i] = allocateSrvUavHeap();
		N_RETURN(m_srvs[i], false);
		m_device->CreateShaderResourceView(m_resource.get(), &desc, { m_srvs[i] });
	}

	return true;
}

bool StructuredBuffer_DX12::CreateUAVs(uint32_t numElements, uint32_t stride,
	const uint32_t* firstElements, uint32_t numDescriptors,
	const size_t* counterOffsetsInBytes)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
	desc.Format = m_resource->GetDesc().Format;
	desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	desc.Buffer.StructureByteStride = stride;

	m_uavs.resize(numDescriptors);
	for (auto i = 0u; i < numDescriptors; ++i)
	{
		const auto firstElement = firstElements ? firstElements[i] : 0;
		desc.Buffer.FirstElement = firstElement;
		desc.Buffer.NumElements = (!firstElements || i + 1 >= numDescriptors ?
			numElements : firstElements[i + 1]) - firstElement;
		desc.Buffer.CounterOffsetInBytes = counterOffsetsInBytes ? counterOffsetsInBytes[i] : 0;

		// Create an unordered access view
		m_uavs[i] = allocateSrvUavHeap();
		N_RETURN(m_uavs[i], false);
		const auto counterResource = m_counter ? static_cast<ID3D12Resource*>(m_counter->GetHandle()) : nullptr;
		m_device->CreateUnorderedAccessView(m_resource.get(), counterResource, &desc, { m_uavs[i] });
	}

	return true;
}

void StructuredBuffer_DX12::SetCounter(const Resource::sptr& counter)
{
	m_counter = counter;
}

Resource::sptr StructuredBuffer_DX12::GetCounter() const
{
	return m_counter;
}

//--------------------------------------------------------------------------------------
// Typed buffer
//--------------------------------------------------------------------------------------

TypedBuffer_DX12::TypedBuffer_DX12() :
	RawBuffer_DX12()
{
}

TypedBuffer_DX12::~TypedBuffer_DX12()
{
}

bool TypedBuffer_DX12::Create(const Device* pDevice, uint32_t numElements, uint32_t stride,
	Format format, ResourceFlag resourceFlags, MemoryType memoryType, uint32_t numSRVs,
	const uint32_t* firstSRVElements, uint32_t numUAVs, const uint32_t* firstUAVElements,
	MemoryFlag memoryFlags, const wchar_t* name)
{
	const auto needPackedUAV = (resourceFlags & ResourceFlag::NEED_PACKED_UAV) == ResourceFlag::NEED_PACKED_UAV;

	const auto hasSRV = (resourceFlags & ResourceFlag::DENY_SHADER_RESOURCE) == ResourceFlag::NONE;
	const bool hasUAV = (resourceFlags & ResourceFlag::ALLOW_UNORDERED_ACCESS) == ResourceFlag::ALLOW_UNORDERED_ACCESS;

	numSRVs = hasSRV ? numSRVs : 0;
	numUAVs = hasUAV ? numUAVs : 0;

	// Map formats
	m_format = format;
	const auto formatUAV = needPackedUAV ? MapToPackedFormat(format) : format;

	// Create buffer
	N_RETURN(create(pDevice, static_cast<size_t>(stride) * numElements, resourceFlags,
		memoryType, numSRVs, numUAVs, memoryFlags, name), false);

	// Create SRV
	if (numSRVs > 0) N_RETURN(CreateSRVs(numElements, m_format, stride, firstSRVElements, numSRVs), false);

	// Create UAV
	if (numUAVs > 0)
	{
		N_RETURN(CreateUAVs(numElements, m_format, stride, firstUAVElements, numUAVs), false);
		if (needPackedUAV) N_RETURN(CreateUAVs(numElements, formatUAV, stride, firstUAVElements, numUAVs, &m_packedUavs), false);
	}

	return true;
}

bool TypedBuffer_DX12::CreateSRVs(uint32_t numElements, Format format, uint32_t stride,
	const uint32_t* firstElements, uint32_t numDescriptors)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	desc.Format = format != Format::UNKNOWN ? GetDXGIFormat(format) : m_resource->GetDesc().Format;
	desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;

	m_srvOffsets.resize(numDescriptors);
	m_srvs.resize(numDescriptors);
	for (auto i = 0u; i < numDescriptors; ++i)
	{
		const auto firstElement = firstElements ? firstElements[i] : 0;
		desc.Buffer.FirstElement = firstElement;
		desc.Buffer.NumElements = (!firstElements || i + 1 >= numDescriptors ?
			numElements : firstElements[i + 1]) - firstElement;

		m_srvOffsets[i] = stride * firstElement;

		// Create a shader resource view
		m_srvs[i] = allocateSrvUavHeap();
		N_RETURN(m_srvs[i], false);
		m_device->CreateShaderResourceView(m_resource.get(), &desc, { m_srvs[i] });
	}

	return true;
}

bool TypedBuffer_DX12::CreateUAVs(uint32_t numElements, Format format, uint32_t stride,
	const uint32_t* firstElements, uint32_t numDescriptors, vector<Descriptor>* pUavs)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
	desc.Format = format != Format::UNKNOWN ? GetDXGIFormat(format) : m_resource->GetDesc().Format;
	desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;

	pUavs = pUavs ? pUavs : &m_uavs;
	pUavs->resize(numDescriptors);
	for (auto i = 0u; i < numDescriptors; ++i)
	{
		const auto firstElement = firstElements ? firstElements[i] : 0;
		desc.Buffer.FirstElement = firstElement;
		desc.Buffer.NumElements = (!firstElements || i + 1 >= numDescriptors ?
			numElements : firstElements[i + 1]) - firstElement;

		// Create an unordered access view
		pUavs->at(i) = allocateSrvUavHeap();
		N_RETURN(pUavs->at(i), false);
		m_device->CreateUnorderedAccessView(m_resource.get(), nullptr, &desc, { pUavs->at(i) });
	}

	return true;
}

const Descriptor& TypedBuffer_DX12::GetPackedUAV(uint32_t index) const
{
	assert(m_packedUavs.size() > index);
	return m_packedUavs[index];
}

//--------------------------------------------------------------------------------------
// Vertex buffer
//--------------------------------------------------------------------------------------

VertexBuffer_DX12::VertexBuffer_DX12() :
	StructuredBuffer_DX12(),
	m_vbvs(0)
{
}

VertexBuffer_DX12::~VertexBuffer_DX12()
{
}

bool VertexBuffer_DX12::Create(const Device* pDevice, uint32_t numVertices, uint32_t stride,
	ResourceFlag resourceFlags, MemoryType memoryType, uint32_t numVBVs,
	const uint32_t* firstVertices, uint32_t numSRVs, const uint32_t* firstSRVElements,
	uint32_t numUAVs, const uint32_t* firstUAVElements, MemoryFlag memoryFlags,
	const wchar_t* name)
{
	const auto hasSRV = (resourceFlags & ResourceFlag::DENY_SHADER_RESOURCE) == ResourceFlag::NONE;
	const bool hasUAV = (resourceFlags & ResourceFlag::ALLOW_UNORDERED_ACCESS) == ResourceFlag::ALLOW_UNORDERED_ACCESS;

	N_RETURN(StructuredBuffer_DX12::Create(pDevice, numVertices, stride, resourceFlags, memoryType,
		numSRVs, firstSRVElements, numUAVs, firstUAVElements, memoryFlags, name), false);

	// Create vertex buffer view
	m_vbvs.resize(numVBVs);
	for (auto i = 0u; i < numVBVs; ++i)
	{
		const auto firstVertex = firstVertices ? firstVertices[i] : 0;
		m_vbvs[i].BufferLocation = m_resource->GetGPUVirtualAddress() + stride * firstVertex;
		m_vbvs[i].StrideInBytes = stride;
		m_vbvs[i].SizeInBytes = stride * ((!firstVertices || i + 1 >= numVBVs ?
			numVertices : firstVertices[i + 1]) - firstVertex);
	}

	return true;
}

bool VertexBuffer_DX12::CreateAsRaw(const Device* pDevice, uint32_t numVertices, uint32_t stride,
	ResourceFlag resourceFlags, MemoryType memoryType, uint32_t numVBVs,
	const uint32_t* firstVertices, uint32_t numSRVs, const uint32_t* firstSRVElements,
	uint32_t numUAVs, const uint32_t* firstUAVElements, MemoryFlag memoryFlags,
	const wchar_t* name)
{
	const auto hasSRV = (resourceFlags & ResourceFlag::DENY_SHADER_RESOURCE) == ResourceFlag::NONE;
	const bool hasUAV = (resourceFlags & ResourceFlag::ALLOW_UNORDERED_ACCESS) == ResourceFlag::ALLOW_UNORDERED_ACCESS;

	N_RETURN(RawBuffer_DX12::Create(pDevice, static_cast<size_t>(stride) * numVertices, resourceFlags,
		memoryType, numSRVs, firstSRVElements, numUAVs, firstUAVElements, memoryFlags, name), false);

	// Create vertex buffer view
	m_vbvs.resize(numVBVs);
	for (auto i = 0u; i < numVBVs; ++i)
	{
		const auto firstVertex = firstVertices ? firstVertices[i] : 0;
		m_vbvs[i].BufferLocation = m_resource->GetGPUVirtualAddress() + stride * firstVertex;
		m_vbvs[i].StrideInBytes = stride;
		m_vbvs[i].SizeInBytes = stride * ((!firstVertices || i + 1 >= numVBVs ?
			numVertices : firstVertices[i + 1]) - firstVertex);
	}

	return true;
}

const VertexBufferView& VertexBuffer_DX12::GetVBV(uint32_t index) const
{
	assert(m_vbvs.size() > index);
	return m_vbvs[index];
}

//--------------------------------------------------------------------------------------
// Index buffer
//--------------------------------------------------------------------------------------

IndexBuffer_DX12::IndexBuffer_DX12() :
	TypedBuffer_DX12(),
	m_ibvs(0)
{
}

IndexBuffer_DX12::~IndexBuffer_DX12()
{
}

bool IndexBuffer_DX12::Create(const Device* pDevice, size_t byteWidth, Format format,
	ResourceFlag resourceFlags, MemoryType memoryType, uint32_t numIBVs,
	const size_t* offsets, uint32_t numSRVs, const uint32_t* firstSRVElements,
	uint32_t numUAVs, const uint32_t* firstUAVElements, MemoryFlag memoryFlags,
	const wchar_t* name)
{
	const auto hasSRV = (resourceFlags & ResourceFlag::DENY_SHADER_RESOURCE) == ResourceFlag::NONE;
	const bool hasUAV = (resourceFlags & ResourceFlag::ALLOW_UNORDERED_ACCESS) == ResourceFlag::ALLOW_UNORDERED_ACCESS;

	assert(format == Format::R32_UINT || format == Format::R16_UINT);
	const uint32_t stride = format == Format::R32_UINT ? sizeof(uint32_t) : sizeof(uint16_t);

	const auto numElements = static_cast<uint32_t>(byteWidth / stride);
	N_RETURN(TypedBuffer_DX12::Create(pDevice, numElements, stride, format, resourceFlags, memoryType,
		numSRVs, firstSRVElements, numUAVs, firstUAVElements, memoryFlags, name), false);

	// Create index buffer view
	m_ibvs.resize(numIBVs);
	for (auto i = 0u; i < numIBVs; ++i)
	{
		const auto offset = offsets ? offsets[i] : 0;
		m_ibvs[i].BufferLocation = m_resource->GetGPUVirtualAddress() + offset;
		m_ibvs[i].SizeInBytes = static_cast<uint32_t>((!offsets || i + 1 >= numIBVs ?
			byteWidth : offsets[i + 1]) - offset);
		m_ibvs[i].Format = format;
	}

	return true;
}

const IndexBufferView& IndexBuffer_DX12::GetIBV(uint32_t index) const
{
	assert(m_ibvs.size() > index);
	return m_ibvs[index];
}
