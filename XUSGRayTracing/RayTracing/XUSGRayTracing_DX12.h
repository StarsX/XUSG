//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSGRayTracing.h"

namespace XUSG
{
	namespace RayTracing
	{
		class Device_DX12 :
			public virtual Device,
			public virtual XUSG::Device_DX12
		{
		public:
			Device_DX12();
			virtual ~Device_DX12();

#if ENABLE_DXR_FALLBACK
			bool CreateInterface(uint8_t flags);
#else
			bool CreateInterface();
#endif

			void* GetRTHandle() const;

		protected:
#if ENABLE_DXR_FALLBACK
			com_ptr<ID3D12RaytracingFallbackDevice> m_deviceRT;
#else
			com_ptr<ID3D12Device5> m_deviceRT;
#endif
		};
	}
}
