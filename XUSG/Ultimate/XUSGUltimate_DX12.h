//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSGUltimate.h"

namespace XUSG
{
	namespace Ultimate
	{
		class CommandList_DX12 :
			public virtual CommandList,
			public virtual XUSG::CommandList_DX12
		{
		public:
			CommandList_DX12();
			CommandList_DX12(XUSG::CommandList& commandList);
			~CommandList_DX12();

			bool CreateInterface();

			void SetSamplePositions(uint8_t numSamplesPerPixel, uint8_t numPixels, SamplePosition* pPositions);
			void RSSetShadingRate(ShadingRate baseShadingRate,
				const ShadingRateCombiner* pCombiners);
			void RSSetShadingRateImage(const Resource& shadingRateImage);
			void DispatchMesh(
				uint32_t ThreadGroupCountX,
				uint32_t ThreadGroupCountY,
				uint32_t ThreadGroupCountZ);

			com_ptr<ID3D12GraphicsCommandList6>& GetGraphicsCommandList();

		protected:
			com_ptr<ID3D12GraphicsCommandList6> m_commandListU;
		};
	}
}
