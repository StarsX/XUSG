//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSG_DX12.h"
#include "XUSGCommand_DX12.h"
#include "XUSGResource_DX12.h"
#include "XUSGEnum_DX12.h"

using namespace std;
using namespace XUSG;

Device_DX12::Device_DX12()
{
}

Device_DX12::~Device_DX12()
{
}

bool Device_DX12::GetCommandQueue(CommandQueue* pCommandQueue, CommandListType type,
	CommandQueueFlag flags, int32_t priority, uint32_t nodeMask, const wchar_t* name)
{
	return pCommandQueue->Create(this, type, flags, priority, nodeMask, name);
}

bool Device_DX12::GetCommandAllocator(CommandAllocator* pCommandAllocator,
	CommandListType type, const wchar_t* name)
{
	return pCommandAllocator->Create(this, type, name);
}

bool Device_DX12::GetCommandList(CommandList* pCommandList, uint32_t nodeMask, CommandListType type,
	const CommandAllocator* pCommandAllocator, const Pipeline& pipeline, const wchar_t* name)
{
	return pCommandList->Create(this, nodeMask, type, pCommandAllocator, pipeline, name);
}

bool Device_DX12::GetFence(Fence* pFence, uint64_t initialValue, FenceFlag flags, const wchar_t* name)
{
	return pFence->Create(this, initialValue, flags, name);
}

uint32_t Device_DX12::Create(void* pAdapter, uint32_t minFeatureLevel, const wchar_t* name)
{
	const auto hr = D3D12CreateDevice(static_cast<IUnknown*>(pAdapter),
		static_cast<D3D_FEATURE_LEVEL>(minFeatureLevel), IID_PPV_ARGS(&m_device));
	if (name) m_device->SetName(name);

	return hr;
}

uint32_t Device_DX12::GetDeviceRemovedReason() const
{
	return m_device->GetDeviceRemovedReason();
}

void Device_DX12::Create(void* pHandle, const wchar_t* name)
{
	m_device = pHandle;
	if (name) m_device->SetName(name);
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

bool Fence_DX12::Create(const Device* pDevice, uint64_t initialValue, FenceFlag flags, const wchar_t* name)
{
	const auto pDxDevice = static_cast<ID3D12Device*>(pDevice->GetHandle());

	assert(pDxDevice);
	V_RETURN(pDxDevice->CreateFence(initialValue, GetDX12FenceFlags(flags), IID_PPV_ARGS(&m_fence)), cerr, false);
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

void Fence_DX12::Create(void* pHandle, const wchar_t* name)
{
	m_fence = pHandle;
	if (name) m_fence->SetName(name);
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

bool CommandLayout_DX12::Create(const Device* pDevice, uint32_t byteStride, uint32_t numArguments,
	const IndirectArgument* pArguments, const PipelineLayout& pipelineLayout, uint32_t nodeMask,
	const wchar_t* name)
{
	vector<D3D12_INDIRECT_ARGUMENT_DESC> argDescs(numArguments);
	for (auto i = 0u; i < numArguments; ++i)
	{
		const auto& argument = pArguments[i];
		auto& argDesc = argDescs[i];
		argDesc.Type = GetDX12IndirectArgumentType(argument.Type);

		switch (argument.Type)
		{
		case IndirectArgumentType::VERTEX_BUFFER_VIEW:
			argDesc.VertexBuffer.Slot = argument.VertexBuffer.Slot;
			break;
		case IndirectArgumentType::CONSTANT:
			argDesc.Constant.RootParameterIndex = argument.Constant.Index;
			argDesc.Constant.DestOffsetIn32BitValues = argument.Constant.DestOffsetIn32BitValues;
			argDesc.Constant.Num32BitValuesToSet = argument.Constant.Num32BitValuesToSet;
			break;
		case IndirectArgumentType::CONSTANT_BUFFER_VIEW:
			argDesc.ConstantBufferView.RootParameterIndex = argument.CbvSrvUav.Index;
			break;
		case IndirectArgumentType::SHADER_RESOURCE_VIEW:
			argDesc.ShaderResourceView.RootParameterIndex = argument.CbvSrvUav.Index;
			break;
		case IndirectArgumentType::UNORDERED_ACCESS_VIEW:
			argDesc.UnorderedAccessView.RootParameterIndex = argument.CbvSrvUav.Index;
			break;
		}
	}

	D3D12_COMMAND_SIGNATURE_DESC desc;
	desc.ByteStride = byteStride;
	desc.NumArgumentDescs = numArguments;
	desc.pArgumentDescs = argDescs.data();
	desc.NodeMask = nodeMask;

	const auto pDxDevice = static_cast<ID3D12Device*>(pDevice->GetHandle());

	assert(pDxDevice);
	V_RETURN(pDxDevice->CreateCommandSignature(&desc, static_cast<ID3D12RootSignature*>(pipelineLayout),
		IID_PPV_ARGS(&m_commandSignature)), cerr, false);
	if (name) m_commandSignature->SetName(name);

	return true;
}

void CommandLayout_DX12::Create(void* pHandle, const wchar_t* name)
{
	m_commandSignature = pHandle;
	if (name) m_commandSignature->SetName(name);
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

bool SwapChain_DX12::Create(void* pFactory, void* hWnd, void* pDevice, uint8_t bufferCount,
	uint32_t width, uint32_t height, Format format, SwapChainFlag flags, bool windowed)
{
	const auto pDXGIFactory = static_cast<IDXGIFactory5*>(pFactory);

	BOOL allowTearing = FALSE;
	const auto hr = pDXGIFactory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING,
		&allowTearing, sizeof(allowTearing));
	m_allowTearing = SUCCEEDED(hr) && allowTearing;
	if (!m_allowTearing) flags &= ~SwapChainFlag::ALLOW_TEARING;

	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = bufferCount;
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = GetDXGIFormat(format);
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.Flags = GetDXGISwapChainFlags(flags);

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc = {};
	fullscreenDesc.Windowed = windowed;

	com_ptr<IDXGISwapChain1> swapChain = nullptr;
	V_RETURN(pDXGIFactory->CreateSwapChainForHwnd(
		static_cast<IUnknown*>(pDevice),	// Swap chain needs the queue so that it can force a flush on it.
		static_cast<HWND>(hWnd),
		&swapChainDesc,
		windowed ? nullptr : &fullscreenDesc,
		nullptr,
		swapChain.put()),
		cerr, false);

	// Store the swap chain.
	V_RETURN(swapChain->QueryInterface(IID_PPV_ARGS(&m_swapChain)), cerr, false);

	return true;
}

bool SwapChain_DX12::Present(uint8_t syncInterval, PresentFlag flags)
{
	if (!m_allowTearing) flags &= ~PresentFlag::ALLOW_TEARING;
	V_RETURN(m_swapChain->Present(syncInterval, GetDXGIPresentFlags(flags)), cerr, false);

	return true;
}

bool SwapChain_DX12::PresentEx(uint8_t syncInterval, PresentFlag flags, uint32_t dirtyRectsCount,
	const RectRange* pDirtyRects, const RectRange* pScrollRect, const Point* pScrollOffset)
{
	m_dirtyRects.resize(dirtyRectsCount);
	for (auto i = 0u; i < dirtyRectsCount; ++i)
	{
		m_dirtyRects[i].left = pDirtyRects[i].Left;
		m_dirtyRects[i].top = pDirtyRects[i].Top;
		m_dirtyRects[i].right = pDirtyRects[i].Right;
		m_dirtyRects[i].bottom = pDirtyRects[i].Bottom;
	}

	RECT scrollRect = {};
	if (pScrollRect)
	{
		scrollRect.left = pScrollRect->Left;
		scrollRect.top = pScrollRect->Top;
		scrollRect.right = pScrollRect->Right;
		scrollRect.bottom = pScrollRect->Bottom;
	}

	POINT scrollOffset = {};
	if (pScrollOffset)
	{
		scrollOffset.x = pScrollOffset->X;
		scrollOffset.y = pScrollOffset->Y;
	}

	DXGI_PRESENT_PARAMETERS params = {};
	params.DirtyRectsCount = dirtyRectsCount;
	params.pDirtyRects = pDirtyRects ? m_dirtyRects.data() : nullptr;
	params.pScrollRect = pScrollRect ? &scrollRect : nullptr;
	params.pScrollOffset = pScrollOffset ? &scrollOffset : nullptr;

	if (!m_allowTearing) flags &= ~PresentFlag::ALLOW_TEARING;
	V_RETURN(m_swapChain->Present1(syncInterval, GetDXGIPresentFlags(flags), &params), cerr, false);

	return true;
}

bool SwapChain_DX12::GetBuffer(uint8_t buffer, Resource* pResource) const
{
	V_RETURN(m_swapChain->GetBuffer(buffer, IID_PPV_ARGS(&dynamic_cast<Resource_DX12*>(pResource)->GetResource())), cerr, false);

	return true;
}

uint32_t SwapChain_DX12::ResizeBuffers(uint8_t bufferCount, uint32_t width,
	uint32_t height, Format format, SwapChainFlag flags)
{
	if (!m_allowTearing) flags &= ~SwapChainFlag::ALLOW_TEARING;
	return m_swapChain->ResizeBuffers(bufferCount, width, height,
		GetDXGIFormat(format), GetDXGISwapChainFlags(flags));
}

uint8_t SwapChain_DX12::GetCurrentBackBufferIndex() const
{
	return m_swapChain->GetCurrentBackBufferIndex();
}

void SwapChain_DX12::Create(void* pHandle)
{
	m_swapChain = pHandle;
}

void* SwapChain_DX12::GetHandle() const
{
	return m_swapChain.get();
}
