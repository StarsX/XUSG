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
	std::string HrToString(uint32_t hr);
	std::wstring GetDebugObjectName(ID3D12Object* pObject);

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
			uint8_t numAllocators, CommandAllocator* const* ppAllocators, const Pipeline& pipeline,
			const wchar_t* name = nullptr);
		bool GetFence(Fence* pFence, uint64_t initialValue, FenceFlag flags,
			const wchar_t* name = nullptr);
		bool MakeResident(uint32_t numObjects, void* const* ppObjects);
		bool Evict(uint32_t numObjects, void* const* ppObjects);

		uint32_t Create(void* pAdapter, uint32_t minFeatureLevel, const wchar_t* name = nullptr);
		uint32_t GetDeviceRemovedReason() const;

		void Create(void* pNativeHandle);

		void* GetNativeHandle() const;
		void* GetAdapterNativeHandle() const;
		void* GetBackend();

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
		void* GetBackend();

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

		uint8_t GetCurrentBackBufferIndex();

		void Create(uint64_t nativeHandle);

		uint64_t GetNativeHandle() const;
		void* GetBackend();

	protected:
		com_ptr<IDXGISwapChain3> m_swapChain;

		bool m_allowTearing;

		std::vector<RECT> m_dirtyRects;
	};

	Blob GetDX12PipelineCache(Pipeline pipeline);

	void GetDX12BlobData(const Blob& blob, const void** ppData, size_t* pSize);

	Device_DX12* AsDX12(Device* p);
	CommandLayout_DX12* AsDX12(CommandLayout* p);
	SwapChain_DX12* AsDX12(SwapChain* p);
}
