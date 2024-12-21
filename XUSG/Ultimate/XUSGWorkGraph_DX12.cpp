//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSG_DX12.h"
#include "Core/XUSGCommand_DX12.h"
#include "Core/XUSGResource_DX12.h"
#include "Core/XUSGPipelineLayout_DX12.h"
#include "XUSGWorkGraph_DX12.h"
#include "XUSGUltimate_DX12.h"

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

void State_DX12::SetShaderLibrary(uint32_t index, const Blob& shaderLib,
	uint32_t numShaders, const wchar_t** pShaderNames)
{
	m_isSerialized = false;

	if (index >= m_keyShaderLibs.size())
		m_keyShaderLibs.resize(index + 1);

	auto& keyShaderLib = m_keyShaderLibs[index];
	keyShaderLib.Lib = shaderLib;
	keyShaderLib.Shaders.resize(numShaders);
	memcpy(keyShaderLib.Shaders.data(), pShaderNames, sizeof(wchar_t*) * numShaders);
}

void State_DX12::SetProgramName(const wchar_t* programName)
{
	m_isSerialized = false;
	m_pKeyHeader->ProgramName = programName;
}

void State_DX12::SetLocalPipelineLayout(uint32_t index, const XUSG::PipelineLayout& layout,
	uint32_t numShaders, const wchar_t** pShaderNames)
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

void State_DX12::SetNodeMask(uint32_t nodeMask)
{
	m_isSerialized = false;
	m_pKeyHeader->NodeMask = nodeMask;
}

void State_DX12::OverrideDispatchGrid(const wchar_t* shaderName, uint32_t x, uint32_t y, uint32_t z, BoolOverride isEntry)
{
	m_isSerialized = false;

	const auto needInit = m_keyBroadCastingOverrides.find(shaderName) == m_keyBroadCastingOverrides.cend();
	auto& keyBroadCastingOverrides = m_keyBroadCastingOverrides[shaderName];
	if (needInit)
	{
		keyBroadCastingOverrides.Shader = shaderName;
		memset(keyBroadCastingOverrides.MaxDispatchGrid, 0, sizeof(uint32_t[3]));
	}

	keyBroadCastingOverrides.DispatchGrid[0] = x;
	keyBroadCastingOverrides.DispatchGrid[1] = y;
	keyBroadCastingOverrides.DispatchGrid[2] = z;
	keyBroadCastingOverrides.IsEntry = isEntry;
}

void State_DX12::OverrideMaxDispatchGrid(const wchar_t* shaderName, uint32_t x, uint32_t y, uint32_t z, BoolOverride isEntry)
{
	m_isSerialized = false;

	const auto needInit = m_keyBroadCastingOverrides.find(shaderName) == m_keyBroadCastingOverrides.cend();
	auto& keyBroadCastingOverrides = m_keyBroadCastingOverrides[shaderName];
	if (needInit)
	{
		keyBroadCastingOverrides.Shader = shaderName;
		memset(keyBroadCastingOverrides.DispatchGrid, 0, sizeof(uint32_t[3]));
	}

	keyBroadCastingOverrides.MaxDispatchGrid[0] = x;
	keyBroadCastingOverrides.MaxDispatchGrid[1] = y;
	keyBroadCastingOverrides.MaxDispatchGrid[2] = z;
	keyBroadCastingOverrides.IsEntry = isEntry;
}

Pipeline State_DX12::CreatePipeline(PipelineLib* pPipelineLib, const wchar_t* name)
{
	const auto pDX12PipelineLib = static_cast<PipelineLib_DX12*>(pPipelineLib);
	const auto pipeline = pDX12PipelineLib->CreateStateObject(GetKey(), name);

	return setStateObject(pipeline);
}

Pipeline State_DX12::GetPipeline(PipelineLib* pPipelineLib, const wchar_t* name)
{
	const auto pDX12PipelineLib = static_cast<PipelineLib_DX12*>(pPipelineLib);
	const auto pipeline = pDX12PipelineLib->GetStateObject(GetKey(), name);

	return setStateObject(pipeline);
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

ProgramIdentifier State_DX12::GetProgramIdentifier(const wchar_t* programName) const
{
	return Ultimate::GetDX12ProgramIdentifier(m_properties.get(), programName);
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
	return m_properties->GetNodeIndex(workGraphIndex, PipelineLib_DX12::GetDX12NodeID(nodeID));
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
	return m_properties->GetEntrypointIndex(workGraphIndex, PipelineLib_DX12::GetDX12NodeID(nodeID));
}

uint32_t State_DX12::GetEntrypointRecordSizeInBytes(uint32_t workGraphIndex, uint32_t entrypointIndex) const
{
	return m_properties->GetEntrypointRecordSizeInBytes(workGraphIndex, entrypointIndex);
}

NodeID State_DX12::GetNodeID(uint32_t workGraphIndex, uint32_t nodeIndex) const
{
#if defined(_MSC_VER) || !defined(_WIN32)
	const auto nodeID = m_properties->GetNodeID(workGraphIndex, nodeIndex);
#else
	D3D12_NODE_ID nodeID;
	const auto pNodeID = m_properties->GetNodeID(&nodeID, workGraphIndex, nodeIndex);
	assert(pNodeID);
#endif

	return { nodeID.Name, nodeID.ArrayIndex };
}

NodeID State_DX12::GetEntrypointID(uint32_t workGraphIndex, uint32_t entrypointIndex) const
{
#if defined(_MSC_VER) || !defined(_WIN32)
	const auto entrypointID = m_properties->GetEntrypointID(workGraphIndex, entrypointIndex);
#else
	D3D12_NODE_ID entrypointID;
	const auto pEntrypointID = m_properties->GetEntrypointID(&entrypointID, workGraphIndex, entrypointIndex);
	assert(pEntrypointID);
#endif

	return { entrypointID.Name, entrypointID.ArrayIndex };
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
	m_pKeyHeader->NumBroadCastingOverrides = static_cast<uint32_t>(m_keyBroadCastingOverrides.size());

	// Calculate total key size
	const auto sizeKeyHeader = sizeof(KeyHeader);

	auto sizeKeyShaderLibs = sizeof(KeyShaderLibHeader) * m_keyShaderLibs.size();
	for (const auto& keyShaderLib : m_keyShaderLibs)
		sizeKeyShaderLibs += sizeof(wchar_t*) * keyShaderLib.Shaders.size();

	auto sizeKeyLocalPipelineLayouts = sizeof(KeyLocalPipelineLayoutHeader) * m_keyLocalPipelineLayouts.size();
	for (const auto& keyLocalPipelineLayout : m_keyLocalPipelineLayouts)
		sizeKeyLocalPipelineLayouts += sizeof(wchar_t*) * keyLocalPipelineLayout.Shaders.size();

	const auto sizeKeyBroadCastingOverrides = sizeof(KeyBroadCastingOverrides) * m_keyBroadCastingOverrides.size();

	m_key.resize(sizeKeyHeader + sizeKeyShaderLibs + sizeKeyLocalPipelineLayouts + sizeKeyBroadCastingOverrides);
	m_pKeyHeader = reinterpret_cast<KeyHeader*>(&m_key[0]);

	auto pKeyShaderLibHeader = reinterpret_cast<KeyShaderLibHeader*>(&m_key[sizeKeyHeader]);
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
		(&m_key[sizeKeyHeader + sizeKeyShaderLibs]);
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

	auto pKeyBroadCastingOverrides = reinterpret_cast<KeyBroadCastingOverrides*>
		(&m_key[sizeKeyHeader + sizeKeyShaderLibs + sizeKeyLocalPipelineLayouts]);
	for (const auto& keyBroadCastingOverrides : m_keyBroadCastingOverrides)
	{
		*pKeyBroadCastingOverrides = keyBroadCastingOverrides.second;

		// Update the pointer
		++pKeyBroadCastingOverrides;
	}

	m_isSerialized = true;
}

Pipeline State_DX12::setStateObject(const com_ptr<ID3D12StateObject>& stateObject)
{
	m_pipeline = stateObject.get();
	V_RETURN(stateObject->QueryInterface(IID_PPV_ARGS(&m_properties)), cerr, m_pipeline);
	assert(m_properties);

	return m_pipeline;
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
	m_device = pDevice->GetHandle();
	assert(m_device);
}

void PipelineLib_DX12::SetPipeline(State* pState, const Pipeline& pipeline)
{
	const auto p = dynamic_cast<State_DX12*>(pState);
	assert(p);

	m_stateObjects[p->GetKey()] = static_cast<ID3D12StateObject*>(pipeline);
}

Pipeline PipelineLib_DX12::CreatePipeline(State* pState, const wchar_t* name)
{
	const auto p = dynamic_cast<State_DX12*>(pState);
	assert(p);

	return CreateStateObject(p->GetKey(), name).get();
}

Pipeline PipelineLib_DX12::GetPipeline(State* pState, const wchar_t* name)
{
	const auto p = dynamic_cast<State_DX12*>(pState);
	assert(p);

	return GetStateObject(p->GetKey(), name).get();
}

const wchar_t* PipelineLib_DX12::GetProgramName(const Pipeline& stateObject, uint32_t workGraphIndex) const
{
	return getWorkGraphProperties(stateObject)->GetProgramName(workGraphIndex);
}

ProgramIdentifier PipelineLib_DX12::GetProgramIdentifier(const Pipeline& stateObject, const wchar_t* programName) const
{
	return Ultimate::GetDX12ProgramIdentifier(stateObject, programName);
}

uint32_t PipelineLib_DX12::GetNumWorkGraphs(const Pipeline& stateObject) const
{
	return getWorkGraphProperties(stateObject)->GetNumWorkGraphs();
}

uint32_t PipelineLib_DX12::GetWorkGraphIndex(const Pipeline& stateObject, const wchar_t* pProgramName) const
{
	return getWorkGraphProperties(stateObject)->GetWorkGraphIndex(pProgramName);
}

uint32_t PipelineLib_DX12::GetNumNodes(const Pipeline& stateObject, uint32_t workGraphIndex) const
{
	return getWorkGraphProperties(stateObject)->GetNumNodes(workGraphIndex);
}

uint32_t PipelineLib_DX12::GetNodeIndex(const Pipeline& stateObject, uint32_t workGraphIndex, const NodeID& nodeID) const
{
	return getWorkGraphProperties(stateObject)->GetNodeIndex(workGraphIndex, GetDX12NodeID(nodeID));
}

uint32_t PipelineLib_DX12::GetNodeLocalRootArgumentsTableIndex(const Pipeline& stateObject, uint32_t workGraphIndex, uint32_t nodeIndex) const
{
	return getWorkGraphProperties(stateObject)->GetNodeLocalRootArgumentsTableIndex(workGraphIndex, nodeIndex);
}

uint32_t PipelineLib_DX12::GetNumEntrypoints(const Pipeline& stateObject, uint32_t workGraphIndex) const
{
	return getWorkGraphProperties(stateObject)->GetNumEntrypoints(workGraphIndex);
}

uint32_t PipelineLib_DX12::GetEntrypointIndex(const Pipeline& stateObject, uint32_t workGraphIndex, const NodeID& nodeID) const
{
	return getWorkGraphProperties(stateObject)->GetEntrypointIndex(workGraphIndex, GetDX12NodeID(nodeID));
}

uint32_t PipelineLib_DX12::GetEntrypointRecordSizeInBytes(const Pipeline& stateObject, uint32_t workGraphIndex, uint32_t entrypointIndex) const
{
	return getWorkGraphProperties(stateObject)->GetEntrypointRecordSizeInBytes(workGraphIndex, entrypointIndex);
}

NodeID PipelineLib_DX12::GetNodeID(const Pipeline& stateObject, uint32_t workGraphIndex, uint32_t nodeIndex) const
{
#if defined(_MSC_VER) || !defined(_WIN32)
	const auto nodeID = getWorkGraphProperties(stateObject)->GetNodeID(workGraphIndex, nodeIndex);
#else
	D3D12_NODE_ID nodeID;
	const auto pNodeID = getWorkGraphProperties(stateObject)->GetNodeID(&nodeID, workGraphIndex, nodeIndex);
	assert(pNodeID);
#endif

	return { nodeID.Name, nodeID.ArrayIndex };
}

NodeID PipelineLib_DX12::GetEntrypointID(const Pipeline& stateObject, uint32_t workGraphIndex, uint32_t entrypointIndex) const
{
#if defined(_MSC_VER) || !defined(_WIN32)
	const auto entrypointID = getWorkGraphProperties(stateObject)->GetEntrypointID(workGraphIndex, entrypointIndex);
#else
	D3D12_NODE_ID entrypointID;
	const auto pEntrypointID = getWorkGraphProperties(stateObject)->GetEntrypointID(&entrypointID, workGraphIndex, entrypointIndex);
	assert(pEntrypointID);
#endif

	return { entrypointID.Name, entrypointID.ArrayIndex };
}

void PipelineLib_DX12::GetMemoryRequirements(const Pipeline& stateObject, uint32_t workGraphIndex, MemoryRequirements* pMemoryReq) const
{
	assert(pMemoryReq);

	D3D12_WORK_GRAPH_MEMORY_REQUIREMENTS memoryReq;
	getWorkGraphProperties(stateObject)->GetWorkGraphMemoryRequirements(workGraphIndex, &memoryReq);

	pMemoryReq->MinByteSize = memoryReq.MinSizeInBytes;
	pMemoryReq->MaxByteSize = memoryReq.MaxSizeInBytes;
	pMemoryReq->SizeGranularityInBytes = memoryReq.SizeGranularityInBytes;
}

com_ptr<ID3D12StateObject> PipelineLib_DX12::CreateStateObject(const string& key, const wchar_t* name)
{
	// Get header
	const auto& keyHeader = reinterpret_cast<const State_DX12::KeyHeader&>(key[0]);
	const auto sizeKeyHeader = sizeof(State_DX12::KeyHeader);
	auto sizeKeyShaderLibs = sizeof(State_DX12::KeyShaderLibHeader) * keyHeader.NumShaderLibs;
	auto sizeKeyLocalPipelineLayouts = sizeof(State_DX12::KeyLocalPipelineLayoutHeader) * keyHeader.NumLocalPipelineLayouts;

	CD3DX12_STATE_OBJECT_DESC pPsoDesc(D3D12_STATE_OBJECT_TYPE_EXECUTABLE);

	// Work graph
	// Defines the shader program name.
	auto pWorkGraph = pPsoDesc.CreateSubobject<CD3DX12_WORK_GRAPH_SUBOBJECT>();
	pWorkGraph->IncludeAllAvailableNodes();
	pWorkGraph->SetProgramName(keyHeader.ProgramName);

	// DXIL library
	auto pKeyShaderLibHeader = reinterpret_cast<const State_DX12::KeyShaderLibHeader*>(&key[sizeKeyHeader]);
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

	// Local pipeline layout and shader association
	// This is a pipeline layout that enables a shader to have unique arguments that come from shader tables.
	auto pKeyLocalPipelineLayoutHeader = reinterpret_cast<const State_DX12::KeyLocalPipelineLayoutHeader*>
		(&key[sizeKeyHeader + sizeKeyShaderLibs]);
	for (auto i = 0u; i < keyHeader.NumLocalPipelineLayouts; ++i)
	{
		// Calculate the size of local pipeline-layout keys
		sizeKeyLocalPipelineLayouts += sizeof(wchar_t*) * pKeyLocalPipelineLayoutHeader->NumShaders;

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
	if (keyHeader.GlobalLayout)
	{
		// This is a pipeline layout that is shared across all node shaders invoked during a DispatchGraph() call.
		const auto pGlobalRootSignature = pPsoDesc.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
		pGlobalRootSignature->SetRootSignature(static_cast<ID3D12RootSignature*>(keyHeader.GlobalLayout));
	}

	// Broad-casting overrides
	const auto pKeyBroadCastingOverrides = reinterpret_cast<const State_DX12::KeyBroadCastingOverrides*>
		(&key[sizeKeyHeader + sizeKeyShaderLibs + sizeKeyLocalPipelineLayouts]);
	for (auto i = 0u; i < keyHeader.NumBroadCastingOverrides; ++i)
	{
		const auto& keyBroadCastingOverrides = pKeyBroadCastingOverrides[i];
		const auto pOverrides = pWorkGraph->CreateBroadcastingLaunchNodeOverrides(keyBroadCastingOverrides.Shader);

		// Override DispatchGrid
		const auto& dispatchGrid = keyBroadCastingOverrides.DispatchGrid;
		if (dispatchGrid[0] && dispatchGrid[1] && dispatchGrid[1])
			pOverrides->DispatchGrid(dispatchGrid[0], dispatchGrid[1], dispatchGrid[2]);

		// Override MaxDispatchGrid
		const auto& maxDispatchGrid = keyBroadCastingOverrides.MaxDispatchGrid;
		if (maxDispatchGrid[0] && maxDispatchGrid[1] && maxDispatchGrid[1])
			pOverrides->MaxDispatchGrid(maxDispatchGrid[0], maxDispatchGrid[1], maxDispatchGrid[2]);

		if (keyBroadCastingOverrides.IsEntry != BoolOverride::IS_NULL)
			pOverrides->ProgramEntry(keyBroadCastingOverrides.IsEntry == BoolOverride::IS_TRUE ? TRUE : FALSE);
	}

	// Node mask
	if (keyHeader.NodeMask)
	{
		const auto pNodeMask = pPsoDesc.CreateSubobject<CD3DX12_NODE_MASK_SUBOBJECT>();
		pNodeMask->SetNodeMask(keyHeader.NodeMask);
	}

	//PrintStateObjectDesc(desc);

	// Create pipeline
	com_ptr<ID3D12StateObject> stateObject = nullptr;
	com_ptr<ID3D12Device9> device = nullptr;
	V_RETURN(m_device->QueryInterface(IID_PPV_ARGS(&device)), cerr, nullptr);
	H_RETURN(device->CreateStateObject(pPsoDesc, IID_PPV_ARGS(&stateObject)), cerr,
		L"Couldn't create DirectX work-graph state object.\n", stateObject.get());

	if (name) stateObject->SetName(name);
	m_stateObjects[key] = stateObject;

	return stateObject;
}

com_ptr<ID3D12StateObject> PipelineLib_DX12::GetStateObject(const string& key, const wchar_t* name)
{
	const auto pStateObject = m_stateObjects.find(key);

	// Create one, if it does not exist
	if (pStateObject == m_stateObjects.end()) return CreateStateObject(key, name);

	return pStateObject->second;
}

D3D12_NODE_ID PipelineLib_DX12::GetDX12NodeID(const NodeID& nodeID)
{
	return { nodeID.Name, nodeID.ArrayIndex };
}

com_ptr<ID3D12WorkGraphProperties> PipelineLib_DX12::getWorkGraphProperties(const Pipeline& stateObject)
{
	com_ptr<ID3D12WorkGraphProperties> properties;
	V_RETURN(static_cast<ID3D12StateObject*>(stateObject)->QueryInterface(IID_PPV_ARGS(&properties)), cerr, nullptr);
	assert(properties);

	return properties;
}
