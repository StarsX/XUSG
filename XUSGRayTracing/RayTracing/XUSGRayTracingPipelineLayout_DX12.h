//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSGRayTracing.h"

namespace XUSG
{
	namespace RayTracing
	{
		class PipelineLayout_DX12 :
			public virtual PipelineLayout,
			public virtual Util::PipelineLayout_DX12
		{
		public:
			PipelineLayout_DX12();
			virtual ~PipelineLayout_DX12();

			XUSG::PipelineLayout CreatePipelineLayout(const Device* pDevice, PipelineLayoutCache* pPipelineLayoutCache,
				PipelineLayoutFlag flags, const wchar_t* name = nullptr);
			XUSG::PipelineLayout GetPipelineLayout(const Device* pDevice, PipelineLayoutCache* pPipelineLayoutCache,
				PipelineLayoutFlag flags, const wchar_t* name = nullptr);

		protected:
			void convertToRootParam1_0(D3D12_ROOT_PARAMETER& rootParam,
				std::vector<D3D12_DESCRIPTOR_RANGE>& descriptorRanges,
				const D3D12_ROOT_PARAMETER1& rootParam1);
		};
	}
}
