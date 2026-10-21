//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSG_VK.h"
#include "XUSGRayTracing_VK.h"
#include "XUSGShaderTable_VK.h"

using namespace std;
using namespace XUSG::RayTracing;

Device_VK::Device_VK()
{
}

Device_VK::~Device_VK()
{
}

uint32_t Device_VK::Create(void* pAdapter, uint32_t minFeatureLevel, const wchar_t* name)
{
	return Create(pAdapter, minFeatureLevel, 0, name);
}

uint32_t Device_VK::Create(void* pAdapter, uint32_t minFeatureLevel, uint8_t flags, const wchar_t* name)
{
	const auto result = XUSG::Device_VK::Create(pAdapter, minFeatureLevel, name);
	if (result < 0) return result;

	return createInterface(flags) ? result : static_cast<uint32_t>(VK_ERROR_INITIALIZATION_FAILED);
}

void Device_VK::Create(void* pNativeHandle)
{
	XUSG::Device_VK::Create(pNativeHandle);
	createInterface();
}

void* Device_VK::GetRTHandle() const
{
	return GetDevice();
}

void* Device_VK::GetRTBackend()
{
	return this;
}

bool Device_VK::createInterface(uint8_t flags)
{
	(void)flags;
	if (!HasAccelerationStructure() && !HasRayTracingPipeline())
	{
		OutputDebugString(L"Vulkan ray tracing is not enabled on this device.\n");

		return false;
	}

	const auto& props = GetRayTracingPipelineProperties();
	CacheShaderRecordAlignment(props.shaderGroupHandleSize, props.shaderGroupHandleAlignment, props.shaderGroupBaseAlignment);

	return true;
}

VkBuildAccelerationStructureFlagsKHR XUSG::RayTracing::GetVKBuildFlag(BuildFlag buildFlag)
{
	static const VkBuildAccelerationStructureFlagsKHR buildFlags[] =
	{
		VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR,
		VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR,
		VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
		VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR,
		VK_BUILD_ACCELERATION_STRUCTURE_LOW_MEMORY_BIT_KHR,
		0,
		VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_OPACITY_MICROMAP_UPDATE_BIT_KHR,
		VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_DISABLE_OPACITY_MICROMAPS_BIT_KHR
	};

	if (buildFlag == BuildFlag::NONE) return 0;

	const auto index = XUSG::Log2(static_cast<uint32_t>(buildFlag));

	return buildFlags[index];
}

VkBuildAccelerationStructureFlagsKHR XUSG::RayTracing::GetVKBuildFlags(BuildFlag buildFlags)
{
	auto flags = VkBuildAccelerationStructureFlagsKHR(0);
	if (static_cast<bool>(buildFlags & BuildFlag::ALLOW_UPDATE))
		flags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
	if (static_cast<bool>(buildFlags & BuildFlag::ALLOW_COMPACTION))
		flags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
	if (static_cast<bool>(buildFlags & BuildFlag::PREFER_FAST_TRACE))
		flags |= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	if (static_cast<bool>(buildFlags & BuildFlag::PREFER_FAST_BUILD))
		flags |= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
	if (static_cast<bool>(buildFlags & BuildFlag::MINIMIZE_MEMORY))
		flags |= VK_BUILD_ACCELERATION_STRUCTURE_LOW_MEMORY_BIT_KHR;
	if (static_cast<bool>(buildFlags & BuildFlag::ALLOW_OMM_UPDATE))
		flags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_OPACITY_MICROMAP_UPDATE_BIT_KHR;
	if (static_cast<bool>(buildFlags & BuildFlag::ALLOW_DISABLE_OMMS))
		flags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_DISABLE_OPACITY_MICROMAPS_BIT_KHR;

	return flags;
}

VkGeometryFlagsKHR XUSG::RayTracing::GetVKGeometryFlag(GeometryFlag geometryFlag)
{
	static const VkGeometryFlagsKHR geometryFlags[] =
	{
		VK_GEOMETRY_OPAQUE_BIT_KHR,
		VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR
	};

	if (geometryFlag == GeometryFlag::NONE) return 0;

	const auto index = XUSG::Log2(static_cast<uint32_t>(geometryFlag));

	return geometryFlags[index];
}

VkGeometryFlagsKHR XUSG::RayTracing::GetVKGeometryFlags(GeometryFlag geometryFlags)
{
	auto flags = VkGeometryFlagsKHR(0);
	if (static_cast<bool>(geometryFlags & GeometryFlag::FULL_OPAQUE))
		flags |= VK_GEOMETRY_OPAQUE_BIT_KHR;
	if (static_cast<bool>(geometryFlags & GeometryFlag::NO_DUPLICATE_ANYHIT_INVOCATION))
		flags |= VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR;

	return flags;
}

VkGeometryInstanceFlagsKHR XUSG::RayTracing::GetVKInstanceFlag(InstanceFlag instanceFlag)
{
	static const VkGeometryInstanceFlagsKHR instanceFlags[] =
	{
		VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
		VK_GEOMETRY_INSTANCE_TRIANGLE_FRONT_COUNTERCLOCKWISE_BIT_KHR,
		VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR,
		VK_GEOMETRY_INSTANCE_FORCE_NO_OPAQUE_BIT_KHR,
		VK_GEOMETRY_INSTANCE_FORCE_OPACITY_MICROMAP_2_STATE_BIT_KHR,
		VK_GEOMETRY_INSTANCE_DISABLE_OPACITY_MICROMAPS_BIT_KHR
	};

	if (instanceFlag == InstanceFlag::NONE) return 0;

	const auto index = XUSG::Log2(static_cast<uint32_t>(instanceFlag));

	return instanceFlags[index];
}

VkGeometryInstanceFlagsKHR XUSG::RayTracing::GetVKInstanceFlags(InstanceFlag instanceFlags)
{
	auto flags = VkGeometryInstanceFlagsKHR(0);
	if (static_cast<bool>(instanceFlags & InstanceFlag::TRIANGLE_CULL_DISABLE))
		flags |= VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
	if (static_cast<bool>(instanceFlags & InstanceFlag::TRIANGLE_FRONT_COUNTERCLOCKWISE))
		flags |= VK_GEOMETRY_INSTANCE_TRIANGLE_FRONT_COUNTERCLOCKWISE_BIT_KHR;
	if (static_cast<bool>(instanceFlags & InstanceFlag::FORCE_OPAQUE))
		flags |= VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR;
	if (static_cast<bool>(instanceFlags & InstanceFlag::FORCE_NON_OPAQUE))
		flags |= VK_GEOMETRY_INSTANCE_FORCE_NO_OPAQUE_BIT_KHR;
	if (static_cast<bool>(instanceFlags & InstanceFlag::FORCE_OMM_2_STATE))
		flags |= VK_GEOMETRY_INSTANCE_FORCE_OPACITY_MICROMAP_2_STATE_BIT_KHR;
	if (static_cast<bool>(instanceFlags & InstanceFlag::DISABLE_OMMS))
		flags |= VK_GEOMETRY_INSTANCE_DISABLE_OPACITY_MICROMAPS_BIT_KHR;

	return flags;
}
