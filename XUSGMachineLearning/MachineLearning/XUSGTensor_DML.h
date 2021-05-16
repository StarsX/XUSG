//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSGMachineLearning.h"

namespace XUSG
{
	namespace ML
	{
		class Tensor_DML :
			public virtual Tensor
		{
		public:
			Tensor_DML();
			virtual ~Tensor_DML();

			uint64_t Create(TensorDataType dataType, uint32_t dimensionCount,
				const uint32_t* pSizes, const uint32_t* pStrides = nullptr,
				TensorFlag flags = TensorFlag::NONE);
			uint64_t GetTensorBufferSize() const;

			const void* GetHandle() const;

		protected:
			uint64_t calcBufferTensorSize(TensorDataType dataType, uint32_t dimensionCount,
				const uint32_t* pSizes, const uint32_t* pStrides);

			DML_BUFFER_TENSOR_DESC	m_bufferTensorDesc;
			DML_TENSOR_DESC			m_tensorDesc;
		};
	}
}
