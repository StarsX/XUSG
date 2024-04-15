//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSG.h"

using namespace std;
using namespace XUSG;
using namespace Graphics;

Rasterizer PipelineLib::RasterizerDefault()
{
	Rasterizer rasterizer;
	rasterizer.Fill = FillMode::SOLID;
	rasterizer.Cull = CullMode::BACK;
	rasterizer.FrontCounterClockwise = false;
	rasterizer.DepthBias = 0;
	rasterizer.DepthBiasClamp = 0.0f;
	rasterizer.SlopeScaledDepthBias = 0.0f;
	rasterizer.DepthClipEnable = true;
	rasterizer.LineRasterizationMode = LineRasterization::ALIASED;
	rasterizer.ForcedSampleCount = 0;
	rasterizer.ConservativeRaster = false;

	return rasterizer;
}

Rasterizer PipelineLib::CullBack()
{
	return RasterizerDefault();
}

Rasterizer PipelineLib::CullNone()
{
	auto rasterizer = RasterizerDefault();
	rasterizer.Cull = CullMode::NONE;

	return rasterizer;
}

Rasterizer PipelineLib::CullFront()
{
	auto rasterizer = RasterizerDefault();
	rasterizer.Cull = CullMode::FRONT;

	return rasterizer;
}

Rasterizer PipelineLib::FillWireframe()
{
	auto rasterizer = RasterizerDefault();
	rasterizer.Fill = FillMode::WIREFRAME;

	return rasterizer;
}
