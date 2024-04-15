//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSG_DX12.h"
#include "Core/XUSGCommand_DX12.h"
#include "Core/XUSGResource_DX12.h"
#include "Core/XUSGEnum_DX12.h"
#include "XUSGProgramState_DX12.h"
#include "XUSGUltimate_DX12.h"

using namespace std;
using namespace XUSG;
using namespace GenericProgram;

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

void State_DX12::SetShaderLibrary(uint8_t index, const Blob& shaderLib)
{
	assert(index < Shader::Stage::NUM_STAGE);
	m_pKey->ShaderLibs[index] = shaderLib;
}

void State_DX12::SetShader(Shader::Stage stage, uint8_t libIndex, const wchar_t* shaderName)
{
	auto& shaderStage = m_pKey->ShaderStages[stage];
	shaderStage.LibIndex = libIndex;
	shaderStage.ShaderName = shaderName;
}

void State_DX12::SetProgram(const wchar_t* programName)
{
	m_pKey->Program = programName;
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

const string& State_DX12::GetKey() const
{
	return m_key;
}

//--------------------------------------------------------------------------------------

PipelineLib_DX12::PipelineLib_DX12() :
	m_device(nullptr),
	m_stateObjects(),
	m_blends(),
	m_rasterizers(),
	m_depthStencils()
{
	// Blend states
	m_pfnBlends[BlendPreset::DEFAULT_OPAQUE] = Graphics::PipelineLib::DefaultOpaque;
	m_pfnBlends[BlendPreset::PREMULTIPLITED] = Graphics::PipelineLib::Premultiplied;
	m_pfnBlends[BlendPreset::ADDTIVE] = Graphics::PipelineLib::Additive;
	m_pfnBlends[BlendPreset::NON_PRE_MUL] = Graphics::PipelineLib::NonPremultiplied;
	m_pfnBlends[BlendPreset::NON_PREMUL_RT0] = Graphics::PipelineLib::NonPremultipliedRT0;
	m_pfnBlends[BlendPreset::ALPHA_TO_COVERAGE] = Graphics::PipelineLib::AlphaToCoverage;
	m_pfnBlends[BlendPreset::ACCUMULATIVE] = Graphics::PipelineLib::Accumulative;
	m_pfnBlends[BlendPreset::AUTO_NON_PREMUL] = Graphics::PipelineLib::AutoNonPremultiplied;
	m_pfnBlends[BlendPreset::ZERO_ALPHA_PREMUL] = Graphics::PipelineLib::ZeroAlphaNonPremultiplied;
	m_pfnBlends[BlendPreset::MULTIPLITED] = Graphics::PipelineLib::Multiplied;
	m_pfnBlends[BlendPreset::WEIGHTED_PREMUL] = Graphics::PipelineLib::WeightedPremul;
	m_pfnBlends[BlendPreset::WEIGHTED_PREMUL_PER_RT] = Graphics::PipelineLib::WeightedPremulPerRT;
	m_pfnBlends[BlendPreset::WEIGHTED_PER_RT] = Graphics::PipelineLib::WeightedPerRT;
	m_pfnBlends[BlendPreset::SELECT_MIN] = Graphics::PipelineLib::SelectMin;
	m_pfnBlends[BlendPreset::SELECT_MAX] = Graphics::PipelineLib::SelectMax;

	// Rasterizer states
	m_pfnRasterizers[RasterizerPreset::CULL_BACK] = Graphics::PipelineLib::CullBack;
	m_pfnRasterizers[RasterizerPreset::CULL_NONE] = Graphics::PipelineLib::CullNone;
	m_pfnRasterizers[RasterizerPreset::CULL_FRONT] = Graphics::PipelineLib::CullFront;
	m_pfnRasterizers[RasterizerPreset::FILL_WIREFRAME] = Graphics::PipelineLib::FillWireframe;

	// Depth stencil states
	m_pfnDepthStencils[DepthStencilPreset::DEFAULT_LESS] = Graphics::PipelineLib::DepthStencilDefault;
	m_pfnDepthStencils[DepthStencilPreset::DEPTH_STENCIL_NONE] = Graphics::PipelineLib::DepthStencilNone;
	m_pfnDepthStencils[DepthStencilPreset::DEPTH_READ_LESS] = Graphics::PipelineLib::DepthRead;
	m_pfnDepthStencils[DepthStencilPreset::DEPTH_READ_LESS_EQUAL] = Graphics::PipelineLib::DepthReadLessEqual;
	m_pfnDepthStencils[DepthStencilPreset::DEPTH_READ_EQUAL] = Graphics::PipelineLib::DepthReadEqual;
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
	m_stateObjects[key] = pipeline;
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
	return createStateObject(pState->GetKey(), name).get();
}

Pipeline PipelineLib_DX12::GetPipeline(const State* pState, const wchar_t* name)
{
	return getStateObject(pState->GetKey(), name).get();
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

const GenericProgram::DepthStencil* PipelineLib_DX12::GetDepthStencil(DepthStencilPreset preset)
{
	if (m_depthStencils[preset] == nullptr)
		m_depthStencils[preset] = make_unique<DepthStencil>(m_pfnDepthStencils[preset]());

	return m_depthStencils[preset].get();
}

com_ptr<ID3D12StateObject> PipelineLib_DX12::createStateObject(const string& key, const wchar_t* name)
{
	const auto pDesc = reinterpret_cast<const PipelineDesc*>(key.data());
	CD3DX12_STATE_OBJECT_DESC pPsoDesc(D3D12_STATE_OBJECT_TYPE_EXECUTABLE);

	// Program
	// Defines the shader program name.
	auto pProgram = pPsoDesc.CreateSubobject<CD3DX12_GENERIC_PROGRAM_SUBOBJECT>();
	pProgram->SetProgramName(pDesc->Program);

	// Root signature
	if (pDesc->Layout)
	{
		const auto pRootSignature = pPsoDesc.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
		pRootSignature->SetRootSignature(static_cast<ID3D12RootSignature*>(pDesc->Layout));
	}

	// DXIL library
	const uint8_t numLibs = Shader::Stage::NUM_STAGE;
	vector<const wchar_t*> libShaderNames[numLibs];
	for (uint8_t i = 0; i < Shader::Stage::NUM_STAGE; ++i)
	{
		const auto shaderStage = pDesc->ShaderStages[i];
		if (shaderStage.ShaderName)
		{
			libShaderNames[shaderStage.LibIndex].emplace_back(shaderStage.ShaderName);
			pProgram->AddExport(shaderStage.ShaderName);
		}
	}

	for (uint8_t i = 0; i < numLibs; ++i)
	{
		if (pDesc->ShaderLibs[i])
		{
			const auto pLib = pPsoDesc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
			const auto libDXIL = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pDesc->ShaderLibs[i]));
			pLib->SetDXILLibrary(&libDXIL);

			const auto& shaderNames = libShaderNames[i];
			if (!shaderNames.empty()) // Export shaders
				pLib->DefineExports(shaderNames.data(), static_cast<uint32_t>(shaderNames.size()));
			// Else, use default shader exports for a DXIL library/collection subobject ~ surface all shaders.
		}
	}

	// Blend state
	if (pDesc->pBlend)
	{
		const auto pBlend = pPsoDesc.CreateSubobject<CD3DX12_BLEND_SUBOBJECT>();
		pBlend->SetAlphaToCoverageEnable(pDesc->pBlend->AlphaToCoverageEnable);
		pBlend->SetIndependentBlendEnable(pDesc->pBlend->IndependentBlendEnable);
		for (uint8_t i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		{
			const auto& src = pDesc->pBlend->RenderTargets[i];
			D3D12_RENDER_TARGET_BLEND_DESC dst;
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
			pBlend->SetRenderTarget(i, dst);
		}
		pProgram->AddSubobject(*pBlend);
	}
	
	// Sample mask
	if (pDesc->SampleMask == UINT_MAX)
	{
		const auto pSampleMask = pPsoDesc.CreateSubobject<CD3DX12_SAMPLE_MASK_SUBOBJECT>();
		pSampleMask->SetSampleMask(pDesc->SampleMask);
		pProgram->AddSubobject(*pSampleMask);
	}

	// Rasterizer state
	if (pDesc->pRasterizer)
	{
		const auto pRasterizer = pPsoDesc.CreateSubobject<CD3DX12_RASTERIZER_SUBOBJECT>();
		pRasterizer->SetFillMode(GetDX12FillMode(pDesc->pRasterizer->Fill));
		pRasterizer->SetCullMode(GetDX12CullMode(pDesc->pRasterizer->Cull));
		pRasterizer->SetFrontCounterClockwise(pDesc->pRasterizer->FrontCounterClockwise ? TRUE : FALSE);
		pRasterizer->SetDepthBias(pDesc->pRasterizer->DepthBias);
		pRasterizer->SetDepthBiasClamp(pDesc->pRasterizer->DepthBiasClamp);
		pRasterizer->SetSlopeScaledDepthBias(pDesc->pRasterizer->SlopeScaledDepthBias);
		pRasterizer->SetDepthClipEnable(pDesc->pRasterizer->DepthClipEnable ? TRUE : FALSE);
		pRasterizer->SetLineRasterizationMode(GetDX12LineRasterizationMode(pDesc->pRasterizer->LineRasterizationMode));
		pRasterizer->SetForcedSampleCount(pDesc->pRasterizer->ForcedSampleCount);
		pRasterizer->SetConservativeRaster(pDesc->pRasterizer->ConservativeRaster ?
			D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF);
		pProgram->AddSubobject(*pRasterizer);
	}

	// Depth-stencil state
	if (pDesc->pDepthStencil)
	{
		const auto pDepthStencil = pPsoDesc.CreateSubobject<CD3DX12_DEPTH_STENCIL2_SUBOBJECT>();
		pDepthStencil->SetDepthEnable(pDesc->pDepthStencil->DepthEnable ? TRUE : FALSE);
		pDepthStencil->SetDepthWriteMask(pDesc->pDepthStencil->DepthWriteMask ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO);
		pDepthStencil->SetDepthFunc(GetDX12ComparisonFunc(pDesc->pDepthStencil->Comparison));
		pDepthStencil->SetStencilEnable(pDesc->pDepthStencil->StencilEnable ? TRUE : FALSE);
		D3D12_DEPTH_STENCILOP_DESC1 frontFace, backFace;
		frontFace.StencilFailOp = GetDX12StencilOp(pDesc->pDepthStencil->FrontFace.StencilFailOp);
		frontFace.StencilDepthFailOp = GetDX12StencilOp(pDesc->pDepthStencil->FrontFace.StencilDepthFailOp);
		frontFace.StencilPassOp = GetDX12StencilOp(pDesc->pDepthStencil->FrontFace.StencilPassOp);
		frontFace.StencilFunc = GetDX12ComparisonFunc(pDesc->pDepthStencil->FrontFace.StencilFunc);
		frontFace.StencilReadMask = pDesc->pDepthStencil->FrontFace.StencilReadMask;
		frontFace.StencilWriteMask = pDesc->pDepthStencil->FrontFace.StencilWriteMask;
		pDepthStencil->SetFrontFace(frontFace);
		backFace.StencilFailOp = GetDX12StencilOp(pDesc->pDepthStencil->BackFace.StencilFailOp);
		backFace.StencilDepthFailOp = GetDX12StencilOp(pDesc->pDepthStencil->BackFace.StencilDepthFailOp);
		backFace.StencilPassOp = GetDX12StencilOp(pDesc->pDepthStencil->BackFace.StencilPassOp);
		backFace.StencilFunc = GetDX12ComparisonFunc(pDesc->pDepthStencil->BackFace.StencilFunc);
		backFace.StencilReadMask = pDesc->pDepthStencil->BackFace.StencilReadMask;
		backFace.StencilWriteMask = pDesc->pDepthStencil->BackFace.StencilWriteMask;
		pDepthStencil->SetBackFace(frontFace);
		pDepthStencil->SetDepthBoundsTestEnable(pDesc->pDepthStencil->DepthBoundsTestEnable ? TRUE : FALSE);
		pProgram->AddSubobject(*pDepthStencil);
	}

	// Input layout
	if (pDesc->pInputLayout)
	{
		const auto pInputLayout = pPsoDesc.CreateSubobject<CD3DX12_INPUT_LAYOUT_SUBOBJECT>();
		const auto numElements = static_cast<uint32_t>(pDesc->pInputLayout->size());
		for (auto i = 0u; i < numElements; ++i)
		{
			const auto& src = pDesc->pInputLayout->at(i);
			D3D12_INPUT_ELEMENT_DESC dst;
			dst.SemanticName = src.SemanticName;
			dst.SemanticIndex = src.SemanticIndex;
			dst.Format = GetDXGIFormat(src.Format);
			dst.InputSlot = src.InputSlot;
			dst.AlignedByteOffset = src.AlignedByteOffset != XUSG_APPEND_ALIGNED_ELEMENT ? src.AlignedByteOffset : D3D12_APPEND_ALIGNED_ELEMENT;
			dst.InputSlotClass = GetDX12InputClassification(src.InputSlotClass);
			dst.InstanceDataStepRate = src.InstanceDataStepRate;
			pInputLayout->AddInputLayoutElementDesc(dst);
		}
		pProgram->AddSubobject(*pInputLayout);
	}

	// Primitive Topology
	if (pDesc->PrimTopologyType != PrimitiveTopologyType::UNDEFINED)
	{
		const auto pPrimitiveTopology = pPsoDesc.CreateSubobject<CD3DX12_PRIMITIVE_TOPOLOGY_SUBOBJECT>();
		pPrimitiveTopology->SetPrimitiveTopologyType(GetDX12PrimitiveTopologyType(pDesc->PrimTopologyType));
		pProgram->AddSubobject(*pPrimitiveTopology);
	}

	// RTV formats
	if (pDesc->NumRenderTargets)
	{
		const auto pRTVFormats = pPsoDesc.CreateSubobject<CD3DX12_RENDER_TARGET_FORMATS_SUBOBJECT>();
		pRTVFormats->SetNumRenderTargets(pDesc->NumRenderTargets);
		for (uint8_t i = 0; i < pDesc->NumRenderTargets; ++i)
			pRTVFormats->SetRenderTargetFormat(i, GetDXGIFormat(pDesc->RTVFormats[i]));
		pProgram->AddSubobject(*pRTVFormats);
	}

	// DSV format
	if (pDesc->DSVFormat != Format::UNKNOWN)
	{
		const auto pDSVFormat = pPsoDesc.CreateSubobject<CD3DX12_DEPTH_STENCIL_FORMAT_SUBOBJECT>();
		pDSVFormat->SetDepthStencilFormat(GetDXGIFormat(pDesc->DSVFormat));
		pProgram->AddSubobject(*pDSVFormat);
	}

	// Sample desc
	if (pDesc->SampleCount > 1 || pDesc->SampleQuality)
	{
		const auto pSampleDesc = pPsoDesc.CreateSubobject<CD3DX12_SAMPLE_DESC_SUBOBJECT>();
		pSampleDesc->SetCount(pDesc->SampleCount);
		pSampleDesc->SetQuality(pDesc->SampleQuality);
		pProgram->AddSubobject(*pSampleDesc);
	}

	// Node mask
	if (pDesc->NodeMask)
	{
		const auto pNodeMask = pPsoDesc.CreateSubobject<CD3DX12_NODE_MASK_SUBOBJECT>();
		pNodeMask->SetNodeMask(pDesc->NodeMask);
		pProgram->AddSubobject(*pNodeMask);
	}

	// Flags
	if (pDesc->Flags != PipelineFlag::NONE)
	{
		const auto pFlags = pPsoDesc.CreateSubobject<CD3DX12_FLAGS_SUBOBJECT>();
		pFlags->SetFlags(GetDX12PipelineFlags(pDesc->Flags));
		pProgram->AddSubobject(*pFlags);
	}

	// IB strip cube value
	if (pDesc->IBStripCutValue)
	{
		const auto pIBStripCutValue = pPsoDesc.CreateSubobject<CD3DX12_IB_STRIP_CUT_VALUE_SUBOBJECT>();
		switch (static_cast<IBStripCutValue>(pDesc->IBStripCutValue))
		{
		case IBStripCutValue::FFFF:
			pIBStripCutValue->SetIBStripCutValue(D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFF);
			break;
		case IBStripCutValue::FFFFFFFF:
			pIBStripCutValue->SetIBStripCutValue(D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF);
			break;
		default:
			assert(D3D12_INDEX_BUFFER_STRIP_CUT_VALUE(*pIBStripCutValue) == D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED);
		}
		pProgram->AddSubobject(*pIBStripCutValue);
	}

	// View instancing
	if (pDesc->NumViewInstances)
	{
		const auto pViewInstancing = pPsoDesc.CreateSubobject<CD3DX12_VIEW_INSTANCING_SUBOBJECT>();
		for (uint8_t i = 0; i < pDesc->NumViewInstances; ++i)
		{
			D3D12_VIEW_INSTANCE_LOCATION viewInstance;
			viewInstance.ViewportArrayIndex = pDesc->ViewInstances[i].ViewportArrayIndex;
			viewInstance.RenderTargetArrayIndex = pDesc->ViewInstances[i].RenderTargetArrayIndex;
			pViewInstancing->AddViewInstanceLocation(viewInstance);
		}
		pViewInstancing->SetFlags(GetDX12ViewInstanceFlags(pDesc->ViewInstanceFlags));
		pProgram->AddSubobject(*pViewInstancing);
	}

	// Create pipeline
	com_ptr<ID3D12StateObject> stateObject = nullptr;
	com_ptr<ID3D12Device9> dxDevice;
	V_RETURN(m_device->QueryInterface(IID_PPV_ARGS(&dxDevice)), cerr, nullptr);
	H_RETURN(dxDevice->CreateStateObject(pPsoDesc, IID_PPV_ARGS(&stateObject)), cerr,
		L"Couldn't create DirectX generic state object.\n", stateObject.get());

	if (name) stateObject->SetName(name);
	m_stateObjects[key] = stateObject;

	return stateObject;
}

com_ptr<ID3D12StateObject> PipelineLib_DX12::getStateObject(const string& key, const wchar_t* name)
{
	const auto pStateObject = m_stateObjects.find(key);

	// Create one, if it does not exist
	if (pStateObject == m_stateObjects.end()) return createStateObject(key, name);

	return pStateObject->second;
}
