//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGGraphicsState_DX12.h"

using namespace std;
using namespace XUSG;
using namespace Graphics;

Blend PipelineLib_DX12::DefaultOpaque(uint8_t n)
{
	Blend blend;
	blend.AlphaToCoverageEnable = false;
	blend.IndependentBlendEnable = false;

	for (auto& renderTarget : blend.RenderTargets)
	{
		renderTarget.BlendEnable = false;
		renderTarget.LogicOpEnable = false;
		renderTarget.SrcBlend = BlendFactor::ONE;
		renderTarget.DestBlend = BlendFactor::ZERO;
		renderTarget.BlendOp = BlendOperator::ADD;
		renderTarget.SrcBlendAlpha = BlendFactor::ONE;
		renderTarget.DestBlendAlpha = BlendFactor::ZERO;
		renderTarget.BlendOpAlpha = BlendOperator::ADD;
		renderTarget.LogicOp = LogicOperator::NOOP;
		renderTarget.WriteMask = ColorWrite::ALL;
	}

	return blend;
}

Blend PipelineLib_DX12::Premultiplied(uint8_t n)
{
	Blend blend;
	blend.AlphaToCoverageEnable = false;
	blend.IndependentBlendEnable = false;

	auto& pRenderTarget = blend.RenderTargets;
	pRenderTarget->BlendEnable = true;
	pRenderTarget->LogicOpEnable = false;
	pRenderTarget->SrcBlend = BlendFactor::ONE;
	pRenderTarget->DestBlend = BlendFactor::INV_SRC_ALPHA;
	pRenderTarget->BlendOp = BlendOperator::ADD;
	pRenderTarget->SrcBlendAlpha = BlendFactor::ONE;
	pRenderTarget->DestBlendAlpha = BlendFactor::INV_SRC_ALPHA;
	pRenderTarget->BlendOpAlpha = BlendOperator::ADD;
	pRenderTarget->LogicOp = LogicOperator::NOOP;
	pRenderTarget->WriteMask = ColorWrite::ALL;

	return blend;
}

Blend PipelineLib_DX12::Additive(uint8_t n)
{
	Blend blend;
	blend.AlphaToCoverageEnable = false;
	blend.IndependentBlendEnable = false;

	auto& pRenderTarget = blend.RenderTargets;
	pRenderTarget->BlendEnable = true;
	pRenderTarget->LogicOpEnable = false;
	pRenderTarget->SrcBlend = BlendFactor::SRC_ALPHA;
	pRenderTarget->DestBlend = BlendFactor::ONE;
	pRenderTarget->BlendOp = BlendOperator::ADD;
	pRenderTarget->SrcBlendAlpha = BlendFactor::SRC_ALPHA;
	pRenderTarget->DestBlendAlpha = BlendFactor::ONE;
	pRenderTarget->BlendOpAlpha = BlendOperator::ADD;
	pRenderTarget->LogicOp = LogicOperator::NOOP;
	pRenderTarget->WriteMask = ColorWrite::ALL;

	return blend;
}

Blend PipelineLib_DX12::NonPremultiplied(uint8_t n)
{
	Blend blend;
	blend.AlphaToCoverageEnable = false;
	blend.IndependentBlendEnable = false;

	auto& pRenderTarget = blend.RenderTargets;
	pRenderTarget->BlendEnable = true;
	pRenderTarget->LogicOpEnable = false;
	pRenderTarget->SrcBlend = BlendFactor::SRC_ALPHA;
	pRenderTarget->DestBlend = BlendFactor::INV_SRC_ALPHA;
	pRenderTarget->BlendOp = BlendOperator::ADD;
	pRenderTarget->SrcBlendAlpha = BlendFactor::SRC_ALPHA;
	pRenderTarget->DestBlendAlpha = BlendFactor::INV_SRC_ALPHA;
	pRenderTarget->BlendOpAlpha = BlendOperator::ADD;
	pRenderTarget->LogicOp = LogicOperator::NOOP;
	pRenderTarget->WriteMask = ColorWrite::ALL;

	return blend;
}

Blend PipelineLib_DX12::NonPremultipliedRT0(uint8_t n)
{
	Blend blend;
	blend.AlphaToCoverageEnable = false;
	blend.IndependentBlendEnable = true;

	auto& renderTarget = blend.RenderTargets[0];
	renderTarget.BlendEnable = true;
	renderTarget.LogicOpEnable = false;
	renderTarget.SrcBlend = BlendFactor::SRC_ALPHA;
	renderTarget.DestBlend = BlendFactor::INV_SRC_ALPHA;
	renderTarget.BlendOp = BlendOperator::ADD;
	renderTarget.SrcBlendAlpha = BlendFactor::SRC_ALPHA;
	renderTarget.DestBlendAlpha = BlendFactor::INV_SRC_ALPHA;
	renderTarget.BlendOpAlpha = BlendOperator::ADD;
	renderTarget.LogicOp = LogicOperator::NOOP;
	renderTarget.WriteMask = ColorWrite::ALL;

	// Default
	for (uint8_t i = 1; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
	{
		auto& renderTarget = blend.RenderTargets[i];
		renderTarget.BlendEnable = false;
		renderTarget.LogicOpEnable = false;
		renderTarget.SrcBlend = BlendFactor::ONE;
		renderTarget.DestBlend = BlendFactor::ZERO;
		renderTarget.BlendOp = BlendOperator::ADD;
		renderTarget.SrcBlendAlpha = BlendFactor::ONE;
		renderTarget.DestBlendAlpha = BlendFactor::ZERO;
		renderTarget.BlendOpAlpha = BlendOperator::ADD;
		renderTarget.LogicOp = LogicOperator::NOOP;
		renderTarget.WriteMask = ColorWrite::ALL;
	}

	return blend;
}

Blend PipelineLib_DX12::AlphaToCoverage(uint8_t n)
{
	Blend blend;
	blend.AlphaToCoverageEnable = true;
	blend.IndependentBlendEnable = false;

	auto& pRenderTarget = blend.RenderTargets;
	pRenderTarget->BlendEnable = FALSE;
	pRenderTarget->LogicOpEnable = false;
	pRenderTarget->SrcBlend = BlendFactor::SRC_ALPHA;
	pRenderTarget->DestBlend = BlendFactor::INV_SRC_ALPHA;
	pRenderTarget->BlendOp = BlendOperator::ADD;
	pRenderTarget->SrcBlendAlpha = BlendFactor::SRC_ALPHA;
	pRenderTarget->DestBlendAlpha = BlendFactor::INV_SRC_ALPHA;
	pRenderTarget->BlendOpAlpha = BlendOperator::ADD;
	pRenderTarget->LogicOp = LogicOperator::NOOP;
	pRenderTarget->WriteMask = ColorWrite::ALL;

	return blend;
}

Blend PipelineLib_DX12::Accumulative(uint8_t n)
{
	Blend blend;
	blend.AlphaToCoverageEnable = false;
	blend.IndependentBlendEnable = false;

	auto& pRenderTarget = blend.RenderTargets;
	pRenderTarget->BlendEnable = true;
	pRenderTarget->LogicOpEnable = false;
	pRenderTarget->SrcBlend = BlendFactor::ONE;
	pRenderTarget->DestBlend = BlendFactor::ONE;
	pRenderTarget->BlendOp = BlendOperator::ADD;
	pRenderTarget->SrcBlendAlpha = BlendFactor::ONE;
	pRenderTarget->DestBlendAlpha = BlendFactor::ONE;
	pRenderTarget->BlendOpAlpha = BlendOperator::ADD;
	pRenderTarget->LogicOp = LogicOperator::NOOP;
	pRenderTarget->WriteMask = ColorWrite::ALL;

	return blend;
}

Blend PipelineLib_DX12::AutoNonPremultiplied(uint8_t n)
{
	Blend blend;
	blend.AlphaToCoverageEnable = false;
	blend.IndependentBlendEnable = false;

	auto& pRenderTarget = blend.RenderTargets;
	pRenderTarget->BlendEnable = true;
	pRenderTarget->LogicOpEnable = false;
	pRenderTarget->SrcBlend = BlendFactor::SRC_ALPHA;
	pRenderTarget->DestBlend = BlendFactor::INV_SRC_ALPHA;
	pRenderTarget->BlendOp = BlendOperator::ADD;
	pRenderTarget->SrcBlendAlpha = BlendFactor::ONE;
	pRenderTarget->DestBlendAlpha = BlendFactor::INV_SRC_ALPHA;
	pRenderTarget->BlendOpAlpha = BlendOperator::ADD;
	pRenderTarget->LogicOp = LogicOperator::NOOP;
	pRenderTarget->WriteMask = ColorWrite::ALL;

	return blend;
}

Blend PipelineLib_DX12::ZeroAlphaNonPremultiplied(uint8_t n)
{
	Blend blend;
	blend.AlphaToCoverageEnable = false;
	blend.IndependentBlendEnable = false;

	auto& pRenderTarget = blend.RenderTargets;
	pRenderTarget->BlendEnable = true;
	pRenderTarget->LogicOpEnable = false;
	pRenderTarget->SrcBlend = BlendFactor::SRC_ALPHA;
	pRenderTarget->DestBlend = BlendFactor::INV_SRC_ALPHA;
	pRenderTarget->BlendOp = BlendOperator::ADD;
	pRenderTarget->SrcBlendAlpha = BlendFactor::ZERO;
	pRenderTarget->DestBlendAlpha = BlendFactor::ZERO;
	pRenderTarget->BlendOpAlpha = BlendOperator::ADD;
	pRenderTarget->LogicOp = LogicOperator::NOOP;
	pRenderTarget->WriteMask = ColorWrite::ALL;

	return blend;
}

Blend PipelineLib_DX12::Multiplied(uint8_t n)
{
	Blend blend;
	blend.AlphaToCoverageEnable = false;
	blend.IndependentBlendEnable = false;

	auto& pRenderTarget = blend.RenderTargets;
	pRenderTarget->BlendEnable = true;
	pRenderTarget->LogicOpEnable = false;
	pRenderTarget->SrcBlend = BlendFactor::ZERO;
	pRenderTarget->DestBlend = BlendFactor::SRC_COLOR;
	pRenderTarget->BlendOp = BlendOperator::ADD;
	pRenderTarget->SrcBlendAlpha = BlendFactor::ZERO;
	pRenderTarget->DestBlendAlpha = BlendFactor::SRC_ALPHA;
	pRenderTarget->BlendOpAlpha = BlendOperator::ADD;
	pRenderTarget->LogicOp = LogicOperator::NOOP;
	pRenderTarget->WriteMask = ColorWrite::ALL;

	return blend;
}

Blend PipelineLib_DX12::WeightedPremul(uint8_t n)
{
	Blend blend;
	blend.AlphaToCoverageEnable = false;
	blend.IndependentBlendEnable = false;

	auto& pRenderTarget = blend.RenderTargets;
	pRenderTarget->BlendEnable = true;
	pRenderTarget->LogicOpEnable = false;
	pRenderTarget->SrcBlend = BlendFactor::ONE;
	pRenderTarget->DestBlend = BlendFactor::ONE;
	pRenderTarget->BlendOp = BlendOperator::ADD;
	pRenderTarget->SrcBlendAlpha = BlendFactor::ONE;
	pRenderTarget->DestBlendAlpha = BlendFactor::ONE;
	pRenderTarget->BlendOpAlpha = BlendOperator::ADD;
	pRenderTarget->LogicOp = LogicOperator::NOOP;
	pRenderTarget->WriteMask = ColorWrite::ALL;

	return blend;
}

Blend PipelineLib_DX12::WeightedPremulPerRT(uint8_t n)
{
	Blend blend;
	blend.AlphaToCoverageEnable = false;
	blend.IndependentBlendEnable = true;

	assert(n < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT);

	// Accumulations
	for (uint8_t i = 0; i < n; ++i)
	{
		auto& renderTarget = blend.RenderTargets[i];
		renderTarget.BlendEnable = true;
		renderTarget.LogicOpEnable = false;
		renderTarget.SrcBlend = BlendFactor::ONE;
		renderTarget.DestBlend = BlendFactor::ONE;
		renderTarget.BlendOp = BlendOperator::ADD;
		renderTarget.SrcBlendAlpha = BlendFactor::ONE;
		renderTarget.DestBlendAlpha = BlendFactor::ONE;
		renderTarget.BlendOpAlpha = BlendOperator::ADD;
		renderTarget.LogicOp = LogicOperator::NOOP;
		renderTarget.WriteMask = ColorWrite::ALL;
	}

	// Production
	auto& renderTarget = blend.RenderTargets[n];
	renderTarget.BlendEnable = true;
	renderTarget.LogicOpEnable = false;
	renderTarget.SrcBlend = BlendFactor::ZERO;
	renderTarget.DestBlend = BlendFactor::INV_SRC_COLOR;
	renderTarget.BlendOp = BlendOperator::ADD;
	renderTarget.SrcBlendAlpha = BlendFactor::ZERO;
	renderTarget.DestBlendAlpha = BlendFactor::INV_SRC_ALPHA;
	renderTarget.BlendOpAlpha = BlendOperator::ADD;
	renderTarget.LogicOp = LogicOperator::NOOP;
	renderTarget.WriteMask = ColorWrite::ALL;

	// Default
	for (auto i = n + 1u; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
	{
		auto& renderTarget = blend.RenderTargets[i];
		renderTarget.BlendEnable = false;
		renderTarget.LogicOpEnable = false;
		renderTarget.SrcBlend = BlendFactor::ONE;
		renderTarget.DestBlend = BlendFactor::ZERO;
		renderTarget.BlendOp = BlendOperator::ADD;
		renderTarget.SrcBlendAlpha = BlendFactor::ONE;
		renderTarget.DestBlendAlpha = BlendFactor::ZERO;
		renderTarget.BlendOpAlpha = BlendOperator::ADD;
		renderTarget.LogicOp = LogicOperator::NOOP;
		renderTarget.WriteMask = ColorWrite::ALL;
	}

	return blend;
}

Blend PipelineLib_DX12::WeightedPerRT(uint8_t n)
{
	Blend blend;
	blend.AlphaToCoverageEnable = false;
	blend.IndependentBlendEnable = true;

	assert(n < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT);

	// Accumulations
	for (uint8_t i = 0; i < n; ++i)
	{
		auto& renderTarget = blend.RenderTargets[i];
		renderTarget.BlendEnable = true;
		renderTarget.LogicOpEnable = false;
		renderTarget.SrcBlend = BlendFactor::SRC_ALPHA;
		renderTarget.DestBlend = BlendFactor::ONE;
		renderTarget.BlendOp = BlendOperator::ADD;
		renderTarget.SrcBlendAlpha = BlendFactor::ONE;
		renderTarget.DestBlendAlpha = BlendFactor::ONE;
		renderTarget.BlendOpAlpha = BlendOperator::ADD;
		renderTarget.LogicOp = LogicOperator::NOOP;
		renderTarget.WriteMask = ColorWrite::ALL;
	}

	// Production
	auto& renderTarget = blend.RenderTargets[n];
	renderTarget.BlendEnable = true;
	renderTarget.LogicOpEnable = false;
	renderTarget.SrcBlend = BlendFactor::ZERO;
	renderTarget.DestBlend = BlendFactor::INV_SRC_COLOR;
	renderTarget.BlendOp = BlendOperator::ADD;
	renderTarget.SrcBlendAlpha = BlendFactor::ZERO;
	renderTarget.DestBlendAlpha = BlendFactor::INV_SRC_ALPHA;
	renderTarget.BlendOpAlpha = BlendOperator::ADD;
	renderTarget.LogicOp = LogicOperator::NOOP;
	renderTarget.WriteMask = ColorWrite::ALL;

	// Default
	for (auto i = n + 1u; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
	{
		auto& renderTarget = blend.RenderTargets[i];
		renderTarget.BlendEnable = false;
		renderTarget.LogicOpEnable = false;
		renderTarget.SrcBlend = BlendFactor::ONE;
		renderTarget.DestBlend = BlendFactor::ZERO;
		renderTarget.BlendOp = BlendOperator::ADD;
		renderTarget.SrcBlendAlpha = BlendFactor::ONE;
		renderTarget.DestBlendAlpha = BlendFactor::ZERO;
		renderTarget.BlendOpAlpha = BlendOperator::ADD;
		renderTarget.LogicOp = LogicOperator::NOOP;
		renderTarget.WriteMask = ColorWrite::ALL;
	}

	return blend;
}

Blend PipelineLib_DX12::SelectMin(uint8_t n)
{
	Blend blend;
	blend.AlphaToCoverageEnable = false;
	blend.IndependentBlendEnable = false;

	auto& pRenderTarget = blend.RenderTargets;
	pRenderTarget->BlendEnable = true;
	pRenderTarget->LogicOpEnable = false;
	pRenderTarget->SrcBlend = BlendFactor::ONE;
	pRenderTarget->DestBlend = BlendFactor::ONE;
	pRenderTarget->BlendOp = BlendOperator::MIN;
	pRenderTarget->SrcBlendAlpha = BlendFactor::ONE;
	pRenderTarget->DestBlendAlpha = BlendFactor::ONE;
	pRenderTarget->BlendOpAlpha = BlendOperator::MIN;
	pRenderTarget->LogicOp = LogicOperator::NOOP;
	pRenderTarget->WriteMask = ColorWrite::ALL;

	return blend;
}

Blend PipelineLib_DX12::SelectMax(uint8_t n)
{
	Blend blend;
	blend.AlphaToCoverageEnable = false;
	blend.IndependentBlendEnable = false;

	auto& pRenderTarget = blend.RenderTargets;
	pRenderTarget->BlendEnable = true;
	pRenderTarget->LogicOpEnable = false;
	pRenderTarget->SrcBlend = BlendFactor::ONE;
	pRenderTarget->DestBlend = BlendFactor::ONE;
	pRenderTarget->BlendOp = BlendOperator::MAX;
	pRenderTarget->SrcBlendAlpha = BlendFactor::ONE;
	pRenderTarget->DestBlendAlpha = BlendFactor::ONE;
	pRenderTarget->BlendOpAlpha = BlendOperator::MAX;
	pRenderTarget->LogicOp = LogicOperator::NOOP;
	pRenderTarget->WriteMask = ColorWrite::ALL;

	return blend;
}
