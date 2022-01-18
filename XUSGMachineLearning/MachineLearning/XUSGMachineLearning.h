//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Core/XUSG.h"

namespace XUSG
{
	namespace ML
	{
		enum class TensorDataType
		{
			UNKNOWN,
			FLOAT32,
			FLOAT16,
			UINT32,
			UINT16,
			UINT8,
			INT32,
			INT16,
			INT8
		};

		enum class OperatorType
		{
			INVALID,
			ELEMENT_WISE_IDENTITY,
			ELEMENT_WISE_ABS,
			ELEMENT_WISE_ACOS,
			ELEMENT_WISE_ADD,
			ELEMENT_WISE_ASIN,
			ELEMENT_WISE_ATAN,
			ELEMENT_WISE_CEIL,
			ELEMENT_WISE_CLIP,
			ELEMENT_WISE_COS,
			ELEMENT_WISE_DIVIDE,
			ELEMENT_WISE_EXP,
			ELEMENT_WISE_FLOOR,
			ELEMENT_WISE_LOG,
			ELEMENT_WISE_LOGICAL_AND,
			ELEMENT_WISE_LOGICAL_EQUALS,
			ELEMENT_WISE_LOGICAL_GREATER_THAN,
			ELEMENT_WISE_LOGICAL_LESS_THAN,
			ELEMENT_WISE_LOGICAL_NOT,
			ELEMENT_WISE_LOGICAL_OR,
			ELEMENT_WISE_LOGICAL_XOR,
			ELEMENT_WISE_MAX,
			ELEMENT_WISE_MEAN,
			ELEMENT_WISE_MIN,
			ELEMENT_WISE_MULTIPLY,
			ELEMENT_WISE_POW,
			ELEMENT_WISE_CONSTANT_POW,
			ELEMENT_WISE_RECIP,
			ELEMENT_WISE_SIN,
			ELEMENT_WISE_SQRT,
			ELEMENT_WISE_SUBTRACT,
			ELEMENT_WISE_TAN,
			ELEMENT_WISE_THRESHOLD,
			ELEMENT_WISE_QUANTIZE_LINEAR,
			ELEMENT_WISE_DEQUANTIZE_LINEAR,
			ACTIVATION_ELU,
			ACTIVATION_HARDMAX,
			ACTIVATION_HARD_SIGMOID,
			ACTIVATION_IDENTITY,
			ACTIVATION_LEAKY_RELU,
			ACTIVATION_LINEAR,
			ACTIVATION_LOG_SOFTMAX,
			ACTIVATION_PARAMETERIZED_RELU,
			ACTIVATION_PARAMETRIC_SOFTPLUS,
			ACTIVATION_RELU,
			ACTIVATION_SCALED_ELU,
			ACTIVATION_SCALED_TANH,
			ACTIVATION_SIGMOID,
			ACTIVATION_SOFTMAX,
			ACTIVATION_SOFTPLUS,
			ACTIVATION_SOFTSIGN,
			ACTIVATION_TANH,
			ACTIVATION_THRESHOLDED_RELU,
			CONVOLUTION,
			GEMM,
			REDUCE,
			AVERAGE_POOLING,
			LP_POOLING,
			MAX_POOLING,
			ROI_POOLING,
			SLICE,
			CAST,
			SPLIT,
			JOIN,
			PADDING,
			VALUE_SCALE_2D,
			UPSAMPLE_2D,
			GATHER,
			SPACE_TO_DEPTH,
			DEPTH_TO_SPACE,
			TILE,
			TOP_K,
			BATCH_NORMALIZATION,
			MEAN_VARIANCE_NORMALIZATION,
			LOCAL_RESPONSE_NORMALIZATION,
			LP_NORMALIZATION,
			RNN,
			LSTM,
			GRU
		};

		enum class TensorFlag
		{
			NONE,
			MANAGED
		};

		DEFINE_ENUM_FLAG_OPERATORS(TensorFlag);

		enum class ExecutionFlag
		{
			NONE,
			ALLOW_HALF_PRECISION_COMPUTATION,
			DISABLE_META_COMMANDS,
			DESCRIPTORS_VOLATILE
		};

		DEFINE_ENUM_FLAG_OPERATORS(ExecutionFlag);

		enum class CreateDeviceFlag
		{
			NONE = 0,
			DEBUG = 0x1
		};

		DEFINE_ENUM_FLAG_OPERATORS(CreateDeviceFlag);

		enum class ReduceFunction
		{
			ARGMAX,
			ARGMIN,
			AVERAGE,
			L1,
			L2,
			LOG_SUM,
			LOG_SUM_EXP,
			MAX,
			MIN,
			MULTIPLY,
			SUM,
			SUM_SQUARE
		};

		enum class MatrixTransform
		{
			NONE,
			TRANSPOSE
		};

		enum class ConvolutionType
		{
			CONVOLUTION,
			CROSS_CORRELATION
		};

		enum class ConvolutionDirection
		{
			FORWARD,
			BACKWARD
		};

		enum class PaddingType
		{
			CONSTANT,
			EDGE,
			REFLECTION,
			SYMMETRIC
		};

		enum class InterpolationType
		{
			NEAREST_NEIGHBOR,
			LINEAR
		};

		enum class RecurrentNetworkDirection
		{
			FORWARD,
			BACKWARD,
			BIDIRECTIONAL
		};

		class Tensor;
		class CommandRecorder;

		using Dispatchable = void*;
		using CompiledOperator = void*;
		using BindingTable = void*;

		struct ScaleBias
		{
			float Scale;
			float Bias;
		};

		//--------------------------------------------------------------------------------------
		// Typed operators
		//--------------------------------------------------------------------------------------

		struct UnaryOp
		{
			OperatorType Type;
			const Tensor* pInput;
			const Tensor* pOutput;
		};

		struct ElementWiseUnaryOp
		{
			OperatorType Type;
			const Tensor* pInput;
			const Tensor* pOutput;
			const ScaleBias* pScaleBias;
		};

		struct ElementWiseBinaryOp
		{
			OperatorType Type;
			const Tensor* pA;
			const Tensor* pB;
			const Tensor* pOutput;
		};

		using ElementWiseIdentity = ElementWiseUnaryOp;
		using ElementWiseAbs = ElementWiseUnaryOp;
		using ElementWiseAcos = ElementWiseUnaryOp;
		using ElementWiseAdd = ElementWiseBinaryOp;
		using ElementWiseAsin = ElementWiseUnaryOp;
		using ElementWiseAtan = ElementWiseUnaryOp;
		using ElementWiseCeil = ElementWiseUnaryOp;

		struct ElementWiseClip
		{
			OperatorType Type;
			const Tensor* pInput;
			const Tensor* pOutput;
			const ScaleBias* pScaleBias;
			float Min;
			float Max;
		};

		using ElementWiseCos = ElementWiseUnaryOp;
		using ElementWiseDivide = ElementWiseBinaryOp;
		using ElementWiseExp = ElementWiseUnaryOp;
		using ElementWiseFloor = ElementWiseUnaryOp;
		using ElementWiseLog = ElementWiseUnaryOp;
		using ElementWiseLogicalAnd = ElementWiseBinaryOp;
		using ElementWiseLogicalEquals = ElementWiseBinaryOp;
		using ElementWiseLogicalGreater = ElementWiseBinaryOp;
		using ElementWiseLogicalLessThan = ElementWiseBinaryOp;
		using ElementWiseLogicalNot = UnaryOp;
		using ElementWiseLogicalOr = ElementWiseBinaryOp;
		using ElementWiseLogicalXor = ElementWiseBinaryOp;
		using ElementWiseMax = ElementWiseBinaryOp;
		using ElementWiseMean = ElementWiseBinaryOp;
		using ElementWiseMin = ElementWiseBinaryOp;
		using ElementWiseMultiply = ElementWiseBinaryOp;

		struct ElementWisePow
		{
			OperatorType Type;
			const Tensor* pInput;
			const Tensor* pExponent;
			const Tensor* pOutput;
			const ScaleBias* pScaleBias;
		};

		struct ElementWiseConstantPow
		{
			OperatorType Type;
			const Tensor* pInput;
			const Tensor* pOutput;
			const ScaleBias* pScaleBias;
			float Exponent;
		};

		using ElementWiseRecip = ElementWiseUnaryOp;
		using ElementWiseSin = ElementWiseUnaryOp;
		using ElementWiseSqrt = ElementWiseUnaryOp;
		using ElementWiseSubtract = ElementWiseBinaryOp;
		using ElementWiseTan = ElementWiseUnaryOp;

		struct ElementWiseThreshold
		{
			OperatorType Type;
			const Tensor* pInput;
			const Tensor* pOutput;
			const ScaleBias* pScaleBias;
			float Min;
		};

		struct ElementWiseQuantizeLinear
		{
			OperatorType Type;
			const Tensor* pInput;
			const Tensor* pScale;
			const Tensor* pZeroPoint;
			const Tensor* pOutput;
		};

		using ElementWiseDequantizeLinear = ElementWiseQuantizeLinear;

		struct ActivationELU
		{
			OperatorType Type;
			const Tensor* pInput;
			const Tensor* pOutput;
			float Alpha;
		};

		using ActivationHardMax = UnaryOp;

		struct ActivationHardSigmoid
		{
			OperatorType Type;
			const Tensor* pInput;
			const Tensor* pOutput;
			float Alpha;
			float Beta;
		};

		using ActivationIdentity = UnaryOp;
		using ActivationLeakyRELU = ActivationELU;
		using ActivationLinear = ActivationHardSigmoid;
		using ActivationLogSoftmax = UnaryOp;

		struct ActivationParameterizedRELU
		{
			OperatorType Type;
			const Tensor* pInput;
			const Tensor* pSlope;
			const Tensor* pOutput;
		};

		using ActivationParametricSoftplus = ActivationLinear;
		using ActivationRELU = UnaryOp;

		struct ActivationScaledELU
		{
			OperatorType Type;
			const Tensor* pInput;
			const Tensor* pOutput;
			float Alpha;
			float Gamma;
		};

		using ActivationScaledTanh = ActivationLinear;
		using ActivationSigmoid = UnaryOp;
		using ActivationSoftmax = UnaryOp;

		struct ActivationSoftplus
		{
			OperatorType Type;
			const Tensor* pInput;
			const Tensor* pOutput;
			float Steepness;
		};

		using ActivationSoftsign = UnaryOp;
		using ActivationTanh = UnaryOp;
		using ActivationThresholdRELU = ActivationELU;

		struct ConvolutionOperator
		{
			OperatorType Type;
			const Tensor* pInput;
			const Tensor* pFilter;
			const Tensor* pBias;
			const Tensor* pOutput;
			ConvolutionType Mode;
			ConvolutionDirection Direction;
			uint32_t DimensionCount;
			const uint32_t* pStrides;
			const uint32_t* pDilations;
			const uint32_t* pStartPadding;
			const uint32_t* pEndPadding;
			const uint32_t* pOutputPadding;
			uint32_t GroupCount;
			const void* pFusedActivation;
		};

		struct GEMMOperator
		{
			OperatorType Type;
			const Tensor* pA;
			const Tensor* pB;
			const Tensor* pC;
			const Tensor* pOutput;
			MatrixTransform TransA;
			MatrixTransform TransB;
			float Alpha;
			float Beta;
			const void* pFusedActivation;
		};

		struct ReduceOperator
		{
			OperatorType Type;
			ReduceFunction Function;
			const Tensor* pInput;
			const Tensor* pOutput;
			uint32_t AxisCount;
			const uint32_t* pAxes;
		};

		struct AveragePooling
		{
			OperatorType Type;
			const Tensor* pInput;
			const Tensor* pOutput;
			uint32_t DimensionCount;
			const uint32_t* pStrides;
			const uint32_t* pWindowSize;
			const uint32_t* pStartPadding;
			const uint32_t* pEndPadding;
			bool IncludePadding;
		};

		struct LPPooling
		{
			OperatorType Type;
			const Tensor* pInput;
			const Tensor* pOutput;
			uint32_t DimensionCount;
			const uint32_t* pStrides;
			const uint32_t* pWindowSize;
			const uint32_t* pStartPadding;
			const uint32_t* pEndPadding;
			uint32_t P;
		};

		struct MaxPooling
		{
			OperatorType Type;
			const Tensor* pInput;
			const Tensor* pOutput;
			uint32_t DimensionCount;
			const uint32_t* pStrides;
			const uint32_t* pWindowSize;
			const uint32_t* pStartPadding;
			const uint32_t* pEndPadding;
		};

		struct ROIPooling
		{
			OperatorType Type;
			const Tensor* pInput;
			const Tensor* pROI;
			const Tensor* pOutput;
			float SpatialScale;
			uint32_t PooledWidth;
			uint32_t PooledHeight;
		};

		struct SliceOperator
		{
			OperatorType Type;
			const Tensor* pInput;
			const Tensor* pOutput;
			uint32_t DimensionCount;
			const uint32_t* pOffsets;
			const uint32_t* pSizes;
			const uint32_t* pStrides;
		};

		using CastOperator = UnaryOp;

		struct SplitOperator
		{
			OperatorType Type;
			const Tensor* pInput;
			uint32_t OutputCount;
			const Tensor* pOutputs;
			uint32_t Axis;
		};

		struct JoinOperator
		{
			OperatorType Type;
			uint32_t InputCount;
			const Tensor* pInputs;
			const Tensor* pOutput;
			uint32_t Axis;
		};

		struct PaddingOperator
		{
			OperatorType Type;
			const Tensor* pInput;
			const Tensor* pOutput;
			PaddingType PaddingMode;
			float PaddingValue;
			uint32_t DimensionCount;
			const uint32_t* pStartPadding;
			const uint32_t* pEndPadding;
		};

		struct ValueScale2D
		{
			OperatorType Type;
			const Tensor* pInput;
			const Tensor* pOutput;
			float Scale;
			uint32_t ChannelCount;
			const float* pBias;
		};

		struct Upsample2D
		{
			OperatorType Type;
			const Tensor* pInput;
			const Tensor* pOutput;
			uint32_t ScaleWidth;
			uint32_t ScaleHeight;
			InterpolationType InterpolationMode;
		};

		struct GatherOperator
		{
			OperatorType Type;
			const Tensor* pInput;
			const Tensor* pIndices;
			const Tensor* pOutput;
			uint32_t Axis;
			uint32_t IndexDimensions;
		};

		struct SpaceToDepth
		{
			OperatorType Type;
			const Tensor* pInput;
			const Tensor* pOutput;
			uint32_t BlockSize;
		};

		using DepthToSpace = SpaceToDepth;

		struct TileOperator
		{
			OperatorType Type;
			const Tensor* pInput;
			const Tensor* pOutput;
			uint32_t RepeatsCount;
			const uint32_t* pRepeats;
		};

		struct TopKOperator
		{
			OperatorType Type;
			const Tensor* pInput;
			const Tensor* pOutputValue;
			const Tensor* pOutputIndex;
			uint32_t Axis;
			uint32_t K;
		};

		struct BatchNormalization
		{
			OperatorType Type;
			const Tensor* pInput;
			const Tensor* pMean;
			const Tensor* pVariance;
			const Tensor* pScale;
			const Tensor* pBias;
			const Tensor* pOutput;
			bool Spatial;
			float Epsilon;
			const void* pFusedActivation;
		};

		struct MeanVarianceNormalization
		{
			OperatorType Type;
			const Tensor* pInput;
			const Tensor* pScale;
			const Tensor* pBias;
			const Tensor* pOutput;
			bool CrossChannel;
			bool NormalizeVariance;
			float Epsilon;
			const void* pFusedActivation;
		};

		struct LocalResponseNormalization
		{
			OperatorType Type;
			const Tensor* pInput;
			const Tensor* pOutput;
			bool CrossChannel;
			uint32_t LocalSize;
			float Alpha;
			float Beta;
			float Bias;
		};

		struct LPNormalization
		{
			OperatorType Type;
			const Tensor* pInput;
			const Tensor* pOutput;
			uint32_t Axis;
			float Epsilon;
			uint32_t P;
		};

		struct RNNOperator
		{
			const Tensor* pInput;
			const Tensor* pWeight;
			const Tensor* pRecurrence;
			const Tensor* pBias;
			const Tensor* pHiddenInit;
			const Tensor* pSequenceLengths;
			const Tensor* pOutputSequence;
			const Tensor* pOutputSingle;
			uint32_t ActivationCount;
			const void** pActivations;
			RecurrentNetworkDirection Direction;
		};

		struct LSTMOperator
		{
			const Tensor* pInput;
			const Tensor* pWeight;
			const Tensor* pRecurrence;
			const Tensor* pBias;
			const Tensor* pHiddenInit;
			const Tensor* pCellMemInit;
			const Tensor* pSequenceLengths;
			const Tensor* pPeephole;
			const Tensor* pOutputSequence;
			const Tensor* pOutputSingle;
			const Tensor* pOutputCellSingle;
			uint32_t ActivationCount;
			const void** pActivations;
			RecurrentNetworkDirection Direction;
			float ClipThreshold;
			bool UseClipThreshold;
			bool CoupleInputForget;
		};

		struct GRUOperator
		{
			const Tensor* pInput;
			const Tensor* pWeight;
			const Tensor* pRecurrence;
			const Tensor* pBias;
			const Tensor* pHiddenInit;
			const Tensor* pSequenceLengths;
			const Tensor* pOutputSequence;
			const Tensor* pOutputSingle;
			uint32_t ActivationCount;
			const void** pActivations;
			RecurrentNetworkDirection Direction;
			bool LinearBeforeReset;
		};

		//--------------------------------------------------------------------------------------
		// Device
		//--------------------------------------------------------------------------------------
		class CommandRecorder;

		class DLL_INTERFACE Device
		{
		public:
			//Device();
			virtual ~Device() {};

			virtual bool GetCommandRecorder(CommandRecorder* pCommandRecorder, const wchar_t* name = nullptr) = 0;
			virtual bool Create(const XUSG::Device* pDevice, CreateDeviceFlag flags, const wchar_t* name = nullptr) = 0;

			virtual void Create(void* pHandle, const wchar_t* name = nullptr) = 0;

			virtual void* GetHandle() const = 0;

			using uptr = std::unique_ptr<Device>;
			using sptr = std::shared_ptr<Device>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
		};

		//--------------------------------------------------------------------------------------
		// Tensor
		//--------------------------------------------------------------------------------------
		class DLL_INTERFACE Tensor
		{
		public:
			//Tensor();
			virtual ~Tensor() {}

			virtual uint64_t Create(TensorDataType dataType, uint32_t dimensionCount,
				const uint32_t* pSizes, const uint32_t* pStrides = nullptr,
				TensorFlag flags = TensorFlag::NONE) = 0;
			virtual uint64_t GetTensorBufferSize() const = 0;

			virtual const void* GetHandle() const = 0;

			using uptr = std::unique_ptr<Tensor>;
			using sptr = std::shared_ptr<Tensor>;

			static uptr MakeUnique(XUSG::API api = XUSG::API::DIRECTX_12);
			static sptr MakeShared(XUSG::API api = XUSG::API::DIRECTX_12);
		};

		//--------------------------------------------------------------------------------------
		// Operator
		//--------------------------------------------------------------------------------------
		class DLL_INTERFACE Operator
		{
		public:
			//Operator();
			virtual ~Operator() {}

			virtual bool Create(const Device* pDevice, const void* pTypedOp,
				ExecutionFlag flags = ExecutionFlag::NONE) = 0;

			virtual Dispatchable GetDispatchable() const = 0;

			virtual CompiledOperator GetCompiled() = 0;

			virtual uint32_t GetDescriptorCount() const = 0;
			virtual uint64_t GetTemporaryResourceSize() const = 0;
			virtual uint64_t GetPersistentResourceSize() const = 0;

			Operator* AsOperator();

			using uptr = std::unique_ptr<Operator>;
			using sptr = std::shared_ptr<Operator>;

			static uptr MakeUnique(XUSG::API api = XUSG::API::DIRECTX_12);
			static sptr MakeShared(XUSG::API api = XUSG::API::DIRECTX_12);
		};

		//--------------------------------------------------------------------------------------
		// Operator initializer
		//--------------------------------------------------------------------------------------
		class DLL_INTERFACE OperatorInitializer :
			public virtual Operator
		{
		public:
			//OperatorInitializer();
			virtual ~OperatorInitializer() {}

			virtual bool Create(const Device* pDevice, const Operator::sptr* pOperators, uint32_t numOperators) = 0;

			using uptr = std::unique_ptr<OperatorInitializer>;
			using sptr = std::shared_ptr<OperatorInitializer>;

			static uptr MakeUnique(XUSG::API api = XUSG::API::DIRECTX_12);
			static sptr MakeShared(XUSG::API api = XUSG::API::DIRECTX_12);
		};

		//--------------------------------------------------------------------------------------
		// Binding
		//--------------------------------------------------------------------------------------
		class DLL_INTERFACE Binding
		{
		public:
			//Binding();
			virtual ~Binding() {}

			virtual bool Create(const Device* pDevice, const Operator& dispatchable, const DescriptorPool& descriptorPool,
				uint32_t descriptorCount, int32_t descriptorOffset = 0) = 0;
			virtual bool Reset(const Operator& dispatchable, const DescriptorPool& descriptorPool,
				uint32_t descriptorCount, int32_t descriptorOffset = 0) = 0;

			virtual void BindInput(uint32_t i, const Resource* pResource, uint64_t size = 0, uint64_t offset = 0) = 0;
			virtual void BindInput(uint32_t i, size_t bindingIndex, uint32_t bindingCount = 1) = 0;
			virtual void BindOutput(uint32_t i, const Resource* pResource, uint64_t size = 0, uint64_t offset = 0) = 0;
			virtual void BindOutput(uint32_t i, size_t bindingIndex, uint32_t bindingCount = 1) = 0;
			virtual void AppendInput(const Resource* pResource, uint64_t size = 0, uint64_t offset = 0) = 0;
			virtual void AppendInput(size_t bindingIndex, uint32_t bindingCount = 1) = 0;
			virtual void AppendOutput(const Resource* pResource, uint64_t size = 0, uint64_t offset = 0) = 0;
			virtual void AppendOutput(size_t bindingIndex, uint32_t bindingCount = 1) = 0;

			virtual void BindInputBuffer(uint32_t i, const Resource* pResource, uint64_t size = 0, uint64_t offset = 0) = 0;
			virtual void BindOutputBuffer(uint32_t i, const Resource* pResource, uint64_t size = 0, uint64_t offset = 0) = 0;

			virtual void BindTemporary(const Resource* pResource, uint64_t size = 0, uint64_t offset = 0) = 0;
			virtual void BindPersistent(const Resource* pResource, uint64_t size = 0, uint64_t offset = 0) = 0;

			virtual BindingTable GetBindingTable() const = 0;
			virtual BindingTable GetDispatchableBindingTable() = 0;

			using uptr = std::unique_ptr<Binding>;
			using sptr = std::shared_ptr<Binding>;

			static uptr MakeUnique(XUSG::API api = XUSG::API::DIRECTX_12);
			static sptr MakeShared(XUSG::API api = XUSG::API::DIRECTX_12);
		};

		//--------------------------------------------------------------------------------------
		// Command recorder
		//--------------------------------------------------------------------------------------
		class DLL_INTERFACE CommandRecorder
		{
		public:
			//CommandRecorder();
			virtual ~CommandRecorder() {}

			virtual bool Create(const Device* pDevice, const wchar_t* name = nullptr) = 0;

			virtual void Dispatch(XUSG::CommandList* pCommandList, const Dispatchable& dispatchable, const BindingTable& bindings) const = 0;

			virtual void* GetHandle() const = 0;
			virtual void* GetDeviceHandle() const = 0;

			virtual const Device* GetDevice() const = 0;

			using uptr = std::unique_ptr<CommandRecorder>;
			using sptr = std::shared_ptr<CommandRecorder>;

			static uptr MakeUnique(XUSG::API api = XUSG::API::DIRECTX_12);
			static sptr MakeShared(XUSG::API api = XUSG::API::DIRECTX_12);
		};

		//--------------------------------------------------------------------------------------
		// Utility
		//--------------------------------------------------------------------------------------

		enum class TensorLayout
		{
			DEFAULT,
			NHWC
		};

		using WeightsType = std::vector<float>;
		using WeightMapType = std::map<std::string, WeightsType>;

		//--------------------------------------------------------------------------------------
		// Util
		//--------------------------------------------------------------------------------------
		class DLL_INTERFACE Util
		{
		public:
			//Util(TensorDataType tensorDataType = TensorDataType::FLOAT32,
				//TensorLayout tensorLayout = TensorLayout::DEFAULT);
			virtual ~Util() {}

			virtual bool CreateUpsampleLayer(const Device* pDevice, const uint32_t inputSizes[4],
				uint64_t& inputBufferRequiredSize, uint64_t& outputBufferRequiredSize, uint32_t outputSizes[4],
				Operator& opOut, uint32_t scaleSizeX = 2, uint32_t scaleSizeY = 2,
				InterpolationType interpolationType = InterpolationType::NEAREST_NEIGHBOR) = 0;
			virtual bool CreateConvolutionLayer(const Device* pDevice, const uint32_t inputSizes[4],
				const uint32_t* filterSizes, bool useBiasAndActivation, uint64_t& inputBufferRequiredSize,
				uint64_t& outputBufferRequiredSize, uint32_t outputSizes[4], Operator& opOut) = 0;
			virtual bool CreateAdditionLayer(const Device* pDevice, const uint32_t inputSizes[4], Operator& opOut) = 0;

			virtual void CreateWeightTensors(WeightMapType& weights, const char* convLayerName, const char* scaleLayerName,
				const char* shiftLayerName, const uint32_t filterSizes[4], std::vector<uint8_t>& filterWeightsOut,
				std::vector<uint8_t>& biasWeightsOut) = 0;

			static void GetStrides(const uint32_t sizes[4], TensorLayout layout, uint32_t stridesOut[4]);

			using uptr = std::unique_ptr<Util>;
			using sptr = std::shared_ptr<Util>;

			static uptr MakeUnique(TensorDataType tensorDataType, TensorLayout tensorLayout, XUSG::API api = XUSG::API::DIRECTX_12);
			static sptr MakeShared(TensorDataType tensorDataType, TensorLayout tensorLayout, XUSG::API api = XUSG::API::DIRECTX_12);
		};
	}
}
