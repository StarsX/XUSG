//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Core/XUSG.h"

namespace XUSG
{
	namespace RayTracing
	{
		struct Device
		{
			operator const XUSG::Device& () const { return Base; }
			operator XUSG::Device& () { return Base; }
			XUSG::Device Base;
#if ENABLE_DXR_FALLBACK
			com_ptr<ID3D12RaytracingFallbackDevice> Derived;
#else
			com_ptr<ID3D12Device5> Derived;
#endif
		};

#if ENABLE_DXR_FALLBACK
		using Pipeline = com_ptr<ID3D12RaytracingFallbackStateObject>;
#else
		using Pipeline = com_ptr<ID3D12StateObject>;
#endif

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

		using BuildDesc = D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC;
		using PrebuildInfo = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO;
		using PostbuildInfo = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC;

		using Geometry = D3D12_RAYTRACING_GEOMETRY_DESC;
		using PipilineDesc = CD3D12_STATE_OBJECT_DESC;

		class CommandList;
	}
}
