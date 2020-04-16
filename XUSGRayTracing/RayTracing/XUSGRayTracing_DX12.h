//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Core/XUSG.h"

namespace XUSG
{
	namespace RayTracing
	{
#if ENABLE_DXR_FALLBACK
		enum class API
		{
			FallbackLayer,
			NativeRaytracing,
		};
#endif

		struct Device
		{
#if ENABLE_DXR_FALLBACK
			API RaytracingAPI;
#endif
			XUSG::Device Common;
#if ENABLE_DXR_FALLBACK
			com_ptr<ID3D12RaytracingFallbackDevice> Fallback;
#endif
			com_ptr<ID3D12Device5> Native;
		};

		struct Pipeline
		{
#if ENABLE_DXR_FALLBACK
			com_ptr<ID3D12RaytracingFallbackStateObject> Fallback;
#endif
			com_ptr<ID3D12StateObject> Native;
		};

		enum class BuildFlags
		{
			NONE				= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE,
			ALLOW_UPDATE		= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE,
			ALLOW_COMPACTION	= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION,
			PREFER_FAST_TRACE	= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE,
			PREFER_FAST_BUILD	= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD,
			MINIMIZE_MEMORY		= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_MINIMIZE_MEMORY,
			PERFORM_UPDATE		= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE
		};

		DEFINE_ENUM_FLAG_OPERATORS(BuildFlags);

		enum class GeometryFlags
		{
			NONE = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE,
			FULL_OPAQUE	= D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE,
			NO_DUPLICATE_ANYHIT_INVOCATION = D3D12_RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION
		};

		DEFINE_ENUM_FLAG_OPERATORS(GeometryFlags);

		enum class HitGroupType : uint8_t
		{
			TRIANGLES = D3D12_HIT_GROUP_TYPE_TRIANGLES,
			PROCEDURAL = D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE
		};

#if ENABLE_DXR_FALLBACK
		using FallbackCommandList = com_ptr<ID3D12RaytracingFallbackCommandList>;
#endif
		using NativeCommandList = com_ptr<ID3D12GraphicsCommandList4>;

		using BuildDesc = D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC;
		using PrebuildInfo = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO;
		using PostbuildInfo = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC;

		using Geometry = D3D12_RAYTRACING_GEOMETRY_DESC;
		using PipilineDesc = CD3D12_STATE_OBJECT_DESC;

		class CommandList;
	}
}
