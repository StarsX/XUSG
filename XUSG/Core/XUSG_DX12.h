//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSG.h"

#pragma once

#define H_RETURN(x, o, m, r)	{ const auto hr = x; if (FAILED(hr)) { o << m << std::endl; assert(!m); return r; } }
#define V_RETURN(x, o, r)		H_RETURN(x, o, HrToString(hr).c_str(), r)
#define F_RETURN(x, o, h, r)	XUSG_M_RETURN(x, o, HrToString(h).c_str(), r)

namespace XUSG
{
	inline std::string HrToString(uint32_t hr)
	{
		char s_str[64] = {};
		sprintf_s(s_str, "HRESULT of 0x%08X", hr);

		return std::string(s_str);
	}

	inline std::wstring GetDebugObjectName(ID3D12Object* pObject)
	{
		if (!pObject) return {};

		UINT size = 0;
		const auto hr = pObject->GetPrivateData(WKPDID_D3DDebugObjectNameW, &size, nullptr);
		if ((FAILED(hr) && hr != DXGI_ERROR_MORE_DATA) || !size) return {};

		std::wstring name((size + sizeof(wchar_t) - 1) / sizeof(wchar_t), L'\0');
		if (FAILED(pObject->GetPrivateData(WKPDID_D3DDebugObjectNameW, &size, name.data()))) return {};

		name.resize(size / sizeof(wchar_t));
		while (!name.empty() && name.back() == L'\0') name.pop_back();

		return name;
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
			CommandAllocator* pCommandAllocator, const Pipeline& pipeline,
			const wchar_t* name = nullptr);
		bool GetFence(Fence* pFence, uint64_t initialValue, FenceFlag flags,
			const wchar_t* name = nullptr);
		bool MakeResident(uint32_t numObjects, void* const* ppObjects);
		bool Evict(uint32_t numObjects, void* const* ppObjects);

		uint32_t Create(void* pAdapter, uint32_t minFeatureLevel, const wchar_t* name = nullptr);
		uint32_t GetDeviceRemovedReason() const;

		void Create(void* pNativeHandle);

		void* GetNativeHandle();
		void* GetAdapterNativeHandle();
		void* GetBackend() override;

	protected:
		void queryAdapter();

		com_ptr<ID3D12Device> m_device;
		com_ptr<IDXGIAdapter> m_adapter;
	};

	class Fence_DX12 :
		public virtual Fence
	{
	public:
		Fence_DX12();
		virtual ~Fence_DX12();

		bool Create(Device* pDevice, uint64_t initialValue,
			FenceFlag flags, const wchar_t* name = nullptr);
		bool SetEventOnCompletion(uint64_t value, void* hEvent);
		bool Signal(uint64_t value);

		uint64_t GetCompletedValue() const;

		void Create(uint64_t nativeHandle, Device* pDevice);

		uint64_t GetNativeHandle() const;

	protected:
		com_ptr<ID3D12Fence> m_fence;
	};

	class CommandLayout_DX12 :
		public virtual CommandLayout
	{
	public:
		CommandLayout_DX12();
		virtual ~CommandLayout_DX12();

		bool Create(Device* pDevice, uint32_t byteStride, uint32_t numArguments,
			const IndirectArgument* pArguments, const PipelineLayout& pipelineLayout = nullptr,
			uint32_t nodeMask = 0, const wchar_t* name = nullptr);

		void Create(uint64_t nativeHandle, Device* pDevice);

		uint64_t GetNativeHandle() const;
		void* GetBackend() override;

	protected:
		com_ptr<ID3D12CommandSignature> m_commandSignature;
	};

	class SwapChain_DX12 :
		public virtual SwapChain
	{
	public:
		SwapChain_DX12();
		virtual ~SwapChain_DX12();

		bool Create(void* pFactory, void* hWnd, void* pDevice, uint8_t bufferCount,
			uint32_t width, uint32_t height, Format format, SwapChainFlag flags = SwapChainFlag::NONE,
			bool windowed = true);
		bool Present(uint8_t syncInterval = 0, PresentFlag flags = PresentFlag::NONE);
		bool PresentEx(uint8_t syncInterval, PresentFlag flags, uint32_t dirtyRectsCount,
			const RectRange* pDirtyRects, const RectRange* pScrollRect = nullptr,
			const Point* pScrollOffset = nullptr);
		bool GetBuffer(uint8_t buffer, Resource* pResource) const;

		uint32_t ResizeBuffers(uint8_t bufferCount, uint32_t width, uint32_t height,
			Format format, SwapChainFlag flags = SwapChainFlag::NONE);

		uint8_t GetCurrentBackBufferIndex() const;

		void Create(uint64_t nativeHandle);

		uint64_t GetNativeHandle() const;
		void* GetBackend() override;

	protected:
		com_ptr<IDXGISwapChain3> m_swapChain;

		bool m_allowTearing;

		std::vector<RECT> m_dirtyRects;
	};

	Blob GetDX12PipelineCache(Pipeline pipeline);

	void GetDX12BlobData(const Blob& blob, const void** ppData, size_t* pSize);

	inline Device_DX12* AsDX12(Device* p)
	{
		assert(!p || p->GetBackend());
		return static_cast<Device_DX12*>(p ? p->GetBackend() : nullptr);
	}

	inline const Device_DX12* AsDX12(const Device* p)
	{
		assert(!p || const_cast<Device*>(p)->GetBackend());
		return static_cast<const Device_DX12*>(p ? const_cast<Device*>(p)->GetBackend() : nullptr);
	}

	inline CommandLayout_DX12* AsDX12(CommandLayout* p)
	{
		assert(!p || p->GetBackend());
		return static_cast<CommandLayout_DX12*>(p ? p->GetBackend() : nullptr);
	}

	inline const CommandLayout_DX12* AsDX12(const CommandLayout* p)
	{
		assert(!p || const_cast<CommandLayout*>(p)->GetBackend());
		return static_cast<const CommandLayout_DX12*>(p ? const_cast<CommandLayout*>(p)->GetBackend() : nullptr);
	}

	inline SwapChain_DX12* AsDX12(SwapChain* p)
	{
		assert(!p || p->GetBackend());
		return static_cast<SwapChain_DX12*>(p ? p->GetBackend() : nullptr);
	}

	inline const SwapChain_DX12* AsDX12(const SwapChain* p)
	{
		assert(!p || const_cast<SwapChain*>(p)->GetBackend());
		return static_cast<const SwapChain_DX12*>(p ? const_cast<SwapChain*>(p)->GetBackend() : nullptr);
	}
}
