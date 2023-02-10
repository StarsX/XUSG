//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSG_DX12.h"
#include "Core/XUSGGraphicsState_DX12.h"
#include "Core/XUSGEnum_DX12.h"
#include "XUSGMeshShaderState_DX12.h"

using namespace std;
using namespace XUSG;
using namespace MeshShader;

State_DX12::State_DX12()
{
	// Default state
	m_key.resize(sizeof(Key));
	m_pKey = reinterpret_cast<Key*>(&m_key[0]);
	memset(m_pKey, 0, sizeof(Key));
	m_pKey->SampleMask = UINT32_MAX;
	m_pKey->PrimTopologyType = PrimitiveTopologyType::TRIANGLE;
	m_pKey->SampleCount = 1;
}

State_DX12::~State_DX12()
{
}

void State_DX12::SetPipelineLayout(const PipelineLayout& layout)
{
	m_pKey->Layout = layout;
}

void State_DX12::SetShader(Shader::Stage stage, const Blob& shader)
{
	const auto stageIdx = stage > Shader::NUM_GRAPHICS ? stage - Shader::NUM_GRAPHICS : stage;
	m_pKey->Shaders[stageIdx] = shader;
}

void State_DX12::SetCachedPipeline(const Blob& cachedPipeline)
{
	m_pKey->CachedPipeline = cachedPipeline;
}

void State_DX12::SetNodeMask(uint32_t nodeMask)
{
	m_pKey->NodeMask = nodeMask;
}

void State_DX12::OMSetBlendState(const Graphics::Blend* pBlend, uint32_t sampleMask)
{
	m_pKey->pBlend = pBlend;
	m_pKey->SampleMask = sampleMask;
}

void State_DX12::RSSetState(const Graphics::Rasterizer* pRasterizer)
{
	m_pKey->pRasterizer = pRasterizer;
}

void State_DX12::DSSetState(const DepthStencil* pDepthStencil)
{
	m_pKey->pDepthStencil = pDepthStencil;
}

void State_DX12::OMSetBlendState(BlendPreset preset, PipelineLib* pPipelineLib,
	uint8_t numColorRTs, uint32_t sampleMask)
{
	OMSetBlendState(pPipelineLib->GetBlend(preset, numColorRTs), sampleMask);
}

void State_DX12::RSSetState(RasterizerPreset preset, PipelineLib* pPipelineLib)
{
	RSSetState(pPipelineLib->GetRasterizer(preset));
}

void State_DX12::DSSetState(DepthStencilPreset preset, PipelineLib* pPipelineLib)
{
	DSSetState(pPipelineLib->GetDepthStencil(preset));
}

void State_DX12::OMSetNumRenderTargets(uint8_t n)
{
	m_pKey->NumRenderTargets = n;

	for (auto i = n; i < 8; ++i)
		OMSetRTVFormat(i, Format::UNKNOWN);
}

void State_DX12::OMSetRTVFormat(uint8_t i, Format format)
{
	m_pKey->RTVFormats[i] = format;
}

void State_DX12::OMSetRTVFormats(const Format* formats, uint8_t n)
{
	OMSetNumRenderTargets(n);

	for (auto i = 0u; i < n; ++i)
		OMSetRTVFormat(i, formats[i]);
}

void State_DX12::OMSetDSVFormat(Format format)
{
	m_pKey->DSVFormat = format;
}

void State_DX12::OMSetSample(uint8_t count, uint8_t quality)
{
	m_pKey->SampleCount = count;
	m_pKey->SampleQuality = quality;
}

Pipeline State_DX12::CreatePipeline(PipelineLib* pPipelineLib, const wchar_t* name) const
{
	return pPipelineLib->CreatePipeline(this, name);
}

Pipeline State_DX12::GetPipeline(PipelineLib* pPipelineLib, const wchar_t* name) const
{
	return pPipelineLib->GetPipeline(this, name);
}

const string& State_DX12::GetKey() const
{
	return m_key;
}

//--------------------------------------------------------------------------------------

PipelineLib_DX12::PipelineLib_DX12() :
	Graphics::PipelineLib_DX12()
{
}

PipelineLib_DX12::PipelineLib_DX12(const Device* pDevice) :
	Graphics::PipelineLib_DX12(pDevice)
{
}

PipelineLib_DX12::~PipelineLib_DX12()
{
}

Pipeline PipelineLib_DX12::CreatePipeline(const State* pState, const wchar_t* name)
{
	return createPipeline(pState->GetKey(), name);
}

Pipeline PipelineLib_DX12::GetPipeline(const State* pState, const wchar_t* name)
{
	return getPipeline(pState->GetKey(), name);
}

Pipeline PipelineLib_DX12::createPipeline(const string& key, const wchar_t* name)
{
	// Fill desc
	const auto pKey = reinterpret_cast<const Key*>(key.data());
	D3DX12_MESH_SHADER_PIPELINE_STATE_DESC psoDesc = {};
	if (pKey->Layout)
		psoDesc.pRootSignature = static_cast<ID3D12RootSignature*>(pKey->Layout);
	if (pKey->Shaders[AS])
		psoDesc.AS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pKey->Shaders[AS]));
	if (pKey->Shaders[MS])
		psoDesc.MS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pKey->Shaders[MS]));
	if (pKey->Shaders[PS])
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pKey->Shaders[PS]));

	// Blend state
	const auto pBlend = pKey->pBlend ? pKey->pBlend : GetBlend(BlendPreset::DEFAULT_OPAQUE);
	psoDesc.BlendState.AlphaToCoverageEnable = pBlend->AlphaToCoverageEnable ? TRUE : FALSE;
	psoDesc.BlendState.IndependentBlendEnable = pBlend->IndependentBlendEnable ? TRUE : FALSE;
	for (uint8_t i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
	{
		const auto& src = pBlend->RenderTargets[i];
		auto& dst = psoDesc.BlendState.RenderTarget[i];
		dst.BlendEnable = src.BlendEnable ? TRUE : FALSE;
		dst.LogicOpEnable = src.LogicOpEnable ? TRUE : FALSE;
		dst.SrcBlend = GetDX12Blend(src.SrcBlend);
		dst.DestBlend = GetDX12Blend(src.DestBlend);
		dst.BlendOp = GetDX12BlendOp(src.BlendOp);
		dst.SrcBlendAlpha = GetDX12Blend(src.SrcBlendAlpha);
		dst.DestBlendAlpha = GetDX12Blend(src.DestBlendAlpha);
		dst.BlendOpAlpha = GetDX12BlendOp(src.BlendOpAlpha);
		dst.LogicOp = GetDX12LogicOp(src.LogicOp);
		dst.RenderTargetWriteMask = GetDX12ColorWrite(src.WriteMask);
	}
	psoDesc.SampleMask = pKey->SampleMask;

	// Rasterizer state
	const auto pRasterizer = pKey->pRasterizer ? pKey->pRasterizer : GetRasterizer(RasterizerPreset::CULL_BACK);
	psoDesc.RasterizerState.FillMode = GetDX12FillMode(pRasterizer->Fill);
	psoDesc.RasterizerState.CullMode = GetDX12CullMode(pRasterizer->Cull);
	psoDesc.RasterizerState.FrontCounterClockwise = pRasterizer->FrontCounterClockwise;
	psoDesc.RasterizerState.DepthBias = pRasterizer->DepthBias;
	psoDesc.RasterizerState.DepthBiasClamp = pRasterizer->DepthBiasClamp;
	psoDesc.RasterizerState.SlopeScaledDepthBias = pRasterizer->SlopeScaledDepthBias;
	psoDesc.RasterizerState.DepthClipEnable = pRasterizer->DepthClipEnable;
	psoDesc.RasterizerState.MultisampleEnable = pRasterizer->MultisampleEnable;
	psoDesc.RasterizerState.AntialiasedLineEnable = pRasterizer->AntialiasedLineEnable;
	psoDesc.RasterizerState.ForcedSampleCount = pRasterizer->ForcedSampleCount;
	psoDesc.RasterizerState.ConservativeRaster = pRasterizer->ConservativeRaster ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	// Depth-stencil state
	const auto pDepthStencil = pKey->pDepthStencil ? pKey->pDepthStencil : GetDepthStencil(DepthStencilPreset::DEFAULT_LESS);
	psoDesc.DepthStencilState.DepthEnable = pDepthStencil->DepthEnable ? TRUE : FALSE;
	psoDesc.DepthStencilState.DepthWriteMask = pDepthStencil->DepthWriteMask ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
	psoDesc.DepthStencilState.DepthFunc = GetDX12ComparisonFunc(pDepthStencil->Comparison);
	psoDesc.DepthStencilState.StencilEnable = pDepthStencil->StencilEnable ? TRUE : FALSE;
	psoDesc.DepthStencilState.StencilReadMask = pDepthStencil->StencilReadMask;
	psoDesc.DepthStencilState.StencilWriteMask = pDepthStencil->StencilWriteMask;
	psoDesc.DepthStencilState.FrontFace.StencilFailOp = GetDX12StencilOp(pDepthStencil->FrontFace.StencilFailOp);
	psoDesc.DepthStencilState.FrontFace.StencilDepthFailOp = GetDX12StencilOp(pDepthStencil->FrontFace.StencilDepthFailOp);
	psoDesc.DepthStencilState.FrontFace.StencilPassOp = GetDX12StencilOp(pDepthStencil->FrontFace.StencilPassOp);
	psoDesc.DepthStencilState.FrontFace.StencilFunc = GetDX12ComparisonFunc(pDepthStencil->FrontFace.StencilFunc);
	psoDesc.DepthStencilState.BackFace.StencilFailOp = GetDX12StencilOp(pDepthStencil->BackFace.StencilFailOp);
	psoDesc.DepthStencilState.BackFace.StencilDepthFailOp = GetDX12StencilOp(pDepthStencil->BackFace.StencilDepthFailOp);
	psoDesc.DepthStencilState.BackFace.StencilPassOp = GetDX12StencilOp(pDepthStencil->BackFace.StencilPassOp);
	psoDesc.DepthStencilState.BackFace.StencilFunc = GetDX12ComparisonFunc(pDepthStencil->BackFace.StencilFunc);

	psoDesc.PrimitiveTopologyType = GetDX12PrimitiveTopologyType(pKey->PrimTopologyType);
	psoDesc.NumRenderTargets = pKey->NumRenderTargets;

	for (auto i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		psoDesc.RTVFormats[i] = GetDXGIFormat(pKey->RTVFormats[i]);
	psoDesc.DSVFormat = GetDXGIFormat(pKey->DSVFormat);
	psoDesc.SampleDesc.Count = pKey->SampleCount;
	psoDesc.SampleDesc.Quality = pKey->SampleQuality;

	const auto pCachedPipeline = static_cast<ID3DBlob*>(pKey->CachedPipeline);
	psoDesc.CachedPSO.pCachedBlob = pCachedPipeline ? pCachedPipeline->GetBufferPointer() : nullptr;
	psoDesc.CachedPSO.CachedBlobSizeInBytes = pCachedPipeline ? pCachedPipeline->GetBufferSize() : 0;
	psoDesc.NodeMask = pKey->NodeMask;

	CD3DX12_PIPELINE_MESH_STATE_STREAM psoStream(psoDesc);
	D3D12_PIPELINE_STATE_STREAM_DESC streamDesc;
	streamDesc.pPipelineStateSubobjectStream = &psoStream;
	streamDesc.SizeInBytes = sizeof(psoStream);

	// Create pipeline
	com_ptr<ID3D12PipelineState> pipeline;
	com_ptr<ID3D12Device2> dxDevice;
	V_RETURN(m_device->QueryInterface(IID_PPV_ARGS(&dxDevice)), cerr, nullptr);
	V_RETURN(dxDevice->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&pipeline)), cerr, nullptr);
	if (name) pipeline->SetName(name);
	m_pipelines[key] = pipeline;

	return pipeline.get();
}
