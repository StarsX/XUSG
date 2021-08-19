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
		DML_MATRIX_TRANSFORM GetDMLMatrixTransform(MatrixTransform transform);
		DML_CONVOLUTION_MODE GetDMLConvolutionMode(ConvolutionType mode);
		DML_CONVOLUTION_DIRECTION GetDMLConvolutionDirection(ConvolutionDirection direction);
		DML_PADDING_MODE GetDMLPaddingMode(PaddingType mode);
		DML_INTERPOLATION_MODE GetDMLInterpolationMode(InterpolationType mode);
		DML_RECURRENT_NETWORK_DIRECTION GetDMLRecurrentNetworkDirection(RecurrentNetworkDirection direction);

		void GetDMLTypedOperator(std::string& dmlTypedOpDesc, const void* pTypedOp);

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
	}
}
