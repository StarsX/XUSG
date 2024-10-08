//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSGCommand_DX12.h"
#include "Core/XUSGResource_DX12.h"
#include "Core/XUSGPipelineLayout_DX12.h"
#include "Ultimate/XUSGUltimate_DX12.h"
#include "RayTracing/XUSGRayTracingCommand_DX12.h"
#include "XUSG-EZ_DX12.h"
#include "XUSGUltimate-EZ_DX12.h"
#include "XUSGRayTracing-EZ_DX12.h"

using namespace std;
using namespace XUSG::RayTracing;
using namespace XUSG::RayTracing::EZ;

XUSG::EZ::ResourceView EZ::GetSRV(TopLevelAS* pTLAS)
{
	XUSG::EZ::ResourceView resourceView;
	resourceView.pResource = nullptr;
	resourceView.View = pTLAS->GetSRV();
	resourceView.Subresources = {};
	resourceView.DstState = XUSG::ResourceState::RAYTRACING_ACCELERATION_STRUCTURE;
	resourceView.pCounter = nullptr;

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
	uint32_t samplerHeapSize, uint32_t cbvSrvUavHeapSize, API api)
{
	return make_unique<CommandList_DXR>(pCommandList, samplerHeapSize, cbvSrvUavHeapSize);
}

EZ::CommandList::sptr EZ::CommandList::MakeShared(RayTracing::CommandList* pCommandList,
	uint32_t samplerHeapSize, uint32_t cbvSrvUavHeapSize, API api)
{
	return make_shared<CommandList_DXR>(pCommandList, samplerHeapSize, cbvSrvUavHeapSize);
}
