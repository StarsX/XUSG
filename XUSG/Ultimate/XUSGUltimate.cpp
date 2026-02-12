//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSGCommand_DX12.h"
#include "Core/XUSGResource_DX12.h"
#include "Core/XUSGPipelineLayout_DX12.h"
#include "Core/XUSGGraphicsState_DX12.h"
#include "XUSGUltimate_DX12.h"
#include "XUSGPipelineState_DX12.h"
#include "XUSGWorkGraph_DX12.h"
#include "XUSGGenericState_DX12.h"

using namespace std;
using namespace XUSG;

Ultimate::CommandList::uptr Ultimate::CommandList::MakeUnique(API api)
{
	return make_unique<CommandList_DX12>();
}

Ultimate::CommandList::sptr Ultimate::CommandList::MakeShared(API api)
{
	return make_shared<CommandList_DX12>();
}

Ultimate::CommandList::uptr Ultimate::CommandList::MakeUnique(XUSG::CommandList& commandList, API api)
{
	return make_unique<CommandList_DX12>(commandList);
}

Ultimate::CommandList::sptr Ultimate::CommandList::MakeShared(XUSG::CommandList& commandList, API api)
{
	return make_shared<CommandList_DX12>(commandList);
}

Ultimate::PipelineLayoutLib::uptr Ultimate::PipelineLayoutLib::MakeUnique(API api)
{
	return make_unique<PipelineLayoutLib_DX12>();
}

Ultimate::PipelineLayoutLib::sptr Ultimate::PipelineLayoutLib::MakeShared(API api)
{
	return make_shared<PipelineLayoutLib_DX12>();
}

Ultimate::PipelineLayoutLib::uptr Ultimate::PipelineLayoutLib::MakeUnique(const Device* pDevice, API api)
{
	return make_unique<PipelineLayoutLib_DX12>(pDevice);
}

Ultimate::PipelineLayoutLib::sptr Ultimate::PipelineLayoutLib::MakeShared(const Device* pDevice, API api)
{
	return make_shared<PipelineLayoutLib_DX12>(pDevice);
}

Ultimate::State::uptr Ultimate::State::MakeUnique(API api)
{
	return make_unique<State_DX12>();
}

Ultimate::State::sptr Ultimate::State::MakeShared(API api)
{
	return make_shared<State_DX12>();
}

Ultimate::PipelineLib::uptr Ultimate::PipelineLib::MakeUnique(API api)
{
	return make_unique<PipelineLib_DX12>();
}

Ultimate::PipelineLib::sptr Ultimate::PipelineLib::MakeShared(API api)
{
	return make_shared<PipelineLib_DX12>();
}

Ultimate::PipelineLib::uptr Ultimate::PipelineLib::MakeUnique(const Device* pDevice, API api)
{
	return make_unique<PipelineLib_DX12>(pDevice);
}

Ultimate::PipelineLib::sptr Ultimate::PipelineLib::MakeShared(const Device* pDevice, API api)
{
	return make_shared<PipelineLib_DX12>(pDevice);
}

Ultimate::SamplerFeedBack::uptr Ultimate::SamplerFeedBack::MakeUnique(API api)
{
	return make_unique<SamplerFeedBack_DX12>();
}

Ultimate::SamplerFeedBack::sptr Ultimate::SamplerFeedBack::MakeShared(API api)
{
	return make_shared<SamplerFeedBack_DX12>();
}

uint32_t Ultimate::SetBarrier(ResourceBarrier* pBufferBarriers, Buffer* pBuffer, ResourceState dstState,
	uint32_t numBarriers, BarrierFlag flags, ResourceState srcState, uint32_t threadIdx)
{
	XUSG::ResourceBarrier barrier;
	auto& bufferBarrier = pBufferBarriers[numBarriers];
	numBarriers = pBuffer->SetBarrier(&barrier, dstState, numBarriers,
		XUSG_BARRIER_ALL_SUBRESOURCES, flags, srcState, threadIdx);

	bufferBarrier.LayoutBefore = bufferBarrier.LayoutAfter = BarrierLayout::UNDEFINED;
	MapBarrierState(bufferBarrier.SyncBefore, bufferBarrier.AccessBefore, barrier.StateBefore);
	MapBarrierState(bufferBarrier.SyncAfter, bufferBarrier.AccessAfter, barrier.StateAfter);
	bufferBarrier.pResource = barrier.pResource;
	bufferBarrier.Offset = 0;
	bufferBarrier.Size = UINT64_MAX;

	switch (flags)
	{
	case BarrierFlag::BEGIN_ONLY:
		bufferBarrier.SyncAfter = BarrierSync::SPLIT;
		break;
	case BarrierFlag::END_ONLY:
		bufferBarrier.SyncBefore = BarrierSync::SPLIT;
		break;
	}

	return numBarriers;
}

uint32_t Ultimate::SetBarrier(ResourceBarrier* pTextureBarriers, Texture* pTexture, ResourceState dstState,
	uint32_t numBarriers, uint32_t indexOrFirstMip, uint8_t numMipLevels, uint16_t firstArraySlice,
	uint16_t numArraySlices, uint8_t firstPlane, uint8_t numPlanes, BarrierFlag flags,
	ResourceState srcState, TextureBarrierFlag textureFlags, uint32_t threadIdx)
{
	XUSG::ResourceBarrier barrier;
	if (numMipLevels)
	{
		const auto firstMipLevel = static_cast<uint8_t>(indexOrFirstMip);
		for (auto planeSlice = firstPlane; planeSlice < firstPlane + numPlanes; ++planeSlice)
			for (auto arraySlice = firstArraySlice; arraySlice < firstArraySlice + numArraySlices; ++arraySlice)
				for (auto mipSlice = firstMipLevel; mipSlice < firstMipLevel + numMipLevels; ++mipSlice)
					pTexture->SetBarrier(&barrier, mipSlice, dstState, numBarriers, arraySlice, flags, srcState, planeSlice, threadIdx);
	}
	else pTexture->SetBarrier(&barrier, dstState, numBarriers, indexOrFirstMip, flags, srcState, threadIdx);

	auto& textureBarrier = pTextureBarriers[numBarriers++];
	MapBarrierState(textureBarrier.SyncBefore, textureBarrier.AccessBefore, textureBarrier.LayoutBefore, barrier.StateBefore);
	MapBarrierState(textureBarrier.SyncAfter, textureBarrier.AccessAfter, textureBarrier.LayoutAfter, barrier.StateAfter);
	textureBarrier.pResource = barrier.pResource;
	textureBarrier.IndexOrFirstMipLevel = indexOrFirstMip;
	textureBarrier.NumMipLevels = numMipLevels;
	textureBarrier.FirstArraySlice = firstArraySlice;
	textureBarrier.NumArraySlices = numArraySlices;
	textureBarrier.FirstPlane = firstPlane;
	textureBarrier.NumPlanes = numPlanes;
	textureBarrier.Flags = textureFlags;

	switch (flags)
	{
	case BarrierFlag::BEGIN_ONLY:
		textureBarrier.SyncAfter = BarrierSync::SPLIT;
		break;
	case BarrierFlag::END_ONLY:
		textureBarrier.SyncBefore = BarrierSync::SPLIT;
		break;
	}

	return numBarriers;
}

void Ultimate::GetLinearAlgebraMatrixInfo(const Device* pDevice, LinearAlgebraMatrixInfo& matrixInfo, API api)
{
	return GetDX12LinearAlgebraMatrixInfo(pDevice, matrixInfo);
}

void Ultimate::SetLinearAlgebraMatrixConversionInfo(LinearAlgebraMatrixConversionInfo& conversion,
	uint64_t dst, uint64_t src, LinearAlgebraDataType srcDataType, LinearAlgebraMatrixLayout srcLayout,
	uint32_t srcSize, uint32_t srcStride)
{
	conversion.Dst = dst;
	conversion.Src = src;
	conversion.SrcDataType = srcDataType;
	conversion.SrcLayout = srcLayout;

	uint8_t elementSize = 0;
	if (srcSize == 0 || srcStride == 0)
	{
		switch (srcDataType)
		{
		case LinearAlgebraDataType::SINT16:
		case LinearAlgebraDataType::UINT16:
		case LinearAlgebraDataType::FLOAT16:
			elementSize = sizeof(uint16_t);
			break;
		case LinearAlgebraDataType::SINT32:
		case LinearAlgebraDataType::UINT32:
		case LinearAlgebraDataType::FLOAT32:
			elementSize = sizeof(uint32_t);
			break;
		default:
			elementSize = sizeof(uint8_t);
		}
	}

	conversion.SrcSize = srcSize ? srcSize : elementSize * conversion.DstInfo.NumRows * conversion.DstInfo.NumColumns;
	conversion.SrcStride = srcStride ? srcStride :
		(srcLayout == LinearAlgebraMatrixLayout::ROW_MAJOR ? elementSize * conversion.DstInfo.NumColumns :
			(srcLayout == LinearAlgebraMatrixLayout::COLUMN_MAJOR ? elementSize * conversion.DstInfo.NumRows : 0));
}

void Ultimate::MapBarrierState(BarrierSync& barrierSync, BarrierAccess& barrierAccess, ResourceState resourceState)
{
	switch (resourceState)
	{
	case ResourceState::COMMON:
		barrierSync = BarrierSync::ALL;
		barrierAccess = BarrierAccess::COMMON;
		return;
	case ResourceState::RENDER_TARGET:
		barrierSync = BarrierSync::RENDER_TARGET;
		barrierAccess = BarrierAccess::RENDER_TARGET;
		return;
	case ResourceState::UNORDERED_ACCESS:
		barrierSync = BarrierSync::ALL_SHADING;
		barrierAccess = BarrierAccess::UNORDERED_ACCESS;
		return;
	case ResourceState::STREAM_OUT:
		barrierSync = BarrierSync::VERTEX_SHADING;
		barrierAccess = BarrierAccess::STREAM_OUTPUT;
		return;
	case ResourceState::COPY_DEST:
		barrierSync = BarrierSync::COPY;
		barrierAccess = BarrierAccess::COPY_DEST;
		return;
	case ResourceState::RESOLVE_DEST:
		barrierSync = BarrierSync::RESOLVE;
		barrierAccess = BarrierAccess::RESOLVE_DEST;
		return;
	case ResourceState::RAYTRACING_ACCELERATION_STRUCTURE:
		barrierSync = BarrierSync::ALL_SHADING |
			BarrierSync::BUILD_RAYTRACING_ACCELERATION_STRUCTURE |
			BarrierSync::COPY_RAYTRACING_ACCELERATION_STRUCTURE |
			BarrierSync::EMIT_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO;
		barrierAccess = BarrierAccess::RAYTRACING_ACCELERATION_STRUCTURE;
		return;
	}

	barrierSync = BarrierSync::NONE;
	barrierAccess = BarrierAccess::COMMON;

	if ((resourceState & ResourceState::VERTEX_AND_CONSTANT_BUFFER) == ResourceState::VERTEX_AND_CONSTANT_BUFFER ||
		(resourceState & ResourceState::ALL_SHADER_RESOURCE) == ResourceState::ALL_SHADER_RESOURCE)
		barrierSync |= BarrierSync::ALL_SHADING;
	else if ((resourceState & ResourceState::NON_PIXEL_SHADER_RESOURCE) == ResourceState::NON_PIXEL_SHADER_RESOURCE)
		barrierSync |= BarrierSync::NON_PIXEL_SHADING;
	else if ((resourceState & ResourceState::PIXEL_SHADER_RESOURCE) == ResourceState::PIXEL_SHADER_RESOURCE ||
		(resourceState & ResourceState::SHADING_RATE_SOURCE) == ResourceState::SHADING_RATE_SOURCE)
		barrierSync |= BarrierSync::PIXEL_SHADING;

	if ((resourceState & ResourceState::VERTEX_AND_CONSTANT_BUFFER) == ResourceState::VERTEX_AND_CONSTANT_BUFFER)
		barrierAccess |= BarrierAccess::VERTEX_BUFFER | BarrierAccess::CONSTANT_BUFFER;

	if ((resourceState & ResourceState::INDEX_BUFFER) == ResourceState::INDEX_BUFFER)
	{
		barrierSync |= BarrierSync::INDEX_INPUT;
		barrierAccess |= BarrierAccess::INDEX_BUFFER;
	}

	if ((resourceState & ResourceState::DEPTH_WRITE) == ResourceState::DEPTH_WRITE)
	{
		barrierSync |= BarrierSync::DEPTH_STENCIL;
		barrierAccess |= BarrierAccess::DEPTH_STENCIL_WRITE;
	}

	if ((resourceState & ResourceState::DEPTH_READ) == ResourceState::DEPTH_READ)
	{
		barrierSync |= BarrierSync::DEPTH_STENCIL;
		barrierAccess |= BarrierAccess::DEPTH_STENCIL_READ;
	}

	if ((resourceState & ResourceState::ALL_SHADER_RESOURCE) != ResourceState::COMMON)
		barrierAccess |= BarrierAccess::SHADER_RESOURCE;

	if ((resourceState & ResourceState::INDIRECT_ARGUMENT) == ResourceState::INDIRECT_ARGUMENT)
	{
		barrierSync |= BarrierSync::EXECUTE_INDIRECT;
		barrierAccess |= BarrierAccess::INDIRECT_ARGUMENT;
	}

	if ((resourceState & ResourceState::COPY_SOURCE) == ResourceState::COPY_SOURCE)
	{
		barrierSync |= BarrierSync::COPY;
		barrierAccess |= BarrierAccess::COPY_SOURCE;
	}

	if ((resourceState & ResourceState::RESOLVE_SOURCE) == ResourceState::RESOLVE_SOURCE)
	{
		barrierSync |= BarrierSync::RESOLVE;
		barrierAccess |= BarrierAccess::RESOLVE_SOURCE;
	}

	if ((resourceState & ResourceState::SHADING_RATE_SOURCE) == ResourceState::SHADING_RATE_SOURCE)
		barrierAccess |= BarrierAccess::SHADING_RATE_SOURCE;
}

void Ultimate::MapBarrierState(BarrierSync& barrierSync, BarrierAccess& barrierAccess,
	BarrierLayout& barrierLayout, ResourceState resourceState)
{
	switch (resourceState)
	{
	case ResourceState::COMMON:
		barrierSync = BarrierSync::ALL;
		barrierAccess = BarrierAccess::COMMON;
		barrierLayout = BarrierLayout::COMMON;
		return;
	case ResourceState::RENDER_TARGET:
		barrierSync = BarrierSync::RENDER_TARGET;
		barrierAccess = BarrierAccess::RENDER_TARGET;
		barrierLayout = BarrierLayout::RENDER_TARGET;
		return;
	case ResourceState::UNORDERED_ACCESS:
		barrierSync = BarrierSync::ALL_SHADING;
		barrierAccess = BarrierAccess::UNORDERED_ACCESS;
		barrierLayout = BarrierLayout::UNORDERED_ACCESS;
		return;
	case ResourceState::DEPTH_WRITE:
		barrierSync = BarrierSync::DEPTH_STENCIL;
		barrierAccess = BarrierAccess::DEPTH_STENCIL_WRITE;
		barrierLayout = BarrierLayout::DEPTH_STENCIL_WRITE;
		return;
	case ResourceState::DEPTH_READ:
		barrierSync = BarrierSync::DEPTH_STENCIL;
		barrierAccess = BarrierAccess::DEPTH_STENCIL_READ;
		barrierLayout = BarrierLayout::DEPTH_STENCIL_READ;
		return;
	case ResourceState::NON_PIXEL_SHADER_RESOURCE:
		barrierSync = BarrierSync::NON_PIXEL_SHADING;
		barrierAccess = BarrierAccess::SHADER_RESOURCE;
		barrierLayout = BarrierLayout::SHADER_RESOURCE;
		return;
	case ResourceState::PIXEL_SHADER_RESOURCE:
		barrierSync = BarrierSync::PIXEL_SHADING;
		barrierAccess = BarrierAccess::SHADER_RESOURCE;
		barrierLayout = BarrierLayout::SHADER_RESOURCE;
		return;
	case ResourceState::ALL_SHADER_RESOURCE:
		barrierSync = BarrierSync::ALL_SHADING;
		barrierAccess = BarrierAccess::SHADER_RESOURCE;
		barrierLayout = BarrierLayout::SHADER_RESOURCE;
		return;
	case ResourceState::COPY_DEST:
		barrierSync = BarrierSync::COPY;
		barrierAccess = BarrierAccess::COPY_DEST;
		barrierLayout = BarrierLayout::COPY_DEST;
		return;
	case ResourceState::COPY_SOURCE:
		barrierSync = BarrierSync::COPY;
		barrierAccess = BarrierAccess::COPY_SOURCE;
		barrierLayout = BarrierLayout::COPY_SOURCE;
		return;
	case ResourceState::RESOLVE_DEST:
		barrierSync = BarrierSync::RESOLVE;
		barrierAccess = BarrierAccess::RESOLVE_DEST;
		barrierLayout = BarrierLayout::RESOLVE_DEST;
		return;
	case ResourceState::RESOLVE_SOURCE:
		barrierSync = BarrierSync::RESOLVE;
		barrierAccess = BarrierAccess::RESOLVE_SOURCE;
		barrierLayout = BarrierLayout::RESOLVE_SOURCE;
		return;
	case ResourceState::RAYTRACING_ACCELERATION_STRUCTURE:
		barrierSync = BarrierSync::ALL_SHADING |
			BarrierSync::BUILD_RAYTRACING_ACCELERATION_STRUCTURE |
			BarrierSync::COPY_RAYTRACING_ACCELERATION_STRUCTURE |
			BarrierSync::EMIT_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO;
		barrierAccess = BarrierAccess::RAYTRACING_ACCELERATION_STRUCTURE;
		barrierLayout = BarrierLayout::UNDEFINED;
		return;
	case ResourceState::SHADING_RATE_SOURCE:
		barrierSync = BarrierSync::ALL_SHADING;
		barrierAccess = BarrierAccess::SHADING_RATE_SOURCE;
		barrierLayout = BarrierLayout::SHADING_RATE_SOURCE;
		return;
	}

	barrierSync = BarrierSync::NONE;
	barrierAccess = BarrierAccess::COMMON;
	barrierLayout = BarrierLayout::UNDEFINED;

	if ((resourceState & ResourceState::VERTEX_AND_CONSTANT_BUFFER) == ResourceState::VERTEX_AND_CONSTANT_BUFFER ||
		(resourceState & ResourceState::ALL_SHADER_RESOURCE) == ResourceState::ALL_SHADER_RESOURCE)
		barrierSync |= BarrierSync::ALL_SHADING;
	else if ((resourceState & ResourceState::NON_PIXEL_SHADER_RESOURCE) == ResourceState::NON_PIXEL_SHADER_RESOURCE)
		barrierSync |= BarrierSync::NON_PIXEL_SHADING;
	else if ((resourceState & ResourceState::PIXEL_SHADER_RESOURCE) == ResourceState::PIXEL_SHADER_RESOURCE ||
		(resourceState & ResourceState::SHADING_RATE_SOURCE) == ResourceState::SHADING_RATE_SOURCE)
		barrierSync |= BarrierSync::PIXEL_SHADING;

	if ((resourceState & ResourceState::VERTEX_AND_CONSTANT_BUFFER) == ResourceState::VERTEX_AND_CONSTANT_BUFFER)
		barrierAccess |= BarrierAccess::VERTEX_BUFFER | BarrierAccess::CONSTANT_BUFFER;

	if ((resourceState & ResourceState::INDEX_BUFFER) == ResourceState::INDEX_BUFFER)
	{
		barrierSync |= BarrierSync::INDEX_INPUT;
		barrierAccess |= BarrierAccess::INDEX_BUFFER;
	}

	if ((resourceState & ResourceState::DEPTH_WRITE) == ResourceState::DEPTH_WRITE)
	{
		barrierSync |= BarrierSync::DEPTH_STENCIL;
		barrierAccess |= BarrierAccess::DEPTH_STENCIL_WRITE;
		barrierLayout = BarrierLayout::DEPTH_STENCIL_WRITE;
	}

	if ((resourceState & ResourceState::DEPTH_READ) == ResourceState::DEPTH_READ)
	{
		barrierSync |= BarrierSync::DEPTH_STENCIL;
		barrierAccess |= BarrierAccess::DEPTH_STENCIL_READ;
	}

	if ((resourceState & ResourceState::ALL_SHADER_RESOURCE) != ResourceState::COMMON)
		barrierAccess |= BarrierAccess::SHADER_RESOURCE;

	if ((resourceState & ResourceState::STREAM_OUT) == ResourceState::STREAM_OUT)
	{
		barrierSync |= BarrierSync::VERTEX_SHADING;
		barrierAccess |= BarrierAccess::STREAM_OUTPUT;
	}

	if ((resourceState & ResourceState::INDIRECT_ARGUMENT) == ResourceState::INDIRECT_ARGUMENT)
	{
		barrierSync |= BarrierSync::EXECUTE_INDIRECT;
		barrierAccess |= BarrierAccess::INDIRECT_ARGUMENT;
	}

	if ((resourceState & ResourceState::COPY_SOURCE) == ResourceState::COPY_SOURCE)
	{
		barrierSync |= BarrierSync::COPY;
		barrierAccess |= BarrierAccess::COPY_SOURCE;
	}

	if ((resourceState & ResourceState::RESOLVE_SOURCE) == ResourceState::RESOLVE_SOURCE)
	{
		barrierSync |= BarrierSync::RESOLVE;
		barrierAccess |= BarrierAccess::RESOLVE_SOURCE;
	}

	if ((resourceState & ResourceState::SHADING_RATE_SOURCE) == ResourceState::SHADING_RATE_SOURCE)
		barrierAccess |= BarrierAccess::SHADING_RATE_SOURCE;

	if ((resourceState & (ResourceState::DEPTH_READ | ResourceState::RESOLVE_SOURCE)) != ResourceState::COMMON)
		barrierLayout = BarrierLayout::DIRECT_QUEUE_GENERIC_READ;
	else if ((resourceState & (ResourceState::ALL_SHADER_RESOURCE | ResourceState::COPY_SOURCE)) != ResourceState::COMMON)
		barrierLayout = BarrierLayout::GENERIC_READ_RESOURCE;

	auto bufferState = ResourceState::VERTEX_AND_CONSTANT_BUFFER;
	bufferState |= ResourceState::INDEX_BUFFER;
	bufferState |= ResourceState::INDIRECT_ARGUMENT;
	if ((resourceState & bufferState) != ResourceState::COMMON)
		barrierLayout = BarrierLayout::UNDEFINED;
}

Ultimate::BarrierSync Ultimate::GetBarrierSync(ResourceState resourceState)
{
	switch (resourceState)
	{
	case ResourceState::COMMON:
		return BarrierSync::ALL;
	case ResourceState::RENDER_TARGET:
		return BarrierSync::RENDER_TARGET;
	case ResourceState::UNORDERED_ACCESS:
		return BarrierSync::ALL_SHADING;
	case ResourceState::STREAM_OUT:
		return BarrierSync::VERTEX_SHADING;
	case ResourceState::COPY_DEST:
		return BarrierSync::COPY;
	case ResourceState::RESOLVE_DEST:
		return BarrierSync::RESOLVE;
	case ResourceState::RAYTRACING_ACCELERATION_STRUCTURE:
		return BarrierSync::ALL_SHADING |
			BarrierSync::BUILD_RAYTRACING_ACCELERATION_STRUCTURE |
			BarrierSync::COPY_RAYTRACING_ACCELERATION_STRUCTURE |
			BarrierSync::EMIT_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO;
	}

	auto barrierSync = BarrierSync::NONE;

	if ((resourceState & ResourceState::VERTEX_AND_CONSTANT_BUFFER) == ResourceState::VERTEX_AND_CONSTANT_BUFFER ||
		(resourceState & ResourceState::ALL_SHADER_RESOURCE) == ResourceState::ALL_SHADER_RESOURCE)
		barrierSync |= BarrierSync::ALL_SHADING;
	else if ((resourceState & ResourceState::NON_PIXEL_SHADER_RESOURCE) == ResourceState::NON_PIXEL_SHADER_RESOURCE)
		barrierSync |= BarrierSync::NON_PIXEL_SHADING;
	else if ((resourceState & ResourceState::PIXEL_SHADER_RESOURCE) == ResourceState::PIXEL_SHADER_RESOURCE ||
		(resourceState & ResourceState::SHADING_RATE_SOURCE) == ResourceState::SHADING_RATE_SOURCE)
		barrierSync |= BarrierSync::PIXEL_SHADING;

	if ((resourceState & ResourceState::INDEX_BUFFER) == ResourceState::INDEX_BUFFER)
		barrierSync |= BarrierSync::INDEX_INPUT;

	if ((resourceState & ResourceState::DEPTH_WRITE) == ResourceState::DEPTH_WRITE ||
		(resourceState & ResourceState::DEPTH_READ) == ResourceState::DEPTH_READ)
		barrierSync |= BarrierSync::DEPTH_STENCIL;

	if ((resourceState & ResourceState::INDIRECT_ARGUMENT) == ResourceState::INDIRECT_ARGUMENT)
		barrierSync |= BarrierSync::EXECUTE_INDIRECT;

	if ((resourceState & ResourceState::COPY_SOURCE) == ResourceState::COPY_SOURCE)
		barrierSync |= BarrierSync::COPY;


	if ((resourceState & ResourceState::RESOLVE_SOURCE) == ResourceState::RESOLVE_SOURCE)
		barrierSync |= BarrierSync::RESOLVE;

	return barrierSync;
}

Ultimate::BarrierAccess Ultimate::GetBarrierAccess(ResourceState resourceState)
{
	switch (resourceState)
	{
	case ResourceState::COMMON:
		return BarrierAccess::COMMON;
	case ResourceState::RENDER_TARGET:
		return BarrierAccess::RENDER_TARGET;
	case ResourceState::UNORDERED_ACCESS:
		return BarrierAccess::UNORDERED_ACCESS;
	case ResourceState::STREAM_OUT:
		return BarrierAccess::STREAM_OUTPUT;
	case ResourceState::COPY_DEST:
		return BarrierAccess::COPY_DEST;
	case ResourceState::RESOLVE_DEST:
		return BarrierAccess::RESOLVE_DEST;
	case ResourceState::RAYTRACING_ACCELERATION_STRUCTURE:
		return BarrierAccess::RAYTRACING_ACCELERATION_STRUCTURE;
	}

	auto barrierAccess = BarrierAccess::COMMON;

	if ((resourceState & ResourceState::VERTEX_AND_CONSTANT_BUFFER) == ResourceState::VERTEX_AND_CONSTANT_BUFFER)
		barrierAccess |= BarrierAccess::VERTEX_BUFFER | BarrierAccess::CONSTANT_BUFFER;

	if ((resourceState & ResourceState::INDEX_BUFFER) == ResourceState::INDEX_BUFFER)
		barrierAccess |= BarrierAccess::INDEX_BUFFER;

	if ((resourceState & ResourceState::DEPTH_WRITE) == ResourceState::DEPTH_WRITE)
		barrierAccess |= BarrierAccess::DEPTH_STENCIL_WRITE;

	if ((resourceState & ResourceState::DEPTH_READ) == ResourceState::DEPTH_READ)
		barrierAccess |= BarrierAccess::DEPTH_STENCIL_READ;

	if ((resourceState & ResourceState::ALL_SHADER_RESOURCE) != ResourceState::COMMON)
		barrierAccess |= BarrierAccess::SHADER_RESOURCE;

	if ((resourceState & ResourceState::INDIRECT_ARGUMENT) == ResourceState::INDIRECT_ARGUMENT)
		barrierAccess |= BarrierAccess::INDIRECT_ARGUMENT;

	if ((resourceState & ResourceState::COPY_SOURCE) == ResourceState::COPY_SOURCE)
		barrierAccess |= BarrierAccess::COPY_SOURCE;

	if ((resourceState & ResourceState::RESOLVE_SOURCE) == ResourceState::RESOLVE_SOURCE)
		barrierAccess |= BarrierAccess::RESOLVE_SOURCE;

	if ((resourceState & ResourceState::SHADING_RATE_SOURCE) == ResourceState::SHADING_RATE_SOURCE)
		barrierAccess |= BarrierAccess::SHADING_RATE_SOURCE;

	return barrierAccess;
}

WorkGraph::State::uptr WorkGraph::State::MakeUnique(API api)
{
	return make_unique<State_DX12>();
}

WorkGraph::State::sptr WorkGraph::State::MakeShared(API api)
{
	return make_shared<State_DX12>();
}

WorkGraph::PipelineLib::uptr WorkGraph::PipelineLib::MakeUnique(API api)
{
	return make_unique<PipelineLib_DX12>();
}

WorkGraph::PipelineLib::sptr WorkGraph::PipelineLib::MakeShared(API api)
{
	return make_shared<PipelineLib_DX12>();
}

WorkGraph::PipelineLib::uptr WorkGraph::PipelineLib::MakeUnique(const Device* pDevice, API api)
{
	return make_unique<PipelineLib_DX12>(pDevice);
}

WorkGraph::PipelineLib::sptr WorkGraph::PipelineLib::MakeShared(const Device* pDevice, API api)
{
	return make_shared<PipelineLib_DX12>(pDevice);
}

Generic::State::uptr Generic::State::MakeUnique(API api)
{
	return make_unique<State_DX12>();
}

Generic::State::sptr Generic::State::MakeShared(API api)
{
	return make_shared<State_DX12>();
}

Generic::PipelineLib::uptr Generic::PipelineLib::MakeUnique(API api)
{
	return make_unique<PipelineLib_DX12>();
}

Generic::PipelineLib::sptr Generic::PipelineLib::MakeShared(API api)
{
	return make_shared<PipelineLib_DX12>();
}

Generic::PipelineLib::uptr Generic::PipelineLib::MakeUnique(const Device* pDevice, API api)
{
	return make_unique<PipelineLib_DX12>(pDevice);
}

Generic::PipelineLib::sptr Generic::PipelineLib::MakeShared(const Device* pDevice, API api)
{
	return make_shared<PipelineLib_DX12>(pDevice);
}
