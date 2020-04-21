//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSGCommand_DX12.h"
#include "XUSGUltimate_DX12.h"

using namespace std;
using namespace XUSG::Ultimate;

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
