//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSGCommand_DX12.h"
#include "Core/XUSGResource_DX12.h"
#include "Core/XUSGPipelineLayout_DX12.h"
#include "Ultimate/XUSGUltimate_DX12.h"
#include "RayTracing/XUSGRayTracingCommand_DX12.h"
#include "XUSG-EZ_Impl.h"
#include "XUSGUltimate-EZ_Impl.h"
#include "XUSGRayTracing-EZ_Impl.h"

using namespace std;
using namespace XUSG::RayTracing;
using namespace XUSG::RayTracing::EZ;

XUSG::EZ::ResourceView EZ::GetSRV(const TopLevelAS* pTLAS)
{
	XUSG::EZ::ResourceView resourceView;
	resourceView.View = pTLAS->GetSRV();
	resourceView.Subresources = {};
	resourceView.DstState = XUSG::ResourceState::RAYTRACING_ACCELERATION_STRUCTURE;

	return resourceView;
}

EZ::CommandList::uptr EZ::CommandList::MakeUnique()
{
	XUSG::Device::LockApi();
#ifndef XUSG_NO_VULKAN
	if (XUSG::Device::GetApi() == API::VULKAN) return make_unique<CommandList_VK>();
#endif

	return make_unique<CommandList_DX12>();
}

EZ::CommandList::sptr EZ::CommandList::MakeShared()
{
	XUSG::Device::LockApi();
#ifndef XUSG_NO_VULKAN
	if (XUSG::Device::GetApi() == API::VULKAN) return make_shared<CommandList_VK>();
#endif

	return make_shared<CommandList_DX12>();
}

EZ::CommandList::uptr EZ::CommandList::MakeUnique(RayTracing::CommandList* pCommandList,
	uint32_t samplerHeapSize, uint32_t cbvSrvUavHeapSize)
{
	XUSG::Device::LockApi();
#ifndef XUSG_NO_VULKAN
	if (XUSG::Device::GetApi() == API::VULKAN)
		return make_unique<CommandList_VK>(pCommandList, samplerHeapSize, cbvSrvUavHeapSize);
#endif

	return make_unique<CommandList_DX12>(pCommandList, samplerHeapSize, cbvSrvUavHeapSize);
}

EZ::CommandList::sptr EZ::CommandList::MakeShared(RayTracing::CommandList* pCommandList,
	uint32_t samplerHeapSize, uint32_t cbvSrvUavHeapSize)
{
	XUSG::Device::LockApi();
#ifndef XUSG_NO_VULKAN
	if (XUSG::Device::GetApi() == API::VULKAN)
		return make_shared<CommandList_VK>(pCommandList, samplerHeapSize, cbvSrvUavHeapSize);
#endif

	return make_shared<CommandList_DX12>(pCommandList, samplerHeapSize, cbvSrvUavHeapSize);
}
