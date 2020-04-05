//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGGraphicsState.h"

using namespace std;
using namespace XUSG;
using namespace Graphics;

Graphics::DepthStencil PipelineCache_DX12::DepthStencilDefault()
{
	return make_shared<D3D12_DEPTH_STENCIL_DESC>(CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT));
}

Graphics::DepthStencil PipelineCache_DX12::DepthStencilNone()
{
	const auto depthStencil = DepthStencilDefault();
	depthStencil->DepthEnable = FALSE;
	depthStencil->DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

	return depthStencil;
}

Graphics::DepthStencil PipelineCache_DX12::DepthRead()
{
	const auto depthStencil = DepthStencilDefault();
	depthStencil->DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

	return depthStencil;
}

Graphics::DepthStencil PipelineCache_DX12::DepthReadLessEqual()
{
	const auto depthStencil = DepthStencilDefault();
	depthStencil->DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	depthStencil->DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	return depthStencil;
}

Graphics::DepthStencil PipelineCache_DX12::DepthReadEqual()
{
	const auto depthStencil = DepthStencilDefault();
	depthStencil->DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	depthStencil->DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;

	return depthStencil;
}
