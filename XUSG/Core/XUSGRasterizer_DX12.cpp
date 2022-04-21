//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGGraphicsState_DX12.h"

using namespace std;
using namespace XUSG;
using namespace Graphics;

Rasterizer PipelineLib_DX12::RasterizerDefault()
{
	Rasterizer rasterizer;
	rasterizer.Fill = FillMode::SOLID;
	rasterizer.Cull = CullMode::BACK;
	rasterizer.FrontCounterClockwise = false;
	rasterizer.DepthBias = 0;
	rasterizer.DepthBiasClamp = 0.0f;
	rasterizer.SlopeScaledDepthBias = 0.0f;
	rasterizer.DepthClipEnable = true;
	rasterizer.MultisampleEnable = false;
	rasterizer.AntialiasedLineEnable = false;
	rasterizer.ForcedSampleCount = 0;
	rasterizer.ConservativeRaster = false;

	return rasterizer;
}

Rasterizer PipelineLib_DX12::CullBack()
{
	return RasterizerDefault();
}

Rasterizer PipelineLib_DX12::CullNone()
{
	auto rasterizer = RasterizerDefault();
	rasterizer.Cull = CullMode::NONE;

	return rasterizer;
}

Rasterizer PipelineLib_DX12::CullFront()
{
	auto rasterizer = RasterizerDefault();
	rasterizer.Cull = CullMode::FRONT;

	return rasterizer;
}

Rasterizer PipelineLib_DX12::FillWireframe()
{
	auto rasterizer = RasterizerDefault();
	rasterizer.Fill = FillMode::WIREFRAME;

	return rasterizer;
}
