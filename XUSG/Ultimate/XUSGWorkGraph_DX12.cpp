//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSG_DX12.h"
#include "XUSGWorkGraph_DX12.h"

using namespace std;
using namespace XUSG;
using namespace XUSG::WorkGraph;

State_DX12::State_DX12() :
	m_keyShaderLibs(0),
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

void State_DX12::SetShaderLibrary(uint32_t index, const Blob& shaderLib, uint32_t numShaders, const void** pShaders)
{
	m_isSerialized = false;

	if (index >= m_keyShaderLibs.size())
		m_keyShaderLibs.resize(index + 1);

	auto& keyShaderLib = m_keyShaderLibs[index];
	keyShaderLib.Lib = shaderLib;
	keyShaderLib.Shaders.resize(numShaders);
	memcpy(keyShaderLib.Shaders.data(), pShaders, sizeof(void*) * numShaders);
}

void State_DX12::SetProgram(const void* program)
{
	m_isSerialized = false;
	m_pKeyHeader->Program = program;
}

void State_DX12::SetLocalPipelineLayout(uint32_t index, const XUSG::PipelineLayout& layout, uint32_t numShaders, const void** pShaders)
{
	m_isSerialized = false;

	if (index >= m_keyLocalPipelineLayouts.size())
		m_keyLocalPipelineLayouts.resize(index + 1);

	auto& keyLocalPipelineLayout = m_keyLocalPipelineLayouts[index];
	keyLocalPipelineLayout.Layout = layout;
	keyLocalPipelineLayout.Shaders.resize(numShaders);
	memcpy(keyLocalPipelineLayout.Shaders.data(), pShaders, sizeof(void*) * numShaders);
}

void State_DX12::SetGlobalPipelineLayout(const XUSG::PipelineLayout& layout)
{
	m_isSerialized = false;
	m_pKeyHeader->GlobalLayout = layout;
}

Pipeline State_DX12::CreatePipeline(PipelineLib* pPipelineCache, const wchar_t* name)
{
	const auto pDX12PipelineCache = static_cast<PipelineLib_DX12*>(pPipelineCache);
	const auto pipeline = pDX12PipelineCache->CreatePipeline(GetKey(), name);
	setStateObject(pipeline);

	return pipeline.get();
}

Pipeline State_DX12::GetPipeline(PipelineLib* pPipelineCache, const wchar_t* name)
{
	const auto pDX12PipelineCache = static_cast<PipelineLib_DX12*>(pPipelineCache);
	const auto pipeline = pDX12PipelineCache->GetPipeline(GetKey(), name);
	setStateObject(pipeline);

	return pipeline.get();
}

const string& State_DX12::GetKey()
{
	if (!m_isSerialized) serialize();

	return m_key;
}

const wchar_t* State_DX12::GetProgramName(uint32_t workGraphIndex) const
{
	return m_properties->GetProgramName(workGraphIndex);
}

uint32_t State_DX12::GetNumWorkGraphs() const
{
	return m_properties->GetNumWorkGraphs();
}

uint32_t State_DX12::GetWorkGraphIndex(const wchar_t* pProgramName) const
{
	return m_properties->GetWorkGraphIndex(pProgramName);
}

uint32_t State_DX12::GetNumNodes(uint32_t workGraphIndex) const
{
	return m_properties->GetNumNodes(workGraphIndex);
}

uint32_t State_DX12::GetNodeIndex(uint32_t workGraphIndex, const NodeID& nodeID) const
{
	D3D12_NODE_ID dx12NodeID;
	dx12NodeID.Name = nodeID.Name;
	dx12NodeID.ArrayIndex = nodeID.ArrayIndex;

	return m_properties->GetNodeIndex(workGraphIndex, dx12NodeID);
}

uint32_t State_DX12::GetNodeLocalRootArgumentsTableIndex(uint32_t workGraphIndex, uint32_t nodeIndex) const
{
	return m_properties->GetNodeLocalRootArgumentsTableIndex(workGraphIndex, nodeIndex);
}

uint32_t State_DX12::GetNumEntrypoints(uint32_t workGraphIndex) const
{
	return m_properties->GetNumEntrypoints(workGraphIndex);
}

uint32_t State_DX12::GetEntrypointIndex(uint32_t workGraphIndex, const NodeID& nodeID) const
{
	D3D12_NODE_ID dx12NodeID;
	dx12NodeID.Name = nodeID.Name;
	dx12NodeID.ArrayIndex = nodeID.ArrayIndex;

	return m_properties->GetEntrypointIndex(workGraphIndex, dx12NodeID);
}

uint32_t State_DX12::GetEntrypointRecordSizeInBytes(uint32_t workGraphIndex, uint32_t entrypointIndex) const
{
	return m_properties->GetEntrypointRecordSizeInBytes(workGraphIndex, entrypointIndex);
}

NodeID State_DX12::GetNodeID(uint32_t workGraphIndex, uint32_t nodeIndex) const
{
	NodeID nodeID;
	GetNodeID(&nodeID, workGraphIndex, nodeIndex);

	return nodeID;
}

NodeID* State_DX12::GetNodeID(NodeID* pRetVal, uint32_t workGraphIndex, uint32_t nodeIndex) const
{
	assert(pRetVal);
#if defined(_MSC_VER) || !defined(_WIN32)
	const auto nodeID = m_properties->GetNodeID(workGraphIndex, nodeIndex);
#else
	D3D12_NODE_ID nodeID;
	m_properties->GetNodeID(&nodeID, workGraphIndex, nodeIndex);
#endif

	pRetVal->Name = nodeID.Name;
	pRetVal->ArrayIndex = nodeID.ArrayIndex;

	return pRetVal;
}

NodeID State_DX12::GetEntrypointID(uint32_t workGraphIndex, uint32_t entrypointIndex) const
{
	NodeID nodeID;
	GetEntrypointID(&nodeID, workGraphIndex, entrypointIndex);

	return nodeID;
}

NodeID* State_DX12::GetEntrypointID(NodeID* pRetVal, uint32_t workGraphIndex, uint32_t entrypointIndex) const
{
	assert(pRetVal);
#if defined(_MSC_VER) || !defined(_WIN32)
	const auto entrypointID = m_properties->GetEntrypointID(workGraphIndex, entrypointIndex);
#else
	D3D12_NODE_ID entrypointID;
	m_properties->GetEntrypointID(&entrypointID, workGraphIndex, entrypointIndex);
#endif

	pRetVal->Name = entrypointID.Name;
	pRetVal->ArrayIndex = entrypointID.ArrayIndex;

	return pRetVal;
}

void State_DX12::GetMemoryRequirements(uint32_t workGraphIndex, MemoryRequirements* pMemoryReq) const
{
	assert(pMemoryReq);

	D3D12_WORK_GRAPH_MEMORY_REQUIREMENTS memoryReq;
	m_properties->GetWorkGraphMemoryRequirements(workGraphIndex, &memoryReq);

	pMemoryReq->MinByteSize = memoryReq.MinSizeInBytes;
	pMemoryReq->MaxByteSize = memoryReq.MaxSizeInBytes;
	pMemoryReq->SizeGranularityInBytes = memoryReq.SizeGranularityInBytes;
}

void State_DX12::serialize()
{
	m_pKeyHeader->NumShaderLibs = static_cast<uint32_t>(m_keyShaderLibs.size());
	m_pKeyHeader->NumLocalPipelineLayouts = static_cast<uint32_t>(m_keyLocalPipelineLayouts.size());

	// Calculate total key size
	const auto sizeKeyHeader = sizeof(KeyHeader);

	auto sizeKeyShaderLibs = sizeof(KeyShaderLibHeader) * m_keyShaderLibs.size();
	for (const auto& keyShaderLib : m_keyShaderLibs)
		sizeKeyShaderLibs += sizeof(void*) * keyShaderLib.Shaders.size();

	auto sizeKeyLocalPipelineLayouts = sizeof(KeyLocalPipelineLayoutHeader) * m_keyLocalPipelineLayouts.size();
	for (const auto& keyLocalPipelineLayout : m_keyLocalPipelineLayouts)
		sizeKeyLocalPipelineLayouts += sizeof(void*) * keyLocalPipelineLayout.Shaders.size();

	m_key.resize(sizeKeyHeader + sizeKeyShaderLibs + sizeKeyLocalPipelineLayouts);
	m_pKeyHeader = reinterpret_cast<KeyHeader*>(&m_key[0]);

	auto pKeyShaderLibHeader = reinterpret_cast<KeyShaderLibHeader*>(&m_key[sizeKeyHeader]);
	for (const auto& keyShaderLib : m_keyShaderLibs)
	{
		// Set the layout and number of associated shaders
		pKeyShaderLibHeader->Lib = keyShaderLib.Lib;
		pKeyShaderLibHeader->NumShaders = static_cast<uint32_t>(keyShaderLib.Shaders.size());

		// Update the pointer of the shaders, and copy the shaders
		const auto pKeyShaders = reinterpret_cast<void**>(&pKeyShaderLibHeader[1]);
		memcpy(pKeyShaders, keyShaderLib.Shaders.data(), sizeof(void*) * pKeyShaderLibHeader->NumShaders);

		// Update the pointer
		pKeyShaderLibHeader = reinterpret_cast<KeyShaderLibHeader*>(&pKeyShaders[pKeyShaderLibHeader->NumShaders]);
	}

	auto pKeyLocalPipelineLayoutHeader = reinterpret_cast<KeyLocalPipelineLayoutHeader*>
		(&m_key[sizeKeyHeader + sizeKeyShaderLibs]);
	for (const auto& keyLocalPipelineLayout : m_keyLocalPipelineLayouts)
	{
		// Set the layout and number of associated shaders
		pKeyLocalPipelineLayoutHeader->Layout = keyLocalPipelineLayout.Layout;
		pKeyLocalPipelineLayoutHeader->NumShaders = static_cast<uint32_t>(keyLocalPipelineLayout.Shaders.size());

		// Update the pointer of association, and copy the associated shaders
		const auto pKeyLocalPipelineAssociation = reinterpret_cast<void**>(&pKeyLocalPipelineLayoutHeader[1]);
		memcpy(pKeyLocalPipelineAssociation, keyLocalPipelineLayout.Shaders.data(),
			sizeof(void*) * pKeyLocalPipelineLayoutHeader->NumShaders);

		// Update the pointer
		pKeyLocalPipelineLayoutHeader = reinterpret_cast<KeyLocalPipelineLayoutHeader*>
			(&pKeyLocalPipelineAssociation[pKeyLocalPipelineLayoutHeader->NumShaders]);
	}

	m_isSerialized = true;
}

bool State_DX12::setStateObject(const com_ptr<ID3D12StateObject>& stateObject)
{
	V_RETURN(stateObject->QueryInterface(IID_PPV_ARGS(&m_properties)), cerr, false);
	assert(m_properties);

	return true;
}

//--------------------------------------------------------------------------------------

PipelineLib_DX12::PipelineLib_DX12() :
	m_device(nullptr),
	m_stateObjects()
{
}

PipelineLib_DX12::PipelineLib_DX12(const Device* pDevice) :
	PipelineLib()
{
	SetDevice(pDevice);
}

PipelineLib_DX12::~PipelineLib_DX12()
{
}

void PipelineLib_DX12::SetDevice(const Device* pDevice)
{
	const auto hr = static_cast<ID3D12Device*>(pDevice->GetHandle())->QueryInterface(IID_PPV_ARGS(&m_device));
	if (FAILED(hr))
	{
		cerr << HrToString(hr).c_str() << std::endl;
		assert(!HrToString(hr).c_str());
	}
	assert(m_device);
}

void PipelineLib_DX12::SetPipeline(const string& key, const Pipeline& pipeline)
{
	m_stateObjects[key] = static_cast<ID3D12StateObject*>(pipeline);
}

Pipeline PipelineLib_DX12::CreatePipeline(State* pState, const wchar_t* name)
{
	return CreatePipeline(pState->GetKey(), name).get();
}

Pipeline PipelineLib_DX12::GetPipeline(State* pState, const wchar_t* name)
{
	return GetPipeline(pState->GetKey(), name).get();
}

com_ptr<ID3D12StateObject> PipelineLib_DX12::CreatePipeline(const string& key, const wchar_t* name)
{
	// Get header
	const auto& keyHeader = reinterpret_cast<const State_DX12::KeyHeader&>(key[0]);
	const auto sizeKeyHeader = sizeof(State_DX12::KeyHeader);
	auto sizeKeyShaderLibs = sizeof(State_DX12::KeyShaderLibHeader) * keyHeader.NumShaderLibs;

	CD3DX12_STATE_OBJECT_DESC pPsoDesc(D3D12_STATE_OBJECT_TYPE_EXECUTABLE);

	// DXIL library
	auto pKeyShaderLibHeader = reinterpret_cast<const State_DX12::KeyShaderLibHeader*>(&key[sizeKeyHeader]);
	for (auto i = 0u; i < keyHeader.NumShaderLibs; ++i)
	{
		const auto pLib = pPsoDesc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
		const auto libDXIL = CD3DX12_SHADER_BYTECODE(static_cast<ID3DBlob*>(pKeyShaderLibHeader->Lib));
		pLib->SetDXILLibrary(&libDXIL);

		if (pKeyShaderLibHeader->NumShaders > 0)
		{
			// Calculate the size of shader-lib keys
			sizeKeyShaderLibs += sizeof(void*) * pKeyShaderLibHeader->NumShaders;

			// Export shaders
			const auto pShaderNames = reinterpret_cast<const wchar_t* const*>(&pKeyShaderLibHeader[1]);
			pLib->DefineExports(pShaderNames, pKeyShaderLibHeader->NumShaders);

			// Update the pointer
			pKeyShaderLibHeader = reinterpret_cast<const State_DX12::KeyShaderLibHeader*>
				(&pShaderNames[pKeyShaderLibHeader->NumShaders]);
		}
		// Else, use default shader exports for a DXIL library/collection subobject ~ surface all shaders.
	}

	// Work graph
	// Defines the shader program name.
	auto pWorkGraph = pPsoDesc.CreateSubobject<CD3DX12_WORK_GRAPH_SUBOBJECT>();
	pWorkGraph->IncludeAllAvailableNodes();
	pWorkGraph->SetProgramName(static_cast<const wchar_t*>(keyHeader.Program));

	// Local pipeline layout and shader association
	// This is a pipeline layout that enables a shader to have unique arguments that come from shader tables.
	auto pKeyLocalPipelineLayoutHeader = reinterpret_cast<const State_DX12::KeyLocalPipelineLayoutHeader*>
		(&key[sizeKeyHeader + sizeKeyShaderLibs]);
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

	//PrintStateObjectDesc(desc);

	// Create pipeline
	com_ptr<ID3D12StateObject> stateObject = nullptr;
	H_RETURN(m_device->CreateStateObject(pPsoDesc, IID_PPV_ARGS(&stateObject)), cerr,
		L"Couldn't create DirectX work-graph state object.\n", stateObject.get());

	if (name) stateObject->SetName(name);
	m_stateObjects[key] = stateObject;

	return stateObject;
}

com_ptr<ID3D12StateObject> PipelineLib_DX12::GetPipeline(const string& key, const wchar_t* name)
{
	const auto pStateObject = m_stateObjects.find(key);

	// Create one, if it does not exist
	if (pStateObject == m_stateObjects.end()) return CreatePipeline(key, name);

	return pStateObject->second;
}
