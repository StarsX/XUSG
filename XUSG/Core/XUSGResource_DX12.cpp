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

#define DECODE_SRV_COMPONENT_MAPPING(i, mapping) static_cast<SrvComponentMapping>((mapping) >> (3 * i) & 0x7)

//--------------------------------------------------------------------------------------
// Resource base
//--------------------------------------------------------------------------------------

Resource_DX12::Resource_DX12() :
	m_device(nullptr),
	m_resource(nullptr),
	m_states(0),
	m_cbvSrvUavHeap(nullptr),
	m_hasPromotion(false)
{
}

Resource_DX12::~Resource_DX12()
{
}

bool Resource_DX12::Initialize(const Device* pDevice)
{
	m_device = pDevice->GetHandle();
	XUSG_M_RETURN(!m_device, cerr, "The device is NULL.", false);

	m_resource = nullptr;
	m_states.clear();
	m_cbvSrvUavHeap = nullptr;
	m_hasPromotion = false;

	return true;
}

Descriptor Resource_DX12::AllocateCbvSrvUavHeap(uint32_t numDescriptors)
{
	const D3D12_DESCRIPTOR_HEAP_DESC desc = { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, numDescriptors };
	V_RETURN(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_cbvSrvUavHeap)), cerr, false);
	if (!m_name.empty()) m_cbvSrvUavHeap->SetName((m_name + L".CbvSrvUavHeap").c_str());

	return m_cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart().ptr;
}

uint32_t Resource_DX12::SetBarrier(ResourceBarrier* pBarriers, ResourceState dstState, uint32_t numBarriers,
	uint32_t subresource, BarrierFlag flags, ResourceState srcState, uint32_t threadIdx)
{
	assert(!m_resource || !m_states.empty());
	if (!m_states.empty())
	{
		const auto& state = m_states[threadIdx][subresource == XUSG_BARRIER_ALL_SUBRESOURCES ? 0 : subresource];
		if ((state & dstState) != dstState || dstState == ResourceState::UNORDERED_ACCESS || dstState == ResourceState::COMMON)
		{
			srcState = Transition(dstState, subresource, flags, srcState, threadIdx);

			static const auto depthOpState = ResourceState::DEPTH_READ | ResourceState::DEPTH_WRITE;
			static const auto promotionState = ResourceState::ALL_SHADER_RESOURCE | ResourceState::COPY_SOURCE | ResourceState::COPY_DEST;
			const auto isPromoted = srcState == ResourceState::COMMON && (dstState & depthOpState) == ResourceState::COMMON &&
				(m_hasPromotion || (dstState & promotionState) != ResourceState::COMMON);

			if (!isPromoted) pBarriers[numBarriers++] = { this, srcState, dstState, subresource, flags };
		}
	}

	return numBarriers;
}

ResourceState Resource_DX12::Transition(ResourceState dstState, uint32_t subresource,
	BarrierFlag flags, ResourceState srcState, uint32_t threadIdx)
{
	const bool isBeginOnly = (flags & BarrierFlag::BEGIN_ONLY) == BarrierFlag::BEGIN_ONLY;
	if (subresource == XUSG_BARRIER_ALL_SUBRESOURCES)
	{
		srcState = srcState == ResourceState::AUTO ? m_states[threadIdx][0] : srcState;
		if (!isBeginOnly)
			for (auto& state : m_states[threadIdx]) state = dstState;
	}
	else
	{
		srcState = srcState == ResourceState::AUTO ? m_states[threadIdx][subresource] : srcState;
		m_states[threadIdx][subresource] = isBeginOnly ? srcState : dstState;
	}

	return srcState;
}

ResourceState Resource_DX12::GetResourceState(uint32_t subresource, uint32_t threadIdx) const
{
	assert(!m_resource || !m_states.empty());
	assert(m_states.empty() || threadIdx < m_states.size());

	return !m_states.empty() && subresource < m_states[threadIdx].size() ? m_states[threadIdx][subresource] : ResourceState::COMMON;
}

uint64_t Resource_DX12::GetWidth() const
{
	return m_resource ? m_resource->GetDesc().Width : 0;
}

uint64_t Resource_DX12::GetVirtualAddress(int offset) const
{
	return m_resource ? m_resource->GetGPUVirtualAddress() + offset : 0;
}

void Resource_DX12::Create(void* pDeviceHandle, void* pResourceHandle,
	const wchar_t* name, uint32_t maxThreads)
{
	m_device = pDeviceHandle;
	m_resource = pResourceHandle;

	if (m_resource)
	{
		const auto desc = m_resource->GetDesc();
		m_states.resize(maxThreads);
		for (auto& states : m_states)
			states.resize(desc.MipLevels * desc.DepthOrArraySize, ResourceState::COMMON);

		if (name) m_resource->SetName((wstring(name) + L".Resource").c_str());
	}
}

void Resource_DX12::SetName(const wchar_t* name)
{
	if (name)
	{
		m_name = name;
		if (m_resource) m_resource->SetName((m_name + L".Resource").c_str());
	}
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
	m_cbvs(0),
	m_cbvByteOffsets(0),
	m_pDataBegin(nullptr)
{
}

ConstantBuffer_DX12::~ConstantBuffer_DX12()
{
	if (m_resource) Unmap();
}

bool ConstantBuffer_DX12::Create(const Device* pDevice, size_t byteWidth, uint32_t numCBVs,
	const size_t* byteOffsets, MemoryType memoryType, MemoryFlag memoryFlags, const wchar_t* name)
{
	XUSG_N_RETURN(Initialize(pDevice), false);

	// Instanced CBVs
	assert(byteWidth);
	vector<size_t> offsetList;
	if (!byteOffsets)
	{
		size_t numBytes = 0;
		// CB size is required to be D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT-byte aligned.
		const auto cbvSize = AlignConstantBufferView(byteWidth / numCBVs);
		offsetList.resize(numCBVs);

		for (auto& offset : offsetList)
		{
			offset = numBytes;
			numBytes += cbvSize;
		}

		byteOffsets = offsetList.data();
		byteWidth = cbvSize * numCBVs;
	}

	if (byteWidth)
		XUSG_N_RETURN(CreateResource(byteWidth, memoryType, memoryFlags, ResourceState::COMMON), false);

	SetName(name);

	// Create constant buffer views.
	m_cbvs.resize(numCBVs);
	m_cbvByteOffsets.resize(numCBVs);
	const auto maxSize = static_cast<uint32_t>(byteWidth);
	const auto cbvHeapStart = AllocateCbvSrvUavHeap(numCBVs);
	for (auto i = 0u; i < numCBVs; ++i)
	{
		const auto& byteOffset = byteOffsets[i];
		const auto byteSize = static_cast<uint32_t>((i + 1 >= numCBVs ? maxSize : byteOffsets[i + 1]) - byteOffset);
		m_cbvByteOffsets[i] = byteOffset;

		XUSG_X_RETURN(m_cbvs[i], CreateCBV(cbvHeapStart, i, byteSize, byteOffset), false);
	}

	return true;
}

bool ConstantBuffer_DX12::CreateResource(size_t byteWidth, MemoryType memoryType,
	MemoryFlag memoryFlags, ResourceState initialResourceState)
{
	assert(byteWidth);
	const CD3DX12_HEAP_PROPERTIES heapProperties(GetDX12HeapType(memoryType));
	const auto desc = CD3DX12_RESOURCE_DESC::Buffer(byteWidth, D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE);

	// Determine initial state
	m_states.resize(1);
	m_states[0].resize(1);
	m_states[0][0] = memoryType == MemoryType::DEFAULT ? initialResourceState : ResourceState::GENERAL_READ;

	V_RETURN(m_device->CreateCommittedResource(&heapProperties, GetDX12HeapFlags(memoryFlags), &desc,
		GetDX12ResourceStates(m_states[0][0]), nullptr, IID_PPV_ARGS(&m_resource)), clog, false);

	return true;
}

bool ConstantBuffer_DX12::Upload(CommandList* pCommandList, Resource* pUploader, const void* pData,
	size_t size, size_t byteOffset, ResourceState srcState, ResourceState dstState)
{
	// Create the GPU upload buffer.
	assert(pUploader);
	auto& uploaderResource = dynamic_cast<Resource_DX12*>(pUploader)->GetResource();
	if (!uploaderResource)
	{
		const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
		const auto desc = CD3DX12_RESOURCE_DESC::Buffer(size);

		V_RETURN(m_device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploaderResource)), clog, false);
		if (!m_name.empty()) uploaderResource->SetName((m_name + L".UploaderResource").c_str());
	}

	// Copy data to the intermediate upload heap and then schedule a copy 
	// from the upload heap to the buffer.
	void* pUploadData;
	V_RETURN(uploaderResource->Map(0, nullptr, &pUploadData), clog, false);
	memcpy(pUploadData, pData, size);
	uploaderResource->Unmap(0, nullptr);

	if (srcState != ResourceState::COMMON)
	{
		const ResourceBarrier barrier = { this, srcState, ResourceState::COPY_DEST, XUSG_BARRIER_ALL_SUBRESOURCES };
		pCommandList->Barrier(1, &barrier);
	}

	const auto pGraphicsCommandList = static_cast<ID3D12GraphicsCommandList*>(pCommandList->GetHandle());
	pGraphicsCommandList->CopyBufferRegion(m_resource.get(), byteOffset, uploaderResource.get(), 0, size);

	const ResourceBarrier barrier = { this, ResourceState::COPY_DEST, dstState, XUSG_BARRIER_ALL_SUBRESOURCES };
	pCommandList->Barrier(1, &barrier);

	return true;
}

bool ConstantBuffer_DX12::Upload(CommandList* pCommandList, uint32_t cbvIndex, Resource* pUploader,
	const void* pData, size_t byteSize, ResourceState srcState, ResourceState dstState)
{
	const auto byteIffset = m_cbvByteOffsets.empty() ? 0 : m_cbvByteOffsets[cbvIndex];

	return Upload(pCommandList, pUploader, pData, byteSize, byteIffset, srcState, dstState);
}

Descriptor ConstantBuffer_DX12::CreateCBV(const Descriptor& cbvHeapStart, uint32_t descriptorIdx,
	uint32_t byteSize, size_t byteOffset)
{
	// Describe and create a constant buffer view.
	D3D12_CONSTANT_BUFFER_VIEW_DESC desc;
	desc.BufferLocation = m_resource->GetGPUVirtualAddress() + byteOffset;
	desc.SizeInBytes = byteSize;

	// Create a constant buffer view
	const auto stride = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	const auto descriptor = cbvHeapStart + stride * descriptorIdx;
	m_device->CreateConstantBufferView(&desc, { descriptor });

	return descriptor;
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

	return &reinterpret_cast<uint8_t*>(m_pDataBegin)[m_cbvByteOffsets[cbvIndex]];
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
	return static_cast<uint32_t>(m_cbvByteOffsets[index]);
}

//--------------------------------------------------------------------------------------
// Resource base
//--------------------------------------------------------------------------------------

ShaderResource_DX12::ShaderResource_DX12() :
	Resource_DX12(),
	m_format(Format::UNKNOWN),
	m_srvs(0)
{
}

ShaderResource_DX12::~ShaderResource_DX12()
{
}

bool ShaderResource_DX12::Initialize(const Device* pDevice, Format format)
{
	m_format = format;

	return Resource_DX12::Initialize(pDevice);
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

Format ShaderResource_DX12::mapToPackedFormat(Format& format)
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
// 2D Texture
//--------------------------------------------------------------------------------------

Texture_DX12::Texture_DX12() :
	ShaderResource_DX12(),
	m_uavs(0),
	m_packedUavs(0),
	m_srvLevels(0)
{
}

Texture_DX12::~Texture_DX12()
{
}

bool Texture_DX12::Create(const Device* pDevice, uint32_t width, uint32_t height, Format format,
	uint16_t arraySize, ResourceFlag resourceFlags, uint8_t numMips, uint8_t sampleCount, bool isCubeMap,
	MemoryFlag memoryFlags, const wchar_t* name, uint16_t srvComponentMapping, TextureLayout textureLayout,
	uint32_t maxThreads)
{
	assert(numMips >= 1);
	assert(sampleCount >= 1 && sampleCount <= 16);
	assert(sampleCount <= 1 || numMips <= 1);
	XUSG_N_RETURN(Initialize(pDevice, format), false);

	const auto needPackedUAV = (resourceFlags & ResourceFlag::NEED_PACKED_UAV) == ResourceFlag::NEED_PACKED_UAV;
	const auto hasSRV = (resourceFlags & ResourceFlag::DENY_SHADER_RESOURCE) == ResourceFlag::NONE;
	const auto hasUAV = (resourceFlags & ResourceFlag::ALLOW_UNORDERED_ACCESS) == ResourceFlag::ALLOW_UNORDERED_ACCESS;

	// Map format
	const auto formatUAV = needPackedUAV ? mapToPackedFormat(format) : format;

	if (width && height && arraySize)
		XUSG_N_RETURN(CreateResource(width, height, format, arraySize, resourceFlags, numMips,
			sampleCount, memoryFlags, ResourceState::COMMON, textureLayout, maxThreads), false);

	SetName(name);

	// Allocate CBV SRV UAV heap
	uint32_t numDescriptors = 0;
	if (hasSRV)
	{
		numDescriptors += numMips;
		if (hasUAV && numMips >= 1) numDescriptors += numMips;
	}
	if (hasUAV)
	{
		numDescriptors += numMips;
		if (needPackedUAV) numDescriptors += numMips;
	}
	AllocateCbvSrvUavHeap(numDescriptors);
	auto descriptorIdx = 0u;

	// Create SRVs
	if (hasSRV)
		XUSG_N_RETURN(createSRVs(descriptorIdx, arraySize, m_format, numMips, sampleCount > 1, isCubeMap, srvComponentMapping), false);

	// Create UAVs
	if (hasUAV)
	{
		XUSG_N_RETURN(createUAVs(descriptorIdx, arraySize, m_format, numMips), false);
		if (needPackedUAV) XUSG_N_RETURN(createPackedUAVs(descriptorIdx, arraySize, formatUAV, numMips), false);
	}

	// Create SRV for each level
	if (hasSRV && hasUAV)
		XUSG_N_RETURN(createSRVLevels(descriptorIdx, arraySize, numMips, m_format, sampleCount > 1, isCubeMap, srvComponentMapping), false);

	return true;
}

bool Texture_DX12::CreateResource(uint32_t width, uint32_t height, Format format,
	uint16_t arraySize, ResourceFlag resourceFlags, uint8_t numMips,
	uint8_t sampleCount, MemoryFlag memoryFlags, ResourceState initialResourceState,
	TextureLayout textureLayout, uint32_t maxThreads)
{
	assert(numMips >= 1);
	assert(sampleCount >= 1 && sampleCount <= 16);
	assert(sampleCount <= 1 || numMips <= 1);
	m_hasPromotion = (resourceFlags & ResourceFlag::ALLOW_SIMULTANEOUS_ACCESS) == ResourceFlag::ALLOW_SIMULTANEOUS_ACCESS;

	// Setup the texture description.
	assert(width && height && arraySize);
	const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
	const auto desc = CD3DX12_RESOURCE_DESC::Tex2D(GetDXGIFormat(format), width, height, arraySize, numMips,
		sampleCount, 0, GetDX12ResourceFlags(resourceFlags), GetDX12TextureLayout(textureLayout));

	// Determine initial state
	assert(maxThreads);
	m_states.resize(maxThreads);
	for (auto& states : m_states)
		states.resize(static_cast<uint32_t>(arraySize) * numMips, initialResourceState);

	// Create the render target texture.
	V_RETURN(m_device->CreateCommittedResource(&heapProperties, GetDX12HeapFlags(memoryFlags), &desc,
		GetDX12ResourceStates(m_states[0][0]), nullptr, IID_PPV_ARGS(&m_resource)), clog, false);

	return true;
}

bool Texture_DX12::Upload(CommandList* pCommandList, Resource* pUploader,
	const SubresourceData* pSubresourceData, uint32_t numSubresources,
	ResourceState dstState, uint32_t firstSubresource, uint32_t threadIdx)
{
	XUSG_N_RETURN(pSubresourceData, false);
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
		const auto uploadBufferSize = GetRequiredIntermediateSize(firstSubresource, numSubresources);
		const auto desc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

		V_RETURN(m_device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploaderResource)), clog, false);
		if (!m_name.empty()) uploaderResource->SetName((m_name + L".UploaderResource").c_str());
	}

	// Copy data to the intermediate upload heap and then schedule a copy 
	// from the upload heap to the Texture2D.
	vector<ResourceBarrier> barriers(numSubresources);
	auto numBarriers = 0u;
	for (auto i = 0u; i < numSubresources; ++i)
		numBarriers = SetBarrier(barriers.data(), ResourceState::COPY_DEST, numBarriers,
			firstSubresource + i, BarrierFlag::NONE, ResourceState::AUTO, threadIdx);
	pCommandList->Barrier(numBarriers, barriers.data());

	const auto pGraphicsCommandList = static_cast<ID3D12GraphicsCommandList*>(pCommandList->GetHandle());
	XUSG_M_RETURN(UpdateSubresources(pGraphicsCommandList, m_resource.get(), uploaderResource.get(),
		0, firstSubresource, numSubresources, subresourceData.data()) <= 0,
		clog, "Failed to upload the resource.", false);

	numBarriers = 0;
	for (auto i = 0u; i < numSubresources; ++i)
		numBarriers = SetBarrier(barriers.data(), dstState, numBarriers, firstSubresource + i,
			BarrierFlag::NONE, ResourceState::AUTO, threadIdx);
	pCommandList->Barrier(numBarriers, barriers.data());

	return true;
}

bool Texture_DX12::Upload(CommandList* pCommandList, Resource* pUploader, const void* pData,
	uint8_t byteStride, ResourceState dstState, uint32_t threadIdx)
{
	const auto desc = m_resource->GetDesc();

	SubresourceData subresourceData;
	subresourceData.pData = pData;
	subresourceData.RowPitch = byteStride * static_cast<intptr_t>(desc.Width);
	subresourceData.SlicePitch = subresourceData.RowPitch * desc.Height;

	return Upload(pCommandList, pUploader, &subresourceData, 1, dstState, threadIdx);
}

bool Texture_DX12::ReadBack(CommandList* pCommandList, Buffer* pReadBuffer, uint32_t* pRowPitches,
	uint32_t numSubresources, uint32_t firstSubresource, size_t offset, ResourceState dstState, uint32_t threadIdx)
{
	// Get copyable footprints
	vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts(numSubresources);
	vector<uint32_t> numRows(numSubresources);
	vector<uint64_t> rowSizesInBytes(numSubresources);
	uint64_t readBufferSize;

	const auto desc = m_resource->GetDesc();

	com_ptr<ID3D12Device> device = nullptr;
	m_resource->GetDevice(IID_PPV_ARGS(&device));
	device->GetCopyableFootprints(&desc, firstSubresource, numSubresources, offset,
		layouts.data(), numRows.data(), rowSizesInBytes.data(), &readBufferSize);

	// Create the GPU read-back buffer.
	assert(pReadBuffer);
	if (!pReadBuffer->GetHandle())
		XUSG_N_RETURN(pReadBuffer->Create(pCommandList->GetDevice(), static_cast<size_t>(readBufferSize),
			ResourceFlag::DENY_SHADER_RESOURCE, MemoryType::READBACK, 0, nullptr, 0, nullptr, MemoryFlag::NONE,
			(m_name + L".ReadResource").c_str()), false);

	auto& readResource = dynamic_cast<Resource_DX12*>(pReadBuffer)->GetResource();
	assert(readResource);

	vector<ResourceState> dstStates(numSubresources);
	vector<ResourceBarrier> barriers(numSubresources);
	auto numBarriers = 0u;
	for (auto i = 0u; i < numSubresources; ++i)
	{
		const auto subresource = firstSubresource + i;
		dstStates[i] = dstState == ResourceState::COMMON ? m_states[threadIdx][subresource] : dstState;
		numBarriers = SetBarrier(barriers.data(), ResourceState::COPY_SOURCE, numBarriers, subresource,
			BarrierFlag::NONE, ResourceState::AUTO, threadIdx);
	}
	pCommandList->Barrier(numBarriers, barriers.data());

	const auto pGraphicsCommandList = static_cast<ID3D12GraphicsCommandList*>(pCommandList->GetHandle());
	for (auto i = 0u; i < numSubresources; ++i)
	{
		const CD3DX12_TEXTURE_COPY_LOCATION dst(readResource.get(), layouts[i]);
		const CD3DX12_TEXTURE_COPY_LOCATION src(m_resource.get(), firstSubresource + i);
		pGraphicsCommandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
		if (pRowPitches) pRowPitches[i] = layouts[i].Footprint.RowPitch;
	}

	numBarriers = 0;
	for (auto i = 0u; i < numSubresources; ++i)
		numBarriers = SetBarrier(barriers.data(), dstStates[i], numBarriers, firstSubresource + i,
			BarrierFlag::NONE, ResourceState::AUTO, threadIdx);
	pCommandList->Barrier(numBarriers, barriers.data());

	return true;
}

Descriptor Texture_DX12::CreateSRV(const Descriptor& srvHeapStart, uint32_t descriptorIdx, uint16_t arraySize,
	uint16_t firstArraySlice, Format format, uint8_t numMips, uint8_t mostDetailedMip, bool multisamples,
	bool isCubeMap, uint16_t srvComponentMapping, uint8_t plane)
{
	// Setup the description of the shader resource view.
	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	assert(m_resource || format != Format::UNKNOWN);
	desc.Format = format != Format::UNKNOWN ? GetDXGIFormat(format) : m_resource->GetDesc().Format;
	desc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
		GetDX12ShaderComponentMapping(DECODE_SRV_COMPONENT_MAPPING(0, srvComponentMapping)),
		GetDX12ShaderComponentMapping(DECODE_SRV_COMPONENT_MAPPING(1, srvComponentMapping)),
		GetDX12ShaderComponentMapping(DECODE_SRV_COMPONENT_MAPPING(2, srvComponentMapping)),
		GetDX12ShaderComponentMapping(DECODE_SRV_COMPONENT_MAPPING(3, srvComponentMapping)));

	if (isCubeMap)
	{
		assert(arraySize % 6 == 0);
		if (arraySize > 6)
		{
			desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
			desc.TextureCubeArray.MipLevels = numMips;
			desc.TextureCubeArray.MostDetailedMip = mostDetailedMip;
			desc.TextureCubeArray.NumCubes = arraySize / 6;
		}
		else
		{
			desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
			desc.TextureCube.MipLevels = numMips;
			desc.TextureCube.MostDetailedMip = mostDetailedMip;
		}
	}
	else if (arraySize > 1 || firstArraySlice > 0)
	{
		if (multisamples)
		{
			desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
			desc.Texture2DMSArray.ArraySize = arraySize;
			desc.Texture2DMSArray.FirstArraySlice = firstArraySlice;
		}
		else
		{
			desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			desc.Texture2DArray.ArraySize = arraySize;
			desc.Texture2DArray.FirstArraySlice = firstArraySlice;
			desc.Texture2DArray.MipLevels = numMips;
			desc.Texture2DArray.MostDetailedMip = mostDetailedMip;
			desc.Texture2DArray.PlaneSlice = plane;
		}
	}
	else
	{
		if (multisamples)
			desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
		else
		{
			desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			desc.Texture2D.MipLevels = numMips;
			desc.Texture2D.MostDetailedMip = mostDetailedMip;
			desc.Texture2D.PlaneSlice = plane;
		}
	}

	// Create a shader resource view
	const auto stride = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	const auto descriptor = srvHeapStart + stride * descriptorIdx;
	m_device->CreateShaderResourceView(m_resource.get(), &desc, { descriptor });

	return descriptor;
}

Descriptor Texture_DX12::CreateUAV(const Descriptor& uavHeapStart, uint32_t descriptorIdx, uint16_t arraySize,
	uint16_t firstArraySlice, Format format, uint8_t mipLevel, uint8_t plane)
{
	// Setup the description of the unordered access view.
	D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
	assert(m_resource || format != Format::UNKNOWN);
	desc.Format = format != Format::UNKNOWN ? GetDXGIFormat(format) : m_resource->GetDesc().Format;

	if (arraySize > 1 || firstArraySlice > 0)
	{
		desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		desc.Texture2DArray.ArraySize = arraySize;
		desc.Texture2DArray.FirstArraySlice = firstArraySlice;
		desc.Texture2DArray.MipSlice = mipLevel;
		desc.Texture2DArray.PlaneSlice = plane;
	}
	else
	{
		desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipSlice = mipLevel;
		desc.Texture2D.PlaneSlice = plane;
	}

	// Create an unordered access view
	const auto stride = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	const auto descriptor = uavHeapStart + stride * descriptorIdx;
	m_device->CreateUnorderedAccessView(m_resource.get(), nullptr, &desc, { descriptor });

	return descriptor;
}

uint32_t Texture_DX12::SetBarrier(ResourceBarrier* pBarriers, ResourceState dstState, uint32_t numBarriers,
	uint32_t subresource, BarrierFlag flags, ResourceState srcState, uint32_t threadIdx)
{
	return ShaderResource_DX12::SetBarrier(pBarriers, dstState, numBarriers, subresource, flags, srcState, threadIdx);
}

uint32_t Texture_DX12::SetBarrier(ResourceBarrier* pBarriers, uint8_t mipLevel, ResourceState dstState,
	uint32_t numBarriers, uint32_t slice, BarrierFlag flags, ResourceState srcState, uint32_t threadIdx)
{
	assert(m_resource);
	const auto desc = m_resource->GetDesc();
	const auto subresource = D3D12CalcSubresource(mipLevel, slice, 0, desc.MipLevels, desc.DepthOrArraySize);

	return SetBarrier(pBarriers, dstState, numBarriers, subresource, flags, srcState, threadIdx);
}

void Texture_DX12::Blit(const CommandList* pCommandList, uint32_t groupSizeX, uint32_t groupSizeY,
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
	pCommandList->Dispatch(XUSG_DIV_UP(width, groupSizeX), XUSG_DIV_UP(height, groupSizeY),
		XUSG_DIV_UP(desc.DepthOrArraySize, groupSizeZ));
}

uint32_t Texture_DX12::Blit(CommandList* pCommandList, ResourceBarrier* pBarriers,
	uint32_t groupSizeX, uint32_t groupSizeY, uint32_t groupSizeZ, uint8_t mipLevel,
	int8_t srcMipLevel, ResourceState srcState, const DescriptorTable& uavSrvTable,
	uint32_t uavSrvSlot, uint32_t numBarriers, const DescriptorTable& srvTable,
	uint32_t srvSlot, uint16_t baseSlice, uint16_t numSlices, uint32_t threadIdx)
{
	const auto prevBarriers = numBarriers;
	const auto desc = m_resource->GetDesc();
	if (!numSlices) numSlices = desc.DepthOrArraySize - baseSlice;

	if (!mipLevel && srcMipLevel <= mipLevel)
		numBarriers = SetBarrier(pBarriers, ResourceState::UNORDERED_ACCESS, numBarriers,
			XUSG_BARRIER_ALL_SUBRESOURCES, BarrierFlag::NONE, ResourceState::AUTO, threadIdx);
	else for (uint16_t i = 0; i < numSlices; ++i)
	{
		const uint16_t j = baseSlice + i;
		const auto subresource = D3D12CalcSubresource(mipLevel, j, 0, desc.MipLevels, desc.DepthOrArraySize);
		if (m_states[threadIdx][subresource] != ResourceState::UNORDERED_ACCESS)
		{
			pBarriers[numBarriers++] = { this, m_states[threadIdx][subresource], ResourceState::UNORDERED_ACCESS, subresource };
			m_states[threadIdx][subresource] = ResourceState::UNORDERED_ACCESS;
		}
		numBarriers = SetBarrier(pBarriers, srcMipLevel, srcState,
			numBarriers, j, BarrierFlag::NONE, ResourceState::AUTO, threadIdx);
	}

	if (numBarriers > prevBarriers)
	{
		pCommandList->Barrier(numBarriers, pBarriers);
		numBarriers = 0;
	}

	Blit(pCommandList, groupSizeX, groupSizeY, groupSizeZ, uavSrvTable, uavSrvSlot, mipLevel, srvTable, srvSlot);

	return numBarriers;
}

uint32_t Texture_DX12::GenerateMips(CommandList* pCommandList, ResourceBarrier* pBarriers, uint32_t groupSizeX,
	uint32_t groupSizeY, uint32_t groupSizeZ, ResourceState dstState, const PipelineLayout& pipelineLayout,
	const Pipeline& pipeline, const DescriptorTable* pUavSrvTables, uint32_t uavSrvSlot, const DescriptorTable& samplerTable,
	uint32_t samplerSlot, uint32_t numBarriers, const DescriptorTable* pSrvTables, uint32_t srvSlot, uint8_t baseMip,
	uint8_t numMips, uint16_t baseSlice, uint16_t numSlices, uint32_t threadIdx)
{
	if (pipelineLayout) pCommandList->SetComputePipelineLayout(pipelineLayout);
	if (samplerTable) pCommandList->SetComputeDescriptorTable(samplerSlot, samplerTable);
	if (pipeline) pCommandList->SetPipelineState(pipeline);

	if (!(numMips || baseMip || numSlices || baseSlice))
		numBarriers = SetBarrier(pBarriers, ResourceState::UNORDERED_ACCESS, numBarriers,
			XUSG_BARRIER_ALL_SUBRESOURCES, BarrierFlag::NONE, ResourceState::AUTO, threadIdx);

	const auto desc = m_resource->GetDesc();
	if (!numSlices) numSlices = desc.DepthOrArraySize - baseSlice;
	if (!numMips) numMips = desc.MipLevels - baseMip;

	for (uint8_t i = 0; i < numMips; ++i)
	{
		const uint8_t j = baseMip + i;
		const auto prevBarriers = numBarriers;

		if (j) for (uint16_t k = 0; k < numSlices; ++k)
		{
			const uint16_t n = baseSlice + k;
			auto subresource = D3D12CalcSubresource(j, n, 0, desc.MipLevels, desc.DepthOrArraySize);
			if (m_states[threadIdx][subresource] != ResourceState::UNORDERED_ACCESS)
			{
				pBarriers[numBarriers++] = { this, m_states[threadIdx][subresource], ResourceState::UNORDERED_ACCESS, subresource };
				m_states[threadIdx][subresource] = ResourceState::UNORDERED_ACCESS;
			}

			subresource = D3D12CalcSubresource(j - 1, n, 0, desc.MipLevels, desc.DepthOrArraySize);
			if ((m_states[threadIdx][subresource] & dstState) != dstState)
				numBarriers = SetBarrier(pBarriers, dstState, numBarriers, subresource,
					BarrierFlag::NONE, ResourceState::AUTO, threadIdx);
		}

		pCommandList->Barrier(numBarriers, pBarriers);
		numBarriers = 0;

		Blit(pCommandList, groupSizeX, groupSizeY, groupSizeZ, pUavSrvTables[i], uavSrvSlot, j,
			pSrvTables ? pSrvTables[i] : XUSG_NULL, srvSlot);
	}

	const auto m = baseMip + numMips - 1;
	for (uint16_t i = 0; i < numSlices; ++i)
		numBarriers = SetBarrier(pBarriers, m, dstState, numBarriers, baseSlice + i,
			BarrierFlag::NONE, ResourceState::AUTO, threadIdx);

	return numBarriers;
}

const Descriptor& Texture_DX12::GetUAV(uint8_t index) const
{
	assert(m_uavs.size() > index);
	return m_uavs[index];
}

const Descriptor& Texture_DX12::GetPackedUAV(uint8_t index) const
{
	assert(m_packedUavs.size() > index);
	return m_packedUavs[index];
}

const Descriptor& Texture_DX12::GetSRVLevel(uint8_t level) const
{
	assert(m_srvLevels.size() > level);
	return m_srvLevels[level];
}

uint32_t Texture_DX12::GetHeight() const
{
	return m_resource ? m_resource->GetDesc().Height : 0;
}

uint16_t Texture_DX12::GetArraySize() const
{
	return m_resource ? m_resource->GetDesc().DepthOrArraySize : 1;
}

uint8_t Texture_DX12::GetNumMips() const
{
	return m_resource ? static_cast<uint8_t>(m_resource->GetDesc().MipLevels) : 1;
}

size_t Texture_DX12::GetRequiredIntermediateSize(uint32_t firstSubresource, uint32_t numSubresources) const
{
	return static_cast<size_t>(::GetRequiredIntermediateSize(m_resource.get(), firstSubresource, numSubresources));
}

bool Texture_DX12::createSRVs(uint32_t& descriptorIdx, uint16_t arraySize, Format format,
	uint8_t numMips, bool multisamples, bool isCubeMap, uint16_t srvComponentMapping)
{
	const auto srvHeapStart = m_cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart().ptr;
	m_srvs.resize(numMips);

	for (auto i = 0u; i < numMips; ++i)
		XUSG_X_RETURN(m_srvs[i], CreateSRV(srvHeapStart, descriptorIdx + i, arraySize, 0, format, numMips - i,
			i, multisamples, isCubeMap, srvComponentMapping), false);
	descriptorIdx += numMips;

	return true;
}

bool Texture_DX12::createSRVLevels(uint32_t& descriptorIdx, uint16_t arraySize, uint8_t numMips, Format format,
	bool multisamples, bool isCubeMap, uint16_t srvComponentMapping)
{
	m_srvLevels.resize(numMips);

	if (numMips <= 1 && !m_srvs.empty()) m_srvLevels[0] = m_srvs[0];
	else
	{
		const auto srvHeapStart = m_cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart().ptr;
		for (auto i = 0u; i < numMips; ++i)
			XUSG_X_RETURN(m_srvLevels[i], CreateSRV(srvHeapStart, descriptorIdx + i, arraySize,
				0, format, 1, i, multisamples, isCubeMap, srvComponentMapping), false);
		descriptorIdx += numMips;
	}

	return true;
}

bool Texture_DX12::createUAVs(uint32_t& descriptorIdx, uint16_t arraySize, Format format, uint8_t numMips)
{
	const auto uavHeapStart = m_cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart().ptr;
	m_uavs.resize(numMips);

	for (auto i = 0u; i < numMips; ++i)
		XUSG_X_RETURN(m_uavs[i], CreateUAV(uavHeapStart, descriptorIdx + i, arraySize, 0, format, i), false);
	descriptorIdx += numMips;

	return true;
}

bool Texture_DX12::createPackedUAVs(uint32_t& descriptorIdx, uint16_t arraySize, Format format, uint8_t numMips)
{
	const auto uavHeapStart = m_cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart().ptr;
	m_packedUavs.resize(numMips);

	for (auto i = 0u; i < numMips; ++i)
		XUSG_X_RETURN(m_packedUavs[i], CreateUAV(uavHeapStart, descriptorIdx + i, arraySize, 0, format, i), false);
	descriptorIdx += numMips;

	return true;
}

//--------------------------------------------------------------------------------------
// Render target
//--------------------------------------------------------------------------------------

RenderTarget_DX12::RenderTarget_DX12() :
	Texture_DX12(),
	m_rtvHeap(nullptr),
	m_rtvs(0)
{
}

RenderTarget_DX12::~RenderTarget_DX12()
{
}

bool RenderTarget_DX12::Create(const Device* pDevice, uint32_t width, uint32_t height, Format format,
	uint16_t arraySize, ResourceFlag resourceFlags, uint8_t numMips, uint8_t sampleCount,
	const float* pClearColor, bool isCubeMap, MemoryFlag memoryFlags, const wchar_t* name,
	uint16_t srvComponentMapping, TextureLayout textureLayout, uint32_t maxThreads)
{
	XUSG_N_RETURN(create(pDevice, width, height, arraySize, format, numMips, sampleCount,
		resourceFlags, pClearColor, isCubeMap, memoryFlags, name, srvComponentMapping,
		textureLayout, maxThreads), false);

	// Allocate RTV heap
	const auto numDescriptors = numMips * arraySize;
	const auto rtvHeapStart = AllocateRtvHeap(numDescriptors);
	auto descriptorIdx = 0u;

	// Create render target views.
	m_rtvs.resize(arraySize);
	for (uint16_t i = 0; i < arraySize; ++i)
	{
		m_rtvs[i].resize(numMips);
		for (auto j = 0u; j < numMips; ++j)
			XUSG_X_RETURN(m_rtvs[i][j], CreateRTV(rtvHeapStart, descriptorIdx++, 1, i, format, j, sampleCount > 1), false);
	}

	return true;
}

bool RenderTarget_DX12::CreateArray(const Device* pDevice, uint32_t width, uint32_t height,
	uint16_t arraySize, Format format, ResourceFlag resourceFlags, uint8_t numMips,
	uint8_t sampleCount, const float* pClearColor, bool isCubeMap, MemoryFlag memoryFlags,
	const wchar_t* name, uint16_t srvComponentMapping, TextureLayout textureLayout,
	uint32_t maxThreads)
{
	XUSG_N_RETURN(create(pDevice, width, height, arraySize, format, numMips, sampleCount,
		resourceFlags, pClearColor, isCubeMap, memoryFlags, name, srvComponentMapping,
		textureLayout, maxThreads), false);

	// Allocate RTV heap
	const auto rtvHeapStart = AllocateRtvHeap(numMips);

	// Create render target views.
	m_rtvs.resize(1);
	m_rtvs[0].resize(numMips);

	for (auto i = 0u; i < numMips; ++i)
		XUSG_X_RETURN(m_rtvs[0][i], CreateRTV(rtvHeapStart, i, arraySize, 0, format, i, sampleCount > 1), false);

	return true;
}

bool RenderTarget_DX12::Initialize(const Device* pDevice, Format format)
{
	m_rtvHeap = nullptr;

	return Texture_DX12::Initialize(pDevice, format);
}

bool RenderTarget_DX12::CreateFromSwapChain(const Device* pDevice, const SwapChain* pSwapChain,
	uint32_t bufferIndex, uint32_t maxThreads)
{
	XUSG_N_RETURN(Resource_DX12::Initialize(pDevice), false);

	m_name = L"SwapChain[" + to_wstring(bufferIndex) + L"]";

	// Determine initial state
	assert(maxThreads);
	m_states.resize(maxThreads);
	for (auto& states : m_states)
	{
		states.resize(1);
		states[0] = ResourceState::PRESENT;
	}

	// Get resource
	XUSG_N_RETURN(pSwapChain->GetBuffer(bufferIndex, this), false);

	// Create RTV
	m_rtvs.resize(1);
	m_rtvs[0].resize(1);
	m_rtvs[0][0] = AllocateRtvHeap(1);
	XUSG_N_RETURN(m_rtvs[0][0], false);
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

bool RenderTarget_DX12::CreateResource(uint32_t width, uint32_t height, Format format,
	uint16_t arraySize, ResourceFlag resourceFlags, uint8_t numMips, uint8_t sampleCount,
	MemoryFlag memoryFlags, ResourceState initialResourceState, const float* pClearColor,
	TextureLayout textureLayout, uint32_t maxThreads)
{
	assert(numMips >= 1);
	assert(sampleCount >= 1 && sampleCount <= 16);
	assert(sampleCount <= 1 || numMips <= 1);
	m_hasPromotion = (resourceFlags & ResourceFlag::ALLOW_SIMULTANEOUS_ACCESS) == ResourceFlag::ALLOW_SIMULTANEOUS_ACCESS;

	// Setup the texture description.
	assert(width && height && arraySize);
	const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
	const auto desc = CD3DX12_RESOURCE_DESC::Tex2D(GetDXGIFormat(format), width, height, arraySize, numMips, sampleCount,
		0, GetDX12ResourceFlags(ResourceFlag::ALLOW_RENDER_TARGET | resourceFlags), GetDX12TextureLayout(textureLayout));

	// Determine initial state
	assert(maxThreads);
	m_states.resize(maxThreads);
	for (auto& states : m_states)
		states.resize(static_cast<uint32_t>(arraySize) * numMips, initialResourceState);

	// Optimized clear value
	D3D12_CLEAR_VALUE clearValue = { GetDXGIFormat(m_format) };
	if (pClearColor) memcpy(clearValue.Color, pClearColor, sizeof(clearValue.Color));

	// Create the render target texture.
	V_RETURN(m_device->CreateCommittedResource(&heapProperties, GetDX12HeapFlags(memoryFlags), &desc,
		GetDX12ResourceStates(m_states[0][0]), &clearValue, IID_PPV_ARGS(&m_resource)), clog, false);

	return true;
}

Descriptor RenderTarget_DX12::AllocateRtvHeap(uint32_t numDescriptors)
{
	const D3D12_DESCRIPTOR_HEAP_DESC desc = { D3D12_DESCRIPTOR_HEAP_TYPE_RTV, numDescriptors };
	V_RETURN(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_rtvHeap)), cerr, 0);
	if (!m_name.empty()) m_rtvHeap->SetName((m_name + L".RtvHeap").c_str());

	return m_rtvHeap->GetCPUDescriptorHandleForHeapStart().ptr;
}

Descriptor RenderTarget_DX12::CreateRTV(const Descriptor& rtvHeapStart, uint32_t descriptorIdx,
	uint16_t arraySize, uint16_t firstArraySlice, Format format, uint8_t mipLevel, bool multisamples)
{
	// Setup the description of the render target view.
	D3D12_RENDER_TARGET_VIEW_DESC desc = {};
	desc.Format = GetDXGIFormat(m_format);

	// Setup the description of the render target view.
	if (arraySize > 1 || firstArraySlice > 0)
	{
		if (multisamples)
		{
			desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
			desc.Texture2DMSArray.FirstArraySlice = firstArraySlice;
			desc.Texture2DMSArray.ArraySize = arraySize;
		}
		else
		{
			desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
			desc.Texture2DArray.FirstArraySlice = firstArraySlice;
			desc.Texture2DArray.ArraySize = arraySize;
			desc.Texture2DArray.MipSlice = mipLevel;
		}
	}
	else
	{
		if (multisamples)
			desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
		else
		{
			desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			desc.Texture2D.MipSlice = mipLevel;
		}
	}

	// Create a render target view
	const auto stride = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	const auto descriptor = rtvHeapStart + stride * descriptorIdx;
	m_device->CreateRenderTargetView(m_resource.get(), &desc, { descriptor });

	return descriptor;
}

void RenderTarget_DX12::Blit(const CommandList* pCommandList, const DescriptorTable& srcSrvTable,
	uint32_t srcSlot, uint8_t mipLevel, uint16_t baseSlice, uint16_t numSlices,
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
	else for (uint16_t i = 0; i < numSlices; ++i)
	{
		// Set render target
		pCommandList->OMSetRenderTargets(1, &GetRTV(baseSlice + i, mipLevel));
		pCommandList->SetGraphics32BitConstant(cbSlot, i, offsetForSliceId);
		pCommandList->Draw(3, 1, 0, 0);
	}
}

uint32_t RenderTarget_DX12::Blit(CommandList* pCommandList, ResourceBarrier* pBarriers, uint8_t mipLevel,
	int8_t srcMipLevel, ResourceState srcState, const DescriptorTable& srcSrvTable, uint32_t srcSlot,
	uint32_t numBarriers, uint16_t baseSlice, uint16_t numSlices,
	uint32_t offsetForSliceId, uint32_t cbSlot, uint32_t threadIdx)
{
	const auto prevBarriers = numBarriers;
	if (!numSlices) numSlices = m_resource->GetDesc().DepthOrArraySize - baseSlice;

	if (!mipLevel && srcMipLevel <= mipLevel)
		numBarriers = SetBarrier(pBarriers, ResourceState::RENDER_TARGET, numBarriers,
			XUSG_BARRIER_ALL_SUBRESOURCES, BarrierFlag::NONE, ResourceState::AUTO, threadIdx);
	else for (uint16_t i = 0; i < numSlices; ++i)
	{
		const auto j = baseSlice + i;
		numBarriers = SetBarrier(pBarriers, mipLevel, ResourceState::RENDER_TARGET,
			numBarriers, j, BarrierFlag::NONE, ResourceState::AUTO, threadIdx);
		numBarriers = SetBarrier(pBarriers, srcMipLevel, srcState, numBarriers,
			j, BarrierFlag::NONE, ResourceState::AUTO, threadIdx);
	}

	if (numBarriers > prevBarriers)
	{
		pCommandList->Barrier(numBarriers, pBarriers);
		numBarriers = 0;
	}

	Blit(pCommandList, srcSrvTable, srcSlot, mipLevel, baseSlice,
		numSlices, XUSG_NULL, 1, nullptr, offsetForSliceId, cbSlot);

	return numBarriers;
}

uint32_t RenderTarget_DX12::GenerateMips(CommandList* pCommandList, ResourceBarrier* pBarriers,
	ResourceState dstState, const PipelineLayout& pipelineLayout, const Pipeline& pipeline,
	const DescriptorTable* pSrcSrvTables, uint32_t srcSlot, const DescriptorTable& samplerTable,
	uint32_t samplerSlot, uint32_t numBarriers, uint8_t baseMip, uint8_t numMips,
	uint16_t baseSlice, uint16_t numSlices, uint32_t offsetForSliceId,
	uint32_t cbSlot, uint32_t threadIdx)
{
	if (pipelineLayout) pCommandList->SetGraphicsPipelineLayout(pipelineLayout);
	if (samplerTable) pCommandList->SetGraphicsDescriptorTable(samplerSlot, samplerTable);
	if (pipeline) pCommandList->SetPipelineState(pipeline);

	if (!(numMips || baseMip || numSlices || baseSlice))
		numBarriers = SetBarrier(pBarriers, ResourceState::RENDER_TARGET, numBarriers,
			XUSG_BARRIER_ALL_SUBRESOURCES, BarrierFlag::NONE, ResourceState::AUTO, threadIdx);

	const auto& desc = m_resource->GetDesc();
	if (!numSlices) numSlices = desc.DepthOrArraySize - baseSlice;
	if (!numMips) numMips = desc.MipLevels - baseMip;

	for (uint8_t i = 0; i < numMips; ++i)
	{
		const uint8_t j = baseMip + i;
		const auto prevBarriers = numBarriers;

		if (j) for (uint16_t k = 0; k < numSlices; ++k)
		{
			const uint16_t n = baseSlice + k;
			numBarriers = SetBarrier(pBarriers, j, ResourceState::RENDER_TARGET,
				numBarriers, n, BarrierFlag::NONE, ResourceState::AUTO, threadIdx);

			const auto subresource = D3D12CalcSubresource(j - 1, n, 0, desc.MipLevels, desc.DepthOrArraySize);
			if ((m_states[threadIdx][subresource] & dstState) != dstState)
				numBarriers = SetBarrier(pBarriers, dstState, numBarriers, subresource,
					BarrierFlag::NONE, ResourceState::AUTO, threadIdx);
		}

		pCommandList->Barrier(numBarriers, pBarriers);
		numBarriers = 0;

		if (numSlices > 1) pCommandList->SetGraphics32BitConstant(cbSlot, 0, offsetForSliceId);
		Blit(pCommandList, pSrcSrvTables[i], srcSlot, j, baseSlice, numSlices,
			XUSG_NULL, samplerSlot, nullptr, offsetForSliceId, cbSlot);
	}

	const auto m = baseMip + numMips - 1;
	for (uint16_t i = 0; i < numSlices; ++i)
		numBarriers = SetBarrier(pBarriers, m, dstState, numBarriers, baseSlice + i,
			BarrierFlag::NONE, ResourceState::AUTO, threadIdx);

	return numBarriers;
}

const Descriptor& RenderTarget_DX12::GetRTV(uint16_t slice, uint8_t mipLevel) const
{
	assert(m_rtvs.size() > slice && m_rtvs[slice].size() > mipLevel);
	return m_rtvs[slice][mipLevel];
}

bool RenderTarget_DX12::create(const Device* pDevice, uint32_t width, uint32_t height, uint16_t arraySize,
	Format format, uint8_t numMips, uint8_t sampleCount, ResourceFlag resourceFlags, const float* pClearColor,
	bool isCubeMap, MemoryFlag memoryFlags, const wchar_t* name, uint16_t srvComponentMapping,
	TextureLayout textureLayout, uint32_t maxThreads)
{
	assert(numMips >= 1);
	assert(sampleCount >= 1 && sampleCount <= 16);
	assert(sampleCount <= 1 || numMips <= 1);
	XUSG_N_RETURN(Initialize(pDevice, format), false);

	const auto needPackedUAV = (resourceFlags & ResourceFlag::NEED_PACKED_UAV) == ResourceFlag::NEED_PACKED_UAV;
	const auto hasSRV = (resourceFlags & ResourceFlag::DENY_SHADER_RESOURCE) == ResourceFlag::NONE;
	const auto hasUAV = (resourceFlags & ResourceFlag::ALLOW_UNORDERED_ACCESS) == ResourceFlag::ALLOW_UNORDERED_ACCESS;

	// Map formats
	const auto formatUAV = needPackedUAV ? mapToPackedFormat(format) : format;

	if (width && height && arraySize)
		XUSG_N_RETURN(CreateResource(width, height, format, arraySize, ResourceFlag::ALLOW_RENDER_TARGET | resourceFlags,
			numMips, sampleCount, memoryFlags, ResourceState::COMMON, pClearColor, textureLayout, maxThreads), false);

	SetName(name);

	// Allocate CBV SRV UAV heap
	uint32_t numDescriptors = 0;
	if (hasSRV)
	{
		numDescriptors += numMips;
		if (numMips >= 1) numDescriptors += numMips;
	}
	if (hasUAV)
	{
		numDescriptors += numMips;
		if (needPackedUAV) numDescriptors += numMips;
	}
	AllocateCbvSrvUavHeap(numDescriptors);
	auto descriptorIdx = 0u;

	// Create SRVs
	if (hasSRV)
	{
		XUSG_N_RETURN(createSRVs(descriptorIdx, arraySize, m_format, numMips, sampleCount > 1, isCubeMap, srvComponentMapping), false);
		XUSG_N_RETURN(createSRVLevels(descriptorIdx, arraySize, numMips, m_format, sampleCount > 1, isCubeMap, srvComponentMapping), false);
	}

	// Create UAVs
	if (hasUAV)
	{
		XUSG_N_RETURN(createUAVs(descriptorIdx, arraySize, m_format, numMips), false);
		if (needPackedUAV) XUSG_N_RETURN(createPackedUAVs(descriptorIdx, arraySize, formatUAV, numMips), false);
	}

	return true;
}

//--------------------------------------------------------------------------------------
// Depth stencil
//--------------------------------------------------------------------------------------

DepthStencil_DX12::DepthStencil_DX12() :
	Texture_DX12(),
	m_dsvs(0),
	m_readOnlyDsvs(0),
	m_stencilSrv(0)
{
}

DepthStencil_DX12::~DepthStencil_DX12()
{
}

bool DepthStencil_DX12::Create(const Device* pDevice, uint32_t width, uint32_t height, Format format,
	ResourceFlag resourceFlags, uint16_t arraySize, uint8_t numMips, uint8_t sampleCount,
	float clearDepth, uint8_t clearStencil, bool isCubeMap, MemoryFlag memoryFlags,
	const wchar_t* name, uint16_t srvComponentMapping, uint16_t stencilSrvComponentMapping,
	TextureLayout textureLayout, uint32_t maxThreads)
{
	bool hasSRV;
	Format formatStencil;
	XUSG_N_RETURN(create(pDevice, width, height, arraySize, numMips, sampleCount, format,
		resourceFlags, clearDepth, clearStencil, hasSRV, formatStencil, isCubeMap, memoryFlags,
		name, srvComponentMapping, stencilSrvComponentMapping, textureLayout, maxThreads), false);

	// Allocate RTV heap
	auto numDescriptors = numMips * arraySize;
	if (hasSRV) numDescriptors *= 2;
	const auto dsvHeapStart = AllocateDsvHeap(numDescriptors);
	auto descriptorIdx = 0u;

	// Create depth-stencil views.
	m_dsvs.resize(arraySize);
	m_readOnlyDsvs.resize(arraySize);

	for (uint16_t i = 0; i < arraySize; ++i)
	{
		uint8_t mipLevel = 0;
		m_dsvs[i].resize(numMips);
		m_readOnlyDsvs[i].resize(numMips);

		for (uint8_t j = 0; j < numMips; ++j)
		{
			XUSG_X_RETURN(m_dsvs[i][j], CreateDSV(dsvHeapStart, descriptorIdx++, 1, i, m_format, j, sampleCount > 1), false);

			// Read-only depth stencil
			if (hasSRV)
			{
				XUSG_X_RETURN(m_readOnlyDsvs[i][j], CreateDSV(dsvHeapStart, descriptorIdx++, 1, i,
					m_format, j, sampleCount > 1, true, formatStencil != Format::UNKNOWN), false);
			}
			else m_readOnlyDsvs[i][j] = m_dsvs[i][j];
		}
	}

	return true;
}

bool DepthStencil_DX12::CreateArray(const Device* pDevice, uint32_t width, uint32_t height,
	uint16_t arraySize, Format format, ResourceFlag resourceFlags, uint8_t numMips,
	uint8_t sampleCount, float clearDepth, uint8_t clearStencil, bool isCubeMap,
	MemoryFlag memoryFlags, const wchar_t* name, uint16_t srvComponentMapping,
	uint16_t stencilSrvComponentMapping, TextureLayout textureLayout, uint32_t maxThreads)
{
	bool hasSRV;
	Format formatStencil;
	XUSG_N_RETURN(create(pDevice, width, height, arraySize, numMips, sampleCount, format,
		resourceFlags, clearDepth, clearStencil, hasSRV, formatStencil, isCubeMap, memoryFlags,
		name, srvComponentMapping, stencilSrvComponentMapping, textureLayout, maxThreads), false);

	// Allocate RTV heap
	auto numDescriptors = numMips;
	if (hasSRV) numDescriptors *= 2;
	const auto dsvHeapStart = AllocateDsvHeap(numDescriptors);
	auto descriptorIdx = 0u;

	// Create depth-stencil views.
	numMips = max<uint8_t>(numMips, 1);
	m_dsvs.resize(1);
	m_dsvs[0].resize(numMips);
	m_readOnlyDsvs.resize(1);
	m_readOnlyDsvs[0].resize(numMips);

	for (uint8_t i = 0; i < numMips; ++i)
	{
		XUSG_X_RETURN(m_dsvs[0][i], CreateDSV(dsvHeapStart, descriptorIdx++, arraySize, 0, m_format, i, sampleCount > 1), false);

		// Read-only depth stencil
		if (hasSRV)
		{
			XUSG_X_RETURN(m_dsvs[0][i], CreateDSV(dsvHeapStart, descriptorIdx++, arraySize, 0,
				m_format, i, sampleCount > 1, true, formatStencil != Format::UNKNOWN), false);
		}
		else m_readOnlyDsvs[0][i] = m_dsvs[0][i];
	}

	return true;
}

bool DepthStencil_DX12::Initialize(const Device* pDevice, Format& format)
{
	m_dsvHeap = nullptr;

	// Map format
	switch (format)
	{
	case Format::R24G8_TYPELESS:
		format = Format::D24_UNORM_S8_UINT;
		break;
	case Format::R32G8X24_TYPELESS:
		format = Format::D32_FLOAT_S8X24_UINT;
		break;
	case Format::R16_TYPELESS:
		format = Format::D16_UNORM;
		break;
	case Format::R32_TYPELESS:
		format = Format::D32_FLOAT;
		break;
	default:
		format = Format::D24_UNORM_S8_UINT;
	}

	return Texture_DX12::Initialize(pDevice, format);
}

bool DepthStencil_DX12::CreateResource(uint32_t width, uint32_t height, Format format,
	uint16_t arraySize, ResourceFlag resourceFlags, uint8_t numMips, uint8_t sampleCount,
	MemoryFlag memoryFlags, ResourceState initialResourceState, float clearDepth,
	uint8_t clearStencil, TextureLayout textureLayout, uint32_t maxThreads)
{
	assert(numMips >= 1);
	assert(sampleCount >= 1 && sampleCount <= 16);
	assert(sampleCount <= 1 || numMips <= 1);
	m_hasPromotion = (resourceFlags & ResourceFlag::ALLOW_SIMULTANEOUS_ACCESS) == ResourceFlag::ALLOW_SIMULTANEOUS_ACCESS;

	// Setup the texture description.
	assert(width && height && arraySize);
	const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
	const auto desc = CD3DX12_RESOURCE_DESC::Tex2D(GetDXGIFormat(format), width, height, arraySize, numMips, sampleCount,
		0, GetDX12ResourceFlags(ResourceFlag::ALLOW_DEPTH_STENCIL | resourceFlags), GetDX12TextureLayout(textureLayout));

	// Determine initial state
	assert(maxThreads);
	m_states.resize(maxThreads);
	for (auto& states : m_states)
		states.resize(static_cast<uint32_t>(arraySize) * numMips, initialResourceState);

	// Optimized clear value
	D3D12_CLEAR_VALUE clearValue = { GetDXGIFormat(m_format) };
	clearValue.DepthStencil.Depth = clearDepth;
	clearValue.DepthStencil.Stencil = clearStencil;

	// Create the render target texture.
	V_RETURN(m_device->CreateCommittedResource(&heapProperties, GetDX12HeapFlags(memoryFlags), &desc,
		GetDX12ResourceStates(m_states[0][0]), &clearValue, IID_PPV_ARGS(&m_resource)), clog, false);

	return true;
}

Descriptor DepthStencil_DX12::AllocateDsvHeap(uint32_t numDescriptors)
{
	const D3D12_DESCRIPTOR_HEAP_DESC desc = { D3D12_DESCRIPTOR_HEAP_TYPE_DSV, numDescriptors };
	V_RETURN(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_dsvHeap)), cerr, 0);
	if (!m_name.empty()) m_dsvHeap->SetName((m_name + L".DsvHeap").c_str());

	return m_dsvHeap->GetCPUDescriptorHandleForHeapStart().ptr;
}

Descriptor DepthStencil_DX12::CreateDSV(const Descriptor& dsvHeapStart, uint32_t descriptorIdx,
	uint16_t arraySize, uint16_t firstArraySlice, Format format, uint8_t mipLevel,
	bool multisamples, bool readOnlyDepth, bool readOnlyStencil)
{
	// Setup the description of the depth stencil view.
	D3D12_DEPTH_STENCIL_VIEW_DESC desc = {};
	desc.Format = GetDXGIFormat(m_format);

	if (arraySize > 1 || firstArraySlice > 0)
	{
		if (multisamples)
		{
			desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
			desc.Texture2DMSArray.FirstArraySlice = firstArraySlice;
			desc.Texture2DMSArray.ArraySize = arraySize;
		}
		else
		{
			desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
			desc.Texture2DArray.FirstArraySlice = firstArraySlice;
			desc.Texture2DArray.ArraySize = arraySize;
			desc.Texture2DArray.MipSlice = mipLevel;
		}
	}
	else
	{
		if (multisamples)
			desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
		else
		{
			desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			desc.Texture2D.MipSlice = mipLevel;
		}
	}

	if (readOnlyDepth) desc.Flags |= D3D12_DSV_FLAG_READ_ONLY_DEPTH;
	if (readOnlyStencil) desc.Flags |= D3D12_DSV_FLAG_READ_ONLY_STENCIL;

	// Create a depth stencil view
	const auto stride = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	const auto descriptor = dsvHeapStart + stride * descriptorIdx;
	m_device->CreateDepthStencilView(m_resource.get(), &desc, { descriptor });

	return descriptor;
}

const Descriptor& DepthStencil_DX12::GetDSV(uint16_t slice, uint8_t mipLevel) const
{
	assert(m_dsvs.size() > slice && m_dsvs[slice].size() > mipLevel);
	return m_dsvs[slice][mipLevel];
}

const Descriptor& DepthStencil_DX12::GetReadOnlyDSV(uint16_t slice, uint8_t mipLevel) const
{
	assert(m_readOnlyDsvs.size() > slice && m_readOnlyDsvs[slice].size() > mipLevel);
	return m_readOnlyDsvs[slice][mipLevel];
}

const Descriptor& DepthStencil_DX12::GetStencilSRV() const
{
	return m_stencilSrv;
}

bool DepthStencil_DX12::create(const Device* pDevice, uint32_t width, uint32_t height, uint16_t arraySize,
	uint8_t numMips, uint8_t sampleCount, Format format, ResourceFlag resourceFlags, float clearDepth,
	uint8_t clearStencil, bool& hasSRV, Format& formatStencil, bool isCubeMap, MemoryFlag memoryFlags,
	const wchar_t* name, uint16_t srvComponentMapping, uint16_t stencilSrvComponentMapping,
	TextureLayout textureLayout, uint32_t maxThreads)
{
	assert(numMips >= 1);
	assert(sampleCount >= 1 && sampleCount <= 16);
	assert(sampleCount <= 1 || numMips <= 1);
	XUSG_N_RETURN(Initialize(pDevice, format), false);

	hasSRV = (resourceFlags & ResourceFlag::DENY_SHADER_RESOURCE) == ResourceFlag::NONE;

	// Map formats
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
			format = Format::R24G8_TYPELESS;
			formatDepth = Format::R24_UNORM_X8_TYPELESS;
		}
	}

	if (width && height && arraySize)
	{
		XUSG_N_RETURN(CreateResource(width, height, format, arraySize, resourceFlags,
			numMips, sampleCount, memoryFlags, ResourceState::DEPTH_WRITE, clearDepth,
			clearStencil, textureLayout, maxThreads), false);
	}

	SetName(name);

	// Allocate CBV SRV UAV heap
	uint32_t numDescriptors = 0;
	if (hasSRV) numDescriptors += numMips;
	if (formatStencil != Format::UNKNOWN) ++numDescriptors;
	const auto srvHeapStart = AllocateCbvSrvUavHeap(numDescriptors);
	auto descriptorIdx = 0u;

	if (hasSRV)
	{
		// Create SRV
		XUSG_N_RETURN(createSRVs(descriptorIdx, arraySize, formatDepth, numMips,
			sampleCount > 1, isCubeMap, srvComponentMapping), false);

		// Has stencil
		if (formatStencil != Format::UNKNOWN)
			XUSG_X_RETURN(m_stencilSrv, CreateSRV(srvHeapStart, descriptorIdx, arraySize, 0, formatStencil,
				numMips, 0, sampleCount > 1, isCubeMap, stencilSrvComponentMapping, 1), false);
	}

	return true;
}

//--------------------------------------------------------------------------------------
// 3D Texture
//--------------------------------------------------------------------------------------

Texture3D_DX12::Texture3D_DX12() :
	Texture_DX12()
{
}

Texture3D_DX12::~Texture3D_DX12()
{
}

bool Texture3D_DX12::Create(const Device* pDevice, uint32_t width, uint32_t height, uint16_t depth,
	Format format, ResourceFlag resourceFlags, uint8_t numMips, MemoryFlag memoryFlags,
	const wchar_t* name, uint16_t srvComponentMapping, TextureLayout textureLayout,
	uint32_t maxThreads)
{
	assert(numMips >= 1);
	XUSG_N_RETURN(Initialize(pDevice, format), false);

	const auto needPackedUAV = (resourceFlags & ResourceFlag::NEED_PACKED_UAV) == ResourceFlag::NEED_PACKED_UAV;
	const auto hasSRV = (resourceFlags & ResourceFlag::DENY_SHADER_RESOURCE) == ResourceFlag::NONE;
	const auto hasUAV = (resourceFlags & ResourceFlag::ALLOW_UNORDERED_ACCESS) == ResourceFlag::ALLOW_UNORDERED_ACCESS;

	// Map formats
	const auto formatUAV = needPackedUAV ? mapToPackedFormat(format) : format;

	if (width && height && depth)
		XUSG_N_RETURN(CreateResource(width, height, depth, format, resourceFlags, numMips,
			memoryFlags, ResourceState::COMMON, textureLayout, maxThreads), false);

	SetName(name);

	// Allocate CBV SRV UAV heap
	uint32_t numDescriptors = 0;
	if (hasSRV)
	{
		numDescriptors += numMips;
		if (hasUAV && numMips >= 1) numDescriptors += numMips;
	}
	if (hasUAV) 
	{
		numDescriptors += numMips;
		if (needPackedUAV) numDescriptors += numMips;
	}
	const auto srvUavHeapStart = AllocateCbvSrvUavHeap(numDescriptors);
	auto descriptorIdx = 0u;

	// Create SRVs
	if (hasSRV)
	{
		m_srvs.resize(numMips);
		for (uint8_t i = 0; i < numMips; ++i)
			XUSG_X_RETURN(m_srvs[i], CreateSRV(srvUavHeapStart, descriptorIdx + i,
				m_format, numMips - i, i, srvComponentMapping), false);
		descriptorIdx += numMips;
	}

	// Create UAVs
	if (hasUAV)
	{
		m_uavs.resize(numMips);
		for (uint8_t i = 0; i < numMips; ++i)
			XUSG_X_RETURN(m_uavs[i], CreateUAV(srvUavHeapStart, descriptorIdx + i, depth >> i, 0, m_format, i), false);
		descriptorIdx += numMips;

		if (needPackedUAV)
		{
			m_packedUavs.resize(numMips);
			for (uint8_t i = 0; i < numMips; ++i)
				XUSG_X_RETURN(m_packedUavs[i], CreateUAV(srvUavHeapStart, descriptorIdx + i, depth >> i, 0, formatUAV, i), false);
			descriptorIdx += numMips;
		}
	}

	// Create SRV for each level
	if (hasSRV && hasUAV)
	{
		m_srvLevels.resize(numMips);
		if (numMips <= 1 && !m_srvs.empty()) m_srvLevels[0] = m_srvs[0];
		else for (uint8_t i = 0; i < numMips; ++i)
			XUSG_X_RETURN(m_srvLevels[i], CreateSRV(srvUavHeapStart, descriptorIdx + i, m_format, 1, i, srvComponentMapping), false);
	}

	return true;
}

bool Texture3D_DX12::CreateResource(uint32_t width, uint32_t height, uint16_t depth,
	Format format, ResourceFlag resourceFlags, uint8_t numMips, MemoryFlag memoryFlags,
	ResourceState initialResourceState, TextureLayout textureLayout, uint32_t maxThreads)
{
	assert(numMips >= 1);
	m_hasPromotion = (resourceFlags & ResourceFlag::ALLOW_SIMULTANEOUS_ACCESS) == ResourceFlag::ALLOW_SIMULTANEOUS_ACCESS;

	// Setup the texture description.
	assert(width && height && depth);
	const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
	const auto desc = CD3DX12_RESOURCE_DESC::Tex3D(GetDXGIFormat(format), width, height, depth,
		numMips, GetDX12ResourceFlags(resourceFlags), GetDX12TextureLayout(textureLayout));

	// Determine initial state
	assert(maxThreads);
	m_states.resize(maxThreads);
	for (auto& states : m_states) states.resize(numMips, initialResourceState);

	V_RETURN(m_device->CreateCommittedResource(&heapProperties, GetDX12HeapFlags(memoryFlags), &desc,
		GetDX12ResourceStates(m_states[0][0]), nullptr, IID_PPV_ARGS(&m_resource)), clog, false);

	return true;
}

Descriptor Texture3D_DX12::CreateSRV(const Descriptor& srvHeapStart, uint32_t descriptorIdx,
	Format format, uint8_t numMips, uint8_t mostDetailedMip, uint16_t srvComponentMapping)
{
	// Setup the description of the shader resource view.
	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	assert(m_resource || format != Format::UNKNOWN);
	desc.Format = format != Format::UNKNOWN ? GetDXGIFormat(format) : m_resource->GetDesc().Format;
	desc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
		GetDX12ShaderComponentMapping(DECODE_SRV_COMPONENT_MAPPING(0, srvComponentMapping)),
		GetDX12ShaderComponentMapping(DECODE_SRV_COMPONENT_MAPPING(1, srvComponentMapping)),
		GetDX12ShaderComponentMapping(DECODE_SRV_COMPONENT_MAPPING(2, srvComponentMapping)),
		GetDX12ShaderComponentMapping(DECODE_SRV_COMPONENT_MAPPING(3, srvComponentMapping)));
	desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;

	desc.Texture3D.MipLevels = numMips;
	desc.Texture3D.MostDetailedMip = mostDetailedMip;

	// Create a shader resource view
	const auto stride = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	const auto descriptor = srvHeapStart + stride * descriptorIdx;
	m_device->CreateShaderResourceView(m_resource.get(), &desc, { descriptor });

	return descriptor;
}

Descriptor Texture3D_DX12::CreateUAV(const Descriptor& uavHeapStart, uint32_t descriptorIdx,
	uint16_t wSize, uint16_t firstWSlice, Format format, uint8_t mipLevel)
{
	// Setup the description of the unordered access view.
	D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
	assert(m_resource || format != Format::UNKNOWN);
	desc.Format = format != Format::UNKNOWN ? GetDXGIFormat(format) : m_resource->GetDesc().Format;
	desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;

	desc.Texture3D.MipSlice = mipLevel;
	desc.Texture3D.WSize = wSize;
	desc.Texture3D.FirstWSlice = firstWSlice;

	// Create an unordered access view
	const auto stride = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	const auto descriptor = uavHeapStart + stride * descriptorIdx;
	m_device->CreateUnorderedAccessView(m_resource.get(), nullptr, &desc, { descriptor });

	return descriptor;
}

uint16_t Texture3D_DX12::GetDepth() const
{
	return m_resource ? m_resource->GetDesc().DepthOrArraySize : 0;
}

//--------------------------------------------------------------------------------------
// Raw buffer
//--------------------------------------------------------------------------------------

Buffer_DX12::Buffer_DX12() :
	ShaderResource_DX12(),
	m_uavs(0),
	m_srvByteOffsets(0),
	m_pDataBegin(nullptr)
{
	m_hasPromotion = true;
}

Buffer_DX12::~Buffer_DX12()
{
	if (m_resource) Unmap();
}

bool Buffer_DX12::Create(const Device* pDevice, size_t byteWidth, ResourceFlag resourceFlags,
	MemoryType memoryType, uint32_t numSRVs, const uint32_t* firstSrvElements,
	uint32_t numUAVs, const uint32_t* firstUavElements, MemoryFlag memoryFlags,
	const wchar_t* name, const size_t* counterByteOffsets, uint32_t maxThreads)
{
	const uint32_t byteStride = sizeof(uint32_t);
	const auto bufferElements = static_cast<uint32_t>(byteWidth / byteStride);

	return create(pDevice, bufferElements, byteStride, Format::R32_TYPELESS, resourceFlags,
		memoryType, numSRVs, firstSrvElements, numUAVs, firstUavElements, memoryFlags,
		name, counterByteOffsets, maxThreads);
}

bool Buffer_DX12::CreateResource(size_t byteWidth, ResourceFlag resourceFlags, MemoryType memoryType,
	MemoryFlag memoryFlags, ResourceState initialResourceState, uint32_t maxThreads)
{
	const auto isRaytracingAS = (resourceFlags & ResourceFlag::ACCELERATION_STRUCTURE) == ResourceFlag::ACCELERATION_STRUCTURE;

	// Setup the buffer description.
	assert(byteWidth);
	const CD3DX12_HEAP_PROPERTIES heapProperties(GetDX12HeapType(memoryType));
	const auto desc = CD3DX12_RESOURCE_DESC::Buffer(byteWidth, GetDX12ResourceFlags(resourceFlags));

	// Determine initial state
	assert(maxThreads);
	m_states.resize(maxThreads);
	for (auto& states : m_states)
	{
		states.resize(1);
		switch (memoryType)
		{
		case MemoryType::UPLOAD:
			states[0] = ResourceState::GENERAL_READ;
			break;
		case MemoryType::READBACK:
			states[0] = ResourceState::COPY_DEST;
			break;
		default:
			if (isRaytracingAS) states[0] = ResourceState::RAYTRACING_ACCELERATION_STRUCTURE;
			else states[0] = initialResourceState;
		}
	}

	V_RETURN(m_device->CreateCommittedResource(&heapProperties, GetDX12HeapFlags(memoryFlags), &desc,
		GetDX12ResourceStates(m_states[0][0]), nullptr, IID_PPV_ARGS(&m_resource)), clog, false);

	return true;
}

bool Buffer_DX12::Upload(CommandList* pCommandList, Resource* pUploader, const void* pData,
	size_t size, size_t offset, ResourceState dstState, uint32_t threadIdx)
{
	// Create the GPU upload buffer.
	assert(pUploader);
	auto& uploaderResource = dynamic_cast<Resource_DX12*>(pUploader)->GetResource();
	if (!uploaderResource)
	{
		const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
		const auto desc = CD3DX12_RESOURCE_DESC::Buffer(size);

		V_RETURN(m_device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploaderResource)), clog, false);
		if (!m_name.empty()) uploaderResource->SetName((m_name + L".UploaderResource").c_str());
	}

	// Copy data to the intermediate upload heap and then schedule a copy 
	// from the upload heap to the buffer.
	void* pUploadData;
	V_RETURN(uploaderResource->Map(0, nullptr, &pUploadData), clog, false);
	memcpy(pUploadData, pData, size);
	uploaderResource->Unmap(0, nullptr);

	ResourceBarrier barrier;
	auto numBarriers = SetBarrier(&barrier, ResourceState::COPY_DEST, 0, XUSG_BARRIER_ALL_SUBRESOURCES,
		BarrierFlag::NONE, ResourceState::AUTO, threadIdx);
	pCommandList->Barrier(numBarriers, &barrier);

	const auto pGraphicsCommandList = static_cast<ID3D12GraphicsCommandList*>(pCommandList->GetHandle());
	pGraphicsCommandList->CopyBufferRegion(m_resource.get(), offset, uploaderResource.get(), 0, size);

	numBarriers = SetBarrier(&barrier, dstState, 0, XUSG_BARRIER_ALL_SUBRESOURCES,
		BarrierFlag::NONE, ResourceState::AUTO, threadIdx);
	pCommandList->Barrier(numBarriers, &barrier);

	return true;
}

bool Buffer_DX12::Upload(CommandList* pCommandList, uint32_t descriptorIndex, Resource* pUploader,
	const void* pData, size_t size, ResourceState dstState, uint32_t threadIdx)
{
	const auto offset = m_srvByteOffsets.empty() ? 0 : m_srvByteOffsets[descriptorIndex];

	return Upload(pCommandList, pUploader, pData, size, offset, dstState, threadIdx);
}

bool Buffer_DX12::ReadBack(CommandList* pCommandList, Buffer* pReadBuffer, size_t size,
	size_t dstOffset, size_t srcOffset, ResourceState dstState, uint32_t threadIdx)
{
	const auto readSize = size ? size : static_cast<size_t>(GetWidth());

	// Create the GPU read-back buffer.
	assert(pReadBuffer);
	if (!pReadBuffer->GetHandle())
		XUSG_N_RETURN(pReadBuffer->Create(pCommandList->GetDevice(), readSize + dstOffset, ResourceFlag::DENY_SHADER_RESOURCE,
			MemoryType::READBACK, 0, nullptr, 0, nullptr, MemoryFlag::NONE, (m_name + L".ReadResource").c_str()), false);

	auto& readResource = dynamic_cast<Resource_DX12*>(pReadBuffer)->GetResource();
	assert(readResource);

	ResourceBarrier barrier;
	dstState = dstState == ResourceState::COMMON ? m_states[threadIdx][0] : dstState;
	auto numBarriers = SetBarrier(&barrier, ResourceState::COPY_SOURCE, 0,
		XUSG_BARRIER_ALL_SUBRESOURCES, BarrierFlag::NONE, ResourceState::AUTO, threadIdx);
	pCommandList->Barrier(numBarriers, &barrier);

	const auto pGraphicsCommandList = static_cast<ID3D12GraphicsCommandList*>(pCommandList->GetHandle());
	pGraphicsCommandList->CopyBufferRegion(readResource.get(), dstOffset, m_resource.get(), srcOffset, readSize);

	numBarriers = SetBarrier(&barrier, dstState, 0, XUSG_BARRIER_ALL_SUBRESOURCES,
		BarrierFlag::NONE, ResourceState::AUTO, threadIdx);
	pCommandList->Barrier(numBarriers, &barrier);

	return true;
}

Descriptor Buffer_DX12::CreateSRV(const Descriptor& srvHeapStart, uint32_t descriptorIdx, uint32_t numElements,
	uint32_t byteStride, Format format, uint32_t firstElement, uint16_t srvComponentMapping)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	desc.Shader4ComponentMapping = srvComponentMapping;

	const auto stride = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	const auto descriptor = srvHeapStart + stride * descriptorIdx;
	if (m_resource && m_states[0][0] == ResourceState::RAYTRACING_ACCELERATION_STRUCTURE)
	{
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
		desc.RaytracingAccelerationStructure.Location = m_resource->GetGPUVirtualAddress();
		desc.RaytracingAccelerationStructure.Location += static_cast<uint64_t>(byteStride) * firstElement;

		// Create a shader resource view
		m_device->CreateShaderResourceView(nullptr, &desc, { descriptor });
	}
	else
	{
		desc.Format = GetDXGIFormat(format);
		desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		desc.Buffer.FirstElement = firstElement;
		desc.Buffer.NumElements = numElements;
		desc.Buffer.StructureByteStride = format == Format::UNKNOWN ? byteStride : 0;
		desc.Buffer.Flags = format == Format::R32_TYPELESS ? D3D12_BUFFER_SRV_FLAG_RAW : D3D12_BUFFER_SRV_FLAG_NONE;

		// Create a shader resource view
		m_device->CreateShaderResourceView(m_resource.get(), &desc, { descriptor });
	}

	return descriptor;
}

Descriptor Buffer_DX12::CreateUAV(const Descriptor& uavHeapStart, uint32_t descriptorIdx, uint32_t numElements,
	uint32_t byteStride, Format format, uint32_t firstElement, size_t counterByteOffset)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
	desc.Format = GetDXGIFormat(format);
	desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	desc.Buffer.FirstElement = firstElement;
	desc.Buffer.NumElements = numElements;
	desc.Buffer.StructureByteStride = format == Format::UNKNOWN ? byteStride : 0;
	desc.Buffer.CounterOffsetInBytes = counterByteOffset;
	desc.Buffer.Flags = format == Format::R32_TYPELESS ? D3D12_BUFFER_UAV_FLAG_RAW : D3D12_BUFFER_UAV_FLAG_NONE;

	// Create an unordered access view
	const auto stride = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	const auto descriptor = uavHeapStart + stride * descriptorIdx;
	const auto counterResource = m_counter ? static_cast<ID3D12Resource*>(m_counter->GetHandle()) : nullptr;
	m_device->CreateUnorderedAccessView(m_resource.get(), counterResource, &desc, { descriptor });

	return descriptor;
}

const Descriptor& Buffer_DX12::GetUAV(uint32_t index) const
{
	assert(m_uavs.size() > index);
	return m_uavs[index];
}

void* Buffer_DX12::Map(uint32_t descriptorIndex, uintptr_t readBegin, uintptr_t readEnd)
{
	const Range range(readBegin, readEnd);

	return Map(&range, descriptorIndex);
}

void* Buffer_DX12::Map(const Range* pReadRange, uint32_t descriptorIndex)
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

	const auto offset = !descriptorIndex ? 0 : m_srvByteOffsets[descriptorIndex];

	return &reinterpret_cast<uint8_t*>(m_pDataBegin)[offset];
}

void Buffer_DX12::Unmap()
{
	if (m_pDataBegin)
	{
		m_resource->Unmap(0, nullptr);
		m_pDataBegin = nullptr;
	}
}

void Buffer_DX12::SetCounter(const Resource::sptr& counter)
{
	m_counter = counter;
}

Resource::sptr Buffer_DX12::GetCounter() const
{
	return m_counter;
}

bool Buffer_DX12::create(const Device* pDevice, uint32_t numElements, uint32_t byteStride, Format format,
	ResourceFlag resourceFlags, MemoryType memoryType, uint32_t numSRVs, const uint32_t* firstSrvElements,
	uint32_t numUAVs, const uint32_t* firstUavElements, MemoryFlag memoryFlags, const wchar_t* name,
	const size_t* counterByteOffsets, uint32_t maxThreads)
{
	XUSG_N_RETURN(Initialize(pDevice, format), false);

	const auto hasSRV = (resourceFlags & ResourceFlag::DENY_SHADER_RESOURCE) == ResourceFlag::NONE;
	const auto hasUAV = (resourceFlags & ResourceFlag::ALLOW_UNORDERED_ACCESS) == ResourceFlag::ALLOW_UNORDERED_ACCESS;

	numSRVs = hasSRV ? numSRVs : 0;
	numUAVs = hasUAV ? numUAVs : 0;

	// Create buffer
	if (numElements)
		XUSG_N_RETURN(CreateResource(static_cast<size_t>(byteStride) * numElements, resourceFlags,
			memoryType, memoryFlags, ResourceState::COMMON, maxThreads), false);

	SetName(name);

	// Allocate CBV SRV UAV heap
	const auto numDescriptors = numSRVs + numUAVs;
	const auto srvUavHeapStart = AllocateCbvSrvUavHeap(numDescriptors);
	auto descriptorIdx = 0u;

	// Create SRVs
	if (numSRVs)
	{
		m_srvByteOffsets.resize(numSRVs);
		m_srvs.resize(numSRVs);
		for (auto i = 0u; i < numSRVs; ++i)
		{
			uint32_t srvNumElements, firstElement;
			getViewRange(srvNumElements, firstElement, i, numElements, byteStride, firstSrvElements, numSRVs, &m_srvByteOffsets[i]);
			XUSG_X_RETURN(m_srvs[i], CreateSRV(srvUavHeapStart, descriptorIdx + i, srvNumElements,
				byteStride, format, firstElement), false);
		}
		descriptorIdx += numSRVs;
	}

	// Create UAVs
	if (numUAVs)
	{
		m_uavs.resize(numUAVs);
		for (auto i = 0u; i < numUAVs; ++i)
		{
			uint32_t uavNumElements, firstElement;
			getViewRange(uavNumElements, firstElement, i, numElements, byteStride, firstUavElements, numUAVs);
			XUSG_X_RETURN(m_uavs[i], CreateUAV(srvUavHeapStart, descriptorIdx + i, uavNumElements, byteStride,
				format, firstElement, counterByteOffsets ? counterByteOffsets[i] : 0), false);
		}
	}

	return true;
}

void Buffer_DX12::getViewRange(uint32_t& viewElements, uint32_t& firstElement, uint32_t i,
	uint32_t bufferElements, uint32_t byteStride, const uint32_t* firstElements,
	uint32_t numDescriptors, size_t* pByteOffset)
{
	firstElement = firstElements ? firstElements[i] : 0;
	viewElements = (!firstElements || i + 1 >= numDescriptors ?
		bufferElements : firstElements[i + 1]) - firstElement;

	if (pByteOffset) *pByteOffset = static_cast<size_t>(byteStride) * firstElement;
}

//--------------------------------------------------------------------------------------
// Structured buffer
//--------------------------------------------------------------------------------------

StructuredBuffer_DX12::StructuredBuffer_DX12() :
	Buffer_DX12()
{
}

StructuredBuffer_DX12::~StructuredBuffer_DX12()
{
}

bool StructuredBuffer_DX12::Create(const Device* pDevice, uint32_t numElements, uint32_t byteStride,
	ResourceFlag resourceFlags, MemoryType memoryType, uint32_t numSRVs, const uint32_t* firstSrvElements,
	uint32_t numUAVs, const uint32_t* firstUavElements, MemoryFlag memoryFlags, const wchar_t* name,
	const size_t* counterByteOffsets, uint32_t maxThreads)
{
	return create(pDevice, numElements, byteStride, Format::UNKNOWN, resourceFlags,
		memoryType, numSRVs, firstSrvElements, numUAVs, firstUavElements, memoryFlags,
		name, counterByteOffsets, maxThreads);
}

bool StructuredBuffer_DX12::Initialize(const Device* pDevice)
{
	return Buffer_DX12::Initialize(pDevice, Format::UNKNOWN);
}

//--------------------------------------------------------------------------------------
// Typed buffer
//--------------------------------------------------------------------------------------

TypedBuffer_DX12::TypedBuffer_DX12() :
	Buffer_DX12()
{
}

TypedBuffer_DX12::~TypedBuffer_DX12()
{
}

bool TypedBuffer_DX12::Create(const Device* pDevice, uint32_t numElements, uint32_t byteStride,
	Format format, ResourceFlag resourceFlags, MemoryType memoryType, uint32_t numSRVs,
	const uint32_t* firstSrvElements, uint32_t numUAVs, const uint32_t* firstUavElements,
	MemoryFlag memoryFlags, const wchar_t* name, uint16_t srvComponentMapping,
	const size_t* counterByteOffsets, uint32_t maxThreads)
{
	XUSG_N_RETURN(Initialize(pDevice, format), false);

	const auto needPackedUAV = (resourceFlags & ResourceFlag::NEED_PACKED_UAV) == ResourceFlag::NEED_PACKED_UAV;
	const auto hasSRV = (resourceFlags & ResourceFlag::DENY_SHADER_RESOURCE) == ResourceFlag::NONE;
	const auto hasUAV = (resourceFlags & ResourceFlag::ALLOW_UNORDERED_ACCESS) == ResourceFlag::ALLOW_UNORDERED_ACCESS;

	numSRVs = hasSRV ? numSRVs : 0;
	numUAVs = hasUAV ? numUAVs : 0;

	// Map formats
	const auto formatUAV = needPackedUAV ? mapToPackedFormat(format) : format;

	// Create buffer
	if (numElements)
		XUSG_N_RETURN(CreateResource(static_cast<size_t>(byteStride) * numElements, resourceFlags,
			memoryType, memoryFlags, ResourceState::COMMON, maxThreads), false);

	SetName(name);

	// Allocate CBV SRV UAV heap
	auto numDescriptors = numSRVs + numUAVs;
	if (needPackedUAV) numDescriptors += numUAVs;
	const auto srvUavHeapStart = AllocateCbvSrvUavHeap(numDescriptors);
	auto descriptorIdx = 0u;

	// Create SRVs
	if (numSRVs)
	{
		m_srvByteOffsets.resize(numSRVs);
		m_srvs.resize(numSRVs);
		for (auto i = 0u; i < numSRVs; ++i)
		{
			uint32_t srvNumElements, firstElement;
			getViewRange(srvNumElements, firstElement, i, numElements, byteStride, firstSrvElements, numSRVs, &m_srvByteOffsets[i]);
			XUSG_X_RETURN(m_srvs[i], CreateSRV(srvUavHeapStart, descriptorIdx + i, srvNumElements,
				byteStride, m_format, firstElement, srvComponentMapping), false);
		}
		descriptorIdx += numSRVs;
	}

	// Create UAVs
	if (numUAVs)
	{
		m_uavs.resize(numUAVs);
		for (auto i = 0u; i < numUAVs; ++i)
		{
			uint32_t uavNumElements, firstElement;
			getViewRange(uavNumElements, firstElement, i, numElements, byteStride, firstUavElements, numUAVs);
			XUSG_X_RETURN(m_uavs[i], CreateUAV(srvUavHeapStart, descriptorIdx + i, uavNumElements, byteStride,
				m_format, firstElement, counterByteOffsets ? counterByteOffsets[i] : 0), false);
		}
		descriptorIdx += numUAVs;

		if (needPackedUAV)
		{
			m_packedUavs.resize(numUAVs);
			for (auto i = 0u; i < numUAVs; ++i)
			{
				uint32_t uavNumElements, firstElement;
				getViewRange(uavNumElements, firstElement, i, numElements, byteStride, firstUavElements, numUAVs);
				XUSG_X_RETURN(m_packedUavs[i], CreateUAV(srvUavHeapStart, descriptorIdx + i, uavNumElements,
					byteStride, formatUAV, firstElement, counterByteOffsets ? counterByteOffsets[i] : 0), false);
			}
		}
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

bool VertexBuffer_DX12::Create(const Device* pDevice, uint32_t numVertices, uint32_t byteStride,
	ResourceFlag resourceFlags, MemoryType memoryType, uint32_t numVBVs,
	const uint32_t* firstVertices, uint32_t numSRVs, const uint32_t* firstSrvElements,
	uint32_t numUAVs, const uint32_t* firstUavElements, MemoryFlag memoryFlags,
	const wchar_t* name, const size_t* counterByteOffsets, uint32_t maxThreads)
{
	const auto hasSRV = (resourceFlags & ResourceFlag::DENY_SHADER_RESOURCE) == ResourceFlag::NONE;
	const auto hasUAV = (resourceFlags & ResourceFlag::ALLOW_UNORDERED_ACCESS) == ResourceFlag::ALLOW_UNORDERED_ACCESS;

	XUSG_N_RETURN(StructuredBuffer_DX12::Create(pDevice, numVertices, byteStride, resourceFlags,
		memoryType, numSRVs, firstSrvElements, numUAVs, firstUavElements, memoryFlags, name,
		counterByteOffsets, maxThreads), false);

	// Create vertex buffer views.
	createVBVs(numVertices, byteStride, numVBVs, firstVertices);

	return true;
}

bool VertexBuffer_DX12::CreateAsRaw(const Device* pDevice, uint32_t numVertices, uint32_t byteStride,
	ResourceFlag resourceFlags, MemoryType memoryType, uint32_t numVBVs,
	const uint32_t* firstVertices, uint32_t numSRVs, const uint32_t* firstSrvElements,
	uint32_t numUAVs, const uint32_t* firstUavElements, MemoryFlag memoryFlags,
	const wchar_t* name, const size_t* counterByteOffsets, uint32_t maxThreads)
{
	const auto hasSRV = (resourceFlags & ResourceFlag::DENY_SHADER_RESOURCE) == ResourceFlag::NONE;
	const auto hasUAV = (resourceFlags & ResourceFlag::ALLOW_UNORDERED_ACCESS) == ResourceFlag::ALLOW_UNORDERED_ACCESS;

	XUSG_N_RETURN(Buffer_DX12::Create(pDevice, static_cast<size_t>(byteStride) * numVertices,
		resourceFlags, memoryType, numSRVs, firstSrvElements, numUAVs, firstUavElements,
		memoryFlags, name, counterByteOffsets, maxThreads), false);

	// Create vertex buffer view
	createVBVs(numVertices, byteStride, numVBVs, firstVertices);

	return true;
}

void VertexBuffer_DX12::CreateVBV(VertexBufferView& vbv, uint32_t numVertices, uint32_t byteStride, uint32_t firstVertex)
{
	vbv.BufferLocation = m_resource->GetGPUVirtualAddress() + byteStride * firstVertex;
	vbv.StrideInBytes = byteStride;
	vbv.SizeInBytes = byteStride * numVertices;
}

const VertexBufferView& VertexBuffer_DX12::GetVBV(uint32_t index) const
{
	assert(m_vbvs.size() > index);
	return m_vbvs[index];
}

void VertexBuffer_DX12::createVBVs(uint32_t numVertices, uint32_t byteStride, uint32_t numVBVs, const uint32_t* firstVertices)
{
	m_vbvs.resize(numVBVs);
	for (auto i = 0u; i < numVBVs; ++i)
	{
		uint32_t vbvNumVertices, firstVertex;
		getViewRange(vbvNumVertices, firstVertex, i, numVertices, byteStride, firstVertices, numVBVs);
		CreateVBV(m_vbvs[i], vbvNumVertices, byteStride, firstVertex);
	}
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
	const size_t* ibvByteOffsets, uint32_t numSRVs, const uint32_t* firstSrvElements,
	uint32_t numUAVs, const uint32_t* firstUavElements, MemoryFlag memoryFlags,
	const wchar_t* name, uint16_t srvComponentMapping,
	const size_t* counterByteOffsets, uint32_t maxThreads)
{
	const auto hasSRV = (resourceFlags & ResourceFlag::DENY_SHADER_RESOURCE) == ResourceFlag::NONE;
	const auto hasUAV = (resourceFlags & ResourceFlag::ALLOW_UNORDERED_ACCESS) == ResourceFlag::ALLOW_UNORDERED_ACCESS;

	assert(format == Format::R32_UINT || format == Format::R16_UINT);
	const uint32_t byteStride = format == Format::R32_UINT ? sizeof(uint32_t) : sizeof(uint16_t);

	const auto numElements = static_cast<uint32_t>(byteWidth / byteStride);
	XUSG_N_RETURN(TypedBuffer_DX12::Create(pDevice, numElements, byteStride, format, resourceFlags,
		memoryType, numSRVs, firstSrvElements, numUAVs, firstUavElements, memoryFlags, name,
		srvComponentMapping, counterByteOffsets, maxThreads), false);

	// Create index buffer views.
	m_ibvs.resize(numIBVs);
	for (auto i = 0u; i < numIBVs; ++i)
	{
		const auto byteOffset = ibvByteOffsets ? ibvByteOffsets[i] : 0;
		const auto byteSize = static_cast<uint32_t>((!ibvByteOffsets || i + 1 >= numIBVs ?
			byteWidth : ibvByteOffsets[i + 1]) - byteOffset);
		CreateIBV(m_ibvs[i], format, byteSize, byteOffset);
	}

	return true;
}

void IndexBuffer_DX12::CreateIBV(IndexBufferView& ibv, Format format, uint32_t byteSize, size_t byteOffset)
{
	ibv.BufferLocation = m_resource->GetGPUVirtualAddress() + byteOffset;
	ibv.SizeInBytes = byteSize;
	ibv.Format = format;
}

const IndexBufferView& IndexBuffer_DX12::GetIBV(uint32_t index) const
{
	assert(m_ibvs.size() > index);
	return m_ibvs[index];
}
