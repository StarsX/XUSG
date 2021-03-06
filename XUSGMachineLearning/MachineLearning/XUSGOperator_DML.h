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

			bool Create(const Device& device, const OperatorDesc& desc, ExecutionFlag flags = ExecutionFlag::NONE);

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

			bool Create(const Device& device, const Operator::sptr* pOperators, uint32_t numOperators);

			uint32_t GetDescriptorCount() const;
			uint64_t GetTemporaryResourceSize() const;

		protected:
			uint32_t m_descriptorCount;
			uint64_t m_temporaryResourceSize;
		};

		//--------------------------------------------------------------------------------------
		// Typed operators
		//--------------------------------------------------------------------------------------
		struct Upsample2D_DML :
			public Upsample2D
		{
			Upsample2D_ML(const Tensor* pInputTensor, const Tensor* pOutputTensor,
				uint32_t scaleSizeX, uint32_t scaleSizeY, InterpolationType interpolationType);

			const void* GetDesc() const;

			//DML_UPSAMPLE_2D_OPERATOR_DESC
		};
	}
}
