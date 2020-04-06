//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGCommand.h"
#include "XUSGResource.h"
#include "XUSGDescriptor.h"
#include "XUSGReflector.h"
#include "XUSGShader.h"
#include "XUSGPipelineLayout.h"
#include "XUSGGraphicsState.h"
#include "XUSGComputeState.h"

using namespace std;
using namespace XUSG;

CommandList::uptr CommandList::MakeUnique(API api)
{
	return make_unique<CommandList_DX12>();
}

CommandList::sptr CommandList::MakeShared(API api)
{
	return make_shared<CommandList_DX12>();
}

ConstantBuffer::uptr ConstantBuffer::MakeUnique(API api)
{
	return make_unique<ConstantBuffer_DX12>();
}

ConstantBuffer::sptr ConstantBuffer::MakeShared(API api)
{
	return make_shared<ConstantBuffer_DX12>();
}

ResourceBase::uptr ResourceBase::MakeUnique(API api)
{
	return make_unique<ResourceBase_DX12>();
}

ResourceBase::sptr ResourceBase::MakeShared(API api)
{
	return make_shared<ResourceBase_DX12>();
}

Texture2D* Texture2D::AsTexture2D()
{
	return static_cast<Texture2D*>(this);
}

Texture2D::uptr Texture2D::MakeUnique(API api)
{
	return make_unique<Texture2D_DX12>();
}

Texture2D::sptr Texture2D::MakeShared(API api)
{
	return make_shared<Texture2D_DX12>();
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

RawBuffer::uptr RawBuffer::MakeUnique(API api)
{
	return make_unique<RawBuffer_DX12>();
}

RawBuffer::sptr RawBuffer::MakeShared(API api)
{
	return make_shared<RawBuffer_DX12>();
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

DescriptorTableCache::uptr DescriptorTableCache::MakeUnique(const Device& device, const wchar_t* name, API api)
{
	return make_unique<DescriptorTableCache_DX12>(device, name);
}

DescriptorTableCache::sptr DescriptorTableCache::MakeShared(const Device& device, const wchar_t* name, API api)
{
	return make_shared<DescriptorTableCache_DX12>(device, name);
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
	*dst = *static_cast<const PipelineLayout_DX12*>(pSrc);

	return dst;
}

Util::PipelineLayout::sptr Util::PipelineLayout::CloneShared(const PipelineLayout* pSrc, API api)
{
	auto dst = make_shared<PipelineLayout_DX12>();
	*dst = *static_cast<const PipelineLayout_DX12*>(pSrc);
	
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

PipelineLayoutCache::uptr PipelineLayoutCache::MakeUnique(const Device& device, API api)
{
	return make_unique<PipelineLayoutCache_DX12>(device);
}

PipelineLayoutCache::sptr PipelineLayoutCache::MakeShared(const Device& device, API api)
{
	return make_shared<PipelineLayoutCache_DX12>(device);
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

Graphics::PipelineCache::uptr Graphics::PipelineCache::MakeUnique(const Device& device, API api)
{
	return make_unique<PipelineCache_DX12>(device);
}

Graphics::PipelineCache::sptr Graphics::PipelineCache::MakeShared(const Device& device, API api)
{
	return make_shared<PipelineCache_DX12>(device);
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

Compute::PipelineCache::uptr Compute::PipelineCache::MakeUnique(const Device& device, API api)
{
	return make_unique<PipelineCache_DX12>(device);
}

Compute::PipelineCache::sptr Compute::PipelineCache::MakeShared(const Device& device, API api)
{
	return make_shared<PipelineCache_DX12>(device);
}
