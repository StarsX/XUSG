//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSG.h"

#pragma once

#define H_RETURN(x, o, m, r)	{ const auto hr = x; if (FAILED(hr)) { o << m << std::endl; assert(!m); return r; } }
#define V_RETURN(x, o, r)		H_RETURN(x, o, HrToString(hr).c_str(), r)
#define F_RETURN(x, o, h, r)	M_RETURN(x, o, HrToString(h).c_str(), r)

namespace XUSG
{
	inline std::string HrToString(uint32_t hr)
	{
		char s_str[64] = {};
		sprintf_s(s_str, "HRESULT of 0x%08X", hr);

		return std::string(s_str);
	}

	// Device
	class Device_DX12 :
		public virtual Device
	{
	public:
		Device_DX12();
		virtual ~Device_DX12();

		bool GetCommandQueue(CommandQueue* pCommandQueue, CommandListType type,
			CommandQueueFlag flags, int32_t priority = 0, uint32_t nodeMask = 0,
			const wchar_t* name = nullptr);
		bool GetCommandAllocator(CommandAllocator* pCommandAllocator,
			CommandListType type, const wchar_t* name = nullptr);
		bool GetCommandList(CommandList* pCommandList, uint32_t nodeMask, CommandListType type,
			const CommandAllocator* pCommandAllocator, const Pipeline& pipeline,
			const wchar_t* name = nullptr);
		bool GetFence(Fence* pFence, uint64_t initialValue, FenceFlag flags,
			const wchar_t* name = nullptr);

		uint32_t Create(void* pAdapter, uint32_t featureLevel);
		uint32_t GetDeviceRemovedReason() const;

		void* GetHandle() const;

	protected:
		com_ptr<ID3D12Device> m_device;
	};

	class Fence_DX12 :
		public virtual Fence
	{
	public:
		Fence_DX12();
		virtual ~Fence_DX12();

		bool Create(const Device* pDevice, uint64_t initialValue,
			FenceFlag flags, const wchar_t* name = nullptr);
		bool SetEventOnCompletion(uint64_t value, void* hEvent);
		bool Signal(uint64_t value);

		virtual uint64_t GetCompletedValue() const;

		void* GetHandle() const;

	protected:
		com_ptr<ID3D12Fence> m_fence;
	};

	class CommandLayout_DX12 :
		public virtual CommandLayout
	{
	public:
		CommandLayout_DX12();
		virtual ~CommandLayout_DX12();

		bool Create(const Device* pDevice, uint32_t byteStride, uint32_t numArguments,
			const IndirectArgument* pArguments, uint32_t nodeMask = 0, const wchar_t* name = nullptr);

		void* GetHandle() const;

	protected:
		com_ptr<ID3D12CommandSignature> m_commandSignature;
	};

	class SwapChain_DX12 :
		public virtual SwapChain
	{
	public:
		SwapChain_DX12();
		virtual ~SwapChain_DX12();

		bool Create(void* pFactory, void* hWnd, const CommandQueue* pCommandQueue, uint8_t bufferCount,
			uint32_t width, uint32_t height, Format format, uint8_t sampleCount = 1);
		bool Present(uint8_t syncInterval = 0, uint32_t flags = 0);
		bool GetBuffer(uint8_t buffer, Resource* pResource) const;

		uint32_t ResizeBuffers(uint8_t bufferCount, uint32_t width,
			uint32_t height, Format format, uint8_t flag = 0);

		uint8_t GetCurrentBackBufferIndex() const;

		void* GetHandle() const;

	protected:
		com_ptr<IDXGISwapChain3> m_swapChain;
	};
}
