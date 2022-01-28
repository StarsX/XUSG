//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSG_DX12.h"
#include "XUSGCommand_DX12.h"
#include "XUSGResource_DX12.h"
#include "XUSGDescriptor_DX12.h"
#include "XUSGReflector_DX12.h"
#include "XUSGShader_DX12.h"
#include "XUSGPipelineLayout_DX12.h"
#include "XUSGGraphicsState_DX12.h"
#include "XUSGComputeState_DX12.h"

using namespace std;
using namespace XUSG;

Device::uptr Device::MakeUnique(API api)
{
	return make_unique<Device_DX12>();
}

Device::sptr Device::MakeShared(API api)
{
	return make_shared<Device_DX12>();
}

Fence::uptr Fence::MakeUnique(API api)
{
	return make_unique<Fence_DX12>();
}

Fence::sptr Fence::MakeShared(API api)
{
	return make_shared<Fence_DX12>();
}

CommandLayout::uptr CommandLayout::MakeUnique(API api)
{
	return make_unique<CommandLayout_DX12>();
}

CommandLayout::sptr CommandLayout::MakeShared(API api)
{
	return make_shared<CommandLayout_DX12>();
}

CommandAllocator::uptr CommandAllocator::MakeUnique(API api)
{
	return make_unique<CommandAllocator_DX12>();
}

CommandAllocator::sptr CommandAllocator::MakeShared(API api)
{
	return make_shared<CommandAllocator_DX12>();
}

CommandList::uptr CommandList::MakeUnique(API api)
{
	return make_unique<CommandList_DX12>();
}

CommandList::sptr CommandList::MakeShared(API api)
{
	return make_shared<CommandList_DX12>();
}

CommandQueue::uptr CommandQueue::MakeUnique(API api)
{
	return make_unique<CommandQueue_DX12>();
}

CommandQueue::sptr CommandQueue::MakeShared(API api)
{
	return make_shared<CommandQueue_DX12>();
}

SwapChain::uptr SwapChain::MakeUnique(API api)
{
	return make_unique<SwapChain_DX12>();
}

SwapChain::sptr SwapChain::MakeShared(API api)
{
	return make_shared<SwapChain_DX12>();
}

Resource::uptr Resource::MakeUnique(API api)
{
	return make_unique<Resource_DX12>();
}

Resource::sptr Resource::MakeShared(API api)
{
	return make_shared<Resource_DX12>();
}

ConstantBuffer::uptr ConstantBuffer::MakeUnique(API api)
{
	return make_unique<ConstantBuffer_DX12>();
}

ConstantBuffer::sptr ConstantBuffer::MakeShared(API api)
{
	return make_shared<ConstantBuffer_DX12>();
}

ShaderResource::uptr ShaderResource::MakeUnique(API api)
{
	return make_unique<ShaderResource_DX12>();
}

ShaderResource::sptr ShaderResource::MakeShared(API api)
{
	return make_shared<ShaderResource_DX12>();
}

Texture* Texture::AsTexture()
{
	return static_cast<Texture*>(this);
}

Texture::uptr Texture::MakeUnique(API api)
{
	return make_unique<Texture_DX12>();
}

Texture::sptr Texture::MakeShared(API api)
{
	return make_shared<Texture_DX12>();
}

RenderTarget::uptr RenderTarget::MakeUnique(API api)
{
	return make_unique<RenderTarget_DX12>();
}

RenderTarget::sptr RenderTarget::MakeShared(API api)
{
	return make_shared<RenderTarget_DX12>();
}

DepthStencil::uptr DepthStencil::MakeUnique(API api)
{
	return make_unique<DepthStencil_DX12>();
}

DepthStencil::sptr DepthStencil::MakeShared(API api)
{
	return make_shared<DepthStencil_DX12>();
}

Texture3D::uptr Texture3D::MakeUnique(API api)
{
	return make_unique<Texture3D_DX12>();
}

Texture3D::sptr Texture3D::MakeShared(API api)
{
	return make_shared<Texture3D_DX12>();
}

Buffer::uptr Buffer::MakeUnique(API api)
{
	return make_unique<Buffer_DX12>();
}

Buffer::sptr Buffer::MakeShared(API api)
{
	return make_shared<Buffer_DX12>();
}

StructuredBuffer::uptr StructuredBuffer::MakeUnique(API api)
{
	return make_unique<StructuredBuffer_DX12>();
}

StructuredBuffer::sptr StructuredBuffer::MakeShared(API api)
{
	return make_shared<StructuredBuffer_DX12>();
}

TypedBuffer::uptr TypedBuffer::MakeUnique(API api)
{
	return make_unique<TypedBuffer_DX12>();
}

TypedBuffer::sptr TypedBuffer::MakeShared(API api)
{
	return make_shared<TypedBuffer_DX12>();
}

VertexBuffer::uptr VertexBuffer::MakeUnique(API api)
{
	return make_unique<VertexBuffer_DX12>();
}

VertexBuffer::sptr VertexBuffer::MakeShared(API api)
{
	return make_shared<VertexBuffer_DX12>();
}

IndexBuffer::uptr IndexBuffer::MakeUnique(API api)
{
	return make_unique<IndexBuffer_DX12>();
}

IndexBuffer::sptr IndexBuffer::MakeShared(API api)
{
	return make_shared<IndexBuffer_DX12>();
}

Util::DescriptorTable::uptr Util::DescriptorTable::MakeUnique(API api)
{
	return make_unique<DescriptorTable_DX12>();
}

Util::DescriptorTable::sptr Util::DescriptorTable::MakeShared(API api)
{
	return make_shared<DescriptorTable_DX12>();
}

DescriptorTableCache::uptr DescriptorTableCache::MakeUnique(API api)
{
	return make_unique<DescriptorTableCache_DX12>();
}

DescriptorTableCache::sptr DescriptorTableCache::MakeShared(API api)
{
	return make_shared<DescriptorTableCache_DX12>();
}

DescriptorTableCache::uptr DescriptorTableCache::MakeUnique(const Device* pDevice, const wchar_t* name, API api)
{
	return make_unique<DescriptorTableCache_DX12>(pDevice, name);
}

DescriptorTableCache::sptr DescriptorTableCache::MakeShared(const Device* pDevice, const wchar_t* name, API api)
{
	return make_shared<DescriptorTableCache_DX12>(pDevice, name);
}

Reflector::uptr Reflector::MakeUnique(API api)
{
	return make_unique<Reflector_DX12>();
}

Reflector::sptr Reflector::MakeShared(API api)
{
	return make_shared<Reflector_DX12>();
}

ShaderPool::uptr ShaderPool::MakeUnique(API api)
{
	return make_unique<ShaderPool_DX12>();
}

ShaderPool::sptr ShaderPool::MakeShared(API api)
{
	return make_shared<ShaderPool_DX12>();
}

Util::PipelineLayout::uptr Util::PipelineLayout::MakeUnique(API api)
{
	return make_unique<PipelineLayout_DX12>();
}

Util::PipelineLayout::sptr Util::PipelineLayout::MakeShared(API api)
{
	return make_shared<PipelineLayout_DX12>();
}

Util::PipelineLayout::uptr Util::PipelineLayout::CloneUnique(const PipelineLayout* pSrc, API api)
{
	auto dst = make_unique<PipelineLayout_DX12>();
	*dst = *dynamic_cast<const PipelineLayout_DX12*>(pSrc);

	return dst;
}

Util::PipelineLayout::sptr Util::PipelineLayout::CloneShared(const PipelineLayout* pSrc, API api)
{
	auto dst = make_shared<PipelineLayout_DX12>();
	*dst = *dynamic_cast<const PipelineLayout_DX12*>(pSrc);
	
	return dst;
}

PipelineLayoutCache::uptr PipelineLayoutCache::MakeUnique(API api)
{
	return make_unique<PipelineLayoutCache_DX12>();
}

PipelineLayoutCache::sptr PipelineLayoutCache::MakeShared(API api)
{
	return make_shared<PipelineLayoutCache_DX12>();
}

PipelineLayoutCache::uptr PipelineLayoutCache::MakeUnique(const Device* pDevice, API api)
{
	return make_unique<PipelineLayoutCache_DX12>(pDevice);
}

PipelineLayoutCache::sptr PipelineLayoutCache::MakeShared(const Device* pDevice, API api)
{
	return make_shared<PipelineLayoutCache_DX12>(pDevice);
}

Graphics::State::uptr Graphics::State::MakeUnique(API api)
{
	return make_unique<State_DX12>();
}

Graphics::State::sptr Graphics::State::MakeShared(API api)
{
	return make_shared<State_DX12>();
}

Graphics::PipelineCache::uptr Graphics::PipelineCache::MakeUnique(API api)
{
	return make_unique<PipelineCache_DX12>();
}

Graphics::PipelineCache::sptr Graphics::PipelineCache::MakeShared(API api)
{
	return make_shared<PipelineCache_DX12>();
}

Graphics::PipelineCache::uptr Graphics::PipelineCache::MakeUnique(const Device* pDevice, API api)
{
	return make_unique<PipelineCache_DX12>(pDevice);
}

Graphics::PipelineCache::sptr Graphics::PipelineCache::MakeShared(const Device* pDevice, API api)
{
	return make_shared<PipelineCache_DX12>(pDevice);
}

Compute::State::uptr Compute::State::MakeUnique(API api)
{
	return make_unique<State_DX12>();
}

Compute::State::sptr Compute::State::MakeShared(API api)
{
	return make_shared<State_DX12>();
}

Compute::PipelineCache::uptr Compute::PipelineCache::MakeUnique(API api)
{
	return make_unique<PipelineCache_DX12>();
}

Compute::PipelineCache::sptr Compute::PipelineCache::MakeShared(API api)
{
	return make_shared<PipelineCache_DX12>();
}

Compute::PipelineCache::uptr Compute::PipelineCache::MakeUnique(const Device* pDevice, API api)
{
	return make_unique<PipelineCache_DX12>(pDevice);
}

Compute::PipelineCache::sptr Compute::PipelineCache::MakeShared(const Device* pDevice, API api)
{
	return make_shared<PipelineCache_DX12>(pDevice);
}

uint32_t XUSG::DivideAndRoundUp(uint32_t x, uint32_t n)
{
	return DIV_UP(x, n);
}
