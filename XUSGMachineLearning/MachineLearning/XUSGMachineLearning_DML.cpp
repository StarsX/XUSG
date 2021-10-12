//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGMachineLearning_DML.h"
#include "Core/XUSG_DX12.h"

using namespace std;
using namespace XUSG;

#define APPEND_FLAG(type, dmlType, flags, flag, none) ((flags & type::flag) == type::flag ? dmlType##_##flag : dmlType##_##none)
#define APPEND_EXECUTION_FLAG(flags, flag) APPEND_FLAG(ExecutionFlag, DML_EXECUTION_FLAG, flags, flag, NONE)

DML_TENSOR_DATA_TYPE ML::GetDMLTensorDataType(TensorDataType dataType)
{
	static const DML_TENSOR_DATA_TYPE dataTypes[] =
	{
		DML_TENSOR_DATA_TYPE_UNKNOWN,
		DML_TENSOR_DATA_TYPE_FLOAT32,
		DML_TENSOR_DATA_TYPE_FLOAT16,
		DML_TENSOR_DATA_TYPE_UINT32,
		DML_TENSOR_DATA_TYPE_UINT16,
		DML_TENSOR_DATA_TYPE_UINT8,
		DML_TENSOR_DATA_TYPE_INT32,
		DML_TENSOR_DATA_TYPE_INT16,
		DML_TENSOR_DATA_TYPE_INT8
	};

	return dataTypes[static_cast<uint32_t>(dataType)];
}

DML_OPERATOR_TYPE ML::GetDMLOpteratorType(OperatorType operatorType)
{
	static const DML_OPERATOR_TYPE operatorTypes[] =
	{
		DML_OPERATOR_INVALID,
		DML_OPERATOR_ELEMENT_WISE_IDENTITY,
		DML_OPERATOR_ELEMENT_WISE_ABS,
		DML_OPERATOR_ELEMENT_WISE_ACOS,
		DML_OPERATOR_ELEMENT_WISE_ADD,
		DML_OPERATOR_ELEMENT_WISE_ASIN,
		DML_OPERATOR_ELEMENT_WISE_ATAN,
		DML_OPERATOR_ELEMENT_WISE_CEIL,
		DML_OPERATOR_ELEMENT_WISE_CLIP,
		DML_OPERATOR_ELEMENT_WISE_COS,
		DML_OPERATOR_ELEMENT_WISE_DIVIDE,
		DML_OPERATOR_ELEMENT_WISE_EXP,
		DML_OPERATOR_ELEMENT_WISE_FLOOR,
		DML_OPERATOR_ELEMENT_WISE_LOG,
		DML_OPERATOR_ELEMENT_WISE_LOGICAL_AND,
		DML_OPERATOR_ELEMENT_WISE_LOGICAL_EQUALS,
		DML_OPERATOR_ELEMENT_WISE_LOGICAL_GREATER_THAN,
		DML_OPERATOR_ELEMENT_WISE_LOGICAL_LESS_THAN,
		DML_OPERATOR_ELEMENT_WISE_LOGICAL_NOT,
		DML_OPERATOR_ELEMENT_WISE_LOGICAL_OR,
		DML_OPERATOR_ELEMENT_WISE_LOGICAL_XOR,
		DML_OPERATOR_ELEMENT_WISE_MAX,
		DML_OPERATOR_ELEMENT_WISE_MEAN,
		DML_OPERATOR_ELEMENT_WISE_MIN,
		DML_OPERATOR_ELEMENT_WISE_MULTIPLY,
		DML_OPERATOR_ELEMENT_WISE_POW,
		DML_OPERATOR_ELEMENT_WISE_CONSTANT_POW,
		DML_OPERATOR_ELEMENT_WISE_RECIP,
		DML_OPERATOR_ELEMENT_WISE_SIN,
		DML_OPERATOR_ELEMENT_WISE_SQRT,
		DML_OPERATOR_ELEMENT_WISE_SUBTRACT,
		DML_OPERATOR_ELEMENT_WISE_TAN,
		DML_OPERATOR_ELEMENT_WISE_THRESHOLD,
		DML_OPERATOR_ELEMENT_WISE_QUANTIZE_LINEAR,
		DML_OPERATOR_ELEMENT_WISE_DEQUANTIZE_LINEAR,
		DML_OPERATOR_ACTIVATION_ELU,
		DML_OPERATOR_ACTIVATION_HARDMAX,
		DML_OPERATOR_ACTIVATION_HARD_SIGMOID,
		DML_OPERATOR_ACTIVATION_IDENTITY,
		DML_OPERATOR_ACTIVATION_LEAKY_RELU,
		DML_OPERATOR_ACTIVATION_LINEAR,
		DML_OPERATOR_ACTIVATION_LOG_SOFTMAX,
		DML_OPERATOR_ACTIVATION_PARAMETERIZED_RELU,
		DML_OPERATOR_ACTIVATION_PARAMETRIC_SOFTPLUS,
		DML_OPERATOR_ACTIVATION_RELU,
		DML_OPERATOR_ACTIVATION_SCALED_ELU,
		DML_OPERATOR_ACTIVATION_SCALED_TANH,
		DML_OPERATOR_ACTIVATION_SIGMOID,
		DML_OPERATOR_ACTIVATION_SOFTMAX,
		DML_OPERATOR_ACTIVATION_SOFTPLUS,
		DML_OPERATOR_ACTIVATION_SOFTSIGN,
		DML_OPERATOR_ACTIVATION_TANH,
		DML_OPERATOR_ACTIVATION_THRESHOLDED_RELU,
		DML_OPERATOR_CONVOLUTION,
		DML_OPERATOR_GEMM,
		DML_OPERATOR_REDUCE,
		DML_OPERATOR_AVERAGE_POOLING,
		DML_OPERATOR_LP_POOLING,
		DML_OPERATOR_MAX_POOLING,
		DML_OPERATOR_ROI_POOLING,
		DML_OPERATOR_SLICE,
		DML_OPERATOR_CAST,
		DML_OPERATOR_SPLIT,
		DML_OPERATOR_JOIN,
		DML_OPERATOR_PADDING,
		DML_OPERATOR_VALUE_SCALE_2D,
		DML_OPERATOR_UPSAMPLE_2D,
		DML_OPERATOR_GATHER,
		DML_OPERATOR_SPACE_TO_DEPTH,
		DML_OPERATOR_DEPTH_TO_SPACE,
		DML_OPERATOR_TILE,
		DML_OPERATOR_TOP_K,
		DML_OPERATOR_BATCH_NORMALIZATION,
		DML_OPERATOR_MEAN_VARIANCE_NORMALIZATION,
		DML_OPERATOR_LOCAL_RESPONSE_NORMALIZATION,
		DML_OPERATOR_LP_NORMALIZATION,
		DML_OPERATOR_RNN,
		DML_OPERATOR_LSTM,
		DML_OPERATOR_GRU
	};

	return operatorTypes[static_cast<uint32_t>(operatorType)];
}

DML_TENSOR_FLAGS ML::GetDMLTensorFlag(TensorFlag tensorFlag)
{
	static const DML_TENSOR_FLAGS tensorFlags[] =
	{
		DML_TENSOR_FLAG_OWNED_BY_DML
	};

	if (tensorFlag == TensorFlag::NONE) return DML_TENSOR_FLAG_NONE;

	const auto index = Log2(static_cast<uint32_t>(tensorFlag));

	return tensorFlags[index];
}

DML_TENSOR_FLAGS ML::GetDMLTensorFlags(TensorFlag tensorFlags)
{
	auto flags = DML_TENSOR_FLAG_NONE;
	flags |= (tensorFlags & TensorFlag::MANAGED) == TensorFlag::MANAGED ? DML_TENSOR_FLAG_OWNED_BY_DML : DML_TENSOR_FLAG_NONE;

	return flags;
}

DML_EXECUTION_FLAGS ML::GetDMLExecutionFlag(ExecutionFlag executionFlag)
{
	static const DML_EXECUTION_FLAGS executionFlags[] =
	{
		DML_EXECUTION_FLAG_ALLOW_HALF_PRECISION_COMPUTATION,
		DML_EXECUTION_FLAG_DISABLE_META_COMMANDS,
		DML_EXECUTION_FLAG_DESCRIPTORS_VOLATILE
	};

	if (executionFlag == ExecutionFlag::NONE) return DML_EXECUTION_FLAG_NONE;

	const auto index = Log2(static_cast<uint32_t>(executionFlag));

	return executionFlags[index];
}

DML_EXECUTION_FLAGS ML::GetDMLExecutionFlags(ExecutionFlag executionFlags)
{
	auto flags = DML_EXECUTION_FLAG_NONE;
	flags |= APPEND_EXECUTION_FLAG(executionFlags, ALLOW_HALF_PRECISION_COMPUTATION);
	flags |= APPEND_EXECUTION_FLAG(executionFlags, DISABLE_META_COMMANDS);
	flags |= APPEND_EXECUTION_FLAG(executionFlags, DESCRIPTORS_VOLATILE);

	return flags;
}

DML_REDUCE_FUNCTION ML::GetDMLReduceFunction(ReduceFunction reduceFunction)
{
	static const DML_REDUCE_FUNCTION reduceFunctions[] =
	{
		DML_REDUCE_FUNCTION_ARGMAX,
		DML_REDUCE_FUNCTION_ARGMIN,
		DML_REDUCE_FUNCTION_AVERAGE,
		DML_REDUCE_FUNCTION_L1,
		DML_REDUCE_FUNCTION_L2,
		DML_REDUCE_FUNCTION_LOG_SUM,
		DML_REDUCE_FUNCTION_LOG_SUM_EXP,
		DML_REDUCE_FUNCTION_MAX,
		DML_REDUCE_FUNCTION_MIN,
		DML_REDUCE_FUNCTION_MULTIPLY,
		DML_REDUCE_FUNCTION_SUM,
		DML_REDUCE_FUNCTION_SUM_SQUARE
	};

	return reduceFunctions[static_cast<uint32_t>(reduceFunction)];
}

DML_MATRIX_TRANSFORM ML::GetDMLMatrixTransform(MatrixTransform matrixTransform)
{
	static const DML_MATRIX_TRANSFORM matrixTransforms[] =
	{
		DML_MATRIX_TRANSFORM_NONE,
		DML_MATRIX_TRANSFORM_TRANSPOSE
	};

	return matrixTransforms[static_cast<uint32_t>(matrixTransform)];
}

DML_CONVOLUTION_MODE ML::GetDMLConvolutionMode(ConvolutionMode convolutionMode)
{
	static const DML_CONVOLUTION_MODE convolutionModes[] =
	{
		DML_CONVOLUTION_MODE_CONVOLUTION,
		DML_CONVOLUTION_MODE_CROSS_CORRELATION
	};

	return convolutionModes[static_cast<uint32_t>(convolutionMode)];
}

DML_CONVOLUTION_DIRECTION ML::GetDMLConvolutionDirection(ConvolutionDirection convolutionDirection)
{
	static const DML_CONVOLUTION_DIRECTION convolutionDirections[] =
	{
		DML_CONVOLUTION_DIRECTION_FORWARD,
		DML_CONVOLUTION_DIRECTION_BACKWARD
	};

	return convolutionDirections[static_cast<uint32_t>(convolutionDirection)];
}

DML_PADDING_MODE ML::GetDMLPaddingMode(PaddingType paddingMode)
{
	static const DML_PADDING_MODE paddingModes[] =
	{
		DML_PADDING_MODE_CONSTANT,
		DML_PADDING_MODE_EDGE,
		DML_PADDING_MODE_REFLECTION,
#if DML_TARGET_VERSION >= 0x3000
		DML_PADDING_MODE_SYMMETRIC
#else
		DML_PADDING_MODE_REFLECTION + 1
#endif
	};

	return paddingModes[static_cast<uint32_t>(paddingMode)];
}

DML_INTERPOLATION_MODE ML::GetDMLInterpolationMode(InterpolationType interpolationMode)
{
	static const DML_INTERPOLATION_MODE interpolationModes[] =
	{
		DML_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
		DML_INTERPOLATION_MODE_LINEAR
	};

	return interpolationModes[static_cast<uint32_t>(interpolationMode)];
}

void ML::GetDMLTypedOperator(vector<uint8_t>& dmlTypedOpDesc, OperatorType type, const void* pOpDesc)
{
	struct DMLUnaryOp
	{
		const DML_TENSOR_DESC* InputTensor;
		const DML_TENSOR_DESC* OutputTensor;
	};

	struct DMLElementWiseUnaryOp
	{
		const DML_TENSOR_DESC* InputTensor;
		const DML_TENSOR_DESC* OutputTensor;
		const DML_SCALE_BIAS* ScaleBias;
	};

	struct DMLElementWiseBinaryOp
	{
		const DML_TENSOR_DESC* ATensor;
		const DML_TENSOR_DESC* BTensor;
		const DML_TENSOR_DESC* OutputTensor;
	};

	static const auto getDMLUnaryOp = [](vector<uint8_t>& dmlTypedOpDesc, const void* pOpDesc)
	{
		dmlTypedOpDesc.resize(sizeof(DMLUnaryOp));
		const auto pDMLDesc = reinterpret_cast<DMLUnaryOp*>(dmlTypedOpDesc.data());
		const auto& desc = *static_cast<const UnaryOp*>(pOpDesc);

		pDMLDesc->InputTensor = desc.pInput ? static_cast<const DML_TENSOR_DESC*>(desc.pInput->GetHandle()) : nullptr;
		pDMLDesc->OutputTensor = desc.pOutput ? static_cast<const DML_TENSOR_DESC*>(desc.pOutput->GetHandle()) : nullptr;
	};

	static const auto getDMLElementWiseUnaryOp = [](vector<uint8_t>& dmlTypedOpDesc, const void* pOpDesc)
	{
		dmlTypedOpDesc.resize(sizeof(DMLElementWiseUnaryOp));
		const auto pDMLDesc = reinterpret_cast<DMLElementWiseUnaryOp*>(dmlTypedOpDesc.data());
		const auto& desc = *static_cast<const ElementWiseUnaryOp*>(pOpDesc);

		pDMLDesc->InputTensor = desc.pInput ? static_cast<const DML_TENSOR_DESC*>(desc.pInput->GetHandle()) : nullptr;
		pDMLDesc->OutputTensor = desc.pOutput ? static_cast<const DML_TENSOR_DESC*>(desc.pOutput->GetHandle()) : nullptr;
		pDMLDesc->ScaleBias = reinterpret_cast<const DML_SCALE_BIAS*>(desc.pScaleBias);
	};

	static const auto getDMLElementWiseBinaryOp = [](vector<uint8_t>& dmlTypedOpDesc, const void* pOpDesc)
	{
		dmlTypedOpDesc.resize(sizeof(DMLElementWiseBinaryOp));
		const auto pDMLDesc = reinterpret_cast<DMLElementWiseBinaryOp*>(dmlTypedOpDesc.data());
		const auto& desc = *static_cast<const ElementWiseBinaryOp*>(pOpDesc);

		pDMLDesc->ATensor = desc.pA ? static_cast<const DML_TENSOR_DESC*>(desc.pA->GetHandle()) : nullptr;
		pDMLDesc->BTensor = desc.pB ? static_cast<const DML_TENSOR_DESC*>(desc.pB->GetHandle()) : nullptr;
		pDMLDesc->OutputTensor = desc.pOutput ? static_cast<const DML_TENSOR_DESC*>(desc.pOutput->GetHandle()) : nullptr;
	};

	static const auto getDMLElementWiseClip = [](vector<uint8_t>& dmlTypedOpDesc, const void* pOpDesc)
	{
		dmlTypedOpDesc.resize(sizeof(DML_ELEMENT_WISE_CLIP_OPERATOR_DESC));
		const auto pDMLDesc = reinterpret_cast<DML_ELEMENT_WISE_CLIP_OPERATOR_DESC*>(dmlTypedOpDesc.data());
		const auto& desc = *static_cast<const ElementWiseClip*>(pOpDesc);

		pDMLDesc->InputTensor = desc.pInput ? static_cast<const DML_TENSOR_DESC*>(desc.pInput->GetHandle()) : nullptr;
		pDMLDesc->OutputTensor = desc.pOutput ? static_cast<const DML_TENSOR_DESC*>(desc.pOutput->GetHandle()) : nullptr;
		pDMLDesc->ScaleBias = reinterpret_cast<const DML_SCALE_BIAS*>(desc.pScaleBias);
		pDMLDesc->Min = desc.Min;
		pDMLDesc->Max = desc.Max;
	};

	static const auto getDMLElementWisePow = [](vector<uint8_t>& dmlTypedOpDesc, const void* pOpDesc)
	{
		dmlTypedOpDesc.resize(sizeof(DML_ELEMENT_WISE_POW_OPERATOR_DESC));
		const auto pDMLDesc = reinterpret_cast<DML_ELEMENT_WISE_POW_OPERATOR_DESC*>(dmlTypedOpDesc.data());
		const auto& desc = *static_cast<const ElementWisePow*>(pOpDesc);

		pDMLDesc->InputTensor = desc.pInput ? static_cast<const DML_TENSOR_DESC*>(desc.pInput->GetHandle()) : nullptr;
		pDMLDesc->ExponentTensor = desc.pExponent ? static_cast<const DML_TENSOR_DESC*>(desc.pExponent->GetHandle()) : nullptr;
		pDMLDesc->OutputTensor = desc.pOutput ? static_cast<const DML_TENSOR_DESC*>(desc.pOutput->GetHandle()) : nullptr;
		pDMLDesc->ScaleBias = reinterpret_cast<const DML_SCALE_BIAS*>(desc.pScaleBias);
	};

	static const auto getDMLElementWiseConstantPow = [](vector<uint8_t>& dmlTypedOpDesc, const void* pOpDesc)
	{
		dmlTypedOpDesc.resize(sizeof(DML_ELEMENT_WISE_CONSTANT_POW_OPERATOR_DESC));
		const auto pDMLDesc = reinterpret_cast<DML_ELEMENT_WISE_CONSTANT_POW_OPERATOR_DESC*>(dmlTypedOpDesc.data());
		const auto& desc = *static_cast<const ElementWiseConstantPow*>(pOpDesc);

		pDMLDesc->InputTensor = desc.pInput ? static_cast<const DML_TENSOR_DESC*>(desc.pInput->GetHandle()) : nullptr;
		pDMLDesc->OutputTensor = desc.pOutput ? static_cast<const DML_TENSOR_DESC*>(desc.pOutput->GetHandle()) : nullptr;
		pDMLDesc->ScaleBias = reinterpret_cast<const DML_SCALE_BIAS*>(desc.pScaleBias);
		pDMLDesc->Exponent = desc.Exponent;
	};

	static const auto getDMLElementWiseThreshold = [](vector<uint8_t>& dmlTypedOpDesc, const void* pOpDesc)
	{
		dmlTypedOpDesc.resize(sizeof(DML_ELEMENT_WISE_THRESHOLD_OPERATOR_DESC));
		const auto pDMLDesc = reinterpret_cast<DML_ELEMENT_WISE_THRESHOLD_OPERATOR_DESC*>(dmlTypedOpDesc.data());
		const auto& desc = *static_cast<const ElementWiseThreshold*>(pOpDesc);

		pDMLDesc->InputTensor = desc.pInput ? static_cast<const DML_TENSOR_DESC*>(desc.pInput->GetHandle()) : nullptr;
		pDMLDesc->OutputTensor = desc.pOutput ? static_cast<const DML_TENSOR_DESC*>(desc.pOutput->GetHandle()) : nullptr;
		pDMLDesc->ScaleBias = reinterpret_cast<const DML_SCALE_BIAS*>(desc.pScaleBias);
		pDMLDesc->Min = desc.Min;
	};

	static const auto getDMLElementWiseQuantizeLinear = [](vector<uint8_t>& dmlTypedOpDesc, const void* pOpDesc)
	{
		dmlTypedOpDesc.resize(sizeof(DML_ELEMENT_WISE_QUANTIZE_LINEAR_OPERATOR_DESC));
		const auto pDMLDesc = reinterpret_cast<DML_ELEMENT_WISE_QUANTIZE_LINEAR_OPERATOR_DESC*>(dmlTypedOpDesc.data());
		const auto& desc = *static_cast<const ElementWiseQuantizeLinear*>(pOpDesc);

		pDMLDesc->InputTensor = desc.pInput ? static_cast<const DML_TENSOR_DESC*>(desc.pInput->GetHandle()) : nullptr;
		pDMLDesc->ScaleTensor = desc.pScale ? static_cast<const DML_TENSOR_DESC*>(desc.pScale->GetHandle()) : nullptr;
		pDMLDesc->ZeroPointTensor = desc.pZeroPoint ? static_cast<const DML_TENSOR_DESC*>(desc.pZeroPoint->GetHandle()) : nullptr;
		pDMLDesc->OutputTensor = desc.pOutput ? static_cast<const DML_TENSOR_DESC*>(desc.pOutput->GetHandle()) : nullptr;
	};

	static const auto getDMLActivationELU = [](vector<uint8_t>& dmlTypedOpDesc, const void* pOpDesc)
	{
		dmlTypedOpDesc.resize(sizeof(DML_ACTIVATION_ELU_OPERATOR_DESC));
		const auto pDMLDesc = reinterpret_cast<DML_ACTIVATION_ELU_OPERATOR_DESC*>(dmlTypedOpDesc.data());
		const auto& desc = *static_cast<const ActivationELU*>(pOpDesc);

		pDMLDesc->InputTensor = desc.pInput ? static_cast<const DML_TENSOR_DESC*>(desc.pInput->GetHandle()) : nullptr;
		pDMLDesc->OutputTensor = desc.pOutput ? static_cast<const DML_TENSOR_DESC*>(desc.pOutput->GetHandle()) : nullptr;
		pDMLDesc->Alpha = desc.Alpha;
	};

	static const auto getDMLActivationLinear = [](vector<uint8_t>& dmlTypedOpDesc, const void* pOpDesc)
	{
		dmlTypedOpDesc.resize(sizeof(DML_ACTIVATION_LINEAR_OPERATOR_DESC));
		const auto pDMLDesc = reinterpret_cast<DML_ACTIVATION_LINEAR_OPERATOR_DESC*>(dmlTypedOpDesc.data());
		const auto& desc = *static_cast<const ActivationLinear*>(pOpDesc);

		pDMLDesc->InputTensor = desc.pInput ? static_cast<const DML_TENSOR_DESC*>(desc.pInput->GetHandle()) : nullptr;
		pDMLDesc->OutputTensor = desc.pOutput ? static_cast<const DML_TENSOR_DESC*>(desc.pOutput->GetHandle()) : nullptr;
		pDMLDesc->Alpha = desc.Alpha;
		pDMLDesc->Beta = desc.Beta;
	};

	static const auto getDMLActivationParameterizedRELU = [](vector<uint8_t>& dmlTypedOpDesc, const void* pOpDesc)
	{
		dmlTypedOpDesc.resize(sizeof(DML_ACTIVATION_PARAMETERIZED_RELU_OPERATOR_DESC));
		const auto pDMLDesc = reinterpret_cast<DML_ACTIVATION_PARAMETERIZED_RELU_OPERATOR_DESC*>(dmlTypedOpDesc.data());
		const auto& desc = *static_cast<const ActivationParameterizedRELU*>(pOpDesc);

		pDMLDesc->InputTensor = desc.pInput ? static_cast<const DML_TENSOR_DESC*>(desc.pInput->GetHandle()) : nullptr;
		pDMLDesc->SlopeTensor = desc.pSlope ? static_cast<const DML_TENSOR_DESC*>(desc.pSlope->GetHandle()) : nullptr;
		pDMLDesc->OutputTensor = desc.pOutput ? static_cast<const DML_TENSOR_DESC*>(desc.pOutput->GetHandle()) : nullptr;
	};

	static const auto getDMLActivationScaledELU = [](vector<uint8_t>& dmlTypedOpDesc, const void* pOpDesc)
	{
		dmlTypedOpDesc.resize(sizeof(DML_ACTIVATION_SCALED_ELU_OPERATOR_DESC));
		const auto pDMLDesc = reinterpret_cast<DML_ACTIVATION_SCALED_ELU_OPERATOR_DESC*>(dmlTypedOpDesc.data());
		const auto& desc = *static_cast<const ActivationScaledELU*>(pOpDesc);

		pDMLDesc->InputTensor = desc.pInput ? static_cast<const DML_TENSOR_DESC*>(desc.pInput->GetHandle()) : nullptr;
		pDMLDesc->OutputTensor = desc.pOutput ? static_cast<const DML_TENSOR_DESC*>(desc.pOutput->GetHandle()) : nullptr;
		pDMLDesc->Alpha = desc.Alpha;
		pDMLDesc->Gamma = desc.Gamma;
	};

	static const auto getDMLActivationSoftplus = [](vector<uint8_t>& dmlTypedOpDesc, const void* pOpDesc)
	{
		dmlTypedOpDesc.resize(sizeof(DML_ACTIVATION_SOFTPLUS_OPERATOR_DESC));
		const auto pDMLDesc = reinterpret_cast<DML_ACTIVATION_SOFTPLUS_OPERATOR_DESC*>(dmlTypedOpDesc.data());
		const auto& desc = *static_cast<const ActivationSoftplus*>(pOpDesc);

		pDMLDesc->InputTensor = desc.pInput ? static_cast<const DML_TENSOR_DESC*>(desc.pInput->GetHandle()) : nullptr;
		pDMLDesc->OutputTensor = desc.pOutput ? static_cast<const DML_TENSOR_DESC*>(desc.pOutput->GetHandle()) : nullptr;
		pDMLDesc->Steepness = desc.Steepness;
	};

	static const auto getDMLConvolution = [](vector<uint8_t>& dmlTypedOpDesc, const void* pOpDesc)
	{
		const auto& desc = *static_cast<const ConvolutionOperator*>(pOpDesc);

		vector<uint8_t> typedFused(0);
		if (desc.pFusedActivation) GetDMLTypedOperator(typedFused, desc.FusedActivationType, desc.pFusedActivation);

		dmlTypedOpDesc.resize(sizeof(DML_CONVOLUTION_OPERATOR_DESC) +
			(desc.pFusedActivation ? sizeof(DML_OPERATOR_DESC) + typedFused.size() : 0));
		const auto pDMLDesc = reinterpret_cast<DML_CONVOLUTION_OPERATOR_DESC*>(dmlTypedOpDesc.data());
		const auto pDMLFused = desc.pFusedActivation ? reinterpret_cast<DML_OPERATOR_DESC*>(
			&dmlTypedOpDesc[sizeof(DML_CONVOLUTION_OPERATOR_DESC)]) : nullptr;
		
		pDMLDesc->InputTensor = desc.pInput ? static_cast<const DML_TENSOR_DESC*>(desc.pInput->GetHandle()) : nullptr;
		pDMLDesc->FilterTensor = desc.pFilter ? static_cast<const DML_TENSOR_DESC*>(desc.pFilter->GetHandle()) : nullptr;
		pDMLDesc->BiasTensor = desc.pBias ? static_cast<const DML_TENSOR_DESC*>(desc.pBias->GetHandle()) : nullptr;
		pDMLDesc->OutputTensor = desc.pOutput ? static_cast<const DML_TENSOR_DESC*>(desc.pOutput->GetHandle()) : nullptr;
		pDMLDesc->Mode = GetDMLConvolutionMode(desc.Mode);
		pDMLDesc->Direction = GetDMLConvolutionDirection(desc.Direction);
		pDMLDesc->DimensionCount = desc.DimensionCount;
		pDMLDesc->Strides = desc.pStrides;
		pDMLDesc->Dilations = desc.pDilations;
		pDMLDesc->StartPadding = desc.pStartPadding;
		pDMLDesc->EndPadding = desc.pEndPadding;
		pDMLDesc->OutputPadding = desc.pOutputPadding;
		pDMLDesc->GroupCount = desc.GroupCount;
		pDMLDesc->FusedActivation = pDMLFused;

		if (pDMLFused)
		{
			assert(desc.pFusedActivation);
			const auto offset = sizeof(DML_CONVOLUTION_OPERATOR_DESC) + sizeof(DML_OPERATOR_DESC);
			pDMLFused->Type = GetDMLOpteratorType(desc.FusedActivationType);
			pDMLFused->Desc = &dmlTypedOpDesc[offset];
			memcpy(&dmlTypedOpDesc[offset], typedFused.data(), typedFused.size());
		}
	};

	static const auto getDMLGEMM = [](vector<uint8_t>& dmlTypedOpDesc, const void* pOpDesc)
	{
		const auto& desc = *static_cast<const GEMMOperator*>(pOpDesc);

		vector<uint8_t> typedFused(0);
		if (desc.pFusedActivation) GetDMLTypedOperator(typedFused, desc.FusedActivationType, desc.pFusedActivation);

		dmlTypedOpDesc.resize(sizeof(DML_GEMM_OPERATOR_DESC) +
			(desc.pFusedActivation ? sizeof(DML_OPERATOR_DESC) + typedFused.size() : 0));
		const auto pDMLDesc = reinterpret_cast<DML_GEMM_OPERATOR_DESC*>(dmlTypedOpDesc.data());
		const auto pDMLFused = desc.pFusedActivation ? reinterpret_cast<DML_OPERATOR_DESC*>(
			&dmlTypedOpDesc[sizeof(DML_GEMM_OPERATOR_DESC)]) : nullptr;

		pDMLDesc->ATensor = desc.pA ? static_cast<const DML_TENSOR_DESC*>(desc.pA->GetHandle()) : nullptr;
		pDMLDesc->BTensor = desc.pB ? static_cast<const DML_TENSOR_DESC*>(desc.pB->GetHandle()) : nullptr;
		pDMLDesc->CTensor = desc.pC ? static_cast<const DML_TENSOR_DESC*>(desc.pC->GetHandle()) : nullptr;
		pDMLDesc->OutputTensor = desc.pOutput ? static_cast<const DML_TENSOR_DESC*>(desc.pOutput->GetHandle()) : nullptr;
		pDMLDesc->TransA = GetDMLMatrixTransform(desc.TransA);
		pDMLDesc->TransB = GetDMLMatrixTransform(desc.TransB);
		pDMLDesc->Alpha = desc.Alpha;
		pDMLDesc->Beta = desc.Beta;
		pDMLDesc->FusedActivation = pDMLFused;

		if (pDMLFused)
		{
			assert(desc.pFusedActivation);
			const auto offset = sizeof(DML_GEMM_OPERATOR_DESC) + sizeof(DML_OPERATOR_DESC);
			pDMLFused->Type = GetDMLOpteratorType(desc.FusedActivationType);
			pDMLFused->Desc = &dmlTypedOpDesc[offset];
			memcpy(&dmlTypedOpDesc[offset], typedFused.data(), typedFused.size());
		}
	};

	static const auto getDMLReduce = [](vector<uint8_t>& dmlTypedOpDesc, const void* pOpDesc)
	{
		dmlTypedOpDesc.resize(sizeof(DML_REDUCE_OPERATOR_DESC));
		const auto pDMLDesc = reinterpret_cast<DML_REDUCE_OPERATOR_DESC*>(dmlTypedOpDesc.data());
		const auto& desc = *static_cast<const ReduceOperator*>(pOpDesc);

		pDMLDesc->Function = GetDMLReduceFunction(desc.Function);
		pDMLDesc->InputTensor = desc.pInput ? static_cast<const DML_TENSOR_DESC*>(desc.pInput->GetHandle()) : nullptr;
		pDMLDesc->OutputTensor = desc.pOutput ? static_cast<const DML_TENSOR_DESC*>(desc.pOutput->GetHandle()) : nullptr;
		pDMLDesc->AxisCount = desc.AxisCount;
		pDMLDesc->Axes = desc.pAxes;
	};

	static const auto getDMLAveragePooling = [](vector<uint8_t>& dmlTypedOpDesc, const void* pOpDesc)
	{
		dmlTypedOpDesc.resize(sizeof(DML_AVERAGE_POOLING_OPERATOR_DESC));
		const auto pDMLDesc = reinterpret_cast<DML_AVERAGE_POOLING_OPERATOR_DESC*>(dmlTypedOpDesc.data());
		const auto& desc = *static_cast<const AveragePooling*>(pOpDesc);

		pDMLDesc->InputTensor = desc.pInput ? static_cast<const DML_TENSOR_DESC*>(desc.pInput->GetHandle()) : nullptr;
		pDMLDesc->OutputTensor = desc.pOutput ? static_cast<const DML_TENSOR_DESC*>(desc.pOutput->GetHandle()) : nullptr;
		pDMLDesc->DimensionCount = desc.DimensionCount;
		pDMLDesc->Strides = desc.pStrides;
		pDMLDesc->WindowSize = desc.pWindowSize;
		pDMLDesc->StartPadding = desc.pStartPadding;
		pDMLDesc->EndPadding = desc.pEndPadding;
		pDMLDesc->IncludePadding = desc.IncludePadding;
	};

	static const auto getDMLLPPooling = [](vector<uint8_t>& dmlTypedOpDesc, const void* pOpDesc)
	{
		dmlTypedOpDesc.resize(sizeof(DML_LP_POOLING_OPERATOR_DESC));
		const auto pDMLDesc = reinterpret_cast<DML_LP_POOLING_OPERATOR_DESC*>(dmlTypedOpDesc.data());
		const auto& desc = *static_cast<const LPPooling*>(pOpDesc);

		pDMLDesc->InputTensor = desc.pInput ? static_cast<const DML_TENSOR_DESC*>(desc.pInput->GetHandle()) : nullptr;
		pDMLDesc->OutputTensor = desc.pOutput ? static_cast<const DML_TENSOR_DESC*>(desc.pOutput->GetHandle()) : nullptr;
		pDMLDesc->DimensionCount = desc.DimensionCount;
		pDMLDesc->Strides = desc.pStrides;
		pDMLDesc->WindowSize = desc.pWindowSize;
		pDMLDesc->StartPadding = desc.pStartPadding;
		pDMLDesc->EndPadding = desc.pEndPadding;
		pDMLDesc->P = desc.P;
	};

	static const auto getDMLMaxPooling = [](vector<uint8_t>& dmlTypedOpDesc, const void* pOpDesc)
	{
		dmlTypedOpDesc.resize(sizeof(DML_MAX_POOLING_OPERATOR_DESC));
		const auto pDMLDesc = reinterpret_cast<DML_MAX_POOLING_OPERATOR_DESC*>(dmlTypedOpDesc.data());
		const auto& desc = *static_cast<const MaxPooling*>(pOpDesc);

		pDMLDesc->InputTensor = desc.pInput ? static_cast<const DML_TENSOR_DESC*>(desc.pInput->GetHandle()) : nullptr;
		pDMLDesc->OutputTensor = desc.pOutput ? static_cast<const DML_TENSOR_DESC*>(desc.pOutput->GetHandle()) : nullptr;
		pDMLDesc->DimensionCount = desc.DimensionCount;
		pDMLDesc->Strides = desc.pStrides;
		pDMLDesc->WindowSize = desc.pWindowSize;
		pDMLDesc->StartPadding = desc.pStartPadding;
		pDMLDesc->EndPadding = desc.pEndPadding;
	};

	static const auto getDMLROIPooling = [](vector<uint8_t>& dmlTypedOpDesc, const void* pOpDesc)
	{
		dmlTypedOpDesc.resize(sizeof(DML_ROI_POOLING_OPERATOR_DESC));
		const auto pDMLDesc = reinterpret_cast<DML_ROI_POOLING_OPERATOR_DESC*>(dmlTypedOpDesc.data());
		const auto& desc = *static_cast<const ROIPooling*>(pOpDesc);

		pDMLDesc->InputTensor = desc.pInput ? static_cast<const DML_TENSOR_DESC*>(desc.pInput->GetHandle()) : nullptr;
		pDMLDesc->ROITensor = desc.pROI ? static_cast<const DML_TENSOR_DESC*>(desc.pROI->GetHandle()) : nullptr;
		pDMLDesc->OutputTensor = desc.pOutput ? static_cast<const DML_TENSOR_DESC*>(desc.pOutput->GetHandle()) : nullptr;
		pDMLDesc->SpatialScale = desc.SpatialScale;
		pDMLDesc->PooledSize.Width = desc.PooledWidth;
		pDMLDesc->PooledSize.Height = desc.PooledHeight;
	};

	static const auto getDMLSlice = [](vector<uint8_t>& dmlTypedOpDesc, const void* pOpDesc)
	{
		dmlTypedOpDesc.resize(sizeof(DML_SLICE_OPERATOR_DESC));
		const auto pDMLDesc = reinterpret_cast<DML_SLICE_OPERATOR_DESC*>(dmlTypedOpDesc.data());
		const auto& desc = *static_cast<const SliceOperator*>(pOpDesc);

		pDMLDesc->InputTensor = desc.pInput ? static_cast<const DML_TENSOR_DESC*>(desc.pInput->GetHandle()) : nullptr;
		pDMLDesc->OutputTensor = desc.pOutput ? static_cast<const DML_TENSOR_DESC*>(desc.pOutput->GetHandle()) : nullptr;
		pDMLDesc->DimensionCount = desc.DimensionCount;
		pDMLDesc->Offsets = desc.pOffsets;
		pDMLDesc->Sizes = desc.pSizes;
		pDMLDesc->Strides = desc.pStrides;
	};

	static const auto getDMLSplit = [](vector<uint8_t>& dmlTypedOpDesc, const void* pOpDesc)
	{
		dmlTypedOpDesc.resize(sizeof(DML_SPLIT_OPERATOR_DESC));
		const auto pDMLDesc = reinterpret_cast<DML_SPLIT_OPERATOR_DESC*>(dmlTypedOpDesc.data());
		const auto& desc = *static_cast<const SplitOperator*>(pOpDesc);

		pDMLDesc->InputTensor = desc.pInput ? static_cast<const DML_TENSOR_DESC*>(desc.pInput->GetHandle()) : nullptr;
		pDMLDesc->OutputCount = desc.OutputCount;
		pDMLDesc->OutputTensors = desc.pOutputs ? static_cast<const DML_TENSOR_DESC*>(desc.pOutputs->GetHandle()) : nullptr;
		pDMLDesc->Axis = desc.Axis;
	};

	static const auto getDMLJoin = [](vector<uint8_t>& dmlTypedOpDesc, const void* pOpDesc)
	{
		dmlTypedOpDesc.resize(sizeof(DML_JOIN_OPERATOR_DESC));
		const auto pDMLDesc = reinterpret_cast<DML_JOIN_OPERATOR_DESC*>(dmlTypedOpDesc.data());
		const auto& desc = *static_cast<const JoinOperator*>(pOpDesc);

		pDMLDesc->InputCount = desc.InputCount;
		pDMLDesc->InputTensors = desc.pInputs ? static_cast<const DML_TENSOR_DESC*>(desc.pInputs->GetHandle()) : nullptr;
		pDMLDesc->OutputTensor = desc.pOutput ? static_cast<const DML_TENSOR_DESC*>(desc.pOutput->GetHandle()) : nullptr;
		pDMLDesc->Axis = desc.Axis;
	};

	static const auto getDMLPadding = [](vector<uint8_t>& dmlTypedOpDesc, const void* pOpDesc)
	{
		dmlTypedOpDesc.resize(sizeof(DML_PADDING_OPERATOR_DESC));
		const auto pDMLDesc = reinterpret_cast<DML_PADDING_OPERATOR_DESC*>(dmlTypedOpDesc.data());
		const auto& desc = *static_cast<const PaddingOperator*>(pOpDesc);

		pDMLDesc->InputTensor = desc.pInput ? static_cast<const DML_TENSOR_DESC*>(desc.pInput->GetHandle()) : nullptr;
		pDMLDesc->OutputTensor = desc.pOutput ? static_cast<const DML_TENSOR_DESC*>(desc.pOutput->GetHandle()) : nullptr;
		pDMLDesc->PaddingMode = GetDMLPaddingMode(desc.PaddingMode);
		pDMLDesc->PaddingValue = desc.PaddingValue;
		pDMLDesc->DimensionCount = desc.DimensionCount;
		pDMLDesc->StartPadding = desc.pStartPadding;
		pDMLDesc->EndPadding = desc.pEndPadding;
	};

	static const auto getDMLValueScale2D = [](vector<uint8_t>& dmlTypedOpDesc, const void* pOpDesc)
	{
		dmlTypedOpDesc.resize(sizeof(DML_VALUE_SCALE_2D_OPERATOR_DESC));
		const auto pDMLDesc = reinterpret_cast<DML_VALUE_SCALE_2D_OPERATOR_DESC*>(dmlTypedOpDesc.data());
		const auto& desc = *static_cast<const ValueScale2D*>(pOpDesc);

		pDMLDesc->InputTensor = desc.pInput ? static_cast<const DML_TENSOR_DESC*>(desc.pInput->GetHandle()) : nullptr;
		pDMLDesc->OutputTensor = desc.pOutput ? static_cast<const DML_TENSOR_DESC*>(desc.pOutput->GetHandle()) : nullptr;
		pDMLDesc->Scale = desc.Scale;
		pDMLDesc->ChannelCount = desc.ChannelCount;
		pDMLDesc->Bias = desc.pBias;
	};

	static const auto getDMLUpsample2D = [](vector<uint8_t>& dmlTypedOpDesc, const void* pOpDesc)
	{
		dmlTypedOpDesc.resize(sizeof(DML_UPSAMPLE_2D_OPERATOR_DESC));
		const auto pDMLDesc = reinterpret_cast<DML_UPSAMPLE_2D_OPERATOR_DESC*>(dmlTypedOpDesc.data());
		const auto& desc = *static_cast<const Upsample2D*>(pOpDesc);

		pDMLDesc->InputTensor = desc.pInput ? static_cast<const DML_TENSOR_DESC*>(desc.pInput->GetHandle()) : nullptr;
		pDMLDesc->OutputTensor = desc.pOutput ? static_cast<const DML_TENSOR_DESC*>(desc.pOutput->GetHandle()) : nullptr;
		pDMLDesc->ScaleSize.Width = desc.ScaleWidth;
		pDMLDesc->ScaleSize.Height = desc.ScaleHeight;
		pDMLDesc->InterpolationMode = GetDMLInterpolationMode(desc.InterpolationMode);
	};

	static const function<void(vector<uint8_t>&, const void*)> pfnGetDMLOps[] =
	{
		nullptr,							// INVALID 
		getDMLElementWiseUnaryOp,			// ELEMENT_WISE_IDENTITY
		getDMLElementWiseUnaryOp,			// ELEMENT_WISE_ABS
		getDMLElementWiseUnaryOp,			// ELEMENT_WISE_ACOS
		getDMLElementWiseBinaryOp,			// ELEMENT_WISE_ADD
		getDMLElementWiseUnaryOp,			// ELEMENT_WISE_ASIN
		getDMLElementWiseUnaryOp,			// ELEMENT_WISE_ATAN
		getDMLElementWiseUnaryOp,			// ELEMENT_WISE_CEIL
		getDMLElementWiseClip,				// ELEMENT_WISE_CLIP
		getDMLElementWiseUnaryOp,			// ELEMENT_WISE_COS
		getDMLElementWiseBinaryOp,			// ELEMENT_WISE_DIVIDE
		getDMLElementWiseUnaryOp,			// ELEMENT_WISE_EXP
		getDMLElementWiseUnaryOp,			// ELEMENT_WISE_FLOOR
		getDMLElementWiseUnaryOp,			// ELEMENT_WISE_LOG
		getDMLElementWiseBinaryOp,			// ELEMENT_WISE_LOGICAL_AND
		getDMLElementWiseBinaryOp,			// ELEMENT_WISE_LOGICAL_EQUALS
		getDMLElementWiseBinaryOp,			// ELEMENT_WISE_LOGICAL_GREATER_THAN
		getDMLElementWiseBinaryOp,			// ELEMENT_WISE_LOGICAL_LESS_THAN
		getDMLUnaryOp,						// ELEMENT_WISE_LOGICAL_NOT
		getDMLElementWiseBinaryOp,			// ELEMENT_WISE_LOGICAL_OR
		getDMLElementWiseBinaryOp,			// ELEMENT_WISE_LOGICAL_XOR
		getDMLElementWiseBinaryOp,			// ELEMENT_WISE_MAX
		getDMLElementWiseBinaryOp,			// ELEMENT_WISE_MEAN
		getDMLElementWiseBinaryOp,			// ELEMENT_WISE_MIN
		getDMLElementWiseBinaryOp,			// ELEMENT_WISE_MULTIPLY
		getDMLElementWisePow,				// ELEMENT_WISE_POW
		getDMLElementWiseConstantPow,		// ELEMENT_WISE_CONSTANT_POW
		getDMLElementWiseUnaryOp,			// ELEMENT_WISE_RECIP
		getDMLElementWiseUnaryOp,			// ELEMENT_WISE_SIN
		getDMLElementWiseUnaryOp,			// ELEMENT_WISE_SQRT
		getDMLElementWiseBinaryOp,			// ELEMENT_WISE_SUBTRACT
		getDMLElementWiseUnaryOp,			// ELEMENT_WISE_TAN
		getDMLElementWiseThreshold,			// ELEMENT_WISE_THRESHOLD
		getDMLElementWiseQuantizeLinear,	// ELEMENT_WISE_QUANTIZE_LINEAR
		getDMLElementWiseQuantizeLinear,	// ELEMENT_WISE_DEQUANTIZE_LINEAR

		getDMLActivationELU,				// ACTIVATION_ELU
		getDMLUnaryOp,						// ACTIVATION_HARDMAX
		getDMLActivationLinear,				// ACTIVATION_HARD_SIGMOID
		getDMLUnaryOp,						// ACTIVATION_IDENTITY
		getDMLActivationELU,				// ACTIVATION_LEAKY_RELU
		getDMLActivationLinear,				// ACTIVATION_LINEAR
		getDMLUnaryOp,						// ACTIVATION_LOG_SOFTMAX
		getDMLActivationParameterizedRELU,	// ACTIVATION_PARAMETERIZED_RELU
		getDMLActivationLinear,				// ACTIVATION_PARAMETRIC_SOFTPLUS
		getDMLUnaryOp,						// ACTIVATION_RELU
		getDMLActivationScaledELU,			// ACTIVATION_SCALED_ELU
		getDMLActivationLinear,				// ACTIVATION_SCALED_TANH
		getDMLUnaryOp,						// ACTIVATION_SIGMOID
		getDMLUnaryOp,						// ACTIVATION_SOFTMAX
		getDMLActivationSoftplus,			// ACTIVATION_SOFTPLUS
		getDMLUnaryOp,						// ACTIVATION_SOFTSIGN
		getDMLUnaryOp,						// ACTIVATION_TANH
		getDMLActivationELU,				// ACTIVATION_THRESHOLDED_RELU

		getDMLConvolution,					// CONVOLUTION
		getDMLGEMM,							// GEMM
		getDMLReduce,						// REDUCE
		getDMLAveragePooling,				// AVERAGE_POOLING
		getDMLLPPooling,					// LP_POOLING
		getDMLMaxPooling,					// MAX_POOLING
		getDMLROIPooling,					// ROI_POOLING
		getDMLSlice,						// SLICE
		getDMLUnaryOp,						// CAST
		getDMLSplit,						// SPLIT
		getDMLJoin,							// JOIN
		getDMLPadding,						// PADDING
		getDMLValueScale2D,					// VALUE_SCALE_2D
		getDMLUpsample2D,					// UPSAMPLE_2D
	};

	pfnGetDMLOps[static_cast<uint32_t>(type)](dmlTypedOpDesc, pOpDesc);
}

//--------------------------------------------------------------------------------------

using namespace XUSG::ML;

Device_DML::Device_DML()
{
}

Device_DML::~Device_DML()
{
}

bool Device_DML::GetCommandRecorder(CommandRecorder* pCommandRecorder, const wchar_t* name)
{
	return pCommandRecorder->Create(this, name);
}

bool Device_DML::Create(const XUSG::Device* pDevice, CreateDeviceFlag flags, const wchar_t* name)
{
	const auto pDxDevice = static_cast<ID3D12Device*>(pDevice->GetHandle());

	assert(pDxDevice);
	V_RETURN(DMLCreateDevice(pDxDevice, static_cast<DML_CREATE_DEVICE_FLAGS>(flags), IID_PPV_ARGS(&m_device)), cerr, false);
	if (name) m_device->SetName(name);

	return true;
}

void Device_DML::Create(void* pHandle, const wchar_t* name)
{
	m_device = static_cast<IDMLDevice*>(pHandle);
	if (name) m_device->SetName(name);
}

void* Device_DML::GetHandle() const
{
	return m_device.get();
}
