//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSG_DX12.h"
#include "XUSGComputeState_DX12.h"
#include "XUSGEnum_DX12.h"

using namespace std;
using namespace XUSG;
using namespace Compute;

State_DX12::State_DX12()
{
	// Default state
	m_key.resize(sizeof(PipelineDesc));
	m_pKey = reinterpret_cast<PipelineDesc*>(&m_key[0]);
	memset(m_pKey, 0, sizeof(PipelineDesc));
}

State_DX12::~State_DX12()
{
}

void State_DX12::SetPipelineLayout(const PipelineLayout& layout)
{
	m_pKey->Layout = layout;
}

void State_DX12::SetShader(const Blob& shader)
{
	m_pKey->Shader = shader;
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

Blob State_DX12::GetShader() const
{
	return m_pKey->Shader;
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

void State_DX12::GetHandleDesc(void* pHandleDesc) const
{
	PipelineLib_DX12::GetHandleDesc(pHandleDesc, GetKey());
}

const string& State_DX12::GetKey() const
{
	return m_key;
}

//--------------------------------------------------------------------------------------

PipelineLib_DX12::PipelineLib_DX12() :
	m_device(nullptr),
	m_pipelines()
{
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

Pipeline PipelineLib_DX12::createPipeline(const string& key, const wchar_t* name)
{
	// Fill desc
	D3D12_COMPUTE_PIPELINE_STATE_DESC desc;
	GetHandleDesc(&desc, key);

	// Create pipeline
	com_ptr<ID3D12PipelineState> pipeline;
	V_RETURN(m_device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pipeline)), cerr, nullptr);
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

void PipelineLib_DX12::GetHandleDesc(void* pHandleDesc, const string& key)
{
	const auto pDesc = reinterpret_cast<const PipelineDesc*>(key.data());
	auto& desc = *static_cast<D3D12_COMPUTE_PIPELINE_STATE_DESC*>(pHandleDesc);
	desc = {};

	// Fill desc
	if (pDesc->Layout)
		desc.pRootSignature = static_cast<ID3D12RootSignature*>(pDesc->Layout);

	if (pDesc->Shader)
		desc.CS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pDesc->Shader));

	if (pDesc->CachedPipeline)
		desc.CachedPSO.CachedBlobSizeInBytes = GetDX12BlobData(pDesc->CachedPipeline, desc.CachedPSO.pCachedBlob);

	desc.NodeMask = pDesc->NodeMask;
	desc.Flags = GetDX12PipelineFlags(pDesc->Flags);
}
