//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSG_DX12.h"
#include "Core/XUSGCommand_DX12.h"
#include "Core/XUSGResource_DX12.h"
#include "Core/XUSGPipelineLayout_DX12.h"
#include "XUSGComputeGraph_DX12.h"
#include "XUSGUltimate_DX12.h"

using namespace std;
using namespace XUSG;
using namespace XUSG::ComputeGraph;

State_DX12::State_DX12() :
	m_pKeyHeader(nullptr),
	m_isSerialized(false),
	m_pipeline(nullptr)
{
	// Default state
	m_key.resize(sizeof(KeyHeader));
	m_pKeyHeader = reinterpret_cast<KeyHeader*>(&m_key[0]);
	memset(m_pKeyHeader, 0, sizeof(KeyHeader));
}

State_DX12::~State_DX12()
{
}

void* State_DX12::GetBackend()
{
	return this;
}

void State_DX12::SetProgramName(const wchar_t* programName)
{
	m_isSerialized = false;
	m_pKeyHeader->ProgramName = programName;
}

void State_DX12::SetBytecode(const void* pBytecode, size_t bytecodeByteSize)
{
	m_isSerialized = false;
	m_pKeyHeader->pBytecode = pBytecode;
	m_pKeyHeader->BytecodeByteSize = bytecodeByteSize;
}

void State_DX12::SetNodeMask(uint32_t nodeMask)
{
	m_isSerialized = false;
	m_pKeyHeader->NodeMask = nodeMask;
}

Pipeline State_DX12::CreatePipeline(PipelineLib* pPipelineLib, const wchar_t* name)
{
	m_pipeline = pPipelineLib->CreatePipeline(this, name);

	return m_pipeline;
}

Pipeline State_DX12::GetPipeline(PipelineLib* pPipelineLib, const wchar_t* name)
{
	m_pipeline = pPipelineLib->GetPipeline(this, name);

	return m_pipeline;
}

const string& State_DX12::GetKey()
{
	if (!m_isSerialized) serialize();

	return m_key;
}

const wchar_t* State_DX12::GetProgramName() const
{
	return m_pKeyHeader->ProgramName;
}

ProgramIdentifier State_DX12::GetProgramIdentifier(const wchar_t* programName) const
{
	return Ultimate::GetDX12ProgramIdentifier(m_pipeline, programName ? programName : m_pKeyHeader->ProgramName);
}

void State_DX12::serialize()
{
	m_key.resize(sizeof(KeyHeader));
	m_pKeyHeader = reinterpret_cast<KeyHeader*>(&m_key[0]);

	m_isSerialized = true;
}

//--------------------------------------------------------------------------------------

PipelineLib_DX12::PipelineLib_DX12() :
	m_device(nullptr),
	m_stateObjects()
{
}

PipelineLib_DX12::PipelineLib_DX12(Device* pDevice) :
	PipelineLib()
{
	SetDevice(pDevice);
}

PipelineLib_DX12::~PipelineLib_DX12()
{
}

void PipelineLib_DX12::SetDevice(Device* pDevice)
{
	m_device = pDevice->GetNativeHandle();
	assert(m_device);
}

void PipelineLib_DX12::SetPipeline(State* pState, const Pipeline& pipeline)
{
	const auto p = AsDX12(pState);
	assert(p);

	lock_guard<mutex> lock(m_mtx);

	m_stateObjects[p->GetKey()] = static_cast<ID3D12StateObject*>(pipeline);
}

Pipeline PipelineLib_DX12::CreatePipeline(State* pState, const wchar_t* name)
{
	const auto p = AsDX12(pState);
	assert(p);

	lock_guard<mutex> lock(m_mtx);

	return createStateObject(p->GetKey(), name).get();
}

Pipeline PipelineLib_DX12::GetPipeline(State* pState, const wchar_t* name)
{
	const auto p = AsDX12(pState);
	assert(p);

	lock_guard<mutex> lock(m_mtx);

	return getStateObject(p->GetKey(), name).get();
}

ProgramIdentifier PipelineLib_DX12::GetProgramIdentifier(const Pipeline& stateObject, const wchar_t* programName) const
{
	return Ultimate::GetDX12ProgramIdentifier(stateObject, programName);
}

com_ptr<ID3D12StateObject> PipelineLib_DX12::createStateObject(const string& key, const wchar_t* name)
{
	// Get header
	const auto& keyHeader = reinterpret_cast<const State_DX12::KeyHeader&>(key[0]);

	CD3DX12_STATE_OBJECT_DESC pPsoDesc(D3D12_STATE_OBJECT_TYPE_EXECUTABLE);

	// MLIR program
	auto pMlirProgram = pPsoDesc.CreateSubobject<CD3DX12_MLIR_PROGRAM_SUBOBJECT>();
	pMlirProgram->SetProgramName(keyHeader.ProgramName);
	pMlirProgram->SetBytecode(keyHeader.pBytecode, keyHeader.BytecodeByteSize);

	// Node mask
	if (keyHeader.NodeMask)
	{
		const auto pNodeMask = pPsoDesc.CreateSubobject<CD3DX12_NODE_MASK_SUBOBJECT>();
		pNodeMask->SetNodeMask(keyHeader.NodeMask);
	}

	// Create pipeline
	com_ptr<ID3D12StateObject> stateObject = nullptr;
	com_ptr<ID3D12Device5> device = nullptr;
	V_RETURN(m_device->QueryInterface(IID_PPV_ARGS(&device)), cerr, nullptr);
	H_RETURN(device->CreateStateObject(pPsoDesc, IID_PPV_ARGS(&stateObject)), cerr,
		L"Couldn't create DirectX compute-graph state object.\n", stateObject.get());

	if (name) stateObject->SetName(name);
	m_stateObjects[key] = stateObject;

	return stateObject;
}

com_ptr<ID3D12StateObject> PipelineLib_DX12::getStateObject(const string& key, const wchar_t* name)
{
	const auto pStateObject = m_stateObjects.find(key);

	// Create one, if it does not exist
	if (pStateObject == m_stateObjects.cend()) return createStateObject(key, name);

	return pStateObject->second;
}

State_DX12* XUSG::ComputeGraph::AsDX12(State* p)
{
	assert(!p || p->GetBackend());
	return static_cast<State_DX12*>(p ? p->GetBackend() : nullptr);
}

//--------------------------------------------------------------------------------------

bool XUSG::ComputeGraph::GetDX12Version(Device* pDevice, uint64_t maxVersion, uint64_t* pHighestVersion)
{
	assert(pDevice);
	assert(pHighestVersion);

	com_ptr<ID3D12Device> device = pDevice->GetNativeHandle();
	assert(device);

	D3D12_FEATURE_DATA_MLIR_COMPUTE_GRAPH_VERSION featureData = {};
	featureData.HighestVersion.Version = maxVersion;
	if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_MLIR_COMPUTE_GRAPH_VERSION,
		&featureData, sizeof(featureData))))
	{
		*pHighestVersion = 0;

		return false;
	}

	*pHighestVersion = featureData.HighestVersion.Version;

	return true;
}

bool XUSG::ComputeGraph::GetDX12Support(Device* pDevice, Support* pSupport)
{
	assert(pDevice);
	assert(pSupport);

	com_ptr<ID3D12Device> device = pDevice->GetNativeHandle();
	assert(device);

	D3D12_FEATURE_DATA_MLIR_COMPUTE_GRAPH_SUPPORT featureData = {};
	if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_MLIR_COMPUTE_GRAPH_SUPPORT,
		&featureData, sizeof(featureData))))
	{
		pSupport->MultisubgraphPartitionsSupported = false;
		pSupport->CpuBindingSupported = false;

		return false;
	}

	pSupport->MultisubgraphPartitionsSupported = featureData.MultisubgraphPartitionsSupported;
	pSupport->CpuBindingSupported = featureData.CpuBindingSupported;

	return true;
}

bool XUSG::ComputeGraph::GetDX12Exchange(Device* pDevice, ExchangeType type, uint64_t irVersion,
	const void* pInputData, size_t inputDataByteSize, void* pOutputData, size_t* pOutputDataByteSize)
{
	assert(pDevice);
	assert(pOutputDataByteSize);

	com_ptr<ID3D12Device> device = pDevice->GetNativeHandle();
	assert(device);

	static const D3D12_MLIR_EXCHANGE_TYPE exchangeTypes[] =
	{
		D3D12_MLIR_EXCHANGE_TYPE_COMPUTE_GRAPH_SUBGRAPH_DECLARATION,
		D3D12_MLIR_EXCHANGE_TYPE_COMPUTE_GRAPH_SUBGRAPH_SPECIALIZATION,
		D3D12_MLIR_EXCHANGE_TYPE_COMPUTE_GRAPH_TARGET_CONFIG
	};

	assert(static_cast<uint8_t>(type) < size(exchangeTypes));

	D3D12_FEATURE_DATA_MLIR_EXCHANGE featureData = {};
	featureData.Type = exchangeTypes[static_cast<uint8_t>(type)];
	featureData.IRVersion.Version = irVersion;
	featureData.pInputData = pInputData;
	featureData.InputDataSizeInBytes = inputDataByteSize;
	featureData.pOutputData = pOutputData;
	featureData.OutputDataSizeInBytes = pOutputDataByteSize;

	return SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_MLIR_EXCHANGE,
		&featureData, sizeof(featureData)));
}
