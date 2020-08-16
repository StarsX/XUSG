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
			MANAGED,
		};

		DEFINE_ENUM_FLAG_OPERATORS(TensorFlag);

		enum class ExecutionFlag
		{
			NONE,
			ALLOW_HALF_PRECISION_COMPUTATION,
			DISABLE_META_COMMANDS,
			DESCRIPTORS_VOLATILE,
		};

		DEFINE_ENUM_FLAG_OPERATORS(ExecutionFlag);

		enum class ConvolutionMode
		{
			CONVOLUTION,
			CROSS_CORRELATION
		};

		enum class ConvolutionDirection
		{
			FORWARD,
			BACKWARD
		};

		enum class InterpolationType
		{
			NEAREST_NEIGHBOR,
			LINEAR
		};

		class CommandRecorder;
	}
}

#include "XUSGMachineLearning_DML.h"

namespace XUSG
{
	namespace ML
	{
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

			virtual const TensorDesc& GetTensorDesc() const = 0;
			virtual uint64_t GetTensorBufferSize() const = 0;

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

			virtual bool Create(const Device& device, const OperatorDesc& desc, ExecutionFlag flags = ExecutionFlag::NONE) = 0;

			virtual const Dispatchable& GetDispatchable() const = 0;

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

			virtual bool Create(const Device& device, const Operator::sptr* pOperators, uint32_t numOperators) = 0;

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

			virtual bool Create(const Device& device, const Operator& dispatchable, const DescriptorPool& descriptorPool,
				uint32_t descriptorCount, int32_t descriptorOffset = 0) = 0;
			virtual bool Reset(const Operator& dispatchable, const DescriptorPool& descriptorPool,
				uint32_t descriptorCount, int32_t descriptorOffset = 0) = 0;

			virtual void BindInput(uint32_t i, const ResourceBase& buffer, uint64_t size = 0, uint64_t offset = 0) = 0;
			virtual void BindInput(uint32_t i, const Resource& resource, uint64_t size = 0, uint64_t offset = 0) = 0;
			virtual void BindInput(uint32_t i, size_t bindingIndex, uint32_t bindingCount = 1) = 0;
			virtual void BindOutput(uint32_t i, const ResourceBase& buffer, uint64_t size = 0, uint64_t offset = 0) = 0;
			virtual void BindOutput(uint32_t i, const Resource& resource, uint64_t size = 0, uint64_t offset = 0) = 0;
			virtual void BindOutput(uint32_t i, size_t bindingIndex, uint32_t bindingCount = 1) = 0;
			virtual void AppendInput(const ResourceBase& buffer, uint64_t size = 0, uint64_t offset = 0) = 0;
			virtual void AppendInput(const Resource& resource, uint64_t size = 0, uint64_t offset = 0) = 0;
			virtual void AppendInput(size_t bindingIndex, uint32_t bindingCount = 1) = 0;
			virtual void AppendOutput(const ResourceBase& buffer, uint64_t size = 0, uint64_t offset = 0) = 0;
			virtual void AppendOutput(const Resource& resource, uint64_t size = 0, uint64_t offset = 0) = 0;
			virtual void AppendOutput(size_t bindingIndex, uint32_t bindingCount = 1) = 0;

			virtual void BindInputBuffer(uint32_t i, const ResourceBase& buffer, uint64_t size = 0, uint64_t offset = 0) = 0;
			virtual void BindInputBuffer(uint32_t i, const Resource& resource, uint64_t size = 0, uint64_t offset = 0) = 0;
			virtual void BindOutputBuffer(uint32_t i, const ResourceBase& buffer, uint64_t size = 0, uint64_t offset = 0) = 0;
			virtual void BindOutputBuffer(uint32_t i, const Resource& resource, uint64_t size = 0, uint64_t offset = 0) = 0;

			virtual void BindTemporary(const ResourceBase& buffer, uint64_t size = 0, uint64_t offset = 0) = 0;
			virtual void BindTemporary(const Resource& resource, uint64_t size = 0, uint64_t offset = 0) = 0;
			virtual void BindPersistent(const ResourceBase& buffer, uint64_t size = 0, uint64_t offset = 0) = 0;
			virtual void BindPersistent(const Resource& resource, uint64_t size = 0, uint64_t offset = 0) = 0;

			virtual const BindingTable& GetBindingTable() const = 0;
			virtual const BindingTable& GetDispatchableBindingTable() = 0;

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

			virtual void Dispatch(XUSG::CommandList* pCommandList, const Dispatchable& dispatchable, const BindingTable& bindings) const = 0;

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
			//Util(const Device& device, TensorDataType tensorDataType = TensorDataType::FLOAT32,
				//TensorLayout tensorLayout = TensorLayout::DEFAULT);
			virtual ~Util() {}

			virtual bool CreateUpsampleLayer(const uint32_t inputSizes[4], uint64_t& inputBufferRequiredSize,
				uint64_t& outputBufferRequiredSize, uint32_t outputSizes[4], Operator& opOut,
				uint32_t scaleSizeX = 2, uint32_t scaleSizeY = 2,
				InterpolationType interpolationType = InterpolationType::NEAREST_NEIGHBOR) = 0;
			virtual bool CreateConvolutionLayer(const uint32_t inputSizes[4], const uint32_t* filterSizes,
				bool useBiasAndActivation, uint64_t& inputBufferRequiredSize, uint64_t& outputBufferRequiredSize,
				uint32_t outputSizes[4], Operator& opOut) = 0;
			virtual bool CreateAdditionLayer(const uint32_t inputSizes[4], Operator& opOut) = 0;

			virtual void CreateWeightTensors(WeightMapType& weights, const char* convLayerName, const char* scaleLayerName,
				const char* shiftLayerName, const uint32_t filterSizes[4], std::vector<uint8_t>& filterWeightsOut,
				std::vector<uint8_t>& biasWeightsOut) = 0;

			static void GetStrides(const uint32_t sizes[4], TensorLayout layout, uint32_t stridesOut[4]);

			using uptr = std::unique_ptr<Util>;
			using sptr = std::shared_ptr<Util>;

			static uptr MakeUnique(const Device& device, TensorDataType tensorDataType,
				TensorLayout tensorLayout, XUSG::API api = XUSG::API::DIRECTX_12);
			static sptr MakeShared(const Device& device, TensorDataType tensorDataType,
				TensorLayout tensorLayout, XUSG::API api = XUSG::API::DIRECTX_12);
		};
	}
}
