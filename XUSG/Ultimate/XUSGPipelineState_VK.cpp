//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGPipelineState_VK.h"
#include "XUSGPipelineState_DX12.h"

using namespace std;
using namespace XUSG;
using namespace Ultimate;

State_VK::State_VK()
{
	m_key.resize(sizeof(PipelineDesc));
	m_pKey = reinterpret_cast<PipelineDesc*>(&m_key[0]);
	memset(m_pKey, 0, sizeof(PipelineDesc));
	m_pKey->SampleMask = UINT_MAX;
	m_pKey->PrimTopologyType = PrimitiveTopologyType::TRIANGLE;
	m_pKey->SampleCount = 1;
}

State_VK::~State_VK()
{
}

void* State_VK::GetBackend()
{
	return this;
}

void State_VK::SetPipelineLayout(const PipelineLayout& layout)
{
	m_pKey->Layout = layout;
}

void State_VK::SetShader(Shader::Stage stage, const Blob& shader)
{
	m_pKey->Shaders[stage] = shader;
}

void State_VK::SetCachedPipeline(const Blob& cachedPipeline)
{
	m_pKey->CachedPipeline = cachedPipeline;
}

void State_VK::SetNodeMask(uint32_t nodeMask)
{
	m_pKey->NodeMask = nodeMask;
}

void State_VK::SetFlags(PipelineFlag flag)
{
	m_pKey->Flags = flag;
}

void State_VK::OMSetBlendState(const Graphics::Blend* pBlend, uint32_t sampleMask)
{
	m_pKey->pBlend = pBlend;
	m_pKey->SampleMask = sampleMask;
}

void State_VK::RSSetState(const Graphics::Rasterizer* pRasterizer)
{
	m_pKey->pRasterizer = pRasterizer;
}

void State_VK::DSSetState(const Graphics::DepthStencil* pDepthStencil)
{
	m_pKey->pDepthStencil = pDepthStencil;
}

void State_VK::OMSetBlendState(Graphics::BlendPreset preset, PipelineLib* pPipelineLib, uint8_t numColorRTs, uint32_t sampleMask)
{
	OMSetBlendState(pPipelineLib->GetBlend(preset, numColorRTs), sampleMask);
}

void State_VK::RSSetState(Graphics::RasterizerPreset preset, PipelineLib* pPipelineLib)
{
	RSSetState(pPipelineLib->GetRasterizer(preset));
}

void State_VK::DSSetState(Graphics::DepthStencilPreset preset, PipelineLib* pPipelineLib)
{
	DSSetState(pPipelineLib->GetDepthStencil(preset));
}

void State_VK::IASetInputLayout(const InputLayout* pLayout)
{
	m_pKey->pInputLayout = pLayout;
}

void State_VK::IASetPrimitiveTopologyType(PrimitiveTopologyType type)
{
	m_pKey->PrimTopologyType = type;
}

void State_VK::IASetIndexBufferStripCutValue(IBStripCutValue ibStripCutValue)
{
	m_pKey->IBStripCutValue = static_cast<uint8_t>(ibStripCutValue);
}

void State_VK::OMSetNumRenderTargets(uint8_t n)
{
	const auto numRTVFormats = static_cast<uint8_t>(size(m_pKey->RTVFormats));
	assert(n <= numRTVFormats);
	m_pKey->NumRenderTargets = n;

	for (auto i = n; i < numRTVFormats; ++i) OMSetRTVFormat(i, Format::UNKNOWN);
}

void State_VK::OMSetRTVFormat(uint8_t i, Format format)
{
	assert(i < size(m_pKey->RTVFormats));
	m_pKey->RTVFormats[i] = format;
}

void State_VK::OMSetRTVFormats(uint8_t n, const Format* formats)
{
	OMSetNumRenderTargets(n);

	for (uint8_t i = 0; i < n; ++i) OMSetRTVFormat(i, formats[i]);
}

void State_VK::OMSetDSVFormat(Format format)
{
	m_pKey->DSVFormat = format;
}

void State_VK::OMSetSample(uint8_t count, uint8_t quality)
{
	m_pKey->SampleCount = count;
	m_pKey->SampleQuality = quality;
}

void State_VK::SetNumViewInstances(uint8_t n, ViewInstanceFlag flags)
{
	const auto numViewInstances = static_cast<uint8_t>(size(m_pKey->ViewInstances));
	assert(n <= numViewInstances);
	m_pKey->NumViewInstances = n;
	m_pKey->ViewInstanceFlags = flags;

	for (auto i = n; i < numViewInstances; ++i) SetViewInstance(i, ViewInstance());
}

void State_VK::SetViewInstance(uint8_t i, const ViewInstance& viewInstance)
{
	assert(i < size(m_pKey->ViewInstances));
	m_pKey->ViewInstances[i] = viewInstance;
}

void State_VK::SetViewInstances(uint8_t n, const ViewInstance* viewInstances, ViewInstanceFlag flags)
{
	SetNumViewInstances(n, flags);

	for (auto i = 0u; i < n; ++i) SetViewInstance(i, viewInstances[i]);
}

Pipeline State_VK::CreatePipeline(PipelineLib* pPipelineLib, const wchar_t* name)
{
	return pPipelineLib->CreatePipeline(this, name);
}

Pipeline State_VK::GetPipeline(PipelineLib* pPipelineLib, const wchar_t* name)
{
	return pPipelineLib->GetPipeline(this, name);
}

PipelineLayout State_VK::GetPipelineLayout() const
{
	return m_pKey->Layout;
}

Blob State_VK::GetShader(Shader::Stage stage) const
{
	assert(stage < Shader::Stage::NUM_STAGE);

	return m_pKey->Shaders[stage];
}

Blob State_VK::GetCachedPipeline() const
{
	return m_pKey->CachedPipeline;
}

uint32_t State_VK::GetNodeMask() const
{
	return m_pKey->NodeMask;
}

PipelineFlag State_VK::GetFlags() const
{
	return m_pKey->Flags;
}

uint32_t State_VK::OMGetSampleMask() const
{
	return m_pKey->SampleMask;
}

const Graphics::Blend* State_VK::OMGetBlendState() const
{
	return m_pKey->pBlend;
}

const Graphics::Rasterizer* State_VK::RSGetState() const
{
	return m_pKey->pRasterizer;
}

const Graphics::DepthStencil* State_VK::DSGetState() const
{
	return m_pKey->pDepthStencil;
}

const InputLayout* State_VK::IAGetInputLayout() const
{
	return m_pKey->pInputLayout;
}

PrimitiveTopologyType State_VK::IAGetPrimitiveTopologyType() const
{
	return m_pKey->PrimTopologyType;
}

IBStripCutValue State_VK::IAGetIndexBufferStripCutValue() const
{
	return static_cast<IBStripCutValue>(m_pKey->IBStripCutValue);
}

uint8_t State_VK::OMGetNumRenderTargets() const
{
	return m_pKey->NumRenderTargets;
}

Format State_VK::OMGetRTVFormat(uint8_t i) const
{
	assert(i < m_pKey->NumRenderTargets);

	return m_pKey->RTVFormats[i];
}

Format State_VK::OMGetDSVFormat() const
{
	return m_pKey->DSVFormat;
}

uint8_t State_VK::OMGetSampleCount() const
{
	return m_pKey->SampleCount;
}

uint8_t State_VK::OMGetSampleQuality() const
{
	return m_pKey->SampleQuality;
}

uint8_t State_VK::GetNumViewInstances(uint8_t n, ViewInstanceFlag flags) const
{
	(void)n;
	(void)flags;

	return m_pKey->NumViewInstances;
}

ViewInstanceFlag State_VK::GetViewInstanceFlags() const
{
	return m_pKey->ViewInstanceFlags;
}

const ViewInstance& State_VK::GetViewInstance(uint8_t i) const
{
	assert(i < m_pKey->NumViewInstances);

	return m_pKey->ViewInstances[i];
}

const string& State_VK::GetKey() const
{
	return m_key;
}

//--------------------------------------------------------------------------------------

PipelineLib_VK::PipelineLib_VK() :
	m_pDevice(nullptr)
{
	InitGraphicsPresets(m_pfnBlends, m_pfnRasterizers, m_pfnDepthStencils);
}

PipelineLib_VK::PipelineLib_VK(Device* pDevice) :
	PipelineLib_VK()
{
	SetDevice(pDevice);
}

PipelineLib_VK::~PipelineLib_VK()
{
	if (m_pipelines.empty()) return;
	const auto device = m_pDevice->GetDevice();
	for (auto& pipeline : m_pipelines) ReleaseVKPipeline(device, pipeline.second);
}

void PipelineLib_VK::SetDevice(Device* pDevice)
{
	m_pDevice = AsVK(pDevice);
	assert(m_pDevice);
}

void PipelineLib_VK::SetPipeline(State* pState, const Pipeline& pipeline)
{
	lock_guard<mutex> lock(m_mtx);
	const auto p = AsVK(pState);
	assert(p);
	const auto& key = p->GetKey();
	const auto incoming = static_cast<Pipeline_VK*>(pipeline);
	RetainVKPipeline(incoming);
	const auto existing = m_pipelines.find(key);
	if (existing != m_pipelines.end()) ReleaseVKPipeline(m_pDevice->GetDevice(), existing->second);
	m_pipelines[key] = incoming;
}

void PipelineLib_VK::SetInputLayout(uint32_t index, uint32_t numElements, const InputElement* pElements)
{
	m_inputLayoutLib.SetLayout(index, numElements, pElements);
}

const InputLayout* PipelineLib_VK::GetInputLayout(uint32_t index) const
{
	return m_inputLayoutLib.GetLayout(index);
}

const InputLayout* PipelineLib_VK::CreateInputLayout(uint32_t numElements, const InputElement* pElements)
{
	return m_inputLayoutLib.CreateLayout(numElements, pElements);
}

Pipeline PipelineLib_VK::CreatePipeline(State* pState, const wchar_t* name)
{
	const auto p = AsVK(pState);
	assert(p);
	lock_guard<mutex> lock(m_mtx);

	return createPipeline(p->GetKey(), name);
}

Pipeline PipelineLib_VK::GetPipeline(State* pState, const wchar_t* name)
{
	const auto p = AsVK(pState);
	assert(p);
	lock_guard<mutex> lock(m_mtx);

	return getPipeline(p->GetKey(), name);
}

const Graphics::Blend* PipelineLib_VK::GetBlend(Graphics::BlendPreset preset, uint8_t numColorRTs)
{
	if (!m_blends[preset]) m_blends[preset] = make_unique<Graphics::Blend>(m_pfnBlends[preset](numColorRTs));

	return m_blends[preset].get();
}

const Graphics::Rasterizer* PipelineLib_VK::GetRasterizer(Graphics::RasterizerPreset preset)
{
	if (!m_rasterizers[preset]) m_rasterizers[preset] = make_unique<Graphics::Rasterizer>(m_pfnRasterizers[preset]());

	return m_rasterizers[preset].get();
}

const Graphics::DepthStencil* PipelineLib_VK::GetDepthStencil(Graphics::DepthStencilPreset preset)
{
	if (!m_depthStencils[preset]) m_depthStencils[preset] = make_unique<Graphics::DepthStencil>(m_pfnDepthStencils[preset]());

	return m_depthStencils[preset].get();
}

Pipeline PipelineLib_VK::createPipeline(const string& key, const wchar_t* name)
{
	const auto pDesc = reinterpret_cast<const PipelineDesc*>(key.data());
	GraphicsPipelineCreateVK desc = {};
	desc.pLayout = static_cast<const PipelineLayoutObject*>(pDesc->Layout);
	desc.pShaders = pDesc->Shaders;
	desc.ShaderCount = Shader::Stage::NUM_STAGE;
	desc.pBlend = pDesc->pBlend;
	desc.pRasterizer = pDesc->pRasterizer;
	desc.pDepthStencil = pDesc->pDepthStencil;
	desc.pInputLayout = pDesc->pInputLayout;
	desc.Topology = pDesc->PrimTopologyType;
	desc.NumRenderTargets = pDesc->NumRenderTargets;
	desc.RTVFormats = pDesc->RTVFormats;
	desc.DSVFormat = pDesc->DSVFormat;
	desc.SampleCount = pDesc->SampleCount;
	desc.Name = name;
	auto pipeline = CreateVKGraphicsPipeline(m_pDevice, desc);
	if (!pipeline) return nullptr;

	const auto existing = m_pipelines.find(key);
	if (existing != m_pipelines.end()) ReleaseVKPipeline(m_pDevice->GetDevice(), existing->second);
	m_pipelines[key] = pipeline;

	return pipeline;
}

Pipeline PipelineLib_VK::getPipeline(const string& key, const wchar_t* name)
{
	const auto pipeline = m_pipelines.find(key);
	if (pipeline == m_pipelines.cend()) return createPipeline(key, name);

	return pipeline->second;
}
