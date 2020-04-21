//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSGGraphicsState_DX12.h"
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
	m_pKey->PrimTopologyType = PrimitiveTopologyType::TRIANGLE;
	m_pKey->SampleCount = 1;
}

State_DX12::~State_DX12()
{
}

void State_DX12::SetPipelineLayout(const PipelineLayout& layout)
{
	m_pKey->PipelineLayout = layout.get();
}

void State_DX12::SetShader(Shader::Stage stage, Blob shader)
{
	const auto stageIdx = stage > Shader::NUM_GRAPHICS ? stage - Shader::NUM_GRAPHICS : stage;
	m_pKey->Shaders[stageIdx] = shader.get();
}

void State_DX12::OMSetBlendState(const Graphics::Blend& blend)
{
	m_pKey->Blend = blend.get();
}

void State_DX12::RSSetState(const Graphics::Rasterizer& rasterizer)
{
	m_pKey->Rasterizer = rasterizer.get();
}

void State_DX12::DSSetState(const DepthStencil& depthStencil)
{
	m_pKey->DepthStencil = depthStencil.get();
}

void State_DX12::OMSetBlendState(BlendPreset preset, PipelineCache& pipelineCache, uint8_t numColorRTs)
{
	OMSetBlendState(pipelineCache.GetBlend(preset, numColorRTs));
}

void State_DX12::RSSetState(RasterizerPreset preset, PipelineCache& pipelineCache)
{
	RSSetState(pipelineCache.GetRasterizer(preset));
}

void State_DX12::DSSetState(DepthStencilPreset preset, PipelineCache& pipelineCache)
{
	DSSetState(pipelineCache.GetDepthStencil(preset));
}

void State_DX12::OMSetNumRenderTargets(uint8_t n)
{
	m_pKey->NumRenderTargets = n;
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

Pipeline State_DX12::CreatePipeline(PipelineCache& pipelineCache, const wchar_t* name) const
{
	return pipelineCache.CreatePipeline(*this, name);
}

Pipeline State_DX12::GetPipeline(PipelineCache& pipelineCache, const wchar_t* name) const
{
	return pipelineCache.GetPipeline(*this, name);
}

const string& State_DX12::GetKey() const
{
	return m_key;
}

//--------------------------------------------------------------------------------------

PipelineCache_DX12::PipelineCache_DX12() :
	Graphics::PipelineCache_DX12()
{
}

PipelineCache_DX12::PipelineCache_DX12(const Device& device) :
	Graphics::PipelineCache_DX12(device)
{
}

PipelineCache_DX12::~PipelineCache_DX12()
{
}

Pipeline PipelineCache_DX12::CreatePipeline(const State& state, const wchar_t* name)
{
	return createPipeline(reinterpret_cast<const Key*>(state.GetKey().data()), name);
}

Pipeline PipelineCache_DX12::GetPipeline(const State& state, const wchar_t* name)
{
	return getPipeline(state.GetKey(), name);
}

Pipeline PipelineCache_DX12::createPipeline(const Key* pKey, const wchar_t* name)
{
	// Fill desc
	D3DX12_MESH_SHADER_PIPELINE_STATE_DESC psoDesc = {};
	if (pKey->PipelineLayout)
		psoDesc.pRootSignature = static_cast<decltype(psoDesc.pRootSignature)>(pKey->PipelineLayout);
	if (pKey->Shaders[AS])
		psoDesc.AS = Shader::ByteCode(static_cast<BlobType*>(pKey->Shaders[AS]));
	if (pKey->Shaders[MS])
		psoDesc.MS = Shader::ByteCode(static_cast<BlobType*>(pKey->Shaders[MS]));
	if (pKey->Shaders[PS])
		psoDesc.PS = Shader::ByteCode(static_cast<BlobType*>(pKey->Shaders[PS]));

	const auto blend = static_cast<decltype(psoDesc.BlendState)*>(pKey->Blend);
	psoDesc.BlendState = *(blend ? blend : GetBlend(BlendPreset::DEFAULT_OPAQUE).get());
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.SampleDesc = DefaultSampleDesc();

	const auto rasterizer = static_cast<decltype(psoDesc.RasterizerState)*>(pKey->Rasterizer);
	const auto depthStencil = static_cast<decltype(psoDesc.DepthStencilState)*>(pKey->DepthStencil);
	psoDesc.RasterizerState = *(rasterizer ? rasterizer : GetRasterizer(RasterizerPreset::CULL_BACK).get());
	psoDesc.DepthStencilState = *(depthStencil ? depthStencil : GetDepthStencil(DepthStencilPreset::DEFAULT_LESS).get());
	psoDesc.PrimitiveTopologyType = GetDX12PrimitiveTopologyType(pKey->PrimTopologyType);
	psoDesc.NumRenderTargets = pKey->NumRenderTargets;

	for (auto i = 0; i < 8; ++i)
		psoDesc.RTVFormats[i] = GetDXGIFormat(pKey->RTVFormats[i]);
	psoDesc.DSVFormat = GetDXGIFormat(pKey->DSVFormat);
	psoDesc.SampleDesc.Count = pKey->SampleCount;

	CD3DX12_PIPELINE_MESH_STATE_STREAM psoStream(psoDesc);
	D3D12_PIPELINE_STATE_STREAM_DESC streamDesc;
	streamDesc.pPipelineStateSubobjectStream = &psoStream;
	streamDesc.SizeInBytes = sizeof(psoStream);

	// Create pipeline
	Pipeline pipeline;
	com_ptr<ID3D12Device2> dxDevice;
	V_RETURN(m_device->QueryInterface(IID_PPV_ARGS(&dxDevice)), cerr, nullptr);
	V_RETURN(dxDevice->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&pipeline)), cerr, nullptr);
	if (name) pipeline->SetName(name);

	return pipeline;
}

Pipeline PipelineCache_DX12::getPipeline(const string& key, const wchar_t* name)
{
	const auto pPipeline = m_pipelines.find(key);

	// Create one, if it does not exist
	if (pPipeline == m_pipelines.end())
	{
		const auto pKey = reinterpret_cast<const Key*>(key.data());
		const auto pipeline = createPipeline(pKey, name);
		m_pipelines[key] = pipeline;

		return pipeline;
	}

	return pPipeline->second;
}
