//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGUltimate.h"

using namespace std;
using namespace XUSG;
using namespace Ultimate;

Ultimate::DepthStencil PipelineLib::DepthStencilDefault()
{
	DepthStencil depthStencil;
	depthStencil.DepthEnable = true;
	depthStencil.DepthWriteMask = true;
	depthStencil.Comparison = ComparisonFunc::LESS;
	depthStencil.StencilEnable = false;

	const DepthStencilOp defaultStencilOp =
	{ StencilOp::KEEP, StencilOp::KEEP, StencilOp::KEEP, ComparisonFunc::ALWAYS, 0xff, 0xff };
	depthStencil.FrontFace = defaultStencilOp;
	depthStencil.BackFace = defaultStencilOp;

	return depthStencil;
}

Ultimate::DepthStencil PipelineLib::DepthStencilNone()
{
	auto depthStencil = DepthStencilDefault();
	depthStencil.DepthEnable = false;
	depthStencil.DepthWriteMask = false;

	return depthStencil;
}

Ultimate::DepthStencil PipelineLib::DepthRead()
{
	auto depthStencil = DepthStencilDefault();
	depthStencil.DepthWriteMask = false;

	return depthStencil;
}

Ultimate::DepthStencil PipelineLib::DepthReadLessEqual()
{
	auto depthStencil = DepthStencilDefault();
	depthStencil.DepthWriteMask = false;
	depthStencil.Comparison = ComparisonFunc::LESS_EQUAL;

	return depthStencil;
}

Ultimate::DepthStencil PipelineLib::DepthReadEqual()
{
	auto depthStencil = DepthStencilDefault();
	depthStencil.DepthWriteMask = false;
	depthStencil.Comparison = ComparisonFunc::EQUAL;

	return depthStencil;
}
