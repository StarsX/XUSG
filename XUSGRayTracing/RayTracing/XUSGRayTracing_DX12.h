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

			uint32_t Create(void* pAdapter, uint32_t minFeatureLevel, const wchar_t* name = nullptr);
			uint32_t Create(void* pAdapter, uint32_t minFeatureLevel, uint8_t flags, const wchar_t* name = nullptr);
			void Create(void* pNativeHandle);

			void* GetRTHandle() const;
			void* GetRTBackend();

		protected:
			bool createInterface(uint8_t flags = 0);

			com_ptr<ID3D12RaytracingFallbackDevice> m_deviceRT;
		};

		Device_DX12* AsDX12(Device* p);

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS GetDXRBuildFlag(BuildFlag buildFlag);
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS GetDXRBuildFlags(BuildFlag buildFlags);

		D3D12_RAYTRACING_GEOMETRY_FLAGS GetDXRGeometryFlag(GeometryFlag geometryFlag);
		D3D12_RAYTRACING_GEOMETRY_FLAGS GetDXRGeometryFlags(GeometryFlag geometryFlags);

		D3D12_RAYTRACING_INSTANCE_FLAGS GetDXRInstanceFlag(InstanceFlag instanceFlag);
		D3D12_RAYTRACING_INSTANCE_FLAGS GetDXRInstanceFlags(InstanceFlag instanceFlags);
	}
}
