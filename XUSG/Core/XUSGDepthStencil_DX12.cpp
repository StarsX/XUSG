//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGGraphicsState_DX12.h"

using namespace std;
using namespace XUSG;
using namespace Graphics;

Graphics::DepthStencil PipelineLib_DX12::DepthStencilDefault()
{
	DepthStencil depthStencil;
	depthStencil.DepthEnable = true;
	depthStencil.DepthWriteMask = true;
	depthStencil.Comparison = ComparisonFunc::LESS;
	depthStencil.StencilEnable = false;
	depthStencil.StencilReadMask = 0xff;
	depthStencil.StencilWriteMask = 0xff;

	const DepthStencilOp defaultStencilOp =
	{ StencilOp::KEEP, StencilOp::KEEP, StencilOp::KEEP, ComparisonFunc::ALWAYS };
	depthStencil.FrontFace = defaultStencilOp;
	depthStencil.BackFace = defaultStencilOp;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib_DX12::DepthStencilNone()
{
	auto depthStencil = DepthStencilDefault();
	depthStencil.DepthEnable = false;
	depthStencil.DepthWriteMask = false;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib_DX12::DepthRead()
{
	auto depthStencil = DepthStencilDefault();
	depthStencil.DepthWriteMask = false;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib_DX12::DepthReadLessEqual()
{
	auto depthStencil = DepthStencilDefault();
	depthStencil.DepthWriteMask = false;
	depthStencil.Comparison = ComparisonFunc::LESS_EQUAL;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib_DX12::DepthReadEqual()
{
	auto depthStencil = DepthStencilDefault();
	depthStencil.DepthWriteMask = false;
	depthStencil.Comparison = ComparisonFunc::EQUAL;

	return depthStencil;
}
