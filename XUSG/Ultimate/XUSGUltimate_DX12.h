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

			void SetSamplePositions(uint8_t numSamplesPerPixel, uint8_t numPixels, SamplePosition* pPositions) const;
			void ResolveSubresourceRegion(const Resource& dstResource, uint32_t dstSubresource,
				uint32_t dstX, uint32_t dstY, const Resource& srcResource, uint32_t srcSubresource,
				RectRange* pSrcRect, Format format, ResolveMode resolveMode) const;

			void RSSetShadingRate(ShadingRate baseShadingRate, const ShadingRateCombiner* pCombiners) const;
			void RSSetShadingRateImage(const Resource& shadingRateImage) const;
			void DispatchMesh(
				uint32_t ThreadGroupCountX,
				uint32_t ThreadGroupCountY,
				uint32_t ThreadGroupCountZ) const;

			com_ptr<ID3D12GraphicsCommandList6>& GetGraphicsCommandList();

		protected:
			com_ptr<ID3D12GraphicsCommandList6> m_commandListU;
		};

		D3D12_SHADING_RATE_COMBINER GetDX12ShadingRateCombiner(ShadingRateCombiner combiner);
		D3D12_RESOLVE_MODE GetDX12ResolveMode(ResolveMode mode);
	}
}
