//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Core/XUSG_VK.h"
#include "XUSGRayTracing.h"

namespace XUSG
{
	namespace RayTracing
	{
		class Device_VK :
			public virtual Device,
			public virtual XUSG::Device_VK
		{
		public:
			Device_VK();
			virtual ~Device_VK();

			uint32_t Create(void* pAdapter, uint32_t minFeatureLevel, const wchar_t* name = nullptr);
			uint32_t Create(void* pAdapter, uint32_t minFeatureLevel, uint8_t flags, const wchar_t* name = nullptr);
			void Create(void* pNativeHandle);

			void* GetRTHandle() const;
			void* GetRTBackend();

		protected:
			bool createInterface(uint8_t flags = 0);
		};

		Device_VK* AsVK(Device* p);

		VkBuildAccelerationStructureFlagsKHR GetVKBuildFlag(BuildFlag buildFlag);
		VkBuildAccelerationStructureFlagsKHR GetVKBuildFlags(BuildFlag buildFlags);

		VkGeometryFlagsKHR GetVKGeometryFlag(GeometryFlag geometryFlag);
		VkGeometryFlagsKHR GetVKGeometryFlags(GeometryFlag geometryFlags);

		VkGeometryInstanceFlagsKHR GetVKInstanceFlag(InstanceFlag instanceFlag);
		VkGeometryInstanceFlagsKHR GetVKInstanceFlags(InstanceFlag instanceFlags);
	}
}
