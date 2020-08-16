//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGMachineLearningUtil.h"
#include "Float16Compressor.h"

using namespace std;
using namespace XUSG::ML;

Util_Impl::Util_Impl(const ML::Device& device, TensorDataType tensorDataType,
	TensorLayout tensorLayout, API api) :
	m_device(device),
	m_tensorDataType(tensorDataType),
	m_tensorLayout(tensorLayout),
	m_api(api)
{
}

Util_Impl::~Util_Impl()
{
}

bool Util_Impl::CreateUpsampleLayer(const uint32_t inputSizes[4], uint64_t& inputBufferRequiredSize,
	uint64_t& outputBufferRequiredSize, uint32_t outputSizes[4], Operator& opOut,
	uint32_t scaleSizeX, uint32_t scaleSizeY, InterpolationType interpolationType)
{
	// Describe input and output tensors
	uint32_t inputStrides[4];
	GetStrides(inputSizes, m_tensorLayout, inputStrides);

	Tensor::uptr inputTensor = Tensor::MakeUnique(m_api);
	const auto inputBufferSize = inputTensor->Create(m_tensorDataType,
		static_cast<uint32_t>(size(inputStrides)), inputSizes, inputStrides);

	// Because we can resuse resources for tensor storage, this tracks the resource size needed to hold the
	// largest possible tensor requested.
	inputBufferRequiredSize = (max)(inputBufferSize, inputBufferRequiredSize);

	// Output size is double in height and width
	outputSizes[0] = inputSizes[0];
	outputSizes[1] = inputSizes[1];
	outputSizes[2] = inputSizes[2] * scaleSizeX;
	outputSizes[3] = inputSizes[3] * scaleSizeY;

	uint32_t outputStrides[4];
	GetStrides(outputSizes, m_tensorLayout, outputStrides);

	Tensor::uptr outputTensor = Tensor::MakeUnique(m_api);
	const auto outputBufferSize = outputTensor->Create(m_tensorDataType,
		static_cast<uint32_t>(size(outputStrides)), outputSizes, outputStrides);
	outputBufferRequiredSize = (max)(outputBufferSize, outputBufferRequiredSize);

	Upsample2D upsample(&inputTensor->GetTensorDesc(), &outputTensor->GetTensorDesc(),
		scaleSizeX, scaleSizeY, interpolationType);

	return opOut.Create(m_device, OperatorDesc(OperatorType::UPSAMPLE_2D, &upsample), ExecutionFlag::ALLOW_HALF_PRECISION_COMPUTATION);
}

bool Util_Impl::CreateConvolutionLayer(const uint32_t inputSizes[4], const uint32_t* filterSizes, bool useBiasAndActivation,
	uint64_t& inputBufferRequiredSize, uint64_t& outputBufferRequiredSize, uint32_t outputSizes[4], Operator& opOut)
{
	// Describe input and output tensors    
	uint32_t inputStrides[4];
	GetStrides(inputSizes, m_tensorLayout, inputStrides);

	Tensor::uptr inputTensor = Tensor::MakeUnique(m_api);
	const auto inputBufferSize = inputTensor->Create(m_tensorDataType,
		static_cast<uint32_t>(size(inputStrides)), inputSizes, inputStrides);
	inputBufferRequiredSize = (max)(inputBufferSize, inputBufferRequiredSize);

	// The output shape has as many channels as there are convolution filters.
	outputSizes[0] = inputSizes[0];
	outputSizes[1] = filterSizes[0];
	outputSizes[2] = inputSizes[2];
	outputSizes[3] = inputSizes[3];

	uint32_t outputStrides[4];
	GetStrides(outputSizes, m_tensorLayout, outputStrides);

	Tensor::uptr outputTensor = Tensor::MakeUnique(m_api);
	const auto outputBufferSize = outputTensor->Create(m_tensorDataType,
		static_cast<uint32_t>(size(outputStrides)), outputSizes, outputStrides);
	outputBufferRequiredSize = (max)(outputBufferSize, outputBufferRequiredSize);

	// Describe weight tensors
	uint32_t filterStrides[4];
	GetStrides(filterSizes, m_tensorLayout, filterStrides);

	Tensor::uptr filterTensor = Tensor::MakeUnique(m_api);
#if ML_MANAGED_WEIGHTS
	const auto filterBufferSize = filterTensor->Create(m_tensorDataType,
		static_cast<uint32_t>(size(filterStrides)), filterSizes, filterStrides, TensorFlag::MANAGED);
#else
	const auto filterBufferSize = filterTensor.Create(m_tensorDataType,
		static_cast<uint32_t>(size(filterStrides)), filterSizes, filterStrides);
#endif
	N_RETURN(filterBufferSize > 0, false);

	const uint32_t biasSizes[] = { 1, filterSizes[0], 1, 1 };	// One bias per output channel    
	uint32_t biasStrides[4];
	GetStrides(biasSizes, m_tensorLayout, biasStrides);

	Tensor::uptr biasTensor = Tensor::MakeUnique(m_api);
#if ML_MANAGED_WEIGHTS
	const auto biasBufferSize = biasTensor->Create(m_tensorDataType,
		static_cast<uint32_t>(size(biasStrides)), biasSizes, biasStrides, TensorFlag::MANAGED);
#else
	const auto biasBufferSize = biasTensor.Create(m_tensorDataType,
		static_cast<uint32_t>(size(biasStrides)), biasSizes, biasStrides);
#endif
	N_RETURN(biasBufferSize > 0, false);

	// Describe, create, and compile convolution operator

	// The output size of a convolution operation is given by:
	//  height = (inputHeight - filterHeight + 2*paddingHeight) / filterStride + 1
	//  width  = (inputWidth  - filterWidth  + 2*paddingWidth ) / filterStride + 1
	//
	// We want to preserve the height and width, so assuming stride is 1, we get:
	//  paddingHeight = (filterHeight - 1) / 2
	//  paddingWidth  = (filterWidth  - 1) / 2
	// If padding is fractional, we pad unevenly with ceil/floor.
	const auto paddingHeightTop = static_cast<uint32_t>(ceil((filterSizes[2] - 1) / 2.0f));
	const auto  paddingHeightBottom = static_cast<uint32_t>(floor((filterSizes[2] - 1) / 2.0f));
	const auto  paddingWidthLeft = static_cast<uint32_t>(ceil((filterSizes[3] - 1) / 2.0f));
	const auto  paddingWidthRight = static_cast<uint32_t>(floor((filterSizes[3] - 1) / 2.0f));

	uint32_t strides[] = { 1, 1 };
	uint32_t dilations[] = { 1, 1 };
	uint32_t startPadding[] = { paddingHeightTop, paddingWidthLeft };
	uint32_t endPadding[] = { paddingHeightBottom, paddingWidthRight };
	uint32_t outputPadding[] = { 0, 0 };

	ActivationRELU fusedRelu = {};
	const OperatorDesc reluDesc(OperatorType::ACTIVATION_RELU, &fusedRelu);

	ConvolutionOperator conv(&inputTensor->GetTensorDesc(),
		&filterTensor->GetTensorDesc(),
		useBiasAndActivation ? &biasTensor->GetTensorDesc() : nullptr,
		&outputTensor->GetTensorDesc(),
		ConvolutionMode::CROSS_CORRELATION,
		ConvolutionDirection::FORWARD,
		2,
		strides,
		dilations,
		startPadding,
		endPadding,
		outputPadding,
		1,
		useBiasAndActivation ? &reluDesc : nullptr);

	return opOut.Create(m_device, OperatorDesc(OperatorType::CONVOLUTION, &conv), ExecutionFlag::ALLOW_HALF_PRECISION_COMPUTATION);
}

bool Util_Impl::CreateAdditionLayer(const uint32_t inputSizes[4], Operator& opOut)
{
	// Describe input and output tensors
	uint32_t strides[4];
	GetStrides(inputSizes, m_tensorLayout, strides);
	
	Tensor::uptr tensor = Tensor::MakeUnique(m_api);
	const auto bufferSize = tensor->Create(m_tensorDataType, static_cast<uint32_t>(size(strides)), inputSizes, strides);

	// Describe, create, and compile elementwise addition operator
	// Inputs and output are all the same size and use the same tensor desc.
	ElementWiseAdd add = { &tensor->GetTensorDesc(), &tensor->GetTensorDesc(), &tensor->GetTensorDesc() };

	return opOut.Create(m_device, OperatorDesc(OperatorType::ELEMENT_WISE_ADD, &add), ExecutionFlag::ALLOW_HALF_PRECISION_COMPUTATION);
}

void Util_Impl::CreateWeightTensors(WeightMapType& weights, const char* convLayerName, const char* scaleLayerName,
	const char* shiftLayerName, const uint32_t filterSizes[4], vector<uint8_t>& filterWeightsOut,
	vector<uint8_t>& biasWeightsOut)
{
	// There are two types of weights for the convolutions: The convolution filters themselves, and scale/shift
	// weights used to normalize and bias the results. The final layer doesn't use scale and shift weights, so
	// these are optional.

	auto useScaleShift = true;
	if (scaleLayerName == nullptr)
	{
		assert(shiftLayerName == nullptr);
		useScaleShift = false;
	}

	// Convert weight values to FP16
	const auto filterWeights = weights[convLayerName];
	WeightsType scaleWeights, shiftWeights;
	if (useScaleShift)
	{
		scaleWeights = weights[scaleLayerName];
		shiftWeights = weights[shiftLayerName];
	}

	const auto useFP16 = m_tensorDataType == TensorDataType::FLOAT16;
	std::vector<float> filterWeightsFP32;
	std::vector<float> biasWeightsFP32;
	std::vector<uint16_t> filterWeightsFP16;
	std::vector<uint16_t> biasWeightsFP16;

	const auto N = filterSizes[0];
	const auto C = filterSizes[1];
	const auto H = filterSizes[2];
	const auto W = filterSizes[3];

	for (auto n = 0u; n < N; ++n)
	{
		switch (m_tensorLayout)
		{
		case TensorLayout::NHWC:
			// We need to convert the weights from NCHW to NHWC.
			for (auto h = 0u; h < H; ++h)
				for (auto w = 0u; w < W; ++w)
					for (auto c = 0u; c < C; ++c)
					{
						// Apply the scale weight now so we don't need a normalization layer
						const auto idx = w + h * W + c * H * W + n * C * H * W;
						const auto scaledWeight = useScaleShift ? filterWeights[idx] * scaleWeights[n] : filterWeights[idx];
						if (useFP16) filterWeightsFP16.push_back(Float16Compressor::compress(scaledWeight));
						else filterWeightsFP32.push_back(scaledWeight);
					}
			break;

		default:
			// Weights are already in the right order
			for (auto i = 0u; i < C * H * W; ++i)
			{
				// Apply the scale weight now so we don't need a normalization layer
				const auto idx = n * C * H * W + i;
				const auto scaledWeight = useScaleShift ? filterWeights[idx] * scaleWeights[n] : filterWeights[idx];
				if (useFP16) filterWeightsFP16.push_back(Float16Compressor::compress(scaledWeight));
				else filterWeightsFP32.push_back(scaledWeight);
			}
		}

		if (useScaleShift)
		{
			// Technically this is initialBias*scale+shift, but the initial bias is 0
			if (useFP16) biasWeightsFP16.push_back(Float16Compressor::compress(shiftWeights[n]));
			else biasWeightsFP32.push_back(shiftWeights[n]);
		}
	}

	if (useFP16)
	{
		auto size = sizeof(uint16_t) * filterWeightsFP16.size();
		filterWeightsOut.resize(size);
		memcpy(filterWeightsOut.data(), filterWeightsFP16.data(), size);

		size = sizeof(uint16_t) * biasWeightsFP16.size();
		biasWeightsOut.resize(size);
		memcpy(biasWeightsOut.data(), biasWeightsFP16.data(), size);
	}
	else
	{
		auto size = sizeof(float) * filterWeightsFP32.size();
		filterWeightsOut.resize(size);
		memcpy(filterWeightsOut.data(), filterWeightsFP32.data(), size);

		size = sizeof(float) * biasWeightsFP32.size();
		biasWeightsOut.resize(size);
		memcpy(biasWeightsOut.data(), biasWeightsFP32.data(), size);
	}
}

void Util::GetStrides(const uint32_t sizes[4], TensorLayout layout, uint32_t stridesOut[4])
{
	switch (layout)
	{
	case TensorLayout::NHWC:
		stridesOut[0] = sizes[1] * sizes[2] * sizes[3];
		stridesOut[1] = 1;
		stridesOut[2] = sizes[1] * sizes[3];
		stridesOut[3] = sizes[1];
		break;

	default:
		stridesOut[0] = sizes[1] * sizes[2] * sizes[3];
		stridesOut[1] = sizes[2] * sizes[3];
		stridesOut[2] = sizes[3];
		stridesOut[3] = 1;
	}
}
