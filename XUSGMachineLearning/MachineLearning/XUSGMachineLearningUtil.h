//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "MachineLearning/XUSGMachineLearning.h"

namespace XUSG
{
	namespace ML
	{
		class Util_Impl :
			public Util
		{
		public:
			Util_Impl(TensorDataType tensorDataType = TensorDataType::FLOAT32,
				TensorLayout tensorLayout = TensorLayout::DEFAULT,
				XUSG::API api = XUSG::API::DIRECTX_12);
			virtual ~Util_Impl();

			bool CreateUpsampleLayer(const Device* pDevice, const uint32_t inputSizes[4],
				uint64_t& inputBufferRequiredSize, uint64_t& outputBufferRequiredSize,
				uint32_t outputSizes[4], Operator& opOut, uint32_t scaleSizeX = 2, uint32_t scaleSizeY = 2,
				InterpolationType interpolationType = InterpolationType::NEAREST_NEIGHBOR);
			bool CreateConvolutionLayer(const Device* pDevice, const uint32_t inputSizes[4],
				const uint32_t* filterSizes, bool useBiasAndActivation, uint64_t& inputBufferRequiredSize,
				uint64_t& outputBufferRequiredSize, uint32_t outputSizes[4], Operator& opOut);
			bool CreateAdditionLayer(const Device* pDevice, const uint32_t inputSizes[4], Operator& opOut);
			
			void CreateWeightTensors(WeightMapType& weights, const char* convLayerName, const char* scaleLayerName,
				const char* shiftLayerName, const uint32_t filterSizes[4], std::vector<uint8_t>& filterWeightsOut,
				std::vector<uint8_t>& biasWeightsOut);

		protected:
			TensorDataType	m_tensorDataType;
			TensorLayout	m_tensorLayout;

			XUSG::API		m_api;
		};
	}
}
