//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGComputeState_VK.h"
#include "XUSGComputeState_DX12.h"

using namespace std;
using namespace XUSG;
using namespace Compute;

State_VK::State_VK()
{
	m_key.resize(sizeof(PipelineDesc));
	m_pKey = reinterpret_cast<PipelineDesc*>(&m_key[0]);
	memset(m_pKey, 0, sizeof(PipelineDesc));
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

void State_VK::SetShader(const Blob& shader)
{
	m_pKey->Shader = shader;
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

Pipeline State_VK::CreatePipeline(PipelineLib* pPipelineLib, const wchar_t* name)
{
	return pPipelineLib->CreatePipeline(this, name);
}

Pipeline State_VK::GetPipeline(PipelineLib* pPipelineLib, const wchar_t* name)
{
	return pPipelineLib->GetPipeline(this, name);
}

const string& State_VK::GetKey() const
{
	return m_key;
}

PipelineLayout State_VK::GetPipelineLayout() const
{
	return m_pKey->Layout;
}

Blob State_VK::GetShader() const
{
	return m_pKey->Shader;
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

void State_VK::GetHandleDesc(void* pHandleDesc) const
{
	(void)pHandleDesc;
}

//--------------------------------------------------------------------------------------

PipelineLib_VK::PipelineLib_VK() :
	m_pDevice(nullptr)
{
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

Pipeline PipelineLib_VK::createPipeline(const string& key, const wchar_t* name)
{
	const auto pDesc = reinterpret_cast<const PipelineDesc*>(key.data());
	auto pipeline = CreateVKComputePipeline(m_pDevice, static_cast<const PipelineLayoutObject*>(pDesc->Layout),
		pDesc->Shader, "main", name);
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
