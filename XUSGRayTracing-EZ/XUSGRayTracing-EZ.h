//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSG-EZ.h"
#include "RayTracing/XUSGRayTracing.h"

namespace XUSG
{
	namespace RayTracing
	{
		namespace EZ
		{
			//--------------------------------------------------------------------------------------
			// RayTracing Command list
			//--------------------------------------------------------------------------------------
			class XUSG_INTERFACE CommandList :
				public virtual XUSG::EZ::CommandList
			{
			public:
				//CommandList();
				virtual ~CommandList() {}

				using uptr = std::unique_ptr<CommandList>;
				using sptr = std::shared_ptr<CommandList>;

				virtual bool Create(XUSG::RayTracing::CommandList* pCommandList, uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
					uint32_t maxSamplers = 16, const uint32_t* pMaxCbvsEachSpace = nullptr, const uint32_t* pMaxSrvsEachSpace = nullptr,
					const uint32_t* pMaxUavsEachSpace = nullptr, uint32_t maxCbvSpaces = 1, uint32_t maxSrvSpaces = 1, uint32_t maxUavSpaces = 1,
					uint32_t maxTLASSrvs = 0, uint32_t spaceTLAS = 0) = 0;
				virtual bool Create(const XUSG::RayTracing::Device* pDevice, void* pHandle, uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
					uint32_t maxSamplers = 16, const uint32_t* pMaxCbvsEachSpace = nullptr, const uint32_t* pMaxSrvsEachSpace = nullptr,
					const uint32_t* pMaxUavsEachSpace = nullptr, uint32_t maxCbvSpaces = 1, uint32_t maxSrvSpaces = 1, uint32_t maxUavSpaces = 1,
					const wchar_t* name = nullptr) = 0;
				virtual bool Close() = 0;
				virtual bool PreBuildBLAS(XUSG::RayTracing::BottomLevelAS* pBLAS, uint32_t numGeometries, const XUSG::RayTracing::GeometryBuffer& geometries,
					XUSG::RayTracing::BuildFlag flags = XUSG::RayTracing::BuildFlag::PREFER_FAST_TRACE) = 0;
				virtual bool PreBuildTLAS(XUSG::RayTracing::TopLevelAS* pTLAS, uint32_t numInstances, XUSG::RayTracing::BuildFlag flags) = 0;

				virtual void BuildBLAS(XUSG::RayTracing::BottomLevelAS* pBLAS, bool update = false) = 0;
				virtual void BuildTLAS(XUSG::RayTracing::TopLevelAS* pTLAS, const Resource* pInstanceDescs, bool update = false) = 0;
				virtual void SetTopLevelAccelerationStructure(uint32_t binding, const XUSG::RayTracing::TopLevelAS* pTopLevelAS) const = 0;
				virtual void RTSetShaderLibrary(Blob shaderLib) = 0;
				virtual void RTSetHitGroup(uint32_t index, const void* hitGroup, const void* closestHitShader,
					const void* anyHitShader = nullptr, const void* intersectionShader = nullptr,
					XUSG::RayTracing::HitGroupType type = XUSG::RayTracing::HitGroupType::TRIANGLES) = 0;
				virtual void RTSetShaderConfig(uint32_t maxPayloadSize, uint32_t maxAttributeSize = sizeof(float[2])) = 0;
				virtual void RTSetMaxRecursionDepth(uint32_t depth) = 0;
				virtual void DispatchRays(uint32_t width, uint32_t height, uint32_t depth,
					const void* rayGenShader, const void* missShader) = 0;

				static uptr MakeUnique(XUSG::API api = XUSG::API::DIRECTX_12);
				static sptr MakeShared(XUSG::API api = XUSG::API::DIRECTX_12);
				static uptr MakeUnique(XUSG::RayTracing::CommandList* pCommandList, uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
					uint32_t maxSamplers = 16, const uint32_t* pMaxCbvsEachSpace = nullptr, const uint32_t* pMaxSrvsEachSpace = nullptr,
					const uint32_t* pMaxUavsEachSpace = nullptr, uint32_t maxCbvSpaces = 1, uint32_t maxSrvSpaces = 1, uint32_t maxUavSpaces = 1,
					XUSG::API api = XUSG::API::DIRECTX_12);
				static sptr MakeShared(XUSG::RayTracing::CommandList* pCommandList, uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
					uint32_t maxSamplers = 16, const uint32_t* pMaxCbvsEachSpace = nullptr, const uint32_t* pMaxSrvsEachSpace = nullptr,
					const uint32_t* pMaxUavsEachSpace = nullptr, uint32_t maxCbvSpaces = 1, uint32_t maxSrvSpaces = 1, uint32_t maxUavSpaces = 1,
					XUSG::API api = XUSG::API::DIRECTX_12);
			};
		}
	}
}
