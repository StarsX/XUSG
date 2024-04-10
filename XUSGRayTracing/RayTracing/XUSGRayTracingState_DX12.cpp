//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSG_DX12.h"
#include "XUSGRayTracingState_DX12.h"

using namespace std;
using namespace XUSG;
using namespace XUSG::RayTracing;

State_DX12::State_DX12() :
	m_keyShaderLibs(0),
	m_keyHitGroups(0),
	m_keyLocalPipelineLayouts(0),
	m_isSerialized(false)
{
	// Default state
	m_key.resize(sizeof(KeyHeader));
	m_pKeyHeader = reinterpret_cast<KeyHeader*>(&m_key[0]);
	memset(m_pKeyHeader, 0, sizeof(KeyHeader));
}

State_DX12::~State_DX12()
{
}

void State_DX12::SetShaderLibrary(uint32_t index, const Blob& shaderLib, uint32_t numShaders, const wchar_t** pShaderNames)
{
	m_isSerialized = false;

	if (index >= m_keyShaderLibs.size())
		m_keyShaderLibs.resize(index + 1);

	auto& keyShaderLib = m_keyShaderLibs[index];
	keyShaderLib.Lib = shaderLib;
	keyShaderLib.Shaders.resize(numShaders);
	memcpy(keyShaderLib.Shaders.data(), pShaderNames, sizeof(wchar_t*) * numShaders);
}

void State_DX12::SetHitGroup(uint32_t index, const wchar_t* hitGroupName, const wchar_t* closestHitShaderName,
	const wchar_t* anyHitShaderName, const wchar_t* intersectionShaderName, HitGroupType type)
{
	const D3D12_HIT_GROUP_TYPE hitGroupTypes[] =
	{
		D3D12_HIT_GROUP_TYPE_TRIANGLES,
		D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE
	};

	m_isSerialized = false;

	if (index >= m_keyHitGroups.size())
		m_keyHitGroups.resize(index + 1);

	auto& keyHitGroup = m_keyHitGroups[index];
	keyHitGroup.HitGroup = hitGroupName;
	keyHitGroup.ClosestHitShader = closestHitShaderName;
	keyHitGroup.AnyHitShader = anyHitShaderName;
	keyHitGroup.IntersectionShader = intersectionShaderName;
	keyHitGroup.Type = hitGroupTypes[static_cast<uint8_t>(type)];
}

void State_DX12::SetShaderConfig(uint32_t maxPayloadSize, uint32_t maxAttributeSize)
{
	m_isSerialized = false;
	m_pKeyHeader->MaxPayloadSize = maxPayloadSize;
	m_pKeyHeader->MaxAttributeSize = maxAttributeSize;
}

void State_DX12::SetLocalPipelineLayout(uint32_t index, const XUSG::PipelineLayout& layout, uint32_t numShaders, const wchar_t** pShaderNames)
{
	m_isSerialized = false;

	if (index >= m_keyLocalPipelineLayouts.size())
		m_keyLocalPipelineLayouts.resize(index + 1);

	auto& keyLocalPipelineLayout = m_keyLocalPipelineLayouts[index];
	keyLocalPipelineLayout.Layout = layout;
	keyLocalPipelineLayout.Shaders.resize(numShaders);
	memcpy(keyLocalPipelineLayout.Shaders.data(), pShaderNames, sizeof(wchar_t*) * numShaders);
}

void State_DX12::SetGlobalPipelineLayout(const XUSG::PipelineLayout& layout)
{
	m_isSerialized = false;
	m_pKeyHeader->GlobalLayout = layout;
}

void State_DX12::SetMaxRecursionDepth(uint32_t depth)
{
	m_isSerialized = false;
	m_pKeyHeader->MaxRecursionDepth = depth;
}

void State_DX12::SetNodeMask(uint32_t nodeMask)
{
	m_isSerialized = false;
	m_pKeyHeader->NodeMask = nodeMask;
}

Pipeline State_DX12::CreatePipeline(PipelineLib* pPipelineCache, const wchar_t* name)
{
	return pPipelineCache->CreatePipeline(this, name);
}

Pipeline State_DX12::GetPipeline(PipelineLib* pPipelineCache, const wchar_t* name)
{
	return pPipelineCache->GetPipeline(this, name);
}

const string& State_DX12::GetKey()
{
	if (!m_isSerialized) serialize();

	return m_key;
}

const wchar_t* State_DX12::GetHitGroupName(uint32_t index)
{
	const auto pHitGroups = reinterpret_cast<const KeyHitGroup*>(&GetKey()[sizeof(KeyHeader)]);

	return static_cast<const wchar_t*>(pHitGroups[index].HitGroup);
}

uint32_t State_DX12::GetNumHitGroups()
{
	GetKey();

	return m_pKeyHeader->NumHitGroups;
}

void State_DX12::serialize()
{
	m_pKeyHeader->NumShaderLibs = static_cast<uint32_t>(m_keyShaderLibs.size());
	m_pKeyHeader->NumHitGroups = static_cast<uint32_t>(m_keyHitGroups.size());
	m_pKeyHeader->NumLocalPipelineLayouts = static_cast<uint32_t>(m_keyLocalPipelineLayouts.size());

	// Calculate total key size
	const auto sizeKeyHeader = sizeof(KeyHeader);
	const auto sizeKeyHitGroups = sizeof(KeyHitGroup) * m_keyHitGroups.size();

	auto sizeKeyShaderLibs = sizeof(KeyShaderLibHeader) * m_keyShaderLibs.size();
	for (const auto& keyShaderLib : m_keyShaderLibs)
		sizeKeyShaderLibs += sizeof(wchar_t*) * keyShaderLib.Shaders.size();

	auto sizeKeyLocalPipelineLayouts = sizeof(KeyLocalPipelineLayoutHeader) * m_keyLocalPipelineLayouts.size();
	for (const auto& keyLocalPipelineLayout : m_keyLocalPipelineLayouts)
		sizeKeyLocalPipelineLayouts += sizeof(wchar_t*) * keyLocalPipelineLayout.Shaders.size();

	m_key.resize(sizeKeyHeader + sizeKeyHitGroups + sizeKeyShaderLibs + sizeKeyLocalPipelineLayouts);
	m_pKeyHeader = reinterpret_cast<KeyHeader*>(&m_key[0]);

	if (m_pKeyHeader->NumHitGroups)
	{
		const auto pKeyHitGroups = reinterpret_cast<KeyHitGroup*>(&m_key[sizeKeyHeader]);
		memcpy(pKeyHitGroups, m_keyHitGroups.data(), sizeKeyHitGroups);
	}

	auto pKeyShaderLibHeader = reinterpret_cast<KeyShaderLibHeader*>(&m_key[sizeKeyHeader + sizeKeyHitGroups]);
	for (const auto& keyShaderLib : m_keyShaderLibs)
	{
		// Set the layout and number of associated shaders
		pKeyShaderLibHeader->Lib = keyShaderLib.Lib;
		pKeyShaderLibHeader->NumShaders = static_cast<uint32_t>(keyShaderLib.Shaders.size());

		// Update the pointer of the shaders, and copy the shaders
		const auto pKeyShaders = reinterpret_cast<wchar_t**>(&pKeyShaderLibHeader[1]);
		memcpy(pKeyShaders, keyShaderLib.Shaders.data(), sizeof(wchar_t*) * pKeyShaderLibHeader->NumShaders);

		// Update the pointer
		pKeyShaderLibHeader = reinterpret_cast<KeyShaderLibHeader*>(&pKeyShaders[pKeyShaderLibHeader->NumShaders]);
	}

	auto pKeyLocalPipelineLayoutHeader = reinterpret_cast<KeyLocalPipelineLayoutHeader*>
		(&m_key[sizeKeyHeader + sizeKeyHitGroups + sizeKeyShaderLibs]);
	for (const auto& keyLocalPipelineLayout : m_keyLocalPipelineLayouts)
	{
		// Set the layout and number of associated shaders
		pKeyLocalPipelineLayoutHeader->Layout = keyLocalPipelineLayout.Layout;
		pKeyLocalPipelineLayoutHeader->NumShaders = static_cast<uint32_t>(keyLocalPipelineLayout.Shaders.size());

		// Update the pointer of association, and copy the associated shaders
		const auto pKeyLocalPipelineAssociation = reinterpret_cast<wchar_t**>(&pKeyLocalPipelineLayoutHeader[1]);
		memcpy(pKeyLocalPipelineAssociation, keyLocalPipelineLayout.Shaders.data(),
			sizeof(wchar_t*) * pKeyLocalPipelineLayoutHeader->NumShaders);

		// Update the pointer
		pKeyLocalPipelineLayoutHeader = reinterpret_cast<KeyLocalPipelineLayoutHeader*>
			(&pKeyLocalPipelineAssociation[pKeyLocalPipelineLayoutHeader->NumShaders]);
	}

	m_isSerialized = true;
}

//--------------------------------------------------------------------------------------

PipelineLib_DX12::PipelineLib_DX12() :
	m_device(nullptr),
	m_stateObjects()
{
}

PipelineLib_DX12::PipelineLib_DX12(const RayTracing::Device* pDevice) :
	PipelineLib()
{
	SetDevice(pDevice);
}

PipelineLib_DX12::~PipelineLib_DX12()
{
}

void PipelineLib_DX12::SetDevice(const RayTracing::Device* pDevice)
{
	m_device = static_cast<ID3D12RaytracingFallbackDevice*>(pDevice->GetRTHandle());
	assert(m_device);
}

void PipelineLib_DX12::SetPipeline(const string& key, const Pipeline& pipeline)
{
	m_stateObjects[key] = static_cast<ID3D12RaytracingFallbackStateObject*>(pipeline);
}

Pipeline PipelineLib_DX12::CreatePipeline(State* pState, const wchar_t* name)
{
	return createStateObject(pState->GetKey(), name).get();
}

Pipeline PipelineLib_DX12::GetPipeline(State* pState, const wchar_t* name)
{
	return getStateObject(pState->GetKey(), name).get();
}

com_ptr<ID3D12RaytracingFallbackStateObject> PipelineLib_DX12::createStateObject(const string& key, const wchar_t* name)
{
	// Get header
	const auto& keyHeader = reinterpret_cast<const State_DX12::KeyHeader&>(key[0]);
	const auto sizeKeyHeader = sizeof(State_DX12::KeyHeader);
	const auto sizeKeyHitGroups = sizeof(State_DX12::KeyHitGroup) * keyHeader.NumHitGroups;
	auto sizeKeyShaderLibs = sizeof(State_DX12::KeyShaderLibHeader) * keyHeader.NumShaderLibs;

	CD3DX12_STATE_OBJECT_DESC pPsoDesc(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);

	// DXIL library
	auto pKeyShaderLibHeader = reinterpret_cast<const State_DX12::KeyShaderLibHeader*>
		(&key[sizeKeyHeader + sizeKeyHitGroups]);
	for (auto i = 0u; i < keyHeader.NumShaderLibs; ++i)
	{
		const auto pLib = pPsoDesc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
		const auto libDXIL = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pKeyShaderLibHeader->Lib));
		pLib->SetDXILLibrary(&libDXIL);

		if (pKeyShaderLibHeader->NumShaders)
		{
			// Calculate the size of shader-lib keys
			sizeKeyShaderLibs += sizeof(wchar_t*) * pKeyShaderLibHeader->NumShaders;

			// Export shaders
			const auto pShaderNames = reinterpret_cast<const wchar_t* const*>(&pKeyShaderLibHeader[1]);
			pLib->DefineExports(pShaderNames, pKeyShaderLibHeader->NumShaders);

			// Update the pointer
			pKeyShaderLibHeader = reinterpret_cast<const State_DX12::KeyShaderLibHeader*>
				(&pShaderNames[pKeyShaderLibHeader->NumShaders]);
		}
		// Else, use default shader exports for a DXIL library/collection subobject ~ surface all shaders.
	}

	// Hit groups
	const auto pKeyHitGroups = reinterpret_cast<const State_DX12::KeyHitGroup*>(&key[sizeKeyHeader]);
	for (auto i = 0u; i < keyHeader.NumHitGroups; ++i)
	{
		const auto& keyHitGroup = pKeyHitGroups[i];
		const auto pHitGroup = pPsoDesc.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
		if (keyHitGroup.ClosestHitShader)
			pHitGroup->SetClosestHitShaderImport(static_cast<const wchar_t*>(keyHitGroup.ClosestHitShader));
		if (keyHitGroup.AnyHitShader)
			pHitGroup->SetAnyHitShaderImport(static_cast<const wchar_t*>(keyHitGroup.AnyHitShader));
		if (keyHitGroup.IntersectionShader)
			pHitGroup->SetIntersectionShaderImport(static_cast<const wchar_t*>(keyHitGroup.IntersectionShader));

		pHitGroup->SetHitGroupExport(static_cast<const wchar_t*>(keyHitGroup.HitGroup));
		pHitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE(keyHitGroup.Type));
	}

	// Shader config
	// Defines the maximum sizes in bytes for the ray rayPayload and attribute structure.
	auto shaderConfig = pPsoDesc.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
	shaderConfig->Config(keyHeader.MaxPayloadSize, keyHeader.MaxAttributeSize);

	// Local pipeline layout and shader association
	// This is a pipeline layout that enables a shader to have unique arguments that come from shader tables.
	auto pKeyLocalPipelineLayoutHeader = reinterpret_cast<const State_DX12::KeyLocalPipelineLayoutHeader*>
		(&key[sizeKeyHeader + sizeKeyHitGroups + sizeKeyShaderLibs]);
	for (auto i = 0u; i < keyHeader.NumLocalPipelineLayouts; ++i)
	{
		const auto pRootSignature = static_cast<ID3D12RootSignature*>(pKeyLocalPipelineLayoutHeader->Layout);

		// Set pipeline layout
		const auto pLocalRootSignature = pPsoDesc.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
		pLocalRootSignature->SetRootSignature(pRootSignature);

		// Shader association
		const auto pAssociatedShaderNames = reinterpret_cast<const wchar_t* const*>(&pKeyLocalPipelineLayoutHeader[1]);
		const auto pRootSignatureAssociation = pPsoDesc.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
		pRootSignatureAssociation->SetSubobjectToAssociate(*pLocalRootSignature);
		pRootSignatureAssociation->AddExports(pAssociatedShaderNames, pKeyLocalPipelineLayoutHeader->NumShaders);

		// Update the pointer
		pKeyLocalPipelineLayoutHeader = reinterpret_cast<const State_DX12::KeyLocalPipelineLayoutHeader*>
			(&pAssociatedShaderNames[pKeyLocalPipelineLayoutHeader->NumShaders]);
	}

	// Global pipeline layout
	// This is a pipeline layout that is shared across all raytracing shaders invoked during a DispatchRays() call.
	const auto pGlobalRootSignature = pPsoDesc.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
	pGlobalRootSignature->SetRootSignature(static_cast<ID3D12RootSignature*>(keyHeader.GlobalLayout));

	// Pipeline config
	// Defines the maximum TraceRay() recursion depth.
	const auto pPipelineConfig = pPsoDesc.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
	// PERFOMANCE TIP: Set max recursion depth as low as needed
	// as drivers may apply optimization strategies for low recursion depths.
	pPipelineConfig->Config(keyHeader.MaxRecursionDepth);

	// Node mask
	if (keyHeader.NodeMask)
	{
		const auto pNodeMask = pPsoDesc.CreateSubobject<CD3DX12_NODE_MASK_SUBOBJECT>();
		pNodeMask->SetNodeMask(keyHeader.NodeMask);
	}

	//PrintStateObjectDesc(desc);

	// Create pipeline
	com_ptr<ID3D12RaytracingFallbackStateObject> stateObject = nullptr;
	H_RETURN(m_device->CreateStateObject(pPsoDesc, IID_PPV_ARGS(&stateObject)), cerr,
		L"Couldn't create DirectX Raytracing state object.\n", stateObject.get());

	if (name) stateObject->GetStateObject()->SetName(name);
	m_stateObjects[key] = stateObject;

	return stateObject;
}

com_ptr<ID3D12RaytracingFallbackStateObject> PipelineLib_DX12::getStateObject(const string& key, const wchar_t* name)
{
	const auto pStateObject = m_stateObjects.find(key);

	// Create one, if it does not exist
	if (pStateObject == m_stateObjects.end()) return createStateObject(key, name);

	return pStateObject->second;
}
