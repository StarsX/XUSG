//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGAdvanced.h"

using namespace XUSG;

uint32_t XUSG::GetCBVSpace()
{
	return Device::GetApi() == API::VULKAN ? XUSG_VK_SPACE_CBV : 0;
}

uint32_t XUSG::GetSRVSpace()
{
	return Device::GetApi() == API::VULKAN ? XUSG_VK_SPACE_SRV : 0;
}

uint32_t XUSG::GetUAVSpace()
{
	return Device::GetApi() == API::VULKAN ? XUSG_VK_SPACE_UAV : 0;
}

uint32_t XUSG::GetSamplerSpace()
{
	return Device::GetApi() == API::VULKAN ? XUSG_VK_SPACE_SAMPLER : 0;
}
