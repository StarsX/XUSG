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

			bool CreateInterface(uint8_t flags = 0);

			void* GetRTHandle() const;

		protected:
			com_ptr<ID3D12RaytracingFallbackDevice> m_deviceRT;
		};

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS GetDXRBuildFlag(BuildFlag buildFlag);
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS GetDXRBuildFlags(BuildFlag buildFlags);

		D3D12_RAYTRACING_GEOMETRY_FLAGS GetDXRGeometryFlag(GeometryFlag geometryFlag);
		D3D12_RAYTRACING_GEOMETRY_FLAGS GetDXRGeometryFlags(GeometryFlag geometryFlags);

		D3D12_RAYTRACING_INSTANCE_FLAGS GetDXRInstanceFlag(InstanceFlag instanceFlag);
		D3D12_RAYTRACING_INSTANCE_FLAGS GetDXRInstanceFlags(InstanceFlag instanceFlags);
	}
}
