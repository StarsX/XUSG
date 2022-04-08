//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSG-EZ_DX12.h"
#include "XUSG-EZ.h"
#include "RayTracing/XUSGRayTracingCommand_DX12.h"

namespace XUSG
{
	namespace EZ
	{
		namespace RayTracing
		{
			class CommandList_DXR :
				virtual public CommandList,
				virtual public XUSG::RayTracing::CommandList_DX12,
				virtual public EZ::CommandList_DX12
			{
			public:
				CommandList_DXR();
				CommandList_DXR(XUSG::RayTracing::CommandList* pCommandList, uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
					uint32_t maxSamplers = 16, const uint32_t* pMaxCbvsEachSpace = nullptr, const uint32_t* pMaxSrvsEachSpace = nullptr,
					const uint32_t* pMaxUavsEachSpace = nullptr, uint32_t maxCbvSpaces = 1, uint32_t maxSrvSpaces = 1, uint32_t maxUavSpaces = 1,
					uint32_t maxTLASSrvs = 0, uint32_t spaceTLAS = 0);
				virtual ~CommandList_DXR();

				bool Create(XUSG::RayTracing::CommandList* pCommandList, uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
					uint32_t maxSamplers = 16, const uint32_t* pMaxCbvsEachSpace = nullptr, const uint32_t* pMaxSrvsEachSpace = nullptr,
					const uint32_t* pMaxUavsEachSpace = nullptr, uint32_t maxCbvSpaces = 1, uint32_t maxSrvSpaces = 1, uint32_t maxUavSpaces = 1,
					uint32_t maxTLASSrvs = 0, uint32_t spaceTLAS = 0);

				bool Close();

				bool PreBuildBLAS(XUSG::RayTracing::BottomLevelAS* pBLAS, uint32_t numGeometries, const XUSG::RayTracing::GeometryBuffer& geometries,
					XUSG::RayTracing::BuildFlag flags = XUSG::RayTracing::BuildFlag::PREFER_FAST_TRACE);
				bool PreBuildTLAS(XUSG::RayTracing::TopLevelAS* pTLAS, uint32_t numGeometries, XUSG::RayTracing::BuildFlag flags);

				bool BuildBLAS(XUSG::RayTracing::BottomLevelAS* pBLAS, bool update = false);
				bool BuildTLAS(XUSG::RayTracing::TopLevelAS* pTLAS, const Resource* pInstanceDescs, bool update = false);

			protected:
				XUSG::Resource* needScratch(uint32_t size);

				bool createPipelineLayouts(uint32_t maxSamplers, const uint32_t* pMaxCbvsEachSpace,
					const uint32_t* pMaxSrvsEachSpace, const uint32_t* pMaxUavsEachSpace,
					uint32_t maxCbvSpaces, uint32_t maxSrvSpaces, uint32_t maxUavSpaces,
					uint32_t maxTLASSrvs, uint32_t spaceTLAS);

			protected:
				uint32_t m_asUavCount;
				uint32_t m_paramIndex;
				std::vector<uint32_t> m_tlasBindingToParamIndexMap;

				uint32_t m_scratchSize;
				std::vector<XUSG::Resource::uptr> m_scratches;
			};
		}
	}
}
