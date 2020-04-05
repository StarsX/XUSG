//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGGraphicsState.h"

using namespace std;
using namespace XUSG;
using namespace Graphics;

Blend PipelineCache_DX12::DefaultOpaque(uint8_t n)
{
	return make_shared<D3D12_BLEND_DESC>(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
}

Blend PipelineCache_DX12::Premultiplied(uint8_t n)
{
	const auto blend = make_shared<D3D12_BLEND_DESC>();
	blend->AlphaToCoverageEnable = FALSE;
	blend->IndependentBlendEnable = FALSE;

	auto& pDesc = blend->RenderTarget;
	pDesc->BlendEnable = TRUE;
	pDesc->LogicOpEnable = FALSE;
	pDesc->SrcBlend = D3D12_BLEND_ONE;
	pDesc->DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	pDesc->BlendOp = D3D12_BLEND_OP_ADD;
	pDesc->SrcBlendAlpha = D3D12_BLEND_ONE;
	pDesc->DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
	pDesc->BlendOpAlpha = D3D12_BLEND_OP_ADD;
	pDesc->LogicOp = D3D12_LOGIC_OP_NOOP;
	pDesc->RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	return blend;
}

Blend PipelineCache_DX12::Additive(uint8_t n)
{
	const auto blend = make_shared<D3D12_BLEND_DESC>();
	blend->AlphaToCoverageEnable = FALSE;
	blend->IndependentBlendEnable = FALSE;

	auto& pDesc = blend->RenderTarget;
	pDesc->BlendEnable = TRUE;
	pDesc->LogicOpEnable = FALSE;
	pDesc->SrcBlend = D3D12_BLEND_SRC_ALPHA;
	pDesc->DestBlend = D3D12_BLEND_ONE;
	pDesc->BlendOp = D3D12_BLEND_OP_ADD;
	pDesc->SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
	pDesc->DestBlendAlpha = D3D12_BLEND_ONE;
	pDesc->BlendOpAlpha = D3D12_BLEND_OP_ADD;
	pDesc->LogicOp = D3D12_LOGIC_OP_NOOP;
	pDesc->RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	return blend;
}

Blend PipelineCache_DX12::NonPremultiplied(uint8_t n)
{
	const auto blend = make_shared<D3D12_BLEND_DESC>();
	blend->AlphaToCoverageEnable = FALSE;
	blend->IndependentBlendEnable = FALSE;

	auto& pDesc = blend->RenderTarget;
	pDesc->BlendEnable = TRUE;
	pDesc->LogicOpEnable = FALSE;
	pDesc->SrcBlend = D3D12_BLEND_SRC_ALPHA;
	pDesc->DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	pDesc->BlendOp = D3D12_BLEND_OP_ADD;
	pDesc->SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
	pDesc->DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
	pDesc->BlendOpAlpha = D3D12_BLEND_OP_ADD;
	pDesc->LogicOp = D3D12_LOGIC_OP_NOOP;
	pDesc->RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	return blend;
}

Blend PipelineCache_DX12::NonPremultipliedRT0(uint8_t n)
{
	const auto blend = make_shared<D3D12_BLEND_DESC>();
	blend->AlphaToCoverageEnable = FALSE;
	blend->IndependentBlendEnable = TRUE;

	auto& desc = blend->RenderTarget[0];
	desc.BlendEnable = TRUE;
	desc.LogicOpEnable = FALSE;
	desc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	desc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	desc.BlendOp = D3D12_BLEND_OP_ADD;
	desc.SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
	desc.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
	desc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	desc.LogicOp = D3D12_LOGIC_OP_NOOP;
	desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	// Default
	D3D12_RENDER_TARGET_BLEND_DESC descDefault;
	descDefault.BlendEnable = FALSE;
	descDefault.LogicOpEnable = FALSE;
	descDefault.SrcBlend = D3D12_BLEND_ONE;
	descDefault.DestBlend = D3D12_BLEND_ZERO;
	descDefault.BlendOp = D3D12_BLEND_OP_ADD;
	descDefault.SrcBlendAlpha = D3D12_BLEND_ONE;
	descDefault.DestBlendAlpha = D3D12_BLEND_ZERO;
	descDefault.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	descDefault.LogicOp = D3D12_LOGIC_OP_NOOP;
	descDefault.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	for (auto i = 1u; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		blend->RenderTarget[i] = descDefault;

	return blend;
}

Blend PipelineCache_DX12::AlphaToCoverage(uint8_t n)
{
	const auto blend = make_shared<D3D12_BLEND_DESC>();
	blend->AlphaToCoverageEnable = TRUE;
	blend->IndependentBlendEnable = FALSE;

	auto& pDesc = blend->RenderTarget;
	pDesc->BlendEnable = FALSE;
	pDesc->LogicOpEnable = FALSE;
	pDesc->SrcBlend = D3D12_BLEND_SRC_ALPHA;
	pDesc->DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	pDesc->BlendOp = D3D12_BLEND_OP_ADD;
	pDesc->SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
	pDesc->DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
	pDesc->BlendOpAlpha = D3D12_BLEND_OP_ADD;
	pDesc->LogicOp = D3D12_LOGIC_OP_NOOP;
	pDesc->RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	return blend;
}

Blend PipelineCache_DX12::Accumulative(uint8_t n)
{
	const auto blend = make_shared<D3D12_BLEND_DESC>();
	blend->AlphaToCoverageEnable = FALSE;
	blend->IndependentBlendEnable = FALSE;

	auto& pDesc = blend->RenderTarget;
	pDesc->BlendEnable = TRUE;
	pDesc->LogicOpEnable = FALSE;
	pDesc->SrcBlend = D3D12_BLEND_ONE;
	pDesc->DestBlend = D3D12_BLEND_ONE;
	pDesc->BlendOp = D3D12_BLEND_OP_ADD;
	pDesc->SrcBlendAlpha = D3D12_BLEND_ONE;
	pDesc->DestBlendAlpha = D3D12_BLEND_ONE;
	pDesc->BlendOpAlpha = D3D12_BLEND_OP_ADD;
	pDesc->LogicOp = D3D12_LOGIC_OP_NOOP;
	pDesc->RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	return blend;
}

Blend PipelineCache_DX12::AutoNonPremultiplied(uint8_t n)
{
	const auto blend = make_shared<D3D12_BLEND_DESC>();
	blend->AlphaToCoverageEnable = FALSE;
	blend->IndependentBlendEnable = FALSE;

	auto& pDesc = blend->RenderTarget;
	pDesc->BlendEnable = TRUE;
	pDesc->LogicOpEnable = FALSE;
	pDesc->SrcBlend = D3D12_BLEND_SRC_ALPHA;
	pDesc->DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	pDesc->BlendOp = D3D12_BLEND_OP_ADD;
	pDesc->SrcBlendAlpha = D3D12_BLEND_ONE;
	pDesc->DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
	pDesc->BlendOpAlpha = D3D12_BLEND_OP_ADD;
	pDesc->LogicOp = D3D12_LOGIC_OP_NOOP;
	pDesc->RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	return blend;
}

Blend PipelineCache_DX12::ZeroAlphaNonPremultiplied(uint8_t n)
{
	const auto blend = make_shared<D3D12_BLEND_DESC>();
	blend->AlphaToCoverageEnable = FALSE;
	blend->IndependentBlendEnable = FALSE;

	auto& pDesc = blend->RenderTarget;
	pDesc->BlendEnable = TRUE;
	pDesc->LogicOpEnable = FALSE;
	pDesc->SrcBlend = D3D12_BLEND_SRC_ALPHA;
	pDesc->DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	pDesc->BlendOp = D3D12_BLEND_OP_ADD;
	pDesc->SrcBlendAlpha = D3D12_BLEND_ZERO;
	pDesc->DestBlendAlpha = D3D12_BLEND_ZERO;
	pDesc->BlendOpAlpha = D3D12_BLEND_OP_ADD;
	pDesc->LogicOp = D3D12_LOGIC_OP_NOOP;
	pDesc->RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	return blend;
}

Blend PipelineCache_DX12::Multiplied(uint8_t n)
{
	const auto blend = make_shared<D3D12_BLEND_DESC>();
	blend->AlphaToCoverageEnable = FALSE;
	blend->IndependentBlendEnable = FALSE;

	auto& pDesc = blend->RenderTarget;
	pDesc->BlendEnable = TRUE;
	pDesc->LogicOpEnable = FALSE;
	pDesc->SrcBlend = D3D12_BLEND_ZERO;
	pDesc->DestBlend = D3D12_BLEND_SRC_COLOR;
	pDesc->BlendOp = D3D12_BLEND_OP_ADD;
	pDesc->SrcBlendAlpha = D3D12_BLEND_ZERO;
	pDesc->DestBlendAlpha = D3D12_BLEND_SRC_ALPHA;
	pDesc->BlendOpAlpha = D3D12_BLEND_OP_ADD;
	pDesc->LogicOp = D3D12_LOGIC_OP_NOOP;
	pDesc->RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	return blend;
}

Blend PipelineCache_DX12::WeightedPremul(uint8_t n)
{
	const auto blend = make_shared<D3D12_BLEND_DESC>();
	blend->AlphaToCoverageEnable = FALSE;
	blend->IndependentBlendEnable = FALSE;

	auto& pDesc = blend->RenderTarget;
	pDesc->BlendEnable = TRUE;
	pDesc->LogicOpEnable = FALSE;
	pDesc->SrcBlend = D3D12_BLEND_ONE;
	pDesc->DestBlend = D3D12_BLEND_ONE;
	pDesc->BlendOp = D3D12_BLEND_OP_ADD;
	pDesc->SrcBlendAlpha = D3D12_BLEND_ONE;
	pDesc->DestBlendAlpha = D3D12_BLEND_ONE;
	pDesc->BlendOpAlpha = D3D12_BLEND_OP_ADD;
	pDesc->LogicOp = D3D12_LOGIC_OP_NOOP;
	pDesc->RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	return blend;
}

Blend PipelineCache_DX12::WeightedPremulPerRT(uint8_t n)
{
	const auto blend = make_shared<D3D12_BLEND_DESC>();
	blend->AlphaToCoverageEnable = FALSE;
	blend->IndependentBlendEnable = TRUE;

	assert(n < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT);

	// Accumulations
	for (auto i = 0ui8; i < n; ++i)
	{
		auto& desc = blend->RenderTarget[0];
		desc.BlendEnable = TRUE;
		desc.LogicOpEnable = FALSE;
		desc.SrcBlend = D3D12_BLEND_ONE;
		desc.DestBlend = D3D12_BLEND_ONE;
		desc.BlendOp = D3D12_BLEND_OP_ADD;
		desc.SrcBlendAlpha = D3D12_BLEND_ONE;
		desc.DestBlendAlpha = D3D12_BLEND_ONE;
		desc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		desc.LogicOp = D3D12_LOGIC_OP_NOOP;
		desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	}

	// Production
	auto& desc = blend->RenderTarget[n];
	desc.BlendEnable = TRUE;
	desc.LogicOpEnable = FALSE;
	desc.SrcBlend = D3D12_BLEND_ZERO;
	desc.DestBlend = D3D12_BLEND_INV_SRC_COLOR;
	desc.BlendOp = D3D12_BLEND_OP_ADD;
	desc.SrcBlendAlpha = D3D12_BLEND_ZERO;
	desc.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
	desc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	desc.LogicOp = D3D12_LOGIC_OP_NOOP;
	desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	// Default
	D3D12_RENDER_TARGET_BLEND_DESC descDefault;
	descDefault.BlendEnable = FALSE;
	descDefault.LogicOpEnable = FALSE;
	descDefault.SrcBlend = D3D12_BLEND_ONE;
	descDefault.DestBlend = D3D12_BLEND_ZERO;
	descDefault.BlendOp = D3D12_BLEND_OP_ADD;
	descDefault.SrcBlendAlpha = D3D12_BLEND_ONE;
	descDefault.DestBlendAlpha = D3D12_BLEND_ZERO;
	descDefault.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	descDefault.LogicOp = D3D12_LOGIC_OP_NOOP;
	descDefault.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	for (auto i = n + 1; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		blend->RenderTarget[i] = descDefault;

	return blend;
}

Blend PipelineCache_DX12::WeightedPerRT(uint8_t n)
{
	const auto blend = make_shared<D3D12_BLEND_DESC>();
	blend->AlphaToCoverageEnable = FALSE;
	blend->IndependentBlendEnable = TRUE;

	assert(n < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT);

	// Accumulations
	for (auto i = 0ui8; i < n; ++i)
	{
		auto& desc = blend->RenderTarget[0];
		desc.BlendEnable = TRUE;
		desc.LogicOpEnable = FALSE;
		desc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
		desc.DestBlend = D3D12_BLEND_ONE;
		desc.BlendOp = D3D12_BLEND_OP_ADD;
		desc.SrcBlendAlpha = D3D12_BLEND_ONE;
		desc.DestBlendAlpha = D3D12_BLEND_ONE;
		desc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		desc.LogicOp = D3D12_LOGIC_OP_NOOP;
		desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	}

	// Production
	auto& desc = blend->RenderTarget[n];
	desc.BlendEnable = TRUE;
	desc.LogicOpEnable = FALSE;
	desc.SrcBlend = D3D12_BLEND_ZERO;
	desc.DestBlend = D3D12_BLEND_INV_SRC_COLOR;
	desc.BlendOp = D3D12_BLEND_OP_ADD;
	desc.SrcBlendAlpha = D3D12_BLEND_ZERO;
	desc.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
	desc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	desc.LogicOp = D3D12_LOGIC_OP_NOOP;
	desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	// Default
	D3D12_RENDER_TARGET_BLEND_DESC descDefault;
	descDefault.BlendEnable = FALSE;
	descDefault.LogicOpEnable = FALSE;
	descDefault.SrcBlend = D3D12_BLEND_ONE;
	descDefault.DestBlend = D3D12_BLEND_ZERO;
	descDefault.BlendOp = D3D12_BLEND_OP_ADD;
	descDefault.SrcBlendAlpha = D3D12_BLEND_ONE;
	descDefault.DestBlendAlpha = D3D12_BLEND_ZERO;
	descDefault.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	descDefault.LogicOp = D3D12_LOGIC_OP_NOOP;
	descDefault.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	for (auto i = n + 1; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		blend->RenderTarget[i] = descDefault;

	return blend;
}

Blend PipelineCache_DX12::SelectMin(uint8_t n)
{
	const auto blend = make_shared<D3D12_BLEND_DESC>();
	blend->AlphaToCoverageEnable = FALSE;
	blend->IndependentBlendEnable = FALSE;

	auto& pDesc = blend->RenderTarget;
	pDesc->BlendEnable = TRUE;
	pDesc->LogicOpEnable = FALSE;
	pDesc->SrcBlend = D3D12_BLEND_ONE;
	pDesc->DestBlend = D3D12_BLEND_ONE;
	pDesc->BlendOp = D3D12_BLEND_OP_MIN;
	pDesc->SrcBlendAlpha = D3D12_BLEND_ONE;
	pDesc->DestBlendAlpha = D3D12_BLEND_ONE;
	pDesc->BlendOpAlpha = D3D12_BLEND_OP_MIN;
	pDesc->LogicOp = D3D12_LOGIC_OP_NOOP;
	pDesc->RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	return blend;
}

Blend PipelineCache_DX12::SelectMax(uint8_t n)
{
	const auto blend = make_shared<D3D12_BLEND_DESC>();
	blend->AlphaToCoverageEnable = FALSE;
	blend->IndependentBlendEnable = FALSE;

	auto& pDesc = blend->RenderTarget;
	pDesc->BlendEnable = TRUE;
	pDesc->LogicOpEnable = FALSE;
	pDesc->SrcBlend = D3D12_BLEND_ONE;
	pDesc->DestBlend = D3D12_BLEND_ONE;
	pDesc->BlendOp = D3D12_BLEND_OP_MAX;
	pDesc->SrcBlendAlpha = D3D12_BLEND_ONE;
	pDesc->DestBlendAlpha = D3D12_BLEND_ONE;
	pDesc->BlendOpAlpha = D3D12_BLEND_OP_MAX;
	pDesc->LogicOp = D3D12_LOGIC_OP_NOOP;
	pDesc->RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	return blend;
}
