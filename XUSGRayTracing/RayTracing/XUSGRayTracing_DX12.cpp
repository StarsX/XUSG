//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSG_DX12.h"
#include "XUSGRayTracing_DX12.h"

using namespace std;
using namespace XUSG::RayTracing;

#define APPEND_FLAG(type, dx12Type, flags, flag, none) ((flags & type::flag) == type::flag ? dx12Type##_##flag : dx12Type##_##none)
#define APPEND_BUILD_FLAG(flags, flag) APPEND_FLAG(BuildFlag, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG, flags, flag, NONE)
#define APPEND_GEOMETRY_FLAG(flags, flag) APPEND_FLAG(GeometryFlag, D3D12_RAYTRACING_GEOMETRY_FLAG, flags, flag, NONE)

Device_DX12::Device_DX12()
{
}

Device_DX12::~Device_DX12()
{
}

#if ENABLE_DXR_FALLBACK
bool Device_DX12::CreateInterface(uint8_t flags)
{
	const auto hr = D3D12CreateRaytracingFallbackDevice(m_device.get(), flags, 0, IID_PPV_ARGS(&m_deviceRT));
	if (FAILED(hr))
	{
		OutputDebugString(L"Couldn't get DirectX Raytracing Fallback interface for the device.\n");

		return false;
	}

	return true;
}
#else
bool Device_DX12::CreateInterface()
{
	const auto hr = m_device->QueryInterface(IID_PPV_ARGS(&m_deviceRT));
	if (FAILED(hr))
	{
		OutputDebugString(L"Couldn't get DirectX Raytracing interface for the device.\n");

		return false;
	}

	return true;
}
#endif

void* Device_DX12::GetRTHandle() const
{
	return m_deviceRT.get();
}

D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS XUSG::RayTracing::GetDXRBuildFlag(BuildFlag buildFlag)
{
	static const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags[] =
	{
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE,
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION,
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE,
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD,
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_MINIMIZE_MEMORY,
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE
	};

	if (buildFlag == BuildFlag::NONE) return D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

	const auto index = XUSG::Log2(static_cast<uint32_t>(buildFlag));

	return buildFlags[index];
}

D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS XUSG::RayTracing::GetDXRBuildFlags(BuildFlag buildFlags)
{
	auto flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
	flags |= APPEND_BUILD_FLAG(buildFlags, ALLOW_UPDATE);
	flags |= APPEND_BUILD_FLAG(buildFlags, ALLOW_COMPACTION);
	flags |= APPEND_BUILD_FLAG(buildFlags, PREFER_FAST_TRACE);
	flags |= APPEND_BUILD_FLAG(buildFlags, PREFER_FAST_BUILD);
	flags |= APPEND_BUILD_FLAG(buildFlags, MINIMIZE_MEMORY);
	flags |= APPEND_BUILD_FLAG(buildFlags, PERFORM_UPDATE);

	return flags;
}

D3D12_RAYTRACING_GEOMETRY_FLAGS XUSG::RayTracing::GetDXRGeometryFlag(GeometryFlag geometryFlag)
{
	static const D3D12_RAYTRACING_GEOMETRY_FLAGS geometryFlags[] =
	{
		D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE,
		D3D12_RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION
	};

	if (geometryFlag == GeometryFlag::NONE) return D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;

	const auto index = XUSG::Log2(static_cast<uint32_t>(geometryFlag));

	return geometryFlags[index];
}

D3D12_RAYTRACING_GEOMETRY_FLAGS XUSG::RayTracing::GetDXRGeometryFlags(GeometryFlag geometryFlags)
{
	auto flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
	flags |= (geometryFlags & GeometryFlag::FULL_OPAQUE) == GeometryFlag::FULL_OPAQUE ?
		D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE : D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
	flags |= APPEND_GEOMETRY_FLAG(geometryFlags, NO_DUPLICATE_ANYHIT_INVOCATION);

	return flags;
}
