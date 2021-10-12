//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGTensor_DML.h"
#include "XUSGMachineLearning_DML.h"

using namespace XUSG::ML;

Tensor_DML::Tensor_DML() :
	m_bufferTensorDesc(),
	m_tensorDesc()
{
}

Tensor_DML::~Tensor_DML()
{
}

uint64_t Tensor_DML::Create(TensorDataType dataType, uint32_t dimensionCount, const uint32_t* pSizes,
	const uint32_t* pStrides, TensorFlag flags)
{
	m_bufferTensorDesc.DataType = GetDMLTensorDataType(dataType);
	m_bufferTensorDesc.Flags = GetDMLTensorFlags(flags);
	m_bufferTensorDesc.DimensionCount = dimensionCount;
	m_bufferTensorDesc.Sizes = pSizes;
	m_bufferTensorDesc.Strides = pStrides;
	m_bufferTensorDesc.TotalTensorSizeInBytes = calcBufferTensorSize(dataType, dimensionCount, pSizes, pStrides);

	// Create DirectML operator(s). Operators represent abstract functions such as "multiply", "reduce", "convolution", or even
	// compound operations such as recurrent neural nets. This example creates an instance of the Identity operator,
	// which applies the function f(x) = x for all elements in a tensor.
	m_tensorDesc.Type = DML_TENSOR_TYPE_BUFFER;
	m_tensorDesc.Desc = &m_bufferTensorDesc;

	return m_bufferTensorDesc.TotalTensorSizeInBytes;
}

uint64_t Tensor_DML::GetTensorBufferSize() const
{
	return m_bufferTensorDesc.TotalTensorSizeInBytes;
}

const void* Tensor_DML::GetHandle() const
{
	return &m_tensorDesc;
}

uint64_t Tensor_DML::calcBufferTensorSize(TensorDataType dataType, uint32_t dimensionCount,
	const uint32_t* pSizes, const uint32_t* pStrides)
{
	auto elementSizeInBytes = 0u;
	switch (dataType)
	{
	case TensorDataType::FLOAT32:
	case TensorDataType::UINT32:
	case TensorDataType::INT32:
		elementSizeInBytes = 4;
		break;

	case TensorDataType::FLOAT16:
	case TensorDataType::UINT16:
	case TensorDataType::INT16:
		elementSizeInBytes = 2;
		break;

	case TensorDataType::UINT8:
	case TensorDataType::INT8:
		elementSizeInBytes = 1;
		break;

	default:
		return 0; // Invalid data type
	}

	uint64_t minimumImpliedSizeInBytes = 0;
	if (!pStrides)
	{
		minimumImpliedSizeInBytes = *pSizes;
		for (auto i = 1u; i < dimensionCount; ++i)
		{
			minimumImpliedSizeInBytes *= pSizes[i];
		}
		minimumImpliedSizeInBytes *= elementSizeInBytes;
	}
	else
	{
		auto indexOfLastElement = 0u;
		for (auto i = 0u; i < dimensionCount; ++i)
		{
			indexOfLastElement += (pSizes[i] - 1) * pStrides[i];
		}

		minimumImpliedSizeInBytes = (indexOfLastElement + 1) * elementSizeInBytes;
	}

	// Round up to the nearest 4 bytes.
	minimumImpliedSizeInBytes = (minimumImpliedSizeInBytes + 3) & ~3ui64;

	return minimumImpliedSizeInBytes;
}
