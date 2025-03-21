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
	rasterizer.DepthBias = 0.0f;
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

Rasterizer PipelineLib::CullBack_Conservative()
{
	auto rasterizer = CullBack();
	rasterizer.ConservativeRaster = true;

	return rasterizer;
}

Rasterizer PipelineLib::CullNone_Conservative()
{
	auto rasterizer = CullNone();
	rasterizer.ConservativeRaster = true;

	return rasterizer;
}

Rasterizer PipelineLib::CullFront_Conservative()
{
	auto rasterizer = CullFront();
	rasterizer.ConservativeRaster = true;

	return rasterizer;
}
