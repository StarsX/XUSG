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

DescriptorTableLib::uptr DescriptorTableLib::MakeUnique(API api)
{
	return make_unique<DescriptorTableLib_DX12>();
}

DescriptorTableLib::sptr DescriptorTableLib::MakeShared(API api)
{
	return make_shared<DescriptorTableLib_DX12>();
}

DescriptorTableLib::uptr DescriptorTableLib::MakeUnique(const Device* pDevice, const wchar_t* name, API api)
{
	return make_unique<DescriptorTableLib_DX12>(pDevice, name);
}

DescriptorTableLib::sptr DescriptorTableLib::MakeShared(const Device* pDevice, const wchar_t* name, API api)
{
	return make_shared<DescriptorTableLib_DX12>(pDevice, name);
}

Reflector::uptr Reflector::MakeUnique(API api)
{
	return make_unique<Reflector_DX12>();
}

Reflector::sptr Reflector::MakeShared(API api)
{
	return make_shared<Reflector_DX12>();
}

ShaderLib::uptr ShaderLib::MakeUnique(API api)
{
	return make_unique<ShaderLib_DX12>();
}

ShaderLib::sptr ShaderLib::MakeShared(API api)
{
	return make_shared<ShaderLib_DX12>();
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

PipelineLayoutLib::uptr PipelineLayoutLib::MakeUnique(API api)
{
	return make_unique<PipelineLayoutLib_DX12>();
}

PipelineLayoutLib::sptr PipelineLayoutLib::MakeShared(API api)
{
	return make_shared<PipelineLayoutLib_DX12>();
}

PipelineLayoutLib::uptr PipelineLayoutLib::MakeUnique(const Device* pDevice, API api)
{
	return make_unique<PipelineLayoutLib_DX12>(pDevice);
}

PipelineLayoutLib::sptr PipelineLayoutLib::MakeShared(const Device* pDevice, API api)
{
	return make_shared<PipelineLayoutLib_DX12>(pDevice);
}

Graphics::State::uptr Graphics::State::MakeUnique(API api)
{
	return make_unique<State_DX12>();
}

Graphics::State::sptr Graphics::State::MakeShared(API api)
{
	return make_shared<State_DX12>();
}

Graphics::PipelineLib::uptr Graphics::PipelineLib::MakeUnique(API api)
{
	return make_unique<PipelineLib_DX12>();
}

Graphics::PipelineLib::sptr Graphics::PipelineLib::MakeShared(API api)
{
	return make_shared<PipelineLib_DX12>();
}

Graphics::PipelineLib::uptr Graphics::PipelineLib::MakeUnique(const Device* pDevice, API api)
{
	return make_unique<PipelineLib_DX12>(pDevice);
}

Graphics::PipelineLib::sptr Graphics::PipelineLib::MakeShared(const Device* pDevice, API api)
{
	return make_shared<PipelineLib_DX12>(pDevice);
}

Compute::State::uptr Compute::State::MakeUnique(API api)
{
	return make_unique<State_DX12>();
}

Compute::State::sptr Compute::State::MakeShared(API api)
{
	return make_shared<State_DX12>();
}

Compute::PipelineLib::uptr Compute::PipelineLib::MakeUnique(API api)
{
	return make_unique<PipelineLib_DX12>();
}

Compute::PipelineLib::sptr Compute::PipelineLib::MakeShared(API api)
{
	return make_shared<PipelineLib_DX12>();
}

Compute::PipelineLib::uptr Compute::PipelineLib::MakeUnique(const Device* pDevice, API api)
{
	return make_unique<PipelineLib_DX12>(pDevice);
}

Compute::PipelineLib::sptr Compute::PipelineLib::MakeShared(const Device* pDevice, API api)
{
	return make_shared<PipelineLib_DX12>(pDevice);
}

XUSG_INTERFACE Blob XUSG::GetPipelineCache(Pipeline pipeline, API api)
{
	return GetDX12PipelineCache(pipeline);
}

uint8_t XUSG::CalculateMipLevels(uint32_t width, uint32_t height, uint32_t depth)
{
	const auto texSize = (std::max)((std::max)(width, height), depth);

	return Log2(texSize) + 1;
}

uint8_t XUSG::CalculateMipLevels(uint64_t width, uint32_t height, uint32_t depth)
{
	return CalculateMipLevels(static_cast<uint32_t>(width), height, depth);
}

uint8_t XUSG::Log2(uint32_t value)
{
#if defined(WIN32) || (_WIN32)
	unsigned long mssb; // most significant set bit

	if (BitScanReverse(&mssb, value) > 0)
		return static_cast<uint8_t>(mssb);
	else return 0;
#else
	return static_cast<uint8_t>(log2(value));
#endif
}
