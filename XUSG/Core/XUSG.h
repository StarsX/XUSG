//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#pragma warning(disable:4250)

#ifndef DLL_INTERFACE
#define DLL_INTERFACE
#endif

#define M_RETURN(x, o, m, r)		if (x) { o << m << std::endl; assert(!m); return r; }
#define C_RETURN(x, r)				if (x) return r
#define N_RETURN(x, r)				C_RETURN(!(x), r)
#define X_RETURN(x, f, r)			{ x = f; N_RETURN(x, r); }

#define DIV_UP(x, n)				(((x) - 1) / (n) + 1)
#define SizeOfInUint32(obj)			DIV_UP(sizeof(obj), sizeof(uint32_t))

#define APPEND_ALIGNED_ELEMENT		0xffffffff
#define BARRIER_ALL_SUBRESOURCES	0xffffffff

namespace XUSG
{
#if defined(WIN32) || (_WIN32)
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
#endif

	// Enumerations
	enum class API
	{
		DIRECTX_12
	};

	enum class Format : uint32_t
	{
		UNKNOWN,
		R32G32B32A32_TYPELESS,
		R32G32B32A32_FLOAT,
		R32G32B32A32_UINT,
		R32G32B32A32_SINT,
		R32G32B32_TYPELESS,
		R32G32B32_FLOAT,
		R32G32B32_UINT,
		R32G32B32_SINT,
		R16G16B16A16_TYPELESS,
		R16G16B16A16_FLOAT,
		R16G16B16A16_UNORM,
		R16G16B16A16_UINT,
		R16G16B16A16_SNORM,
		R16G16B16A16_SINT,
		R32G32_TYPELESS,
		R32G32_FLOAT,
		R32G32_UINT,
		R32G32_SINT,
		R32G8X24_TYPELESS,
		D32_FLOAT_S8X24_UINT,
		R32_FLOAT_X8X24_TYPELESS,
		X32_TYPELESS_G8X24_UINT,
		R10G10B10A2_TYPELESS,
		R10G10B10A2_UNORM,
		R10G10B10A2_UINT,
		R11G11B10_FLOAT,
		R8G8B8A8_TYPELESS,
		R8G8B8A8_UNORM,
		R8G8B8A8_UNORM_SRGB,
		R8G8B8A8_UINT,
		R8G8B8A8_SNORM,
		R8G8B8A8_SINT,
		R16G16_TYPELESS,
		R16G16_FLOAT,
		R16G16_UNORM,
		R16G16_UINT,
		R16G16_SNORM,
		R16G16_SINT,
		R32_TYPELESS,
		D32_FLOAT,
		R32_FLOAT,
		R32_UINT,
		R32_SINT,
		R24G8_TYPELESS,
		D24_UNORM_S8_UINT,
		R24_UNORM_X8_TYPELESS,
		X24_TYPELESS_G8_UINT,
		R8G8_TYPELESS,
		R8G8_UNORM,
		R8G8_UINT,
		R8G8_SNORM,
		R8G8_SINT,
		R16_TYPELESS,
		R16_FLOAT,
		D16_UNORM,
		R16_UNORM,
		R16_UINT,
		R16_SNORM,
		R16_SINT,
		R8_TYPELESS,
		R8_UNORM,
		R8_UINT,
		R8_SNORM,
		R8_SINT,
		A8_UNORM,
		R1_UNORM,
		R9G9B9E5_SHAREDEXP,
		R8G8_B8G8_UNORM,
		G8R8_G8B8_UNORM,
		BC1_TYPELESS,
		BC1_UNORM,
		BC1_UNORM_SRGB,
		BC2_TYPELESS,
		BC2_UNORM,
		BC2_UNORM_SRGB,
		BC3_TYPELESS,
		BC3_UNORM,
		BC3_UNORM_SRGB,
		BC4_TYPELESS,
		BC4_UNORM,
		BC4_SNORM,
		BC5_TYPELESS,
		BC5_UNORM,
		BC5_SNORM,
		B5G6R5_UNORM,
		B5G5R5A1_UNORM,
		B8G8R8A8_UNORM,
		B8G8R8X8_UNORM,
		RGB10_XR_BIAS_A2_UNORM,
		B8G8R8A8_TYPELESS,
		B8G8R8A8_UNORM_SRGB,
		B8G8R8X8_TYPELESS,
		B8G8R8X8_UNORM_SRGB,
		BC6H_TYPELESS,
		BC6H_UF16,
		BC6H_SF16,
		BC7_TYPELESS,
		BC7_UNORM,
		BC7_UNORM_SRGB,
		AYUV,
		Y410,
		Y416,
		NV12,
		P010,
		P016,
		OPAQUE_420,
		YUY2,
		Y210,
		Y216,
		NV11,
		AI44,
		IA44,
		P8,
		A8P8,
		B4G4R4A4_UNORM,

		P208,
		V208,
		V408,

		MIN_MIP_OPAQUE,
		MIP_REGION_USED_OPAQUE,

		FORCE_UINT
	};

	enum class CommandListType : uint8_t
	{
		DIRECT,
		BUNDLE,
		COMPUTE,
		COPY,
		VIDEO_DECODE,
		VIDEO_PROCESS,
		VIDEO_ENCODE
	};

	enum class InputClassification : uint8_t
	{
		PER_VERTEX_DATA,
		PER_INSTANCE_DATA
	};

	enum class MemoryType : uint8_t
	{
		DEFAULT,
		UPLOAD,
		READBACK,
		CUSTOM
	};

	enum class PrimitiveTopologyType : uint8_t
	{
		UNDEFINED,
		POINT,
		LINE,
		TRIANGLE,
		PATCH
	};

	enum class PrimitiveTopology : uint8_t
	{
		UNDEFINED,
		POINTLIST,
		LINELIST,
		LINESTRIP,
		TRIANGLELIST,
		TRIANGLESTRIP,
		LINELIST_ADJ,
		LINESTRIP_ADJ,
		TRIANGLELIST_ADJ,
		TRIANGLESTRIP_ADJ,
		CONTROL_POINT1_PATCHLIST,
		CONTROL_POINT2_PATCHLIST,
		CONTROL_POINT3_PATCHLIST,
		CONTROL_POINT4_PATCHLIST,
		CONTROL_POINT5_PATCHLIST,
		CONTROL_POINT6_PATCHLIST,
		CONTROL_POINT7_PATCHLIST,
		CONTROL_POINT8_PATCHLIST,
		CONTROL_POINT9_PATCHLIST,
		CONTROL_POINT10_PATCHLIST,
		CONTROL_POINT11_PATCHLIST,
		CONTROL_POINT12_PATCHLIST,
		CONTROL_POINT13_PATCHLIST,
		CONTROL_POINT14_PATCHLIST,
		CONTROL_POINT15_PATCHLIST,
		CONTROL_POINT16_PATCHLIST,
		CONTROL_POINT17_PATCHLIST,
		CONTROL_POINT18_PATCHLIST,
		CONTROL_POINT19_PATCHLIST,
		CONTROL_POINT20_PATCHLIST,
		CONTROL_POINT21_PATCHLIST,
		CONTROL_POINT22_PATCHLIST,
		CONTROL_POINT23_PATCHLIST,
		CONTROL_POINT24_PATCHLIST,
		CONTROL_POINT25_PATCHLIST,
		CONTROL_POINT26_PATCHLIST,
		CONTROL_POINT27_PATCHLIST,
		CONTROL_POINT28_PATCHLIST,
		CONTROL_POINT29_PATCHLIST,
		CONTROL_POINT30_PATCHLIST,
		CONTROL_POINT31_PATCHLIST,
		CONTROL_POINT32_PATCHLIST
	};

	enum class FillMode
	{
		WIREFRAME,
		SOLID
	};

	enum class CullMode
	{
		NONE,
		FRONT,
		BACK
	};

	enum class ResourceDimension : uint8_t
	{
		UNKNOWN,
		BUFFER,
		TEXTURE1D,
		TEXTURE2D,
		TEXTURE3D
	};

	enum class DescriptorType : uint8_t
	{
		SRV,
		UAV,
		CBV,
		SAMPLER,
		CONSTANT,
		ROOT_SRV,
		ROOT_UAV,
		ROOT_CBV,

		NUM
	};

	enum class IndirectArgumentType : uint32_t
	{
		DRAW,
		DRAW_INDEXED,
		DISPATCH,
		VERTEX_BUFFER_VIEW,
		INDEX_BUFFER_VIEW,
		CONSTANT,
		CONSTANT_BUFFER_VIEW,
		SHADER_RESOURCE_VIEW,
		UNORDERED_ACCESS_VIEW
	};

	enum class MemoryFlag : uint32_t
	{
		NONE = 0,
		SHARED = (1 << 0),
		DENY_BUFFERS = (1 << 1),
		ALLOW_DISPLAY = (1 << 2),
		SHARED_CROSS_ADAPTER = (1 << 3),
		DENY_RT_DS_TEXTURES = (1 << 4),
		DENY_NON_RT_DS_TEXTURES = (1 << 5),
		HARDWARE_PROTECTED = (1 << 6),
		ALLOW_WRITE_WATCH = (1 << 7),
		ALLOW_SHADER_ATOMICS = (1 << 8),
		CREATE_NOT_RESIDENT = (1 << 9),
		CREATE_NOT_ZEROED = (1 << 10),
		ALLOW_ALL_BUFFERS_AND_TEXTURES = 0,
		ALLOW_ONLY_BUFFERS = DENY_RT_DS_TEXTURES | DENY_NON_RT_DS_TEXTURES,
		ALLOW_ONLY_NON_RT_DS_TEXTURES = D3D12_HEAP_FLAG_DENY_BUFFERS | DENY_RT_DS_TEXTURES,
		ALLOW_ONLY_RT_DS_TEXTURES = D3D12_HEAP_FLAG_DENY_BUFFERS | DENY_NON_RT_DS_TEXTURES
	};

	DEFINE_ENUM_FLAG_OPERATORS(MemoryFlag);

	enum class ResourceFlag : uint32_t
	{
		NONE = 0,
		ALLOW_RENDER_TARGET = (1 << 0),
		ALLOW_DEPTH_STENCIL = (1 << 1),
		ALLOW_UNORDERED_ACCESS = (1 << 2),
		DENY_SHADER_RESOURCE = (1 << 3),
		ALLOW_CROSS_ADAPTER = (1 << 4),
		ALLOW_SIMULTANEOUS_ACCESS = (1 << 5),
		VIDEO_DECODE_REFERENCE_ONLY = (1 << 6),
		NEED_PACKED_UAV = ALLOW_UNORDERED_ACCESS | 0x8000,
		ACCELERATION_STRUCTURE = ALLOW_UNORDERED_ACCESS | 0x400000
	};

	DEFINE_ENUM_FLAG_OPERATORS(ResourceFlag);

	enum class ResourceState : uint32_t
	{
		COMMON = 0,
		VERTEX_AND_CONSTANT_BUFFER = (1 << 0),
		INDEX_BUFFER = (1 << 1),
		RENDER_TARGET = (1 << 2),
		UNORDERED_ACCESS = (1 << 3),
		DEPTH_WRITE = (1 << 4),
		DEPTH_READ = (1 << 5),
		NON_PIXEL_SHADER_RESOURCE = (1 << 6),
		PIXEL_SHADER_RESOURCE = (1 << 7),
		STREAM_OUT = (1 << 8),
		INDIRECT_ARGUMENT = (1 << 9),
		COPY_DEST = (1 << 10),
		COPY_SOURCE = (1 << 11),
		RESOLVE_DEST = (1 << 12),
		RESOLVE_SOURCE = (1 << 13),
		PREDICATION = (1 << 14),
		RAYTRACING_ACCELERATION_STRUCTURE = (1 << 15),
		SHADING_RATE_SOURCE = (1 << 16),

		GENERAL_READ = (VERTEX_AND_CONSTANT_BUFFER | INDEX_BUFFER | NON_PIXEL_SHADER_RESOURCE | PIXEL_SHADER_RESOURCE | INDIRECT_ARGUMENT | COPY_SOURCE | PREDICATION),
		PRESENT = 0,

		VIDEO_DECODE_READ = (1 << 17),
		VIDEO_DECODE_WRITE = (1 << 18),
		VIDEO_PROCESS_READ = (1 << 19),
		VIDEO_PROCESS_WRITE = (1 << 20),
		VIDEO_ENCODE_READ = (1 << 21),
		VIDEO_ENCODE_WRITE = (1 << 22)
	};

	DEFINE_ENUM_FLAG_OPERATORS(ResourceState);

	enum class BarrierFlag : uint8_t
	{
		NONE = 0,
		BEGIN_ONLY = (1 << 0),
		END_ONLY = (1 << 1)
	};

	DEFINE_ENUM_FLAG_OPERATORS(BarrierFlag);

	enum class DescriptorFlag : uint8_t
	{
		NONE = 0,
		DESCRIPTORS_VOLATILE = (1 << 0),
		DATA_VOLATILE = (1 << 1),
		DATA_STATIC_WHILE_SET_AT_EXECUTE = (1 << 2),
		DATA_STATIC = (1 << 3),
		DESCRIPTORS_STATIC_KEEPING_BUFFER_BOUNDS_CHECKS = (1 << 4)
	};

	DEFINE_ENUM_FLAG_OPERATORS(DescriptorFlag);

	enum class PipelineLayoutFlag : uint32_t
	{
		NONE = 0,
		ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT = (1 << 0),
		DENY_VERTEX_SHADER_ROOT_ACCESS = (1 << 1),
		DENY_HULL_SHADER_ROOT_ACCESS = (1 << 2),
		DENY_DOMAIN_SHADER_ROOT_ACCESS = (1 << 3),
		DENY_GEOMETRY_SHADER_ROOT_ACCESS = (1 << 4),
		DENY_PIXEL_SHADER_ROOT_ACCESS = (1 << 5),
		ALLOW_STREAM_OUTPUT = (1 << 6),
		LOCAL_PIPELINE_LAYOUT = (1 << 7),
		DENY_AMPLIFICATION_SHADER_ROOT_ACCESS = (1 << 8),
		DENY_MESH_SHADER_ROOT_ACCESS = (1 << 9),
		CBV_SRV_UAV_POOL_DIRECTLY_INDEXED = (1 << 10),
		SAMPLER_POOL_DIRECTLY_INDEXED = (1 << 11)
	};

	DEFINE_ENUM_FLAG_OPERATORS(PipelineLayoutFlag);

	enum class CommandQueueFlag : uint8_t
	{
		NONE = 0,
		DISABLE_GPU_TIMEOUT = (1 << 0)
	};

	DEFINE_ENUM_FLAG_OPERATORS(CommandQueueFlag);

	enum class FenceFlag : uint8_t
	{
		NONE = 0,
		SHARED = (1 << 0),
		SHARED_CROSS_ADAPTER = (1 << 1),
		NON_MONITORED = (1 << 2)
	};

	DEFINE_ENUM_FLAG_OPERATORS(FenceFlag);

	enum class ClearFlag : uint8_t
	{
		NONE = 0,
		DEPTH = (1 << 0),
		STENCIL = (1 << 1)
	};

	DEFINE_ENUM_FLAG_OPERATORS(ClearFlag);

	enum class BlendFactor : uint8_t
	{
		ZERO,
		ONE,
		SRC_COLOR,
		INV_SRC_COLOR,
		SRC_ALPHA,
		INV_SRC_ALPHA,
		DEST_ALPHA,
		INV_DEST_ALPHA,
		DEST_COLOR,
		INV_DEST_COLOR,
		SRC_ALPHA_SAT,
		BLEND_FACTOR,
		INV_BLEND_FACTOR,
		SRC1_COLOR,
		INV_SRC1_COLOR,
		SRC1_ALPHA,
		INV_SRC1_ALPHA
	};

	enum class BlendOperator : uint8_t
	{
		ADD,
		SUBTRACT,
		REV_SUBTRACT,
		MIN,
		MAX
	};

	enum class LogicOperator : uint8_t
	{
		CLEAR,
		SET,
		COPY,
		COPY_INVERTED,
		NOOP,
		INVERT,
		AND,
		NAND,
		OR,
		NOR,
		XOR,
		EQUIV,
		AND_REVERSE,
		AND_INVERTED,
		OR_REVERSE,
		OR_INVERTED
	};

	enum class ColorWrite : uint8_t
	{
		RED,
		GREEN,
		BLUE,
		ALPHA,
		ALL
	};

	enum class ComparisonFunc : uint8_t
	{
		NEVER,
		LESS,
		EQUAL,
		LESS_EQUAL,
		GREATER,
		NOT_EQUAL,
		GREATER_EQUAL,
		ALWAYS
	};

	enum class StencilOp : uint8_t
	{
		KEEP,
		ZERO,
		REPLACE,
		INCR_SAT,
		DECR_SAT,
		INVERT,
		INCR,
		DECR
	};

	enum class IBStripCutValue : uint8_t
	{
		DISABLED,
		FFFF,
		FFFFFFFF
	};

	enum class QueryType : uint8_t
	{
		OCCLUSION,
		BINARY_OCCLUSION,
		TIMESTAMP,
		PIPELINE_STATISTICS,
		SO_STATISTICS_STREAM0,
		SO_STATISTICS_STREAM1,
		SO_STATISTICS_STREAM2,
		SO_STATISTICS_STREAM3,
		VIDEO_DECODE_STATISTICS
	};

	enum class TileCopyFlag : uint8_t
	{
		NONE = 0,
		NO_HAZARD = (1 << 0),
		LINEAR_BUFFER_TO_SWIZZLED_TILED_RESOURCE = (1 << 1),
		SWIZZLED_TILED_RESOURCE_TO_LINEAR_BUFFER = (1 << 2)
	};

	DEFINE_ENUM_FLAG_OPERATORS(TileCopyFlag);

	namespace Shader
	{
		enum Stage : uint8_t
		{
			PS,
			VS,
			DS,
			HS,
			GS,
			CS,
			ALL = CS,

			NUM_GRAPHICS = ALL,

			MS,
			AS,

			NUM_STAGE,
		};
	}

	enum DescriptorPoolType : uint8_t
	{
		CBV_SRV_UAV_POOL,
		SAMPLER_POOL,
		RTV_POOL,

		NUM_DESCRIPTOR_POOL
	};

	enum SamplerPreset : uint8_t
	{
		POINT_WRAP,
		POINT_MIRROR,
		POINT_CLAMP,
		POINT_BORDER,
		POINT_LESS_EQUAL,

		LINEAR_WRAP,
		LINEAR_MIRROR,
		LINEAR_CLAMP,
		LINEAR_BORDER,
		LINEAR_LESS_EQUAL,

		ANISOTROPIC_WRAP,
		ANISOTROPIC_MIRROR,
		ANISOTROPIC_CLAMP,
		ANISOTROPIC_BORDER,
		ANISOTROPIC_LESS_EQUAL,

		NUM_SAMPLER_PRESET
	};

	enum Requirement : uint32_t
	{
		REQ_MIP_LEVELS,
		REQ_TEXTURECUBE_DIMENSION,
		REQ_TEXTURE1D_U_DIMENSION,
		REQ_TEXTURE2D_U_OR_V_DIMENSION,
		REQ_TEXTURE3D_U_V_OR_W_DIMENSION,
		REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION,
		REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION
	};

	// Resources related
	class Resource;

	struct SubresourceData
	{
		const void* pData;
		intptr_t RowPitch;
		intptr_t SlicePitch;
	};

	struct ResourceBarrier
	{
		const Resource* pResource;
		ResourceState StateBefore;
		ResourceState StateAfter;
		uint32_t Subresource;
		BarrierFlag Flags;
		const Resource* pResourceAfter;
	};

	struct VertexBufferView
	{
		uint64_t BufferLocation;
		uint32_t SizeInBytes;
		uint32_t StrideInBytes;
	};

	struct IndexBufferView
	{
		uint64_t BufferLocation;
		uint32_t SizeInBytes;
		Format Format;
	};

	struct StreamOutBufferView
	{
		uint64_t BufferLocation;
		uint64_t SizeInBytes;
		uint64_t BufferFilledSizeLocation;
	};

	struct SamplerDesc
	{
		uint16_t Filter;
		uint8_t AddressU;
		uint8_t AddressV;
		uint8_t AddressW;
		float MipLODBias;
		uint8_t MaxAnisotropy;
		uint8_t ComparisonFunc;
		float BorderColor[4];
		float MinLOD;
		float MaxLOD;
	};

	using Sampler = std::shared_ptr<SamplerDesc>;

	// Pipeline state related
	struct Viewport
	{
		Viewport() = default;
		Viewport(float topLeftX, float topLeftY, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f)
		{
			TopLeftX = topLeftX;
			TopLeftY = topLeftY;
			Width = width;
			Height = height;
			MinDepth = minDepth;
			MaxDepth = maxDepth;
		}

		float TopLeftX;
		float TopLeftY;
		float Width;
		float Height;
		float MinDepth;
		float MaxDepth;
	};

	struct Range
	{
		Range() = default;
		Range(uintptr_t begin, uintptr_t end)
		{
			Begin = begin;
			End = end;
		}

		uintptr_t Begin;
		uintptr_t End;
	};

	struct RectRange
	{
		RectRange() = default;
		RectRange(long left, long top, long right, long bottom)
		{
			Left = left;
			Top = top;
			Right = right;
			Bottom = bottom;
		}

		long Left;
		long Top;
		long Right;
		long Bottom;
	};

	struct BoxRange
	{
		BoxRange() = default;
		BoxRange(long left, long right)
		{
			Left = static_cast<uint32_t>(left);
			Top = 0;
			Front = 0;
			Right = static_cast<uint32_t>(right);
			Bottom = 1;
			Back = 1;
		}
		BoxRange(long left, long top, long right, long bottom)
		{
			Left = static_cast<uint32_t>(left);
			Top = static_cast<uint32_t>(top);
			Front = 0;
			Right = static_cast<uint32_t>(right);
			Bottom = static_cast<uint32_t>(bottom);
			Back = 1;
		}
		BoxRange(long left, long top, long front, long right, long bottom, long back)
		{
			Left = static_cast<uint32_t>(left);
			Top = static_cast<uint32_t>(top);
			Front = static_cast<uint32_t>(front);
			Right = static_cast<uint32_t>(right);
			Bottom = static_cast<uint32_t>(bottom);
			Back = static_cast<uint32_t>(back);
		}

		uint32_t Left;
		uint32_t Top;
		uint32_t Front;
		uint32_t Right;
		uint32_t Bottom;
		uint32_t Back;
	};

	// Resource copy related
	struct TextureCopyLocation
	{
		TextureCopyLocation() = default;
		TextureCopyLocation(const Resource* pRes, uint32_t sub)
		{
			pResource = pRes;
			SubresourceIndex = sub;
		}

		const Resource* pResource;
		uint32_t SubresourceIndex;
	};

	struct TiledResourceCoord
	{
		TiledResourceCoord() = default;
		TiledResourceCoord(uint32_t x, uint32_t y, uint32_t z, uint32_t subresource)
		{
			X = x;
			Y = y;
			Z = z;
			Subresource = subresource;
		}

		uint32_t X;
		uint32_t Y;
		uint32_t Z;
		uint32_t Subresource;
	};

	struct TileRegionSize
	{
		uint32_t NumTiles;
		bool UseBox;
		uint32_t Width;
		uint16_t Height;
		uint16_t Depth;
	};

	// Descriptors related
	struct DescriptorRange
	{
		DescriptorType Type;
		uint32_t NumDescriptors;
		uint32_t BaseBinding;
		uint32_t Space;
		DescriptorFlag Flags;
		uint32_t OffsetInDescriptors;
	};

	using Descriptor = uintptr_t;
	using DescriptorTable = std::shared_ptr<uint64_t>;
	struct Framebuffer
	{
		uint32_t NumRenderTargetDescriptors;
		std::shared_ptr<Descriptor> RenderTargetViews;
		Descriptor DepthStencilView;
	};

	// Input layouts related
	struct InputElement
	{
		const char* SemanticName;
		uint32_t SemanticIndex;
		Format Format;
		uint32_t InputSlot;
		uint32_t AlignedByteOffset;
		InputClassification InputSlotClass;
		uint32_t InstanceDataStepRate;
	};
	using InputLayout = std::vector<InputElement>;

	// Indirect command related
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
			} CbvSrvUav;
		};
	};

	// Handles
	using Pipeline = void*;
	using PipelineLayout = void*;
	using DescriptorTableLayout = const void*;
	using DescriptorPool = void*;
	using Blob = void*;
	using QueryPool = void*;

	//--------------------------------------------------------------------------------------
	// Device
	//--------------------------------------------------------------------------------------
	class CommandAllocator;
	class CommandList;
	class CommandQueue;
	class Fence;

	class DLL_INTERFACE Device
	{
	public:
		//Device();
		virtual ~Device() {};

		virtual bool GetCommandQueue(CommandQueue* pCommandQueue, CommandListType type,
			CommandQueueFlag flags, int32_t priority = 0, uint32_t nodeMask = 0,
			const wchar_t* name = nullptr) = 0;
		virtual bool GetCommandAllocator(CommandAllocator* pCommandAllocator,
			CommandListType type, const wchar_t* name = nullptr) = 0;
		virtual bool GetCommandList(CommandList* pCommandList, uint32_t nodeMask, CommandListType type,
			const CommandAllocator* pCommandAllocator, const Pipeline& pipeline,
			const wchar_t* name = nullptr) = 0;
		virtual bool GetFence(Fence* pFence, uint64_t initialValue, FenceFlag flags,
			const wchar_t* name = nullptr) = 0;

		virtual uint32_t Create(void* pAdapter, uint32_t minFeatureLevel, const wchar_t* name = nullptr) = 0;
		virtual uint32_t GetDeviceRemovedReason() const = 0;

		virtual void Create(void* pHandle, const wchar_t* name = nullptr) = 0;

		virtual void* GetHandle() const = 0;

		using uptr = std::unique_ptr<Device>;
		using sptr = std::shared_ptr<Device>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Fence
	//--------------------------------------------------------------------------------------
	class DLL_INTERFACE Fence
	{
	public:
		//Fence();
		virtual ~Fence() {};

		virtual bool Create(const Device* pDevice, uint64_t initialValue,
			FenceFlag flags, const wchar_t* name = nullptr) = 0;
		virtual bool SetEventOnCompletion(uint64_t value, void* hEvent) = 0;
		virtual bool Signal(uint64_t value) = 0;

		virtual uint64_t GetCompletedValue() const = 0;

		virtual void* GetHandle() const = 0;

		using uptr = std::unique_ptr<Fence>;
		using sptr = std::shared_ptr<Fence>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	struct Semaphore
	{
		Fence::uptr Fence;
		uint64_t Value;
	};

	//--------------------------------------------------------------------------------------
	// Command layout
	//--------------------------------------------------------------------------------------
	class DLL_INTERFACE CommandLayout
	{
	public:
		//CommandLayout();
		virtual ~CommandLayout() {};

		virtual bool Create(const Device* pDevice, uint32_t byteStride, uint32_t numArguments,
			const IndirectArgument* pArguments, uint32_t nodeMask = 0, const wchar_t* name = nullptr) = 0;

		virtual void* GetHandle() const = 0;

		using uptr = std::unique_ptr<CommandLayout>;
		using sptr = std::shared_ptr<CommandLayout>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Command queue
	//--------------------------------------------------------------------------------------
	class DLL_INTERFACE CommandAllocator
	{
	public:
		//CommandAllocator();
		virtual ~CommandAllocator() {};

		virtual bool Create(const Device* pDevice, CommandListType type, const wchar_t* name = nullptr) = 0;
		virtual bool Reset() = 0;

		virtual void* GetHandle() const = 0;

		using uptr = std::unique_ptr<CommandAllocator>;
		using sptr = std::shared_ptr<CommandAllocator>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Command list
	//--------------------------------------------------------------------------------------
	class DLL_INTERFACE CommandList
	{
	public:
		//CommandList();
		virtual ~CommandList() {};

		virtual bool Create(const Device* pDevice, uint32_t nodeMask, CommandListType type,
			const CommandAllocator* pAllocator, const Pipeline& pipeline,
			const wchar_t* name = nullptr) = 0;
		virtual bool Close() const = 0;
		virtual bool Reset(const CommandAllocator* pAllocator,
			const Pipeline& initialState) const = 0;

		virtual void ClearState(const Pipeline& initialState) const = 0;
		virtual void Draw(
			uint32_t vertexCountPerInstance,
			uint32_t instanceCount,
			uint32_t startVertexLocation,
			uint32_t startInstanceLocation) const = 0;
		virtual void DrawIndexed(
			uint32_t indexCountPerInstance,
			uint32_t instanceCount,
			uint32_t startIndexLocation,
			int32_t baseVertexLocation,
			uint32_t startInstanceLocation) const = 0;
		virtual void Dispatch(
			uint32_t threadGroupCountX,
			uint32_t threadGroupCountY,
			uint32_t threadGroupCountZ) const = 0;
		virtual void CopyBufferRegion(const Resource* pDstBuffer, uint64_t dstOffset,
			const Resource* pSrcBuffer, uint64_t srcOffset, uint64_t numBytes) const = 0;
		virtual void CopyTextureRegion(const TextureCopyLocation& dst,
			uint32_t dstX, uint32_t dstY, uint32_t dstZ,
			const TextureCopyLocation& src, const BoxRange* pSrcBox = nullptr) const = 0;
		virtual void CopyResource(const Resource* pDstResource, const Resource* pSrcResource) const = 0;
		virtual void CopyTiles(const Resource* pTiledResource, const TiledResourceCoord* pTileRegionStartCoord,
			const TileRegionSize* pTileRegionSize, const Resource* pBuffer, uint64_t bufferStartOffsetInBytes,
			TileCopyFlag flags) const = 0;
		virtual void ResolveSubresource(const Resource* pDstResource, uint32_t dstSubresource,
			const Resource* pSrcResource, uint32_t srcSubresource, Format format) const = 0;
		virtual void IASetPrimitiveTopology(PrimitiveTopology primitiveTopology) const = 0;
		virtual void RSSetViewports(uint32_t numViewports, const Viewport* pViewports) const = 0;
		virtual void RSSetScissorRects(uint32_t numRects, const RectRange* pRects) const = 0;
		virtual void OMSetBlendFactor(const float blendFactor[4]) const = 0;
		virtual void OMSetStencilRef(uint32_t stencilRef) const = 0;
		virtual void SetPipelineState(const Pipeline& pipelineState) const = 0;
		virtual void Barrier(uint32_t numBarriers, const ResourceBarrier* pBarriers) const = 0;
		virtual void ExecuteBundle(const CommandList* pCommandList) const = 0;
		virtual void SetDescriptorPools(uint32_t numDescriptorPools, const DescriptorPool* pDescriptorPools) const = 0;
		virtual void SetComputePipelineLayout(const PipelineLayout& pipelineLayout) const = 0;
		virtual void SetGraphicsPipelineLayout(const PipelineLayout& pipelineLayout) const = 0;
		virtual void SetComputeDescriptorTable(uint32_t index, const DescriptorTable& descriptorTable) const = 0;
		virtual void SetGraphicsDescriptorTable(uint32_t index, const DescriptorTable& descriptorTable) const = 0;
		virtual void SetCompute32BitConstant(uint32_t index, uint32_t srcData, uint32_t destOffsetIn32BitValues = 0) const = 0;
		virtual void SetGraphics32BitConstant(uint32_t index, uint32_t srcData, uint32_t destOffsetIn32BitValues = 0) const = 0;
		virtual void SetCompute32BitConstants(uint32_t index, uint32_t num32BitValuesToSet,
			const void* pSrcData, uint32_t destOffsetIn32BitValues = 0) const = 0;
		virtual void SetGraphics32BitConstants(uint32_t index, uint32_t num32BitValuesToSet,
			const void* pSrcData, uint32_t destOffsetIn32BitValues = 0) const = 0;
		virtual void SetComputeRootConstantBufferView(uint32_t index, const Resource* pResource, int offset = 0) const = 0;
		virtual void SetGraphicsRootConstantBufferView(uint32_t index, const Resource* pResource, int offset = 0) const = 0;
		virtual void SetComputeRootShaderResourceView(uint32_t index, const Resource* pResource, int offset = 0) const = 0;
		virtual void SetGraphicsRootShaderResourceView(uint32_t index, const Resource* pResource, int offset = 0) const = 0;
		virtual void SetComputeRootUnorderedAccessView(uint32_t index, const Resource* pResource, int offset = 0) const = 0;
		virtual void SetGraphicsRootUnorderedAccessView(uint32_t index, const Resource* pResource, int offset = 0) const = 0;
		virtual void IASetIndexBuffer(const IndexBufferView& view) const = 0;
		virtual void IASetVertexBuffers(uint32_t startSlot, uint32_t numViews, const VertexBufferView* pViews) const = 0;
		virtual void SOSetTargets(uint32_t startSlot, uint32_t numViews, const StreamOutBufferView* pViews) const = 0;
		virtual void OMSetFramebuffer(const Framebuffer& framebuffer) const = 0;
		virtual void OMSetRenderTargets(
			uint32_t numRenderTargetDescriptors,
			const Descriptor* pRenderTargetViews,
			const Descriptor* pDepthStencilView = nullptr,
			bool rtsSingleHandleToDescriptorRange = false) const = 0;
		virtual void ClearDepthStencilView(const Framebuffer& framebuffer, ClearFlag clearFlags,
			float depth, uint8_t stencil = 0, uint32_t numRects = 0, const RectRange* pRects = nullptr) const = 0;
		virtual void ClearDepthStencilView(const Descriptor& depthStencilView, ClearFlag clearFlags,
			float depth, uint8_t stencil = 0, uint32_t numRects = 0, const RectRange* pRects = nullptr) const = 0;
		virtual void ClearRenderTargetView(const Descriptor& renderTargetView, const float colorRGBA[4],
			uint32_t numRects = 0, const RectRange* pRects = nullptr) const = 0;
		virtual void ClearUnorderedAccessViewUint(const DescriptorTable& descriptorTable,
			const Descriptor& descriptor, const Resource* pResource, const uint32_t values[4],
			uint32_t numRects = 0, const RectRange* pRects = nullptr) const = 0;
		virtual void ClearUnorderedAccessViewFloat(const DescriptorTable& descriptorTable,
			const Descriptor& descriptor, const Resource* pResource, const float values[4],
			uint32_t numRects = 0, const RectRange* pRects = nullptr) const = 0;
		virtual void DiscardResource(const Resource*pResource, uint32_t numRects, const RectRange* pRects,
			uint32_t firstSubresource, uint32_t numSubresources) const = 0;
		virtual void BeginQuery(const QueryPool& queryPool, QueryType type, uint32_t index) const = 0;
		virtual void EndQuery(const QueryPool& queryPool, QueryType type, uint32_t index) const = 0;
		virtual void ResolveQueryData(const QueryPool& queryPool, QueryType type, uint32_t startIndex,
			uint32_t numQueries, const Resource* pDstBuffer, uint64_t alignedDstBufferOffset) const = 0;
		virtual void SetPredication(const Resource* pBuffer, uint64_t alignedBufferOffset, bool opEqualZero)const = 0;
		virtual void SetMarker(uint32_t metaData, const void* pData, uint32_t size) const = 0;
		virtual void BeginEvent(uint32_t metaData, const void* pData, uint32_t size) const = 0;
		virtual void EndEvent() = 0;
		virtual void ExecuteIndirect(const CommandLayout* pCommandlayout, uint32_t maxCommandCount,
			const Resource* pArgumentBuffer, uint64_t argumentBufferOffset = 0,
			const Resource* pCountBuffer = nullptr, uint64_t countBufferOffset = 0) = 0;

		virtual void Create(void* pHandle, const wchar_t* name = nullptr) = 0;

		virtual void* GetHandle() const = 0;

		using uptr = std::unique_ptr<CommandList>;
		using sptr = std::shared_ptr<CommandList>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Command queue
	//--------------------------------------------------------------------------------------
	class DLL_INTERFACE CommandQueue
	{
	public:
		//CommandQueue();
		virtual ~CommandQueue() {};

		virtual bool Create(const Device* pDevice, CommandListType type, CommandQueueFlag flags,
			int32_t priority = 0, uint32_t nodeMask = 0, const wchar_t* name = nullptr) = 0;

		virtual bool SubmitCommandLists(uint32_t numCommandLists, const CommandList* const* ppCommandLists,
			const Semaphore* pWaits = nullptr, uint32_t numWaits = 0,
			const Semaphore* pSignals = nullptr, uint32_t numSignals = 0) = 0;
		virtual bool SubmitCommandList(const CommandList* pCommandList,
			const Semaphore* pWaits = nullptr, uint32_t numWaits = 0,
			const Semaphore* pSignals = nullptr, uint32_t numSignals = 0) = 0;
		virtual bool Wait(const Fence* pFence, uint64_t value) = 0;
		virtual bool Signal(const Fence* pFence, uint64_t value) = 0;

		virtual void ExecuteCommandLists(uint32_t numCommandLists, const CommandList* const* ppCommandLists) = 0;
		virtual void ExecuteCommandList(const CommandList* pCommandList) = 0;

		virtual void* GetHandle() const = 0;

		using uptr = std::unique_ptr<CommandQueue>;
		using sptr = std::shared_ptr<CommandQueue>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Swap chain
	//--------------------------------------------------------------------------------------
	class Resource;

	class DLL_INTERFACE SwapChain
	{
	public:
		//SwapChain();
		virtual ~SwapChain() {};

		virtual bool Create(void* pFactory, void* hWnd, const CommandQueue* pCommandQueue,
			uint8_t bufferCount, uint32_t width, uint32_t height, Format format,
			uint8_t sampleCount = 1) = 0;
		virtual bool Present(uint8_t syncInterval = 0, uint32_t flags = 0) = 0;
		virtual bool GetBuffer(uint8_t buffer, Resource* pResource) const = 0;

		virtual uint32_t ResizeBuffers(uint8_t bufferCount, uint32_t width,
			uint32_t height, Format format, uint8_t flag = 0) = 0;

		virtual uint8_t GetCurrentBackBufferIndex() const = 0;

		virtual void* GetHandle() const = 0;

		using uptr = std::unique_ptr<SwapChain>;
		using sptr = std::shared_ptr<SwapChain>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Resource
	//--------------------------------------------------------------------------------------
	class DLL_INTERFACE Resource
	{
	public:
		//Resource();
		virtual ~Resource() {};

		virtual uint32_t SetBarrier(ResourceBarrier* pBarriers, ResourceState dstState,
			uint32_t numBarriers = 0, uint32_t subresource = BARRIER_ALL_SUBRESOURCES,
			BarrierFlag flags = BarrierFlag::NONE) = 0;

		virtual ResourceBarrier	Transition(ResourceState dstState, uint32_t subresource = BARRIER_ALL_SUBRESOURCES,
			BarrierFlag flag = BarrierFlag::NONE) = 0;
		virtual ResourceState	GetResourceState(uint32_t subresource = 0) const = 0;

		virtual uint32_t GetWidth() const = 0;

		virtual uint64_t GetVirtualAddress(int offset = 0) const = 0;

		virtual void Create(void* pDeviceHandle, void* pResourceHandle, const wchar_t* name = nullptr) = 0;

		virtual void* GetHandle() const = 0;

		using uptr = std::unique_ptr<Resource>;
		using sptr = std::shared_ptr<Resource>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Constant buffer
	//--------------------------------------------------------------------------------------
	class DLL_INTERFACE ConstantBuffer :
		public virtual Resource
	{
	public:
		//ConstantBuffer();
		virtual ~ConstantBuffer() {};

		virtual bool Create(const Device* pDevice, size_t byteWidth, uint32_t numCBVs = 1,
			const size_t* offsets = nullptr, MemoryType memoryType = MemoryType::UPLOAD,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr) = 0;
		virtual bool Upload(CommandList* pCommandList, Resource* pUploader, const void* pData,
			size_t size, uint32_t cbvIndex = 0, ResourceState srcState = ResourceState::COMMON,
			ResourceState dstState = ResourceState::COMMON) = 0;

		virtual void* Map(uint32_t cbvIndex = 0) = 0;
		virtual void Unmap() = 0;

		virtual const Descriptor& GetCBV(uint32_t index = 0) const = 0;
		virtual uint32_t GetCBVOffset(uint32_t index) const = 0;

		using uptr = std::unique_ptr<ConstantBuffer>;
		using sptr = std::shared_ptr<ConstantBuffer>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Resource base
	//--------------------------------------------------------------------------------------
	class DLL_INTERFACE ShaderResource :
		public virtual Resource
	{
	public:
		//ShaderResource();
		virtual ~ShaderResource() {};

		virtual const Descriptor& GetSRV(uint32_t index = 0) const = 0;

		virtual Format GetFormat() const = 0;

		using uptr = std::unique_ptr<ShaderResource>;
		using sptr = std::shared_ptr<ShaderResource>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// 2D Texture
	//--------------------------------------------------------------------------------------
	class DLL_INTERFACE Texture2D :
		public virtual ShaderResource
	{
	public:
		//Texture2D();
		virtual ~Texture2D() {};

		virtual bool Create(const Device* pDevice, uint32_t width, uint32_t height, Format format,
			uint32_t arraySize = 1, ResourceFlag resourceFlags = ResourceFlag::NONE,
			uint8_t numMips = 1, uint8_t sampleCount = 1, bool isCubeMap = false,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr) = 0;
		virtual bool Upload(CommandList* pCommandList, Resource* pUploader,
			const SubresourceData* pSubresourceData, uint32_t numSubresources = 1,
			ResourceState dstState = ResourceState::COMMON, uint32_t firstSubresource = 0) = 0;
		virtual bool Upload(CommandList* pCommandList, Resource* pUploader, const void* pData,
			uint8_t stride = sizeof(float), ResourceState dstState = ResourceState::COMMON) = 0;
		virtual bool CreateSRVs(uint32_t arraySize, Format format = Format::UNKNOWN, uint8_t numMips = 1,
			uint8_t sampleCount = 1, bool isCubeMap = false) = 0;
		virtual bool CreateSRVLevels(uint32_t arraySize, uint8_t numMips, Format format = Format::UNKNOWN,
			uint8_t sampleCount = 1, bool isCubeMap = false) = 0;
		virtual bool CreateUAVs(uint32_t arraySize, Format format = Format::UNKNOWN, uint8_t numMips = 1,
			std::vector<Descriptor>* pUavs = nullptr) = 0;

		virtual uint32_t SetBarrier(ResourceBarrier* pBarriers, ResourceState dstState,
			uint32_t numBarriers = 0, uint32_t subresource = BARRIER_ALL_SUBRESOURCES,
			BarrierFlag flags = BarrierFlag::NONE) = 0;
		virtual uint32_t SetBarrier(ResourceBarrier* pBarriers, uint8_t mipLevel, ResourceState dstState,
			uint32_t numBarriers = 0, uint32_t slice = 0, BarrierFlag flags = BarrierFlag::NONE) = 0;

		virtual void Blit(const CommandList* pCommandList, uint32_t groupSizeX, uint32_t groupSizeY,
			uint32_t groupSizeZ, const DescriptorTable& uavSrvTable, uint32_t uavSrvSlot = 0,
			uint8_t mipLevel = 0, const DescriptorTable& srvTable = nullptr, uint32_t srvSlot = 0,
			const DescriptorTable& samplerTable = nullptr, uint32_t samplerSlot = 1,
			const Pipeline& pipeline = nullptr) = 0;

		virtual uint32_t Blit(const CommandList* pCommandList, ResourceBarrier* pBarriers, uint32_t groupSizeX,
			uint32_t groupSizeY, uint32_t groupSizeZ, uint8_t mipLevel, int8_t srcMipLevel,
			ResourceState srcState, const DescriptorTable& uavSrvTable, uint32_t uavSrvSlot = 0,
			uint32_t numBarriers = 0, const DescriptorTable& srvTable = nullptr,
			uint32_t srvSlot = 0, uint32_t baseSlice = 0, uint32_t numSlices = 0) = 0;
		virtual uint32_t GenerateMips(const CommandList* pCommandList, ResourceBarrier* pBarriers, uint32_t groupSizeX,
			uint32_t groupSizeY, uint32_t groupSizeZ, ResourceState dstState, const PipelineLayout& pipelineLayout,
			const Pipeline& pipeline, const DescriptorTable* pUavSrvTables, uint32_t uavSrvSlot = 0,
			const DescriptorTable& samplerTable = nullptr, uint32_t samplerSlot = 1, uint32_t numBarriers = 0,
			const DescriptorTable* pSrvTables = nullptr, uint32_t srvSlot = 0, uint8_t baseMip = 1,
			uint8_t numMips = 0, uint32_t baseSlice = 0, uint32_t numSlices = 0) = 0;

		virtual const Descriptor& GetUAV(uint8_t index = 0) const = 0;
		virtual const Descriptor& GetPackedUAV(uint8_t index = 0) const = 0;
		virtual const Descriptor& GetSRVLevel(uint8_t level) const = 0;

		virtual uint32_t	GetHeight() const = 0;
		virtual uint32_t	GetArraySize() const = 0;
		virtual uint8_t		GetNumMips() const = 0;

		Texture2D* AsTexture2D();

		using uptr = std::unique_ptr<Texture2D>;
		using sptr = std::shared_ptr<Texture2D>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Render target
	//--------------------------------------------------------------------------------------
	class DLL_INTERFACE RenderTarget :
		public virtual Texture2D
	{
	public:
		//RenderTarget();
		virtual ~RenderTarget() {};

		// Create() will create multiple RTVs (1 slice per RTV)
		virtual bool Create(const Device* pDevice, uint32_t width, uint32_t height, Format format,
			uint32_t arraySize = 1, ResourceFlag resourceFlags = ResourceFlag::NONE,
			uint8_t numMips = 1, uint8_t sampleCount = 1, const float* pClearColor = nullptr,
			bool isCubeMap = false, MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr) = 0;
		// CreateArray() will create a single array RTV of n slices
		virtual bool CreateArray(const Device* pDevice, uint32_t width, uint32_t height, uint32_t arraySize,
			Format format, ResourceFlag resourceFlags = ResourceFlag::NONE, uint8_t numMips = 1,
			uint8_t sampleCount = 1, const float* pClearColor = nullptr, bool isCubeMap = false,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr) = 0;
		virtual bool CreateFromSwapChain(const Device* pDevice, const SwapChain* pSwapChain, uint32_t bufferIndex) = 0;

		virtual void Blit(const CommandList* pCommandList, const DescriptorTable& srcSrvTable,
			uint32_t srcSlot = 0, uint8_t mipLevel = 0, uint32_t baseSlice = 0,
			uint32_t numSlices = 0, const DescriptorTable& samplerTable = nullptr,
			uint32_t samplerSlot = 1, const Pipeline& pipeline = nullptr,
			uint32_t offsetForSliceId = 0, uint32_t cbSlot = 2) = 0;

		virtual uint32_t Blit(const CommandList* pCommandList, ResourceBarrier* pBarriers, uint8_t mipLevel,
			int8_t srcMipLevel, ResourceState srcState, const DescriptorTable& srcSrvTable,
			uint32_t srcSlot = 0, uint32_t numBarriers = 0, uint32_t baseSlice = 0, uint32_t numSlices = 0,
			uint32_t offsetForSliceId = 0, uint32_t cbSlot = 2) = 0;
		virtual uint32_t GenerateMips(const CommandList* pCommandList, ResourceBarrier* pBarriers, ResourceState dstState,
			const PipelineLayout& pipelineLayout, const Pipeline& pipeline, const DescriptorTable* pSrcSrvTables,
			uint32_t srcSlot = 0, const DescriptorTable& samplerTable = nullptr, uint32_t samplerSlot = 1,
			uint32_t numBarriers = 0, uint8_t baseMip = 1, uint8_t numMips = 0, uint32_t baseSlice = 0,
			uint32_t numSlices = 0, uint32_t offsetForSliceId = 0, uint32_t cbSlot = 2) = 0;

		virtual const Descriptor& GetRTV(uint32_t slice = 0, uint8_t mipLevel = 0) const = 0;

		using uptr = std::unique_ptr<RenderTarget>;
		using sptr = std::shared_ptr<RenderTarget>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Depth stencil
	//--------------------------------------------------------------------------------------
	class DLL_INTERFACE DepthStencil :
		public virtual Texture2D
	{
	public:
		//DepthStencil();
		virtual ~DepthStencil() {};

		virtual bool Create(const Device* pDevice, uint32_t width, uint32_t height,
			Format format = Format::UNKNOWN, ResourceFlag resourceFlags = ResourceFlag::NONE,
			uint32_t arraySize = 1, uint8_t numMips = 1, uint8_t sampleCount = 1,
			float clearDepth = 1.0f, uint8_t clearStencil = 0, bool isCubeMap = false,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr) = 0;
		virtual bool CreateArray(const Device* pDevice, uint32_t width, uint32_t height, uint32_t arraySize,
			Format format = Format::UNKNOWN, ResourceFlag resourceFlags = ResourceFlag::NONE,
			uint8_t numMips = 1, uint8_t sampleCount = 1, float clearDepth = 1.0f,
			uint8_t clearStencil = 0, bool isCubeMap = false, MemoryFlag memoryFlags = MemoryFlag::NONE,
			const wchar_t* name = nullptr) = 0;

		virtual const Descriptor& GetDSV(uint32_t slice = 0, uint8_t mipLevel = 0) const = 0;
		virtual const Descriptor& GetReadOnlyDSV(uint32_t slice = 0, uint8_t mipLevel = 0) const = 0;
		virtual const Descriptor& GetStencilSRV() const = 0;

		virtual uint32_t	GetArraySize() const = 0;
		virtual uint8_t		GetNumMips() const = 0;

		using uptr = std::unique_ptr<DepthStencil>;
		using sptr = std::shared_ptr<DepthStencil>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// 3D Texture
	//--------------------------------------------------------------------------------------
	class DLL_INTERFACE Texture3D :
		public virtual Texture2D
	{
	public:
		//Texture3D();
		virtual ~Texture3D() {};

		virtual bool Create(const Device* pDevice, uint32_t width, uint32_t height, uint32_t depth,
			Format format, ResourceFlag resourceFlags = ResourceFlag::NONE, uint8_t numMips = 1,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr) = 0;
		virtual bool CreateSRVs(Format format = Format::UNKNOWN, uint8_t numMips = 1) = 0;
		virtual bool CreateSRVLevels(uint8_t numMips, Format format = Format::UNKNOWN) = 0;
		virtual bool CreateUAVs(Format format = Format::UNKNOWN, uint8_t numMips = 1,
			std::vector<Descriptor>* pUavs = nullptr) = 0;

		virtual uint32_t GetDepth() const = 0;

		using uptr = std::unique_ptr<Texture3D>;
		using sptr = std::shared_ptr<Texture3D>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Raw buffer
	//--------------------------------------------------------------------------------------
	class DLL_INTERFACE RawBuffer :
		public virtual ShaderResource
	{
	public:
		//RawBuffer();
		virtual ~RawBuffer() {};

		virtual bool Create(const Device* pDevice, size_t byteWidth, ResourceFlag resourceFlags = ResourceFlag::NONE,
			MemoryType memoryType = MemoryType::DEFAULT, uint32_t numSRVs = 1,
			const uint32_t* firstSRVElements = nullptr, uint32_t numUAVs = 1,
			const uint32_t* firstUAVElements = nullptr, MemoryFlag memoryFlags = MemoryFlag::NONE,
			const wchar_t* name = nullptr) = 0;
		virtual bool Upload(CommandList* pCommandList, Resource* pUploader, const void* pData, size_t size,
			uint32_t descriptorIndex = 0, ResourceState dstState = ResourceState::COMMON) = 0;
		virtual bool CreateSRVs(size_t byteWidth, const uint32_t* firstElements = nullptr,
			uint32_t numDescriptors = 1) = 0;
		virtual bool CreateUAVs(size_t byteWidth, const uint32_t* firstElements = nullptr,
			uint32_t numDescriptors = 1) = 0;

		virtual const Descriptor& GetUAV(uint32_t index = 0) const = 0;

		virtual void* Map(uint32_t descriptorIndex = 0, uintptr_t readBegin = 0, uintptr_t readEnd = 0) = 0;
		virtual void* Map(const Range* pReadRange, uint32_t descriptorIndex = 0) = 0;
		virtual void Unmap() = 0;

		using uptr = std::unique_ptr<RawBuffer>;
		using sptr = std::shared_ptr<RawBuffer>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Structured buffer
	//--------------------------------------------------------------------------------------
	class DLL_INTERFACE StructuredBuffer :
		public virtual RawBuffer
	{
	public:
		//StructuredBuffer();
		virtual ~StructuredBuffer() {};

		virtual bool Create(const Device* pDevice, uint32_t numElements, uint32_t stride,
			ResourceFlag resourceFlags = ResourceFlag::NONE, MemoryType memoryType = MemoryType::DEFAULT,
			uint32_t numSRVs = 1, const uint32_t* firstSRVElements = nullptr,
			uint32_t numUAVs = 1, const uint32_t* firstUAVElements = nullptr,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr,
			const size_t* counterOffsetsInBytes = nullptr) = 0;

		virtual bool CreateSRVs(uint32_t numElements, uint32_t stride,
			const uint32_t* firstElements = nullptr, uint32_t numDescriptors = 1) = 0;
		virtual bool CreateUAVs(uint32_t numElements, uint32_t stride,
			const uint32_t* firstElements = nullptr, uint32_t numDescriptors = 1,
			const size_t* counterOffsetsInBytes = nullptr) = 0;

		virtual void SetCounter(const Resource::sptr& counter) = 0;
		virtual Resource::sptr GetCounter() const = 0;

		using uptr = std::unique_ptr<StructuredBuffer>;
		using sptr = std::shared_ptr<StructuredBuffer>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Typed buffer
	//--------------------------------------------------------------------------------------
	class DLL_INTERFACE TypedBuffer :
		public virtual RawBuffer
	{
	public:
		//TypedBuffer();
		virtual ~TypedBuffer() {};

		virtual bool Create(const Device* pDevice, uint32_t numElements, uint32_t stride, Format format,
			ResourceFlag resourceFlags = ResourceFlag::NONE, MemoryType memoryType = MemoryType::DEFAULT,
			uint32_t numSRVs = 1, const uint32_t* firstSRVElements = nullptr,
			uint32_t numUAVs = 1, const uint32_t* firstUAVElements = nullptr,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr) = 0;

		virtual bool CreateSRVs(uint32_t numElements, Format format, uint32_t stride,
			const uint32_t* firstElements = nullptr, uint32_t numDescriptors = 1) = 0;
		virtual bool CreateUAVs(uint32_t numElements, Format format, uint32_t stride,
			const uint32_t* firstElements = nullptr, uint32_t numDescriptors = 1,
			std::vector<Descriptor>* pUavs = nullptr) = 0;

		virtual const Descriptor& GetPackedUAV(uint32_t index = 0) const = 0;

		using uptr = std::unique_ptr<TypedBuffer>;
		using sptr = std::shared_ptr<TypedBuffer>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Vertex buffer
	//--------------------------------------------------------------------------------------
	class DLL_INTERFACE VertexBuffer :
		public virtual StructuredBuffer
	{
	public:
		//VertexBuffer();
		virtual ~VertexBuffer() {};

		virtual bool Create(const Device* pDevice, uint32_t numVertices, uint32_t stride,
			ResourceFlag resourceFlags = ResourceFlag::NONE, MemoryType memoryType = MemoryType::DEFAULT,
			uint32_t numVBVs = 1, const uint32_t* firstVertices = nullptr,
			uint32_t numSRVs = 1, const uint32_t* firstSRVElements = nullptr,
			uint32_t numUAVs = 1, const uint32_t* firstUAVElements = nullptr,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr) = 0;
		virtual bool CreateAsRaw(const Device* pDevice, uint32_t numVertices, uint32_t stride,
			ResourceFlag resourceFlags = ResourceFlag::NONE, MemoryType memoryType = MemoryType::DEFAULT,
			uint32_t numVBVs = 1, const uint32_t* firstVertices = nullptr,
			uint32_t numSRVs = 1, const uint32_t* firstSRVElements = nullptr,
			uint32_t numUAVs = 1, const uint32_t* firstUAVElements = nullptr,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr) = 0;

		virtual const VertexBufferView& GetVBV(uint32_t index = 0) const = 0;

		using uptr = std::unique_ptr<VertexBuffer>;
		using sptr = std::shared_ptr<VertexBuffer>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Index buffer
	//--------------------------------------------------------------------------------------
	class DLL_INTERFACE IndexBuffer :
		public virtual TypedBuffer
	{
	public:
		//IndexBuffer();
		virtual ~IndexBuffer() {};

		virtual bool Create(const Device* pDevice, size_t byteWidth, Format format = Format::R32_UINT,
			ResourceFlag resourceFlags = ResourceFlag::DENY_SHADER_RESOURCE,
			MemoryType memoryType = MemoryType::DEFAULT,
			uint32_t numIBVs = 1, const size_t* offsets = nullptr,
			uint32_t numSRVs = 1, const uint32_t* firstSRVElements = nullptr,
			uint32_t numUAVs = 1, const uint32_t* firstUAVElements = nullptr,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr) = 0;

		virtual const IndexBufferView& GetIBV(uint32_t index = 0) const = 0;

		using uptr = std::unique_ptr<IndexBuffer>;
		using sptr = std::shared_ptr<IndexBuffer>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Descriptor
	//--------------------------------------------------------------------------------------
	class DescriptorTableCache;

	namespace Util
	{
		class DLL_INTERFACE DescriptorTable
		{
		public:
			//DescriptorTable();
			virtual ~DescriptorTable() {};

			virtual void SetDescriptors(uint32_t start, uint32_t num, const Descriptor* srcDescriptors,
				uint8_t descriptorPoolIndex = 0) = 0;
			virtual void SetSamplers(uint32_t start, uint32_t num, const SamplerPreset* presets,
				DescriptorTableCache* pDescriptorTableCache, uint8_t descriptorPoolIndex = 0) = 0;

			virtual XUSG::DescriptorTable CreateCbvSrvUavTable(DescriptorTableCache* pDescriptorTableCache,
				const XUSG::DescriptorTable& table = nullptr) = 0;
			virtual XUSG::DescriptorTable GetCbvSrvUavTable(DescriptorTableCache* pDescriptorTableCache,
				const XUSG::DescriptorTable& table = nullptr) = 0;

			virtual XUSG::DescriptorTable CreateSamplerTable(DescriptorTableCache* pDescriptorTableCache,
				const XUSG::DescriptorTable& table = nullptr) = 0;
			virtual XUSG::DescriptorTable GetSamplerTable(DescriptorTableCache* pDescriptorTableCache,
				const XUSG::DescriptorTable& table = nullptr) = 0;

			virtual Framebuffer CreateFramebuffer(DescriptorTableCache* pDescriptorTableCache,
				const Descriptor* pDsv = nullptr, const Framebuffer* pFramebuffer = nullptr) = 0;
			virtual Framebuffer GetFramebuffer(DescriptorTableCache* pDescriptorTableCache,
				const Descriptor* pDsv = nullptr, const Framebuffer* pFramebuffer = nullptr) = 0;

			virtual const std::string& GetKey() const = 0;

			using uptr = std::unique_ptr<DescriptorTable>;
			using sptr = std::shared_ptr<DescriptorTable>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
		};
	}

	class DLL_INTERFACE DescriptorTableCache
	{
	public:
		//DescriptorTableCache();
		//DescriptorTableCache(const Device* pDevice, const wchar_t* name = nullptr);
		virtual ~DescriptorTableCache() {};

		virtual void SetDevice(const Device* pDevice) = 0;
		virtual void SetName(const wchar_t* name) = 0;
		virtual void ResetDescriptorPool(DescriptorPoolType type, uint8_t index) = 0;

		virtual bool AllocateDescriptorPool(DescriptorPoolType type, uint32_t numDescriptors, uint8_t index = 0) = 0;

		virtual DescriptorTable CreateCbvSrvUavTable(const Util::DescriptorTable* pUtil, const DescriptorTable& table = nullptr) = 0;
		virtual DescriptorTable GetCbvSrvUavTable(const Util::DescriptorTable* pUtil, const DescriptorTable& table = nullptr) = 0;

		virtual DescriptorTable CreateSamplerTable(const Util::DescriptorTable* pUtil, const DescriptorTable& table = nullptr) = 0;
		virtual DescriptorTable GetSamplerTable(const Util::DescriptorTable* pUtil, const DescriptorTable& table = nullptr) = 0;

		virtual Framebuffer CreateFramebuffer(const Util::DescriptorTable* pUtil,
			const Descriptor* pDsv = nullptr, const Framebuffer* pFramebuffer = nullptr) = 0;
		virtual Framebuffer GetFramebuffer(const Util::DescriptorTable* pUtil,
			const Descriptor* pDsv = nullptr, const Framebuffer* pFramebuffer = nullptr) = 0;

		virtual DescriptorPool GetDescriptorPool(DescriptorPoolType type, uint8_t index = 0) const = 0;

		virtual const Sampler& GetSampler(SamplerPreset preset) = 0;

		virtual uint32_t GetDescriptorStride(DescriptorPoolType type) const = 0;

		using uptr = std::unique_ptr<DescriptorTableCache>;
		using sptr = std::shared_ptr<DescriptorTableCache>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
		static uptr MakeUnique(const Device* pDevice, const wchar_t* name = nullptr, API api = API::DIRECTX_12);
		static sptr MakeShared(const Device* pDevice, const wchar_t* name = nullptr, API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Shader reflector
	//--------------------------------------------------------------------------------------
	class DLL_INTERFACE Reflector
	{
	public:
		//Reflector();
		virtual ~Reflector() {};

		virtual bool SetShader(const Blob& shader) = 0;
		virtual bool IsValid() const = 0;
		virtual uint32_t GetResourceBindingPointByName(const char* name, uint32_t defaultVal = UINT32_MAX) const = 0;

		using uptr = std::unique_ptr<Reflector>;
		using sptr = std::shared_ptr<Reflector>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Shader
	//--------------------------------------------------------------------------------------
	class DLL_INTERFACE ShaderPool
	{
	public:
		//ShaderPool();
		virtual ~ShaderPool() {};

		virtual void SetShader(Shader::Stage stage, uint32_t index, const Blob& shader) = 0;
		virtual void SetShader(Shader::Stage stage, uint32_t index, const Blob& shader, const Reflector::sptr& reflector) = 0;
		virtual void SetReflector(Shader::Stage stage, uint32_t index, const Reflector::sptr& reflector) = 0;

		virtual Blob CreateShader(Shader::Stage stage, uint32_t index, const std::wstring& fileName) = 0;
		virtual Blob GetShader(Shader::Stage stage, uint32_t index) const = 0;
		virtual Reflector::sptr GetReflector(Shader::Stage stage, uint32_t index) const = 0;

		using uptr = std::unique_ptr<ShaderPool>;
		using sptr = std::shared_ptr<ShaderPool>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Pipeline layout
	//--------------------------------------------------------------------------------------
	class PipelineLayoutCache;

	namespace Util
	{
		class DLL_INTERFACE PipelineLayout
		{
		public:
			//PipelineLayout();
			virtual ~PipelineLayout() {};

			virtual void SetShaderStage(uint32_t index, Shader::Stage stage) = 0;
			virtual void SetRange(uint32_t index, DescriptorType type, uint32_t num, uint32_t baseBinding,
				uint32_t space = 0, DescriptorFlag flags = DescriptorFlag::NONE) = 0;
			virtual void SetConstants(uint32_t index, uint32_t num32BitValues, uint32_t binding,
				uint32_t space = 0, Shader::Stage stage = Shader::Stage::ALL) = 0;
			virtual void SetRootSRV(uint32_t index, uint32_t binding, uint32_t space = 0,
				DescriptorFlag flags = DescriptorFlag::NONE, Shader::Stage stage = Shader::Stage::ALL) = 0;
			virtual void SetRootUAV(uint32_t index, uint32_t binding, uint32_t space = 0,
				DescriptorFlag flags = DescriptorFlag::DATA_STATIC_WHILE_SET_AT_EXECUTE, Shader::Stage stage = Shader::Stage::ALL) = 0;
			virtual void SetRootCBV(uint32_t index, uint32_t binding, uint32_t space = 0, Shader::Stage stage = Shader::Stage::ALL) = 0;
			virtual void SetStaticSamplers(const Sampler* pSamplers, uint32_t num, uint32_t baseBinding,
				uint32_t space = 0, Shader::Stage stage = Shader::Stage::ALL) = 0;

			virtual XUSG::PipelineLayout CreatePipelineLayout(PipelineLayoutCache* pPipelineLayoutCache, PipelineLayoutFlag flags,
				const wchar_t* name = nullptr) = 0;
			virtual XUSG::PipelineLayout GetPipelineLayout(PipelineLayoutCache* pPipelineLayoutCache, PipelineLayoutFlag flags,
				const wchar_t* name = nullptr) = 0;

			virtual DescriptorTableLayout CreateDescriptorTableLayout(uint32_t index, PipelineLayoutCache* pPipelineLayoutCache) const = 0;
			virtual DescriptorTableLayout GetDescriptorTableLayout(uint32_t index, PipelineLayoutCache* pPipelineLayoutCache) const = 0;

			virtual const std::vector<std::string>& GetDescriptorTableLayoutKeys() const = 0;
			virtual std::string& GetPipelineLayoutKey(PipelineLayoutCache* pPipelineLayoutCache) = 0;

			using uptr = std::unique_ptr<PipelineLayout>;
			using sptr = std::shared_ptr<PipelineLayout>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
			static uptr CloneUnique(const PipelineLayout* pSrc, API api = API::DIRECTX_12);
			static sptr CloneShared(const PipelineLayout* pSrc, API api = API::DIRECTX_12);
		};
	}

	class DLL_INTERFACE PipelineLayoutCache
	{
	public:
		//PipelineLayoutCache();
		//PipelineLayoutCache(const Device* pDevice) = 0;
		virtual ~PipelineLayoutCache() {};

		virtual void SetDevice(const Device* pDevice) = 0;
		virtual void SetPipelineLayout(const std::string& key, const PipelineLayout& pipelineLayout) = 0;

		virtual PipelineLayout CreatePipelineLayout(Util::PipelineLayout* pUtil, PipelineLayoutFlag flags,
			const wchar_t* name = nullptr) = 0;
		virtual PipelineLayout GetPipelineLayout(Util::PipelineLayout* pUtil, PipelineLayoutFlag flags,
			const wchar_t* name = nullptr, bool create = true) = 0;

		virtual DescriptorTableLayout CreateDescriptorTableLayout(uint32_t index, const Util::PipelineLayout* pUtil) = 0;
		virtual DescriptorTableLayout GetDescriptorTableLayout(uint32_t index, const Util::PipelineLayout* pUtil) = 0;

		using uptr = std::unique_ptr<PipelineLayoutCache>;
		using sptr = std::shared_ptr<PipelineLayoutCache>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
		static uptr MakeUnique(const Device* pDevice, API api = API::DIRECTX_12);
		static sptr MakeShared(const Device* pDevice, API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Graphics pipeline state
	//--------------------------------------------------------------------------------------
	namespace Graphics
	{
		enum BlendPreset : uint8_t
		{
			DEFAULT_OPAQUE,
			PREMULTIPLITED,
			ADDTIVE,
			NON_PRE_MUL,
			NON_PREMUL_RT0,
			ALPHA_TO_COVERAGE,
			ACCUMULATIVE,
			AUTO_NON_PREMUL,
			ZERO_ALPHA_PREMUL,
			MULTIPLITED,
			WEIGHTED_PREMUL,
			WEIGHTED_PREMUL_PER_RT,
			WEIGHTED_PER_RT,
			SELECT_MIN,
			SELECT_MAX,

			NUM_BLEND_PRESET
		};

		enum RasterizerPreset : uint8_t
		{
			CULL_BACK,
			CULL_NONE,
			CULL_FRONT,
			FILL_WIREFRAME,

			NUM_RS_PRESET
		};

		enum DepthStencilPreset : uint8_t
		{
			DEFAULT_LESS,
			DEPTH_STENCIL_NONE,
			DEPTH_READ_LESS,
			DEPTH_READ_LESS_EQUAL,
			DEPTH_READ_EQUAL,

			NUM_DS_PRESET
		};

		struct RenderTargetBlend
		{
			bool BlendEnable;
			bool LogicOpEnable;
			BlendFactor SrcBlend;
			BlendFactor DestBlend;
			BlendOperator BlendOp;
			BlendFactor SrcBlendAlpha;
			BlendFactor DestBlendAlpha;
			BlendOperator BlendOpAlpha;
			LogicOperator LogicOp;
			ColorWrite WriteMask;
		};

		struct Blend
		{
			bool AlphaToCoverageEnable;
			bool IndependentBlendEnable;
			RenderTargetBlend RenderTargets[8];
		};

		struct Rasterizer
		{
			FillMode Fill;
			CullMode Cull;
			bool FrontCounterClockwise;
			int DepthBias;
			float DepthBiasClamp;
			float SlopeScaledDepthBias;
			bool DepthClipEnable;
			bool MultisampleEnable;
			bool AntialiasedLineEnable;
			uint8_t ForcedSampleCount;
			bool ConservativeRaster;
		};

		struct DepthStencilOp
		{
			StencilOp StencilFailOp;
			StencilOp StencilDepthFailOp;
			StencilOp StencilPassOp;
			ComparisonFunc StencilFunc;
		};

		struct DepthStencil
		{
			bool DepthEnable;
			bool DepthWriteMask;
			ComparisonFunc DepthFunc;
			bool StencilEnable;
			uint8_t StencilReadMask;
			uint8_t StencilWriteMask;
			DepthStencilOp FrontFace;
			DepthStencilOp BackFace;
		};

		class PipelineCache;
		
		class DLL_INTERFACE State
		{
		public:
			//State();
			virtual ~State() {};

			virtual void SetPipelineLayout(const PipelineLayout& layout) = 0;
			virtual void SetShader(Shader::Stage stage, const Blob& shader) = 0;
			virtual void SetCachedPipeline(const void* pCachedBlob, size_t size) = 0;
			virtual void SetNodeMask(uint32_t nodeMask) = 0;

			virtual void OMSetBlendState(const Blend* pBlend, uint32_t sampleMask = UINT_MAX) = 0;
			virtual void RSSetState(const Rasterizer* pRasterizer) = 0;
			virtual void DSSetState(const DepthStencil* DepthStencil) = 0;

			virtual void OMSetBlendState(BlendPreset preset, PipelineCache* pPipelineCache,
				uint8_t numColorRTs = 1, uint32_t sampleMask = UINT_MAX) = 0;
			virtual void RSSetState(RasterizerPreset preset, PipelineCache* pPipelineCache) = 0;
			virtual void DSSetState(DepthStencilPreset preset, PipelineCache* pPipelineCache) = 0;

			virtual void IASetInputLayout(const InputLayout* pLayout) = 0;
			virtual void IASetPrimitiveTopologyType(PrimitiveTopologyType type) = 0;
			virtual void IASetIndexBufferStripCutValue(IBStripCutValue ibStripCutValue) = 0;

			virtual void OMSetNumRenderTargets(uint8_t n) = 0;
			virtual void OMSetRTVFormat(uint8_t i, Format format) = 0;
			virtual void OMSetRTVFormats(const Format* formats, uint8_t n) = 0;
			virtual void OMSetDSVFormat(Format format) = 0;
			virtual void OMSetSample(uint8_t count, uint8_t quality = 0) = 0;

			virtual Pipeline CreatePipeline(PipelineCache* pPipelineCache, const wchar_t* name = nullptr) const = 0;
			virtual Pipeline GetPipeline(PipelineCache* pPipelineCache, const wchar_t* name = nullptr) const = 0;

			virtual const std::string& GetKey() const = 0;

			using uptr = std::unique_ptr<State>;
			using sptr = std::shared_ptr<State>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
		};

		class DLL_INTERFACE PipelineCache
		{
		public:
			//PipelineCache();
			//PipelineCache(const Device* pDevice);
			virtual ~PipelineCache() {};

			virtual void SetDevice(const Device* pDevice) = 0;
			virtual void SetPipeline(const std::string& key, const Pipeline& pipeline) = 0;

			virtual void SetInputLayout(uint32_t index, const InputElement* pElements, uint32_t numElements) = 0;
			virtual const InputLayout* GetInputLayout(uint32_t index) const = 0;
			virtual const InputLayout* CreateInputLayout(const InputElement* pElements, uint32_t numElements) = 0;

			virtual Pipeline CreatePipeline(const State* pState, const wchar_t* name = nullptr) = 0;
			virtual Pipeline GetPipeline(const State* pState, const wchar_t* name = nullptr) = 0;

			virtual const Blend* GetBlend(BlendPreset preset, uint8_t numColorRTs = 1) = 0;
			virtual const Rasterizer* GetRasterizer(RasterizerPreset preset) = 0;
			virtual const DepthStencil* GetDepthStencil(DepthStencilPreset preset) = 0;

			using uptr = std::unique_ptr<PipelineCache>;
			using sptr = std::shared_ptr<PipelineCache>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
			static uptr MakeUnique(const Device* pDevice, API api = API::DIRECTX_12);
			static sptr MakeShared(const Device* pDevice, API api = API::DIRECTX_12);
		};
	}

	//--------------------------------------------------------------------------------------
	// Compute pipeline state
	//--------------------------------------------------------------------------------------
	namespace Compute
	{
		class PipelineCache;
		
		class DLL_INTERFACE State
		{
		public:
			//State();
			virtual ~State() {};

			virtual void SetPipelineLayout(const PipelineLayout& layout) = 0;
			virtual void SetShader(const Blob& shader) = 0;
			virtual void SetCachedPipeline(const void* pCachedBlob, size_t size) = 0;
			virtual void SetNodeMask(uint32_t nodeMask) = 0;

			virtual Pipeline CreatePipeline(PipelineCache* pPipelineCache, const wchar_t* name = nullptr) const = 0;
			virtual Pipeline GetPipeline(PipelineCache* pPipelineCache, const wchar_t* name = nullptr) const = 0;

			virtual const std::string& GetKey() const = 0;

			using uptr = std::unique_ptr<State>;
			using sptr = std::shared_ptr<State>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
		};

		class DLL_INTERFACE PipelineCache
		{
		public:
			//PipelineCache();
			//PipelineCache(const Device* pDevice);
			virtual ~PipelineCache() {};

			virtual void SetDevice(const Device* pDevice) = 0;
			virtual void SetPipeline(const std::string& key, const Pipeline& pipeline) = 0;

			virtual Pipeline CreatePipeline(const State* pState, const wchar_t* name = nullptr) = 0;
			virtual Pipeline GetPipeline(const State* pState, const wchar_t* name = nullptr) = 0;

			using uptr = std::unique_ptr<PipelineCache>;
			using sptr = std::shared_ptr<PipelineCache>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
			static uptr MakeUnique(const Device* pDevice, API api = API::DIRECTX_12);
			static sptr MakeShared(const Device* pDevice, API api = API::DIRECTX_12);
		};
	}

	__forceinline uint8_t Log2(uint32_t value)
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
}
