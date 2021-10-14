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

void ML::GetDMLTypedOperator(string& dmlTypedOpDesc, const void* pTypedOp)
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

	static const auto getDMLUnaryOp = [](string& dmlTypedOpDesc, const void* pTypedOp)
	{
		dmlTypedOpDesc.resize(sizeof(DMLUnaryOp));
		auto& dmlDesc = reinterpret_cast<DMLUnaryOp&>(dmlTypedOpDesc[0]);
		const auto& typedOp = *static_cast<const UnaryOp*>(pTypedOp);

		dmlDesc.InputTensor = typedOp.pInput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pInput->GetHandle()) : nullptr;
		dmlDesc.OutputTensor = typedOp.pOutput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pOutput->GetHandle()) : nullptr;
	};

	static const auto getDMLElementWiseUnaryOp = [](string& dmlTypedOpDesc, const void* pTypedOp)
	{
		dmlTypedOpDesc.resize(sizeof(DMLElementWiseUnaryOp));
		auto& dmlDesc = reinterpret_cast<DMLElementWiseUnaryOp&>(dmlTypedOpDesc[0]);
		const auto& typedOp = *static_cast<const ElementWiseUnaryOp*>(pTypedOp);

		dmlDesc.InputTensor = typedOp.pInput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pInput->GetHandle()) : nullptr;
		dmlDesc.OutputTensor = typedOp.pOutput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pOutput->GetHandle()) : nullptr;
		dmlDesc.ScaleBias = reinterpret_cast<const DML_SCALE_BIAS*>(typedOp.pScaleBias);
	};

	static const auto getDMLElementWiseBinaryOp = [](string& dmlTypedOpDesc, const void* pTypedOp)
	{
		dmlTypedOpDesc.resize(sizeof(DMLElementWiseBinaryOp));
		auto& dmlDesc = reinterpret_cast<DMLElementWiseBinaryOp&>(dmlTypedOpDesc[0]);
		const auto& typedOp = *static_cast<const ElementWiseBinaryOp*>(pTypedOp);

		dmlDesc.ATensor = typedOp.pA ? static_cast<const DML_TENSOR_DESC*>(typedOp.pA->GetHandle()) : nullptr;
		dmlDesc.BTensor = typedOp.pB ? static_cast<const DML_TENSOR_DESC*>(typedOp.pB->GetHandle()) : nullptr;
		dmlDesc.OutputTensor = typedOp.pOutput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pOutput->GetHandle()) : nullptr;
	};

	static const auto getDMLElementWiseClip = [](string& dmlTypedOpDesc, const void* pTypedOp)
	{
		dmlTypedOpDesc.resize(sizeof(DML_ELEMENT_WISE_CLIP_OPERATOR_DESC));
		auto& dmlDesc = reinterpret_cast<DML_ELEMENT_WISE_CLIP_OPERATOR_DESC&>(dmlTypedOpDesc[0]);
		const auto& typedOp = *static_cast<const ElementWiseClip*>(pTypedOp);

		dmlDesc.InputTensor = typedOp.pInput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pInput->GetHandle()) : nullptr;
		dmlDesc.OutputTensor = typedOp.pOutput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pOutput->GetHandle()) : nullptr;
		dmlDesc.ScaleBias = reinterpret_cast<const DML_SCALE_BIAS*>(typedOp.pScaleBias);
		dmlDesc.Min = typedOp.Min;
		dmlDesc.Max = typedOp.Max;
	};

	static const auto getDMLElementWisePow = [](string& dmlTypedOpDesc, const void* pTypedOp)
	{
		dmlTypedOpDesc.resize(sizeof(DML_ELEMENT_WISE_POW_OPERATOR_DESC));
		auto& dmlDesc = reinterpret_cast<DML_ELEMENT_WISE_POW_OPERATOR_DESC&>(dmlTypedOpDesc[0]);
		const auto& typedOp = *static_cast<const ElementWisePow*>(pTypedOp);

		dmlDesc.InputTensor = typedOp.pInput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pInput->GetHandle()) : nullptr;
		dmlDesc.ExponentTensor = typedOp.pExponent ? static_cast<const DML_TENSOR_DESC*>(typedOp.pExponent->GetHandle()) : nullptr;
		dmlDesc.OutputTensor = typedOp.pOutput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pOutput->GetHandle()) : nullptr;
		dmlDesc.ScaleBias = reinterpret_cast<const DML_SCALE_BIAS*>(typedOp.pScaleBias);
	};

	static const auto getDMLElementWiseConstantPow = [](string& dmlTypedOpDesc, const void* pTypedOp)
	{
		dmlTypedOpDesc.resize(sizeof(DML_ELEMENT_WISE_CONSTANT_POW_OPERATOR_DESC));
		auto& dmlDesc = reinterpret_cast<DML_ELEMENT_WISE_CONSTANT_POW_OPERATOR_DESC&>(dmlTypedOpDesc[0]);
		const auto& typedOp = *static_cast<const ElementWiseConstantPow*>(pTypedOp);

		dmlDesc.InputTensor = typedOp.pInput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pInput->GetHandle()) : nullptr;
		dmlDesc.OutputTensor = typedOp.pOutput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pOutput->GetHandle()) : nullptr;
		dmlDesc.ScaleBias = reinterpret_cast<const DML_SCALE_BIAS*>(typedOp.pScaleBias);
		dmlDesc.Exponent = typedOp.Exponent;
	};

	static const auto getDMLElementWiseThreshold = [](string& dmlTypedOpDesc, const void* pTypedOp)
	{
		dmlTypedOpDesc.resize(sizeof(DML_ELEMENT_WISE_THRESHOLD_OPERATOR_DESC));
		auto& dmlDesc = reinterpret_cast<DML_ELEMENT_WISE_THRESHOLD_OPERATOR_DESC&>(dmlTypedOpDesc[0]);
		const auto& typedOp = *static_cast<const ElementWiseThreshold*>(pTypedOp);

		dmlDesc.InputTensor = typedOp.pInput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pInput->GetHandle()) : nullptr;
		dmlDesc.OutputTensor = typedOp.pOutput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pOutput->GetHandle()) : nullptr;
		dmlDesc.ScaleBias = reinterpret_cast<const DML_SCALE_BIAS*>(typedOp.pScaleBias);
		dmlDesc.Min = typedOp.Min;
	};

	static const auto getDMLElementWiseQuantizeLinear = [](string& dmlTypedOpDesc, const void* pTypedOp)
	{
		dmlTypedOpDesc.resize(sizeof(DML_ELEMENT_WISE_QUANTIZE_LINEAR_OPERATOR_DESC));
		auto& dmlDesc = reinterpret_cast<DML_ELEMENT_WISE_QUANTIZE_LINEAR_OPERATOR_DESC&>(dmlTypedOpDesc[0]);
		const auto& typedOp = *static_cast<const ElementWiseQuantizeLinear*>(pTypedOp);

		dmlDesc.InputTensor = typedOp.pInput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pInput->GetHandle()) : nullptr;
		dmlDesc.ScaleTensor = typedOp.pScale ? static_cast<const DML_TENSOR_DESC*>(typedOp.pScale->GetHandle()) : nullptr;
		dmlDesc.ZeroPointTensor = typedOp.pZeroPoint ? static_cast<const DML_TENSOR_DESC*>(typedOp.pZeroPoint->GetHandle()) : nullptr;
		dmlDesc.OutputTensor = typedOp.pOutput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pOutput->GetHandle()) : nullptr;
	};

	static const auto getDMLActivationELU = [](string& dmlTypedOpDesc, const void* pTypedOp)
	{
		dmlTypedOpDesc.resize(sizeof(DML_ACTIVATION_ELU_OPERATOR_DESC));
		auto& dmlDesc = reinterpret_cast<DML_ACTIVATION_ELU_OPERATOR_DESC&>(dmlTypedOpDesc[0]);
		const auto& typedOp = *static_cast<const ActivationELU*>(pTypedOp);

		dmlDesc.InputTensor = typedOp.pInput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pInput->GetHandle()) : nullptr;
		dmlDesc.OutputTensor = typedOp.pOutput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pOutput->GetHandle()) : nullptr;
		dmlDesc.Alpha = typedOp.Alpha;
	};

	static const auto getDMLActivationLinear = [](string& dmlTypedOpDesc, const void* pTypedOp)
	{
		dmlTypedOpDesc.resize(sizeof(DML_ACTIVATION_LINEAR_OPERATOR_DESC));
		auto& dmlDesc = reinterpret_cast<DML_ACTIVATION_LINEAR_OPERATOR_DESC&>(dmlTypedOpDesc[0]);
		const auto& typedOp = *static_cast<const ActivationLinear*>(pTypedOp);

		dmlDesc.InputTensor = typedOp.pInput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pInput->GetHandle()) : nullptr;
		dmlDesc.OutputTensor = typedOp.pOutput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pOutput->GetHandle()) : nullptr;
		dmlDesc.Alpha = typedOp.Alpha;
		dmlDesc.Beta = typedOp.Beta;
	};

	static const auto getDMLActivationParameterizedRELU = [](string& dmlTypedOpDesc, const void* pTypedOp)
	{
		dmlTypedOpDesc.resize(sizeof(DML_ACTIVATION_PARAMETERIZED_RELU_OPERATOR_DESC));
		auto& dmlDesc = reinterpret_cast<DML_ACTIVATION_PARAMETERIZED_RELU_OPERATOR_DESC&>(dmlTypedOpDesc[0]);
		const auto& typedOp = *static_cast<const ActivationParameterizedRELU*>(pTypedOp);

		dmlDesc.InputTensor = typedOp.pInput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pInput->GetHandle()) : nullptr;
		dmlDesc.SlopeTensor = typedOp.pSlope ? static_cast<const DML_TENSOR_DESC*>(typedOp.pSlope->GetHandle()) : nullptr;
		dmlDesc.OutputTensor = typedOp.pOutput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pOutput->GetHandle()) : nullptr;
	};

	static const auto getDMLActivationScaledELU = [](string& dmlTypedOpDesc, const void* pTypedOp)
	{
		dmlTypedOpDesc.resize(sizeof(DML_ACTIVATION_SCALED_ELU_OPERATOR_DESC));
		auto& dmlDesc = reinterpret_cast<DML_ACTIVATION_SCALED_ELU_OPERATOR_DESC&>(dmlTypedOpDesc[0]);
		const auto& typedOp = *static_cast<const ActivationScaledELU*>(pTypedOp);

		dmlDesc.InputTensor = typedOp.pInput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pInput->GetHandle()) : nullptr;
		dmlDesc.OutputTensor = typedOp.pOutput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pOutput->GetHandle()) : nullptr;
		dmlDesc.Alpha = typedOp.Alpha;
		dmlDesc.Gamma = typedOp.Gamma;
	};

	static const auto getDMLActivationSoftplus = [](string& dmlTypedOpDesc, const void* pTypedOp)
	{
		dmlTypedOpDesc.resize(sizeof(DML_ACTIVATION_SOFTPLUS_OPERATOR_DESC));
		auto& dmlDesc = reinterpret_cast<DML_ACTIVATION_SOFTPLUS_OPERATOR_DESC&>(dmlTypedOpDesc[0]);
		const auto& typedOp = *static_cast<const ActivationSoftplus*>(pTypedOp);

		dmlDesc.InputTensor = typedOp.pInput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pInput->GetHandle()) : nullptr;
		dmlDesc.OutputTensor = typedOp.pOutput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pOutput->GetHandle()) : nullptr;
		dmlDesc.Steepness = typedOp.Steepness;
	};

	static const auto getDMLConvolution = [](string& dmlTypedOpDesc, const void* pTypedOp)
	{
		const auto& typedOp = *static_cast<const ConvolutionOperator*>(pTypedOp);

		string dmlFusedActivation;
		if (typedOp.pFusedActivation) GetDMLTypedOperator(dmlFusedActivation, typedOp.pFusedActivation);

		dmlTypedOpDesc.resize(sizeof(DML_CONVOLUTION_OPERATOR_DESC) +
			(typedOp.pFusedActivation ? sizeof(DML_OPERATOR_DESC) + dmlFusedActivation.size() : 0));
		auto& dmlDesc = reinterpret_cast<DML_CONVOLUTION_OPERATOR_DESC&>(dmlTypedOpDesc[0]);
		const auto pDMLFusedActivation = typedOp.pFusedActivation ? reinterpret_cast<DML_OPERATOR_DESC*>(
			&dmlTypedOpDesc[sizeof(DML_CONVOLUTION_OPERATOR_DESC)]) : nullptr;
		
		dmlDesc.InputTensor = typedOp.pInput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pInput->GetHandle()) : nullptr;
		dmlDesc.FilterTensor = typedOp.pFilter ? static_cast<const DML_TENSOR_DESC*>(typedOp.pFilter->GetHandle()) : nullptr;
		dmlDesc.BiasTensor = typedOp.pBias ? static_cast<const DML_TENSOR_DESC*>(typedOp.pBias->GetHandle()) : nullptr;
		dmlDesc.OutputTensor = typedOp.pOutput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pOutput->GetHandle()) : nullptr;
		dmlDesc.Mode = GetDMLConvolutionMode(typedOp.Mode);
		dmlDesc.Direction = GetDMLConvolutionDirection(typedOp.Direction);
		dmlDesc.DimensionCount = typedOp.DimensionCount;
		dmlDesc.Strides = typedOp.pStrides;
		dmlDesc.Dilations = typedOp.pDilations;
		dmlDesc.StartPadding = typedOp.pStartPadding;
		dmlDesc.EndPadding = typedOp.pEndPadding;
		dmlDesc.OutputPadding = typedOp.pOutputPadding;
		dmlDesc.GroupCount = typedOp.GroupCount;
		dmlDesc.FusedActivation = pDMLFusedActivation;

		if (pDMLFusedActivation)
		{
			assert(typedOp.pFusedActivation);
			const auto offset = sizeof(DML_CONVOLUTION_OPERATOR_DESC) + sizeof(DML_OPERATOR_DESC);
			pDMLFusedActivation->Type = GetDMLOpteratorType(*static_cast<const OperatorType*>(typedOp.pFusedActivation));
			pDMLFusedActivation->Desc = &dmlTypedOpDesc[offset];
			memcpy(&dmlTypedOpDesc[offset], dmlFusedActivation.data(), dmlFusedActivation.size());
		}
	};

	static const auto getDMLGEMM = [](string& dmlTypedOpDesc, const void* pTypedOp)
	{
		const auto& typedOp = *static_cast<const GEMMOperator*>(pTypedOp);

		string dmlFusedActivation;
		if (typedOp.pFusedActivation) GetDMLTypedOperator(dmlFusedActivation, typedOp.pFusedActivation);

		dmlTypedOpDesc.resize(sizeof(DML_GEMM_OPERATOR_DESC) +
			(typedOp.pFusedActivation ? sizeof(DML_OPERATOR_DESC) + dmlFusedActivation.size() : 0));
		auto& dmlDesc = reinterpret_cast<DML_GEMM_OPERATOR_DESC&>(dmlTypedOpDesc[0]);
		const auto pDMLFusedActivation = typedOp.pFusedActivation ? reinterpret_cast<DML_OPERATOR_DESC*>(
			&dmlTypedOpDesc[sizeof(DML_GEMM_OPERATOR_DESC)]) : nullptr;

		dmlDesc.ATensor = typedOp.pA ? static_cast<const DML_TENSOR_DESC*>(typedOp.pA->GetHandle()) : nullptr;
		dmlDesc.BTensor = typedOp.pB ? static_cast<const DML_TENSOR_DESC*>(typedOp.pB->GetHandle()) : nullptr;
		dmlDesc.CTensor = typedOp.pC ? static_cast<const DML_TENSOR_DESC*>(typedOp.pC->GetHandle()) : nullptr;
		dmlDesc.OutputTensor = typedOp.pOutput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pOutput->GetHandle()) : nullptr;
		dmlDesc.TransA = GetDMLMatrixTransform(typedOp.TransA);
		dmlDesc.TransB = GetDMLMatrixTransform(typedOp.TransB);
		dmlDesc.Alpha = typedOp.Alpha;
		dmlDesc.Beta = typedOp.Beta;
		dmlDesc.FusedActivation = pDMLFusedActivation;

		if (pDMLFusedActivation)
		{
			assert(typedOp.pFusedActivation);
			const auto offset = sizeof(DML_GEMM_OPERATOR_DESC) + sizeof(DML_OPERATOR_DESC);
			pDMLFusedActivation->Type = GetDMLOpteratorType(*static_cast<const OperatorType*>(typedOp.pFusedActivation));
			pDMLFusedActivation->Desc = &dmlTypedOpDesc[offset];
			memcpy(&dmlTypedOpDesc[offset], dmlFusedActivation.data(), dmlFusedActivation.size());
		}
	};

	static const auto getDMLReduce = [](string& dmlTypedOpDesc, const void* pTypedOp)
	{
		dmlTypedOpDesc.resize(sizeof(DML_REDUCE_OPERATOR_DESC));
		auto& dmlDesc = reinterpret_cast<DML_REDUCE_OPERATOR_DESC&>(dmlTypedOpDesc[0]);
		const auto& typedOp = *static_cast<const ReduceOperator*>(pTypedOp);

		dmlDesc.Function = GetDMLReduceFunction(typedOp.Function);
		dmlDesc.InputTensor = typedOp.pInput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pInput->GetHandle()) : nullptr;
		dmlDesc.OutputTensor = typedOp.pOutput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pOutput->GetHandle()) : nullptr;
		dmlDesc.AxisCount = typedOp.AxisCount;
		dmlDesc.Axes = typedOp.pAxes;
	};

	static const auto getDMLAveragePooling = [](string& dmlTypedOpDesc, const void* pTypedOp)
	{
		dmlTypedOpDesc.resize(sizeof(DML_AVERAGE_POOLING_OPERATOR_DESC));
		auto& dmlDesc = reinterpret_cast<DML_AVERAGE_POOLING_OPERATOR_DESC&>(dmlTypedOpDesc[0]);
		const auto& typedOp = *static_cast<const AveragePooling*>(pTypedOp);

		dmlDesc.InputTensor = typedOp.pInput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pInput->GetHandle()) : nullptr;
		dmlDesc.OutputTensor = typedOp.pOutput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pOutput->GetHandle()) : nullptr;
		dmlDesc.DimensionCount = typedOp.DimensionCount;
		dmlDesc.Strides = typedOp.pStrides;
		dmlDesc.WindowSize = typedOp.pWindowSize;
		dmlDesc.StartPadding = typedOp.pStartPadding;
		dmlDesc.EndPadding = typedOp.pEndPadding;
		dmlDesc.IncludePadding = typedOp.IncludePadding;
	};

	static const auto getDMLLPPooling = [](string& dmlTypedOpDesc, const void* pTypedOp)
	{
		dmlTypedOpDesc.resize(sizeof(DML_LP_POOLING_OPERATOR_DESC));
		auto& dmlDesc = reinterpret_cast<DML_LP_POOLING_OPERATOR_DESC&>(dmlTypedOpDesc[0]);
		const auto& typedOp = *static_cast<const LPPooling*>(pTypedOp);

		dmlDesc.InputTensor = typedOp.pInput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pInput->GetHandle()) : nullptr;
		dmlDesc.OutputTensor = typedOp.pOutput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pOutput->GetHandle()) : nullptr;
		dmlDesc.DimensionCount = typedOp.DimensionCount;
		dmlDesc.Strides = typedOp.pStrides;
		dmlDesc.WindowSize = typedOp.pWindowSize;
		dmlDesc.StartPadding = typedOp.pStartPadding;
		dmlDesc.EndPadding = typedOp.pEndPadding;
		dmlDesc.P = typedOp.P;
	};

	static const auto getDMLMaxPooling = [](string& dmlTypedOpDesc, const void* pTypedOp)
	{
		dmlTypedOpDesc.resize(sizeof(DML_MAX_POOLING_OPERATOR_DESC));
		auto& dmlDesc = reinterpret_cast<DML_MAX_POOLING_OPERATOR_DESC&>(dmlTypedOpDesc[0]);
		const auto& typedOp = *static_cast<const MaxPooling*>(pTypedOp);

		dmlDesc.InputTensor = typedOp.pInput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pInput->GetHandle()) : nullptr;
		dmlDesc.OutputTensor = typedOp.pOutput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pOutput->GetHandle()) : nullptr;
		dmlDesc.DimensionCount = typedOp.DimensionCount;
		dmlDesc.Strides = typedOp.pStrides;
		dmlDesc.WindowSize = typedOp.pWindowSize;
		dmlDesc.StartPadding = typedOp.pStartPadding;
		dmlDesc.EndPadding = typedOp.pEndPadding;
	};

	static const auto getDMLROIPooling = [](string& dmlTypedOpDesc, const void* pTypedOp)
	{
		dmlTypedOpDesc.resize(sizeof(DML_ROI_POOLING_OPERATOR_DESC));
		auto& dmlDesc = reinterpret_cast<DML_ROI_POOLING_OPERATOR_DESC&>(dmlTypedOpDesc[0]);
		const auto& typedOp = *static_cast<const ROIPooling*>(pTypedOp);

		dmlDesc.InputTensor = typedOp.pInput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pInput->GetHandle()) : nullptr;
		dmlDesc.ROITensor = typedOp.pROI ? static_cast<const DML_TENSOR_DESC*>(typedOp.pROI->GetHandle()) : nullptr;
		dmlDesc.OutputTensor = typedOp.pOutput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pOutput->GetHandle()) : nullptr;
		dmlDesc.SpatialScale = typedOp.SpatialScale;
		dmlDesc.PooledSize.Width = typedOp.PooledWidth;
		dmlDesc.PooledSize.Height = typedOp.PooledHeight;
	};

	static const auto getDMLSlice = [](string& dmlTypedOpDesc, const void* pTypedOp)
	{
		dmlTypedOpDesc.resize(sizeof(DML_SLICE_OPERATOR_DESC));
		auto& dmlDesc = reinterpret_cast<DML_SLICE_OPERATOR_DESC&>(dmlTypedOpDesc[0]);
		const auto& typedOp = *static_cast<const SliceOperator*>(pTypedOp);

		dmlDesc.InputTensor = typedOp.pInput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pInput->GetHandle()) : nullptr;
		dmlDesc.OutputTensor = typedOp.pOutput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pOutput->GetHandle()) : nullptr;
		dmlDesc.DimensionCount = typedOp.DimensionCount;
		dmlDesc.Offsets = typedOp.pOffsets;
		dmlDesc.Sizes = typedOp.pSizes;
		dmlDesc.Strides = typedOp.pStrides;
	};

	static const auto getDMLSplit = [](string& dmlTypedOpDesc, const void* pTypedOp)
	{
		dmlTypedOpDesc.resize(sizeof(DML_SPLIT_OPERATOR_DESC));
		auto& dmlDesc = reinterpret_cast<DML_SPLIT_OPERATOR_DESC&>(dmlTypedOpDesc[0]);
		const auto& typedOp = *static_cast<const SplitOperator*>(pTypedOp);

		dmlDesc.InputTensor = typedOp.pInput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pInput->GetHandle()) : nullptr;
		dmlDesc.OutputCount = typedOp.OutputCount;
		dmlDesc.OutputTensors = typedOp.pOutputs ? static_cast<const DML_TENSOR_DESC*>(typedOp.pOutputs->GetHandle()) : nullptr;
		dmlDesc.Axis = typedOp.Axis;
	};

	static const auto getDMLJoin = [](string& dmlTypedOpDesc, const void* pTypedOp)
	{
		dmlTypedOpDesc.resize(sizeof(DML_JOIN_OPERATOR_DESC));
		auto& dmlDesc = reinterpret_cast<DML_JOIN_OPERATOR_DESC&>(dmlTypedOpDesc[0]);
		const auto& typedOp = *static_cast<const JoinOperator*>(pTypedOp);

		dmlDesc.InputCount = typedOp.InputCount;
		dmlDesc.InputTensors = typedOp.pInputs ? static_cast<const DML_TENSOR_DESC*>(typedOp.pInputs->GetHandle()) : nullptr;
		dmlDesc.OutputTensor = typedOp.pOutput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pOutput->GetHandle()) : nullptr;
		dmlDesc.Axis = typedOp.Axis;
	};

	static const auto getDMLPadding = [](string& dmlTypedOpDesc, const void* pTypedOp)
	{
		dmlTypedOpDesc.resize(sizeof(DML_PADDING_OPERATOR_DESC));
		auto& dmlDesc = reinterpret_cast<DML_PADDING_OPERATOR_DESC&>(dmlTypedOpDesc[0]);
		const auto& typedOp = *static_cast<const PaddingOperator*>(pTypedOp);

		dmlDesc.InputTensor = typedOp.pInput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pInput->GetHandle()) : nullptr;
		dmlDesc.OutputTensor = typedOp.pOutput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pOutput->GetHandle()) : nullptr;
		dmlDesc.PaddingMode = GetDMLPaddingMode(typedOp.PaddingMode);
		dmlDesc.PaddingValue = typedOp.PaddingValue;
		dmlDesc.DimensionCount = typedOp.DimensionCount;
		dmlDesc.StartPadding = typedOp.pStartPadding;
		dmlDesc.EndPadding = typedOp.pEndPadding;
	};

	static const auto getDMLValueScale2D = [](string& dmlTypedOpDesc, const void* pTypedOp)
	{
		dmlTypedOpDesc.resize(sizeof(DML_VALUE_SCALE_2D_OPERATOR_DESC));
		auto& dmlDesc = reinterpret_cast<DML_VALUE_SCALE_2D_OPERATOR_DESC&>(dmlTypedOpDesc[0]);
		const auto& typedOp = *static_cast<const ValueScale2D*>(pTypedOp);

		dmlDesc.InputTensor = typedOp.pInput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pInput->GetHandle()) : nullptr;
		dmlDesc.OutputTensor = typedOp.pOutput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pOutput->GetHandle()) : nullptr;
		dmlDesc.Scale = typedOp.Scale;
		dmlDesc.ChannelCount = typedOp.ChannelCount;
		dmlDesc.Bias = typedOp.pBias;
	};

	static const auto getDMLUpsample2D = [](string& dmlTypedOpDesc, const void* pTypedOp)
	{
		dmlTypedOpDesc.resize(sizeof(DML_UPSAMPLE_2D_OPERATOR_DESC));
		auto& dmlDesc = reinterpret_cast<DML_UPSAMPLE_2D_OPERATOR_DESC&>(dmlTypedOpDesc[0]);
		const auto& typedOp = *static_cast<const Upsample2D*>(pTypedOp);

		dmlDesc.InputTensor = typedOp.pInput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pInput->GetHandle()) : nullptr;
		dmlDesc.OutputTensor = typedOp.pOutput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pOutput->GetHandle()) : nullptr;
		dmlDesc.ScaleSize.Width = typedOp.ScaleWidth;
		dmlDesc.ScaleSize.Height = typedOp.ScaleHeight;
		dmlDesc.InterpolationMode = GetDMLInterpolationMode(typedOp.InterpolationMode);
	};

	static const auto getDMLGather = [](string& dmlTypedOpDesc, const void* pTypedOp)
	{
		dmlTypedOpDesc.resize(sizeof(DML_GATHER_OPERATOR_DESC));
		auto& dmlDesc = reinterpret_cast<DML_GATHER_OPERATOR_DESC&>(dmlTypedOpDesc[0]);
		const auto& typedOp = *static_cast<const GatherOperator*>(pTypedOp);

		dmlDesc.InputTensor = typedOp.pInput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pInput->GetHandle()) : nullptr;
		dmlDesc.IndicesTensor = typedOp.pIndices ? static_cast<const DML_TENSOR_DESC*>(typedOp.pIndices->GetHandle()) : nullptr;
		dmlDesc.OutputTensor = typedOp.pOutput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pOutput->GetHandle()) : nullptr;
		dmlDesc.Axis = typedOp.Axis;
		dmlDesc.IndexDimensions = typedOp.IndexDimensions;
	};

	static const auto getDMLSpaceDepth = [](string& dmlTypedOpDesc, const void* pTypedOp)
	{
		dmlTypedOpDesc.resize(sizeof(DML_SPACE_TO_DEPTH_OPERATOR_DESC));
		auto& dmlDesc = reinterpret_cast<DML_SPACE_TO_DEPTH_OPERATOR_DESC&>(dmlTypedOpDesc[0]);
		const auto& typedOp = *static_cast<const SpaceToDepth*>(pTypedOp);

		dmlDesc.InputTensor = typedOp.pInput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pInput->GetHandle()) : nullptr;
		dmlDesc.OutputTensor = typedOp.pOutput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pOutput->GetHandle()) : nullptr;
		dmlDesc.BlockSize = typedOp.BlockSize;
	};

	static const auto getDMLTile = [](string& dmlTypedOpDesc, const void* pTypedOp)
	{
		dmlTypedOpDesc.resize(sizeof(DML_TILE_OPERATOR_DESC));
		auto& dmlDesc = reinterpret_cast<DML_TILE_OPERATOR_DESC&>(dmlTypedOpDesc[0]);
		const auto& typedOp = *static_cast<const TileOperator*>(pTypedOp);

		dmlDesc.InputTensor = typedOp.pInput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pInput->GetHandle()) : nullptr;
		dmlDesc.OutputTensor = typedOp.pOutput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pOutput->GetHandle()) : nullptr;
		dmlDesc.RepeatsCount = typedOp.RepeatsCount;
		dmlDesc.Repeats = typedOp.pRepeats;
	};

	static const auto getDMLTopK = [](string& dmlTypedOpDesc, const void* pTypedOp)
	{
		dmlTypedOpDesc.resize(sizeof(DML_TOP_K_OPERATOR_DESC));
		auto& dmlDesc = reinterpret_cast<DML_TOP_K_OPERATOR_DESC&>(dmlTypedOpDesc[0]);
		const auto& typedOp = *static_cast<const TopKOperator*>(pTypedOp);

		dmlDesc.InputTensor = typedOp.pInput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pInput->GetHandle()) : nullptr;
		dmlDesc.OutputValueTensor = typedOp.pOutputValue ? static_cast<const DML_TENSOR_DESC*>(typedOp.pOutputValue->GetHandle()) : nullptr;
		dmlDesc.OutputIndexTensor = typedOp.pOutputIndex ? static_cast<const DML_TENSOR_DESC*>(typedOp.pOutputIndex->GetHandle()) : nullptr;
		dmlDesc.Axis = typedOp.Axis;
		dmlDesc.K = typedOp.K;
	};

	static const auto getDMLBatchNormalization = [](string& dmlTypedOpDesc, const void* pTypedOp)
	{
		const auto& typedOp = *static_cast<const BatchNormalization*>(pTypedOp);

		string dmlFusedActivation;
		if (typedOp.pFusedActivation) GetDMLTypedOperator(dmlFusedActivation, typedOp.pFusedActivation);

		dmlTypedOpDesc.resize(sizeof(DML_BATCH_NORMALIZATION_OPERATOR_DESC) +
			(typedOp.pFusedActivation ? sizeof(DML_OPERATOR_DESC) + dmlFusedActivation.size() : 0));
		auto& dmlDesc = reinterpret_cast<DML_BATCH_NORMALIZATION_OPERATOR_DESC&>(dmlTypedOpDesc[0]);
		const auto pDMLFusedActivation = typedOp.pFusedActivation ? reinterpret_cast<DML_OPERATOR_DESC*>(
			&dmlTypedOpDesc[sizeof(DML_BATCH_NORMALIZATION_OPERATOR_DESC)]) : nullptr;

		dmlDesc.InputTensor = typedOp.pInput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pInput->GetHandle()) : nullptr;
		dmlDesc.MeanTensor = typedOp.pMean ? static_cast<const DML_TENSOR_DESC*>(typedOp.pMean->GetHandle()) : nullptr;
		dmlDesc.VarianceTensor = typedOp.pVariance ? static_cast<const DML_TENSOR_DESC*>(typedOp.pVariance->GetHandle()) : nullptr;
		dmlDesc.ScaleTensor = typedOp.pScale ? static_cast<const DML_TENSOR_DESC*>(typedOp.pScale->GetHandle()) : nullptr;
		dmlDesc.BiasTensor = typedOp.pBias ? static_cast<const DML_TENSOR_DESC*>(typedOp.pBias->GetHandle()) : nullptr;
		dmlDesc.OutputTensor = typedOp.pOutput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pOutput->GetHandle()) : nullptr;
		dmlDesc.Spatial = typedOp.Spatial;
		dmlDesc.Epsilon = typedOp.Epsilon;
		dmlDesc.FusedActivation = pDMLFusedActivation;

		if (pDMLFusedActivation)
		{
			assert(typedOp.pFusedActivation);
			const auto offset = sizeof(DML_CONVOLUTION_OPERATOR_DESC) + sizeof(DML_OPERATOR_DESC);
			pDMLFusedActivation->Type = GetDMLOpteratorType(*static_cast<const OperatorType*>(typedOp.pFusedActivation));
			pDMLFusedActivation->Desc = &dmlTypedOpDesc[offset];
			memcpy(&dmlTypedOpDesc[offset], dmlFusedActivation.data(), dmlFusedActivation.size());
		}
	};

	static const auto getDMLMeanVarianceNormalization = [](string& dmlTypedOpDesc, const void* pTypedOp)
	{
		const auto& typedOp = *static_cast<const MeanVarianceNormalization*>(pTypedOp);

		string dmlFusedActivation;
		if (typedOp.pFusedActivation) GetDMLTypedOperator(dmlFusedActivation, typedOp.pFusedActivation);

		dmlTypedOpDesc.resize(sizeof(DML_MEAN_VARIANCE_NORMALIZATION_OPERATOR_DESC) +
			(typedOp.pFusedActivation ? sizeof(DML_OPERATOR_DESC) + dmlFusedActivation.size() : 0));
		auto& dmlDesc = reinterpret_cast<DML_MEAN_VARIANCE_NORMALIZATION_OPERATOR_DESC&>(dmlTypedOpDesc[0]);
		const auto pDMLFusedActivation = typedOp.pFusedActivation ? reinterpret_cast<DML_OPERATOR_DESC*>(
			&dmlTypedOpDesc[sizeof(DML_MEAN_VARIANCE_NORMALIZATION_OPERATOR_DESC)]) : nullptr;

		dmlDesc.InputTensor = typedOp.pInput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pInput->GetHandle()) : nullptr;
		dmlDesc.ScaleTensor = typedOp.pScale ? static_cast<const DML_TENSOR_DESC*>(typedOp.pScale->GetHandle()) : nullptr;
		dmlDesc.BiasTensor = typedOp.pBias ? static_cast<const DML_TENSOR_DESC*>(typedOp.pBias->GetHandle()) : nullptr;
		dmlDesc.OutputTensor = typedOp.pOutput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pOutput->GetHandle()) : nullptr;
		dmlDesc.CrossChannel = typedOp.CrossChannel;
		dmlDesc.NormalizeVariance = typedOp.NormalizeVariance;
		dmlDesc.Epsilon = typedOp.Epsilon;
		dmlDesc.FusedActivation = pDMLFusedActivation;

		if (pDMLFusedActivation)
		{
			assert(typedOp.pFusedActivation);
			const auto offset = sizeof(DML_CONVOLUTION_OPERATOR_DESC) + sizeof(DML_OPERATOR_DESC);
			pDMLFusedActivation->Type = GetDMLOpteratorType(*static_cast<const OperatorType*>(typedOp.pFusedActivation));
			pDMLFusedActivation->Desc = &dmlTypedOpDesc[offset];
			memcpy(&dmlTypedOpDesc[offset], dmlFusedActivation.data(), dmlFusedActivation.size());
		}
	};

	static const auto getDMLLocalResponseNormalization = [](string& dmlTypedOpDesc, const void* pTypedOp)
	{
		dmlTypedOpDesc.resize(sizeof(DML_LOCAL_RESPONSE_NORMALIZATION_OPERATOR_DESC));
		auto& dmlDesc = reinterpret_cast<DML_LOCAL_RESPONSE_NORMALIZATION_OPERATOR_DESC&>(dmlTypedOpDesc[0]);
		const auto& typedOp = *static_cast<const LocalResponseNormalization*>(pTypedOp);

		dmlDesc.InputTensor = typedOp.pInput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pInput->GetHandle()) : nullptr;
		dmlDesc.OutputTensor = typedOp.pOutput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pOutput->GetHandle()) : nullptr;
		dmlDesc.CrossChannel = typedOp.CrossChannel;
		dmlDesc.LocalSize = typedOp.LocalSize;
		dmlDesc.Alpha = typedOp.Alpha;
		dmlDesc.Beta = typedOp.Beta;
		dmlDesc.Bias = typedOp.Bias;
	};

	static const auto getDMLLPNormalization = [](string& dmlTypedOpDesc, const void* pTypedOp)
	{
		dmlTypedOpDesc.resize(sizeof(DML_LP_NORMALIZATION_OPERATOR_DESC));
		auto& dmlDesc = reinterpret_cast<DML_LP_NORMALIZATION_OPERATOR_DESC&>(dmlTypedOpDesc[0]);
		const auto& typedOp = *static_cast<const LPNormalization*>(pTypedOp);

		dmlDesc.InputTensor = typedOp.pInput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pInput->GetHandle()) : nullptr;
		dmlDesc.OutputTensor = typedOp.pOutput ? static_cast<const DML_TENSOR_DESC*>(typedOp.pOutput->GetHandle()) : nullptr;
		dmlDesc.Axis = typedOp.Axis;
		dmlDesc.Epsilon = typedOp.Epsilon;
		dmlDesc.P = typedOp.P;
	};

	static const function<void(string&, const void*)> pfnGetDMLOps[] =
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
		getDMLGather,						// GATHER
		getDMLSpaceDepth,					// SPACE_TO_DEPTH
		getDMLSpaceDepth,					// DEPTH_TO_SPACE
		getDMLTile,							// TILE
		getDMLTopK,							// TOP_K

		getDMLBatchNormalization,			// BATCH_NORMALIZATION
		getDMLMeanVarianceNormalization,	// MEAN_VARIANCE_NORMALIZATION
		getDMLLocalResponseNormalization,	// LOCAL_RESPONSE_NORMALIZATION
		getDMLLPNormalization,				// LP_NORMALIZATION
	};

	pfnGetDMLOps[*static_cast<const uint32_t*>(pTypedOp)](dmlTypedOpDesc, pTypedOp);
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
