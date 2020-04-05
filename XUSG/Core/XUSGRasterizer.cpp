//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGGraphicsState.h"

using namespace std;
using namespace XUSG;
using namespace Graphics;

Rasterizer PipelineCache_DX12::RasterizerDefault()
{
	return make_shared<D3D12_RASTERIZER_DESC>(CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT));
}

Rasterizer PipelineCache_DX12::CullBack()
{
	return RasterizerDefault();
}

Rasterizer PipelineCache_DX12::CullNone()
{
	const auto rasterizer = RasterizerDefault();
	rasterizer->CullMode = D3D12_CULL_MODE_NONE;

	return rasterizer;
}

Rasterizer PipelineCache_DX12::CullFront()
{
	const auto rasterizer = RasterizerDefault();
	rasterizer->CullMode = D3D12_CULL_MODE_FRONT;

	return rasterizer;
}

Rasterizer PipelineCache_DX12::CullWireframe()
{
	const auto rasterizer = RasterizerDefault();
	rasterizer->FillMode = D3D12_FILL_MODE_WIREFRAME;

	return rasterizer;
}
