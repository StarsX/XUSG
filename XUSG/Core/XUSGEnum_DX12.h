//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

namespace XUSG
{
	// DX12 enum transfer functions
	DXGI_FORMAT GetDXGIFormat(Format format);

	D3D12_COMMAND_LIST_TYPE GetDX12CommandListType(CommandListType commandListType);
	D3D12_INPUT_CLASSIFICATION GetDX12InputClassification(InputClassification inputClassification);
	D3D12_HEAP_TYPE GetDX12HeapType(MemoryType memoryType);
	D3D12_PRIMITIVE_TOPOLOGY_TYPE GetDX12PrimitiveTopologyType(PrimitiveTopologyType primitiveTopologyType);
	D3D_PRIMITIVE_TOPOLOGY GetDX12PrimitiveTopology(PrimitiveTopology primitiveTopology);
	D3D12_FILL_MODE GetDX12FillMode(FillMode fillMode);
	D3D12_CULL_MODE GetDX12CullMode(CullMode cullMode);
	D3D12_INDIRECT_ARGUMENT_TYPE GetDX12IndirectArgumentType(IndirectArgumentType indirectArgumentType);
	D3D12_RESOURCE_DIMENSION GetDX12ResourceDimension(ResourceDimension resourceDimension);
	D3D12_TEXTURE_LAYOUT GetDX12TextureLayout(TextureLayout textureLayout);

	D3D12_COMMAND_QUEUE_FLAGS GetDX12CommandQueueFlag(CommandQueueFlag commandQueueFlag);
	D3D12_COMMAND_QUEUE_FLAGS GetDX12CommandQueueFlags(CommandQueueFlag commandQueueFlags);

	D3D12_HEAP_FLAGS GetDX12HeapFlag(MemoryFlag memoryFlag);
	D3D12_HEAP_FLAGS GetDX12HeapFlags(MemoryFlag memoryFlags);

	D3D12_RESOURCE_FLAGS GetDX12ResourceFlag(ResourceFlag resourceFlag);
	D3D12_RESOURCE_FLAGS GetDX12ResourceFlags(ResourceFlag resourceFlags);

	D3D12_RESOURCE_STATES GetDX12ResourceState(ResourceState resourceState);
	D3D12_RESOURCE_STATES GetDX12ResourceStates(ResourceState resourceStates);

	D3D12_RESOURCE_BARRIER_FLAGS GetDX12BarrierFlag(BarrierFlag barrierFlag);
	D3D12_RESOURCE_BARRIER_FLAGS GetDX12BarrierFlags(BarrierFlag barrierFlags);

	D3D12_BARRIER_LAYOUT GetDX12BarrierLayout(BarrierLayout barrierLayout);

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

	D3D12_SAMPLER_FLAGS GetDX12SamplerFlag(SamplerFlag samplerFlag);
	D3D12_SAMPLER_FLAGS GetDX12SamplerFlags(SamplerFlag samplerFlags);

	D3D12_BLEND GetDX12Blend(BlendFactor blend);
	D3D12_BLEND_OP GetDX12BlendOp(BlendOperator blendOp);
	D3D12_LOGIC_OP GetDX12LogicOp(LogicOperator logicOp);

	uint8_t GetDX12ColorWrite(ColorWrite writeMask);

	D3D12_LINE_RASTERIZATION_MODE GetDX12LineRasterizationMode(LineRasterization mode);

	D3D12_COMPARISON_FUNC GetDX12ComparisonFunc(ComparisonFunc comparisonFunc);
	D3D12_STENCIL_OP GetDX12StencilOp(StencilOp stencilOp);

	D3D12_PIPELINE_STATE_FLAGS GetDX12PipelineFlag(PipelineFlag pipelineFlag);
	D3D12_PIPELINE_STATE_FLAGS GetDX12PipelineFlags(PipelineFlag pipelineFlags);

	D3D12_QUERY_TYPE GetDX12QueryType(QueryType type);

	D3D12_TILE_COPY_FLAGS GetDX12TileCopyFlag(TileCopyFlag tileCopyFlag);
	D3D12_TILE_COPY_FLAGS GetDX12TileCopyFlags(TileCopyFlag tileCopyFlags);

	D3D12_FILTER GetDX12Filter(SamplerFilter filter);
	D3D12_TEXTURE_ADDRESS_MODE GetDX12TextureAddressMode(TextureAddressMode textureAddressMode);

	D3D12_SHADER_COMPONENT_MAPPING GetDX12ShaderComponentMapping(SrvComponentMapping srvComponentMapping);

	uint32_t GetDX12Requirement(Requirement requirement);

	uint32_t GetDXGISwapChainFlag(SwapChainFlag swapChainFlag);
	uint32_t GetDXGISwapChainFlags(SwapChainFlag swapChainFlags);

	uint32_t GetDXGIPresentFlag(PresentFlag presentFlag);
	uint32_t GetDXGIPresentFlags(PresentFlag presentFlags);
}
