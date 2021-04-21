//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGComputeState_DX12.h"

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
	m_pKey->PipelineLayout = layout;
}

void State_DX12::SetShader(const Blob& shader)
{
	m_pKey->Shader = shader;
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
	m_pipelines()
{
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
	m_device = static_cast<ID3D12Device*>(device.GetHandle());
	assert(m_device);
}

void PipelineCache_DX12::SetPipeline(const string& key, const Pipeline& pipeline)
{
	m_pipelines[key] = static_cast<ID3D12PipelineState*>(pipeline);
}

Pipeline PipelineCache_DX12::CreatePipeline(const State& state, const wchar_t* name)
{
	return createPipeline(state.GetKey(), name);
}

Pipeline PipelineCache_DX12::GetPipeline(const State& state, const wchar_t* name)
{
	return getPipeline(state.GetKey(), name);
}

Pipeline PipelineCache_DX12::createPipeline(const string& key, const wchar_t* name)
{
	// Fill desc
	const auto pDesc = reinterpret_cast<const PipelineDesc*>(key.data());
	D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
	if (pDesc->PipelineLayout)
		desc.pRootSignature = static_cast<ID3D12RootSignature*>(pDesc->PipelineLayout);

	if (pDesc->Shader)
		desc.CS = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pDesc->Shader));

	desc.CachedPSO.pCachedBlob = pDesc->CachedPipeline;
	desc.CachedPSO.CachedBlobSizeInBytes = pDesc->CachedPipelineSize;
	desc.NodeMask = pDesc->NodeMask;

	// Create pipeline
	com_ptr<ID3D12PipelineState> pipeline;
	V_RETURN(m_device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pipeline)), cerr, nullptr);
	if (name) pipeline->SetName(name);
	m_pipelines[key] = pipeline;

	return pipeline.get();
}

Pipeline PipelineCache_DX12::getPipeline(const string& key, const wchar_t* name)
{
	const auto pPipeline = m_pipelines.find(key);

	// Create one, if it does not exist
	if (pPipeline == m_pipelines.end())
	{
		const auto pipeline = createPipeline(key, name);

		return pipeline;
	}

	return pPipeline->second.get();
}
