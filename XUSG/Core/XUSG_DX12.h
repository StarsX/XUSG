//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#define H_RETURN(x, o, m, r)		{ const auto hr = x; if (FAILED(hr)) { o << m << std::endl; assert(!m); return r; } }
#define V_RETURN(x, o, r)			H_RETURN(x, o, XUSG::HrToString(hr).c_str(), r)

#define M_RETURN(x, o, m, r)		if (x) { o << m << std::endl; assert(!m); return r; }
#define F_RETURN(x, o, h, r)		M_RETURN(x, o, XUSG::HrToString(h).c_str(), r)

#define BARRIER_ALL_SUBRESOURCES	D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES

namespace XUSG
{
#if _HAS_CXX17
	template <typename T>
	class com_ptr :
		public winrt::com_ptr<T>
	{
	public:
		using element_type = T;
		using winrt::com_ptr<T>::com_ptr;

		element_type** operator&() noexcept { return this->put(); }
	};
#else
	template <typename T>
	class com_ptr :
		public Microsoft::WRL::ComPtr<T>
	{
	public:
		using element_type = T;
		using Microsoft::WRL::ComPtr<T>::ComPtr;

		element_type* get() const throw() { return this->Get(); }
	};
#endif

	__forceinline uint8_t Log2(uint32_t value)
	{
		unsigned long mssb; // most significant set bit

		if (BitScanReverse(&mssb, value) > 0)
			return static_cast<uint8_t>(mssb);
		else return 0;
	}

	inline std::string HrToString(HRESULT hr)
	{
		char s_str[64] = {};
		sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<uint32_t>(hr));

		return std::string(s_str);
	}

	// Swapchain and blobs
	using Blob = com_ptr<ID3DBlob>;
	using SwapChain = com_ptr<IDXGISwapChain3>;

	// Command lists related
	using CommandAllocator = com_ptr<ID3D12CommandAllocator>;
	using Fence = com_ptr<ID3D12Fence>;

	MIDL_INTERFACE("0ec870a6-5d7e-4c22-8cfc-5baae07616ed")
		DLL_INTERFACE DX12CommandQueue : public ID3D12CommandQueue
	{
		void SubmitCommandLists(uint32_t numCommandLists, CommandList* const* ppCommandLists);
		void SubmitCommandList(CommandList* const pCommandList);
	};
	using CommandQueue = com_ptr<DX12CommandQueue>;

	// Resources related
	using Resource = com_ptr<ID3D12Resource>;
	using VertexBufferView = D3D12_VERTEX_BUFFER_VIEW;
	using IndexBufferView = D3D12_INDEX_BUFFER_VIEW;
	using StreamOutBufferView = D3D12_STREAM_OUTPUT_BUFFER_VIEW;
	using Sampler = std::shared_ptr<D3D12_SAMPLER_DESC>;
	using ResourceBarrier = CD3DX12_RESOURCE_BARRIER;
	using TileCopyFlags = D3D12_TILE_COPY_FLAGS;

	// Descriptors related
	using DescriptorPool = com_ptr<ID3D12DescriptorHeap>;

	// Pipeline layouts related
	using PipelineLayout = com_ptr<ID3D12RootSignature>;
	using DescriptorRangeList = std::vector<CD3DX12_DESCRIPTOR_RANGE1>;

	// Pipeline layouts related
	struct RootParameter : public CD3DX12_ROOT_PARAMETER1
	{
		DescriptorRangeList ranges;
	};
	using DescriptorTableLayout = std::shared_ptr<RootParameter>;

	using Pipeline = com_ptr<ID3D12PipelineState>;
	using CommandLayout = com_ptr<ID3D12CommandSignature>;

	// Device
	MIDL_INTERFACE("189819f1-1db6-4b57-be54-1821339b85f7")
		DLL_INTERFACE DX12Device : public ID3D12Device
	{
		bool GetCommandQueue(CommandQueue& commandQueue, CommandListType type, CommandQueueFlag flags, int32_t priority = 0, uint32_t nodeMask = 0);
		bool GetCommandAllocator(CommandAllocator& commandAllocator, CommandListType type);
		bool GetCommandList(CommandList* pCommandList, uint32_t nodeMask, CommandListType type,
			const CommandAllocator& commandAllocator, const Pipeline& pipeline);
		bool GetCommandList(CommandList& commandList, uint32_t nodeMask, CommandListType type,
			const CommandAllocator& commandAllocator, const Pipeline& pipeline);
		bool GetFence(Fence& fence, uint64_t initialValue, FenceFlag flags);
		bool CreateCommandLayout(CommandLayout& commandLayout, uint32_t byteStride, uint32_t numArguments,
			const IndirectArgument* pArguments, uint32_t nodeMask = 0);
	};
	using Device = com_ptr<DX12Device>;

	// Graphics pipelines related
	namespace Graphics
	{
		using Blend = std::shared_ptr<D3D12_BLEND_DESC>;
		using Rasterizer = std::shared_ptr<D3D12_RASTERIZER_DESC>;
		using DepthStencil = std::shared_ptr<D3D12_DEPTH_STENCIL_DESC>;
	}

	// DX12 format transfer function
	DLL_INTERFACE DXGI_FORMAT GetDXGIFormat(Format format);

	uint32_t GetDX12Requirement(Requirement requirement);
}
