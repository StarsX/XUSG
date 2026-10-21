//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSG.h"

using namespace std;
using namespace XUSG;
using namespace Graphics;

Blend PipelineLib::DefaultOpaque(uint8_t n)
{
	Blend blend = {};
	//blend.AlphaToCoverageEnable = false;
	//blend.IndependentBlendEnable = false;

	//blend.RenderTargets->BlendEnable = false;
	//blend.RenderTargets->LogicOpEnable = false;
	blend.RenderTargets->SrcBlend = BlendFactor::ONE;
	blend.RenderTargets->DestBlend = BlendFactor::ZERO;
	blend.RenderTargets->BlendOp = BlendOperator::ADD;
	blend.RenderTargets->SrcBlendAlpha = BlendFactor::ONE;
	blend.RenderTargets->DestBlendAlpha = BlendFactor::ZERO;
	blend.RenderTargets->BlendOpAlpha = BlendOperator::ADD;
	blend.RenderTargets->LogicOp = LogicOperator::NOOP;
	blend.RenderTargets->WriteMask = ColorWrite::ALL;

	return blend;
}

Blend PipelineLib::Premultiplied(uint8_t n)
{
	Blend blend = {};
	//blend.AlphaToCoverageEnable = false;
	//blend.IndependentBlendEnable = false;

	blend.RenderTargets->BlendEnable = true;
	//blend.RenderTargets->LogicOpEnable = false;
	blend.RenderTargets->SrcBlend = BlendFactor::ONE;
	blend.RenderTargets->DestBlend = BlendFactor::INV_SRC_ALPHA;
	blend.RenderTargets->BlendOp = BlendOperator::ADD;
	blend.RenderTargets->SrcBlendAlpha = BlendFactor::ONE;
	blend.RenderTargets->DestBlendAlpha = BlendFactor::INV_SRC_ALPHA;
	blend.RenderTargets->BlendOpAlpha = BlendOperator::ADD;
	blend.RenderTargets->LogicOp = LogicOperator::NOOP;
	blend.RenderTargets->WriteMask = ColorWrite::ALL;

	return blend;
}

Blend PipelineLib::Additive(uint8_t n)
{
	Blend blend = {};
	//blend.AlphaToCoverageEnable = false;
	//blend.IndependentBlendEnable = false;

	blend.RenderTargets->BlendEnable = true;
	//blend.RenderTargets->LogicOpEnable = false;
	blend.RenderTargets->SrcBlend = BlendFactor::SRC_ALPHA;
	blend.RenderTargets->DestBlend = BlendFactor::ONE;
	blend.RenderTargets->BlendOp = BlendOperator::ADD;
	blend.RenderTargets->SrcBlendAlpha = BlendFactor::SRC_ALPHA;
	blend.RenderTargets->DestBlendAlpha = BlendFactor::ONE;
	blend.RenderTargets->BlendOpAlpha = BlendOperator::ADD;
	blend.RenderTargets->LogicOp = LogicOperator::NOOP;
	blend.RenderTargets->WriteMask = ColorWrite::ALL;

	return blend;
}

Blend PipelineLib::NonPremultiplied(uint8_t n)
{
	Blend blend = {};
	//blend.AlphaToCoverageEnable = false;
	//blend.IndependentBlendEnable = false;

	blend.RenderTargets->BlendEnable = true;
	//blend.RenderTargets->LogicOpEnable = false;
	blend.RenderTargets->SrcBlend = BlendFactor::SRC_ALPHA;
	blend.RenderTargets->DestBlend = BlendFactor::INV_SRC_ALPHA;
	blend.RenderTargets->BlendOp = BlendOperator::ADD;
	blend.RenderTargets->SrcBlendAlpha = BlendFactor::SRC_ALPHA;
	blend.RenderTargets->DestBlendAlpha = BlendFactor::INV_SRC_ALPHA;
	blend.RenderTargets->BlendOpAlpha = BlendOperator::ADD;
	blend.RenderTargets->LogicOp = LogicOperator::NOOP;
	blend.RenderTargets->WriteMask = ColorWrite::ALL;

	return blend;
}

Blend PipelineLib::NonPremultipliedRT0(uint8_t n)
{
	Blend blend;
	blend.AlphaToCoverageEnable = false;
	blend.IndependentBlendEnable = true;

	blend.RenderTargets->BlendEnable = true;
	blend.RenderTargets->LogicOpEnable = false;
	blend.RenderTargets->SrcBlend = BlendFactor::SRC_ALPHA;
	blend.RenderTargets->DestBlend = BlendFactor::INV_SRC_ALPHA;
	blend.RenderTargets->BlendOp = BlendOperator::ADD;
	blend.RenderTargets->SrcBlendAlpha = BlendFactor::SRC_ALPHA;
	blend.RenderTargets->DestBlendAlpha = BlendFactor::INV_SRC_ALPHA;
	blend.RenderTargets->BlendOpAlpha = BlendOperator::ADD;
	blend.RenderTargets->LogicOp = LogicOperator::NOOP;
	blend.RenderTargets->WriteMask = ColorWrite::ALL;

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

Blend PipelineLib::AlphaToCoverage(uint8_t n)
{
	Blend blend = {};
	blend.AlphaToCoverageEnable = true;
	//blend.IndependentBlendEnable = false;

	//blend.RenderTargets->BlendEnable = false;
	//blend.RenderTargets->LogicOpEnable = false;
	blend.RenderTargets->SrcBlend = BlendFactor::SRC_ALPHA;
	blend.RenderTargets->DestBlend = BlendFactor::INV_SRC_ALPHA;
	blend.RenderTargets->BlendOp = BlendOperator::ADD;
	blend.RenderTargets->SrcBlendAlpha = BlendFactor::SRC_ALPHA;
	blend.RenderTargets->DestBlendAlpha = BlendFactor::INV_SRC_ALPHA;
	blend.RenderTargets->BlendOpAlpha = BlendOperator::ADD;
	blend.RenderTargets->LogicOp = LogicOperator::NOOP;
	blend.RenderTargets->WriteMask = ColorWrite::ALL;

	return blend;
}

Blend PipelineLib::Accumulative(uint8_t n)
{
	Blend blend = {};
	//blend.AlphaToCoverageEnable = false;
	//blend.IndependentBlendEnable = false;

	blend.RenderTargets->BlendEnable = true;
	//blend.RenderTargets->LogicOpEnable = false;
	blend.RenderTargets->SrcBlend = BlendFactor::ONE;
	blend.RenderTargets->DestBlend = BlendFactor::ONE;
	blend.RenderTargets->BlendOp = BlendOperator::ADD;
	blend.RenderTargets->SrcBlendAlpha = BlendFactor::ONE;
	blend.RenderTargets->DestBlendAlpha = BlendFactor::ONE;
	blend.RenderTargets->BlendOpAlpha = BlendOperator::ADD;
	blend.RenderTargets->LogicOp = LogicOperator::NOOP;
	blend.RenderTargets->WriteMask = ColorWrite::ALL;

	return blend;
}

Blend PipelineLib::AutoNonPremultiplied(uint8_t n)
{
	Blend blend = {};
	//blend.AlphaToCoverageEnable = false;
	//blend.IndependentBlendEnable = false;

	blend.RenderTargets->BlendEnable = true;
	//blend.RenderTargets->LogicOpEnable = false;
	blend.RenderTargets->SrcBlend = BlendFactor::SRC_ALPHA;
	blend.RenderTargets->DestBlend = BlendFactor::INV_SRC_ALPHA;
	blend.RenderTargets->BlendOp = BlendOperator::ADD;
	blend.RenderTargets->SrcBlendAlpha = BlendFactor::ONE;
	blend.RenderTargets->DestBlendAlpha = BlendFactor::INV_SRC_ALPHA;
	blend.RenderTargets->BlendOpAlpha = BlendOperator::ADD;
	blend.RenderTargets->LogicOp = LogicOperator::NOOP;
	blend.RenderTargets->WriteMask = ColorWrite::ALL;

	return blend;
}

Blend PipelineLib::ZeroAlphaNonPremultiplied(uint8_t n)
{
	Blend blend = {};
	//blend.AlphaToCoverageEnable = false;
	//blend.IndependentBlendEnable = false;

	blend.RenderTargets->BlendEnable = true;
	//blend.RenderTargets->LogicOpEnable = false;
	blend.RenderTargets->SrcBlend = BlendFactor::SRC_ALPHA;
	blend.RenderTargets->DestBlend = BlendFactor::INV_SRC_ALPHA;
	blend.RenderTargets->BlendOp = BlendOperator::ADD;
	blend.RenderTargets->SrcBlendAlpha = BlendFactor::ZERO;
	blend.RenderTargets->DestBlendAlpha = BlendFactor::ZERO;
	blend.RenderTargets->BlendOpAlpha = BlendOperator::ADD;
	blend.RenderTargets->LogicOp = LogicOperator::NOOP;
	blend.RenderTargets->WriteMask = ColorWrite::ALL;

	return blend;
}

Blend PipelineLib::Multiplied(uint8_t n)
{
	Blend blend = {};
	//blend.AlphaToCoverageEnable = false;
	//blend.IndependentBlendEnable = false;

	blend.RenderTargets->BlendEnable = true;
	//blend.RenderTargets->LogicOpEnable = false;
	blend.RenderTargets->SrcBlend = BlendFactor::ZERO;
	blend.RenderTargets->DestBlend = BlendFactor::SRC_COLOR;
	blend.RenderTargets->BlendOp = BlendOperator::ADD;
	blend.RenderTargets->SrcBlendAlpha = BlendFactor::ZERO;
	blend.RenderTargets->DestBlendAlpha = BlendFactor::SRC_ALPHA;
	blend.RenderTargets->BlendOpAlpha = BlendOperator::ADD;
	blend.RenderTargets->LogicOp = LogicOperator::NOOP;
	blend.RenderTargets->WriteMask = ColorWrite::ALL;

	return blend;
}

Blend PipelineLib::WeightedPremul(uint8_t n)
{
	Blend blend = {};
	//blend.AlphaToCoverageEnable = false;
	//blend.IndependentBlendEnable = false;

	blend.RenderTargets->BlendEnable = true;
	//blend.RenderTargets->LogicOpEnable = false;
	blend.RenderTargets->SrcBlend = BlendFactor::ONE;
	blend.RenderTargets->DestBlend = BlendFactor::ONE;
	blend.RenderTargets->BlendOp = BlendOperator::ADD;
	blend.RenderTargets->SrcBlendAlpha = BlendFactor::ONE;
	blend.RenderTargets->DestBlendAlpha = BlendFactor::ONE;
	blend.RenderTargets->BlendOpAlpha = BlendOperator::ADD;
	blend.RenderTargets->LogicOp = LogicOperator::NOOP;
	blend.RenderTargets->WriteMask = ColorWrite::ALL;

	return blend;
}

Blend PipelineLib::WeightedPremulPerRT(uint8_t n)
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
	for (uint8_t i = n + 1; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
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

Blend PipelineLib::WeightedPerRT(uint8_t n)
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
	for (uint8_t i = n + 1; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
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

Blend PipelineLib::SelectMin(uint8_t n)
{
	Blend blend = {};
	//blend.AlphaToCoverageEnable = false;
	//blend.IndependentBlendEnable = false;

	blend.RenderTargets->BlendEnable = true;
	//blend.RenderTargets->LogicOpEnable = false;
	blend.RenderTargets->SrcBlend = BlendFactor::ONE;
	blend.RenderTargets->DestBlend = BlendFactor::ONE;
	blend.RenderTargets->BlendOp = BlendOperator::MIN;
	blend.RenderTargets->SrcBlendAlpha = BlendFactor::ONE;
	blend.RenderTargets->DestBlendAlpha = BlendFactor::ONE;
	blend.RenderTargets->BlendOpAlpha = BlendOperator::MIN;
	blend.RenderTargets->LogicOp = LogicOperator::NOOP;
	blend.RenderTargets->WriteMask = ColorWrite::ALL;

	return blend;
}

Blend PipelineLib::SelectMax(uint8_t n)
{
	Blend blend = {};
	//blend.AlphaToCoverageEnable = false;
	//blend.IndependentBlendEnable = false;

	blend.RenderTargets->BlendEnable = true;
	//blend.RenderTargets->LogicOpEnable = false;
	blend.RenderTargets->SrcBlend = BlendFactor::ONE;
	blend.RenderTargets->DestBlend = BlendFactor::ONE;
	blend.RenderTargets->BlendOp = BlendOperator::MAX;
	blend.RenderTargets->SrcBlendAlpha = BlendFactor::ONE;
	blend.RenderTargets->DestBlendAlpha = BlendFactor::ONE;
	blend.RenderTargets->BlendOpAlpha = BlendOperator::MAX;
	blend.RenderTargets->LogicOp = LogicOperator::NOOP;
	blend.RenderTargets->WriteMask = ColorWrite::ALL;

	return blend;
}
