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

ResourceView EZ::GetCBV(ConstantBuffer* pResource, uint32_t index)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.view = pResource->GetCBV(index);

	return resourceView;
}

ResourceView EZ::GetSRV(Buffer* pResource, uint32_t index)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.view = pResource->GetSRV(index);
	resourceView.Subresources = { XUSG_BARRIER_ALL_SUBRESOURCES };

	return resourceView;
}

ResourceView EZ::GetSRV(Texture* pResource, uint32_t index)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.view = pResource->GetSRV(index);

	const auto numMips = pResource->GetNumMips();
	const auto arraySize = pResource->GetArraySize();
	resourceView.Subresources.resize(arraySize * (numMips - index));

	for (auto i = index; i < numMips; ++i)
		for (auto j = 0u; j < arraySize; ++j)
			resourceView.Subresources[i] = CalcSubresource(pResource, i, j);

	return resourceView;
}

ResourceView EZ::GetSRVLevel(Texture* pResource, uint8_t level)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.view = pResource->GetSRVLevel(level);
	CalcSubresources(resourceView.Subresources, pResource, level);

	return resourceView;
}

ResourceView EZ::GetUAV(Buffer* pResource, uint8_t index)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.view = pResource->GetUAV(index);
	resourceView.Subresources = { XUSG_BARRIER_ALL_SUBRESOURCES };

	return resourceView;
}

ResourceView EZ::GetUAV(Texture* pResource, uint8_t index)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.view = pResource->GetUAV(index);
	CalcSubresources(resourceView.Subresources, pResource, index);

	return resourceView;
}

ResourceView EZ::GetPackedUAV(Texture* pResource, uint8_t index)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.view = pResource->GetUAV(index);
	CalcSubresources(resourceView.Subresources, pResource, index);

	return resourceView;
}

ResourceView EZ::GetPackedUAV(TypedBuffer* pResource, uint8_t index)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.view = pResource->GetUAV(index);
	resourceView.Subresources = { XUSG_BARRIER_ALL_SUBRESOURCES };

	return resourceView;
}

ResourceView EZ::GetRTV(RenderTarget* pResource, uint32_t slice, uint8_t mipLevel)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.view = pResource->GetRTV(slice, mipLevel);
	resourceView.Subresources = { CalcSubresource(pResource, mipLevel, slice) };

	return resourceView;
}

ResourceView EZ::GetArrayRTV(RenderTarget* pResource, uint8_t mipLevel)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.view = pResource->GetRTV(0, mipLevel);
	CalcSubresources(resourceView.Subresources, pResource, mipLevel);

	return resourceView;
}

ResourceView EZ::GetDSV(DepthStencil* pResource, uint32_t slice, uint8_t mipLevel)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.view = pResource->GetDSV(slice, mipLevel);
	resourceView.Subresources = { CalcSubresource(pResource, mipLevel, slice) };

	return resourceView;
}

ResourceView EZ::GetArrayDSV(DepthStencil* pResource, uint8_t mipLevel)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.view = pResource->GetDSV(0, mipLevel);
	CalcSubresources(resourceView.Subresources, pResource, mipLevel);

	return resourceView;
}

ResourceView EZ::GetReadOnlyDSV(DepthStencil* pResource, uint32_t slice, uint8_t mipLevel)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.view = pResource->GetReadOnlyDSV(slice, mipLevel);
	resourceView.Subresources = { CalcSubresource(pResource, mipLevel, slice) };

	return resourceView;
}

ResourceView EZ::GetReadOnlyArrayDSV(DepthStencil* pResource, uint8_t mipLevel)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.view = pResource->GetReadOnlyDSV(0, mipLevel);
	CalcSubresources(resourceView.Subresources, pResource, mipLevel);

	return resourceView;
}

ResourceView EZ::GetStencilSRV(DepthStencil* pResource)
{
	ResourceView resourceView;
	resourceView.pResource = pResource;
	resourceView.view = pResource->GetStencilSRV();
	resourceView.Subresources = { XUSG_BARRIER_ALL_SUBRESOURCES };

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
