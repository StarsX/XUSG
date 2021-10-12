//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSG_DX12.h"
#include "XUSGOperator_DML.h"
#include "XUSGMachineLearning_DML.h"

using namespace std;
using namespace XUSG::ML;

//--------------------------------------------------------------------------------------
// Operator
//--------------------------------------------------------------------------------------

Operator_DML::Operator_DML() :
	m_dispatchable(nullptr)
{
}

Operator_DML::~Operator_DML()
{
}

bool Operator_DML::Create(const Device* pDevice, OperatorType type, const void* pTypedOp, ExecutionFlag flags)
{
	vector<uint8_t> dmlTypedOpDesc;
	GetDMLTypedOperator(dmlTypedOpDesc, type, pTypedOp);
	DML_OPERATOR_DESC dmlOpDesc = { GetDMLOpteratorType(type), dmlTypedOpDesc.data() };

	com_ptr<IDMLOperator> dmlOperator = nullptr;
	const auto pDMLDevice = static_cast<IDMLDevice*>(pDevice->GetHandle());
	V_RETURN(pDMLDevice->CreateOperator(&dmlOpDesc, IID_PPV_ARGS(&dmlOperator)), cerr, false);

	// Compile the operator into an object that can be dispatched to the GPU. In this step, DirectML performs operator
	// fusion and just-in-time (JIT) compilation of shader bytecode, then compiles it into a Direct3D 12 pipeline state object (PSO).
	// The resulting compiled operator is a baked, optimized form of an operator suitable for execution on the GPU.
	V_RETURN(pDMLDevice->CompileOperator(dmlOperator.get(), GetDMLExecutionFlags(flags),
		IID_PPV_ARGS(&m_dispatchable)), cerr, false);

	return true;
}

Dispatchable Operator_DML::GetDispatchable() const
{
	return m_dispatchable.get();
}

CompiledOperator Operator_DML::GetCompiled()
{
	if (!m_compiledOperator)
		V_RETURN(m_dispatchable->QueryInterface(IID_PPV_ARGS(&m_compiledOperator)), cerr, nullptr);

	return m_compiledOperator.get();
}

uint32_t Operator_DML::GetDescriptorCount() const
{
	return m_dispatchable->GetBindingProperties().RequiredDescriptorCount;
}

uint64_t Operator_DML::GetTemporaryResourceSize() const
{
	return m_dispatchable->GetBindingProperties().TemporaryResourceSize;
}

uint64_t Operator_DML::GetPersistentResourceSize() const
{
	return m_dispatchable->GetBindingProperties().PersistentResourceSize;
}

//--------------------------------------------------------------------------------------
// Operator initializer
//--------------------------------------------------------------------------------------

OperatorInitializer_DML::OperatorInitializer_DML() :
	Operator()
{
}

OperatorInitializer_DML::~OperatorInitializer_DML()
{
}

bool OperatorInitializer_DML::Create(const Device* pDevice, const Operator::sptr* pOperators, uint32_t numOperators)
{
	vector<IDMLCompiledOperator*> compiledOperators(numOperators);
	for (auto i = 0u; i < numOperators; ++i)
		compiledOperators[i] = static_cast<IDMLCompiledOperator*>(pOperators[i]->GetCompiled());

	com_ptr<IDMLOperatorInitializer> operatorInitializer = nullptr;
	const auto pDMLDevice = static_cast<IDMLDevice*>(pDevice->GetHandle());
	V_RETURN(pDMLDevice->CreateOperatorInitializer(numOperators, compiledOperators.data(),
		IID_PPV_ARGS(&operatorInitializer)), cerr, false);
	m_dispatchable = operatorInitializer;

	// Query the operator for the required size (in descriptors) of its binding table.
	// You need to initialize an operator exactly once before it can be executed, and
	// the two stages require different numbers of descriptors for binding. For simplicity,
	// we create a single descriptor heap that's large enough to satisfy them both.
	const auto initializeBindingProperties = operatorInitializer->GetBindingProperties();
	m_descriptorCount = initializeBindingProperties.RequiredDescriptorCount;
	m_temporaryResourceSize = initializeBindingProperties.TemporaryResourceSize;
	for (auto i = 0u; i < numOperators; ++i)
	{
		m_descriptorCount = (max)(pOperators[i]->GetDescriptorCount(), m_descriptorCount);
		m_temporaryResourceSize = (max)(pOperators[i]->GetTemporaryResourceSize(), m_temporaryResourceSize);
	}

	return true;
}

uint32_t OperatorInitializer_DML::GetDescriptorCount() const
{
	return m_descriptorCount;
}

uint64_t OperatorInitializer_DML::GetTemporaryResourceSize() const
{
	return m_temporaryResourceSize;
}
