//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Core/XUSG.h"

namespace XUSG
{
	namespace ML
	{
		DML_TENSOR_DATA_TYPE GetDMLTensorDataType(TensorDataType dataType);
		DML_OPERATOR_TYPE GetDMLOpteratorType(OperatorType dataType);

		DML_TENSOR_FLAGS GetDMLTensorFlag(TensorFlag tensorFlag);
		DML_TENSOR_FLAGS GetDMLTensorFlags(TensorFlag tensorFlags);

		DML_EXECUTION_FLAGS GetDMLExecutionFlag(ExecutionFlag executionFlag);
		DML_EXECUTION_FLAGS GetDMLExecutionFlags(ExecutionFlag executionFlags);

		DML_CONVOLUTION_MODE GetDMLConvolutionMode(ConvolutionMode convolutionMode);
		DML_CONVOLUTION_DIRECTION GetDMLConvolutionDirection(ConvolutionDirection convolutionDirection);
		DML_INTERPOLATION_MODE GetDMLInterpolationMode(InterpolationType interpolationMode);

		// Device
		interface DML_DECLARE_INTERFACE("6dbd6437-96fd-423f-a98c-ae5e7c2a573f")
			DLL_INTERFACE DMLDevice : public IDMLDevice
		{
			bool GetCommandRecorder(CommandRecorder* pCommandRecorder);
			bool GetCommandRecorder(CommandRecorder& commandRecorder);
		};
		using Device = com_ptr<DMLDevice>;
		using Dispatchable = com_ptr<IDMLDispatchable>;
		using BindingTable = com_ptr<IDMLBindingTable>;

		using BufferTensorDesc = DML_BUFFER_TENSOR_DESC;
		using TensorDesc = DML_TENSOR_DESC;
		using BufferBinding = DML_BUFFER_BINDING;
		using ArrayBinding = DML_BUFFER_ARRAY_BINDING;
		using BindingDesc = DML_BINDING_DESC;

		struct OperatorDesc : public DML_OPERATOR_DESC
		{
			OperatorDesc(OperatorType type, const void* pDesc)
			{
				Type = GetDMLOpteratorType(type);
				Desc = pDesc;
			}
		};

		using ElementWiseIdentity = DML_ELEMENT_WISE_IDENTITY_OPERATOR_DESC;
		using ElementWiseAbs = DML_ELEMENT_WISE_ABS_OPERATOR_DESC;
		using ElementWiseAcos = DML_ELEMENT_WISE_ACOS_OPERATOR_DESC;
		using ElementWiseAdd = DML_ELEMENT_WISE_ADD_OPERATOR_DESC;
		using ElementWiseAsin = DML_ELEMENT_WISE_ASIN_OPERATOR_DESC;
		using ElementWiseAtan = DML_ELEMENT_WISE_ATAN_OPERATOR_DESC;
		using ElementWiseCeil = DML_ELEMENT_WISE_CEIL_OPERATOR_DESC;
		using ElementWiseClip = DML_ELEMENT_WISE_CLIP_OPERATOR_DESC;
		using ElementWiseCos = DML_ELEMENT_WISE_COS_OPERATOR_DESC;
		using ElementWiseDivide = DML_ELEMENT_WISE_DIVIDE_OPERATOR_DESC;
		using ElementWiseExp = DML_ELEMENT_WISE_EXP_OPERATOR_DESC;
		using ElementWiseFloor = DML_ELEMENT_WISE_FLOOR_OPERATOR_DESC;
		using ElementWiseLog = DML_ELEMENT_WISE_LOG_OPERATOR_DESC;
		using ElementWiseLogicalAnd = DML_ELEMENT_WISE_LOGICAL_AND_OPERATOR_DESC;
		using ElementWiseLogicalEquals = DML_ELEMENT_WISE_LOGICAL_EQUALS_OPERATOR_DESC;
		using ElementWiseLogicalGreater = DML_ELEMENT_WISE_LOGICAL_GREATER_THAN_OPERATOR_DESC;
		using ElementWiseLogicalLessThan = DML_ELEMENT_WISE_LOGICAL_LESS_THAN_OPERATOR_DESC;
		using ElementWiseLogicalNot = DML_ELEMENT_WISE_LOGICAL_NOT_OPERATOR_DESC;
		using ElementWiseLogicalOr = DML_ELEMENT_WISE_LOGICAL_OR_OPERATOR_DESC;
		using ElementWiseLogicalXor = DML_ELEMENT_WISE_LOGICAL_XOR_OPERATOR_DESC;
		using ElementWiseMax = DML_ELEMENT_WISE_MAX_OPERATOR_DESC;
		using ElementWiseMean = DML_ELEMENT_WISE_MEAN_OPERATOR_DESC;
		using ElementWiseMin = DML_ELEMENT_WISE_MIN_OPERATOR_DESC;
		using ElementWiseMultiply = DML_ELEMENT_WISE_MULTIPLY_OPERATOR_DESC;
		using ElementWisePow = DML_ELEMENT_WISE_POW_OPERATOR_DESC;
		using ElementWiseConstantPow = DML_ELEMENT_WISE_CONSTANT_POW_OPERATOR_DESC;
		using ElementWiseRecip = DML_ELEMENT_WISE_RECIP_OPERATOR_DESC;
		using ElementWiseSin = DML_ELEMENT_WISE_SIN_OPERATOR_DESC;
		using ElementWiseSqrt = DML_ELEMENT_WISE_SQRT_OPERATOR_DESC;
		using ElementWiseSubtract = DML_ELEMENT_WISE_SUBTRACT_OPERATOR_DESC;
		using ElementWiseTan = DML_ELEMENT_WISE_TAN_OPERATOR_DESC;
		using ElementWiseThreshold = DML_ELEMENT_WISE_THRESHOLD_OPERATOR_DESC;
		using ElementWiseQuantizeLinear = DML_ELEMENT_WISE_QUANTIZE_LINEAR_OPERATOR_DESC;
		using ElementWiseDequantizeLinear = DML_ELEMENT_WISE_DEQUANTIZE_LINEAR_OPERATOR_DESC;

		using ActivationELU = DML_ACTIVATION_ELU_OPERATOR_DESC;
		using ActivationHardMax = DML_ACTIVATION_HARDMAX_OPERATOR_DESC;
		using ActivationHardSigmoid = DML_ACTIVATION_HARD_SIGMOID_OPERATOR_DESC;
		using ActivationIdentity = DML_ACTIVATION_IDENTITY_OPERATOR_DESC;
		using ActivationLeakyRELU = DML_ACTIVATION_LEAKY_RELU_OPERATOR_DESC;
		using ActivationLinear = DML_ACTIVATION_LINEAR_OPERATOR_DESC;
		using ActivationLogSoftmax = DML_ACTIVATION_LOG_SOFTMAX_OPERATOR_DESC;
		using ActivationParameterizedRELU = DML_ACTIVATION_PARAMETERIZED_RELU_OPERATOR_DESC;
		using ActivationParametricSoftplus = DML_ACTIVATION_PARAMETRIC_SOFTPLUS_OPERATOR_DESC;
		using ActivationRELU = DML_ACTIVATION_RELU_OPERATOR_DESC;
		using ActivationScaledELU = DML_ACTIVATION_SCALED_ELU_OPERATOR_DESC;
		using ActivationScaledTanh = DML_ACTIVATION_SCALED_TANH_OPERATOR_DESC;
		using ActivationSigmoid = DML_ACTIVATION_SIGMOID_OPERATOR_DESC;
		using ActivationSoftmax = DML_ACTIVATION_SOFTMAX_OPERATOR_DESC;
		using ActivationSoftplus = DML_ACTIVATION_SOFTPLUS_OPERATOR_DESC;
		using ActivationSoftsign = DML_ACTIVATION_SOFTSIGN_OPERATOR_DESC;
		using ActivationTanh = DML_ACTIVATION_TANH_OPERATOR_DESC;
		using ActivationThresholdRELU = DML_ACTIVATION_THRESHOLDED_RELU_OPERATOR_DESC;

		struct ConvolutionOperator : public DML_CONVOLUTION_OPERATOR_DESC
		{
			ConvolutionOperator(const TensorDesc* pInputTensor, const TensorDesc* pFilterTensor,
				const TensorDesc* pBiasTensor, const TensorDesc* pOutputTensor,
				ConvolutionMode mode, ConvolutionDirection direction, uint32_t dimensionCount,
				const uint32_t* pStrides, const uint32_t* pDilations, const uint32_t* pStartPadding,
				const uint32_t* pEndPadding, const uint32_t* pOutputPadding, uint32_t groupCount,
				const OperatorDesc* pFusedActivation)
			{
				InputTensor = pInputTensor;
				FilterTensor = pFilterTensor;
				BiasTensor = pBiasTensor;
				OutputTensor = pOutputTensor;
				Mode = GetDMLConvolutionMode(mode);
				Direction = GetDMLConvolutionDirection(direction);
				DimensionCount = dimensionCount;
				Strides = pStrides;
				Dilations = pDilations;
				StartPadding = pStartPadding;
				EndPadding = pEndPadding;
				OutputPadding = pOutputPadding;
				GroupCount = groupCount;
				FusedActivation = pFusedActivation;
			}
		};
		using GEMMOperator = DML_GEMM_OPERATOR_DESC;
		using ReduceOperator = DML_REDUCE_OPERATOR_DESC;

		using AveragePooling = DML_AVERAGE_POOLING_OPERATOR_DESC;
		using LpPooling = DML_LP_POOLING_OPERATOR_DESC;
		using MaxPooling = DML_MAX_POOLING_OPERATOR_DESC;
		using RoiPooling = DML_ROI_POOLING_OPERATOR_DESC;

		using SliceOperator = DML_SLICE_OPERATOR_DESC;
		using CastOperator = DML_CAST_OPERATOR_DESC;
		using SplitOperator = DML_SPLIT_OPERATOR_DESC;
		using JoinOperator = DML_JOIN_OPERATOR_DESC;
		using PaddingOperator = DML_PADDING_OPERATOR_DESC;
		using ValueScale2D = DML_VALUE_SCALE_2D_OPERATOR_DESC;
		struct Upsample2D : public DML_UPSAMPLE_2D_OPERATOR_DESC
		{
			Upsample2D(const TensorDesc* pInputTensor, const TensorDesc* pOutputTensor,
				uint32_t scaleSizeX, uint32_t scaleSizeY, InterpolationType interpolationType)
			{
				InputTensor = pInputTensor;
				OutputTensor = pOutputTensor;
				ScaleSize.Width = scaleSizeX;
				ScaleSize.Height = scaleSizeY;
				InterpolationMode = GetDMLInterpolationMode(interpolationType);
			}
		};
		using GatherOperator = DML_GATHER_OPERATOR_DESC;
		using SpaceToDepth = DML_SPACE_TO_DEPTH_OPERATOR_DESC;
		using DepthToSpace = DML_DEPTH_TO_SPACE_OPERATOR_DESC;
		using TileOperator = DML_TILE_OPERATOR_DESC;
		using TopKOperator = DML_TOP_K_OPERATOR_DESC;

		using BatchNormalization = DML_BATCH_NORMALIZATION_OPERATOR_DESC;
		using MeanVarianceNormalization = DML_MEAN_VARIANCE_NORMALIZATION_OPERATOR_DESC;
		using LocalResponseNormalization = DML_LOCAL_RESPONSE_NORMALIZATION_OPERATOR_DESC;
		using LpNormalization = DML_LP_NORMALIZATION_OPERATOR_DESC;
		
		using RNNOperator = DML_RNN_OPERATOR_DESC;
		using LSTMOperator = DML_LSTM_OPERATOR_DESC;
		using GRUOperator = DML_GRU_OPERATOR_DESC;
	}
}
