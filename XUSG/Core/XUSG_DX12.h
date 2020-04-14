//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#define H_RETURN(x, o, m, r)		{ const auto hr = x; if (FAILED(hr)) { o << m << std::endl; return r; } }
#define V_RETURN(x, o, r)			H_RETURN(x, o, HrToString(hr), r)

#define M_RETURN(x, o, m, r)		if (x) { o << m << std::endl; return r; }
#define F_RETURN(x, o, h, r)		M_RETURN(x, o, HrToString(h), r)

#define BARRIER_ALL_SUBRESOURCES	D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES

namespace XUSG
{
	template <typename T>
	class com_ptr :
		public Microsoft::WRL::ComPtr<T>
	{
	public:
		using element_type = T;

		com_ptr() : Microsoft::WRL::ComPtr<T>::ComPtr() {}
		com_ptr(decltype(__nullptr) null) : Microsoft::WRL::ComPtr<T>::ComPtr(null) {}

		template<class U>
		com_ptr(U* other) : Microsoft::WRL::ComPtr<T>::ComPtr(other) {}
		com_ptr(const Microsoft::WRL::ComPtr<T>& other) : Microsoft::WRL::ComPtr<T>::ComPtr(other) {}

		template<class U>
		com_ptr(const Microsoft::WRL::ComPtr<U>& other, typename Microsoft::WRL::Details::EnableIf<__is_convertible_to(U*, T*), void*>::type* t = 0) :
			Microsoft::WRL::ComPtr<T>::ComPtr(other, t) {}
		com_ptr(Microsoft::WRL::ComPtr<T>&& other) : Microsoft::WRL::ComPtr<T>::ComPtr(other) {}

		template<class U>
		com_ptr(Microsoft::WRL::ComPtr<U>&& other, typename Microsoft::WRL::Details::EnableIf<__is_convertible_to(U*, T*), void*>::type* t = 0) :
			Microsoft::WRL::ComPtr<T>::ComPtr(other, t) {}

		T* get() const { return Microsoft::WRL::ComPtr<T>::Get(); }
	};

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
	using BlobType = ID3DBlob;
	using Blob = com_ptr<BlobType>;
	using SwapChain = com_ptr<IDXGISwapChain3>;

	// Command lists related
	using BaseCommandList = ID3D12CommandList;
	using GraphicsCommandList = com_ptr<ID3D12GraphicsCommandList>;
	using CommandAllocator = com_ptr<ID3D12CommandAllocator>;
	using CommandQueue = com_ptr<ID3D12CommandQueue>;
	using Fence = com_ptr<ID3D12Fence>;

	// Resources related
	using Resource = com_ptr<ID3D12Resource>;
	using VertexBufferView = D3D12_VERTEX_BUFFER_VIEW;
	using IndexBufferView = D3D12_INDEX_BUFFER_VIEW;
	using StreamOutBufferView = D3D12_STREAM_OUTPUT_BUFFER_VIEW;
	using Sampler = D3D12_SAMPLER_DESC;
	using ResourceBarrier = CD3DX12_RESOURCE_BARRIER;

	using SubresourceData = D3D12_SUBRESOURCE_DATA;
	using TextureCopyLocation = CD3DX12_TEXTURE_COPY_LOCATION;
	using Viewport = CD3DX12_VIEWPORT;
	using Range = CD3DX12_RANGE;
	using RectRange = CD3DX12_RECT;
	using BoxRange = CD3DX12_BOX;
	using TiledResourceCoord = CD3DX12_TILED_RESOURCE_COORDINATE;
	using TileRegionSize = D3D12_TILE_REGION_SIZE;
	using TileCopyFlags = D3D12_TILE_COPY_FLAGS;

	// Descriptors related
	using DescriptorPool = com_ptr<ID3D12DescriptorHeap>;
	using Descriptor = CD3DX12_CPU_DESCRIPTOR_HANDLE;
	using DescriptorTable = std::shared_ptr<CD3DX12_GPU_DESCRIPTOR_HANDLE>;
	struct Framebuffer
	{
		uint32_t NumRenderTargetDescriptors;
		std::shared_ptr<Descriptor> RenderTargetViews;
		Descriptor DepthStencilView;
	};

	// Pipeline layouts related
	using PipelineLayout = com_ptr<ID3D12RootSignature>;
	using DescriptorRangeList = std::vector<CD3DX12_DESCRIPTOR_RANGE1>;

	// Input layouts related
	struct InputElementDesc
	{
		const char* SemanticName;
		uint32_t SemanticIndex;
		Format Format;
		uint32_t InputSlot;
		uint32_t AlignedByteOffset;
		InputClassification InputSlotClass;
		uint32_t InstanceDataStepRate;
	};
	using InputElementTable = std::vector<InputElementDesc>;
	struct InputLayoutDesc : public D3D12_INPUT_LAYOUT_DESC
	{
		std::vector<D3D12_INPUT_ELEMENT_DESC> Elements;
	};
	using InputLayout = std::shared_ptr<InputLayoutDesc>;

	// Pipeline layouts related
	struct RootParameter : public CD3DX12_ROOT_PARAMETER1
	{
		DescriptorRangeList ranges;
	};
	using DescriptorTableLayout = std::shared_ptr<RootParameter>;

	using Pipeline = com_ptr<ID3D12PipelineState>;

	struct IndirectArgument
	{
		IndirectArgumentType Type;
		union
		{
			struct
			{
				uint32_t Slot;
			} VertexBuffer;
			struct
			{
				uint32_t Index;
				uint32_t DestOffsetIn32BitValues;
				uint32_t Num32BitValuesToSet;
			} Constant;
			struct
			{
				uint32_t Index;
			} ConstantBufferView;
			struct
			{
				uint32_t Index;
			} ShaderResourceView;
			struct
			{
				uint32_t Index;
			} UnorderedAccessView;
		};
	};
	using CommandLayout = com_ptr<ID3D12CommandSignature>;

	// Device
	MIDL_INTERFACE("189819f1-1db6-4b57-be54-1821339b85f7")
		DLL_INTERFACE DX12Device : public ID3D12Device
	{
		bool GetCommandQueue(CommandQueue & commandQueue, CommandListType type, CommandQueueFlag flags, int32_t priority = 0, uint32_t nodeMask = 0);
		bool GetCommandAllocator(CommandAllocator& commandAllocator, CommandListType type);
		bool GetCommandList(GraphicsCommandList& commandList, uint32_t nodeMask, CommandListType type,
			const CommandAllocator& commandAllocator, const Pipeline& pipeline);
		bool GetFence(Fence& fence, uint64_t initialValue, FenceFlag flags);
		bool CreateCommandLayout(CommandLayout& commandLayout, uint32_t byteStride, uint32_t numArguments,
			const IndirectArgument* pArguments, uint32_t nodeMask = 0);
	};
	using Device = com_ptr<DX12Device>;

	// Shaders related
	namespace Shader
	{
		using ByteCode = CD3DX12_SHADER_BYTECODE;
		using Reflection = com_ptr<ID3D12ShaderReflection>;
		using LibReflection = com_ptr<ID3D12LibraryReflection>;
	}

	// Graphics pipelines related
	namespace Graphics
	{
		using PipelineDesc = D3D12_GRAPHICS_PIPELINE_STATE_DESC;

		using Blend = std::shared_ptr<D3D12_BLEND_DESC>;
		using Rasterizer = std::shared_ptr < D3D12_RASTERIZER_DESC>;
		using DepthStencil = std::shared_ptr < D3D12_DEPTH_STENCIL_DESC>;
	}

	// Compute pipelines related
	namespace Compute
	{
		using PipelineDesc = D3D12_COMPUTE_PIPELINE_STATE_DESC;
	}

	// DX12 enum transfer functions
	DXGI_FORMAT GetDXGIFormat(Format format);

	D3D12_COMMAND_LIST_TYPE GetDX12CommandListType(CommandListType commandListType);
	D3D12_INPUT_CLASSIFICATION GetDX12InputClassification(InputClassification inputClassification);
	D3D12_HEAP_TYPE GetDX12HeapType(MemoryType memoryType);
	D3D12_PRIMITIVE_TOPOLOGY_TYPE GetDX12PrimitiveTopologyType(PrimitiveTopologyType primitiveTopologyType);
	D3D_PRIMITIVE_TOPOLOGY GetDX12PrimitiveTopology(PrimitiveTopology primitiveTopology);
	D3D12_INDIRECT_ARGUMENT_TYPE GetDX12IndirectArgumentType(IndirectArgumentType indirectArgumentType);
	D3D12_RESOURCE_DIMENSION GetDX12ResourceDimension(ResourceDimension resourceDimension);

	D3D12_COMMAND_QUEUE_FLAGS GetDX12CommandQueueFlag(CommandQueueFlag commandQueueFlag);
	D3D12_COMMAND_QUEUE_FLAGS GetDX12CommandQueueFlags(CommandQueueFlag commandQueueFlags);

	D3D12_RESOURCE_FLAGS GetDX12ResourceFlag(ResourceFlag resourceFlag);
	D3D12_RESOURCE_FLAGS GetDX12ResourceFlags(ResourceFlag resourceFlags);

	D3D12_RESOURCE_STATES GetDX12ResourceState(ResourceState resourceState);
	D3D12_RESOURCE_STATES GetDX12ResourceStates(ResourceState resourceStates);

	D3D12_RESOURCE_BARRIER_FLAGS GetDX12BarrierFlag(BarrierFlag barrierFlag);
	D3D12_RESOURCE_BARRIER_FLAGS GetDX12BarrierFlags(BarrierFlag barrierFlags);

	D3D12_DESCRIPTOR_RANGE_FLAGS GetDX12DescriptorRangeFlag(DescriptorFlag descriptorFlag);
	D3D12_DESCRIPTOR_RANGE_FLAGS GetDX12DescriptorRangeFlags(DescriptorFlag descriptorFlag);

	D3D12_ROOT_DESCRIPTOR_FLAGS GetDX12RootDescriptorFlag(DescriptorFlag descriptorFlag);
	D3D12_ROOT_DESCRIPTOR_FLAGS GetDX12RootDescriptorFlags(DescriptorFlag descriptorFlag);

	D3D12_ROOT_SIGNATURE_FLAGS GetDX12RootSignatureFlag(PipelineLayoutFlag pipelineLayoutFlag);
	D3D12_ROOT_SIGNATURE_FLAGS GetDX12RootSignatureFlags(PipelineLayoutFlag pipelineLayoutFlags);

	D3D12_CLEAR_FLAGS GetDX12ClearFlag(ClearFlag clearFlag);
	D3D12_CLEAR_FLAGS GetDX12ClearFlags(ClearFlag clearFlags);

	D3D12_FENCE_FLAGS GetDX12FenceFlag(FenceFlag fenceFlag);
	D3D12_FENCE_FLAGS GetDX12FenceFlags(FenceFlag fenceFlags);

	uint32_t GetDX12Requirement(Requirement requirement);
}
