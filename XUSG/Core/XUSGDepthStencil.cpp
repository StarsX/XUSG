//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSG.h"

using namespace std;
using namespace XUSG;
using namespace Graphics;

Graphics::DepthStencil PipelineLib::DepthStencilDefault()
{
	DepthStencil depthStencil;
	depthStencil.DepthEnable = true;
	depthStencil.DepthWriteMask = true;
	depthStencil.Comparison = ComparisonFunc::LESS;
	depthStencil.StencilEnable = false;
	depthStencil.DepthBoundsTestEnable = false;

	const DepthStencilOp defaultStencilOp =
	{ StencilOp::KEEP, StencilOp::KEEP, StencilOp::KEEP, ComparisonFunc::ALWAYS, 0xff, 0xff };
	depthStencil.FrontFace = defaultStencilOp;
	depthStencil.BackFace = defaultStencilOp;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthStencilNone()
{
	auto depthStencil = DepthStencilDefault();
	depthStencil.DepthEnable = false;
	depthStencil.DepthWriteMask = false;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthNever()
{
	return DepthReadNever();
}

Graphics::DepthStencil PipelineLib::DepthLess()
{
	return DepthStencilDefault();
}

Graphics::DepthStencil PipelineLib::DepthEqual()
{
	auto depthStencil = DepthStencilDefault();
	depthStencil.Comparison = ComparisonFunc::EQUAL;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthLessEqual()
{
	auto depthStencil = DepthStencilDefault();
	depthStencil.Comparison = ComparisonFunc::LESS_EQUAL;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthGreater()
{
	auto depthStencil = DepthStencilDefault();
	depthStencil.Comparison = ComparisonFunc::GREATER;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthNotEqual()
{
	auto depthStencil = DepthStencilDefault();
	depthStencil.Comparison = ComparisonFunc::NOT_EQUAL;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthGreaterEqual()
{
	auto depthStencil = DepthStencilDefault();
	depthStencil.Comparison = ComparisonFunc::GREATER_EQUAL;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthAlways()
{
	auto depthStencil = DepthStencilDefault();
	depthStencil.Comparison = ComparisonFunc::ALWAYS;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthReadNever()
{
	auto depthStencil = DepthStencilDefault();
	depthStencil.DepthWriteMask = false;
	depthStencil.Comparison = ComparisonFunc::NEVER;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthReadLess()
{
	auto depthStencil = DepthLess();
	depthStencil.DepthWriteMask = false;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthReadEqual()
{
	auto depthStencil = DepthEqual();
	depthStencil.DepthWriteMask = false;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthReadLessEqual()
{
	auto depthStencil = DepthLessEqual();
	depthStencil.DepthWriteMask = false;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthReadGreater()
{
	auto depthStencil = DepthGreater();
	depthStencil.DepthWriteMask = false;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthReadNotEqual()
{
	auto depthStencil = DepthNotEqual();
	depthStencil.DepthWriteMask = false;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthReadGreaterEqual()
{
	auto depthStencil = DepthGreaterEqual();
	depthStencil.DepthWriteMask = false;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthReadAlways()
{
	auto depthStencil = DepthAlways();
	depthStencil.DepthWriteMask = false;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthNever_StencilZero()
{
	auto depthStencil = DepthNever();
	depthStencil.StencilEnable = true;
	depthStencil.BackFace.StencilPassOp = depthStencil.FrontFace.StencilPassOp = StencilOp::ZERO;
	depthStencil.BackFace.StencilFunc = depthStencil.FrontFace.StencilFunc = ComparisonFunc::ALWAYS;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthLess_StencilZero()
{
	auto depthStencil = DepthLess();
	depthStencil.StencilEnable = true;
	depthStencil.BackFace.StencilPassOp = depthStencil.FrontFace.StencilPassOp = StencilOp::ZERO;
	depthStencil.BackFace.StencilFunc = depthStencil.FrontFace.StencilFunc = ComparisonFunc::ALWAYS;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthEqual_StencilZero()
{
	auto depthStencil = DepthEqual();
	depthStencil.StencilEnable = true;
	depthStencil.BackFace.StencilPassOp = depthStencil.FrontFace.StencilPassOp = StencilOp::ZERO;
	depthStencil.BackFace.StencilFunc = depthStencil.FrontFace.StencilFunc = ComparisonFunc::ALWAYS;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthLessEqual_StencilZero()
{
	auto depthStencil = DepthLessEqual();
	depthStencil.StencilEnable = true;
	depthStencil.BackFace.StencilPassOp = depthStencil.FrontFace.StencilPassOp = StencilOp::ZERO;
	depthStencil.BackFace.StencilFunc = depthStencil.FrontFace.StencilFunc = ComparisonFunc::ALWAYS;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthGreater_StencilZero()
{
	auto depthStencil = DepthGreater();
	depthStencil.StencilEnable = true;
	depthStencil.BackFace.StencilPassOp = depthStencil.FrontFace.StencilPassOp = StencilOp::ZERO;
	depthStencil.BackFace.StencilFunc = depthStencil.FrontFace.StencilFunc = ComparisonFunc::ALWAYS;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthNotEqual_StencilZero()
{
	auto depthStencil = DepthNotEqual();
	depthStencil.StencilEnable = true;
	depthStencil.BackFace.StencilPassOp = depthStencil.FrontFace.StencilPassOp = StencilOp::ZERO;
	depthStencil.BackFace.StencilFunc = depthStencil.FrontFace.StencilFunc = ComparisonFunc::ALWAYS;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthGreaterEqual_StencilZero()
{
	auto depthStencil = DepthGreaterEqual();
	depthStencil.StencilEnable = true;
	depthStencil.BackFace.StencilPassOp = depthStencil.FrontFace.StencilPassOp = StencilOp::ZERO;
	depthStencil.BackFace.StencilFunc = depthStencil.FrontFace.StencilFunc = ComparisonFunc::ALWAYS;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthAlways_StencilZero()
{
	auto depthStencil = DepthAlways();
	depthStencil.StencilEnable = true;
	depthStencil.BackFace.StencilPassOp = depthStencil.FrontFace.StencilPassOp = StencilOp::ZERO;
	depthStencil.BackFace.StencilFunc = depthStencil.FrontFace.StencilFunc = ComparisonFunc::ALWAYS;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthNever_StencilReplace()
{
	auto depthStencil = DepthNever();
	depthStencil.StencilEnable = true;
	depthStencil.BackFace.StencilPassOp = depthStencil.FrontFace.StencilPassOp = StencilOp::REPLACE;
	depthStencil.BackFace.StencilFunc = depthStencil.FrontFace.StencilFunc = ComparisonFunc::ALWAYS;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthLess_StencilReplace()
{
	auto depthStencil = DepthLess();
	depthStencil.StencilEnable = true;
	depthStencil.BackFace.StencilPassOp = depthStencil.FrontFace.StencilPassOp = StencilOp::REPLACE;
	depthStencil.BackFace.StencilFunc = depthStencil.FrontFace.StencilFunc = ComparisonFunc::ALWAYS;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthEqual_StencilReplace()
{
	auto depthStencil = DepthEqual();
	depthStencil.StencilEnable = true;
	depthStencil.BackFace.StencilPassOp = depthStencil.FrontFace.StencilPassOp = StencilOp::REPLACE;
	depthStencil.BackFace.StencilFunc = depthStencil.FrontFace.StencilFunc = ComparisonFunc::ALWAYS;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthLessEqual_StencilReplace()
{
	auto depthStencil = DepthLessEqual();
	depthStencil.StencilEnable = true;
	depthStencil.BackFace.StencilPassOp = depthStencil.FrontFace.StencilPassOp = StencilOp::REPLACE;
	depthStencil.BackFace.StencilFunc = depthStencil.FrontFace.StencilFunc = ComparisonFunc::ALWAYS;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthGreater_StencilReplace()
{
	auto depthStencil = DepthGreater();
	depthStencil.StencilEnable = true;
	depthStencil.BackFace.StencilPassOp = depthStencil.FrontFace.StencilPassOp = StencilOp::REPLACE;
	depthStencil.BackFace.StencilFunc = depthStencil.FrontFace.StencilFunc = ComparisonFunc::ALWAYS;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthNotEqual_StencilReplace()
{
	auto depthStencil = DepthNotEqual();
	depthStencil.StencilEnable = true;
	depthStencil.BackFace.StencilPassOp = depthStencil.FrontFace.StencilPassOp = StencilOp::REPLACE;
	depthStencil.BackFace.StencilFunc = depthStencil.FrontFace.StencilFunc = ComparisonFunc::ALWAYS;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthGreaterEqual_StencilReplace()
{
	auto depthStencil = DepthGreaterEqual();
	depthStencil.StencilEnable = true;
	depthStencil.BackFace.StencilPassOp = depthStencil.FrontFace.StencilPassOp = StencilOp::REPLACE;
	depthStencil.BackFace.StencilFunc = depthStencil.FrontFace.StencilFunc = ComparisonFunc::ALWAYS;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthAlways_StencilReplace()
{
	auto depthStencil = DepthAlways();
	depthStencil.StencilEnable = true;
	depthStencil.BackFace.StencilPassOp = depthStencil.FrontFace.StencilPassOp = StencilOp::REPLACE;
	depthStencil.BackFace.StencilFunc = depthStencil.FrontFace.StencilFunc = ComparisonFunc::ALWAYS;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthNever_StencilEqual()
{
	auto depthStencil = DepthNever();
	depthStencil.StencilEnable = true;
	depthStencil.BackFace.StencilFunc = depthStencil.FrontFace.StencilFunc = ComparisonFunc::EQUAL;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthLess_StencilEqual()
{
	auto depthStencil = DepthLess();
	depthStencil.StencilEnable = true;
	depthStencil.BackFace.StencilFunc = depthStencil.FrontFace.StencilFunc = ComparisonFunc::EQUAL;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthEqual_StencilEqual()
{
	auto depthStencil = DepthEqual();
	depthStencil.StencilEnable = true;
	depthStencil.BackFace.StencilFunc = depthStencil.FrontFace.StencilFunc = ComparisonFunc::EQUAL;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthLessEqual_StencilEqual()
{
	auto depthStencil = DepthLessEqual();
	depthStencil.StencilEnable = true;
	depthStencil.BackFace.StencilFunc = depthStencil.FrontFace.StencilFunc = ComparisonFunc::EQUAL;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthGreater_StencilEqual()
{
	auto depthStencil = DepthGreater();
	depthStencil.StencilEnable = true;
	depthStencil.BackFace.StencilFunc = depthStencil.FrontFace.StencilFunc = ComparisonFunc::EQUAL;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthNotEqual_StencilEqual()
{
	auto depthStencil = DepthNotEqual();
	depthStencil.StencilEnable = true;
	depthStencil.BackFace.StencilFunc = depthStencil.FrontFace.StencilFunc = ComparisonFunc::EQUAL;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthGreaterEqual_StencilEqual()
{
	auto depthStencil = DepthGreaterEqual();
	depthStencil.StencilEnable = true;
	depthStencil.BackFace.StencilFunc = depthStencil.FrontFace.StencilFunc = ComparisonFunc::EQUAL;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthAlways_StencilEqual()
{
	auto depthStencil = DepthAlways();
	depthStencil.StencilEnable = true;
	depthStencil.BackFace.StencilFunc = depthStencil.FrontFace.StencilFunc = ComparisonFunc::EQUAL;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthNever_StencilNotEqual()
{
	auto depthStencil = DepthNever();
	depthStencil.StencilEnable = true;
	depthStencil.BackFace.StencilFunc = depthStencil.FrontFace.StencilFunc = ComparisonFunc::NOT_EQUAL;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthLess_StencilNotEqual()
{
	auto depthStencil = DepthLess();
	depthStencil.StencilEnable = true;
	depthStencil.BackFace.StencilFunc = depthStencil.FrontFace.StencilFunc = ComparisonFunc::NOT_EQUAL;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthEqual_StencilNotEqual()
{
	auto depthStencil = DepthEqual();
	depthStencil.StencilEnable = true;
	depthStencil.BackFace.StencilFunc = depthStencil.FrontFace.StencilFunc = ComparisonFunc::NOT_EQUAL;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthLessEqual_StencilNotEqual()
{
	auto depthStencil = DepthLessEqual();
	depthStencil.StencilEnable = true;
	depthStencil.BackFace.StencilFunc = depthStencil.FrontFace.StencilFunc = ComparisonFunc::NOT_EQUAL;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthGreater_StencilNotEqual()
{
	auto depthStencil = DepthGreater();
	depthStencil.StencilEnable = true;
	depthStencil.BackFace.StencilFunc = depthStencil.FrontFace.StencilFunc = ComparisonFunc::NOT_EQUAL;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthNotEqual_StencilNotEqual()
{
	auto depthStencil = DepthNotEqual();
	depthStencil.StencilEnable = true;
	depthStencil.BackFace.StencilFunc = depthStencil.FrontFace.StencilFunc = ComparisonFunc::NOT_EQUAL;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthGreaterEqual_StencilNotEqual()
{
	auto depthStencil = DepthGreaterEqual();
	depthStencil.StencilEnable = true;
	depthStencil.BackFace.StencilFunc = depthStencil.FrontFace.StencilFunc = ComparisonFunc::NOT_EQUAL;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::DepthAlways_StencilNotEqual()
{
	auto depthStencil = DepthAlways();
	depthStencil.StencilEnable = true;
	depthStencil.BackFace.StencilFunc = depthStencil.FrontFace.StencilFunc = ComparisonFunc::NOT_EQUAL;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::StencilFrontIncrBackDecr()
{
	auto depthStencil = DepthStencilDefault();
	depthStencil.StencilEnable = true;
	depthStencil.FrontFace.StencilPassOp = StencilOp::INCR;
	depthStencil.BackFace.StencilPassOp = StencilOp::DECR;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::StencilFrontDecrBackIncr()
{
	auto depthStencil = DepthStencilDefault();
	depthStencil.StencilEnable = true;
	depthStencil.FrontFace.StencilPassOp = StencilOp::DECR;
	depthStencil.BackFace.StencilPassOp = StencilOp::INCR;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::StencilPassZero()
{
	auto depthStencil = DepthReadAlways();
	depthStencil.StencilEnable = true;
	depthStencil.BackFace.StencilPassOp = depthStencil.FrontFace.StencilPassOp = StencilOp::ZERO;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::StencilFailZero()
{
	auto depthStencil = DepthReadAlways();
	depthStencil.StencilEnable = true;
	depthStencil.BackFace.StencilFailOp = depthStencil.FrontFace.StencilFailOp = StencilOp::ZERO;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::StencilPassInvert()
{
	auto depthStencil = DepthReadAlways();
	depthStencil.StencilEnable = true;
	depthStencil.BackFace.StencilPassOp = depthStencil.FrontFace.StencilPassOp = StencilOp::INVERT;

	return depthStencil;
}

Graphics::DepthStencil PipelineLib::StencilFailInvert()
{
	auto depthStencil = DepthReadAlways();
	depthStencil.StencilEnable = true;
	depthStencil.BackFace.StencilFailOp = depthStencil.FrontFace.StencilFailOp = StencilOp::INVERT;

	return depthStencil;
}
