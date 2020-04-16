//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGRayTracingState_DX12.h"

using namespace std;
using namespace XUSG;
using namespace XUSG::RayTracing;

State_DX12::State_DX12() :
	m_keyHitGroups(0),
	m_keyLocalPipelineLayouts(0),
	m_isComplete(false)
{
	// Default state
	m_key.resize(sizeof(KeyHeader));
	m_pKeyHeader = reinterpret_cast<KeyHeader*>(&m_key[0]);
	memset(m_pKeyHeader, 0, sizeof(KeyHeader));
}

State_DX12::~State_DX12()
{
}

void State_DX12::SetShaderLibrary(Blob shaderLib)
{
	m_isComplete = false;
	m_pKeyHeader->ShaderLib = shaderLib.get();
}

void State_DX12::SetHitGroup(uint32_t index, const void* hitGroup, const void* closestHitShader,
	const void* anyHitShader, const void* intersectionShader, HitGroupType type)
{
	m_isComplete = false;

	if (index >= m_keyHitGroups.size())
		m_keyHitGroups.resize(index + 1);

	auto& keyHitGroup = m_keyHitGroups[index];
	keyHitGroup.HitGroup = hitGroup;
	keyHitGroup.ClosestHitShader = closestHitShader;
	keyHitGroup.AnyHitShader = anyHitShader;
	keyHitGroup.IntersectionShader = intersectionShader;
	keyHitGroup.Type = static_cast<uint8_t>(type);
}

void State_DX12::SetShaderConfig(uint32_t maxPayloadSize, uint32_t maxAttributeSize)
{
	m_isComplete = false;
	m_pKeyHeader->MaxPayloadSize = maxPayloadSize;
	m_pKeyHeader->MaxAttributeSize = maxAttributeSize;
}

void State_DX12::SetLocalPipelineLayout(uint32_t index, const XUSG::PipelineLayout& layout, uint32_t numShaders, const void** pShaders)
{
	m_isComplete = false;

	if (index >= m_keyLocalPipelineLayouts.size())
		m_keyLocalPipelineLayouts.resize(index + 1);

	auto& keyLocalPipelineLayout = m_keyLocalPipelineLayouts[index];
	keyLocalPipelineLayout.Header.PipelineLayout = layout.get();
	keyLocalPipelineLayout.Header.NumShaders = numShaders;

	keyLocalPipelineLayout.Shaders.resize(numShaders);
	memcpy(keyLocalPipelineLayout.Shaders.data(), pShaders, sizeof(void*) * numShaders);
}

void State_DX12::SetGlobalPipelineLayout(const XUSG::PipelineLayout& layout)
{
	m_isComplete = false;
	m_pKeyHeader->GlobalPipelineLayout = layout.get();
}

void State_DX12::SetMaxRecursionDepth(uint32_t depth)
{
	m_isComplete = false;
	m_pKeyHeader->MaxRecursionDepth = depth;
}

RayTracing::Pipeline State_DX12::CreatePipeline(PipelineCache& pipelineCache, const wchar_t* name)
{
	return pipelineCache.CreatePipeline(*this, name);
}

RayTracing::Pipeline State_DX12::GetPipeline(PipelineCache& pipelineCache, const wchar_t* name)
{
	return pipelineCache.GetPipeline(*this, name);
}

const string& State_DX12::GetKey()
{
	if (!m_isComplete) complete();

	return m_key;
}

void State_DX12::complete()
{
	m_pKeyHeader->NumHitGroups = static_cast<uint32_t>(m_keyHitGroups.size());
	m_pKeyHeader->NumLocalPipelineLayouts = static_cast<uint32_t>(m_keyLocalPipelineLayouts.size());

	// Calculate total key size
	const auto sizeKeyHitGroups = sizeof(KeyHitGroup) * m_keyHitGroups.size();
	auto size = sizeof(KeyHeader) + sizeKeyHitGroups;
	size += sizeof(KeyLocalPipelineLayoutHeader) * m_keyLocalPipelineLayouts.size();
	for (const auto& keyLocalPipelineLayout : m_keyLocalPipelineLayouts)
		size += sizeof(void*) * keyLocalPipelineLayout.Header.NumShaders;

	m_key.resize(size);

	const auto pKeyHitGroups = reinterpret_cast<KeyHitGroup*>(&m_key[sizeof(KeyHeader)]);
	memcpy(pKeyHitGroups, m_keyHitGroups.data(), sizeKeyHitGroups);

	auto pKeyLocalPipelineLayoutHeader = reinterpret_cast<KeyLocalPipelineLayoutHeader*>
		(&m_key[sizeof(KeyHeader) + sizeKeyHitGroups]);
	for (const auto& keyLocalPipelineLayout : m_keyLocalPipelineLayouts)
	{
		// Copy the header
		memcpy(pKeyLocalPipelineLayoutHeader, &keyLocalPipelineLayout.Header, sizeof(KeyLocalPipelineLayoutHeader));

		// Update the pointer of association, and copy the associated shaders
		const auto pkeyLocalPipelineAssociation = reinterpret_cast<void**>(&pKeyLocalPipelineLayoutHeader[1]);
		memcpy(pkeyLocalPipelineAssociation, keyLocalPipelineLayout.Shaders.data(),
			sizeof(void*) * keyLocalPipelineLayout.Header.NumShaders);

		// Update the pointer of header
		pKeyLocalPipelineLayoutHeader = reinterpret_cast<KeyLocalPipelineLayoutHeader*>
			(&pkeyLocalPipelineAssociation[pKeyLocalPipelineLayoutHeader->NumShaders]);
	}

	m_isComplete = true;
}

//--------------------------------------------------------------------------------------

PipelineCache_DX12::PipelineCache_DX12() :
	//m_device(nullptr),
	m_pipelines()
{
}

PipelineCache_DX12::PipelineCache_DX12(const RayTracing::Device& device) :
	PipelineCache()
{
	SetDevice(device);
}

PipelineCache_DX12::~PipelineCache_DX12()
{
}

void PipelineCache_DX12::SetDevice(const RayTracing::Device& device)
{
	m_device = device;
}

void PipelineCache_DX12::SetPipeline(const string& key, const RayTracing::Pipeline& pipeline)
{
	m_pipelines[key] = pipeline;
}

RayTracing::Pipeline PipelineCache_DX12::CreatePipeline(State& state, const wchar_t* name)
{
	return createPipeline(state.GetKey(), name);
}

RayTracing::Pipeline PipelineCache_DX12::GetPipeline(State& state, const wchar_t* name)
{
	return getPipeline(state.GetKey(), name);
}

RayTracing::Pipeline PipelineCache_DX12::createPipeline(const string& key, const wchar_t* name)
{
	// Get header
	const auto& keyHeader = reinterpret_cast<const State_DX12::KeyHeader&>(key[0]);

	PipilineDesc pDesc(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);

	// DXIL library
	auto lib = pDesc.CreateSubobject<CD3D12_DXIL_LIBRARY_SUBOBJECT>();
	auto libdxil = Shader::ByteCode(static_cast<BlobType*>(keyHeader.ShaderLib));
	lib->SetDXILLibrary(&libdxil);
	// Use default shader exports for a DXIL library/collection subobject ~ surface all shaders.

	// Hit groups
	const auto pKeyHitGroups = reinterpret_cast<const State_DX12::KeyHitGroup*>(&key[sizeof(State_DX12::KeyHeader)]);
	for (auto i = 0u; i < keyHeader.NumHitGroups; ++i)
	{
		const auto& keyHitGroup = pKeyHitGroups[i];
		auto hitGroup = pDesc.CreateSubobject<CD3D12_HIT_GROUP_SUBOBJECT>();
		if (keyHitGroup.ClosestHitShader)
			hitGroup->SetClosestHitShaderImport(reinterpret_cast<const wchar_t*>(keyHitGroup.ClosestHitShader));
		if (keyHitGroup.AnyHitShader)
			hitGroup->SetAnyHitShaderImport(reinterpret_cast<const wchar_t*>(keyHitGroup.AnyHitShader));
		if (keyHitGroup.IntersectionShader)
			hitGroup->SetIntersectionShaderImport(reinterpret_cast<const wchar_t*>(keyHitGroup.IntersectionShader));

		hitGroup->SetHitGroupExport(reinterpret_cast<const wchar_t*>(keyHitGroup.HitGroup));
		hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE(keyHitGroup.Type));
	}

	// Shader config
	// Defines the maximum sizes in bytes for the ray rayPayload and attribute structure.
	auto shaderConfig = pDesc.CreateSubobject<CD3D12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
	shaderConfig->Config(keyHeader.MaxPayloadSize, keyHeader.MaxAttributeSize);

	// Local pipeline layout and shader association
	// This is a pipeline layout that enables a shader to have unique arguments that come from shader tables.
	auto pkeyLocalPipelineLayoutHeader = reinterpret_cast<const State_DX12::KeyLocalPipelineLayoutHeader*>
		(&key[sizeof(State_DX12::KeyHeader) + sizeof(State_DX12::KeyHitGroup) * keyHeader.NumHitGroups]);
	for (auto i = 0u; i < keyHeader.NumLocalPipelineLayouts; ++i)
	{
		const auto pipelineLayout = static_cast<ID3D12RootSignature*>(pkeyLocalPipelineLayoutHeader->PipelineLayout);

		// Set pipeline layout
		auto localPipelineLayout = pDesc.CreateSubobject<CD3D12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
		localPipelineLayout->SetRootSignature(pipelineLayout);

		// Shader association
		const auto pkeyLocalPipelineAssociation = reinterpret_cast<void* const*>(&pkeyLocalPipelineLayoutHeader[1]);
		auto rootSignatureAssociation = pDesc.CreateSubobject<CD3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
		rootSignatureAssociation->SetSubobjectToAssociate(*localPipelineLayout);
		for (auto j = 0u; j < pkeyLocalPipelineLayoutHeader->NumShaders; ++j)
			rootSignatureAssociation->AddExport(reinterpret_cast<const wchar_t*>(pkeyLocalPipelineAssociation[j]));

		// Update the pointer
		pkeyLocalPipelineLayoutHeader = reinterpret_cast<const State_DX12::KeyLocalPipelineLayoutHeader*>
			(&pkeyLocalPipelineAssociation[pkeyLocalPipelineLayoutHeader->NumShaders]);
	}

	// Global pipeline layout
	// This is a pipeline layout that is shared across all raytracing shaders invoked during a DispatchRays() call.
	auto globalRootSignature = pDesc.CreateSubobject<CD3D12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
	globalRootSignature->SetRootSignature(static_cast<ID3D12RootSignature*>(keyHeader.GlobalPipelineLayout));

	// Pipeline config
	// Defines the maximum TraceRay() recursion depth.
	auto pipelineConfig = pDesc.CreateSubobject<CD3D12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
	// PERFOMANCE TIP: Set max recursion depth as low as needed
	// as drivers may apply optimization strategies for low recursion depths.
	pipelineConfig->Config(keyHeader.MaxRecursionDepth);

	//PrintStateObjectDesc(desc);

	// Create pipeline
	Pipeline pipeline = {};
#if ENABLE_DXR_FALLBACK
	if (m_device.RaytracingAPI == RayTracing::API::FallbackLayer)
		H_RETURN(m_device.Fallback->CreateStateObject(pDesc, IID_PPV_ARGS(&pipeline.Fallback)), cerr,
			L"Couldn't create DirectX Raytracing state object.\n", pipeline)
	else // DirectX Raytracing
#endif
		H_RETURN(m_device.Native->CreateStateObject(pDesc, IID_PPV_ARGS(&pipeline.Native)), cerr,
			L"Couldn't create DirectX Raytracing state object.\n", pipeline);

	if (name)
	{
		if (pipeline.Native) pipeline.Native->SetName(name);
#if ENABLE_DXR_FALLBACK
		if (pipeline.Fallback) pipeline.Fallback->GetStateObject()->SetName(name);
#endif
	}

	return pipeline;
}

RayTracing::Pipeline PipelineCache_DX12::getPipeline(const string& key, const wchar_t* name)
{
	const auto pPipeline = m_pipelines.find(key);

	// Create one, if it does not exist
	if (pPipeline == m_pipelines.end())
	{
		const auto pipeline = createPipeline(key, name);
		m_pipelines[key] = pipeline;

		return pipeline;
	}

	return pPipeline->second;
}
