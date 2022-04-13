//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSGCommand_DX12.h"
#include "RayTracing/XUSGRayTracingCommand_DX12.h"
#include "XUSG-EZ_DX12.h"
#include "XUSGRayTracing-EZ_DX12.h"

using namespace std;
using namespace XUSG::RayTracing;
using namespace XUSG::RayTracing::EZ;

EZ::CommandList::uptr EZ::CommandList::MakeUnique(API api)
{
	return make_unique<CommandList_DXR>();
}

EZ::CommandList::sptr EZ::CommandList::MakeShared(API api)
{
	return make_shared<CommandList_DXR>();
}

EZ::CommandList::uptr EZ::CommandList::MakeUnique(RayTracing::CommandList* pCommandList,
	uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize, uint32_t maxSamplers,
	const uint32_t* pMaxCbvsEachSpace, const uint32_t* pMaxSrvsEachSpace,
	const uint32_t* pMaxUavsEachSpace, uint32_t maxCbvSpaces,
	uint32_t maxSrvSpaces, uint32_t maxUavSpaces, API api)
{
	return make_unique<CommandList_DXR>(pCommandList, samplerPoolSize, cbvSrvUavPoolSize,
		maxSamplers, pMaxCbvsEachSpace, pMaxSrvsEachSpace, pMaxUavsEachSpace,
		maxCbvSpaces, maxSrvSpaces, maxUavSpaces);
}

EZ::CommandList::sptr EZ::CommandList::MakeShared(RayTracing::CommandList* pCommandList,
	uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize, uint32_t maxSamplers,
	const uint32_t* pMaxCbvsEachSpace, const uint32_t* pMaxSrvsEachSpace,
	const uint32_t* pMaxUavsEachSpace, uint32_t maxCbvSpaces,
	uint32_t maxSrvSpaces, uint32_t maxUavSpaces, API api)
{
	return make_shared<CommandList_DXR>(pCommandList, samplerPoolSize, cbvSrvUavPoolSize,
		maxSamplers, pMaxCbvsEachSpace, pMaxSrvsEachSpace, pMaxUavsEachSpace,
		maxCbvSpaces, maxSrvSpaces, maxUavSpaces);
}
