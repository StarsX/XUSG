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

unique_ptr<CommandList> CommandList::MakeUnique(API api)
{
	return make_unique<CommandList_DX12>();
}

shared_ptr<CommandList> CommandList::MakeShared(API api)
{
	return make_shared<CommandList_DX12>();
}

unique_ptr<ConstantBuffer> ConstantBuffer::MakeUnique(API api)
{
	return make_unique<ConstantBuffer_DX12>();
}

shared_ptr<ConstantBuffer> ConstantBuffer::MakeShared(API api)
{
	return make_shared<ConstantBuffer_DX12>();
}

unique_ptr<ResourceBase> ResourceBase::MakeUnique(API api)
{
	return make_unique<ResourceBase_DX12>();
}

shared_ptr<ResourceBase> ResourceBase::MakeShared(API api)
{
	return make_shared<ResourceBase_DX12>();
}

unique_ptr<Texture2D> Texture2D::MakeUnique(API api)
{
	return make_unique<Texture2D_DX12>();
}

shared_ptr<Texture2D> Texture2D::MakeShared(API api)
{
	return make_shared<Texture2D_DX12>();
}

unique_ptr<RenderTarget> RenderTarget::MakeUnique(API api)
{
	return make_unique<RenderTarget_DX12>();
}

shared_ptr<RenderTarget> RenderTarget::MakeShared(API api)
{
	return make_shared<RenderTarget_DX12>();
}

unique_ptr<DepthStencil> DepthStencil::MakeUnique(API api)
{
	return make_unique<DepthStencil_DX12>();
}

shared_ptr<DepthStencil> DepthStencil::MakeShared(API api)
{
	return make_shared<DepthStencil_DX12>();
}

unique_ptr<Texture3D> Texture3D::MakeUnique(API api)
{
	return make_unique<Texture3D_DX12>();
}

shared_ptr<Texture3D> Texture3D::MakeShared(API api)
{
	return make_shared<Texture3D_DX12>();
}

unique_ptr<RawBuffer> RawBuffer::MakeUnique(API api)
{
	return make_unique<RawBuffer_DX12>();
}

shared_ptr<RawBuffer> RawBuffer::MakeShared(API api)
{
	return make_shared<RawBuffer_DX12>();
}

unique_ptr<StructuredBuffer> StructuredBuffer::MakeUnique(API api)
{
	return make_unique<StructuredBuffer_DX12>();
}

shared_ptr<StructuredBuffer> StructuredBuffer::MakeShared(API api)
{
	return make_shared<StructuredBuffer_DX12>();
}

unique_ptr<TypedBuffer> TypedBuffer::MakeUnique(API api)
{
	return make_unique<TypedBuffer_DX12>();
}

shared_ptr<TypedBuffer> TypedBuffer::MakeShared(API api)
{
	return make_shared<TypedBuffer_DX12>();
}

unique_ptr<VertexBuffer> VertexBuffer::MakeUnique(API api)
{
	return make_unique<VertexBuffer_DX12>();
}

shared_ptr<VertexBuffer> VertexBuffer::MakeShared(API api)
{
	return make_shared<VertexBuffer_DX12>();
}

unique_ptr<IndexBuffer> IndexBuffer::MakeUnique(API api)
{
	return make_unique<IndexBuffer_DX12>();
}

shared_ptr<IndexBuffer> IndexBuffer::MakeShared(API api)
{
	return make_shared<IndexBuffer_DX12>();
}

unique_ptr<Util::DescriptorTable> Util::DescriptorTable::MakeUnique(API api)
{
	return make_unique<DescriptorTable_DX12>();
}

shared_ptr<Util::DescriptorTable> Util::DescriptorTable::MakeShared(API api)
{
	return make_shared<DescriptorTable_DX12>();
}

unique_ptr<DescriptorTableCache> DescriptorTableCache::MakeUnique(API api)
{
	return make_unique<DescriptorTableCache_DX12>();
}

shared_ptr<DescriptorTableCache> DescriptorTableCache::MakeShared(API api)
{
	return make_shared<DescriptorTableCache_DX12>();
}

unique_ptr<DescriptorTableCache> DescriptorTableCache::MakeUnique(const Device& device, const wchar_t* name, API api)
{
	return make_unique<DescriptorTableCache_DX12>(device, name);
}

shared_ptr<DescriptorTableCache> DescriptorTableCache::MakeShared(const Device& device, const wchar_t* name, API api)
{
	return make_shared<DescriptorTableCache_DX12>(device, name);
}

unique_ptr<Reflector> Reflector::MakeUnique(API api)
{
	return make_unique<Reflector_DX12>();
}

shared_ptr<Reflector> Reflector::MakeShared(API api)
{
	return make_shared<Reflector_DX12>();
}

unique_ptr<ShaderPool> ShaderPool::MakeUnique(API api)
{
	return make_unique<ShaderPool_DX12>();
}

shared_ptr<ShaderPool> ShaderPool::MakeShared(API api)
{
	return make_shared<ShaderPool_DX12>();
}

unique_ptr<Util::PipelineLayout> Util::PipelineLayout::MakeUnique(API api)
{
	return make_unique<PipelineLayout_DX12>();
}

shared_ptr<Util::PipelineLayout> Util::PipelineLayout::MakeShared(API api)
{
	return make_shared<PipelineLayout_DX12>();
}

unique_ptr<PipelineLayoutCache> PipelineLayoutCache::MakeUnique(API api)
{
	return make_unique<PipelineLayoutCache_DX12>();
}

shared_ptr<PipelineLayoutCache> PipelineLayoutCache::MakeShared(API api)
{
	return make_shared<PipelineLayoutCache_DX12>();
}

unique_ptr<PipelineLayoutCache> PipelineLayoutCache::MakeUnique(const Device& device, API api)
{
	return make_unique<PipelineLayoutCache_DX12>(device);
}

shared_ptr<PipelineLayoutCache> PipelineLayoutCache::MakeShared(const Device& device, API api)
{
	return make_shared<PipelineLayoutCache_DX12>(device);
}

unique_ptr<Graphics::State> Graphics::State::MakeUnique(API api)
{
	return make_unique<State_DX12>();
}

shared_ptr<Graphics::State> Graphics::State::MakeShared(API api)
{
	return make_shared<State_DX12>();
}

unique_ptr<Graphics::PipelineCache> Graphics::PipelineCache::MakeUnique(API api)
{
	return make_unique<PipelineCache_DX12>();
}

shared_ptr<Graphics::PipelineCache> Graphics::PipelineCache::MakeShared(API api)
{
	return make_shared<PipelineCache_DX12>();
}

unique_ptr<Graphics::PipelineCache> Graphics::PipelineCache::MakeUnique(const Device& device, API api)
{
	return make_unique<PipelineCache_DX12>(device);
}

shared_ptr<Graphics::PipelineCache> Graphics::PipelineCache::MakeShared(const Device& device, API api)
{
	return make_shared<PipelineCache_DX12>(device);
}

unique_ptr<Compute::State> Compute::State::MakeUnique(API api)
{
	return make_unique<State_DX12>();
}

shared_ptr<Compute::State> Compute::State::MakeShared(API api)
{
	return make_shared<State_DX12>();
}

unique_ptr<Compute::PipelineCache> Compute::PipelineCache::MakeUnique(API api)
{
	return make_unique<PipelineCache_DX12>();
}

shared_ptr<Compute::PipelineCache> Compute::PipelineCache::MakeShared(API api)
{
	return make_shared<PipelineCache_DX12>();
}

unique_ptr<Compute::PipelineCache> Compute::PipelineCache::MakeUnique(const Device& device, API api)
{
	return make_unique<PipelineCache_DX12>(device);
}

shared_ptr<Compute::PipelineCache> Compute::PipelineCache::MakeShared(const Device& device, API api)
{
	return make_shared<PipelineCache_DX12>(device);
}
