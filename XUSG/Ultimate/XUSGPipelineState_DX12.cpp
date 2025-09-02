//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSG_DX12.h"
#include "Core/XUSGCommand_DX12.h"
#include "Core/XUSGResource_DX12.h"
#include "Core/XUSGPipelineLayout_DX12.h"
#include "Core/XUSGEnum_DX12.h"
#include "XUSGPipelineState_DX12.h"
#include "XUSGUltimate_DX12.h"

using namespace std;
using namespace XUSG;
using namespace Ultimate;

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

void State_DX12::OMSetBlendState(const Graphics::Blend* pBlend, uint32_t sampleMask)
{
	m_pKey->pBlend = pBlend;
	m_pKey->SampleMask = sampleMask;
}

void State_DX12::RSSetState(const Graphics::Rasterizer* pRasterizer)
{
	m_pKey->pRasterizer = pRasterizer;
}

void State_DX12::DSSetState(const Graphics::DepthStencil* pDepthStencil)
{
	m_pKey->pDepthStencil = pDepthStencil;
}

void State_DX12::OMSetBlendState(Graphics::BlendPreset preset, PipelineLib* pPipelineLib,
	uint8_t numColorRTs, uint32_t sampleMask)
{
	OMSetBlendState(pPipelineLib->GetBlend(preset, numColorRTs), sampleMask);
}

void State_DX12::RSSetState(Graphics::RasterizerPreset preset, PipelineLib* pPipelineLib)
{
	RSSetState(pPipelineLib->GetRasterizer(preset));
}

void State_DX12::DSSetState(Graphics::DepthStencilPreset preset, PipelineLib* pPipelineLib)
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

	for (auto i = 0u; i < n; ++i) OMSetRTVFormat(i, formats[i]);
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

void State_DX12::SetNumViewInstances(uint8_t n, ViewInstanceFlag flags)
{
	const auto numViewInstances = static_cast<uint8_t>(size(m_pKey->ViewInstances));
	assert(n <= numViewInstances);
	m_pKey->NumViewInstances = n;

	for (auto i = n; i < numViewInstances; ++i) SetViewInstance(i, ViewInstance());
}

void State_DX12::SetViewInstance(uint8_t i, const ViewInstance& viewInstance)
{
	assert(i < size(m_pKey->ViewInstances));
	m_pKey->ViewInstances[i] = viewInstance;
}

void State_DX12::SetViewInstances(const ViewInstance* viewInstances, uint8_t n, ViewInstanceFlag flags)
{
	SetNumViewInstances(n, flags);

	for (auto i = 0u; i < n; ++i) SetViewInstance(i, viewInstances[i]);
}

Pipeline State_DX12::CreatePipeline(PipelineLib* pPipelineLib, const wchar_t* name) const
{
	return pPipelineLib->CreatePipeline(this, name);
}

Pipeline State_DX12::GetPipeline(PipelineLib* pPipelineLib, const wchar_t* name) const
{
	return pPipelineLib->GetPipeline(this, name);
}

PipelineLayout State_DX12::GetPipelineLayout() const
{
	return m_pKey->Layout;
}

Blob State_DX12::GetShader(Shader::Stage stage) const
{
	assert(stage < Shader::Stage::NUM_STAGE);

	return m_pKey->Shaders[stage];
}

Blob State_DX12::GetCachedPipeline() const
{
	return m_pKey->CachedPipeline;
}

uint32_t State_DX12::GetNodeMask() const
{
	return m_pKey->NodeMask;
}

PipelineFlag State_DX12::GetFlags() const
{
	return m_pKey->Flags;
}

uint32_t State_DX12::OMGetSampleMask() const
{
	return m_pKey->SampleMask;
}

const Graphics::Blend* State_DX12::OMGetBlendState() const
{
	return m_pKey->pBlend;
}

const Graphics::Rasterizer* State_DX12::RSGetState() const
{
	return m_pKey->pRasterizer;
}

const Graphics::DepthStencil* State_DX12::DSGetState() const
{
	return m_pKey->pDepthStencil;
}

const InputLayout* State_DX12::IAGetInputLayout() const
{
	return m_pKey->pInputLayout;
}

PrimitiveTopologyType State_DX12::IAGetPrimitiveTopologyType() const
{
	return m_pKey->PrimTopologyType;
}

IBStripCutValue State_DX12::IAGetIndexBufferStripCutValue() const
{
	return static_cast<IBStripCutValue>(m_pKey->IBStripCutValue);
}

uint8_t State_DX12::OMGetNumRenderTargets() const
{
	return m_pKey->NumRenderTargets;
}

Format State_DX12::OMGetRTVFormat(uint8_t i) const
{
	assert(i < m_pKey->NumRenderTargets);

	return m_pKey->RTVFormats[i];
}

Format State_DX12::OMGetDSVFormat() const
{
	return m_pKey->DSVFormat;
}

uint8_t State_DX12::OMGetSampleCount() const
{
	return m_pKey->SampleCount;
}

uint8_t State_DX12::OMGetSampleQuality() const
{
	return m_pKey->SampleQuality;
}

uint8_t State_DX12::GetNumViewInstances(uint8_t n, ViewInstanceFlag flags) const
{
	return m_pKey->NumViewInstances;
}

ViewInstanceFlag State_DX12::GetViewInstanceFlags() const
{
	return m_pKey->ViewInstanceFlags;
}

const ViewInstance& State_DX12::GetViewInstance(uint8_t i) const
{
	return m_pKey->ViewInstances[i];
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
	m_pfnBlends[Graphics::BlendPreset::DEFAULT_OPAQUE] = Graphics::PipelineLib::DefaultOpaque;
	m_pfnBlends[Graphics::BlendPreset::PREMULTIPLITED] = Graphics::PipelineLib::Premultiplied;
	m_pfnBlends[Graphics::BlendPreset::ADDTIVE] = Graphics::PipelineLib::Additive;
	m_pfnBlends[Graphics::BlendPreset::NON_PRE_MUL] = Graphics::PipelineLib::NonPremultiplied;
	m_pfnBlends[Graphics::BlendPreset::NON_PREMUL_RT0] = Graphics::PipelineLib::NonPremultipliedRT0;
	m_pfnBlends[Graphics::BlendPreset::ALPHA_TO_COVERAGE] = Graphics::PipelineLib::AlphaToCoverage;
	m_pfnBlends[Graphics::BlendPreset::ACCUMULATIVE] = Graphics::PipelineLib::Accumulative;
	m_pfnBlends[Graphics::BlendPreset::AUTO_NON_PREMUL] = Graphics::PipelineLib::AutoNonPremultiplied;
	m_pfnBlends[Graphics::BlendPreset::ZERO_ALPHA_PREMUL] = Graphics::PipelineLib::ZeroAlphaNonPremultiplied;
	m_pfnBlends[Graphics::BlendPreset::MULTIPLITED] = Graphics::PipelineLib::Multiplied;
	m_pfnBlends[Graphics::BlendPreset::WEIGHTED_PREMUL] = Graphics::PipelineLib::WeightedPremul;
	m_pfnBlends[Graphics::BlendPreset::WEIGHTED_PREMUL_PER_RT] = Graphics::PipelineLib::WeightedPremulPerRT;
	m_pfnBlends[Graphics::BlendPreset::WEIGHTED_PER_RT] = Graphics::PipelineLib::WeightedPerRT;
	m_pfnBlends[Graphics::BlendPreset::SELECT_MIN] = Graphics::PipelineLib::SelectMin;
	m_pfnBlends[Graphics::BlendPreset::SELECT_MAX] = Graphics::PipelineLib::SelectMax;

	// Rasterizer states
	m_pfnRasterizers[Graphics::RasterizerPreset::CULL_BACK] = Graphics::PipelineLib::CullBack;
	m_pfnRasterizers[Graphics::RasterizerPreset::CULL_NONE] = Graphics::PipelineLib::CullNone;
	m_pfnRasterizers[Graphics::RasterizerPreset::CULL_FRONT] = Graphics::PipelineLib::CullFront;
	m_pfnRasterizers[Graphics::RasterizerPreset::FILL_WIREFRAME] = Graphics::PipelineLib::FillWireframe;
	m_pfnRasterizers[Graphics::RasterizerPreset::CULL_BACK_CONSERVATIVE] = Graphics::PipelineLib::CullBack_Conservative;
	m_pfnRasterizers[Graphics::RasterizerPreset::CULL_NONE_CONSERVATIVE] = Graphics::PipelineLib::CullNone_Conservative;
	m_pfnRasterizers[Graphics::RasterizerPreset::CULL_FRONT_CONSERVATIVE] = Graphics::PipelineLib::CullFront_Conservative;

	// Depth stencil states
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEFAULT_LESS] = Graphics::PipelineLib::DepthStencilDefault;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_STENCIL_NONE] = Graphics::PipelineLib::DepthStencilNone;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_LESS_EQUAL] = Graphics::PipelineLib::DepthLessEqual;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_EQUAL] = Graphics::PipelineLib::DepthEqual;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_GREATER] = Graphics::PipelineLib::DepthGreater;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_GREATER_EQUAL] = Graphics::PipelineLib::DepthGreaterEqual;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_NOT_EQUAL] = Graphics::PipelineLib::DepthNotEqual;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_ALWAYS] = Graphics::PipelineLib::DepthAlways;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_READ_LESS] = Graphics::PipelineLib::DepthReadLess;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_READ_LESS_EQUAL] = Graphics::PipelineLib::DepthReadLessEqual;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_READ_EQUAL] = Graphics::PipelineLib::DepthReadEqual;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_READ_GREATER] = Graphics::PipelineLib::DepthReadGreater;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_READ_GREATER_EQUAL] = Graphics::PipelineLib::DepthReadGreaterEqual;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_READ_NOT_EQUAL] = Graphics::PipelineLib::DepthReadNotEqual;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_READ_NEVER] = Graphics::PipelineLib::DepthReadNever;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_READ_ALWAYS] = Graphics::PipelineLib::DepthReadAlways;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_LESS_STENCIL_ZERO] = Graphics::PipelineLib::DepthLess_StencilZero;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_LESS_EQUAL_STENCIL_ZERO] = Graphics::PipelineLib::DepthLessEqual_StencilZero;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_EQUAL_STENCIL_ZERO] = Graphics::PipelineLib::DepthEqual_StencilZero;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_GREATER_STENCIL_ZERO] = Graphics::PipelineLib::DepthGreater_StencilZero;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_GREATER_EQUAL_STENCIL_ZERO] = Graphics::PipelineLib::DepthGreaterEqual_StencilZero;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_NOT_EQUAL_STENCIL_ZERO] = Graphics::PipelineLib::DepthNotEqual_StencilZero;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_NEVER_STENCIL_ZERO] = Graphics::PipelineLib::DepthNever_StencilZero;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_ALWAYS_STENCIL_ZERO] = Graphics::PipelineLib::DepthAlways_StencilZero;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_LESS_STENCIL_REPLACE] = Graphics::PipelineLib::DepthLess_StencilReplace;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_LESS_EQUAL_STENCIL_REPLACE] = Graphics::PipelineLib::DepthLessEqual_StencilReplace;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_EQUAL_STENCIL_REPLACE] = Graphics::PipelineLib::DepthEqual_StencilReplace;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_GREATER_STENCIL_REPLACE] = Graphics::PipelineLib::DepthGreater_StencilReplace;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_GREATER_EQUAL_STENCIL_REPLACE] = Graphics::PipelineLib::DepthGreaterEqual_StencilReplace;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_NOT_EQUAL_STENCIL_REPLACE] = Graphics::PipelineLib::DepthNotEqual_StencilReplace;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_NEVER_STENCIL_REPLACE] = Graphics::PipelineLib::DepthNever_StencilReplace;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_ALWAYS_STENCIL_REPLACE] = Graphics::PipelineLib::DepthAlways_StencilReplace;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_LESS_STENCIL_EQUAL] = Graphics::PipelineLib::DepthLess_StencilEqual;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_LESS_EQUAL_STENCIL_EQUAL] = Graphics::PipelineLib::DepthLessEqual_StencilEqual;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_EQUAL_STENCIL_EQUAL] = Graphics::PipelineLib::DepthEqual_StencilEqual;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_GREATER_STENCIL_EQUAL] = Graphics::PipelineLib::DepthGreater_StencilEqual;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_GREATER_EQUAL_STENCIL_EQUAL] = Graphics::PipelineLib::DepthGreaterEqual_StencilEqual;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_NOT_EQUAL_STENCIL_EQUAL] = Graphics::PipelineLib::DepthNotEqual_StencilEqual;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_NEVER_STENCIL_EQUAL] = Graphics::PipelineLib::DepthNever_StencilEqual;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_ALWAYS_STENCIL_EQUAL] = Graphics::PipelineLib::DepthAlways_StencilEqual;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_LESS_STENCIL_NOT_EQUAL] = Graphics::PipelineLib::DepthLess_StencilNotEqual;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_LESS_EQUAL_STENCIL_NOT_EQUAL] = Graphics::PipelineLib::DepthLessEqual_StencilNotEqual;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_EQUAL_STENCIL_NOT_EQUAL] = Graphics::PipelineLib::DepthEqual_StencilNotEqual;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_GREATER_STENCIL_NOT_EQUAL] = Graphics::PipelineLib::DepthGreater_StencilNotEqual;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_GREATER_EQUAL_STENCIL_NOT_EQUAL] = Graphics::PipelineLib::DepthGreaterEqual_StencilNotEqual;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_NOT_EQUAL_STENCIL_NOT_EQUAL] = Graphics::PipelineLib::DepthNotEqual_StencilNotEqual;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_NEVER_STENCIL_NOT_EQUAL] = Graphics::PipelineLib::DepthNever_StencilNotEqual;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::DEPTH_ALWAYS_STENCIL_NOT_EQUAL] = Graphics::PipelineLib::DepthAlways_StencilNotEqual;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::STENCIL_FRONT_INC_BACK_DEC] = Graphics::PipelineLib::StencilFrontIncrBackDecr;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::STENCIL_FRONT_DEC_BACK_INC] = Graphics::PipelineLib::StencilFrontDecrBackIncr;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::STENCIL_FRONT_PASS_ZERO] = Graphics::PipelineLib::StencilPassZero;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::STENCIL_FRONT_FAIL_ZERO] = Graphics::PipelineLib::StencilFailZero;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::STENCIL_FRONT_PASS_INVERT] = Graphics::PipelineLib::StencilPassInvert;
	m_pfnDepthStencils[Graphics::DepthStencilPreset::STENCIL_FRONT_FAIL_INVERT] = Graphics::PipelineLib::StencilFailInvert;
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

void PipelineLib_DX12::SetPipeline(const State* pState, const Pipeline& pipeline)
{
	const auto p = dynamic_cast<const State_DX12*>(pState);
	assert(p);

	m_pipelines[p->GetKey()] = pipeline;
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
	const auto p = dynamic_cast<const State_DX12*>(pState);
	assert(p);

	return createPipeline(p->GetKey(), name);
}

Pipeline PipelineLib_DX12::GetPipeline(const State* pState, const wchar_t* name)
{
	const auto p = dynamic_cast<const State_DX12*>(pState);
	assert(p);

	return getPipeline(p->GetKey(), name);
}

const Graphics::Blend* PipelineLib_DX12::GetBlend(Graphics::BlendPreset preset, uint8_t numColorRTs)
{
	if (m_blends[preset] == nullptr)
		m_blends[preset] = make_unique<Graphics::Blend>(m_pfnBlends[preset](numColorRTs));

	return m_blends[preset].get();
}

const Graphics::Rasterizer* PipelineLib_DX12::GetRasterizer(Graphics::RasterizerPreset preset)
{
	if (m_rasterizers[preset] == nullptr)
		m_rasterizers[preset] = make_unique<Graphics::Rasterizer>(m_pfnRasterizers[preset]());

	return m_rasterizers[preset].get();
}

const Graphics::DepthStencil* PipelineLib_DX12::GetDepthStencil(Graphics::DepthStencilPreset preset)
{
	if (m_depthStencils[preset] == nullptr)
		m_depthStencils[preset] = make_unique<Graphics::DepthStencil>(m_pfnDepthStencils[preset]());

	return m_depthStencils[preset].get();
}

Pipeline PipelineLib_DX12::createPipeline(const string& key, const wchar_t* name)
{
	D3D12_PIPELINE_STATE_STREAM_DESC desc;
	vector<D3D12_INPUT_ELEMENT_DESC> inputElements;

	com_ptr<ID3D12PipelineState> pipeline = nullptr;
	com_ptr<ID3D12Device2> device = nullptr;
	V_RETURN(m_device->QueryInterface(IID_PPV_ARGS(&device)), cerr, nullptr);

	D3D12_FEATURE_DATA_D3D12_OPTIONS19 featureData = {};
	if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS19, &featureData, sizeof(featureData))) ||
		!featureData.RasterizerDesc2Supported)
	{
		D3D12_FEATURE_DATA_D3D12_OPTIONS16 featureData = {};
		if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS16, &featureData, sizeof(featureData))) ||
			!featureData.DynamicDepthBiasSupported)
		{
			D3D12_FEATURE_DATA_D3D12_OPTIONS14 featureData = {};
			if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS14, &featureData, sizeof(featureData))) ||
				!featureData.IndependentFrontAndBackStencilRefMaskSupported)
			{
				// Fill desc
				CD3DX12_PIPELINE_STATE_STREAM2 stream;
				getStream(&stream, &inputElements, key);

				desc.pPipelineStateSubobjectStream = &stream;
				desc.SizeInBytes = sizeof(stream);

				// Create pipeline
				V_RETURN(device->CreatePipelineState(&desc, IID_PPV_ARGS(&pipeline)), cerr, nullptr);
			}
			else
			{
				// Fill desc
				CD3DX12_PIPELINE_STATE_STREAM3 stream;
				getStream(&stream, &inputElements, key);

				desc.pPipelineStateSubobjectStream = &stream;
				desc.SizeInBytes = sizeof(stream);

				// Create pipeline
				V_RETURN(device->CreatePipelineState(&desc, IID_PPV_ARGS(&pipeline)), cerr, nullptr);
			}
		}
		else
		{
			// Fill desc
			CD3DX12_PIPELINE_STATE_STREAM4 stream;
			getStream(&stream, &inputElements, key);

			desc.pPipelineStateSubobjectStream = &stream;
			desc.SizeInBytes = sizeof(stream);

			// Create pipeline
			V_RETURN(device->CreatePipelineState(&desc, IID_PPV_ARGS(&pipeline)), cerr, nullptr);
		}
	}
	else
	{
		// Fill desc
		CD3DX12_PIPELINE_STATE_STREAM5 stream;
		getStream(&stream, &inputElements, key);

		desc.pPipelineStateSubobjectStream = &stream;
		desc.SizeInBytes = sizeof(stream);

		// Create pipeline
		V_RETURN(device->CreatePipelineState(&desc, IID_PPV_ARGS(&pipeline)), cerr, nullptr);
	}

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

void PipelineLib_DX12::getStream(CD3DX12_PIPELINE_STATE_STREAM2* pStream, void* pInputElements, const std::string& key)
{
	const auto pDesc = reinterpret_cast<const PipelineDesc*>(key.data());
	auto& stream = *static_cast<CD3DX12_PIPELINE_STATE_STREAM2*>(pStream);
	stream = {};

	// Fill desc
	if (pDesc->Layout)
		stream.pRootSignature = static_cast<ID3D12RootSignature*>(pDesc->Layout);

	if (pDesc->Shaders[Shader::Stage::VS])
		stream.VS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pDesc->Shaders[Shader::Stage::VS]));
	if (pDesc->Shaders[Shader::Stage::PS])
		stream.PS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pDesc->Shaders[Shader::Stage::PS]));
	if (pDesc->Shaders[Shader::Stage::DS])
		stream.DS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pDesc->Shaders[Shader::Stage::DS]));
	if (pDesc->Shaders[Shader::Stage::HS])
		stream.HS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pDesc->Shaders[Shader::Stage::HS]));
	if (pDesc->Shaders[Shader::Stage::GS])
		stream.GS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pDesc->Shaders[Shader::Stage::GS]));
	if (pDesc->Shaders[Shader::Stage::CS])
		stream.CS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pDesc->Shaders[Shader::Stage::CS]));
	if (pDesc->Shaders[Shader::Stage::MS])
		stream.MS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pDesc->Shaders[Shader::Stage::MS]));
	if (pDesc->Shaders[Shader::Stage::AS])
		stream.AS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pDesc->Shaders[Shader::Stage::AS]));

	// Blend state
	unique_ptr<Graphics::Blend> blend;
	auto pBlend = pDesc->pBlend;
	if (pBlend == nullptr)
	{
		blend = make_unique<Graphics::Blend>(Graphics::PipelineLib::DefaultOpaque(pDesc->NumRenderTargets));
		pBlend = blend.get();
	}
	CD3DX12_BLEND_DESC& blendState = stream.BlendState;
	blendState.AlphaToCoverageEnable = pBlend->AlphaToCoverageEnable ? TRUE : FALSE;
	blendState.IndependentBlendEnable = pBlend->IndependentBlendEnable ? TRUE : FALSE;
	for (uint8_t i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
	{
		const auto& src = pBlend->RenderTargets[i];
		auto& dst = blendState.RenderTarget[i];
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
	stream.SampleMask = pDesc->SampleMask;

	// Rasterizer state
	unique_ptr<Graphics::Rasterizer> rasterizer;
	auto pRasterizer = pDesc->pRasterizer;
	if (pRasterizer == nullptr)
	{
		rasterizer = make_unique<Graphics::Rasterizer>(Graphics::PipelineLib::CullBack());
		pRasterizer = rasterizer.get();
	}
	CD3DX12_RASTERIZER_DESC& rasterizerState = stream.RasterizerState;
	rasterizerState.FillMode = GetDX12FillMode(pRasterizer->Fill);
	rasterizerState.CullMode = GetDX12CullMode(pRasterizer->Cull);
	rasterizerState.FrontCounterClockwise = pRasterizer->FrontCounterClockwise ? TRUE : FALSE;
	rasterizerState.DepthBias = static_cast<INT>(pRasterizer->DepthBias);
	rasterizerState.DepthBiasClamp = pRasterizer->DepthBiasClamp;
	rasterizerState.SlopeScaledDepthBias = pRasterizer->SlopeScaledDepthBias;
	rasterizerState.DepthClipEnable = pRasterizer->DepthClipEnable ? TRUE : FALSE;
	rasterizerState.MultisampleEnable = FALSE;
	rasterizerState.AntialiasedLineEnable = FALSE;
	if (pRasterizer->LineRasterizationMode == LineRasterization::ALPHA_ANTIALIASED)
		rasterizerState.AntialiasedLineEnable = TRUE;
	else if (pRasterizer->LineRasterizationMode != LineRasterization::ALIASED)
		rasterizerState.MultisampleEnable = TRUE;
	rasterizerState.ForcedSampleCount = pRasterizer->ForcedSampleCount;
	rasterizerState.ConservativeRaster = pRasterizer->ConservativeRaster ?
		D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	// Depth-stencil state
	unique_ptr<Graphics::DepthStencil> depthStencil;
	auto pDepthStencil = pDesc->pDepthStencil;
	if (pDepthStencil == nullptr)
	{
		depthStencil = make_unique<Graphics::DepthStencil>(Graphics::PipelineLib::DepthStencilDefault());
		pDepthStencil = depthStencil.get();
	}
	CD3DX12_DEPTH_STENCIL_DESC1& depthStencilState = stream.DepthStencilState;
	depthStencilState.DepthEnable = pDepthStencil->DepthEnable ? TRUE : FALSE;
	depthStencilState.DepthWriteMask = pDepthStencil->DepthWriteMask ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
	depthStencilState.DepthFunc = GetDX12ComparisonFunc(pDepthStencil->Comparison);
	depthStencilState.StencilEnable = pDepthStencil->StencilEnable ? TRUE : FALSE;
	depthStencilState.StencilReadMask = pDepthStencil->FrontFace.StencilReadMask;
	depthStencilState.StencilWriteMask = pDepthStencil->FrontFace.StencilWriteMask;
	depthStencilState.FrontFace.StencilFailOp = GetDX12StencilOp(pDepthStencil->FrontFace.StencilFailOp);
	depthStencilState.FrontFace.StencilDepthFailOp = GetDX12StencilOp(pDepthStencil->FrontFace.StencilDepthFailOp);
	depthStencilState.FrontFace.StencilPassOp = GetDX12StencilOp(pDepthStencil->FrontFace.StencilPassOp);
	depthStencilState.FrontFace.StencilFunc = GetDX12ComparisonFunc(pDepthStencil->FrontFace.StencilFunc);
	depthStencilState.BackFace.StencilFailOp = GetDX12StencilOp(pDepthStencil->BackFace.StencilFailOp);
	depthStencilState.BackFace.StencilDepthFailOp = GetDX12StencilOp(pDepthStencil->BackFace.StencilDepthFailOp);
	depthStencilState.BackFace.StencilPassOp = GetDX12StencilOp(pDepthStencil->BackFace.StencilPassOp);
	depthStencilState.BackFace.StencilFunc = GetDX12ComparisonFunc(pDepthStencil->BackFace.StencilFunc);
	depthStencilState.DepthBoundsTestEnable = pDepthStencil->DepthBoundsTestEnable ? TRUE : FALSE;

	// Input layout
	auto& inputElements = *static_cast<vector<D3D12_INPUT_ELEMENT_DESC>*>(pInputElements);
	if (pDesc->pInputLayout)
	{
		D3D12_INPUT_LAYOUT_DESC& inputLayout = stream.InputLayout;
		inputLayout.NumElements = static_cast<uint32_t>(pDesc->pInputLayout->size());
		inputElements.resize(inputLayout.NumElements);

		for (auto i = 0u; i < inputLayout.NumElements; ++i)
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

		inputLayout.pInputElementDescs = inputElements.data();
	}

	// Primitive Topology
	stream.PrimitiveTopologyType = GetDX12PrimitiveTopologyType(pDesc->PrimTopologyType);

	// RTV formats and DSV format
	D3D12_RT_FORMAT_ARRAY& rtvFormats = stream.RTVFormats;
	rtvFormats.NumRenderTargets = pDesc->NumRenderTargets;
	for (uint8_t i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		rtvFormats.RTFormats[i] = GetDXGIFormat(pDesc->RTVFormats[i]);
	stream.DSVFormat = GetDXGIFormat(pDesc->DSVFormat);

	// Sample desc
	DXGI_SAMPLE_DESC& sampleDesc = stream.SampleDesc;
	sampleDesc.Count = pDesc->SampleCount;
	sampleDesc.Quality = pDesc->SampleQuality;

	// Cached PSO
	if (pDesc->CachedPipeline)
	{
		D3D12_CACHED_PIPELINE_STATE& cachedPSO = stream.CachedPSO;
		size_t cachedBlobSize;
		GetDX12BlobData(pDesc->CachedPipeline, &cachedPSO.pCachedBlob, &cachedBlobSize);
		cachedPSO.CachedBlobSizeInBytes = cachedBlobSize;
	}

	// Node mask and flags
	stream.NodeMask = pDesc->NodeMask;
	stream.Flags = GetDX12PipelineFlags(pDesc->Flags);

	// IB strip cube value
	switch (static_cast<IBStripCutValue>(pDesc->IBStripCutValue))
	{
	case IBStripCutValue::FFFF:
		stream.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFF;
		break;
	case IBStripCutValue::FFFFFFFF:
		stream.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF;
		break;
	default:
		assert(stream.IBStripCutValue == D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED);
	}

	// View instancing
	D3D12_VIEW_INSTANCING_DESC& viewInstancing = stream.ViewInstancingDesc;
	D3D12_VIEW_INSTANCE_LOCATION viewInstances[D3D12_MAX_VIEW_INSTANCE_COUNT];
	viewInstancing.ViewInstanceCount = pDesc->NumViewInstances;
	for (uint8_t i = 0; i < pDesc->NumViewInstances; ++i)
	{
		viewInstances[i].ViewportArrayIndex = pDesc->ViewInstances[i].ViewportArrayIndex;
		viewInstances[i].RenderTargetArrayIndex = pDesc->ViewInstances[i].RenderTargetArrayIndex;
	}
	viewInstancing.pViewInstanceLocations = viewInstances;
	viewInstancing.Flags = GetDX12ViewInstanceFlags(pDesc->ViewInstanceFlags);
}

void PipelineLib_DX12::getStream(CD3DX12_PIPELINE_STATE_STREAM3* pStream, void* pInputElements, const std::string& key)
{
	const auto pDesc = reinterpret_cast<const PipelineDesc*>(key.data());
	auto& stream = *static_cast<CD3DX12_PIPELINE_STATE_STREAM3*>(pStream);
	stream = {};

	// Fill desc
	if (pDesc->Layout)
		stream.pRootSignature = static_cast<ID3D12RootSignature*>(pDesc->Layout);

	if (pDesc->Shaders[Shader::Stage::VS])
		stream.VS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pDesc->Shaders[Shader::Stage::VS]));
	if (pDesc->Shaders[Shader::Stage::PS])
		stream.PS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pDesc->Shaders[Shader::Stage::PS]));
	if (pDesc->Shaders[Shader::Stage::DS])
		stream.DS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pDesc->Shaders[Shader::Stage::DS]));
	if (pDesc->Shaders[Shader::Stage::HS])
		stream.HS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pDesc->Shaders[Shader::Stage::HS]));
	if (pDesc->Shaders[Shader::Stage::GS])
		stream.GS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pDesc->Shaders[Shader::Stage::GS]));
	if (pDesc->Shaders[Shader::Stage::CS])
		stream.CS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pDesc->Shaders[Shader::Stage::CS]));
	if (pDesc->Shaders[Shader::Stage::MS])
		stream.MS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pDesc->Shaders[Shader::Stage::MS]));
	if (pDesc->Shaders[Shader::Stage::AS])
		stream.AS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pDesc->Shaders[Shader::Stage::AS]));

	// Blend state
	unique_ptr<Graphics::Blend> blend;
	auto pBlend = pDesc->pBlend;
	if (pBlend == nullptr)
	{
		blend = make_unique<Graphics::Blend>(Graphics::PipelineLib::DefaultOpaque(pDesc->NumRenderTargets));
		pBlend = blend.get();
	}
	CD3DX12_BLEND_DESC& blendState = stream.BlendState;
	blendState.AlphaToCoverageEnable = pBlend->AlphaToCoverageEnable ? TRUE : FALSE;
	blendState.IndependentBlendEnable = pBlend->IndependentBlendEnable ? TRUE : FALSE;
	for (uint8_t i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
	{
		const auto& src = pBlend->RenderTargets[i];
		auto& dst = blendState.RenderTarget[i];
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
	stream.SampleMask = pDesc->SampleMask;

	// Rasterizer state
	unique_ptr<Graphics::Rasterizer> rasterizer;
	auto pRasterizer = pDesc->pRasterizer;
	if (pRasterizer == nullptr)
	{
		rasterizer = make_unique<Graphics::Rasterizer>(Graphics::PipelineLib::CullBack());
		pRasterizer = rasterizer.get();
	}
	CD3DX12_RASTERIZER_DESC& rasterizerState = stream.RasterizerState;
	rasterizerState.FillMode = GetDX12FillMode(pRasterizer->Fill);
	rasterizerState.CullMode = GetDX12CullMode(pRasterizer->Cull);
	rasterizerState.FrontCounterClockwise = pRasterizer->FrontCounterClockwise ? TRUE : FALSE;
	rasterizerState.DepthBias = static_cast<INT>(pRasterizer->DepthBias);
	rasterizerState.DepthBiasClamp = pRasterizer->DepthBiasClamp;
	rasterizerState.SlopeScaledDepthBias = pRasterizer->SlopeScaledDepthBias;
	rasterizerState.DepthClipEnable = pRasterizer->DepthClipEnable ? TRUE : FALSE;
	rasterizerState.MultisampleEnable = FALSE;
	rasterizerState.AntialiasedLineEnable = FALSE;
	if (pRasterizer->LineRasterizationMode == LineRasterization::ALPHA_ANTIALIASED)
		rasterizerState.AntialiasedLineEnable = TRUE;
	else if (pRasterizer->LineRasterizationMode != LineRasterization::ALIASED)
		rasterizerState.MultisampleEnable = TRUE;
	rasterizerState.ForcedSampleCount = pRasterizer->ForcedSampleCount;
	rasterizerState.ConservativeRaster = pRasterizer->ConservativeRaster ?
		D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	// Depth-stencil state
	unique_ptr<Graphics::DepthStencil> depthStencil;
	auto pDepthStencil = pDesc->pDepthStencil;
	if (pDepthStencil == nullptr)
	{
		depthStencil = make_unique<Graphics::DepthStencil>(Graphics::PipelineLib::DepthStencilDefault());
		pDepthStencil = depthStencil.get();
	}
	CD3DX12_DEPTH_STENCIL_DESC2& depthStencilState = stream.DepthStencilState;
	depthStencilState.DepthEnable = pDepthStencil->DepthEnable ? TRUE : FALSE;
	depthStencilState.DepthWriteMask = pDepthStencil->DepthWriteMask ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
	depthStencilState.DepthFunc = GetDX12ComparisonFunc(pDepthStencil->Comparison);
	depthStencilState.StencilEnable = pDepthStencil->StencilEnable ? TRUE : FALSE;
	depthStencilState.FrontFace.StencilFailOp = GetDX12StencilOp(pDepthStencil->FrontFace.StencilFailOp);
	depthStencilState.FrontFace.StencilDepthFailOp = GetDX12StencilOp(pDepthStencil->FrontFace.StencilDepthFailOp);
	depthStencilState.FrontFace.StencilPassOp = GetDX12StencilOp(pDepthStencil->FrontFace.StencilPassOp);
	depthStencilState.FrontFace.StencilFunc = GetDX12ComparisonFunc(pDepthStencil->FrontFace.StencilFunc);
	depthStencilState.FrontFace.StencilReadMask = pDepthStencil->FrontFace.StencilReadMask;
	depthStencilState.FrontFace.StencilWriteMask = pDepthStencil->FrontFace.StencilWriteMask;
	depthStencilState.BackFace.StencilFailOp = GetDX12StencilOp(pDepthStencil->BackFace.StencilFailOp);
	depthStencilState.BackFace.StencilDepthFailOp = GetDX12StencilOp(pDepthStencil->BackFace.StencilDepthFailOp);
	depthStencilState.BackFace.StencilPassOp = GetDX12StencilOp(pDepthStencil->BackFace.StencilPassOp);
	depthStencilState.BackFace.StencilFunc = GetDX12ComparisonFunc(pDepthStencil->BackFace.StencilFunc);
	depthStencilState.BackFace.StencilReadMask = pDepthStencil->FrontFace.StencilReadMask;
	depthStencilState.BackFace.StencilWriteMask = pDepthStencil->FrontFace.StencilWriteMask;
	depthStencilState.DepthBoundsTestEnable = pDepthStencil->DepthBoundsTestEnable ? TRUE : FALSE;

	// Input layout
	auto& inputElements = *static_cast<vector<D3D12_INPUT_ELEMENT_DESC>*>(pInputElements);
	if (pDesc->pInputLayout)
	{
		D3D12_INPUT_LAYOUT_DESC& inputLayout = stream.InputLayout;
		inputLayout.NumElements = static_cast<uint32_t>(pDesc->pInputLayout->size());
		inputElements.resize(inputLayout.NumElements);

		for (auto i = 0u; i < inputLayout.NumElements; ++i)
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

		inputLayout.pInputElementDescs = inputElements.data();
	}

	// Primitive Topology
	stream.PrimitiveTopologyType = GetDX12PrimitiveTopologyType(pDesc->PrimTopologyType);

	// RTV formats and DSV format
	D3D12_RT_FORMAT_ARRAY& rtvFormats = stream.RTVFormats;
	rtvFormats.NumRenderTargets = pDesc->NumRenderTargets;
	for (uint8_t i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		rtvFormats.RTFormats[i] = GetDXGIFormat(pDesc->RTVFormats[i]);
	stream.DSVFormat = GetDXGIFormat(pDesc->DSVFormat);

	// Sample desc
	DXGI_SAMPLE_DESC& sampleDesc = stream.SampleDesc;
	sampleDesc.Count = pDesc->SampleCount;
	sampleDesc.Quality = pDesc->SampleQuality;

	// Cached PSO
	if (pDesc->CachedPipeline)
	{
		D3D12_CACHED_PIPELINE_STATE& cachedPSO = stream.CachedPSO;
		size_t cachedBlobSize;
		GetDX12BlobData(pDesc->CachedPipeline, &cachedPSO.pCachedBlob, &cachedBlobSize);
		cachedPSO.CachedBlobSizeInBytes = cachedBlobSize;
	}

	// Node mask and flags
	stream.NodeMask = pDesc->NodeMask;
	stream.Flags = GetDX12PipelineFlags(pDesc->Flags);

	// IB strip cube value
	switch (static_cast<IBStripCutValue>(pDesc->IBStripCutValue))
	{
	case IBStripCutValue::FFFF:
		stream.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFF;
		break;
	case IBStripCutValue::FFFFFFFF:
		stream.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF;
		break;
	default:
		assert(stream.IBStripCutValue == D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED);
	}

	// View instancing
	D3D12_VIEW_INSTANCING_DESC& viewInstancing = stream.ViewInstancingDesc;
	D3D12_VIEW_INSTANCE_LOCATION viewInstances[D3D12_MAX_VIEW_INSTANCE_COUNT];
	viewInstancing.ViewInstanceCount = pDesc->NumViewInstances;
	for (uint8_t i = 0; i < pDesc->NumViewInstances; ++i)
	{
		viewInstances[i].ViewportArrayIndex = pDesc->ViewInstances[i].ViewportArrayIndex;
		viewInstances[i].RenderTargetArrayIndex = pDesc->ViewInstances[i].RenderTargetArrayIndex;
	}
	viewInstancing.pViewInstanceLocations = viewInstances;
	viewInstancing.Flags = GetDX12ViewInstanceFlags(pDesc->ViewInstanceFlags);
}

void PipelineLib_DX12::getStream(CD3DX12_PIPELINE_STATE_STREAM4* pStream, void* pInputElements, const std::string& key)
{
	const auto pDesc = reinterpret_cast<const PipelineDesc*>(key.data());
	auto& stream = *static_cast<CD3DX12_PIPELINE_STATE_STREAM4*>(pStream);
	stream = {};

	// Fill desc
	if (pDesc->Layout)
		stream.pRootSignature = static_cast<ID3D12RootSignature*>(pDesc->Layout);

	if (pDesc->Shaders[Shader::Stage::VS])
		stream.VS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pDesc->Shaders[Shader::Stage::VS]));
	if (pDesc->Shaders[Shader::Stage::PS])
		stream.PS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pDesc->Shaders[Shader::Stage::PS]));
	if (pDesc->Shaders[Shader::Stage::DS])
		stream.DS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pDesc->Shaders[Shader::Stage::DS]));
	if (pDesc->Shaders[Shader::Stage::HS])
		stream.HS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pDesc->Shaders[Shader::Stage::HS]));
	if (pDesc->Shaders[Shader::Stage::GS])
		stream.GS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pDesc->Shaders[Shader::Stage::GS]));
	if (pDesc->Shaders[Shader::Stage::CS])
		stream.CS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pDesc->Shaders[Shader::Stage::CS]));
	if (pDesc->Shaders[Shader::Stage::MS])
		stream.MS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pDesc->Shaders[Shader::Stage::MS]));
	if (pDesc->Shaders[Shader::Stage::AS])
		stream.AS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pDesc->Shaders[Shader::Stage::AS]));

	// Blend state
	unique_ptr<Graphics::Blend> blend;
	auto pBlend = pDesc->pBlend;
	if (pBlend == nullptr)
	{
		blend = make_unique<Graphics::Blend>(Graphics::PipelineLib::DefaultOpaque(pDesc->NumRenderTargets));
		pBlend = blend.get();
	}
	CD3DX12_BLEND_DESC& blendState = stream.BlendState;
	blendState.AlphaToCoverageEnable = pBlend->AlphaToCoverageEnable ? TRUE : FALSE;
	blendState.IndependentBlendEnable = pBlend->IndependentBlendEnable ? TRUE : FALSE;
	for (uint8_t i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
	{
		const auto& src = pBlend->RenderTargets[i];
		auto& dst = blendState.RenderTarget[i];
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
	stream.SampleMask = pDesc->SampleMask;

	// Rasterizer state
	unique_ptr<Graphics::Rasterizer> rasterizer;
	auto pRasterizer = pDesc->pRasterizer;
	if (pRasterizer == nullptr)
	{
		rasterizer = make_unique<Graphics::Rasterizer>(Graphics::PipelineLib::CullBack());
		pRasterizer = rasterizer.get();
	}
	CD3DX12_RASTERIZER_DESC1& rasterizerState = stream.RasterizerState;
	rasterizerState.FillMode = GetDX12FillMode(pRasterizer->Fill);
	rasterizerState.CullMode = GetDX12CullMode(pRasterizer->Cull);
	rasterizerState.FrontCounterClockwise = pRasterizer->FrontCounterClockwise ? TRUE : FALSE;
	rasterizerState.DepthBias = pRasterizer->DepthBias;
	rasterizerState.DepthBiasClamp = pRasterizer->DepthBiasClamp;
	rasterizerState.SlopeScaledDepthBias = pRasterizer->SlopeScaledDepthBias;
	rasterizerState.DepthClipEnable = pRasterizer->DepthClipEnable ? TRUE : FALSE;
	rasterizerState.MultisampleEnable = FALSE;
	rasterizerState.AntialiasedLineEnable = FALSE;
	if (pRasterizer->LineRasterizationMode == LineRasterization::ALPHA_ANTIALIASED)
		rasterizerState.AntialiasedLineEnable = TRUE;
	else if (pRasterizer->LineRasterizationMode != LineRasterization::ALIASED)
		rasterizerState.MultisampleEnable = TRUE;
	rasterizerState.ForcedSampleCount = pRasterizer->ForcedSampleCount;
	rasterizerState.ConservativeRaster = pRasterizer->ConservativeRaster ?
		D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	// Depth-stencil state
	unique_ptr<Graphics::DepthStencil> depthStencil;
	auto pDepthStencil = pDesc->pDepthStencil;
	if (pDepthStencil == nullptr)
	{
		depthStencil = make_unique<Graphics::DepthStencil>(Graphics::PipelineLib::DepthStencilDefault());
		pDepthStencil = depthStencil.get();
	}
	CD3DX12_DEPTH_STENCIL_DESC2& depthStencilState = stream.DepthStencilState;
	depthStencilState.DepthEnable = pDepthStencil->DepthEnable ? TRUE : FALSE;
	depthStencilState.DepthWriteMask = pDepthStencil->DepthWriteMask ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
	depthStencilState.DepthFunc = GetDX12ComparisonFunc(pDepthStencil->Comparison);
	depthStencilState.StencilEnable = pDepthStencil->StencilEnable ? TRUE : FALSE;
	depthStencilState.FrontFace.StencilFailOp = GetDX12StencilOp(pDepthStencil->FrontFace.StencilFailOp);
	depthStencilState.FrontFace.StencilDepthFailOp = GetDX12StencilOp(pDepthStencil->FrontFace.StencilDepthFailOp);
	depthStencilState.FrontFace.StencilPassOp = GetDX12StencilOp(pDepthStencil->FrontFace.StencilPassOp);
	depthStencilState.FrontFace.StencilFunc = GetDX12ComparisonFunc(pDepthStencil->FrontFace.StencilFunc);
	depthStencilState.FrontFace.StencilReadMask = pDepthStencil->FrontFace.StencilReadMask;
	depthStencilState.FrontFace.StencilWriteMask = pDepthStencil->FrontFace.StencilWriteMask;
	depthStencilState.BackFace.StencilFailOp = GetDX12StencilOp(pDepthStencil->BackFace.StencilFailOp);
	depthStencilState.BackFace.StencilDepthFailOp = GetDX12StencilOp(pDepthStencil->BackFace.StencilDepthFailOp);
	depthStencilState.BackFace.StencilPassOp = GetDX12StencilOp(pDepthStencil->BackFace.StencilPassOp);
	depthStencilState.BackFace.StencilFunc = GetDX12ComparisonFunc(pDepthStencil->BackFace.StencilFunc);
	depthStencilState.BackFace.StencilReadMask = pDepthStencil->FrontFace.StencilReadMask;
	depthStencilState.BackFace.StencilWriteMask = pDepthStencil->FrontFace.StencilWriteMask;
	depthStencilState.DepthBoundsTestEnable = pDepthStencil->DepthBoundsTestEnable ? TRUE : FALSE;

	// Input layout
	auto& inputElements = *static_cast<vector<D3D12_INPUT_ELEMENT_DESC>*>(pInputElements);
	if (pDesc->pInputLayout)
	{
		D3D12_INPUT_LAYOUT_DESC& inputLayout = stream.InputLayout;
		inputLayout.NumElements = static_cast<uint32_t>(pDesc->pInputLayout->size());
		inputElements.resize(inputLayout.NumElements);

		for (auto i = 0u; i < inputLayout.NumElements; ++i)
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

		inputLayout.pInputElementDescs = inputElements.data();
	}

	// Primitive Topology
	stream.PrimitiveTopologyType = GetDX12PrimitiveTopologyType(pDesc->PrimTopologyType);

	// RTV formats and DSV format
	D3D12_RT_FORMAT_ARRAY& rtvFormats = stream.RTVFormats;
	rtvFormats.NumRenderTargets = pDesc->NumRenderTargets;
	for (uint8_t i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		rtvFormats.RTFormats[i] = GetDXGIFormat(pDesc->RTVFormats[i]);
	stream.DSVFormat = GetDXGIFormat(pDesc->DSVFormat);

	// Sample desc
	DXGI_SAMPLE_DESC& sampleDesc = stream.SampleDesc;
	sampleDesc.Count = pDesc->SampleCount;
	sampleDesc.Quality = pDesc->SampleQuality;

	// Cached PSO
	if (pDesc->CachedPipeline)
	{
		D3D12_CACHED_PIPELINE_STATE& cachedPSO = stream.CachedPSO;
		size_t cachedBlobSize;
		GetDX12BlobData(pDesc->CachedPipeline, &cachedPSO.pCachedBlob, &cachedBlobSize);
		cachedPSO.CachedBlobSizeInBytes = cachedBlobSize;
	}

	// Node mask and flags
	stream.NodeMask = pDesc->NodeMask;
	stream.Flags = GetDX12PipelineFlags(pDesc->Flags);

	// IB strip cube value
	switch (static_cast<IBStripCutValue>(pDesc->IBStripCutValue))
	{
	case IBStripCutValue::FFFF:
		stream.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFF;
		break;
	case IBStripCutValue::FFFFFFFF:
		stream.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF;
		break;
	default:
		assert(stream.IBStripCutValue == D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED);
	}

	// View instancing
	D3D12_VIEW_INSTANCING_DESC& viewInstancing = stream.ViewInstancingDesc;
	D3D12_VIEW_INSTANCE_LOCATION viewInstances[D3D12_MAX_VIEW_INSTANCE_COUNT];
	viewInstancing.ViewInstanceCount = pDesc->NumViewInstances;
	for (uint8_t i = 0; i < pDesc->NumViewInstances; ++i)
	{
		viewInstances[i].ViewportArrayIndex = pDesc->ViewInstances[i].ViewportArrayIndex;
		viewInstances[i].RenderTargetArrayIndex = pDesc->ViewInstances[i].RenderTargetArrayIndex;
	}
	viewInstancing.pViewInstanceLocations = viewInstances;
	viewInstancing.Flags = GetDX12ViewInstanceFlags(pDesc->ViewInstanceFlags);
}

void PipelineLib_DX12::getStream(CD3DX12_PIPELINE_STATE_STREAM5* pStream, void* pInputElements, const std::string& key)
{
	const auto pDesc = reinterpret_cast<const PipelineDesc*>(key.data());
	auto& stream = *static_cast<CD3DX12_PIPELINE_STATE_STREAM5*>(pStream);
	stream = {};

	// Fill desc
	if (pDesc->Layout)
		stream.pRootSignature = static_cast<ID3D12RootSignature*>(pDesc->Layout);

	if (pDesc->Shaders[Shader::Stage::VS])
		stream.VS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pDesc->Shaders[Shader::Stage::VS]));
	if (pDesc->Shaders[Shader::Stage::PS])
		stream.PS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pDesc->Shaders[Shader::Stage::PS]));
	if (pDesc->Shaders[Shader::Stage::DS])
		stream.DS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pDesc->Shaders[Shader::Stage::DS]));
	if (pDesc->Shaders[Shader::Stage::HS])
		stream.HS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pDesc->Shaders[Shader::Stage::HS]));
	if (pDesc->Shaders[Shader::Stage::GS])
		stream.GS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pDesc->Shaders[Shader::Stage::GS]));
	if (pDesc->Shaders[Shader::Stage::CS])
		stream.CS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pDesc->Shaders[Shader::Stage::CS]));
	if (pDesc->Shaders[Shader::Stage::MS])
		stream.MS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pDesc->Shaders[Shader::Stage::MS]));
	if (pDesc->Shaders[Shader::Stage::AS])
		stream.AS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pDesc->Shaders[Shader::Stage::AS]));

	// Blend state
	unique_ptr<Graphics::Blend> blend;
	auto pBlend = pDesc->pBlend;
	if (pBlend == nullptr)
	{
		blend = make_unique<Graphics::Blend>(Graphics::PipelineLib::DefaultOpaque(pDesc->NumRenderTargets));
		pBlend = blend.get();
	}
	CD3DX12_BLEND_DESC& blendState = stream.BlendState;
	blendState.AlphaToCoverageEnable = pBlend->AlphaToCoverageEnable ? TRUE : FALSE;
	blendState.IndependentBlendEnable = pBlend->IndependentBlendEnable ? TRUE : FALSE;
	for (uint8_t i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
	{
		const auto& src = pBlend->RenderTargets[i];
		auto& dst = blendState.RenderTarget[i];
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
	stream.SampleMask = pDesc->SampleMask;

	// Rasterizer state
	unique_ptr<Graphics::Rasterizer> rasterizer;
	auto pRasterizer = pDesc->pRasterizer;
	if (pRasterizer == nullptr)
	{
		rasterizer = make_unique<Graphics::Rasterizer>(Graphics::PipelineLib::CullBack());
		pRasterizer = rasterizer.get();
	}
	CD3DX12_RASTERIZER_DESC2& rasterizerState = stream.RasterizerState;
	rasterizerState.FillMode = GetDX12FillMode(pRasterizer->Fill);
	rasterizerState.CullMode = GetDX12CullMode(pRasterizer->Cull);
	rasterizerState.FrontCounterClockwise = pRasterizer->FrontCounterClockwise ? TRUE : FALSE;
	rasterizerState.DepthBias = pRasterizer->DepthBias;
	rasterizerState.DepthBiasClamp = pRasterizer->DepthBiasClamp;
	rasterizerState.SlopeScaledDepthBias = pRasterizer->SlopeScaledDepthBias;
	rasterizerState.DepthClipEnable = pRasterizer->DepthClipEnable ? TRUE : FALSE;
	rasterizerState.LineRasterizationMode = GetDX12LineRasterizationMode(pRasterizer->LineRasterizationMode);
	rasterizerState.ForcedSampleCount = pRasterizer->ForcedSampleCount;
	rasterizerState.ConservativeRaster = pRasterizer->ConservativeRaster ?
		D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	// Depth-stencil state
	unique_ptr<Graphics::DepthStencil> depthStencil;
	auto pDepthStencil = pDesc->pDepthStencil;
	if (pDepthStencil == nullptr)
	{
		depthStencil = make_unique<Graphics::DepthStencil>(Graphics::PipelineLib::DepthStencilDefault());
		pDepthStencil = depthStencil.get();
	}
	CD3DX12_DEPTH_STENCIL_DESC2& depthStencilState = stream.DepthStencilState;
	depthStencilState.DepthEnable = pDepthStencil->DepthEnable ? TRUE : FALSE;
	depthStencilState.DepthWriteMask = pDepthStencil->DepthWriteMask ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
	depthStencilState.DepthFunc = GetDX12ComparisonFunc(pDepthStencil->Comparison);
	depthStencilState.StencilEnable = pDepthStencil->StencilEnable ? TRUE : FALSE;
	depthStencilState.FrontFace.StencilFailOp = GetDX12StencilOp(pDepthStencil->FrontFace.StencilFailOp);
	depthStencilState.FrontFace.StencilDepthFailOp = GetDX12StencilOp(pDepthStencil->FrontFace.StencilDepthFailOp);
	depthStencilState.FrontFace.StencilPassOp = GetDX12StencilOp(pDepthStencil->FrontFace.StencilPassOp);
	depthStencilState.FrontFace.StencilFunc = GetDX12ComparisonFunc(pDepthStencil->FrontFace.StencilFunc);
	depthStencilState.FrontFace.StencilReadMask = pDepthStencil->FrontFace.StencilReadMask;
	depthStencilState.FrontFace.StencilWriteMask = pDepthStencil->FrontFace.StencilWriteMask;
	depthStencilState.BackFace.StencilFailOp = GetDX12StencilOp(pDepthStencil->BackFace.StencilFailOp);
	depthStencilState.BackFace.StencilDepthFailOp = GetDX12StencilOp(pDepthStencil->BackFace.StencilDepthFailOp);
	depthStencilState.BackFace.StencilPassOp = GetDX12StencilOp(pDepthStencil->BackFace.StencilPassOp);
	depthStencilState.BackFace.StencilFunc = GetDX12ComparisonFunc(pDepthStencil->BackFace.StencilFunc);
	depthStencilState.BackFace.StencilReadMask = pDepthStencil->FrontFace.StencilReadMask;
	depthStencilState.BackFace.StencilWriteMask = pDepthStencil->FrontFace.StencilWriteMask;
	depthStencilState.DepthBoundsTestEnable = pDepthStencil->DepthBoundsTestEnable ? TRUE : FALSE;

	// Input layout
	auto& inputElements = *static_cast<vector<D3D12_INPUT_ELEMENT_DESC>*>(pInputElements);
	if (pDesc->pInputLayout)
	{
		D3D12_INPUT_LAYOUT_DESC& inputLayout = stream.InputLayout;
		inputLayout.NumElements = static_cast<uint32_t>(pDesc->pInputLayout->size());
		inputElements.resize(inputLayout.NumElements);

		for (auto i = 0u; i < inputLayout.NumElements; ++i)
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

		inputLayout.pInputElementDescs = inputElements.data();
	}

	// Primitive Topology
	stream.PrimitiveTopologyType = GetDX12PrimitiveTopologyType(pDesc->PrimTopologyType);

	// RTV formats and DSV format
	D3D12_RT_FORMAT_ARRAY& rtvFormats = stream.RTVFormats;
	rtvFormats.NumRenderTargets = pDesc->NumRenderTargets;
	for (uint8_t i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		rtvFormats.RTFormats[i] = GetDXGIFormat(pDesc->RTVFormats[i]);
	stream.DSVFormat = GetDXGIFormat(pDesc->DSVFormat);

	// Sample desc
	DXGI_SAMPLE_DESC& sampleDesc = stream.SampleDesc;
	sampleDesc.Count = pDesc->SampleCount;
	sampleDesc.Quality = pDesc->SampleQuality;

	// Cached PSO
	if (pDesc->CachedPipeline)
	{
		D3D12_CACHED_PIPELINE_STATE& cachedPSO = stream.CachedPSO;
		size_t cachedBlobSize;
		GetDX12BlobData(pDesc->CachedPipeline, &cachedPSO.pCachedBlob, &cachedBlobSize);
		cachedPSO.CachedBlobSizeInBytes = cachedBlobSize;
	}

	// Node mask and flags
	stream.NodeMask = pDesc->NodeMask;
	stream.Flags = GetDX12PipelineFlags(pDesc->Flags);

	// IB strip cube value
	switch (static_cast<IBStripCutValue>(pDesc->IBStripCutValue))
	{
	case IBStripCutValue::FFFF:
		stream.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFF;
		break;
	case IBStripCutValue::FFFFFFFF:
		stream.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF;
		break;
	default:
		assert(stream.IBStripCutValue == D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED);
	}

	// View instancing
	D3D12_VIEW_INSTANCING_DESC& viewInstancing = stream.ViewInstancingDesc;
	D3D12_VIEW_INSTANCE_LOCATION viewInstances[D3D12_MAX_VIEW_INSTANCE_COUNT];
	viewInstancing.ViewInstanceCount = pDesc->NumViewInstances;
	for (uint8_t i = 0; i < pDesc->NumViewInstances; ++i)
	{
		viewInstances[i].ViewportArrayIndex = pDesc->ViewInstances[i].ViewportArrayIndex;
		viewInstances[i].RenderTargetArrayIndex = pDesc->ViewInstances[i].RenderTargetArrayIndex;
	}
	viewInstancing.pViewInstanceLocations = viewInstances;
	viewInstancing.Flags = GetDX12ViewInstanceFlags(pDesc->ViewInstanceFlags);
}
