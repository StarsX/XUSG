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

			bool CreateInterface(uint8_t flags = 0);

			void* GetRTHandle() const;
		};

		VkBuildAccelerationStructureFlagsKHR GetVKBuildFlag(BuildFlag buildFlag);
		VkBuildAccelerationStructureFlagsKHR GetVKBuildFlags(BuildFlag buildFlags);

		VkGeometryFlagsKHR GetVKGeometryFlag(GeometryFlag geometryFlag);
		VkGeometryFlagsKHR GetVKGeometryFlags(GeometryFlag geometryFlags);

		VkGeometryInstanceFlagsKHR GetVKInstanceFlag(InstanceFlag instanceFlag);
		VkGeometryInstanceFlagsKHR GetVKInstanceFlags(InstanceFlag instanceFlags);
	}
}
