//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSGMachineLearning.h"

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

		DML_REDUCE_FUNCTION GetDMLReduceFunction(ReduceFunction reduceFunction);
		DML_MATRIX_TRANSFORM GetDMLMatrixTransform(MatrixTransform matrixTransform);
		DML_CONVOLUTION_MODE GetDMLConvolutionMode(ConvolutionMode convolutionMode);
		DML_CONVOLUTION_DIRECTION GetDMLConvolutionDirection(ConvolutionDirection convolutionDirection);
		DML_PADDING_MODE GetDMLPaddingMode(PaddingType paddingMode);
		DML_INTERPOLATION_MODE GetDMLInterpolationMode(InterpolationType interpolationMode);

		void GetDMLTypedOperator(std::vector<uint8_t>& dmlTypedOpDesc, OperatorType type, const void* pOpDesc);

		// Device
		class Device_DML :
			public virtual Device
		{
		public:
			Device_DML();
			virtual ~Device_DML();

			bool GetCommandRecorder(CommandRecorder* pCommandRecorder, const wchar_t* name = nullptr);
			bool Create(const XUSG::Device* pDevice, CreateDeviceFlag flags, const wchar_t* name = nullptr);

			void Create(void* pHandle, const wchar_t* name = nullptr);

			void* GetHandle() const;

		protected:
			com_ptr<IDMLDevice> m_device;
		};

		using BatchNormalization = DML_BATCH_NORMALIZATION_OPERATOR_DESC;
		using MeanVarianceNormalization = DML_MEAN_VARIANCE_NORMALIZATION_OPERATOR_DESC;
		using LocalResponseNormalization = DML_LOCAL_RESPONSE_NORMALIZATION_OPERATOR_DESC;
		using LPNormalization = DML_LP_NORMALIZATION_OPERATOR_DESC;
		
		using RNNOperator = DML_RNN_OPERATOR_DESC;
		using LSTMOperator = DML_LSTM_OPERATOR_DESC;
		using GRUOperator = DML_GRU_OPERATOR_DESC;
	}
}
