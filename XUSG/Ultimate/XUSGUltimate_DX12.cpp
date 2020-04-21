//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSGCommand_DX12.h"
#include "Core/XUSGResource_DX12.h"
#include "Core/XUSGEnum_DX12.h"
#include "XUSGUltimate_DX12.h"

using namespace std;
using namespace XUSG::Ultimate;

//--------------------------------------------------------------------------------------
// Command list
//--------------------------------------------------------------------------------------

CommandList_DX12::CommandList_DX12() :
	XUSG::CommandList_DX12()
{
	if (m_commandList)
		m_commandList->QueryInterface(IID_PPV_ARGS(&m_commandListU));
}

CommandList_DX12::CommandList_DX12(XUSG::CommandList& commandList) :
	XUSG::CommandList_DX12()
{
	m_commandList = dynamic_cast<XUSG::CommandList_DX12&>(commandList).GetGraphicsCommandList();
	CreateInterface();
}

CommandList_DX12::~CommandList_DX12()
{
}

bool CommandList_DX12::CreateInterface()
{
	if (!m_commandList) return false;
	const auto hr = m_commandList->QueryInterface(IID_PPV_ARGS(&m_commandListU));

	if (FAILED(hr))
	{
		OutputDebugString(L"Couldn't get DirectX ultimate interface for the command list.\n");

		return false;
	}

	return true;
}

void CommandList_DX12::SetSamplePositions(uint8_t numSamplesPerPixel, uint8_t numPixels, SamplePosition* pPositions) const
{
	m_commandListU->SetSamplePositions(numSamplesPerPixel, numPixels, reinterpret_cast<D3D12_SAMPLE_POSITION*>(pPositions));
}

void CommandList_DX12::ResolveSubresourceRegion(const Resource& dstResource, uint32_t dstSubresource, uint32_t dstX, uint32_t dstY,
	const Resource& srcResource, uint32_t srcSubresource, RectRange* pSrcRect, Format format, ResolveMode resolveMode) const
{
	m_commandListU->ResolveSubresourceRegion(dstResource.get(), dstSubresource, dstX, dstY,
		srcResource.get(), srcSubresource, reinterpret_cast<D3D12_RECT*>(pSrcRect),
		GetDXGIFormat(format), GetDX12ResolveMode(resolveMode));
}

void CommandList_DX12::RSSetShadingRate(ShadingRate baseShadingRate, const ShadingRateCombiner* pCombiners) const
{
	D3D12_SHADING_RATE_COMBINER combiners[D3D12_RS_SET_SHADING_RATE_COMBINER_COUNT];
	for (auto i = 0ui8; i < D3D12_RS_SET_SHADING_RATE_COMBINER_COUNT; ++i)
		combiners[i] = GetDX12ShadingRateCombiner(pCombiners[i]);
	m_commandListU->RSSetShadingRate(D3D12_SHADING_RATE(baseShadingRate), combiners);
}

void CommandList_DX12::RSSetShadingRateImage(const Resource& shadingRateImage)  const
{
	m_commandListU->RSSetShadingRateImage(shadingRateImage.get());
}

void CommandList_DX12::DispatchMesh(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)  const
{
	m_commandListU->DispatchMesh(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}

XUSG::com_ptr<ID3D12GraphicsCommandList6>& CommandList_DX12::GetGraphicsCommandList()
{
	return m_commandListU;
}

//--------------------------------------------------------------------------------------
// Sampler feedback
//--------------------------------------------------------------------------------------

SamplerFeedBack_DX12::SamplerFeedBack_DX12() :
	XUSG::Texture2D_DX12()
{
}

SamplerFeedBack_DX12::~SamplerFeedBack_DX12()
{
}

bool SamplerFeedBack_DX12::Create(const Device& device, const Texture2D& target, Format format,
	uint32_t mipRegionWidth, uint32_t mipRegionHeight, uint32_t mipRegionDepth,
	ResourceFlag resourceFlags, MemoryType memoryType, bool isCubeMap, const wchar_t* name)
{
	M_RETURN(!device, cerr, "The device is NULL.", false);
	setDevice(device);

	if (name) m_name = name;

	const auto hasSRV = (resourceFlags & ResourceFlag::DENY_SHADER_RESOURCE) == ResourceFlag::NONE;

	// Get paired properties
	const auto arraySize = target.GetArraySize();
	const auto numMips = target.GetNumMips();

	// Setup the texture description.
	assert(format == Format::MIN_MIP_OPAQUE || format == Format::MIP_REGION_USED_OPAQUE);
	const CD3DX12_HEAP_PROPERTIES heapProperties(GetDX12HeapType(memoryType));
	const auto desc = CD3DX12_RESOURCE_DESC1::Tex2D(GetDXGIFormat(format),
		target.GetWidth(), target.GetHeight(), arraySize, numMips, 1, 0,
		GetDX12ResourceFlags(ResourceFlag::ALLOW_UNORDERED_ACCESS | resourceFlags),
		D3D12_TEXTURE_LAYOUT_UNKNOWN, 0, mipRegionWidth, mipRegionHeight, mipRegionDepth);

	// Determine initial state
	m_states.resize(arraySize * numMips);
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
		for (auto& state : m_states)
			state = ResourceState::COMMON;
	}

	com_ptr<ID3D12Device8> dxDevice;
	V_RETURN(m_device->QueryInterface(IID_PPV_ARGS(&dxDevice)), cerr, false);
	V_RETURN(dxDevice->CreateCommittedResource2(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc,
		GetDX12ResourceStates(m_states[0]), nullptr, nullptr,  IID_PPV_ARGS(&m_resource)), clog, false);
	if (!m_name.empty()) m_resource->SetName((m_name + L".Resource").c_str());

	// Create SRV
	if (hasSRV) N_RETURN(CreateSRVs(arraySize, format, numMips, 1, isCubeMap), false);

	// Create UAVs
	N_RETURN(CreateUAV(target.GetResource()), false);

	return true;
}

bool XUSG::Ultimate::SamplerFeedBack_DX12::CreateUAV(const Resource& target)
{
	return false;
}

//--------------------------------------------------------------------------------------
// DX12 enum transfer functions
//--------------------------------------------------------------------------------------

D3D12_SHADING_RATE_COMBINER XUSG::Ultimate::GetDX12ShadingRateCombiner(ShadingRateCombiner combiner)
{
	static const D3D12_SHADING_RATE_COMBINER combiners[] =
	{
		D3D12_SHADING_RATE_COMBINER_PASSTHROUGH,
		D3D12_SHADING_RATE_COMBINER_OVERRIDE,
		D3D12_SHADING_RATE_COMBINER_MIN,
		D3D12_SHADING_RATE_COMBINER_MAX,
		D3D12_SHADING_RATE_COMBINER_SUM
	};

	return combiners[static_cast<uint32_t>(combiner)];
}

D3D12_RESOLVE_MODE XUSG::Ultimate::GetDX12ResolveMode(ResolveMode mode)
{
	static const D3D12_RESOLVE_MODE modes[] =
	{
		D3D12_RESOLVE_MODE_DECOMPRESS,
		D3D12_RESOLVE_MODE_MIN,
		D3D12_RESOLVE_MODE_MAX,
		D3D12_RESOLVE_MODE_AVERAGE,
		D3D12_RESOLVE_MODE_ENCODE_SAMPLER_FEEDBACK,
		D3D12_RESOLVE_MODE_DECODE_SAMPLER_FEEDBACK
	};

	return modes[static_cast<uint32_t>(mode)];
}
