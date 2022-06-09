//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSGCommand_DX12.h"
#include "Core/XUSGResource_DX12.h"
#include "Ultimate/XUSGUltimate_DX12.h"
#include "RayTracing/XUSGRayTracingCommand_DX12.h"
#include "XUSG-EZ_DX12.h"
#include "XUSGUltimate-EZ_DX12.h"
#include "XUSGRayTracing-EZ_DX12.h"

using namespace std;
using namespace XUSG::RayTracing;
using namespace XUSG::RayTracing::EZ;

XUSG::EZ::ResourceView EZ::GetSRV(AccelerationStructure* pAS)
{
	const auto pResource = pAS->GetResult().get();

	XUSG::EZ::ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.View = pResource->GetSRV();
	resourceView.Subresources = { XUSG_BARRIER_ALL_SUBRESOURCES };
	resourceView.DstState = XUSG::ResourceState::RAYTRACING_ACCELERATION_STRUCTURE;

	return resourceView;
}

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
