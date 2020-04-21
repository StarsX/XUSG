//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGGraphicsState_DX12.h"
#include "XUSGEnum_DX12.h"

using namespace std;
using namespace XUSG;
using namespace Graphics;

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
	m_pKey->PipelineLayout = layout.get();
}

void State_DX12::SetShader(Shader::Stage stage, Blob shader)
{
	m_pKey->Shaders[stage] = shader.get();
}

void State_DX12::SetCachedPipeline(const void* pCachedBlob, size_t size)
{
	m_pKey->CachedPipeline = pCachedBlob;
	m_pKey->CachedPipelineSize = size;
}

void State_DX12::SetNodeMask(uint32_t nodeMask)
{
	m_pKey->NodeMask = nodeMask;
}

void State_DX12::OMSetBlendState(const Blend& blend, uint32_t sampleMask)
{
	m_pKey->Blend = blend.get();
	m_pKey->SampleMask = sampleMask;
}

void State_DX12::RSSetState(const Rasterizer& rasterizer)
{
	m_pKey->Rasterizer = rasterizer.get();
}

void State_DX12::DSSetState(const DepthStencil& depthStencil)
{
	m_pKey->DepthStencil = depthStencil.get();
}

void State_DX12::OMSetBlendState(BlendPreset preset, PipelineCache& pipelineCache,
	uint8_t numColorRTs, uint32_t sampleMask)
{
	OMSetBlendState(pipelineCache.GetBlend(preset, numColorRTs), sampleMask);
}

void State_DX12::RSSetState(RasterizerPreset preset, PipelineCache& pipelineCache)
{
	RSSetState(pipelineCache.GetRasterizer(preset));
}

void State_DX12::DSSetState(DepthStencilPreset preset, PipelineCache& pipelineCache)
{
	DSSetState(pipelineCache.GetDepthStencil(preset));
}

void State_DX12::IASetInputLayout(const InputLayout& layout)
{
	m_pKey->InputLayout = layout.get();
}

void State_DX12::IASetPrimitiveTopologyType(PrimitiveTopologyType type)
{
	m_pKey->PrimTopologyType = type;
}

void State_DX12::IASetIndexBufferStripCutValue(uint32_t ibStripCutValue)
{
	assert(ibStripCutValue == 0 || ibStripCutValue == 0xffff || ibStripCutValue == 0xffffffff);
	m_pKey->IBStripCutValue = ibStripCutValue;
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

void State_DX12::OMSetSample(uint8_t count, uint8_t quality)
{
	m_pKey->SampleCount = count;
	m_pKey->SampleQuality = quality;
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
	m_device(nullptr),
	m_pipelines(),
	m_blends(),
	m_rasterizers(),
	m_depthStencils()
{
	// Blend states
	m_pfnBlends[BlendPreset::DEFAULT_OPAQUE] = DefaultOpaque;
	m_pfnBlends[BlendPreset::PREMULTIPLITED] = Premultiplied;
	m_pfnBlends[BlendPreset::ADDTIVE] = Additive;
	m_pfnBlends[BlendPreset::NON_PRE_MUL] = NonPremultiplied;
	m_pfnBlends[BlendPreset::NON_PREMUL_RT0] = NonPremultipliedRT0;
	m_pfnBlends[BlendPreset::ALPHA_TO_COVERAGE] = AlphaToCoverage;
	m_pfnBlends[BlendPreset::ACCUMULATIVE] = Accumulative;
	m_pfnBlends[BlendPreset::AUTO_NON_PREMUL] = AutoNonPremultiplied;
	m_pfnBlends[BlendPreset::ZERO_ALPHA_PREMUL] = ZeroAlphaNonPremultiplied;
	m_pfnBlends[BlendPreset::MULTIPLITED] = Multiplied;
	m_pfnBlends[BlendPreset::WEIGHTED_PREMUL] = WeightedPremul;
	m_pfnBlends[BlendPreset::WEIGHTED_PREMUL_PER_RT] = WeightedPremulPerRT;
	m_pfnBlends[BlendPreset::WEIGHTED_PER_RT] = WeightedPerRT;
	m_pfnBlends[BlendPreset::SELECT_MIN] = SelectMin;
	m_pfnBlends[BlendPreset::SELECT_MAX] = SelectMax;

	// Rasterizer states
	m_pfnRasterizers[RasterizerPreset::CULL_BACK] = CullBack;
	m_pfnRasterizers[RasterizerPreset::CULL_NONE] = CullNone;
	m_pfnRasterizers[RasterizerPreset::CULL_FRONT] = CullFront;
	m_pfnRasterizers[RasterizerPreset::FILL_WIREFRAME] = CullWireframe;

	// Depth stencil states
	m_pfnDepthStencils[DepthStencilPreset::DEFAULT_LESS] = DepthStencilDefault;
	m_pfnDepthStencils[DepthStencilPreset::DEPTH_STENCIL_NONE] = DepthStencilNone;
	m_pfnDepthStencils[DepthStencilPreset::DEPTH_READ_LESS] = DepthRead;
	m_pfnDepthStencils[DepthStencilPreset::DEPTH_READ_LESS_EQUAL] = DepthReadLessEqual;
	m_pfnDepthStencils[DepthStencilPreset::DEPTH_READ_EQUAL] = DepthReadEqual;
}

PipelineCache_DX12::PipelineCache_DX12(const Device& device) :
	PipelineCache_DX12()
{
	SetDevice(device);
}

PipelineCache_DX12::~PipelineCache_DX12()
{
}

void PipelineCache_DX12::SetDevice(const Device& device)
{
	m_device = device;
}

void PipelineCache_DX12::SetPipeline(const string& key, const Pipeline& pipeline)
{
	m_pipelines[key] = pipeline;
}

void PipelineCache_DX12::SetInputLayout(uint32_t index, const InputElementTable& elementTable)
{
	m_inputLayoutPool.SetLayout(index, elementTable);
}

InputLayout PipelineCache_DX12::GetInputLayout(uint32_t index) const
{
	return m_inputLayoutPool.GetLayout(index);
}

InputLayout PipelineCache_DX12::CreateInputLayout(const InputElementTable& elementTable)
{
	return m_inputLayoutPool.CreateLayout(elementTable);
}

Pipeline PipelineCache_DX12::CreatePipeline(const State& state, const wchar_t* name)
{
	return createPipeline(reinterpret_cast<const Key*>(state.GetKey().data()), name);
}

Pipeline PipelineCache_DX12::GetPipeline(const State& state, const wchar_t* name)
{
	return getPipeline(state.GetKey(), name);
}

const Blend& PipelineCache_DX12::GetBlend(BlendPreset preset, uint8_t numColorRTs)
{
	if (m_blends[preset] == nullptr)
		m_blends[preset] = m_pfnBlends[preset](numColorRTs);

	return m_blends[preset];
}

const Rasterizer& PipelineCache_DX12::GetRasterizer(RasterizerPreset preset)
{
	if (m_rasterizers[preset] == nullptr)
		m_rasterizers[preset] = m_pfnRasterizers[preset]();

	return m_rasterizers[preset];
}

const Graphics::DepthStencil& PipelineCache_DX12::GetDepthStencil(DepthStencilPreset preset)
{
	if (m_depthStencils[preset] == nullptr)
		m_depthStencils[preset] = m_pfnDepthStencils[preset]();

	return m_depthStencils[preset];
}

Pipeline PipelineCache_DX12::createPipeline(const Key* pKey, const wchar_t* name)
{
	// Fill desc
	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
	if (pKey->PipelineLayout)
		desc.pRootSignature = static_cast<decltype(desc.pRootSignature)>(pKey->PipelineLayout);

	if (pKey->Shaders[Shader::Stage::VS])
		desc.VS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pKey->Shaders[Shader::Stage::VS]));
	if (pKey->Shaders[Shader::Stage::PS])
		desc.PS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pKey->Shaders[Shader::Stage::PS]));
	if (pKey->Shaders[Shader::Stage::DS])
		desc.DS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pKey->Shaders[Shader::Stage::DS]));
	if (pKey->Shaders[Shader::Stage::HS])
		desc.HS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pKey->Shaders[Shader::Stage::HS]));
	if (pKey->Shaders[Shader::Stage::GS])
		desc.GS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pKey->Shaders[Shader::Stage::GS]));

	const auto blend = static_cast<const D3D12_BLEND_DESC*>(pKey->Blend);
	desc.BlendState = *(blend ? blend : GetBlend(BlendPreset::DEFAULT_OPAQUE).get());
	desc.SampleMask = pKey->SampleMask;

	const auto rasterizer = static_cast<const D3D12_RASTERIZER_DESC*>(pKey->Rasterizer);
	const auto depthStencil = static_cast<const D3D12_DEPTH_STENCIL_DESC*>(pKey->DepthStencil);
	desc.RasterizerState = *(rasterizer ? rasterizer : GetRasterizer(RasterizerPreset::CULL_BACK).get());
	desc.RasterizerState.MultisampleEnable = pKey->SampleCount > 1 ? TRUE : FALSE;
	desc.DepthStencilState = *(depthStencil ? depthStencil : GetDepthStencil(DepthStencilPreset::DEFAULT_LESS).get());
	if (pKey->InputLayout) desc.InputLayout = *static_cast<const D3D12_INPUT_LAYOUT_DESC*>(pKey->InputLayout);
	desc.PrimitiveTopologyType = GetDX12PrimitiveTopologyType(pKey->PrimTopologyType);
	desc.NumRenderTargets = pKey->NumRenderTargets;

	for (auto i = 0; i < 8; ++i)
		desc.RTVFormats[i] = GetDXGIFormat(pKey->RTVFormats[i]);
	desc.DSVFormat = GetDXGIFormat(pKey->DSVFormat);
	desc.SampleDesc.Count = pKey->SampleCount;
	desc.SampleDesc.Quality = pKey->SampleQuality;

	desc.CachedPSO.pCachedBlob = pKey->CachedPipeline;
	desc.CachedPSO.CachedBlobSizeInBytes = pKey->CachedPipelineSize;
	desc.NodeMask = pKey->NodeMask;

	switch (pKey->IBStripCutValue)
	{
	case 0xffff:
		desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFF;
		break;
	case 0xffffffff:
		desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFF;
		break;
	default:
		assert(desc.IBStripCutValue == D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED);
	}

	// Create pipeline
	Pipeline pipeline;
	V_RETURN(m_device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipeline)), cerr, nullptr);
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
