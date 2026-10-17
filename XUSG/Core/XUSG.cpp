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
#ifndef XUSG_NO_VULKAN
#include "XUSG_VK.h"
#include "XUSGCommand_VK.h"
#include "XUSGResource_VK.h"
#include "XUSGDescriptor_VK.h"
#include "XUSGReflector_VK.h"
#include "XUSGShader_VK.h"
#include "XUSGPipelineLayout_VK.h"
#include "XUSGGraphicsState_VK.h"
#include "XUSGComputeState_VK.h"
#endif

using namespace std;
using namespace XUSG;

#ifdef XUSG_NO_VULKAN
#define XUSG_IF_VK(...)
#else
#define XUSG_IF_VK(...) if (Device::GetApi() == API::VULKAN) { __VA_ARGS__ }
#endif

static bool g_apiLocked = false;
static API g_api = API::DIRECTX_12;

void Device::LockApi()
{
	g_apiLocked = true;
}

void Device::UnlockApi()
{
	g_apiLocked = false;
}

void Device::SetApi(API api)
{
	assert(!g_apiLocked);
	if (g_apiLocked) return;
#ifdef XUSG_NO_VULKAN
	assert(api == API::DIRECTX_12 && "Vulkan is not available in this build");
	if (api != API::DIRECTX_12) return;
#endif

	g_api = api;
}

API Device::GetApi()
{
	LockApi();

	return g_api;
}

Device::uptr Device::MakeUnique()
{
	Device::LockApi();
	XUSG_IF_VK(return make_unique<Device_VK>();)

	return make_unique<Device_DX12>();
}

Device::sptr Device::MakeShared()
{
	Device::LockApi();
	XUSG_IF_VK(return make_shared<Device_VK>();)

	return make_shared<Device_DX12>();
}

Fence::uptr Fence::MakeUnique()
{
	Device::LockApi();
	XUSG_IF_VK(return make_unique<Fence_VK>();)

	return make_unique<Fence_DX12>();
}

Fence::sptr Fence::MakeShared()
{
	Device::LockApi();
	XUSG_IF_VK(return make_shared<Fence_VK>();)

	return make_shared<Fence_DX12>();
}

CommandLayout::uptr CommandLayout::MakeUnique()
{
	Device::LockApi();
	XUSG_IF_VK(return make_unique<CommandLayout_VK>();)

	return make_unique<CommandLayout_DX12>();
}

CommandLayout::sptr CommandLayout::MakeShared()
{
	Device::LockApi();
	XUSG_IF_VK(return make_shared<CommandLayout_VK>();)

	return make_shared<CommandLayout_DX12>();
}

CommandAllocator::uptr CommandAllocator::MakeUnique()
{
	Device::LockApi();
	XUSG_IF_VK(return make_unique<CommandAllocator_VK>();)

	return make_unique<CommandAllocator_DX12>();
}

CommandAllocator::sptr CommandAllocator::MakeShared()
{
	Device::LockApi();
	XUSG_IF_VK(return make_shared<CommandAllocator_VK>();)

	return make_shared<CommandAllocator_DX12>();
}

CommandList::uptr CommandList::MakeUnique()
{
	Device::LockApi();
	XUSG_IF_VK(return make_unique<CommandList_VK>();)

	return make_unique<CommandList_DX12>();
}

CommandList::sptr CommandList::MakeShared()
{
	Device::LockApi();
	XUSG_IF_VK(return make_shared<CommandList_VK>();)

	return make_shared<CommandList_DX12>();
}

CommandQueue::uptr CommandQueue::MakeUnique()
{
	Device::LockApi();
	XUSG_IF_VK(return make_unique<CommandQueue_VK>();)

	return make_unique<CommandQueue_DX12>();
}

CommandQueue::sptr CommandQueue::MakeShared()
{
	Device::LockApi();
	XUSG_IF_VK(return make_shared<CommandQueue_VK>();)

	return make_shared<CommandQueue_DX12>();
}

SwapChain::uptr SwapChain::MakeUnique()
{
	Device::LockApi();
	XUSG_IF_VK(return make_unique<SwapChain_VK>();)

	return make_unique<SwapChain_DX12>();
}

SwapChain::sptr SwapChain::MakeShared()
{
	Device::LockApi();
	XUSG_IF_VK(return make_shared<SwapChain_VK>();)

	return make_shared<SwapChain_DX12>();
}

Heap::uptr Heap::MakeUnique()
{
	Device::LockApi();
	XUSG_IF_VK(return make_unique<Heap_VK>();)

	return make_unique<Heap_DX12>();
}

Heap::sptr Heap::MakeShared()
{
	Device::LockApi();
	XUSG_IF_VK(return make_shared<Heap_VK>();)

	return make_shared<Heap_DX12>();
}

Resource::uptr Resource::MakeUnique()
{
	Device::LockApi();
	XUSG_IF_VK(return make_unique<Resource_VK>();)

	return make_unique<Resource_DX12>();
}

Resource::sptr Resource::MakeShared()
{
	Device::LockApi();
	XUSG_IF_VK(return make_shared<Resource_VK>();)

	return make_shared<Resource_DX12>();
}

BarrierLayout Resource::GetBarrierLayout(ResourceState resourceState)
{
	auto bufferState = ResourceState::VERTEX_AND_CONSTANT_BUFFER;
	bufferState |= ResourceState::INDEX_BUFFER;
	bufferState |= ResourceState::STREAM_OUT;
	bufferState |= ResourceState::INDIRECT_ARGUMENT;
	bufferState |= ResourceState::RAYTRACING_ACCELERATION_STRUCTURE;
	assert((resourceState & bufferState) == ResourceState::COMMON);

	switch (resourceState)
	{
	case ResourceState::COMMON:
		return BarrierLayout::COMMON;
	case ResourceState::RENDER_TARGET:
		return BarrierLayout::RENDER_TARGET;
	case ResourceState::UNORDERED_ACCESS:
		return BarrierLayout::UNORDERED_ACCESS;
	case ResourceState::DEPTH_WRITE:
		return BarrierLayout::DEPTH_STENCIL_WRITE;
	case ResourceState::DEPTH_READ:
		return BarrierLayout::DEPTH_STENCIL_READ;
	case ResourceState::NON_PIXEL_SHADER_RESOURCE:
	case ResourceState::PIXEL_SHADER_RESOURCE:
	case ResourceState::ALL_SHADER_RESOURCE:
		return BarrierLayout::SHADER_RESOURCE;
	case ResourceState::COPY_DEST:
		return BarrierLayout::COPY_DEST;
	case ResourceState::COPY_SOURCE:
		return BarrierLayout::COPY_SOURCE;
	case ResourceState::RESOLVE_DEST:
		return BarrierLayout::RESOLVE_DEST;
	case ResourceState::RESOLVE_SOURCE:
		return BarrierLayout::RESOLVE_SOURCE;
	case ResourceState::SHADING_RATE_SOURCE:
		return BarrierLayout::SHADING_RATE_SOURCE;
	}

	if ((resourceState & (ResourceState::DEPTH_READ | ResourceState::RESOLVE_SOURCE)) != ResourceState::COMMON)
		return BarrierLayout::DIRECT_QUEUE_GENERIC_READ;
	else if ((resourceState & (ResourceState::ALL_SHADER_RESOURCE | ResourceState::COPY_SOURCE)) != ResourceState::COMMON)
		return BarrierLayout::GENERIC_READ_RESOURCE;

	return BarrierLayout::UNDEFINED;
}

ConstantBuffer::uptr ConstantBuffer::MakeUnique()
{
	Device::LockApi();
	XUSG_IF_VK(return make_unique<ConstantBuffer_VK>();)

	return make_unique<ConstantBuffer_DX12>();
}

ConstantBuffer::sptr ConstantBuffer::MakeShared()
{
	Device::LockApi();
	XUSG_IF_VK(return make_shared<ConstantBuffer_VK>();)

	return make_shared<ConstantBuffer_DX12>();
}

void ConstantBuffer::GetAllocationInfo(uint64_t& byteSize, uint64_t& alignment, Device* pDevice,
	size_t byteWidth, uint64_t requiredAlignment)
{
#ifndef XUSG_NO_VULKAN
	if (Device::GetApi() == API::VULKAN)
		return ConstantBuffer_VK::GetAllocationInfo(byteSize, alignment, pDevice, byteWidth, requiredAlignment);
#endif

	ConstantBuffer_DX12::GetAllocationInfo(byteSize, alignment, pDevice, byteWidth, requiredAlignment);
}

size_t ConstantBuffer::AlignCBV(size_t byteSize)
{
	XUSG_IF_VK(return ConstantBuffer_VK::AlignCBV(byteSize);)

	return ConstantBuffer_DX12::AlignCBV(byteSize);
}

ShaderResource::uptr ShaderResource::MakeUnique()
{
	Device::LockApi();
	XUSG_IF_VK(return make_unique<ShaderResource_VK>();)

	return make_unique<ShaderResource_DX12>();
}

ShaderResource::sptr ShaderResource::MakeShared()
{
	Device::LockApi();
	XUSG_IF_VK(return make_shared<ShaderResource_VK>();)

	return make_shared<ShaderResource_DX12>();
}

Texture::uptr Texture::MakeUnique()
{
	Device::LockApi();
	XUSG_IF_VK(return make_unique<Texture_VK>();)

	return make_unique<Texture_DX12>();
}

Texture::sptr Texture::MakeShared()
{
	Device::LockApi();
	XUSG_IF_VK(return make_shared<Texture_VK>();)

	return make_shared<Texture_DX12>();
}

void Texture::GetAllocationInfo(uint64_t& byteSize, uint64_t& alignment, Device* pDevice,
	uint32_t width, uint32_t height, Format format, uint16_t arraySize, ResourceFlag resourceFlags,
	uint8_t numMips, uint8_t sampleCount, TextureLayout textureLayout, uint64_t requiredAlignment)
{
#ifndef XUSG_NO_VULKAN
	if (Device::GetApi() == API::VULKAN)
		return Texture_VK::GetAllocationInfo(byteSize, alignment, pDevice, width, height, format,
			arraySize, resourceFlags, numMips, sampleCount, textureLayout, requiredAlignment);
#endif

	Texture_DX12::GetAllocationInfo(byteSize, alignment, pDevice, width, height, format,
		arraySize, resourceFlags, numMips, sampleCount, textureLayout, requiredAlignment);
}

uint8_t Texture::CalculateMipLevels(uint32_t width, uint32_t height, uint32_t depth)
{
	const auto texSize = (max)((max)(width, height), depth);

	return Log2(texSize) + 1;
}

RenderTarget::uptr RenderTarget::MakeUnique()
{
	Device::LockApi();
	XUSG_IF_VK(return make_unique<RenderTarget_VK>();)

	return make_unique<RenderTarget_DX12>();
}

RenderTarget::sptr RenderTarget::MakeShared()
{
	Device::LockApi();
	XUSG_IF_VK(return make_shared<RenderTarget_VK>();)

	return make_shared<RenderTarget_DX12>();
}

void RenderTarget::GetAllocationInfo(uint64_t& byteSize, uint64_t& alignment, Device* pDevice,
	uint32_t width, uint32_t height, Format format, uint16_t arraySize, ResourceFlag resourceFlags,
	uint8_t numMips, uint8_t sampleCount, TextureLayout textureLayout, uint64_t requiredAlignment)
{
	Texture::GetAllocationInfo(byteSize, alignment, pDevice, width, height,
		format, arraySize, ResourceFlag::ALLOW_RENDER_TARGET | resourceFlags,
		numMips, sampleCount, textureLayout, requiredAlignment);
}

DepthStencil::uptr DepthStencil::MakeUnique()
{
	Device::LockApi();
	XUSG_IF_VK(return make_unique<DepthStencil_VK>();)

	return make_unique<DepthStencil_DX12>();
}

DepthStencil::sptr DepthStencil::MakeShared()
{
	Device::LockApi();
	XUSG_IF_VK(return make_shared<DepthStencil_VK>();)

	return make_shared<DepthStencil_DX12>();
}

void DepthStencil::GetAllocationInfo(uint64_t& byteSize, uint64_t& alignment, Device* pDevice,
	uint32_t width, uint32_t height, Format format, uint16_t arraySize, ResourceFlag resourceFlags,
	uint8_t numMips, uint8_t sampleCount, TextureLayout textureLayout, uint64_t requiredAlignment)
{
	Texture::GetAllocationInfo(byteSize, alignment, pDevice, width, height,
		format, arraySize, ResourceFlag::ALLOW_DEPTH_STENCIL | resourceFlags,
		numMips, sampleCount, textureLayout, requiredAlignment);
}

Texture3D::uptr Texture3D::MakeUnique()
{
	Device::LockApi();
	XUSG_IF_VK(return make_unique<Texture3D_VK>();)

	return make_unique<Texture3D_DX12>();
}

Texture3D::sptr Texture3D::MakeShared()
{
	Device::LockApi();
	XUSG_IF_VK(return make_shared<Texture3D_VK>();)

	return make_shared<Texture3D_DX12>();
}

void Texture3D::GetAllocationInfo(uint64_t& byteSize, uint64_t& alignment, Device* pDevice,
	uint32_t width, uint32_t height, uint16_t depth, Format format, ResourceFlag resourceFlags,
	uint8_t numMips, TextureLayout textureLayout, uint64_t requiredAlignment)
{
#ifndef XUSG_NO_VULKAN
	if (Device::GetApi() == API::VULKAN)
		return Texture3D_VK::GetAllocationInfo(byteSize, alignment, pDevice, width, height, depth,
			format, resourceFlags, numMips, textureLayout, requiredAlignment);
#endif

	Texture3D_DX12::GetAllocationInfo(byteSize, alignment, pDevice, width, height, depth,
		format, resourceFlags, numMips, textureLayout, requiredAlignment);
}

Buffer::uptr Buffer::MakeUnique()
{
	Device::LockApi();
	XUSG_IF_VK(return make_unique<Buffer_VK>();)

	return make_unique<Buffer_DX12>();
}

Buffer::sptr Buffer::MakeShared()
{
	Device::LockApi();
	XUSG_IF_VK(return make_shared<Buffer_VK>();)

	return make_shared<Buffer_DX12>();
}

void Buffer::GetAllocationInfo(uint64_t& byteSize, uint64_t& alignment, Device* pDevice,
	size_t byteWidth, ResourceFlag resourceFlags, uint64_t requiredAlignment)
{
#ifndef XUSG_NO_VULKAN
	if (Device::GetApi() == API::VULKAN)
		return Buffer_VK::GetAllocationInfo(byteSize, alignment, pDevice, byteWidth, resourceFlags, requiredAlignment);
#endif

	Buffer_DX12::GetAllocationInfo(byteSize, alignment, pDevice, byteWidth, resourceFlags, requiredAlignment);
}

size_t Buffer::AlignRawView(size_t byteSize)
{
	XUSG_IF_VK(return Buffer_VK::AlignRawView(byteSize);)

	return Buffer_DX12::AlignRawView(byteSize);
}

size_t Buffer::AlignCounter(size_t byteSize)
{
	XUSG_IF_VK(return Buffer_VK::AlignCounter(byteSize);)

	return Buffer_DX12::AlignCounter(byteSize);
}

StructuredBuffer::uptr StructuredBuffer::MakeUnique()
{
	Device::LockApi();
	XUSG_IF_VK(return make_unique<StructuredBuffer_VK>();)

	return make_unique<StructuredBuffer_DX12>();
}

StructuredBuffer::sptr StructuredBuffer::MakeShared()
{
	Device::LockApi();
	XUSG_IF_VK(return make_shared<StructuredBuffer_VK>();)

	return make_shared<StructuredBuffer_DX12>();
}

TypedBuffer::uptr TypedBuffer::MakeUnique()
{
	Device::LockApi();
	XUSG_IF_VK(return make_unique<TypedBuffer_VK>();)

	return make_unique<TypedBuffer_DX12>();
}

TypedBuffer::sptr TypedBuffer::MakeShared()
{
	Device::LockApi();
	XUSG_IF_VK(return make_shared<TypedBuffer_VK>();)

	return make_shared<TypedBuffer_DX12>();
}

VertexBuffer::uptr VertexBuffer::MakeUnique()
{
	Device::LockApi();
	XUSG_IF_VK(return make_unique<VertexBuffer_VK>();)

	return make_unique<VertexBuffer_DX12>();
}

VertexBuffer::sptr VertexBuffer::MakeShared()
{
	Device::LockApi();
	XUSG_IF_VK(return make_shared<VertexBuffer_VK>();)

	return make_shared<VertexBuffer_DX12>();
}

IndexBuffer::uptr IndexBuffer::MakeUnique()
{
	Device::LockApi();
	XUSG_IF_VK(return make_unique<IndexBuffer_VK>();)

	return make_unique<IndexBuffer_DX12>();
}

IndexBuffer::sptr IndexBuffer::MakeShared()
{
	Device::LockApi();
	XUSG_IF_VK(return make_shared<IndexBuffer_VK>();)

	return make_shared<IndexBuffer_DX12>();
}

Util::DescriptorTable::uptr Util::DescriptorTable::MakeUnique()
{
	Device::LockApi();
	XUSG_IF_VK(return make_unique<DescriptorTable_VK>();)

	return make_unique<DescriptorTable_DX12>();
}

Util::DescriptorTable::sptr Util::DescriptorTable::MakeShared()
{
	Device::LockApi();
	XUSG_IF_VK(return make_shared<DescriptorTable_VK>();)

	return make_shared<DescriptorTable_DX12>();
}

DescriptorTableLib::uptr DescriptorTableLib::MakeUnique()
{
	Device::LockApi();
	XUSG_IF_VK(return make_unique<DescriptorTableLib_VK>();)

	return make_unique<DescriptorTableLib_DX12>();
}

DescriptorTableLib::sptr DescriptorTableLib::MakeShared()
{
	Device::LockApi();
	XUSG_IF_VK(return make_shared<DescriptorTableLib_VK>();)

	return make_shared<DescriptorTableLib_DX12>();
}

DescriptorTableLib::uptr DescriptorTableLib::MakeUnique(Device* pDevice, const wchar_t* name)
{
	Device::LockApi();
	XUSG_IF_VK(return make_unique<DescriptorTableLib_VK>(pDevice, name);)

	return make_unique<DescriptorTableLib_DX12>(pDevice, name);
}

DescriptorTableLib::sptr DescriptorTableLib::MakeShared(Device* pDevice, const wchar_t* name)
{
	Device::LockApi();
	XUSG_IF_VK(return make_shared<DescriptorTableLib_VK>(pDevice, name);)

	return make_shared<DescriptorTableLib_DX12>(pDevice, name);
}

uint32_t Reflector::ReflectBinding(const char* name, uint32_t defaultVal, uint32_t expectedSpace) const
{
	assert(IsValid());
	const auto binding = GetResourceBindingPointByName(name);
	if (binding == UINT32_MAX) return defaultVal;

	assert(GetResourceSpaceByName(name) == expectedSpace);

	return binding;
}

Reflector::uptr Reflector::MakeUnique()
{
	Device::LockApi();
	XUSG_IF_VK(return make_unique<Reflector_VK>();)

	return make_unique<Reflector_DX12>();
}

Reflector::sptr Reflector::MakeShared()
{
	Device::LockApi();
	XUSG_IF_VK(return make_shared<Reflector_VK>();)

	return make_shared<Reflector_DX12>();
}

bool ShaderLib::CreateShader(Shader::Stage stage, uint32_t index,
	const void* pDxbc, size_t dxbcSize, const void* pSpirv, size_t spirvSize)
{
	const auto vulkan = Device::GetApi() == API::VULKAN;

	return CreateShader(stage, index, vulkan ? pSpirv : pDxbc, vulkan ? spirvSize : dxbcSize) != nullptr;
}

ShaderLib::uptr ShaderLib::MakeUnique()
{
	Device::LockApi();
	XUSG_IF_VK(return make_unique<ShaderLib_VK>();)

	return make_unique<ShaderLib_DX12>();
}

ShaderLib::sptr ShaderLib::MakeShared()
{
	Device::LockApi();
	XUSG_IF_VK(return make_shared<ShaderLib_VK>();)

	return make_shared<ShaderLib_DX12>();
}

Util::PipelineLayout::uptr Util::PipelineLayout::MakeUnique()
{
	Device::LockApi();
	XUSG_IF_VK(return make_unique<PipelineLayout_VK>();)

	return make_unique<PipelineLayout_DX12>();
}

Util::PipelineLayout::sptr Util::PipelineLayout::MakeShared()
{
	Device::LockApi();
	XUSG_IF_VK(return make_shared<PipelineLayout_VK>();)

	return make_shared<PipelineLayout_DX12>();
}

Util::PipelineLayout::uptr Util::PipelineLayout::CloneUnique(const PipelineLayout* pSrc)
{
#ifndef XUSG_NO_VULKAN
	if (Device::GetApi() == API::VULKAN)
	{
		auto dst = make_unique<PipelineLayout_VK>();
		const auto p = AsVK(pSrc);
		assert(p);
		*dst = *p;

		return dst;
	}
#endif

	auto dst = make_unique<PipelineLayout_DX12>();
	const auto p = AsDX12(pSrc);
	assert(p);
	*dst = *p;

	return dst;
}

Util::PipelineLayout::sptr Util::PipelineLayout::CloneShared(const PipelineLayout* pSrc)
{
#ifndef XUSG_NO_VULKAN
	if (Device::GetApi() == API::VULKAN)
	{
		auto dst = make_shared<PipelineLayout_VK>();
		const auto p = AsVK(pSrc);
		assert(p);
		*dst = *p;

		return dst;
	}
#endif

	auto dst = make_shared<PipelineLayout_DX12>();
	const auto p = AsDX12(pSrc);
	assert(p);
	*dst = *p;

	return dst;
}

PipelineLayoutLib::uptr PipelineLayoutLib::MakeUnique()
{
	Device::LockApi();
	XUSG_IF_VK(return make_unique<PipelineLayoutLib_VK>();)

	return make_unique<PipelineLayoutLib_DX12>();
}

PipelineLayoutLib::sptr PipelineLayoutLib::MakeShared()
{
	Device::LockApi();
	XUSG_IF_VK(return make_shared<PipelineLayoutLib_VK>();)

	return make_shared<PipelineLayoutLib_DX12>();
}

PipelineLayoutLib::uptr PipelineLayoutLib::MakeUnique(Device* pDevice)
{
	Device::LockApi();
	XUSG_IF_VK(return make_unique<PipelineLayoutLib_VK>(pDevice);)

	return make_unique<PipelineLayoutLib_DX12>(pDevice);
}

PipelineLayoutLib::sptr PipelineLayoutLib::MakeShared(Device* pDevice)
{
	Device::LockApi();
	XUSG_IF_VK(return make_shared<PipelineLayoutLib_VK>(pDevice);)

	return make_shared<PipelineLayoutLib_DX12>(pDevice);
}

Graphics::State::uptr Graphics::State::MakeUnique()
{
	Device::LockApi();
	XUSG_IF_VK(return make_unique<State_VK>();)

	return make_unique<State_DX12>();
}

Graphics::State::sptr Graphics::State::MakeShared()
{
	Device::LockApi();
	XUSG_IF_VK(return make_shared<State_VK>();)

	return make_shared<State_DX12>();
}

Graphics::PipelineLib::uptr Graphics::PipelineLib::MakeUnique()
{
	Device::LockApi();
	XUSG_IF_VK(return make_unique<PipelineLib_VK>();)

	return make_unique<PipelineLib_DX12>();
}

Graphics::PipelineLib::sptr Graphics::PipelineLib::MakeShared()
{
	Device::LockApi();
	XUSG_IF_VK(return make_shared<PipelineLib_VK>();)

	return make_shared<PipelineLib_DX12>();
}

Graphics::PipelineLib::uptr Graphics::PipelineLib::MakeUnique(Device* pDevice)
{
	Device::LockApi();
	XUSG_IF_VK(return make_unique<PipelineLib_VK>(pDevice);)

	return make_unique<PipelineLib_DX12>(pDevice);
}

Graphics::PipelineLib::sptr Graphics::PipelineLib::MakeShared(Device* pDevice)
{
	Device::LockApi();
	XUSG_IF_VK(return make_shared<PipelineLib_VK>(pDevice);)

	return make_shared<PipelineLib_DX12>(pDevice);
}

Compute::State::uptr Compute::State::MakeUnique()
{
	Device::LockApi();
	XUSG_IF_VK(return make_unique<State_VK>();)

	return make_unique<State_DX12>();
}

Compute::State::sptr Compute::State::MakeShared()
{
	Device::LockApi();
	XUSG_IF_VK(return make_shared<State_VK>();)

	return make_shared<State_DX12>();
}

Compute::PipelineLib::uptr Compute::PipelineLib::MakeUnique()
{
	Device::LockApi();
	XUSG_IF_VK(return make_unique<PipelineLib_VK>();)

	return make_unique<PipelineLib_DX12>();
}

Compute::PipelineLib::sptr Compute::PipelineLib::MakeShared()
{
	Device::LockApi();
	XUSG_IF_VK(return make_shared<PipelineLib_VK>();)

	return make_shared<PipelineLib_DX12>();
}

Compute::PipelineLib::uptr Compute::PipelineLib::MakeUnique(Device* pDevice)
{
	Device::LockApi();
	XUSG_IF_VK(return make_unique<PipelineLib_VK>(pDevice);)

	return make_unique<PipelineLib_DX12>(pDevice);
}

Compute::PipelineLib::sptr Compute::PipelineLib::MakeShared(Device* pDevice)
{
	Device::LockApi();
	XUSG_IF_VK(return make_shared<PipelineLib_VK>(pDevice);)

	return make_shared<PipelineLib_DX12>(pDevice);
}

void XUSG::GetPipelineCacheData(Pipeline pipeline, const void** ppData, size_t* pSize)
{
	const auto blob = GetPipelineCache(pipeline);

	return GetBlobData(blob, ppData, pSize);
}

void XUSG::GetBlobData(const Blob& blob, const void** ppData, size_t* pSize)
{
	XUSG_IF_VK(return GetVKBlobData(blob, ppData, pSize);)

	return GetDX12BlobData(blob, ppData, pSize);
}

Blob XUSG::GetPipelineCache(Pipeline pipeline)
{
	XUSG_IF_VK(return GetVKPipelineCache(pipeline);)

	return GetDX12PipelineCache(pipeline);
}

size_t XUSG::Align(size_t size, size_t alignment)
{
	return (size + (alignment - 1)) & ~(alignment - 1);
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

uint32_t XUSG::DivideAndRoundUp(uint32_t x, uint32_t n)
{
	return XUSG_DIV_UP(x, n);
}
