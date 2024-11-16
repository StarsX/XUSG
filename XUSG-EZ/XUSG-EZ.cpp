//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSGCommand_DX12.h"
#include "XUSG-EZ_DX12.h"

using namespace std;
using namespace XUSG;
using namespace XUSG::EZ;

void EZ::CalcSubresources(vector<uint32_t>& subresources, const Texture* pResource, uint8_t mipSlice, uint8_t planeSlice)
{
	const auto arraySize = pResource->GetArraySize();
	subresources.resize(arraySize);

	for (uint16_t i = 0; i < arraySize; ++i)
		subresources[i] = pResource->CalculateSubresource(mipSlice, i, planeSlice);
}

ResourceView EZ::GetCBV(ConstantBuffer* pResource, uint32_t index)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.View = pResource->GetCBV(index);
	resourceView.DstState = ResourceState::GENERIC_READ_RESOURCE;
	resourceView.pCounter = nullptr;

	return resourceView;
}

ResourceView EZ::GetSRV(Buffer* pResource, uint32_t index, ResourceState dstState)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.View = pResource->GetSRV(index);
	resourceView.Subresources = { XUSG_BARRIER_ALL_SUBRESOURCES };
	resourceView.DstState = pResource->GetResourceState() == ResourceState::GENERIC_READ_RESOURCE ?
		ResourceState::GENERIC_READ_RESOURCE : dstState;
	resourceView.pCounter = nullptr;

	return resourceView;
}

ResourceView EZ::GetSRV(VertexBuffer* pResource, uint32_t index, ResourceState dstState)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.View = pResource->GetSRV(index);
	resourceView.Subresources = { XUSG_BARRIER_ALL_SUBRESOURCES };
	resourceView.DstState = pResource->GetResourceState() == ResourceState::GENERIC_READ_RESOURCE ?
		ResourceState::GENERIC_READ_RESOURCE : dstState;
	resourceView.pCounter = nullptr;

	return resourceView;
}

ResourceView EZ::GetSRV(IndexBuffer* pResource, uint32_t index, ResourceState dstState)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.View = pResource->GetSRV(index);
	resourceView.Subresources = { XUSG_BARRIER_ALL_SUBRESOURCES };
	resourceView.DstState = pResource->GetResourceState() == ResourceState::GENERIC_READ_RESOURCE ?
		ResourceState::GENERIC_READ_RESOURCE : dstState;
	resourceView.pCounter = nullptr;

	return resourceView;
}

ResourceView EZ::GetSRV(Texture* pResource, uint8_t firstLevel, bool singleLevel, ResourceState dstState)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.View = pResource->GetSRV(firstLevel, singleLevel);
	resourceView.DstState = dstState;
	resourceView.pCounter = nullptr;

	if (singleLevel) CalcSubresources(resourceView.Subresources, pResource, firstLevel);
	else
	{
		const auto numMips = pResource->GetNumMips();
		const auto arraySize = pResource->GetArraySize();
		resourceView.Subresources.resize((numMips - firstLevel) * arraySize);

		auto i = 0u;
		for (uint16_t arraySlice = 0; arraySlice < arraySize; ++arraySlice)
			for (auto mipSlice = firstLevel; mipSlice < numMips; ++mipSlice)
				resourceView.Subresources[i++] = pResource->CalculateSubresource(mipSlice, arraySlice);
	}

	return resourceView;
}

ResourceView EZ::GetSRV(Texture3D* pResource, uint8_t firstLevel, bool singleLevel, ResourceState dstState)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.View = pResource->GetSRV(firstLevel, singleLevel);
	resourceView.DstState = dstState;
	resourceView.pCounter = nullptr;

	if (singleLevel) resourceView.Subresources = { pResource->CalculateSubresource(firstLevel) };
	else
	{
		const auto numMips = pResource->GetNumMips();
		resourceView.Subresources.resize(numMips - firstLevel);

		auto i = 0u;
		for (auto mipSlice = firstLevel; mipSlice < numMips; ++mipSlice)
			resourceView.Subresources[i++] = pResource->CalculateSubresource(mipSlice);
	}

	return resourceView;
}

ResourceView EZ::GetUAV(Buffer* pResource, uint32_t index)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.View = pResource->GetUAV(index);
	resourceView.Subresources = { XUSG_BARRIER_ALL_SUBRESOURCES };
	resourceView.DstState = ResourceState::UNORDERED_ACCESS;
	resourceView.pCounter = pResource->GetCounter().get();

	return resourceView;
}

ResourceView EZ::GetUAV(Texture* pResource, uint8_t level, Format format)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.View = pResource->GetUAV(level, format);
	resourceView.DstState = ResourceState::UNORDERED_ACCESS;
	resourceView.pCounter = nullptr;
	CalcSubresources(resourceView.Subresources, pResource, level);

	return resourceView;
}

ResourceView EZ::GetUAV(Texture3D* pResource, uint8_t level, Format format)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.View = pResource->GetUAV(level, format);
	resourceView.Subresources = { pResource->CalculateSubresource(level) };
	resourceView.DstState = ResourceState::UNORDERED_ACCESS;
	resourceView.pCounter = nullptr;

	return resourceView;
}

ResourceView EZ::GetUAV(TypedBuffer* pResource, uint32_t index, Format format)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.View = pResource->GetUAV(index, format);
	resourceView.Subresources = { XUSG_BARRIER_ALL_SUBRESOURCES };
	resourceView.DstState = ResourceState::UNORDERED_ACCESS;
	resourceView.pCounter = pResource->GetCounter().get();

	return resourceView;
}

ResourceView EZ::GetRTV(RenderTarget* pResource, uint16_t slice, uint8_t mipLevel)
{
	const bool isArray = slice == UINT16_MAX;

	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.View = pResource->GetRTV(isArray ? 0 : slice, mipLevel);
	resourceView.DstState = ResourceState::RENDER_TARGET;
	resourceView.pCounter = nullptr;

	if (isArray) CalcSubresources(resourceView.Subresources, pResource, mipLevel);
	else resourceView.Subresources = { pResource->CalculateSubresource(mipLevel, slice) };

	return resourceView;
}

ResourceView EZ::GetDSV(DepthStencil* pResource, uint16_t slice, uint8_t mipLevel)
{
	const bool isArray = slice == UINT16_MAX;

	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.View = pResource->GetDSV(isArray ? 0 : slice, mipLevel);
	resourceView.DstState = ResourceState::DEPTH_WRITE;
	resourceView.pCounter = nullptr;

	if (isArray) CalcSubresources(resourceView.Subresources, pResource, mipLevel);
	else resourceView.Subresources = { pResource->CalculateSubresource(mipLevel, slice) };

	return resourceView;
}

ResourceView EZ::GetReadOnlyDSV(DepthStencil* pResource, uint16_t slice, uint8_t mipLevel, ResourceState dstSrvState)
{
	const bool isArray = slice == UINT16_MAX;

	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.View = pResource->GetDSV(isArray ? 0 : slice, mipLevel, true);
	resourceView.DstState = ResourceState::DEPTH_READ;
	resourceView.DstState |= pResource->GetSRV() ? dstSrvState : resourceView.DstState;
	resourceView.pCounter = nullptr;

	if (isArray) CalcSubresources(resourceView.Subresources, pResource, mipLevel);
	else resourceView.Subresources = { pResource->CalculateSubresource(mipLevel, slice) };

	return resourceView;
}

ResourceView EZ::GetStencilSRV(DepthStencil* pResource, ResourceState dstState)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.View = pResource->GetSRV(0, true, true);
	resourceView.Subresources = { XUSG_BARRIER_ALL_SUBRESOURCES };
	resourceView.DstState = dstState;
	resourceView.pCounter = nullptr;

	return resourceView;
}

EZ::VertexBufferView EZ::GetVBV(VertexBuffer* pResource, uint32_t index, ResourceState dstSrvState)
{
	VertexBufferView resourceView;
	resourceView.pResource = pResource;
	resourceView.pView = &pResource->GetVBV(index);
	resourceView.DstState = ResourceState::VERTEX_AND_CONSTANT_BUFFER;
	resourceView.DstState |= pResource->GetSRV() ? dstSrvState : resourceView.DstState;

	return resourceView;
}

EZ::IndexBufferView EZ::GetIBV(IndexBuffer* pResource, uint32_t index, ResourceState dstSrvState)
{
	IndexBufferView resourceView;
	resourceView.pResource = pResource;
	resourceView.pView = &pResource->GetIBV(index);
	resourceView.DstState = ResourceState::INDEX_BUFFER;
	resourceView.DstState |= pResource->GetSRV() ? dstSrvState : resourceView.DstState;

	return resourceView;
}

EZ::CommandList::uptr EZ::CommandList::MakeUnique(API api)
{
	return make_unique<EZ::CommandList_DX12>();
}

EZ::CommandList::sptr EZ::CommandList::MakeShared(API api)
{
	return make_shared<EZ::CommandList_DX12>();
}

EZ::CommandList::uptr EZ::CommandList::MakeUnique(XUSG::CommandList* pCommandList,
	uint32_t samplerHeapSize, uint32_t cbvSrvUavHeapSize,
	const uint32_t maxSamplers[Shader::Stage::NUM_STAGE],
	const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_STAGE],
	const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_STAGE],
	const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_STAGE],
	const uint32_t maxCbvSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t maxSrvSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t maxUavSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t max32BitConstants[Shader::Stage::NUM_STAGE],
	const uint32_t constantSlots[Shader::Stage::NUM_STAGE],
	const uint32_t constantSpaces[Shader::Stage::NUM_STAGE],
	uint32_t slotExt, uint32_t spaceExt, API api)
{
	return make_unique<EZ::CommandList_DX12>(pCommandList, samplerHeapSize, cbvSrvUavHeapSize,
		maxSamplers, pMaxCbvsEachSpace, pMaxSrvsEachSpace, pMaxUavsEachSpace,
		maxCbvSpaces, maxSrvSpaces, maxUavSpaces, max32BitConstants,
		constantSlots, constantSpaces, slotExt, spaceExt);
}

EZ::CommandList::sptr EZ::CommandList::MakeShared(XUSG::CommandList* pCommandList,
	uint32_t samplerHeapSize, uint32_t cbvSrvUavHeapSize,
	const uint32_t maxSamplers[Shader::Stage::NUM_STAGE],
	const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_STAGE],
	const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_STAGE],
	const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_STAGE],
	const uint32_t maxCbvSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t maxSrvSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t maxUavSpaces[Shader::Stage::NUM_STAGE],
	const uint32_t max32BitConstants[Shader::Stage::NUM_STAGE],
	const uint32_t constantSlots[Shader::Stage::NUM_STAGE],
	const uint32_t constantSpaces[Shader::Stage::NUM_STAGE],
	uint32_t slotExt, uint32_t spaceExt, API api)
{
	return make_shared<EZ::CommandList_DX12>(pCommandList, samplerHeapSize, cbvSrvUavHeapSize,
		maxSamplers, pMaxCbvsEachSpace, pMaxSrvsEachSpace, pMaxUavsEachSpace,
		maxCbvSpaces, maxSrvSpaces, maxUavSpaces, max32BitConstants,
		constantSlots, constantSpaces, slotExt, spaceExt);
}
