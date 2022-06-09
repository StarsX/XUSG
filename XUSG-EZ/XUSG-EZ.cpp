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

	for (auto i = 0u; i < arraySize; ++i)
		subresources[i] = CalcSubresource(pResource, mipSlice, i, planeSlice);
}

uint32_t EZ::CalcSubresource(const Texture* pResource, uint8_t mipSlice, uint32_t arraySlice, uint8_t planeSlice)
{
	const auto numMips = pResource->GetNumMips();

	return mipSlice + arraySlice * numMips + planeSlice * numMips * pResource->GetArraySize();
}

uint32_t EZ::CalcSubresource(const Texture3D* pResource, uint8_t mipSlice, uint8_t planeSlice)
{
	const auto numMips = pResource->GetNumMips();

	return mipSlice + planeSlice * numMips;
}

ResourceView EZ::GetCBV(ConstantBuffer* pResource, uint32_t index)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.View = pResource->GetCBV(index);
	resourceView.DstState = ResourceState::VERTEX_AND_CONSTANT_BUFFER;

	return resourceView;
}

ResourceView EZ::GetSRV(Buffer* pResource, uint32_t index, ResourceState dstState)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.View = pResource->GetSRV(index);
	resourceView.Subresources = { XUSG_BARRIER_ALL_SUBRESOURCES };
	resourceView.DstState = dstState;

	return resourceView;
}

ResourceView EZ::GetSRV(VertexBuffer* pResource, uint32_t index, ResourceState dstState)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.View = pResource->GetSRV(index);
	resourceView.Subresources = { XUSG_BARRIER_ALL_SUBRESOURCES };
	resourceView.DstState = dstState;

	return resourceView;
}

ResourceView EZ::GetSRV(IndexBuffer* pResource, uint32_t index, ResourceState dstState)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.View = pResource->GetSRV(index);
	resourceView.Subresources = { XUSG_BARRIER_ALL_SUBRESOURCES };
	resourceView.DstState = dstState;

	return resourceView;
}

ResourceView EZ::GetSRV(Texture* pResource, uint32_t index, ResourceState dstState)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.View = pResource->GetSRV(index);
	resourceView.DstState = dstState;

	const auto numMips = pResource->GetNumMips();
	const auto arraySize = pResource->GetArraySize();
	resourceView.Subresources.resize(arraySize * (numMips - index));

	for (auto i = index; i < numMips; ++i)
		for (auto j = 0u; j < arraySize; ++j)
			resourceView.Subresources[i] = CalcSubresource(pResource, i, j);

	return resourceView;
}

ResourceView EZ::GetSRV(Texture3D* pResource, ResourceState dstState)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.View = pResource->GetSRV();
	resourceView.DstState = dstState;

	const auto numMips = pResource->GetNumMips();
	resourceView.Subresources.resize(numMips);

	for (uint8_t i = 0; i < numMips; ++i)
			resourceView.Subresources[i] = CalcSubresource(pResource, i);

	return resourceView;
}

ResourceView EZ::GetSRVLevel(Texture* pResource, uint8_t level, ResourceState dstState)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.View = pResource->GetSRVLevel(level);
	resourceView.DstState = dstState;
	CalcSubresources(resourceView.Subresources, pResource, level);

	return resourceView;
}

ResourceView EZ::GetSRVLevel(Texture3D* pResource, uint8_t level, ResourceState dstState)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.View = pResource->GetSRVLevel(level);
	resourceView.DstState = dstState;
	resourceView.Subresources = { CalcSubresource(pResource, level) };

	return resourceView;
}

ResourceView EZ::GetUAV(Buffer* pResource, uint8_t index)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.View = pResource->GetUAV(index);
	resourceView.Subresources = { XUSG_BARRIER_ALL_SUBRESOURCES };
	resourceView.DstState = ResourceState::UNORDERED_ACCESS;

	return resourceView;
}

ResourceView EZ::GetUAV(Texture* pResource, uint8_t index)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.View = pResource->GetUAV(index);
	resourceView.DstState = ResourceState::UNORDERED_ACCESS;
	CalcSubresources(resourceView.Subresources, pResource, index);

	return resourceView;
}

ResourceView EZ::GetUAV(Texture3D* pResource, uint8_t index)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.View = pResource->GetUAV(index);
	resourceView.Subresources = { CalcSubresource(pResource, index) };
	resourceView.DstState = ResourceState::UNORDERED_ACCESS;

	return resourceView;
}

ResourceView EZ::GetPackedUAV(Texture* pResource, uint8_t index)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.View = pResource->GetPackedUAV(index);
	resourceView.DstState = ResourceState::UNORDERED_ACCESS;
	CalcSubresources(resourceView.Subresources, pResource, index);

	return resourceView;
}

ResourceView EZ::GetPackedUAV(Texture3D* pResource, uint8_t index)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.View = pResource->GetPackedUAV(index);
	resourceView.Subresources = { CalcSubresource(pResource, index) };
	resourceView.DstState = ResourceState::UNORDERED_ACCESS;

	return resourceView;
}

ResourceView EZ::GetPackedUAV(TypedBuffer* pResource, uint8_t index)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.View = pResource->GetPackedUAV(index);
	resourceView.Subresources = { XUSG_BARRIER_ALL_SUBRESOURCES };
	resourceView.DstState = ResourceState::UNORDERED_ACCESS;

	return resourceView;
}

ResourceView EZ::GetRTV(RenderTarget* pResource, uint32_t slice, uint8_t mipLevel)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.View = pResource->GetRTV(slice, mipLevel);
	resourceView.Subresources = { CalcSubresource(pResource, mipLevel, slice) };
	resourceView.DstState = ResourceState::RENDER_TARGET;

	return resourceView;
}

ResourceView EZ::GetArrayRTV(RenderTarget* pResource, uint8_t mipLevel)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.View = pResource->GetRTV(0, mipLevel);
	resourceView.DstState = ResourceState::RENDER_TARGET;
	CalcSubresources(resourceView.Subresources, pResource, mipLevel);

	return resourceView;
}

ResourceView EZ::GetDSV(DepthStencil* pResource, uint32_t slice, uint8_t mipLevel)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.View = pResource->GetDSV(slice, mipLevel);
	resourceView.Subresources = { CalcSubresource(pResource, mipLevel, slice) };
	resourceView.DstState = ResourceState::DEPTH_WRITE;

	return resourceView;
}

ResourceView EZ::GetArrayDSV(DepthStencil* pResource, uint8_t mipLevel)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.View = pResource->GetDSV(0, mipLevel);
	resourceView.DstState = ResourceState::DEPTH_WRITE;
	CalcSubresources(resourceView.Subresources, pResource, mipLevel);

	return resourceView;
}

ResourceView EZ::GetReadOnlyDSV(DepthStencil* pResource, uint32_t slice, uint8_t mipLevel, ResourceState dstSrvState)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.View = pResource->GetReadOnlyDSV(slice, mipLevel);
	resourceView.Subresources = { CalcSubresource(pResource, mipLevel, slice) };
	resourceView.DstState = ResourceState::DEPTH_READ;
	resourceView.DstState |= pResource->GetSRV() ? dstSrvState : resourceView.DstState;

	return resourceView;
}

ResourceView EZ::GetReadOnlyArrayDSV(DepthStencil* pResource, uint8_t mipLevel, ResourceState dstSrvState)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.View = pResource->GetReadOnlyDSV(0, mipLevel);
	resourceView.DstState = ResourceState::DEPTH_READ;
	resourceView.DstState |= pResource->GetSRV() ? dstSrvState : resourceView.DstState;
	CalcSubresources(resourceView.Subresources, pResource, mipLevel);

	return resourceView;
}

ResourceView EZ::GetStencilSRV(DepthStencil* pResource, ResourceState dstSrvState)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.View = pResource->GetStencilSRV();
	resourceView.Subresources = { XUSG_BARRIER_ALL_SUBRESOURCES };
	resourceView.DstState = dstSrvState;

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
	uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize, uint32_t maxSamplers,
	const uint32_t* pMaxCbvsEachSpace, const uint32_t* pMaxSrvsEachSpace,
	const uint32_t* pMaxUavsEachSpace, uint32_t maxCbvSpaces,
	uint32_t maxSrvSpaces, uint32_t maxUavSpaces, API api)
{
	return make_unique<EZ::CommandList_DX12>(pCommandList, samplerPoolSize, cbvSrvUavPoolSize,
		maxSamplers, pMaxCbvsEachSpace, pMaxSrvsEachSpace, pMaxUavsEachSpace,
		maxCbvSpaces, maxSrvSpaces, maxUavSpaces);
}

EZ::CommandList::sptr EZ::CommandList::MakeShared(XUSG::CommandList* pCommandList,
	uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize, uint32_t maxSamplers,
	const uint32_t* pMaxCbvsEachSpace, const uint32_t* pMaxSrvsEachSpace,
	const uint32_t* pMaxUavsEachSpace, uint32_t maxCbvSpaces,
	uint32_t maxSrvSpaces, uint32_t maxUavSpaces, API api)
{
	return make_shared<EZ::CommandList_DX12>(pCommandList, samplerPoolSize, cbvSrvUavPoolSize,
		maxSamplers, pMaxCbvsEachSpace, pMaxSrvsEachSpace, pMaxUavsEachSpace,
		maxCbvSpaces, maxSrvSpaces, maxUavSpaces);
}
