//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSG_DX12.h"
#include "XUSGEnum_DX12.h"

using namespace XUSG;

#define REMOVE_PACKED_UAV		(~ResourceFlag::NEED_PACKED_UAV | ResourceFlag::ALLOW_UNORDERED_ACCESS)
#define REMOVE_RAYTRACING_AS	(~ResourceFlag::ACCELERATION_STRUCTURE | ResourceFlag::ALLOW_UNORDERED_ACCESS)

#define APPEND_FLAG(type, dx12Type, flags, flag, none) (static_cast<bool>(flags & type::flag) ? dx12Type##_##flag : dx12Type##_##none)
#define APPEND_COMMAND_QUEUE_FLAG(flags, flag) APPEND_FLAG(CommandQueueFlag, D3D12_COMMAND_QUEUE_FLAG, flags, flag, NONE)
#define APPEND_HEAP_FLAG(flags, flag) APPEND_FLAG(MemoryFlag, D3D12_HEAP_FLAG, flags, flag, NONE)
#define APPEND_RESOURCE_FLAG(flags, flag) APPEND_FLAG(ResourceFlag, D3D12_RESOURCE_FLAG, flags, flag, NONE)
#define APPEND_RESOURCE_STATE(states, state) APPEND_FLAG(ResourceState, D3D12_RESOURCE_STATE, states, state, COMMON)
#define APPEND_BARRIER_FLAG(flags, flag) APPEND_FLAG(BarrierFlag, D3D12_RESOURCE_BARRIER_FLAG, flags, flag, NONE)
#define APPEND_DESCRIPTOR_RANGE_FLAG(flags, flag) APPEND_FLAG(DescriptorFlag, D3D12_DESCRIPTOR_RANGE_FLAG, flags, flag, NONE)
#define APPEND_ROOT_DESCRIPTOR_FLAG(flags, flag) APPEND_FLAG(DescriptorFlag, D3D12_ROOT_DESCRIPTOR_FLAG, flags, flag, NONE)
#define APPEND_ROOT_SIGNATURE_FLAG(flags, flag) APPEND_FLAG(PipelineLayoutFlag, D3D12_ROOT_SIGNATURE_FLAG, flags, flag, NONE)
#define APPEND_FENCE_FLAG(flags, flag) APPEND_FLAG(FenceFlag, D3D12_FENCE_FLAG, flags, flag, NONE)
#define APPEND_TILE_COPY_FLAG(flags, flag) APPEND_FLAG(TileCopyFlag, D3D12_TILE_COPY_FLAG, flags, flag, NONE)
#define APPEND_SWAP_CHAIN_FLAG(flags, flag) (static_cast<bool>(flags & SwapChainFlag::flag) ? DXGI_SWAP_CHAIN_FLAG_##flag : 0)
#define APPEND_PRESENT_FLAG(flags, flag) (static_cast<bool>(flags & PresentFlag::flag) ? DXGI_PRESENT_##flag : 0)

DXGI_FORMAT XUSG::GetDXGIFormat(Format format)
{
	static const DXGI_FORMAT formats[] =
	{
		DXGI_FORMAT_UNKNOWN,
		DXGI_FORMAT_R32G32B32A32_TYPELESS,
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		DXGI_FORMAT_R32G32B32A32_UINT,
		DXGI_FORMAT_R32G32B32A32_SINT,
		DXGI_FORMAT_R32G32B32_TYPELESS,
		DXGI_FORMAT_R32G32B32_FLOAT,
		DXGI_FORMAT_R32G32B32_UINT,
		DXGI_FORMAT_R32G32B32_SINT,
		DXGI_FORMAT_R16G16B16A16_TYPELESS,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		DXGI_FORMAT_R16G16B16A16_UNORM,
		DXGI_FORMAT_R16G16B16A16_UINT,
		DXGI_FORMAT_R16G16B16A16_SNORM,
		DXGI_FORMAT_R16G16B16A16_SINT,
		DXGI_FORMAT_R32G32_TYPELESS,
		DXGI_FORMAT_R32G32_FLOAT,
		DXGI_FORMAT_R32G32_UINT,
		DXGI_FORMAT_R32G32_SINT,
		DXGI_FORMAT_R32G8X24_TYPELESS,
		DXGI_FORMAT_D32_FLOAT_S8X24_UINT,
		DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS,
		DXGI_FORMAT_X32_TYPELESS_G8X24_UINT,
		DXGI_FORMAT_R10G10B10A2_TYPELESS,
		DXGI_FORMAT_R10G10B10A2_UNORM,
		DXGI_FORMAT_R10G10B10A2_UINT,
		DXGI_FORMAT_R11G11B10_FLOAT,
		DXGI_FORMAT_R8G8B8A8_TYPELESS,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		DXGI_FORMAT_R8G8B8A8_UINT,
		DXGI_FORMAT_R8G8B8A8_SNORM,
		DXGI_FORMAT_R8G8B8A8_SINT,
		DXGI_FORMAT_R16G16_TYPELESS,
		DXGI_FORMAT_R16G16_FLOAT,
		DXGI_FORMAT_R16G16_UNORM,
		DXGI_FORMAT_R16G16_UINT,
		DXGI_FORMAT_R16G16_SNORM,
		DXGI_FORMAT_R16G16_SINT,
		DXGI_FORMAT_R32_TYPELESS,
		DXGI_FORMAT_D32_FLOAT,
		DXGI_FORMAT_R32_FLOAT,
		DXGI_FORMAT_R32_UINT,
		DXGI_FORMAT_R32_SINT,
		DXGI_FORMAT_R24G8_TYPELESS,
		DXGI_FORMAT_D24_UNORM_S8_UINT,
		DXGI_FORMAT_R24_UNORM_X8_TYPELESS,
		DXGI_FORMAT_X24_TYPELESS_G8_UINT,
		DXGI_FORMAT_R8G8_TYPELESS,
		DXGI_FORMAT_R8G8_UNORM,
		DXGI_FORMAT_R8G8_UINT,
		DXGI_FORMAT_R8G8_SNORM,
		DXGI_FORMAT_R8G8_SINT,
		DXGI_FORMAT_R16_TYPELESS,
		DXGI_FORMAT_R16_FLOAT,
		DXGI_FORMAT_D16_UNORM,
		DXGI_FORMAT_R16_UNORM,
		DXGI_FORMAT_R16_UINT,
		DXGI_FORMAT_R16_SNORM,
		DXGI_FORMAT_R16_SINT,
		DXGI_FORMAT_R8_TYPELESS,
		DXGI_FORMAT_R8_UNORM,
		DXGI_FORMAT_R8_UINT,
		DXGI_FORMAT_R8_SNORM,
		DXGI_FORMAT_R8_SINT,
		DXGI_FORMAT_A8_UNORM,
		DXGI_FORMAT_R1_UNORM,
		DXGI_FORMAT_R9G9B9E5_SHAREDEXP,
		DXGI_FORMAT_R8G8_B8G8_UNORM,
		DXGI_FORMAT_G8R8_G8B8_UNORM,
		DXGI_FORMAT_BC1_TYPELESS,
		DXGI_FORMAT_BC1_UNORM,
		DXGI_FORMAT_BC1_UNORM_SRGB,
		DXGI_FORMAT_BC2_TYPELESS,
		DXGI_FORMAT_BC2_UNORM,
		DXGI_FORMAT_BC2_UNORM_SRGB,
		DXGI_FORMAT_BC3_TYPELESS,
		DXGI_FORMAT_BC3_UNORM,
		DXGI_FORMAT_BC3_UNORM_SRGB,
		DXGI_FORMAT_BC4_TYPELESS,
		DXGI_FORMAT_BC4_UNORM,
		DXGI_FORMAT_BC4_SNORM,
		DXGI_FORMAT_BC5_TYPELESS,
		DXGI_FORMAT_BC5_UNORM,
		DXGI_FORMAT_BC5_SNORM,
		DXGI_FORMAT_B5G6R5_UNORM,
		DXGI_FORMAT_B5G5R5A1_UNORM,
		DXGI_FORMAT_B8G8R8A8_UNORM,
		DXGI_FORMAT_B8G8R8X8_UNORM,
		DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM,
		DXGI_FORMAT_B8G8R8A8_TYPELESS,
		DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
		DXGI_FORMAT_B8G8R8X8_TYPELESS,
		DXGI_FORMAT_B8G8R8X8_UNORM_SRGB,
		DXGI_FORMAT_BC6H_TYPELESS,
		DXGI_FORMAT_BC6H_UF16,
		DXGI_FORMAT_BC6H_SF16,
		DXGI_FORMAT_BC7_TYPELESS,
		DXGI_FORMAT_BC7_UNORM,
		DXGI_FORMAT_BC7_UNORM_SRGB,
		DXGI_FORMAT_AYUV,
		DXGI_FORMAT_Y410,
		DXGI_FORMAT_Y416,
		DXGI_FORMAT_NV12,
		DXGI_FORMAT_P010,
		DXGI_FORMAT_P016,
		DXGI_FORMAT_420_OPAQUE,
		DXGI_FORMAT_YUY2,
		DXGI_FORMAT_Y210,
		DXGI_FORMAT_Y216,
		DXGI_FORMAT_NV11,
		DXGI_FORMAT_AI44,
		DXGI_FORMAT_IA44,
		DXGI_FORMAT_P8,
		DXGI_FORMAT_A8P8,
		DXGI_FORMAT_B4G4R4A4_UNORM,

		DXGI_FORMAT_P208,
		DXGI_FORMAT_V208,
		DXGI_FORMAT_V408,

		DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE,
		DXGI_FORMAT_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE,

		DXGI_FORMAT_FORCE_UINT
	};

	return formats[static_cast<uint32_t>(format)];
}

D3D12_COMMAND_LIST_TYPE XUSG::GetDX12CommandListType(CommandListType commandListType)
{
	static const D3D12_COMMAND_LIST_TYPE commandListTypes[] =
	{
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		D3D12_COMMAND_LIST_TYPE_BUNDLE,
		D3D12_COMMAND_LIST_TYPE_COMPUTE,
		D3D12_COMMAND_LIST_TYPE_COPY,
		D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE,
		D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS,
		D3D12_COMMAND_LIST_TYPE_VIDEO_ENCODE
	};

	return commandListTypes[static_cast<uint32_t>(commandListType)];
}

D3D12_INPUT_CLASSIFICATION XUSG::GetDX12InputClassification(InputClassification inputClassification)
{
	static const D3D12_INPUT_CLASSIFICATION inputClassifications[] =
	{
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA
	};

	return inputClassifications[static_cast<uint32_t>(inputClassification)];
}

D3D12_HEAP_TYPE XUSG::GetDX12HeapType(MemoryType memoryType)
{
	static const D3D12_HEAP_TYPE heapTypes[] =
	{
		D3D12_HEAP_TYPE_DEFAULT,
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_HEAP_TYPE_READBACK,
		D3D12_HEAP_TYPE_CUSTOM
	};

	return heapTypes[static_cast<uint32_t>(memoryType)];
}

D3D12_PRIMITIVE_TOPOLOGY_TYPE XUSG::GetDX12PrimitiveTopologyType(PrimitiveTopologyType primitiveTopologyType)
{
	static const D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopologyTypes[] =
	{
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED,
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT,
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE,
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH
	};

	return primitiveTopologyTypes[static_cast<uint32_t>(primitiveTopologyType)];
}

D3D_PRIMITIVE_TOPOLOGY XUSG::GetDX12PrimitiveTopology(PrimitiveTopology primitiveTopology)
{
	static const D3D_PRIMITIVE_TOPOLOGY primitiveTopologies[] =
	{
		D3D_PRIMITIVE_TOPOLOGY_UNDEFINED,
		D3D_PRIMITIVE_TOPOLOGY_POINTLIST,
		D3D_PRIMITIVE_TOPOLOGY_LINELIST,
		D3D_PRIMITIVE_TOPOLOGY_LINESTRIP,
		D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
		D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
		D3D_PRIMITIVE_TOPOLOGY_TRIANGLEFAN,
		D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ,
		D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ,
		D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ,
		D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ,
		D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST,
		D3D_PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST,
		D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST,
		D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST,
		D3D_PRIMITIVE_TOPOLOGY_5_CONTROL_POINT_PATCHLIST,
		D3D_PRIMITIVE_TOPOLOGY_6_CONTROL_POINT_PATCHLIST,
		D3D_PRIMITIVE_TOPOLOGY_7_CONTROL_POINT_PATCHLIST,
		D3D_PRIMITIVE_TOPOLOGY_8_CONTROL_POINT_PATCHLIST,
		D3D_PRIMITIVE_TOPOLOGY_9_CONTROL_POINT_PATCHLIST,
		D3D_PRIMITIVE_TOPOLOGY_10_CONTROL_POINT_PATCHLIST,
		D3D_PRIMITIVE_TOPOLOGY_11_CONTROL_POINT_PATCHLIST,
		D3D_PRIMITIVE_TOPOLOGY_12_CONTROL_POINT_PATCHLIST,
		D3D_PRIMITIVE_TOPOLOGY_13_CONTROL_POINT_PATCHLIST,
		D3D_PRIMITIVE_TOPOLOGY_14_CONTROL_POINT_PATCHLIST,
		D3D_PRIMITIVE_TOPOLOGY_15_CONTROL_POINT_PATCHLIST,
		D3D_PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST,
		D3D_PRIMITIVE_TOPOLOGY_17_CONTROL_POINT_PATCHLIST,
		D3D_PRIMITIVE_TOPOLOGY_18_CONTROL_POINT_PATCHLIST,
		D3D_PRIMITIVE_TOPOLOGY_19_CONTROL_POINT_PATCHLIST,
		D3D_PRIMITIVE_TOPOLOGY_20_CONTROL_POINT_PATCHLIST,
		D3D_PRIMITIVE_TOPOLOGY_21_CONTROL_POINT_PATCHLIST,
		D3D_PRIMITIVE_TOPOLOGY_22_CONTROL_POINT_PATCHLIST,
		D3D_PRIMITIVE_TOPOLOGY_23_CONTROL_POINT_PATCHLIST,
		D3D_PRIMITIVE_TOPOLOGY_24_CONTROL_POINT_PATCHLIST,
		D3D_PRIMITIVE_TOPOLOGY_25_CONTROL_POINT_PATCHLIST,
		D3D_PRIMITIVE_TOPOLOGY_26_CONTROL_POINT_PATCHLIST,
		D3D_PRIMITIVE_TOPOLOGY_27_CONTROL_POINT_PATCHLIST,
		D3D_PRIMITIVE_TOPOLOGY_28_CONTROL_POINT_PATCHLIST,
		D3D_PRIMITIVE_TOPOLOGY_29_CONTROL_POINT_PATCHLIST,
		D3D_PRIMITIVE_TOPOLOGY_30_CONTROL_POINT_PATCHLIST,
		D3D_PRIMITIVE_TOPOLOGY_31_CONTROL_POINT_PATCHLIST,
		D3D_PRIMITIVE_TOPOLOGY_32_CONTROL_POINT_PATCHLIST
	};

	return primitiveTopologies[static_cast<uint32_t>(primitiveTopology)];
}

D3D12_FILL_MODE XUSG::GetDX12FillMode(FillMode fillMode)
{
	static const D3D12_FILL_MODE fillModes[] =
	{
		D3D12_FILL_MODE_WIREFRAME,
		D3D12_FILL_MODE_SOLID
	};

	return fillModes[static_cast<uint32_t>(fillMode)];
}

D3D12_CULL_MODE XUSG::GetDX12CullMode(CullMode cullMode)
{
	static const D3D12_CULL_MODE cullModes[] =
	{
		D3D12_CULL_MODE_NONE,
		D3D12_CULL_MODE_FRONT,
		D3D12_CULL_MODE_BACK
	};

	return cullModes[static_cast<uint32_t>(cullMode)];
}

D3D12_INDIRECT_ARGUMENT_TYPE XUSG::GetDX12IndirectArgumentType(IndirectArgumentType indirectArgumentType)
{
	static const D3D12_INDIRECT_ARGUMENT_TYPE indirectArgumentTypes[] =
	{
		D3D12_INDIRECT_ARGUMENT_TYPE_DRAW,
		D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED,
		D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH,
		D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW,
		D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW,
		D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT,
		D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW,
		D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW,
		D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW
	};

	return indirectArgumentTypes[static_cast<uint32_t>(indirectArgumentType)];
}

D3D12_RESOURCE_DIMENSION XUSG::GetDX12ResourceDimension(ResourceDimension resourceDimension)
{
	static const D3D12_RESOURCE_DIMENSION resourceDimensions[] =
	{
		D3D12_RESOURCE_DIMENSION_UNKNOWN,
		D3D12_RESOURCE_DIMENSION_BUFFER,
		D3D12_RESOURCE_DIMENSION_TEXTURE1D,
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		D3D12_RESOURCE_DIMENSION_TEXTURE3D
	};

	return resourceDimensions[static_cast<uint32_t>(resourceDimension)];
}

D3D12_COMMAND_QUEUE_FLAGS XUSG::GetDX12CommandQueueFlag(CommandQueueFlag commandQueueFlag)
{
	static const D3D12_COMMAND_QUEUE_FLAGS commandQueueFlags[] =
	{
		D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT
	};

	if (commandQueueFlag == CommandQueueFlag::NONE) return D3D12_COMMAND_QUEUE_FLAG_NONE;

	const auto index = Log2(static_cast<uint32_t>(commandQueueFlag));

	return commandQueueFlags[index];
}

D3D12_COMMAND_QUEUE_FLAGS XUSG::GetDX12CommandQueueFlags(CommandQueueFlag commandQueueFlags)
{
	auto flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	flags |= APPEND_COMMAND_QUEUE_FLAG(commandQueueFlags, DISABLE_GPU_TIMEOUT);
	
	return flags;
}

D3D12_HEAP_FLAGS XUSG::GetDX12HeapFlag(MemoryFlag memoryFlag)
{
	static const D3D12_HEAP_FLAGS heapFlags[] =
	{
		D3D12_HEAP_FLAG_SHARED,
		D3D12_HEAP_FLAG_DENY_BUFFERS,
		D3D12_HEAP_FLAG_ALLOW_DISPLAY,
		D3D12_HEAP_FLAG_SHARED_CROSS_ADAPTER,
		D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES,
		D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES,
		D3D12_HEAP_FLAG_HARDWARE_PROTECTED,
		D3D12_HEAP_FLAG_ALLOW_WRITE_WATCH,
		D3D12_HEAP_FLAG_ALLOW_SHADER_ATOMICS,
		D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT,
		D3D12_HEAP_FLAG_CREATE_NOT_ZEROED
	};

	if (memoryFlag == MemoryFlag::NONE) return D3D12_HEAP_FLAG_NONE;

	const auto index = Log2(static_cast<uint32_t>(memoryFlag));

	return heapFlags[index];
}

D3D12_HEAP_FLAGS XUSG::GetDX12HeapFlags(MemoryFlag memoryFlags)
{
	auto flags = D3D12_HEAP_FLAG_NONE;
	flags |= APPEND_HEAP_FLAG(memoryFlags, SHARED);
	flags |= APPEND_HEAP_FLAG(memoryFlags, DENY_BUFFERS);
	flags |= APPEND_HEAP_FLAG(memoryFlags, ALLOW_DISPLAY);
	flags |= APPEND_HEAP_FLAG(memoryFlags, SHARED_CROSS_ADAPTER);
	flags |= APPEND_HEAP_FLAG(memoryFlags, DENY_RT_DS_TEXTURES);
	flags |= APPEND_HEAP_FLAG(memoryFlags, DENY_NON_RT_DS_TEXTURES);
	flags |= APPEND_HEAP_FLAG(memoryFlags, HARDWARE_PROTECTED);
	flags |= APPEND_HEAP_FLAG(memoryFlags, ALLOW_WRITE_WATCH);
	flags |= APPEND_HEAP_FLAG(memoryFlags, ALLOW_SHADER_ATOMICS);
	flags |= APPEND_HEAP_FLAG(memoryFlags, CREATE_NOT_RESIDENT);
	flags |= APPEND_HEAP_FLAG(memoryFlags, CREATE_NOT_ZEROED);

	return flags;
}

D3D12_RESOURCE_FLAGS XUSG::GetDX12ResourceFlag(ResourceFlag resourceFlag)
{
	static const D3D12_RESOURCE_FLAGS resourceFlags[] =
	{
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE,
		D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER,
		D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS,
		D3D12_RESOURCE_FLAG_VIDEO_DECODE_REFERENCE_ONLY
	};

	if (resourceFlag == ResourceFlag::NONE) return D3D12_RESOURCE_FLAG_NONE;

	const auto index = Log2(static_cast<uint32_t>(resourceFlag));

	return resourceFlags[index];
}

D3D12_RESOURCE_FLAGS XUSG::GetDX12ResourceFlags(ResourceFlag resourceFlags)
{
	resourceFlags &= REMOVE_PACKED_UAV;
	resourceFlags &= REMOVE_RAYTRACING_AS;

	auto flags = D3D12_RESOURCE_FLAG_NONE;
	flags |= APPEND_RESOURCE_FLAG(resourceFlags, ALLOW_RENDER_TARGET);
	flags |= APPEND_RESOURCE_FLAG(resourceFlags, ALLOW_DEPTH_STENCIL);
	flags |= APPEND_RESOURCE_FLAG(resourceFlags, ALLOW_UNORDERED_ACCESS);
	flags |= APPEND_RESOURCE_FLAG(resourceFlags, DENY_SHADER_RESOURCE);
	flags |= APPEND_RESOURCE_FLAG(resourceFlags, ALLOW_CROSS_ADAPTER);
	flags |= APPEND_RESOURCE_FLAG(resourceFlags, ALLOW_SIMULTANEOUS_ACCESS);
	flags |= APPEND_RESOURCE_FLAG(resourceFlags, VIDEO_DECODE_REFERENCE_ONLY);

	return flags;
}

D3D12_RESOURCE_STATES XUSG::GetDX12ResourceState(ResourceState resourceState)
{
	static const D3D12_RESOURCE_STATES resourceStates[] =
	{
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
		D3D12_RESOURCE_STATE_INDEX_BUFFER,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		D3D12_RESOURCE_STATE_DEPTH_READ,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_STREAM_OUT,
		D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_COPY_SOURCE,
		D3D12_RESOURCE_STATE_RESOLVE_DEST,
		D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
		D3D12_RESOURCE_STATE_PREDICATION,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE,
		D3D12_RESOURCE_STATE_VIDEO_DECODE_READ,
		D3D12_RESOURCE_STATE_VIDEO_DECODE_WRITE,
		D3D12_RESOURCE_STATE_VIDEO_PROCESS_READ,
		D3D12_RESOURCE_STATE_VIDEO_PROCESS_WRITE,
		D3D12_RESOURCE_STATE_VIDEO_ENCODE_READ,
		D3D12_RESOURCE_STATE_VIDEO_ENCODE_WRITE
	};

	if (resourceState == ResourceState::COMMON) return D3D12_RESOURCE_STATE_COMMON;

	const auto index = Log2(static_cast<uint32_t>(resourceState));

	return resourceStates[index];
}

D3D12_RESOURCE_STATES XUSG::GetDX12ResourceStates(ResourceState resourceStates)
{
	auto states = D3D12_RESOURCE_STATE_COMMON;
	states |= APPEND_RESOURCE_STATE(resourceStates, VERTEX_AND_CONSTANT_BUFFER);
	states |= APPEND_RESOURCE_STATE(resourceStates, INDEX_BUFFER);
	states |= APPEND_RESOURCE_STATE(resourceStates, RENDER_TARGET);
	states |= APPEND_RESOURCE_STATE(resourceStates, UNORDERED_ACCESS);
	states |= APPEND_RESOURCE_STATE(resourceStates, DEPTH_WRITE);
	states |= APPEND_RESOURCE_STATE(resourceStates, DEPTH_READ);
	states |= APPEND_RESOURCE_STATE(resourceStates, NON_PIXEL_SHADER_RESOURCE);
	states |= APPEND_RESOURCE_STATE(resourceStates, PIXEL_SHADER_RESOURCE);
	states |= APPEND_RESOURCE_STATE(resourceStates, STREAM_OUT);
	states |= APPEND_RESOURCE_STATE(resourceStates, INDIRECT_ARGUMENT);
	states |= APPEND_RESOURCE_STATE(resourceStates, COPY_DEST);
	states |= APPEND_RESOURCE_STATE(resourceStates, COPY_SOURCE);
	states |= APPEND_RESOURCE_STATE(resourceStates, RESOLVE_DEST);
	states |= APPEND_RESOURCE_STATE(resourceStates, RESOLVE_SOURCE);
	states |= APPEND_RESOURCE_STATE(resourceStates, PREDICATION);
	states |= APPEND_RESOURCE_STATE(resourceStates, RAYTRACING_ACCELERATION_STRUCTURE);
	states |= APPEND_RESOURCE_STATE(resourceStates, SHADING_RATE_SOURCE);
	states |= APPEND_RESOURCE_STATE(resourceStates, VIDEO_DECODE_READ);
	states |= APPEND_RESOURCE_STATE(resourceStates, VIDEO_DECODE_WRITE);
	states |= APPEND_RESOURCE_STATE(resourceStates, VIDEO_PROCESS_READ);
	states |= APPEND_RESOURCE_STATE(resourceStates, VIDEO_PROCESS_WRITE);
	states |= APPEND_RESOURCE_STATE(resourceStates, VIDEO_ENCODE_READ);
	states |= APPEND_RESOURCE_STATE(resourceStates, VIDEO_ENCODE_WRITE);

	return states;
}

D3D12_RESOURCE_BARRIER_FLAGS XUSG::GetDX12BarrierFlag(BarrierFlag barrierFlag)
{
	static const D3D12_RESOURCE_BARRIER_FLAGS barrierFlags[] =
	{
		D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY,
		D3D12_RESOURCE_BARRIER_FLAG_END_ONLY
	};

	if (barrierFlag == BarrierFlag::NONE) return D3D12_RESOURCE_BARRIER_FLAG_NONE;

	const auto index = Log2(static_cast<uint32_t>(barrierFlag));

	return barrierFlags[index];
}

D3D12_RESOURCE_BARRIER_FLAGS XUSG::GetDX12BarrierFlags(BarrierFlag barrierFlags)
{
	auto flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	flags |= APPEND_BARRIER_FLAG(barrierFlags, BEGIN_ONLY);
	flags |= APPEND_BARRIER_FLAG(barrierFlags, END_ONLY);
	
	return flags;
}

D3D12_DESCRIPTOR_RANGE_FLAGS XUSG::GetDX12DescriptorRangeFlag(DescriptorFlag descriptorFlag)
{
	static const D3D12_DESCRIPTOR_RANGE_FLAGS descriptorRangeFlags[] =
	{
		D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE,
		D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE,
		D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE,
		D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC,
		D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_STATIC_KEEPING_BUFFER_BOUNDS_CHECKS
	};

	if (descriptorFlag == DescriptorFlag::NONE) return D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

	const auto index = Log2(static_cast<uint32_t>(descriptorFlag));

	return descriptorRangeFlags[index];
}

D3D12_DESCRIPTOR_RANGE_FLAGS XUSG::GetDX12DescriptorRangeFlags(DescriptorFlag descriptorFlags)
{
	auto flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
	flags |= APPEND_DESCRIPTOR_RANGE_FLAG(descriptorFlags, DESCRIPTORS_VOLATILE);
	flags |= APPEND_DESCRIPTOR_RANGE_FLAG(descriptorFlags, DATA_VOLATILE);
	flags |= APPEND_DESCRIPTOR_RANGE_FLAG(descriptorFlags, DATA_STATIC_WHILE_SET_AT_EXECUTE);
	flags |= APPEND_DESCRIPTOR_RANGE_FLAG(descriptorFlags, DATA_STATIC);
	flags |= APPEND_DESCRIPTOR_RANGE_FLAG(descriptorFlags, DESCRIPTORS_STATIC_KEEPING_BUFFER_BOUNDS_CHECKS);

	return flags;
}

D3D12_ROOT_DESCRIPTOR_FLAGS XUSG::GetDX12RootDescriptorFlag(DescriptorFlag descriptorFlag)
{
	static const D3D12_ROOT_DESCRIPTOR_FLAGS rootDescriptorFlags[] =
	{
		D3D12_ROOT_DESCRIPTOR_FLAGS(1),
		D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE,
		D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE,
		D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC
	};

	if (descriptorFlag == DescriptorFlag::NONE ||
		descriptorFlag == DescriptorFlag::DESCRIPTORS_VOLATILE ||
		descriptorFlag == DescriptorFlag::DESCRIPTORS_STATIC_KEEPING_BUFFER_BOUNDS_CHECKS)
		return D3D12_ROOT_DESCRIPTOR_FLAG_NONE;

	const auto index = Log2(static_cast<uint32_t>(descriptorFlag));

	return rootDescriptorFlags[index];
}

D3D12_ROOT_DESCRIPTOR_FLAGS XUSG::GetDX12RootDescriptorFlags(DescriptorFlag descriptorFlags)
{
	auto flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
	flags |= APPEND_ROOT_DESCRIPTOR_FLAG(descriptorFlags, DATA_VOLATILE);
	flags |= APPEND_ROOT_DESCRIPTOR_FLAG(descriptorFlags, DATA_STATIC_WHILE_SET_AT_EXECUTE);
	flags |= APPEND_ROOT_DESCRIPTOR_FLAG(descriptorFlags, DATA_STATIC);

	return flags;
}

D3D12_ROOT_SIGNATURE_FLAGS XUSG::GetDX12RootSignatureFlag(PipelineLayoutFlag pipelineLayoutFlag)
{
	static const D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags[] =
	{
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
		D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS,
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS,
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS,
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS,
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT,
		D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE,
		D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS,
		D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS,
		D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED,
		D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED
	};

	if (pipelineLayoutFlag == PipelineLayoutFlag::NONE) return D3D12_ROOT_SIGNATURE_FLAG_NONE;

	const auto index = Log2(static_cast<uint32_t>(pipelineLayoutFlag));

	return rootSignatureFlags[index];
}

D3D12_ROOT_SIGNATURE_FLAGS XUSG::GetDX12RootSignatureFlags(PipelineLayoutFlag pipelineLayoutFlags)
{
	auto flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
	flags |= APPEND_ROOT_SIGNATURE_FLAG(pipelineLayoutFlags, ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	flags |= APPEND_ROOT_SIGNATURE_FLAG(pipelineLayoutFlags, DENY_VERTEX_SHADER_ROOT_ACCESS);
	flags |= APPEND_ROOT_SIGNATURE_FLAG(pipelineLayoutFlags, DENY_HULL_SHADER_ROOT_ACCESS);
	flags |= APPEND_ROOT_SIGNATURE_FLAG(pipelineLayoutFlags, DENY_DOMAIN_SHADER_ROOT_ACCESS);
	flags |= APPEND_ROOT_SIGNATURE_FLAG(pipelineLayoutFlags, DENY_GEOMETRY_SHADER_ROOT_ACCESS);
	flags |= APPEND_ROOT_SIGNATURE_FLAG(pipelineLayoutFlags, DENY_PIXEL_SHADER_ROOT_ACCESS);
	flags |= APPEND_ROOT_SIGNATURE_FLAG(pipelineLayoutFlags, ALLOW_STREAM_OUTPUT);
	flags |= static_cast<bool>(pipelineLayoutFlags & PipelineLayoutFlag::LOCAL_PIPELINE_LAYOUT) ?
		D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE :
		D3D12_ROOT_SIGNATURE_FLAG_NONE;
	flags |= APPEND_ROOT_SIGNATURE_FLAG(pipelineLayoutFlags, DENY_AMPLIFICATION_SHADER_ROOT_ACCESS);
	flags |= APPEND_ROOT_SIGNATURE_FLAG(pipelineLayoutFlags, DENY_MESH_SHADER_ROOT_ACCESS);
	flags |= static_cast<bool>(pipelineLayoutFlags & PipelineLayoutFlag::CBV_SRV_UAV_POOL_DIRECTLY_INDEXED) ?
		D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED :
		D3D12_ROOT_SIGNATURE_FLAG_NONE;
	flags |= static_cast<bool>(pipelineLayoutFlags & PipelineLayoutFlag::SAMPLER_POOL_DIRECTLY_INDEXED) ?
		D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED :
		D3D12_ROOT_SIGNATURE_FLAG_NONE;
	
	return flags;
}

D3D12_CLEAR_FLAGS XUSG::GetDX12ClearFlag(ClearFlag clearFlag)
{
	static const D3D12_CLEAR_FLAGS clearFlags[] =
	{
		D3D12_CLEAR_FLAGS(0),
		D3D12_CLEAR_FLAG_DEPTH,
		D3D12_CLEAR_FLAG_STENCIL,
	};

	if (clearFlag == ClearFlag::NONE) return D3D12_CLEAR_FLAGS(0);

	const auto index = Log2(static_cast<uint32_t>(clearFlag));

	return clearFlags[index];
}

D3D12_CLEAR_FLAGS XUSG::GetDX12ClearFlags(ClearFlag clearFlags)
{
	auto flags = D3D12_CLEAR_FLAGS(0);
	flags |= (clearFlags & ClearFlag::DEPTH) == ClearFlag::DEPTH ? D3D12_CLEAR_FLAG_DEPTH : D3D12_CLEAR_FLAGS(0);
	flags |= (clearFlags & ClearFlag::STENCIL) == ClearFlag::DEPTH ? D3D12_CLEAR_FLAG_STENCIL : D3D12_CLEAR_FLAGS(0);

	return flags;
}

D3D12_FENCE_FLAGS XUSG::GetDX12FenceFlag(FenceFlag fenceFlag)
{
	static const D3D12_FENCE_FLAGS fenceFlags[] =
	{
		D3D12_FENCE_FLAG_SHARED,
		D3D12_FENCE_FLAG_SHARED_CROSS_ADAPTER,
		D3D12_FENCE_FLAG_NON_MONITORED
	};

	if (fenceFlag == FenceFlag::NONE) return D3D12_FENCE_FLAG_NONE;

	const auto index = Log2(static_cast<uint32_t>(fenceFlag));

	return fenceFlags[index];
}

D3D12_FENCE_FLAGS XUSG::GetDX12FenceFlags(FenceFlag fenceFlags)
{
	auto flags = D3D12_FENCE_FLAG_NONE;
	flags |= APPEND_FENCE_FLAG(fenceFlags, SHARED);
	flags |= APPEND_FENCE_FLAG(fenceFlags, SHARED_CROSS_ADAPTER);
	flags |= APPEND_FENCE_FLAG(fenceFlags, NON_MONITORED);

	return flags;
}

D3D12_BLEND XUSG::GetDX12Blend(BlendFactor blend)
{
	static const D3D12_BLEND blends[] =
	{
		D3D12_BLEND_ZERO,
		D3D12_BLEND_ONE,
		D3D12_BLEND_SRC_COLOR,
		D3D12_BLEND_INV_SRC_COLOR,
		D3D12_BLEND_SRC_ALPHA,
		D3D12_BLEND_INV_SRC_ALPHA,
		D3D12_BLEND_DEST_ALPHA,
		D3D12_BLEND_INV_DEST_ALPHA,
		D3D12_BLEND_DEST_COLOR,
		D3D12_BLEND_INV_DEST_COLOR,
		D3D12_BLEND_SRC_ALPHA_SAT,
		D3D12_BLEND_BLEND_FACTOR,
		D3D12_BLEND_INV_BLEND_FACTOR,
		D3D12_BLEND_SRC1_COLOR,
		D3D12_BLEND_INV_SRC1_COLOR,
		D3D12_BLEND_SRC1_ALPHA,
		D3D12_BLEND_INV_SRC1_ALPHA
	};

	return blends[static_cast<uint32_t>(blend)];
}

D3D12_BLEND_OP XUSG::GetDX12BlendOp(BlendOperator blendOp)
{
	static const D3D12_BLEND_OP blendOps[] =
	{
		D3D12_BLEND_OP_ADD,
		D3D12_BLEND_OP_SUBTRACT,
		D3D12_BLEND_OP_REV_SUBTRACT,
		D3D12_BLEND_OP_MIN,
		D3D12_BLEND_OP_MAX
	};

	return blendOps[static_cast<uint32_t>(blendOp)];
}

D3D12_LOGIC_OP XUSG::GetDX12LogicOp(LogicOperator logicOp)
{
	static const D3D12_LOGIC_OP logicOps[] =
	{
		D3D12_LOGIC_OP_CLEAR,
		D3D12_LOGIC_OP_SET,
		D3D12_LOGIC_OP_COPY,
		D3D12_LOGIC_OP_COPY_INVERTED,
		D3D12_LOGIC_OP_NOOP,
		D3D12_LOGIC_OP_INVERT,
		D3D12_LOGIC_OP_AND,
		D3D12_LOGIC_OP_NAND,
		D3D12_LOGIC_OP_OR,
		D3D12_LOGIC_OP_NOR,
		D3D12_LOGIC_OP_XOR,
		D3D12_LOGIC_OP_EQUIV,
		D3D12_LOGIC_OP_AND_REVERSE,
		D3D12_LOGIC_OP_AND_INVERTED,
		D3D12_LOGIC_OP_OR_REVERSE,
		D3D12_LOGIC_OP_OR_INVERTED
	};

	return logicOps[static_cast<uint32_t>(logicOp)];
}

D3D12_COLOR_WRITE_ENABLE XUSG::GetDX12ColorWrite(ColorWrite writeMask)
{
	static const D3D12_COLOR_WRITE_ENABLE writeMasks[] =
	{
		D3D12_COLOR_WRITE_ENABLE_RED,
		D3D12_COLOR_WRITE_ENABLE_GREEN,
		D3D12_COLOR_WRITE_ENABLE_BLUE,
		D3D12_COLOR_WRITE_ENABLE_ALPHA,
		D3D12_COLOR_WRITE_ENABLE_ALL
	};

	return writeMasks[static_cast<uint32_t>(writeMask)];
}

D3D12_COMPARISON_FUNC XUSG::GetDX12ComparisonFunc(ComparisonFunc comparisonFunc)
{
	static const D3D12_COMPARISON_FUNC comparisonFuncs[] =
	{
		D3D12_COMPARISON_FUNC_NEVER,
		D3D12_COMPARISON_FUNC_LESS,
		D3D12_COMPARISON_FUNC_EQUAL,
		D3D12_COMPARISON_FUNC_LESS_EQUAL,
		D3D12_COMPARISON_FUNC_GREATER,
		D3D12_COMPARISON_FUNC_NOT_EQUAL,
		D3D12_COMPARISON_FUNC_GREATER_EQUAL,
		D3D12_COMPARISON_FUNC_ALWAYS
	};

	return comparisonFuncs[static_cast<uint32_t>(comparisonFunc)];
}

D3D12_STENCIL_OP XUSG::GetDX12StencilOp(StencilOp stencilOp)
{
	static const D3D12_STENCIL_OP stencilOps[] =
	{
		D3D12_STENCIL_OP_KEEP,
		D3D12_STENCIL_OP_ZERO,
		D3D12_STENCIL_OP_REPLACE,
		D3D12_STENCIL_OP_INCR_SAT,
		D3D12_STENCIL_OP_DECR_SAT,
		D3D12_STENCIL_OP_INVERT,
		D3D12_STENCIL_OP_INCR,
		D3D12_STENCIL_OP_DECR
	};

	return stencilOps[static_cast<uint32_t>(stencilOp)];
}

D3D12_QUERY_TYPE XUSG::GetDX12QueryType(QueryType type)
{
	static const D3D12_QUERY_TYPE types[] =
	{
		D3D12_QUERY_TYPE_OCCLUSION,
		D3D12_QUERY_TYPE_BINARY_OCCLUSION,
		D3D12_QUERY_TYPE_TIMESTAMP,
		D3D12_QUERY_TYPE_PIPELINE_STATISTICS,
		D3D12_QUERY_TYPE_SO_STATISTICS_STREAM0,
		D3D12_QUERY_TYPE_SO_STATISTICS_STREAM1,
		D3D12_QUERY_TYPE_SO_STATISTICS_STREAM2,
		D3D12_QUERY_TYPE_SO_STATISTICS_STREAM3,
		D3D12_QUERY_TYPE_VIDEO_DECODE_STATISTICS
	};

	return types[static_cast<uint32_t>(type)];
}

D3D12_TILE_COPY_FLAGS XUSG::GetDX12TileCopyFlag(TileCopyFlag tileCopyFlag)
{
	static const D3D12_TILE_COPY_FLAGS fenceFlags[] =
	{
		D3D12_TILE_COPY_FLAG_NO_HAZARD,
		D3D12_TILE_COPY_FLAG_LINEAR_BUFFER_TO_SWIZZLED_TILED_RESOURCE,
		D3D12_TILE_COPY_FLAG_SWIZZLED_TILED_RESOURCE_TO_LINEAR_BUFFER
	};

	if (tileCopyFlag == TileCopyFlag::NONE) return D3D12_TILE_COPY_FLAG_NONE;

	const auto index = Log2(static_cast<uint32_t>(tileCopyFlag));

	return fenceFlags[index];
}

D3D12_TILE_COPY_FLAGS XUSG::GetDX12TileCopyFlags(TileCopyFlag tileCopyFlags)
{
	auto flags = D3D12_TILE_COPY_FLAG_NONE;
	flags |= APPEND_TILE_COPY_FLAG(tileCopyFlags, NO_HAZARD);
	flags |= APPEND_TILE_COPY_FLAG(tileCopyFlags, LINEAR_BUFFER_TO_SWIZZLED_TILED_RESOURCE);
	flags |= APPEND_TILE_COPY_FLAG(tileCopyFlags, SWIZZLED_TILED_RESOURCE_TO_LINEAR_BUFFER);

	return flags;
}

D3D12_FILTER XUSG::GetDX12Filter(SamplerFilter filter)
{
	static const D3D12_FILTER filters[] =
	{
		D3D12_FILTER_MIN_MAG_MIP_POINT,
		D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR,
		D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT,
		D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR,
		D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT,
		D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
		D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_FILTER_ANISOTROPIC,
		D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT,
		D3D12_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR,
		D3D12_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT,
		D3D12_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR,
		D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT,
		D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
		D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
		D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
		D3D12_FILTER_COMPARISON_ANISOTROPIC,
		D3D12_FILTER_MINIMUM_MIN_MAG_MIP_POINT,
		D3D12_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR,
		D3D12_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT,
		D3D12_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR,
		D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT,
		D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
		D3D12_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT,
		D3D12_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR,
		D3D12_FILTER_MINIMUM_ANISOTROPIC,
		D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_POINT,
		D3D12_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR,
		D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT,
		D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR,
		D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT,
		D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
		D3D12_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT,
		D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR,
		D3D12_FILTER_MAXIMUM_ANISOTROPIC
	};

	return filters[static_cast<uint32_t>(filter)];
}

D3D12_TEXTURE_ADDRESS_MODE XUSG::GetDX12TextureAddressMode(TextureAddressMode textureAddressMode)
{
	static const D3D12_TEXTURE_ADDRESS_MODE textureAddressModes[] =
	{
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_MIRROR,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,
		D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE
	};

	return textureAddressModes[static_cast<uint32_t>(textureAddressMode)];
}

uint32_t XUSG::GetDX12Requirement(Requirement requirement)
{
	static const uint32_t requirements[] =
	{
		D3D12_REQ_MIP_LEVELS,
		D3D12_REQ_TEXTURECUBE_DIMENSION,
		D3D12_REQ_TEXTURE1D_U_DIMENSION,
		D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION,
		D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION,
		D3D12_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION,
		D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION
	};

	return requirements[static_cast<uint32_t>(requirement)];
}

uint32_t XUSG::GetDXGISwapChainFlag(SwapChainFlag swapChainFlag)
{
	static const DXGI_SWAP_CHAIN_FLAG swapChainFlags[] =
	{
		DXGI_SWAP_CHAIN_FLAG_NONPREROTATED,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH,
		DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE,
		DXGI_SWAP_CHAIN_FLAG_RESTRICTED_CONTENT,
		DXGI_SWAP_CHAIN_FLAG_RESTRICT_SHARED_RESOURCE_DRIVER,
		DXGI_SWAP_CHAIN_FLAG_DISPLAY_ONLY,
		DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT,
		DXGI_SWAP_CHAIN_FLAG_FOREGROUND_LAYER,
		DXGI_SWAP_CHAIN_FLAG_FULLSCREEN_VIDEO,
		DXGI_SWAP_CHAIN_FLAG_YUV_VIDEO,
		DXGI_SWAP_CHAIN_FLAG_HW_PROTECTED,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING,
		DXGI_SWAP_CHAIN_FLAG_RESTRICTED_TO_ALL_HOLOGRAPHIC_DISPLAYS
	};

	if (swapChainFlag == SwapChainFlag::NONE) return 0;

	const auto index = Log2(static_cast<uint32_t>(swapChainFlag));

	return swapChainFlags[index];
}

uint32_t XUSG::GetDXGISwapChainFlags(SwapChainFlag swapChainFlags)
{
	auto flags = 0u;
	flags |= APPEND_SWAP_CHAIN_FLAG(swapChainFlags, NONPREROTATED);
	flags |= APPEND_SWAP_CHAIN_FLAG(swapChainFlags, ALLOW_MODE_SWITCH);
	flags |= APPEND_SWAP_CHAIN_FLAG(swapChainFlags, GDI_COMPATIBLE);
	flags |= APPEND_SWAP_CHAIN_FLAG(swapChainFlags, RESTRICTED_CONTENT);
	flags |= APPEND_SWAP_CHAIN_FLAG(swapChainFlags, RESTRICT_SHARED_RESOURCE_DRIVER);
	flags |= APPEND_SWAP_CHAIN_FLAG(swapChainFlags, DISPLAY_ONLY);
	flags |= APPEND_SWAP_CHAIN_FLAG(swapChainFlags, FRAME_LATENCY_WAITABLE_OBJECT);
	flags |= APPEND_SWAP_CHAIN_FLAG(swapChainFlags, FOREGROUND_LAYER);
	flags |= APPEND_SWAP_CHAIN_FLAG(swapChainFlags, FULLSCREEN_VIDEO);
	flags |= APPEND_SWAP_CHAIN_FLAG(swapChainFlags, YUV_VIDEO);
	flags |= APPEND_SWAP_CHAIN_FLAG(swapChainFlags, HW_PROTECTED);
	flags |= APPEND_SWAP_CHAIN_FLAG(swapChainFlags, ALLOW_TEARING);
	flags |= APPEND_SWAP_CHAIN_FLAG(swapChainFlags, RESTRICTED_TO_ALL_HOLOGRAPHIC_DISPLAYS);

	return flags;
}

uint32_t XUSG::GetDXGIPresentFlag(PresentFlag presentFlag)
{
	static const uint32_t presentFlags[] =
	{
		DXGI_PRESENT_TEST,
		DXGI_PRESENT_DO_NOT_SEQUENCE,
		DXGI_PRESENT_RESTART,
		DXGI_PRESENT_DO_NOT_WAIT,
		DXGI_PRESENT_STEREO_PREFER_RIGHT,
		DXGI_PRESENT_STEREO_TEMPORARY_MONO,
		DXGI_PRESENT_RESTRICT_TO_OUTPUT,
		0,
		DXGI_PRESENT_USE_DURATION,
		DXGI_PRESENT_ALLOW_TEARING
	};

	if (presentFlag == PresentFlag::NONE) return 0;

	const auto index = Log2(static_cast<uint32_t>(presentFlag));

	return presentFlags[index];
}

uint32_t XUSG::GetDXGIPresentFlags(PresentFlag presentFlags)
{
	auto flags = 0u;
	flags |= APPEND_PRESENT_FLAG(presentFlags, TEST);
	flags |= APPEND_PRESENT_FLAG(presentFlags, DO_NOT_SEQUENCE);
	flags |= APPEND_PRESENT_FLAG(presentFlags, RESTART);
	flags |= APPEND_PRESENT_FLAG(presentFlags, DO_NOT_WAIT);
	flags |= APPEND_PRESENT_FLAG(presentFlags, STEREO_PREFER_RIGHT);
	flags |= APPEND_PRESENT_FLAG(presentFlags, STEREO_TEMPORARY_MONO);
	flags |= APPEND_PRESENT_FLAG(presentFlags, RESTRICT_TO_OUTPUT);
	flags |= APPEND_PRESENT_FLAG(presentFlags, USE_DURATION);
	flags |= APPEND_PRESENT_FLAG(presentFlags, ALLOW_TEARING);

	return flags;
}
