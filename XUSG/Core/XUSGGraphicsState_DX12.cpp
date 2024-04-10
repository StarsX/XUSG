//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSG_DX12.h"
#include "XUSGGraphicsState_DX12.h"
#include "XUSGEnum_DX12.h"

using namespace std;
using namespace XUSG;
using namespace Graphics;

State_DX12::State_DX12()
{
	// Default state
	m_key.resize(sizeof(PipelineDesc));
	m_pKey = reinterpret_cast<PipelineDesc*>(&m_key[0]);
	memset(m_pKey, 0, sizeof(PipelineDesc));
	m_pKey->SampleMask = UINT_MAX;
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
	m_pKey->Shaders[stage] = shader;
}

void State_DX12::SetCachedPipeline(const Blob& cachedPipeline)
{
	m_pKey->CachedPipeline = cachedPipeline;
}

void State_DX12::SetNodeMask(uint32_t nodeMask)
{
	m_pKey->NodeMask = nodeMask;
}

void State_DX12::SetFlags(PipelineFlag flag)
{
	m_pKey->Flags = flag;
}

void State_DX12::OMSetBlendState(const Blend* pBlend, uint32_t sampleMask)
{
	m_pKey->pBlend = pBlend;
	m_pKey->SampleMask = sampleMask;
}

void State_DX12::RSSetState(const Rasterizer* pRasterizer)
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

void State_DX12::IASetInputLayout(const InputLayout* pLayout)
{
	m_pKey->pInputLayout = pLayout;
}

void State_DX12::IASetPrimitiveTopologyType(PrimitiveTopologyType type)
{
	m_pKey->PrimTopologyType = type;
}

void State_DX12::IASetIndexBufferStripCutValue(IBStripCutValue ibStripCutValue)
{
	m_pKey->IBStripCutValue = static_cast<uint8_t>(ibStripCutValue);
}

void State_DX12::OMSetNumRenderTargets(uint8_t n)
{
	const auto numRTVFormats = static_cast<uint8_t>(size(m_pKey->RTVFormats));
	assert(n <= numRTVFormats);
	m_pKey->NumRenderTargets = n;

	for (auto i = n; i < numRTVFormats; ++i) OMSetRTVFormat(i, Format::UNKNOWN);
}

void State_DX12::OMSetRTVFormat(uint8_t i, Format format)
{
	assert(i < size(m_pKey->RTVFormats));
	m_pKey->RTVFormats[i] = format;
}

void State_DX12::OMSetRTVFormats(const Format* formats, uint8_t n)
{
	OMSetNumRenderTargets(n);

	for (uint8_t i = 0; i < n; ++i) OMSetRTVFormat(i, formats[i]);
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
	m_pfnRasterizers[RasterizerPreset::FILL_WIREFRAME] = FillWireframe;

	// Depth stencil states
	m_pfnDepthStencils[DepthStencilPreset::DEFAULT_LESS] = DepthStencilDefault;
	m_pfnDepthStencils[DepthStencilPreset::DEPTH_STENCIL_NONE] = DepthStencilNone;
	m_pfnDepthStencils[DepthStencilPreset::DEPTH_READ_LESS] = DepthRead;
	m_pfnDepthStencils[DepthStencilPreset::DEPTH_READ_LESS_EQUAL] = DepthReadLessEqual;
	m_pfnDepthStencils[DepthStencilPreset::DEPTH_READ_EQUAL] = DepthReadEqual;
}

PipelineLib_DX12::PipelineLib_DX12(const Device* pDevice) :
	PipelineLib_DX12()
{
	SetDevice(pDevice);
}

PipelineLib_DX12::~PipelineLib_DX12()
{
}

void PipelineLib_DX12::SetDevice(const Device* pDevice)
{
	m_device = pDevice->GetHandle();
	assert(m_device);
}

void PipelineLib_DX12::SetPipeline(const string& key, const Pipeline& pipeline)
{
	m_pipelines[key] = pipeline;
}

void PipelineLib_DX12::SetInputLayout(uint32_t index, const InputElement* pElements, uint32_t numElements)
{
	m_inputLayoutLib.SetLayout(index, pElements, numElements);
}

const InputLayout* PipelineLib_DX12::GetInputLayout(uint32_t index) const
{
	return m_inputLayoutLib.GetLayout(index);
}

const InputLayout* PipelineLib_DX12::CreateInputLayout(const InputElement* pElements, uint32_t numElements)
{
	return m_inputLayoutLib.CreateLayout(pElements, numElements);
}

Pipeline PipelineLib_DX12::CreatePipeline(const State* pState, const wchar_t* name)
{
	return createPipeline(pState->GetKey(), name);
}

Pipeline PipelineLib_DX12::GetPipeline(const State* pState, const wchar_t* name)
{
	return getPipeline(pState->GetKey(), name);
}

const Blend* PipelineLib_DX12::GetBlend(BlendPreset preset, uint8_t numColorRTs)
{
	if (m_blends[preset] == nullptr)
		m_blends[preset] = make_unique<Blend>(m_pfnBlends[preset](numColorRTs));

	return m_blends[preset].get();
}

const Rasterizer* PipelineLib_DX12::GetRasterizer(RasterizerPreset preset)
{
	if (m_rasterizers[preset] == nullptr)
		m_rasterizers[preset] = make_unique<Rasterizer>(m_pfnRasterizers[preset]());

	return m_rasterizers[preset].get();
}

const Graphics::DepthStencil* PipelineLib_DX12::GetDepthStencil(DepthStencilPreset preset)
{
	if (m_depthStencils[preset] == nullptr)
		m_depthStencils[preset] = make_unique<DepthStencil>(m_pfnDepthStencils[preset]());

	return m_depthStencils[preset].get();
}

Pipeline PipelineLib_DX12::createPipeline(const std::string& key, const wchar_t* name)
{
	// Fill desc
	const auto pDesc = reinterpret_cast<const PipelineDesc*>(key.data());
	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
	if (pDesc->Layout)
		desc.pRootSignature = static_cast<ID3D12RootSignature*>(pDesc->Layout);

	if (pDesc->Shaders[Shader::Stage::VS])
		desc.VS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pDesc->Shaders[Shader::Stage::VS]));
	if (pDesc->Shaders[Shader::Stage::PS])
		desc.PS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pDesc->Shaders[Shader::Stage::PS]));
	if (pDesc->Shaders[Shader::Stage::DS])
		desc.DS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pDesc->Shaders[Shader::Stage::DS]));
	if (pDesc->Shaders[Shader::Stage::HS])
		desc.HS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pDesc->Shaders[Shader::Stage::HS]));
	if (pDesc->Shaders[Shader::Stage::GS])
		desc.GS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pDesc->Shaders[Shader::Stage::GS]));

	// Blend state
	const auto pBlend = pDesc->pBlend ? pDesc->pBlend : GetBlend(BlendPreset::DEFAULT_OPAQUE);
	desc.BlendState.AlphaToCoverageEnable = pBlend->AlphaToCoverageEnable ? TRUE : FALSE;
	desc.BlendState.IndependentBlendEnable = pBlend->IndependentBlendEnable ? TRUE : FALSE;
	for (uint8_t i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
	{
		const auto& src = pBlend->RenderTargets[i];
		auto& dst = desc.BlendState.RenderTarget[i];
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
	desc.SampleMask = pDesc->SampleMask;

	// Rasterizer state
	const auto pRasterizer = pDesc->pRasterizer ? pDesc->pRasterizer : GetRasterizer(RasterizerPreset::CULL_BACK);
	desc.RasterizerState.FillMode = GetDX12FillMode(pRasterizer->Fill);
	desc.RasterizerState.CullMode = GetDX12CullMode(pRasterizer->Cull);
	desc.RasterizerState.FrontCounterClockwise = pRasterizer->FrontCounterClockwise ? TRUE : FALSE;
	desc.RasterizerState.DepthBias = pRasterizer->DepthBias;
	desc.RasterizerState.DepthBiasClamp = pRasterizer->DepthBiasClamp;
	desc.RasterizerState.SlopeScaledDepthBias = pRasterizer->SlopeScaledDepthBias;
	desc.RasterizerState.DepthClipEnable = pRasterizer->DepthClipEnable ? TRUE : FALSE;
	desc.RasterizerState.MultisampleEnable = pRasterizer->MultisampleEnable ? TRUE : FALSE;
	desc.RasterizerState.AntialiasedLineEnable = pRasterizer->AntialiasedLineEnable ? TRUE : FALSE;
	desc.RasterizerState.ForcedSampleCount = pRasterizer->ForcedSampleCount;
	desc.RasterizerState.ConservativeRaster = pRasterizer->ConservativeRaster ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	// Depth-stencil state
	const auto pDepthStencil = pDesc->pDepthStencil ? pDesc->pDepthStencil : GetDepthStencil(DepthStencilPreset::DEFAULT_LESS);
	desc.DepthStencilState.DepthEnable = pDepthStencil->DepthEnable ? TRUE : FALSE;
	desc.DepthStencilState.DepthWriteMask = pDepthStencil->DepthWriteMask ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
	desc.DepthStencilState.DepthFunc = GetDX12ComparisonFunc(pDepthStencil->Comparison);
	desc.DepthStencilState.StencilEnable = pDepthStencil->StencilEnable ? TRUE : FALSE;
	desc.DepthStencilState.StencilReadMask = pDepthStencil->StencilReadMask;
	desc.DepthStencilState.StencilWriteMask = pDepthStencil->StencilWriteMask;
	desc.DepthStencilState.FrontFace.StencilFailOp = GetDX12StencilOp(pDepthStencil->FrontFace.StencilFailOp);
	desc.DepthStencilState.FrontFace.StencilDepthFailOp = GetDX12StencilOp(pDepthStencil->FrontFace.StencilDepthFailOp);
	desc.DepthStencilState.FrontFace.StencilPassOp = GetDX12StencilOp(pDepthStencil->FrontFace.StencilPassOp);
	desc.DepthStencilState.FrontFace.StencilFunc = GetDX12ComparisonFunc(pDepthStencil->FrontFace.StencilFunc);
	desc.DepthStencilState.BackFace.StencilFailOp = GetDX12StencilOp(pDepthStencil->BackFace.StencilFailOp);
	desc.DepthStencilState.BackFace.StencilDepthFailOp = GetDX12StencilOp(pDepthStencil->BackFace.StencilDepthFailOp);
	desc.DepthStencilState.BackFace.StencilPassOp = GetDX12StencilOp(pDepthStencil->BackFace.StencilPassOp);
	desc.DepthStencilState.BackFace.StencilFunc = GetDX12ComparisonFunc(pDepthStencil->BackFace.StencilFunc);

	// Input layout
	vector<D3D12_INPUT_ELEMENT_DESC> inputElements;
	if (pDesc->pInputLayout)
	{
		desc.InputLayout.NumElements = static_cast<uint32_t>(pDesc->pInputLayout->size());
		inputElements.resize(desc.InputLayout.NumElements);

		for (auto i = 0u; i < desc.InputLayout.NumElements; ++i)
		{
			const auto& src = pDesc->pInputLayout->at(i);
			auto& dst = inputElements[i];
			dst.SemanticName = src.SemanticName;
			dst.SemanticIndex = src.SemanticIndex;
			dst.Format = GetDXGIFormat(src.Format);
			dst.InputSlot = src.InputSlot;
			dst.AlignedByteOffset = src.AlignedByteOffset != XUSG_APPEND_ALIGNED_ELEMENT ? src.AlignedByteOffset : D3D12_APPEND_ALIGNED_ELEMENT;
			dst.InputSlotClass = GetDX12InputClassification(src.InputSlotClass);
			dst.InstanceDataStepRate = src.InstanceDataStepRate;
		}

		desc.InputLayout.pInputElementDescs = inputElements.data();
	}

	desc.PrimitiveTopologyType = GetDX12PrimitiveTopologyType(pDesc->PrimTopologyType);
	desc.NumRenderTargets = pDesc->NumRenderTargets;

	for (uint8_t i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		desc.RTVFormats[i] = GetDXGIFormat(pDesc->RTVFormats[i]);
	desc.DSVFormat = GetDXGIFormat(pDesc->DSVFormat);
	desc.SampleDesc.Count = pDesc->SampleCount;
	desc.SampleDesc.Quality = pDesc->SampleQuality;

	const auto pCachedPipeline = static_cast<ID3DBlob*>(pDesc->CachedPipeline);
	desc.CachedPSO.pCachedBlob = pCachedPipeline ? pCachedPipeline->GetBufferPointer() : nullptr;
	desc.CachedPSO.CachedBlobSizeInBytes = pCachedPipeline ? pCachedPipeline->GetBufferSize() : 0;
	desc.NodeMask = pDesc->NodeMask;
	desc.Flags = GetDX12PipelineFlags(pDesc->Flags);

	switch (static_cast<IBStripCutValue>(pDesc->IBStripCutValue))
	{
	case IBStripCutValue::FFFF:
		desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFF;
		break;
	case IBStripCutValue::FFFFFFFF:
		desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF;
		break;
	default:
		assert(desc.IBStripCutValue == D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED);
	}

	// Create pipeline
	com_ptr<ID3D12PipelineState> pipeline;
	V_RETURN(m_device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipeline)), cerr, nullptr);
	if (name) pipeline->SetName(name);
	m_pipelines[key] = pipeline;

	return pipeline.get();
}

Pipeline PipelineLib_DX12::getPipeline(const string& key, const wchar_t* name)
{
	const auto pPipeline = m_pipelines.find(key);

	// Create one, if it does not exist
	if (pPipeline == m_pipelines.end()) return createPipeline(key, name);

	return pPipeline->second.get();
}
