//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSG_DX12.h"
#include "XUSGCommand_DX12.h"
#include "XUSGResource_DX12.h"
#include "XUSGEnum_DX12.h"

using namespace std;
using namespace XUSG;

Device_DX12::Device_DX12(void* handle)
{
	m_device = static_cast<ID3D12Device*>(handle);
}

Device_DX12::~Device_DX12()
{
}

bool Device_DX12::GetCommandQueue(CommandQueue& commandQueue, CommandListType type,
	CommandQueueFlag flags, int32_t priority, uint32_t nodeMask, const wchar_t* name)
{
	return commandQueue.Create(*this, type, flags, priority, nodeMask, name);
}

bool Device_DX12::GetCommandAllocator(CommandAllocator& commandAllocator,
	CommandListType type, const wchar_t* name)
{
	return commandAllocator.Create(*this, type, name);
}

bool Device_DX12::GetCommandList(CommandList& commandList, uint32_t nodeMask, CommandListType type,
	const CommandAllocator& commandAllocator, const Pipeline& pipeline, const wchar_t* name)
{
	return commandList.Create(*this, nodeMask, type, commandAllocator, pipeline, name);
}

bool Device_DX12::GetFence(Fence& fence, uint64_t initialValue, FenceFlag flags, const wchar_t* name)
{
	return fence.Create(*this, initialValue, flags, name);
}

uint32_t Device_DX12::GetDeviceRemovedReason() const
{
	return m_device->GetDeviceRemovedReason();
}

void* Device_DX12::GetHandle() const
{
	return m_device.get();
}

//--------------------------------------------------------------------------------------

Fence_DX12::Fence_DX12()
{
}

Fence_DX12::~Fence_DX12()
{
}

bool Fence_DX12::Create(const Device& device, uint64_t initialValue, FenceFlag flags, const wchar_t* name)
{
	const auto pDevice = static_cast<ID3D12Device*>(device.GetHandle());

	assert(pDevice);
	V_RETURN(pDevice->CreateFence(initialValue, GetDX12FenceFlags(flags), IID_PPV_ARGS(&m_fence)), cerr, false);
	if (name) m_fence->SetName(name);

	return true;
}

bool Fence_DX12::SetEventOnCompletion(uint64_t value, void* hEvent)
{
	V_RETURN(m_fence->SetEventOnCompletion(value, hEvent), cerr, false);

	return true;
}

bool Fence_DX12::Signal(uint64_t value)
{
	V_RETURN(m_fence->Signal(value), cerr, false);

	return true;
}

uint64_t Fence_DX12::GetCompletedValue() const
{
	return m_fence->GetCompletedValue();
}

void* Fence_DX12::GetHandle() const
{
	return m_fence.get();
}

//--------------------------------------------------------------------------------------

CommandLayout_DX12::CommandLayout_DX12()
{
}

CommandLayout_DX12::~CommandLayout_DX12()
{
}

bool CommandLayout_DX12::Create(const Device& device, uint32_t byteStride, uint32_t numArguments,
	const IndirectArgument* pArguments, uint32_t nodeMask, const wchar_t* name)
{
	D3D12_COMMAND_SIGNATURE_DESC desc;
	desc.ByteStride = byteStride;
	desc.NumArgumentDescs = numArguments;
	desc.pArgumentDescs = reinterpret_cast<const D3D12_INDIRECT_ARGUMENT_DESC*>(pArguments);
	desc.NodeMask = nodeMask;

	const auto pDevice = static_cast<ID3D12Device*>(device.GetHandle());

	assert(pDevice);
	V_RETURN(pDevice->CreateCommandSignature(&desc, nullptr, IID_PPV_ARGS(&m_commandSignature)), cerr, false);
	if (name) m_commandSignature->SetName(name);

	return true;
}

void* CommandLayout_DX12::GetHandle() const
{
	return m_commandSignature.get();
}

//--------------------------------------------------------------------------------------

SwapChain_DX12::SwapChain_DX12()
{
}

SwapChain_DX12::~SwapChain_DX12()
{
}

bool SwapChain_DX12::Create(void* factory, void* hWnd, const CommandQueue& commandQueue,
	uint8_t bufferCount, uint32_t width, uint32_t height, Format format, uint8_t sampleCount)
{
	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = bufferCount;
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = GetDXGIFormat(format);
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = sampleCount;

	com_ptr<IDXGISwapChain1> swapChain = nullptr;
	V_RETURN(static_cast<IDXGIFactory4*>(factory)->CreateSwapChainForHwnd(
		static_cast<ID3D12CommandQueue*>(commandQueue.GetHandle()),	// Swap chain needs the queue so that it can force a flush on it.
		static_cast<HWND>(hWnd),
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain
	), cerr, false);

	// Store the swap chain.
	V_RETURN(swapChain->QueryInterface(IID_PPV_ARGS(&m_swapChain)), cerr, false);

	return true;
}

bool SwapChain_DX12::Present(uint8_t syncInterval, uint32_t flags)
{
	V_RETURN(m_swapChain->Present(syncInterval, flags), cerr, false);

	return true;
}

bool SwapChain_DX12::GetBuffer(uint8_t buffer, Resource& resource) const
{
	V_RETURN(m_swapChain->GetBuffer(buffer, IID_PPV_ARGS(&dynamic_cast<Resource_DX12&>(resource).GetResource())), cerr, false);

	return true;
}

uint32_t SwapChain_DX12::ResizeBuffers(uint8_t bufferCount, uint32_t width,
	uint32_t height, Format format, uint8_t flag)
{
	return m_swapChain->ResizeBuffers(bufferCount, width, height, GetDXGIFormat(format), flag);
}

void* SwapChain_DX12::GetHandle() const
{
	return m_swapChain.get();
}

uint8_t SwapChain_DX12::GetCurrentBackBufferIndex() const
{
	return m_swapChain->GetCurrentBackBufferIndex();
}
