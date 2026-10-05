//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGGenericState_VK.h"
#include "XUSGGenericState_DX12.h"

using namespace std;
using namespace XUSG;
using namespace Generic;

State_VK::State_VK() :
	m_pipeline(nullptr)
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

void State_VK::SetPipelineLayout(const PipelineLayout& layout)
{
	m_pKey->Layout = layout;
}

void State_VK::SetShaderLibrary(uint8_t index, const Blob& shaderLib)
{
	assert(index < Shader::Stage::NUM_STAGE);
	m_pKey->ShaderLibs[index] = shaderLib;
}

void State_VK::SetShader(Shader::Stage stage, uint8_t libIndex, const wchar_t* shaderName)
{
	m_pKey->ShaderStages[stage].LibIndex = libIndex;
	m_pKey->ShaderStages[stage].ShaderName = shaderName;
}

void State_VK::SetProgram(const wchar_t* programName)
{
	m_pKey->Program = programName;
}

void State_VK::SetNodeMask(uint32_t nodeMask)
{
	m_pKey->NodeMask = nodeMask;
}

void State_VK::SetFlags(PipelineFlag flag)
{
	m_pKey->Flags = flag;
}

void State_VK::OMSetBlendState(const Blend* pBlend, uint32_t sampleMask)
{
	m_pKey->pBlend = pBlend;
	m_pKey->SampleMask = sampleMask;
}

void State_VK::RSSetState(const Rasterizer* pRasterizer)
{
	m_pKey->pRasterizer = pRasterizer;
}

void State_VK::DSSetState(const DepthStencil* pDepthStencil)
{
	m_pKey->pDepthStencil = pDepthStencil;
}

void State_VK::OMSetBlendState(BlendPreset preset, PipelineLib* pPipelineLib, uint8_t numColorRTs, uint32_t sampleMask)
{
	OMSetBlendState(pPipelineLib->GetBlend(preset, numColorRTs), sampleMask);
}

void State_VK::RSSetState(RasterizerPreset preset, PipelineLib* pPipelineLib)
{
	RSSetState(pPipelineLib->GetRasterizer(preset));
}

void State_VK::DSSetState(DepthStencilPreset preset, PipelineLib* pPipelineLib)
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
	m_pipeline = pPipelineLib->CreatePipeline(this, name);

	return m_pipeline;
}

Pipeline State_VK::GetPipeline(PipelineLib* pPipelineLib, const wchar_t* name)
{
	m_pipeline = pPipelineLib->GetPipeline(this, name);

	return m_pipeline;
}

PipelineLayout State_VK::GetPipelineLayout() const
{
	return m_pKey->Layout;
}

Blob State_VK::GetShaderLibrary(uint8_t index) const
{
	assert(index < Shader::Stage::NUM_STAGE);

	return m_pKey->ShaderLibs[index];
}

uint8_t State_VK::GetShaderLibraryIndex(Shader::Stage stage) const
{
	return m_pKey->ShaderStages[stage].LibIndex;
}

const wchar_t* State_VK::GetShaderName(Shader::Stage stage) const
{
	return m_pKey->ShaderStages[stage].ShaderName;
}

const wchar_t* State_VK::GetProgramName() const
{
	return m_pKey->Program;
}

ProgramIdentifier State_VK::GetProgramIdentifier(const wchar_t* programName) const
{
	assert(!"GetProgramIdentifier is not implemented on Vulkan");
	(void)programName;

	return {};
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
	m_pDevice(nullptr),
	m_device(VK_NULL_HANDLE)
{
	InitGraphicsPresets(m_pfnBlends, m_pfnRasterizers, m_pfnDepthStencils);
}

PipelineLib_VK::PipelineLib_VK(const Device* pDevice) :
	PipelineLib_VK()
{
	SetDevice(pDevice);
}

PipelineLib_VK::~PipelineLib_VK()
{
	for (auto& pipeline : m_pipelines) ReleaseVKPipeline(m_device, pipeline.second);
}

void PipelineLib_VK::SetDevice(const Device* pDevice)
{
	m_pDevice = dynamic_cast<const Device_VK*>(pDevice);
	assert(m_pDevice);
	m_device = m_pDevice->GetDevice();
}

void PipelineLib_VK::SetPipeline(const State* pState, const Pipeline& pipeline)
{
	lock_guard<mutex> lock(m_mtx);
	const auto p = dynamic_cast<const State_VK*>(pState);
	assert(p);
	const auto& key = p->GetKey();
	const auto incoming = static_cast<Pipeline_VK*>(pipeline);
	RetainVKPipeline(incoming);
	const auto existing = m_pipelines.find(key);
	if (existing != m_pipelines.end()) ReleaseVKPipeline(m_device, existing->second);
	m_pipelines[key] = incoming;
}

void PipelineLib_VK::SetInputLayout(uint32_t index, uint32_t numElements, const InputElement* pElements)
{
	m_inputLayoutLib.SetLayout(index, numElements, pElements);
}

const InputLayout* PipelineLib_VK::GetInputLayout(uint32_t index)
{
	return m_inputLayoutLib.GetLayout(index);
}

const InputLayout* PipelineLib_VK::CreateInputLayout(uint32_t numElements, const InputElement* pElements)
{
	return m_inputLayoutLib.CreateLayout(numElements, pElements);
}

Pipeline PipelineLib_VK::CreatePipeline(const State* pState, const wchar_t* name)
{
	const auto p = dynamic_cast<const State_VK*>(pState);
	assert(p);
	lock_guard<mutex> lock(m_mtx);

	return createPipeline(p->GetKey(), name);
}

Pipeline PipelineLib_VK::GetPipeline(const State* pState, const wchar_t* name)
{
	const auto p = dynamic_cast<const State_VK*>(pState);
	assert(p);
	lock_guard<mutex> lock(m_mtx);

	return getPipeline(p->GetKey(), name);
}

ProgramIdentifier PipelineLib_VK::GetProgramIdentifier(const Pipeline& stateObject, const wchar_t* programName) const
{
	assert(!"GetProgramIdentifier is not implemented on Vulkan");
	(void)stateObject;
	(void)programName;

	return {};
}

const Blend* PipelineLib_VK::GetBlend(BlendPreset preset, uint8_t numColorRTs)
{
	if (!m_blends[preset]) m_blends[preset] = make_unique<Blend>(m_pfnBlends[preset](numColorRTs));

	return m_blends[preset].get();
}

const Rasterizer* PipelineLib_VK::GetRasterizer(RasterizerPreset preset)
{
	if (!m_rasterizers[preset]) m_rasterizers[preset] = make_unique<Rasterizer>(m_pfnRasterizers[preset]());

	return m_rasterizers[preset].get();
}

const Graphics::DepthStencil* PipelineLib_VK::GetDepthStencil(DepthStencilPreset preset)
{
	if (!m_depthStencils[preset]) m_depthStencils[preset] = make_unique<Graphics::DepthStencil>(m_pfnDepthStencils[preset]());

	return m_depthStencils[preset].get();
}

Pipeline PipelineLib_VK::createPipeline(const string& key, const wchar_t* name)
{
	const auto pDesc = reinterpret_cast<const PipelineDesc*>(key.data());
	Blob shaders[Shader::Stage::NUM_STAGE] = {};
	const char* entries[Shader::Stage::NUM_STAGE] = {};
	string entryStorage[Shader::Stage::NUM_STAGE];
	auto graphics = false;
	auto compute = false;
	for (auto i = 0u; i < Shader::Stage::NUM_STAGE; ++i)
	{
		if (!pDesc->ShaderStages[i].ShaderName) continue;
		const auto stage = static_cast<Shader::Stage>(i);
		shaders[i] = pDesc->ShaderLibs[pDesc->ShaderStages[i].LibIndex];
		entryStorage[i] = WideToUtf8(pDesc->ShaderStages[i].ShaderName);
		entries[i] = entryStorage[i].c_str();
		if (stage == Shader::CS) compute = true;
		else graphics = true;
	}

	Pipeline_VK* pipeline = nullptr;
	if (graphics)
	{
		GraphicsPipelineCreateVK desc = {};
		desc.pLayout = static_cast<const PipelineLayoutObject*>(pDesc->Layout);
		desc.pShaders = shaders;
		desc.pEntryPoints = entries;
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
		pipeline = CreateVKGraphicsPipeline(m_pDevice, desc);
	}
	else if (compute)
		pipeline = CreateVKComputePipeline(m_pDevice, static_cast<const PipelineLayoutObject*>(pDesc->Layout),
			shaders[Shader::CS], entries[Shader::CS], name);
	if (!pipeline) return nullptr;

	const auto existing = m_pipelines.find(key);
	if (existing != m_pipelines.end()) ReleaseVKPipeline(m_device, existing->second);
	m_pipelines[key] = pipeline;

	return pipeline;
}

Pipeline PipelineLib_VK::getPipeline(const string& key, const wchar_t* name)
{
	const auto pipeline = m_pipelines.find(key);
	if (pipeline == m_pipelines.cend()) return createPipeline(key, name);

	return pipeline->second;
}
