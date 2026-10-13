//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Core/XUSGPipelineLayout_VK.h"
#include "XUSGRayTracing.h"

namespace XUSG
{
	namespace RayTracing
	{
		class PipelineLayout_VK :
			public virtual PipelineLayout,
			public virtual Util::PipelineLayout_VK
		{
		public:
			PipelineLayout_VK();
			virtual ~PipelineLayout_VK();

			XUSG::PipelineLayout CreatePipelineLayout(Device* pDevice, PipelineLayoutLib* pPipelineLayoutLib,
				PipelineLayoutFlag flags, const wchar_t* name = nullptr);
			XUSG::PipelineLayout GetPipelineLayout(Device* pDevice, PipelineLayoutLib* pPipelineLayoutLib,
				PipelineLayoutFlag flags, const wchar_t* name = nullptr);
		};
	}
}
