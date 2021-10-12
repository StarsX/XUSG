//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSGMachineLearning.h"

namespace XUSG
{
	namespace ML
	{
		//--------------------------------------------------------------------------------------
		// Operator
		//--------------------------------------------------------------------------------------
		class Operator_DML :
			public virtual Operator
		{
		public:
			Operator_DML();
			virtual ~Operator_DML();

			bool Create(const ML::Device* pDevice, OperatorType type, const void* pTypedOp,
				ExecutionFlag flags = ExecutionFlag::NONE);

			Dispatchable GetDispatchable() const;

			CompiledOperator GetCompiled();

			uint32_t GetDescriptorCount() const;
			uint64_t GetTemporaryResourceSize() const;
			uint64_t GetPersistentResourceSize() const;

		protected:
			com_ptr<IDMLDispatchable>		m_dispatchable;
			com_ptr<IDMLCompiledOperator>	m_compiledOperator;
		};

		//--------------------------------------------------------------------------------------
		// Operator initializer
		//--------------------------------------------------------------------------------------
		class OperatorInitializer_DML :
			public virtual OperatorInitializer,
			public virtual Operator_DML
		{
		public:
			OperatorInitializer_DML();
			virtual ~OperatorInitializer_DML();

			bool Create(const Device* pDevice, const Operator::sptr* pOperators, uint32_t numOperators);

			uint32_t GetDescriptorCount() const;
			uint64_t GetTemporaryResourceSize() const;

		protected:
			uint32_t m_descriptorCount;
			uint64_t m_temporaryResourceSize;
		};
	}
}
