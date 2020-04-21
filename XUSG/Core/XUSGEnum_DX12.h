//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

namespace XUSG
{
	// DX12 enum transfer functions
	D3D12_COMMAND_LIST_TYPE GetDX12CommandListType(CommandListType commandListType);
	D3D12_INPUT_CLASSIFICATION GetDX12InputClassification(InputClassification inputClassification);
	D3D12_HEAP_TYPE GetDX12HeapType(MemoryType memoryType);
	D3D12_PRIMITIVE_TOPOLOGY_TYPE GetDX12PrimitiveTopologyType(PrimitiveTopologyType primitiveTopologyType);
	D3D_PRIMITIVE_TOPOLOGY GetDX12PrimitiveTopology(PrimitiveTopology primitiveTopology);
	D3D12_FILL_MODE GetDX12FillMode(FillMode fillMode);
	D3D12_CULL_MODE GetDX12CullMode(CullMode cullMode);
	D3D12_INDIRECT_ARGUMENT_TYPE GetDX12IndirectArgumentType(IndirectArgumentType indirectArgumentType);
	D3D12_RESOURCE_DIMENSION GetDX12ResourceDimension(ResourceDimension resourceDimension);

	D3D12_COMMAND_QUEUE_FLAGS GetDX12CommandQueueFlag(CommandQueueFlag commandQueueFlag);
	D3D12_COMMAND_QUEUE_FLAGS GetDX12CommandQueueFlags(CommandQueueFlag commandQueueFlags);

	D3D12_RESOURCE_FLAGS GetDX12ResourceFlag(ResourceFlag resourceFlag);
	D3D12_RESOURCE_FLAGS GetDX12ResourceFlags(ResourceFlag resourceFlags);

	D3D12_RESOURCE_STATES GetDX12ResourceState(ResourceState resourceState);
	D3D12_RESOURCE_STATES GetDX12ResourceStates(ResourceState resourceStates);

	D3D12_RESOURCE_BARRIER_FLAGS GetDX12BarrierFlag(BarrierFlag barrierFlag);
	D3D12_RESOURCE_BARRIER_FLAGS GetDX12BarrierFlags(BarrierFlag barrierFlags);

	D3D12_DESCRIPTOR_RANGE_FLAGS GetDX12DescriptorRangeFlag(DescriptorFlag descriptorFlag);
	D3D12_DESCRIPTOR_RANGE_FLAGS GetDX12DescriptorRangeFlags(DescriptorFlag descriptorFlag);

	D3D12_ROOT_DESCRIPTOR_FLAGS GetDX12RootDescriptorFlag(DescriptorFlag descriptorFlag);
	D3D12_ROOT_DESCRIPTOR_FLAGS GetDX12RootDescriptorFlags(DescriptorFlag descriptorFlag);

	D3D12_ROOT_SIGNATURE_FLAGS GetDX12RootSignatureFlag(PipelineLayoutFlag pipelineLayoutFlag);
	D3D12_ROOT_SIGNATURE_FLAGS GetDX12RootSignatureFlags(PipelineLayoutFlag pipelineLayoutFlags);

	D3D12_CLEAR_FLAGS GetDX12ClearFlag(ClearFlag clearFlag);
	D3D12_CLEAR_FLAGS GetDX12ClearFlags(ClearFlag clearFlags);

	D3D12_FENCE_FLAGS GetDX12FenceFlag(FenceFlag fenceFlag);
	D3D12_FENCE_FLAGS GetDX12FenceFlags(FenceFlag fenceFlags);

	D3D12_BLEND GetDX12Blend(BlendFactor blend);
	D3D12_BLEND_OP GetDX12BlendOp(BlendOperator blendOp);
	D3D12_LOGIC_OP GetDX12LogicOp(LogicOperator logicOp);
	D3D12_COLOR_WRITE_ENABLE GetDX12ColorWrite(ColorWrite writeMask);

	D3D12_COMPARISON_FUNC GetDX12ComparisonFunc(ComparisonFunc comparisonFunc);
	D3D12_STENCIL_OP GetDX12StencilOp(StencilOp stencilOp);

	D3D12_QUERY_TYPE GetDX12QueryType(QueryType type);
}
