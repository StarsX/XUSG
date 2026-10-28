//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSG_VK.h"
#include "Core/XUSGPipelineLayout_VK.h"
#include "XUSGRayTracingPipelineLayout_VK.h"

using namespace std;
using namespace XUSG;
using namespace XUSG::RayTracing;

RayTracing::PipelineLayout_VK::PipelineLayout_VK() :
	Util::PipelineLayout_VK()
{
}

RayTracing::PipelineLayout_VK::~PipelineLayout_VK()
{
}

XUSG::PipelineLayout RayTracing::PipelineLayout_VK::CreatePipelineLayout(Device* pDevice,
	PipelineLayoutLib* pPipelineLayoutLib, PipelineLayoutFlag flags, const wchar_t* name)
{
	assert(pDevice);

	return Util::PipelineLayout_VK::CreatePipelineLayout(pPipelineLayoutLib, flags, name);
}

XUSG::PipelineLayout RayTracing::PipelineLayout_VK::GetPipelineLayout(Device* pDevice,
	PipelineLayoutLib* pPipelineLayoutLib, PipelineLayoutFlag flags, const wchar_t* name)
{
	assert(pDevice);

	return Util::PipelineLayout_VK::GetPipelineLayout(pPipelineLayoutLib, flags, name);
}
