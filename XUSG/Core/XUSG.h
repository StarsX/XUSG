//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#pragma warning(disable:4250)

#ifndef XUSG_INTERFACE
#define XUSG_INTERFACE
#endif

#define XUSG_M_RETURN(x, o, m, r)		if (x) { o << m << std::endl; assert(!m); return r; }
#define XUSG_C_RETURN(x, r)				if (x) return r
#define XUSG_N_RETURN(x, r)				XUSG_C_RETURN(!(x), r)
#define XUSG_X_RETURN(x, f, r)			{ x = f; XUSG_N_RETURN(x, r); }

#define XUSG_DIV_UP(x, n)				(((x) + (n) - 1) / (n))
#define XUSG_UINT32_SIZE_OF(obj)		XUSG_DIV_UP(sizeof(obj), sizeof(uint32_t))

#define XUSG_APPEND_ALIGNED_ELEMENT		0xffffffff
#define XUSG_BARRIER_ALL_SUBRESOURCES	0xffffffff
#define XUSG_DESCRIPTOR_OFFSET_APPEND	0xffffffff
#define XUSG_NULL						0

#define XUSG_DEF_ENUM_FLAG_OPERATORS(ENUMTYPE) \
extern "C++" \
{ \
	inline constexpr ENUMTYPE operator | (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((std::underlying_type_t<ENUMTYPE>)a) | ((std::underlying_type_t<ENUMTYPE>)b)); } \
	inline ENUMTYPE &operator |= (ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((std::underlying_type_t<ENUMTYPE>&)a) |= ((std::underlying_type_t<ENUMTYPE>)b)); } \
	inline constexpr ENUMTYPE operator & (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((std::underlying_type_t<ENUMTYPE>)a) & ((std::underlying_type_t<ENUMTYPE>)b)); } \
	inline ENUMTYPE &operator &= (ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((std::underlying_type_t<ENUMTYPE>&)a) &= ((std::underlying_type_t<ENUMTYPE>)b)); } \
	inline constexpr ENUMTYPE operator ~ (ENUMTYPE a) { return ENUMTYPE(~((std::underlying_type_t<ENUMTYPE>)a)); } \
	inline constexpr ENUMTYPE operator ^ (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((std::underlying_type_t<ENUMTYPE>)a) ^ ((std::underlying_type_t<ENUMTYPE>)b)); } \
	inline ENUMTYPE &operator ^= (ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((std::underlying_type_t<ENUMTYPE>&)a) ^= ((std::underlying_type_t<ENUMTYPE>)b)); } \
}

#define XUSG_ENCODE_SRV_COMPONENT_MAPPING(src0, src1, src2, src3)	(0x1000 | \
																	(static_cast<uint16_t>(src0) << 0) | \
																	(static_cast<uint16_t>(src1) << 3) | \
																	(static_cast<uint16_t>(src2) << 6) | \
																	(static_cast<uint16_t>(src3) << 9))
#define XUSG_DEFAULT_SRV_COMPONENT_MAPPING			XUSG_ENCODE_SRV_COMPONENT_MAPPING(SrvCM::MC0, SrvCM::MC1, SrvCM::MC2, SrvCM::MC3)
#define XUSG_DEFAULT_STENCIL_SRV_COMPONENT_MAPPING	XUSG_ENCODE_SRV_COMPONENT_MAPPING(SrvCM::MC1, SrvCM::FV0, SrvCM::FV0, SrvCM::FV0)

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

		com_ptr(void* ptr) noexcept : com_ptr() { this->copy_from(static_cast<T*>(ptr)); }
	};
#else
	template <typename T>
	class com_ptr :
		public Microsoft::WRL::ComPtr<T>
	{
	public:
		using element_type = T;
		using Microsoft::WRL::ComPtr<T>::ComPtr;

		com_ptr(void* ptr = nullptr) throw() : com_ptr(static_cast<T*>(ptr)) {}

		element_type* get() const throw() { return this->Get(); }
		element_type** put() throw() { return &this->ptr_; }
	};
#endif
#endif

	// Enumerations
	enum class API
	{
		DIRECTX_12
	};

	enum class Format : uint8_t
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
		CUSTOM,
		GPU_UPLOAD
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
		TRIANGLEFAN,
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

	enum class FillMode : uint8_t
	{
		WIREFRAME,
		SOLID
	};

	enum class CullMode : uint8_t
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

	enum class TextureLayout
	{
		UNKNOWN,
		ROW_MAJOR,
		UNDEFINED_SWIZZLE,
		STANDARD_SWIZZLE
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

	enum class IndirectArgumentType : uint8_t
	{
		DRAW,
		DRAW_INDEXED,
		DISPATCH,
		VERTEX_BUFFER_VIEW,
		INDEX_BUFFER_VIEW,
		CONSTANT,
		CONSTANT_BUFFER_VIEW,
		SHADER_RESOURCE_VIEW,
		UNORDERED_ACCESS_VIEW,
		DISPATCH_RAYS,
		DISPATCH_MESH,
		INCREMENTING_CONSTANT
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

	XUSG_DEF_ENUM_FLAG_OPERATORS(MemoryFlag);

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
		VIDEO_ENCODE_REFERENCE_ONLY = (1 << 7),
		ACCELERATION_STRUCTURE = ALLOW_UNORDERED_ACCESS | (1 << 8),
		NEED_PACKED_UAV = ALLOW_UNORDERED_ACCESS | (1 << 9)
	};

	XUSG_DEF_ENUM_FLAG_OPERATORS(ResourceFlag);

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
		RAYTRACING_ACCELERATION_STRUCTURE = (1 << 14),
		SHADING_RATE_SOURCE = (1 << 15),

		ALL_SHADER_RESOURCE = NON_PIXEL_SHADER_RESOURCE | PIXEL_SHADER_RESOURCE,
		GENERAL_READ = VERTEX_AND_CONSTANT_BUFFER | INDEX_BUFFER | ALL_SHADER_RESOURCE | INDIRECT_ARGUMENT | COPY_SOURCE,
		PRESENT = COMMON,
		PREDICATION = INDIRECT_ARGUMENT,

		VIDEO_DECODE_READ = (1 << 16),
		VIDEO_DECODE_WRITE = (1 << 17),
		VIDEO_PROCESS_READ = (1 << 18),
		VIDEO_PROCESS_WRITE = (1 << 19),
		VIDEO_ENCODE_READ = (1 << 20),
		VIDEO_ENCODE_WRITE = (1 << 21),

		AUTO = (1u << 31)
	};

	XUSG_DEF_ENUM_FLAG_OPERATORS(ResourceState);

	enum class BarrierFlag : uint8_t
	{
		NONE = 0,
		BEGIN_ONLY = (1 << 0),
		END_ONLY = (1 << 1)
	};

	XUSG_DEF_ENUM_FLAG_OPERATORS(BarrierFlag);

	enum class DescriptorFlag : uint8_t
	{
		NONE = 0,
		DESCRIPTORS_VOLATILE = (1 << 0),
		DATA_VOLATILE = (1 << 1),
		DATA_STATIC_WHILE_SET_AT_EXECUTE = (1 << 2),
		DATA_STATIC = (1 << 3),
		DESCRIPTORS_STATIC_KEEPING_BUFFER_BOUNDS_CHECKS = (1 << 4)
	};

	XUSG_DEF_ENUM_FLAG_OPERATORS(DescriptorFlag);

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
		CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED = (1 << 10),
		SAMPLER_HEAP_DIRECTLY_INDEXED = (1 << 11)
	};

	XUSG_DEF_ENUM_FLAG_OPERATORS(PipelineLayoutFlag);

	enum class CommandQueueFlag : uint8_t
	{
		NONE = 0,
		DISABLE_GPU_TIMEOUT = (1 << 0)
	};

	XUSG_DEF_ENUM_FLAG_OPERATORS(CommandQueueFlag);

	enum class FenceFlag : uint8_t
	{
		NONE = 0,
		SHARED = (1 << 0),
		SHARED_CROSS_ADAPTER = (1 << 1),
		NON_MONITORED = (1 << 2)
	};

	XUSG_DEF_ENUM_FLAG_OPERATORS(FenceFlag);

	enum class ClearFlag : uint8_t
	{
		NONE = 0,
		DEPTH = (1 << 0),
		STENCIL = (1 << 1)
	};

	XUSG_DEF_ENUM_FLAG_OPERATORS(ClearFlag);

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
		RED = (1 << 0),
		GREEN = (1 << 1),
		BLUE = (1 << 2),
		ALPHA = (1 << 3),
		ALL = RED | GREEN | BLUE | ALPHA
	};

	XUSG_DEF_ENUM_FLAG_OPERATORS(ColorWrite);

	enum class LineRasterization : uint8_t
	{
		ALIASED,
		ALPHA_ANTIALIASED,
		QUADRILATERAL_WIDE,
		QUADRILATERAL_NARROW
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

	enum class PipelineFlag : uint8_t
	{
		NONE = 0,
		TOOL_DEBUG = (1 << 0),
		DYNAMIC_DEPTH_BIAS = (1 << 1),
		DYNAMIC_INDEX_BUFFER_STRIP_CUT = (1 << 2)
	};

	XUSG_DEF_ENUM_FLAG_OPERATORS(PipelineFlag);

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

	XUSG_DEF_ENUM_FLAG_OPERATORS(TileCopyFlag);

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

			NUM_STAGE
		};
	}

	enum DescriptorHeapType : uint8_t
	{
		CBV_SRV_UAV_HEAP,
		SAMPLER_HEAP,
		RTV_HEAP,

		NUM_DESCRIPTOR_HEAP,
		NUM_SHADER_VISIBLE_DESCRIPTOR_HEAP = SAMPLER_HEAP + 1
	};

	enum class SamplerFilter : uint8_t
	{
		MIN_MAG_MIP_POINT,
		MIN_MAG_POINT_MIP_LINEAR,
		MIN_POINT_MAG_LINEAR_MIP_POINT,
		MIN_POINT_MAG_MIP_LINEAR,
		MIN_LINEAR_MAG_MIP_POINT,
		MIN_LINEAR_MAG_POINT_MIP_LINEAR,
		MIN_MAG_LINEAR_MIP_POINT,
		MIN_MAG_MIP_LINEAR,
		ANISOTROPIC,
		COMPARISON_MIN_MAG_MIP_POINT,
		COMPARISON_MIN_MAG_POINT_MIP_LINEAR,
		COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT,
		COMPARISON_MIN_POINT_MAG_MIP_LINEAR,
		COMPARISON_MIN_LINEAR_MAG_MIP_POINT,
		COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
		COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
		COMPARISON_MIN_MAG_MIP_LINEAR,
		COMPARISON_ANISOTROPIC,
		MINIMUM_MIN_MAG_MIP_POINT,
		MINIMUM_MIN_MAG_POINT_MIP_LINEAR,
		MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT,
		MINIMUM_MIN_POINT_MAG_MIP_LINEAR,
		MINIMUM_MIN_LINEAR_MAG_MIP_POINT,
		MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
		MINIMUM_MIN_MAG_LINEAR_MIP_POINT,
		MINIMUM_MIN_MAG_MIP_LINEAR,
		MINIMUM_ANISOTROPIC,
		MAXIMUM_MIN_MAG_MIP_POINT,
		MAXIMUM_MIN_MAG_POINT_MIP_LINEAR,
		MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT,
		MAXIMUM_MIN_POINT_MAG_MIP_LINEAR,
		MAXIMUM_MIN_LINEAR_MAG_MIP_POINT,
		MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
		MAXIMUM_MIN_MAG_LINEAR_MIP_POINT,
		MAXIMUM_MIN_MAG_MIP_LINEAR,
		MAXIMUM_ANISOTROPIC
	};

	enum class TextureAddressMode : uint8_t
	{
		WRAP,
		MIRROR,
		CLAMP,
		BORDER,
		MIRROR_ONCE
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

		MIN_LINEAR_WRAP,
		MIN_LINEAR_MIRROR,
		MIN_LINEAR_CLAMP,
		MIN_LINEAR_BORDER,

		MIN_ANISOTROPIC_WRAP,
		MIN_ANISOTROPIC_MIRROR,
		MIN_ANISOTROPIC_CLAMP,
		MIN_ANISOTROPIC_BORDER,

		MAX_LINEAR_WRAP,
		MAX_LINEAR_MIRROR,
		MAX_LINEAR_CLAMP,
		MAX_LINEAR_BORDER,

		MAX_ANISOTROPIC_WRAP,
		MAX_ANISOTROPIC_MIRROR,
		MAX_ANISOTROPIC_CLAMP,
		MAX_ANISOTROPIC_BORDER,

		NUM_SAMPLER_PRESET
	};

	typedef enum SrvComponentMapping : uint16_t
	{
		MEMORY_COMPONENT_0 = 0,
		MEMORY_COMPONENT_1 = 1,
		MEMORY_COMPONENT_2 = 2,
		MEMORY_COMPONENT_3 = 3,
		FORCE_VALUE_0 = 4,
		FORCE_VALUE_1 = 5,

		MC0 = MEMORY_COMPONENT_0,
		MC1 = MEMORY_COMPONENT_1,
		MC2 = MEMORY_COMPONENT_2,
		MC3 = MEMORY_COMPONENT_3,
		FV0 = FORCE_VALUE_0,
		FV1 = FORCE_VALUE_1
	} SrvCM;

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

	enum class SwapChainFlag : uint32_t
	{
		NONE = 0,
		NONPREROTATED = (1 << 0),
		ALLOW_MODE_SWITCH = (1 << 1),
		GDI_COMPATIBLE = (1 << 2),
		RESTRICTED_CONTENT = (1 << 3),
		RESTRICT_SHARED_RESOURCE_DRIVER = (1 << 4),
		DISPLAY_ONLY = (1 << 5),
		FRAME_LATENCY_WAITABLE_OBJECT = (1 << 6),
		FOREGROUND_LAYER = (1 << 7),
		FULLSCREEN_VIDEO = (1 << 8),
		YUV_VIDEO = (1 << 9),
		HW_PROTECTED = (1 << 10),
		ALLOW_TEARING = (1 << 11),
		RESTRICTED_TO_ALL_HOLOGRAPHIC_DISPLAYS = (1 << 12)
	};

	XUSG_DEF_ENUM_FLAG_OPERATORS(SwapChainFlag);

	enum class PresentFlag : uint32_t
	{
		NONE = 0,
		TEST = (1 << 0),
		DO_NOT_SEQUENCE = (1 << 1),
		RESTART = (1 << 2),
		DO_NOT_WAIT = (1 << 3),
		STEREO_PREFER_RIGHT = (1 << 4),
		STEREO_TEMPORARY_MONO = (1 << 5),
		RESTRICT_TO_OUTPUT = (1 << 6),
		USE_DURATION = (1 << 8),
		ALLOW_TEARING = (1 << 9)
	};

	XUSG_DEF_ENUM_FLAG_OPERATORS(PresentFlag);

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

	enum class SamplerFlag : uint8_t
	{
		NONE = 0,
		UINT_BORDER_COLOR = (1 << 0),
		NON_NORMALIZED_COORDINATES = (1 << 1)
	};

	XUSG_DEF_ENUM_FLAG_OPERATORS(SamplerFlag);

	struct Sampler
	{
		SamplerFilter Filter;
		TextureAddressMode AddressU;
		TextureAddressMode AddressV;
		TextureAddressMode AddressW;
		float MipLODBias;
		uint8_t MaxAnisotropy;
		ComparisonFunc Comparison;
		union
		{
			float BorderColor[4];
			uint32_t BorderColorU[4];
		};
		float MinLOD;
		float MaxLOD;
		SamplerFlag Flags;
	};

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

	struct Point
	{
		Point() = default;
		Point(long x, long y)
		{
			X = x;
			Y = y;
		}

		long X;
		long Y;
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
	using DescriptorTable = uint64_t;
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
	using DescriptorHeap = void*;
	using Blob = void*;
	using QueryHeap = void*;

	//--------------------------------------------------------------------------------------
	// Device
	//--------------------------------------------------------------------------------------
	class CommandAllocator;
	class CommandList;
	class CommandQueue;
	class Fence;

	class XUSG_INTERFACE Device
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

		// Create from API native handle
		virtual void Create(void* pHandle, const wchar_t* name = nullptr) = 0;

		// Get API native handle
		virtual void* GetHandle() const = 0;

		using uptr = std::unique_ptr<Device>;
		using sptr = std::shared_ptr<Device>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Fence
	//--------------------------------------------------------------------------------------
	class XUSG_INTERFACE Fence
	{
	public:
		//Fence();
		virtual ~Fence() {};

		virtual bool Create(const Device* pDevice, uint64_t initialValue,
			FenceFlag flags, const wchar_t* name = nullptr) = 0;
		virtual bool SetEventOnCompletion(uint64_t value, void* hEvent) = 0;
		virtual bool Signal(uint64_t value) = 0;

		virtual uint64_t GetCompletedValue() const = 0;

		// Create from API native handle
		virtual void Create(void* pHandle, const wchar_t* name = nullptr) = 0;

		// Get API native handle
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
	class XUSG_INTERFACE CommandLayout
	{
	public:
		//CommandLayout();
		virtual ~CommandLayout() {};

		virtual bool Create(const Device* pDevice, uint32_t byteStride, uint32_t numArguments,
			const IndirectArgument* pArguments, const PipelineLayout& pipelineLayout = nullptr,
			uint32_t nodeMask = 0, const wchar_t* name = nullptr) = 0;

		// Create from API native handle
		virtual void Create(void* pHandle, const wchar_t* name = nullptr) = 0;

		// Get API native handle
		virtual void* GetHandle() const = 0;

		using uptr = std::unique_ptr<CommandLayout>;
		using sptr = std::shared_ptr<CommandLayout>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Command queue
	//--------------------------------------------------------------------------------------
	class XUSG_INTERFACE CommandAllocator
	{
	public:
		//CommandAllocator();
		virtual ~CommandAllocator() {};

		virtual bool Create(const Device* pDevice, CommandListType type, const wchar_t* name = nullptr) = 0;
		virtual bool Reset() = 0;

		// Create from API native handle
		virtual void Create(void* pHandle, const wchar_t* name = nullptr) = 0;

		// Get API native handle
		virtual void* GetHandle() const = 0;

		// Get API native handle of the device for this command allocator
		virtual void* GetDeviceHandle() const = 0;

		virtual const Device* GetDevice() const = 0;

		using uptr = std::unique_ptr<CommandAllocator>;
		using sptr = std::shared_ptr<CommandAllocator>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Command list
	//--------------------------------------------------------------------------------------
	class XUSG_INTERFACE CommandList
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
		virtual void Barrier(uint32_t numBarriers, const ResourceBarrier* pBarriers) = 0;
		virtual void ExecuteBundle(const CommandList* pCommandList) const = 0;
		virtual void SetDescriptorHeaps(uint32_t numDescriptorHeaps, const DescriptorHeap* pDescriptorHeaps) = 0;
		virtual void SetComputePipelineLayout(const PipelineLayout& pipelineLayout) const = 0;
		virtual void SetGraphicsPipelineLayout(const PipelineLayout& pipelineLayout) const = 0;
		virtual void SetComputeDescriptorTable(uint32_t index, const DescriptorTable& descriptorTable) const = 0;
		virtual void SetGraphicsDescriptorTable(uint32_t index, const DescriptorTable& descriptorTable) const = 0;
		virtual void SetComputeDescriptorTable(uint32_t index, const DescriptorHeap& descriptorHeap, int32_t offset) const = 0;
		virtual void SetGraphicsDescriptorTable(uint32_t index, const DescriptorHeap& descriptorHeap, int32_t offset) const = 0;
		virtual void SetCompute32BitConstant(uint32_t index, uint32_t srcData, uint32_t destOffsetIn32BitValues = 0) const = 0;
		virtual void SetGraphics32BitConstant(uint32_t index, uint32_t srcData, uint32_t destOffsetIn32BitValues = 0) const = 0;
		virtual void SetCompute32BitConstants(uint32_t index, uint32_t num32BitValuesToSet,
			const void* pSrcData, uint32_t destOffsetIn32BitValues = 0) const = 0;
		virtual void SetGraphics32BitConstants(uint32_t index, uint32_t num32BitValuesToSet,
			const void* pSrcData, uint32_t destOffsetIn32BitValues = 0) const = 0;
		virtual void SetComputeRootConstantBufferView(uint32_t index, const Resource* pResource, int32_t offset = 0) const = 0;
		virtual void SetGraphicsRootConstantBufferView(uint32_t index, const Resource* pResource, int32_t offset = 0) const = 0;
		virtual void SetComputeRootShaderResourceView(uint32_t index, const Resource* pResource, int32_t offset = 0) const = 0;
		virtual void SetGraphicsRootShaderResourceView(uint32_t index, const Resource* pResource, int32_t offset = 0) const = 0;
		virtual void SetComputeRootUnorderedAccessView(uint32_t index, const Resource* pResource, int32_t offset = 0) const = 0;
		virtual void SetGraphicsRootUnorderedAccessView(uint32_t index, const Resource* pResource, int32_t offset = 0) const = 0;
		virtual void SetComputeRootConstantBufferView(uint32_t index, uint64_t address) const = 0;
		virtual void SetGraphicsRootConstantBufferView(uint32_t index, uint64_t address) const = 0;
		virtual void SetComputeRootShaderResourceView(uint32_t index, uint64_t address) const = 0;
		virtual void SetGraphicsRootShaderResourceView(uint32_t index, uint64_t address) const = 0;
		virtual void SetComputeRootUnorderedAccessView(uint32_t index, uint64_t address) const = 0;
		virtual void SetGraphicsRootUnorderedAccessView(uint32_t index, uint64_t address) const = 0;
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
			float depth, uint8_t stencil = 0, uint32_t numRects = 0, const RectRange* pRects = nullptr) = 0;
		virtual void ClearDepthStencilView(const Descriptor& depthStencilView, ClearFlag clearFlags,
			float depth, uint8_t stencil = 0, uint32_t numRects = 0, const RectRange* pRects = nullptr) = 0;
		virtual void ClearRenderTargetView(const Descriptor& renderTargetView, const float colorRGBA[4],
			uint32_t numRects = 0, const RectRange* pRects = nullptr) = 0;
		virtual void ClearUnorderedAccessViewUint(const DescriptorTable& descriptorTable,
			const Descriptor& descriptor, const Resource* pResource, const uint32_t values[4],
			uint32_t numRects = 0, const RectRange* pRects = nullptr) = 0;
		virtual void ClearUnorderedAccessViewFloat(const DescriptorTable& descriptorTable,
			const Descriptor& descriptor, const Resource* pResource, const float values[4],
			uint32_t numRects = 0, const RectRange* pRects = nullptr) = 0;
		virtual void DiscardResource(const Resource*pResource, uint32_t numRects, const RectRange* pRects,
			uint32_t firstSubresource, uint32_t numSubresources) = 0;
		virtual void BeginQuery(const QueryHeap& queryHeap, QueryType type, uint32_t index) const = 0;
		virtual void EndQuery(const QueryHeap& queryHeap, QueryType type, uint32_t index) const = 0;
		virtual void ResolveQueryData(const QueryHeap& queryHeap, QueryType type, uint32_t startIndex,
			uint32_t numQueries, const Resource* pDstBuffer, uint64_t alignedDstBufferOffset) const = 0;
		virtual void SetPredication(const Resource* pBuffer, uint64_t alignedBufferOffset, bool opEqualZero)const = 0;
		virtual void SetMarker(uint32_t metaData, const void* pData, uint32_t size) const = 0;
		virtual void BeginEvent(uint32_t metaData, const void* pData, uint32_t size) const = 0;
		virtual void EndEvent() = 0;
		virtual void ExecuteIndirect(const CommandLayout* pCommandlayout, uint32_t maxCommandCount,
			const Resource* pArgumentBuffer, uint64_t argumentBufferOffset = 0,
			const Resource* pCountBuffer = nullptr, uint64_t countBufferOffset = 0) = 0;

		// Create from API native handle
		virtual void Create(void* pHandle, const wchar_t* name = nullptr) = 0;

		// Get API native handle
		virtual void* GetHandle() const = 0;

		// Get API native handle of the device for this command list
		virtual void* GetDeviceHandle() const = 0;

		virtual const Device* GetDevice() const = 0;

		using uptr = std::unique_ptr<CommandList>;
		using sptr = std::shared_ptr<CommandList>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Command queue
	//--------------------------------------------------------------------------------------
	class XUSG_INTERFACE CommandQueue
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

		// Create from API native handle
		virtual void Create(void* pHandle, const wchar_t* name = nullptr) = 0;

		// Get API native handle
		virtual void* GetHandle() const = 0;

		// Get API native handle of the device for this command list
		virtual void* GetDeviceHandle() const = 0;

		virtual const Device* GetDevice() const = 0;

		using uptr = std::unique_ptr<CommandQueue>;
		using sptr = std::shared_ptr<CommandQueue>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Swap chain
	//--------------------------------------------------------------------------------------
	class Resource;

	class XUSG_INTERFACE SwapChain
	{
	public:
		//SwapChain();
		virtual ~SwapChain() {};

		virtual bool Create(void* pFactory, void* hWnd, void* pDevice,
			uint8_t bufferCount, uint32_t width, uint32_t height, Format format,
			SwapChainFlag flags = SwapChainFlag::NONE, bool windowed = true) = 0;
		virtual bool Present(uint8_t syncInterval = 0, PresentFlag flags = PresentFlag::NONE) = 0;
		virtual bool PresentEx(uint8_t syncInterval, PresentFlag flags, uint32_t dirtyRectsCount,
			const RectRange* pDirtyRects, const RectRange* pScrollRect = nullptr,
			const Point* pScrollOffset = nullptr) = 0;
		virtual bool GetBuffer(uint8_t buffer, Resource* pResource) const = 0;

		virtual uint32_t ResizeBuffers(uint8_t bufferCount, uint32_t width, uint32_t height,
			Format format, SwapChainFlag flags = SwapChainFlag::NONE) = 0;

		virtual uint8_t GetCurrentBackBufferIndex() const = 0;

		// Create from API native handle
		virtual void Create(void* pHandle) = 0;

		// Get API native handle
		virtual void* GetHandle() const = 0;

		using uptr = std::unique_ptr<SwapChain>;
		using sptr = std::shared_ptr<SwapChain>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Resource
	//--------------------------------------------------------------------------------------
	class XUSG_INTERFACE Resource
	{
	public:
		//Resource();
		virtual ~Resource() {};

		virtual bool Initialize(const Device* pDevice) = 0;

		virtual Descriptor AllocateCbvSrvUavHeap(uint32_t numDescriptors) = 0;

		virtual uint32_t SetBarrier(ResourceBarrier* pBarriers, ResourceState dstState,
			uint32_t numBarriers = 0, uint32_t subresource = XUSG_BARRIER_ALL_SUBRESOURCES,
			BarrierFlag flags = BarrierFlag::NONE, ResourceState srcState = ResourceState::AUTO,
			uint32_t threadIdx = 0) = 0;

		virtual ResourceState Transition(ResourceState dstState, uint32_t subresource = XUSG_BARRIER_ALL_SUBRESOURCES,
			BarrierFlag flag = BarrierFlag::NONE, ResourceState srcState = ResourceState::AUTO, uint32_t threadIdx = 0) = 0;
		virtual ResourceState GetResourceState(uint32_t subresource = 0, uint32_t threadIdx = 0) const = 0;

		virtual uint64_t GetWidth() const = 0;

		virtual uint64_t GetVirtualAddress(int offset = 0) const = 0;

		// Create from API native handle
		virtual void Create(void* pDeviceHandle, void* pResourceHandle,
			const wchar_t* name = nullptr, uint32_t maxThreads = 1) = 0;
		virtual void SetName(const wchar_t* name) = 0;

		// Get API native handle
		virtual void* GetHandle() const = 0;

		using uptr = std::unique_ptr<Resource>;
		using sptr = std::shared_ptr<Resource>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Constant buffer
	//--------------------------------------------------------------------------------------
	class XUSG_INTERFACE ConstantBuffer :
		public virtual Resource
	{
	public:
		//ConstantBuffer();
		virtual ~ConstantBuffer() {};

		virtual bool Create(const Device* pDevice, size_t byteWidth, uint32_t numCBVs = 1,
			const size_t* cbvByteOffsets = nullptr, MemoryType memoryType = MemoryType::UPLOAD,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr) = 0;
		virtual bool CreateResource(size_t byteWidth, MemoryType memoryType = MemoryType::UPLOAD,
			MemoryFlag memoryFlags = MemoryFlag::NONE, ResourceState initialResourceState = ResourceState::COMMON) = 0;
		virtual bool Upload(CommandList* pCommandList, Resource* pUploader, const void* pData,
			size_t size, size_t byteOffset = 0, ResourceState srcState = ResourceState::COMMON,
			ResourceState dstState = ResourceState::COMMON) = 0;
		virtual bool Upload(CommandList* pCommandList, uint32_t cbvIndex, Resource* pUploader,
			const void* pData, size_t byteSize, ResourceState srcState = ResourceState::COMMON,
			ResourceState dstState = ResourceState::COMMON) = 0;

		virtual Descriptor CreateCBV(const Descriptor& cbvHeapStart, uint32_t descriptorIdx,
			uint32_t byteSize, size_t byteOffset = 0) = 0;

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
	class XUSG_INTERFACE ShaderResource :
		public virtual Resource
	{
	public:
		//ShaderResource();
		virtual ~ShaderResource() {};

		virtual bool Initialize(const Device* pDevice, Format format) = 0;

		virtual const Descriptor& GetSRV(uint32_t index = 0) const = 0;

		virtual Format GetFormat() const = 0;

		using uptr = std::unique_ptr<ShaderResource>;
		using sptr = std::shared_ptr<ShaderResource>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Texture
	//--------------------------------------------------------------------------------------
	class Buffer;

	class XUSG_INTERFACE Texture :
		public virtual ShaderResource
	{
	public:
		//Texture();
		virtual ~Texture() {};

		virtual bool Create(const Device* pDevice, uint32_t width, uint32_t height, Format format,
			uint16_t arraySize = 1, ResourceFlag resourceFlags = ResourceFlag::NONE,
			uint8_t numMips = 1, uint8_t sampleCount = 1, bool isCubeMap = false,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr,
			uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
			TextureLayout textureLayout = TextureLayout::UNKNOWN,
			uint32_t maxThreads = 1) = 0;
		virtual bool CreateResource(uint32_t width, uint32_t height, Format format, uint16_t arraySize = 1,
			ResourceFlag resourceFlags = ResourceFlag::NONE, uint8_t numMips = 1, uint8_t sampleCount = 1,
			MemoryFlag memoryFlags = MemoryFlag::NONE, ResourceState initialResourceState = ResourceState::COMMON,
			TextureLayout textureLayout = TextureLayout::UNKNOWN, uint32_t maxThreads = 1) = 0;
		virtual bool Upload(CommandList* pCommandList, Resource* pUploader,
			const SubresourceData* pSubresourceData, uint32_t numSubresources = 1,
			ResourceState dstState = ResourceState::COMMON, uint32_t firstSubresource = 0,
			uint32_t threadIdx = 0) = 0;
		virtual bool Upload(CommandList* pCommandList, Resource* pUploader, const void* pData,
			uint8_t byteStride = sizeof(float), ResourceState dstState = ResourceState::COMMON,
			uint32_t threadIdx = 0) = 0;
		virtual bool ReadBack(CommandList* pCommandList, Buffer* pReadBuffer, uint32_t* pRowPitches = nullptr,
			uint32_t numSubresources = 1, uint32_t firstSubresource = 0, size_t offset = 0,
			ResourceState dstState = ResourceState::COMMON, uint32_t threadIdx = 0) = 0;

		virtual Descriptor CreateSRV(const Descriptor& srvHeapStart, uint32_t descriptorIdx, uint16_t arraySize,
			uint16_t firstArraySlice = 0, Format format = Format::UNKNOWN, uint8_t numMips = 1,
			uint8_t mostDetailedMip = 0, bool multisamples = false, bool isCubeMap = false,
			uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING, uint8_t plane = 0) = 0;
		virtual Descriptor CreateUAV(const Descriptor& uavHeapStart, uint32_t descriptorIdx, uint16_t arraySize,
			uint16_t firstArraySlice = 0, Format format = Format::UNKNOWN, uint8_t mipLevel = 0,
			uint8_t plane = 0) = 0;

		virtual uint32_t SetBarrier(ResourceBarrier* pBarriers, ResourceState dstState,
			uint32_t numBarriers = 0, uint32_t subresource = XUSG_BARRIER_ALL_SUBRESOURCES,
			BarrierFlag flags = BarrierFlag::NONE, ResourceState srcState = ResourceState::AUTO,
			uint32_t threadIdx = 0) = 0;
		virtual uint32_t SetBarrier(ResourceBarrier* pBarriers, uint8_t mipLevel, ResourceState dstState,
			uint32_t numBarriers = 0, uint32_t slice = 0, BarrierFlag flags = BarrierFlag::NONE,
			ResourceState srcState = ResourceState::AUTO, uint32_t threadIdx = 0) = 0;

		virtual void Blit(const CommandList* pCommandList, uint32_t groupSizeX, uint32_t groupSizeY,
			uint32_t groupSizeZ, const DescriptorTable& uavSrvTable, uint32_t uavSrvSlot = 0,
			uint8_t mipLevel = 0, const DescriptorTable& srvTable = XUSG_NULL, uint32_t srvSlot = 0,
			const DescriptorTable& samplerTable = XUSG_NULL, uint32_t samplerSlot = 1,
			const Pipeline& pipeline = nullptr) = 0;

		virtual uint32_t Blit(CommandList* pCommandList, ResourceBarrier* pBarriers, uint32_t groupSizeX,
			uint32_t groupSizeY, uint32_t groupSizeZ, uint8_t mipLevel, int8_t srcMipLevel,
			ResourceState srcState, const DescriptorTable& uavSrvTable, uint32_t uavSrvSlot = 0,
			uint32_t numBarriers = 0, const DescriptorTable& srvTable = XUSG_NULL, uint32_t srvSlot = 0,
			uint16_t baseSlice = 0, uint16_t numSlices = 0, uint32_t threadIdx = 0) = 0;
		virtual uint32_t GenerateMips(CommandList* pCommandList, ResourceBarrier* pBarriers, uint32_t groupSizeX,
			uint32_t groupSizeY, uint32_t groupSizeZ, ResourceState dstState, const PipelineLayout& pipelineLayout,
			const Pipeline& pipeline, const DescriptorTable* pUavSrvTables, uint32_t uavSrvSlot = 0,
			const DescriptorTable& samplerTable = XUSG_NULL, uint32_t samplerSlot = 1, uint32_t numBarriers = 0,
			const DescriptorTable* pSrvTables = nullptr, uint32_t srvSlot = 0, uint8_t baseMip = 1,
			uint8_t numMips = 0, uint16_t baseSlice = 0, uint16_t numSlices = 0, uint32_t threadIdx = 0) = 0;

		virtual const Descriptor& GetUAV(uint8_t index = 0) const = 0;
		virtual const Descriptor& GetPackedUAV(uint8_t index = 0) const = 0;
		virtual const Descriptor& GetSRVLevel(uint8_t level) const = 0;

		virtual uint32_t	GetHeight() const = 0;
		virtual uint16_t	GetArraySize() const = 0;
		virtual uint8_t		GetNumMips() const = 0;

		virtual size_t GetRequiredIntermediateSize(uint32_t firstSubresource, uint32_t numSubresources) const = 0;

		Texture* AsTexture();

		using uptr = std::unique_ptr<Texture>;
		using sptr = std::shared_ptr<Texture>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// 2D Texture
	//--------------------------------------------------------------------------------------
	using Texture2D = Texture;

	//--------------------------------------------------------------------------------------
	// Render target
	//--------------------------------------------------------------------------------------
	class XUSG_INTERFACE RenderTarget :
		public virtual Texture
	{
	public:
		//RenderTarget();
		virtual ~RenderTarget() {};

		// Create() will create multiple RTVs (1 slice per RTV)
		virtual bool Create(const Device* pDevice, uint32_t width, uint32_t height, Format format,
			uint16_t arraySize = 1, ResourceFlag resourceFlags = ResourceFlag::NONE, uint8_t numMips = 1,
			uint8_t sampleCount = 1, const float* pClearColor = nullptr, bool isCubeMap = false,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr,
			uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
			TextureLayout textureLayout = TextureLayout::UNKNOWN,
			uint32_t maxThreads = 1) = 0;
		// CreateArray() will create a single array RTV of n slices
		virtual bool CreateArray(const Device* pDevice, uint32_t width, uint32_t height, uint16_t arraySize,
			Format format, ResourceFlag resourceFlags = ResourceFlag::NONE, uint8_t numMips = 1,
			uint8_t sampleCount = 1, const float* pClearColor = nullptr, bool isCubeMap = false,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr,
			uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
			TextureLayout textureLayout = TextureLayout::UNKNOWN,
			uint32_t maxThreads = 1) = 0;
		virtual bool CreateFromSwapChain(const Device* pDevice, const SwapChain* pSwapChain,
			uint32_t bufferIndex, uint32_t maxThreads = 1) = 0;
		virtual bool Initialize(const Device* pDevice, Format format) = 0;
		virtual bool CreateResource(uint32_t width, uint32_t height, Format format, uint16_t arraySize = 1,
			ResourceFlag resourceFlags = ResourceFlag::NONE, uint8_t numMips = 1, uint8_t sampleCount = 1,
			MemoryFlag memoryFlags = MemoryFlag::NONE, ResourceState initialResourceState = ResourceState::COMMON,
			const float* pClearColor = nullptr, TextureLayout textureLayout = TextureLayout::UNKNOWN,
			uint32_t maxThreads = 1) = 0;

		virtual Descriptor AllocateRtvHeap(uint32_t numDescriptors) = 0;
		virtual Descriptor CreateRTV(const Descriptor& rtvHeapStart, uint32_t descriptorIdx, uint16_t arraySize,
			uint16_t firstArraySlice = 0, Format format = Format::UNKNOWN, uint8_t mipLevel = 0,
			bool multisamples = false) = 0;

		virtual void Blit(const CommandList* pCommandList, const DescriptorTable& srcSrvTable,
			uint32_t srcSlot = 0, uint8_t mipLevel = 0, uint16_t baseSlice = 0,
			uint16_t numSlices = 0, const DescriptorTable& samplerTable = XUSG_NULL,
			uint32_t samplerSlot = 1, const Pipeline& pipeline = nullptr,
			uint32_t offsetForSliceId = 0, uint32_t cbSlot = 2) = 0;

		virtual uint32_t Blit(CommandList* pCommandList, ResourceBarrier* pBarriers, uint8_t mipLevel,
			int8_t srcMipLevel, ResourceState srcState, const DescriptorTable& srcSrvTable,
			uint32_t srcSlot = 0, uint32_t numBarriers = 0, uint16_t baseSlice = 0, uint16_t numSlices = 0,
			uint32_t offsetForSliceId = 0, uint32_t cbSlot = 2, uint32_t threadIdx = 0) = 0;
		virtual uint32_t GenerateMips(CommandList* pCommandList, ResourceBarrier* pBarriers, ResourceState dstState,
			const PipelineLayout& pipelineLayout, const Pipeline& pipeline, const DescriptorTable* pSrcSrvTables,
			uint32_t srcSlot = 0, const DescriptorTable& samplerTable = XUSG_NULL, uint32_t samplerSlot = 1,
			uint32_t numBarriers = 0, uint8_t baseMip = 1, uint8_t numMips = 0, uint16_t baseSlice = 0,
			uint16_t numSlices = 0, uint32_t offsetForSliceId = 0, uint32_t cbSlot = 2, uint32_t threadIdx = 0) = 0;

		virtual const Descriptor& GetRTV(uint16_t slice = 0, uint8_t mipLevel = 0) const = 0;

		using uptr = std::unique_ptr<RenderTarget>;
		using sptr = std::shared_ptr<RenderTarget>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Depth stencil
	//--------------------------------------------------------------------------------------
	class XUSG_INTERFACE DepthStencil :
		public virtual Texture
	{
	public:
		//DepthStencil();
		virtual ~DepthStencil() {};

		virtual bool Create(const Device* pDevice, uint32_t width, uint32_t height,
			Format format = Format::UNKNOWN, ResourceFlag resourceFlags = ResourceFlag::NONE,
			uint16_t arraySize = 1, uint8_t numMips = 1, uint8_t sampleCount = 1,
			float clearDepth = 1.0f, uint8_t clearStencil = 0, bool isCubeMap = false,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr,
			uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
			uint16_t stencilSrvComponentMapping = XUSG_DEFAULT_STENCIL_SRV_COMPONENT_MAPPING,
			TextureLayout textureLayout = TextureLayout::UNKNOWN, uint32_t maxThreads = 1) = 0;
		virtual bool CreateArray(const Device* pDevice, uint32_t width, uint32_t height, uint16_t arraySize,
			Format format = Format::UNKNOWN, ResourceFlag resourceFlags = ResourceFlag::NONE,
			uint8_t numMips = 1, uint8_t sampleCount = 1, float clearDepth = 1.0f, uint8_t clearStencil = 0,
			bool isCubeMap = false, MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr,
			uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
			uint16_t stencilSrvComponentMapping = XUSG_DEFAULT_STENCIL_SRV_COMPONENT_MAPPING,
			TextureLayout textureLayout = TextureLayout::UNKNOWN, uint32_t maxThreads = 1) = 0;
		virtual bool Initialize(const Device* pDevice, Format& format) = 0;
		virtual bool CreateResource(uint32_t width, uint32_t height, Format format, uint16_t arraySize = 1,
			ResourceFlag resourceFlags = ResourceFlag::NONE, uint8_t numMips = 1, uint8_t sampleCount = 1,
			MemoryFlag memoryFlags = MemoryFlag::NONE, ResourceState initialResourceState = ResourceState::DEPTH_WRITE,
			float clearDepth = 1.0f, uint8_t clearStencil = 0, TextureLayout textureLayout = TextureLayout::UNKNOWN,
			uint32_t maxThreads = 1) = 0;

		virtual Descriptor AllocateDsvHeap(uint32_t numDescriptors) = 0;
		virtual Descriptor CreateDSV(const Descriptor& dsvHeapStart, uint32_t descriptorIdx, uint16_t arraySize,
			uint16_t firstArraySlice = 0, Format format = Format::UNKNOWN, uint8_t mipLevel = 0,
			bool multisamples = false, bool readOnlyDepth = false, bool readOnlyStencil = false) = 0;

		virtual const Descriptor& GetDSV(uint16_t slice = 0, uint8_t mipLevel = 0) const = 0;
		virtual const Descriptor& GetReadOnlyDSV(uint16_t slice = 0, uint8_t mipLevel = 0) const = 0;
		virtual const Descriptor& GetStencilSRV() const = 0;

		using uptr = std::unique_ptr<DepthStencil>;
		using sptr = std::shared_ptr<DepthStencil>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// 3D Texture
	//--------------------------------------------------------------------------------------
	class XUSG_INTERFACE Texture3D :
		public virtual Texture
	{
	public:
		//Texture3D();
		virtual ~Texture3D() {};

		virtual bool Create(const Device* pDevice, uint32_t width, uint32_t height, uint16_t depth,
			Format format, ResourceFlag resourceFlags = ResourceFlag::NONE, uint8_t numMips = 1,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr,
			uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
			TextureLayout textureLayout = TextureLayout::UNKNOWN,
			uint32_t maxThreads = 1) = 0;
		virtual bool CreateResource(uint32_t width, uint32_t height, uint16_t depth,
			Format format, ResourceFlag resourceFlags = ResourceFlag::NONE,
			uint8_t numMips = 1, MemoryFlag memoryFlags = MemoryFlag::NONE,
			ResourceState initialResourceState = ResourceState::COMMON,
			TextureLayout textureLayout = TextureLayout::UNKNOWN,
			uint32_t maxThreads = 1) = 0;

		virtual Descriptor CreateSRV(const Descriptor& srvHeapStart, uint32_t descriptorIdx,
			Format format = Format::UNKNOWN, uint8_t numMips = 1, uint8_t mostDetailedMip = 0,
			uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING) = 0;
		virtual Descriptor CreateUAV(const Descriptor& uavHeapStart, uint32_t descriptorIdx, uint16_t wSize,
			uint16_t firstWSlice = 0, Format format = Format::UNKNOWN, uint8_t mipLevel = 0) = 0;

		virtual uint16_t GetDepth() const = 0;

		using uptr = std::unique_ptr<Texture3D>;
		using sptr = std::shared_ptr<Texture3D>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Buffer
	//--------------------------------------------------------------------------------------
	class XUSG_INTERFACE Buffer :
		public virtual ShaderResource
	{
	public:
		//Buffer();
		virtual ~Buffer() {};

		virtual bool Create(const Device* pDevice, size_t byteWidth, ResourceFlag resourceFlags = ResourceFlag::NONE,
			MemoryType memoryType = MemoryType::DEFAULT, uint32_t numSRVs = 1,
			const uint32_t* firstSrvElements = nullptr, uint32_t numUAVs = 1,
			const uint32_t* firstUavElements = nullptr, MemoryFlag memoryFlags = MemoryFlag::NONE,
			const wchar_t* name = nullptr, const size_t* counterByteOffsets = nullptr, uint32_t maxThreads = 1) = 0;
		virtual bool CreateResource(size_t byteWidth, ResourceFlag resourceFlags = ResourceFlag::NONE,
			MemoryType memoryType = MemoryType::DEFAULT, MemoryFlag memoryFlags = MemoryFlag::NONE,
			ResourceState initialResourceState = ResourceState::COMMON, uint32_t maxThreads = 1) = 0;
		virtual bool Upload(CommandList* pCommandList, Resource* pUploader, const void* pData, size_t size,
			size_t offset = 0, ResourceState dstState = ResourceState::COMMON, uint32_t threadIdx = 0) = 0;
		virtual bool Upload(CommandList* pCommandList, uint32_t descriptorIndex, Resource* pUploader, const void* pData,
			size_t size, ResourceState dstState = ResourceState::COMMON, uint32_t threadIdx = 0) = 0;
		virtual bool ReadBack(CommandList* pCommandList, Buffer* pReadBuffer, size_t size = 0, size_t dstOffset = 0,
			size_t srcOffset = 0, ResourceState dstState = ResourceState::COMMON, uint32_t threadIdx = 0) = 0;

		virtual Descriptor CreateSRV(const Descriptor& srvHeapStart, uint32_t descriptorIdx,
			uint32_t numElements, uint32_t byteStride, Format format, uint32_t firstElement = 0,
			uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING) = 0;
		virtual Descriptor CreateUAV(const Descriptor& uavHeapStart, uint32_t descriptorIdx, uint32_t numElements,
			uint32_t byteStride, Format format, uint32_t firstElement = 0, size_t counterByteOffset = 0) = 0;

		virtual const Descriptor& GetUAV(uint32_t index = 0) const = 0;

		virtual void* Map(uint32_t descriptorIndex = 0, uintptr_t readBegin = 0, uintptr_t readEnd = 0) = 0;
		virtual void* Map(const Range* pReadRange, uint32_t descriptorIndex = 0) = 0;

		virtual void Unmap() = 0;
		virtual void SetCounter(const Resource::sptr& counter) = 0;

		virtual Resource::sptr GetCounter() const = 0;

		using uptr = std::unique_ptr<Buffer>;
		using sptr = std::shared_ptr<Buffer>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Raw buffer
	//--------------------------------------------------------------------------------------
	using RawBuffer = Buffer;

	//--------------------------------------------------------------------------------------
	// Structured buffer
	//--------------------------------------------------------------------------------------
	class XUSG_INTERFACE StructuredBuffer :
		public virtual Buffer
	{
	public:
		//StructuredBuffer();
		virtual ~StructuredBuffer() {};

		virtual bool Create(const Device* pDevice, uint32_t numElements, uint32_t byteStride,
			ResourceFlag resourceFlags = ResourceFlag::NONE, MemoryType memoryType = MemoryType::DEFAULT,
			uint32_t numSRVs = 1, const uint32_t* firstSrvElements = nullptr,
			uint32_t numUAVs = 1, const uint32_t* firstUavElements = nullptr,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr,
			const size_t* counterByteOffsets = nullptr, uint32_t maxThreads = 1) = 0;
		virtual bool Initialize(const Device* pDevice) = 0;

		using uptr = std::unique_ptr<StructuredBuffer>;
		using sptr = std::shared_ptr<StructuredBuffer>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Typed buffer
	//--------------------------------------------------------------------------------------
	class XUSG_INTERFACE TypedBuffer :
		public virtual Buffer
	{
	public:
		//TypedBuffer();
		virtual ~TypedBuffer() {};

		virtual bool Create(const Device* pDevice, uint32_t numElements, uint32_t byteStride, Format format,
			ResourceFlag resourceFlags = ResourceFlag::NONE, MemoryType memoryType = MemoryType::DEFAULT,
			uint32_t numSRVs = 1, const uint32_t* firstSrvElements = nullptr,
			uint32_t numUAVs = 1, const uint32_t* firstUavElements = nullptr,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr,
			uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
			const size_t* counterByteOffsets = nullptr, uint32_t maxThreads = 1) = 0;

		virtual const Descriptor& GetPackedUAV(uint32_t index = 0) const = 0;

		using uptr = std::unique_ptr<TypedBuffer>;
		using sptr = std::shared_ptr<TypedBuffer>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Vertex buffer
	//--------------------------------------------------------------------------------------
	class XUSG_INTERFACE VertexBuffer :
		public virtual StructuredBuffer
	{
	public:
		//VertexBuffer();
		virtual ~VertexBuffer() {};

		virtual bool Create(const Device* pDevice, uint32_t numVertices, uint32_t byteStride,
			ResourceFlag resourceFlags = ResourceFlag::NONE, MemoryType memoryType = MemoryType::DEFAULT,
			uint32_t numVBVs = 1, const uint32_t* firstVertices = nullptr,
			uint32_t numSRVs = 1, const uint32_t* firstSrvElements = nullptr,
			uint32_t numUAVs = 1, const uint32_t* firstUavElements = nullptr,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr,
			const size_t* counterByteOffsets = nullptr, uint32_t maxThreads = 1) = 0;
		virtual bool CreateAsRaw(const Device* pDevice, uint32_t numVertices, uint32_t byteStride,
			ResourceFlag resourceFlags = ResourceFlag::NONE, MemoryType memoryType = MemoryType::DEFAULT,
			uint32_t numVBVs = 1, const uint32_t* firstVertices = nullptr,
			uint32_t numSRVs = 1, const uint32_t* firstSrvElements = nullptr,
			uint32_t numUAVs = 1, const uint32_t* firstUavElements = nullptr,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr,
			const size_t* counterByteOffsets = nullptr, uint32_t maxThreads = 1) = 0;

		virtual void CreateVBV(VertexBufferView& vbv, uint32_t numVertices, uint32_t byteStride, uint32_t firstVertex = 0) = 0;

		virtual const VertexBufferView& GetVBV(uint32_t index = 0) const = 0;

		using uptr = std::unique_ptr<VertexBuffer>;
		using sptr = std::shared_ptr<VertexBuffer>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Index buffer
	//--------------------------------------------------------------------------------------
	class XUSG_INTERFACE IndexBuffer :
		public virtual TypedBuffer
	{
	public:
		//IndexBuffer();
		virtual ~IndexBuffer() {};

		virtual bool Create(const Device* pDevice, size_t byteWidth, Format format = Format::R32_UINT,
			ResourceFlag resourceFlags = ResourceFlag::DENY_SHADER_RESOURCE,
			MemoryType memoryType = MemoryType::DEFAULT,
			uint32_t numIBVs = 1, const size_t* ibvByteOffsets = nullptr,
			uint32_t numSRVs = 1, const uint32_t* firstSrvElements = nullptr,
			uint32_t numUAVs = 1, const uint32_t* firstUavElements = nullptr,
			MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr,
			uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
			const size_t* counterByteOffsets = nullptr, uint32_t maxThreads = 1) = 0;

		virtual void CreateIBV(IndexBufferView& ibv, Format format, uint32_t byteSize, size_t byteOffset = 0) = 0;

		virtual const IndexBufferView& GetIBV(uint32_t index = 0) const = 0;

		using uptr = std::unique_ptr<IndexBuffer>;
		using sptr = std::shared_ptr<IndexBuffer>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Descriptor
	//--------------------------------------------------------------------------------------
	class DescriptorTableLib;

	namespace Util
	{
		class XUSG_INTERFACE DescriptorTable
		{
		public:
			//DescriptorTable();
			virtual ~DescriptorTable() {};

			virtual void SetDescriptors(uint32_t start, uint32_t num, const Descriptor* srcDescriptors,
				uint8_t descriptorHeapIndex = 0) = 0;
			virtual void SetSamplers(uint32_t start, uint32_t num, const Sampler* const* ppSamplers,
				uint8_t descriptorHeapIndex = 0) = 0;
			virtual void SetSamplers(uint32_t start, uint32_t num, const SamplerPreset* presets,
				DescriptorTableLib* pDescriptorTableLib, uint8_t descriptorHeapIndex = 0) = 0;

			virtual XUSG::DescriptorTable CreateCbvSrvUavTable(DescriptorTableLib* pDescriptorTableLib,
				const XUSG::DescriptorTable& table = XUSG_NULL) = 0;
			virtual XUSG::DescriptorTable GetCbvSrvUavTable(DescriptorTableLib* pDescriptorTableLib,
				const XUSG::DescriptorTable& table = XUSG_NULL) = 0;

			virtual XUSG::DescriptorTable CreateSamplerTable(DescriptorTableLib* pDescriptorTableLib,
				const XUSG::DescriptorTable& table = XUSG_NULL) = 0;
			virtual XUSG::DescriptorTable GetSamplerTable(DescriptorTableLib* pDescriptorTableLib,
				const XUSG::DescriptorTable& table = XUSG_NULL) = 0;

			virtual Framebuffer CreateFramebuffer(DescriptorTableLib* pDescriptorTableLib,
				const Descriptor* pDsv = nullptr, const Framebuffer* pFramebuffer = nullptr) = 0;
			virtual Framebuffer GetFramebuffer(DescriptorTableLib* pDescriptorTableLib,
				const Descriptor* pDsv = nullptr, const Framebuffer* pFramebuffer = nullptr) = 0;

			virtual const std::string& GetKey() const = 0;

			virtual uint32_t CreateCbvSrvUavTableIndex(DescriptorTableLib* pDescriptorTableLib, XUSG::DescriptorTable table = XUSG_NULL) = 0;
			virtual uint32_t GetCbvSrvUavTableIndex(DescriptorTableLib* pDescriptorTableLib, XUSG::DescriptorTable table = XUSG_NULL) = 0;
			virtual uint32_t CreateSamplerTableIndex(DescriptorTableLib* pDescriptorTableLib, XUSG::DescriptorTable table = XUSG_NULL) = 0;
			virtual uint32_t GetSamplerTableIndex(DescriptorTableLib* pDescriptorTableLib, XUSG::DescriptorTable table = XUSG_NULL) = 0;
			virtual uint32_t GetDescriptorTableIndex(DescriptorTableLib* pDescriptorTableLib, DescriptorHeapType type,
				const XUSG::DescriptorTable& table) const = 0;

			using uptr = std::unique_ptr<DescriptorTable>;
			using sptr = std::shared_ptr<DescriptorTable>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
		};
	}

	class XUSG_INTERFACE DescriptorTableLib
	{
	public:
		//DescriptorTableLib();
		//DescriptorTableLib(const Device* pDevice, const wchar_t* name = nullptr);
		virtual ~DescriptorTableLib() {};

		virtual void SetDevice(const Device* pDevice) = 0;
		virtual void SetName(const wchar_t* name) = 0;
		virtual void ResetDescriptorHeap(DescriptorHeapType type, uint8_t index = 0) = 0;

		virtual bool AllocateDescriptorHeap(DescriptorHeapType type, uint32_t numDescriptors, uint8_t index = 0) = 0;

		virtual DescriptorTable CreateCbvSrvUavTable(const Util::DescriptorTable* pUtil, const DescriptorTable& table = XUSG_NULL) = 0;
		virtual DescriptorTable GetCbvSrvUavTable(const Util::DescriptorTable* pUtil, const DescriptorTable& table = XUSG_NULL) = 0;

		virtual DescriptorTable CreateSamplerTable(const Util::DescriptorTable* pUtil, const DescriptorTable& table = XUSG_NULL) = 0;
		virtual DescriptorTable GetSamplerTable(const Util::DescriptorTable* pUtil, const DescriptorTable& table = XUSG_NULL) = 0;

		virtual Framebuffer CreateFramebuffer(const Util::DescriptorTable* pUtil,
			const Descriptor* pDsv = nullptr, const Framebuffer* pFramebuffer = nullptr) = 0;
		virtual Framebuffer GetFramebuffer(const Util::DescriptorTable* pUtil,
			const Descriptor* pDsv = nullptr, const Framebuffer* pFramebuffer = nullptr) = 0;

		virtual DescriptorHeap GetDescriptorHeap(DescriptorHeapType type, uint8_t index = 0) const = 0;

		virtual const Sampler* GetSampler(SamplerPreset preset) = 0;

		virtual uint32_t GetDescriptorStride(DescriptorHeapType type) const = 0;

		static Sampler SamplerPointWrap();
		static Sampler SamplerPointMirror();
		static Sampler SamplerPointClamp();
		static Sampler SamplerPointBorder();
		static Sampler SamplerPointLessEqual();

		static Sampler SamplerLinearWrap();
		static Sampler SamplerLinearMirror();
		static Sampler SamplerLinearClamp();
		static Sampler SamplerLinearBorder();
		static Sampler SamplerLinearLessEqual();

		static Sampler SamplerAnisotropicWrap();
		static Sampler SamplerAnisotropicMirror();
		static Sampler SamplerAnisotropicClamp();
		static Sampler SamplerAnisotropicBorder();
		static Sampler SamplerAnisotropicLessEqual();

		static Sampler SamplerMinLinearWrap();
		static Sampler SamplerMinLinearMirror();
		static Sampler SamplerMinLinearClamp();
		static Sampler SamplerMinLinearBorder();

		static Sampler SamplerMinAnisotropicWrap();
		static Sampler SamplerMinAnisotropicMirror();
		static Sampler SamplerMinAnisotropicClamp();
		static Sampler SamplerMinAnisotropicBorder();

		static Sampler SamplerMaxLinearWrap();
		static Sampler SamplerMaxLinearMirror();
		static Sampler SamplerMaxLinearClamp();
		static Sampler SamplerMaxLinearBorder();

		static Sampler SamplerMaxAnisotropicWrap();
		static Sampler SamplerMaxAnisotropicMirror();
		static Sampler SamplerMaxAnisotropicClamp();
		static Sampler SamplerMaxAnisotropicBorder();

		using uptr = std::unique_ptr<DescriptorTableLib>;
		using sptr = std::shared_ptr<DescriptorTableLib>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
		static uptr MakeUnique(const Device* pDevice, const wchar_t* name = nullptr, API api = API::DIRECTX_12);
		static sptr MakeShared(const Device* pDevice, const wchar_t* name = nullptr, API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Shader reflector
	//--------------------------------------------------------------------------------------
	class XUSG_INTERFACE Reflector
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
	class XUSG_INTERFACE ShaderLib
	{
	public:
		//ShaderLib();
		virtual ~ShaderLib() {};

		virtual void SetShader(Shader::Stage stage, uint32_t index, const Blob& shader) = 0;
		virtual void SetShader(Shader::Stage stage, uint32_t index, const Blob& shader, const Reflector::sptr& reflector) = 0;
		virtual void SetReflector(Shader::Stage stage, uint32_t index, const Reflector::sptr& reflector) = 0;

		virtual Blob CreateShader(Shader::Stage stage, uint32_t index, const wchar_t* fileName) = 0;
		virtual Blob CreateShader(Shader::Stage stage, uint32_t index, const void* pData, size_t size) = 0;
		virtual Blob GetShader(Shader::Stage stage, uint32_t index) const = 0;
		virtual Reflector::sptr GetReflector(Shader::Stage stage, uint32_t index) const = 0;

		using uptr = std::unique_ptr<ShaderLib>;
		using sptr = std::shared_ptr<ShaderLib>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Pipeline layout
	//--------------------------------------------------------------------------------------
	class PipelineLayoutLib;

	namespace Util
	{
		class XUSG_INTERFACE PipelineLayout
		{
		public:
			//PipelineLayout();
			virtual ~PipelineLayout() {};

			virtual void SetShaderStage(uint32_t index, Shader::Stage stage) = 0;
			virtual void SetRange(uint32_t index, DescriptorType type, uint32_t num, uint32_t baseBinding,
				uint32_t space = 0, DescriptorFlag flags = DescriptorFlag::NONE,
				uint32_t offsetInDescriptors = XUSG_DESCRIPTOR_OFFSET_APPEND) = 0;
			virtual void SetConstants(uint32_t index, uint32_t num32BitValues, uint32_t binding,
				uint32_t space = 0, Shader::Stage stage = Shader::Stage::ALL) = 0;
			virtual void SetRootSRV(uint32_t index, uint32_t binding, uint32_t space = 0,
				DescriptorFlag flags = DescriptorFlag::NONE, Shader::Stage stage = Shader::Stage::ALL) = 0;
			virtual void SetRootUAV(uint32_t index, uint32_t binding, uint32_t space = 0,
				DescriptorFlag flags = DescriptorFlag::DATA_STATIC_WHILE_SET_AT_EXECUTE, Shader::Stage stage = Shader::Stage::ALL) = 0;
			virtual void SetRootCBV(uint32_t index, uint32_t binding, uint32_t space = 0, Shader::Stage stage = Shader::Stage::ALL) = 0;
			virtual void SetStaticSamplers(const Sampler* const* ppSamplers, uint32_t num, uint32_t baseBinding,
				uint32_t space = 0, Shader::Stage stage = Shader::Stage::ALL) = 0;

			virtual XUSG::PipelineLayout CreatePipelineLayout(PipelineLayoutLib* pPipelineLayoutLib, PipelineLayoutFlag flags,
				const wchar_t* name = nullptr) = 0;
			virtual XUSG::PipelineLayout GetPipelineLayout(PipelineLayoutLib* pPipelineLayoutLib, PipelineLayoutFlag flags,
				const wchar_t* name = nullptr) = 0;

			virtual DescriptorTableLayout CreateDescriptorTableLayout(uint32_t index, PipelineLayoutLib* pPipelineLayoutLib) const = 0;
			virtual DescriptorTableLayout GetDescriptorTableLayout(uint32_t index, PipelineLayoutLib* pPipelineLayoutLib) const = 0;

			virtual const std::vector<std::string>& GetDescriptorTableLayoutKeys() const = 0;
			virtual std::string& GetPipelineLayoutKey(PipelineLayoutLib* pPipelineLayoutLib) = 0;

			using uptr = std::unique_ptr<PipelineLayout>;
			using sptr = std::shared_ptr<PipelineLayout>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
			static uptr CloneUnique(const PipelineLayout* pSrc, API api = API::DIRECTX_12);
			static sptr CloneShared(const PipelineLayout* pSrc, API api = API::DIRECTX_12);
		};
	}

	class XUSG_INTERFACE PipelineLayoutLib
	{
	public:
		//PipelineLayoutLib();
		//PipelineLayoutLib(const Device* pDevice) = 0;
		virtual ~PipelineLayoutLib() {};

		virtual void SetDevice(const Device* pDevice) = 0;
		virtual void SetPipelineLayout(const std::string& key, const PipelineLayout& pipelineLayout) = 0;

		virtual PipelineLayout CreatePipelineLayout(Util::PipelineLayout* pUtil, PipelineLayoutFlag flags,
			const wchar_t* name = nullptr, uint32_t nodeMask = 0) = 0;
		virtual PipelineLayout GetPipelineLayout(Util::PipelineLayout* pUtil, PipelineLayoutFlag flags,
			const wchar_t* name = nullptr, bool create = true, uint32_t nodeMask = 0) = 0;
		virtual PipelineLayout CreateRootSignature(const void* pBlobSignature, size_t size,
			const wchar_t* name, uint32_t nodeMask = 0) = 0;
		virtual PipelineLayout GetRootSignature(const void* pBlobSignature, size_t size,
			const wchar_t* name = nullptr, bool create = true, uint32_t nodeMask = 0) = 0;

		virtual DescriptorTableLayout CreateDescriptorTableLayout(uint32_t index, const Util::PipelineLayout* pUtil) = 0;
		virtual DescriptorTableLayout GetDescriptorTableLayout(uint32_t index, const Util::PipelineLayout* pUtil) = 0;

		using uptr = std::unique_ptr<PipelineLayoutLib>;
		using sptr = std::shared_ptr<PipelineLayoutLib>;

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
			float DepthBias;
			float DepthBiasClamp;
			float SlopeScaledDepthBias;
			bool DepthClipEnable;
			LineRasterization LineRasterizationMode;
			uint8_t ForcedSampleCount;
			bool ConservativeRaster;
		};

		struct DepthStencilOp
		{
			StencilOp StencilFailOp;
			StencilOp StencilDepthFailOp;
			StencilOp StencilPassOp;
			ComparisonFunc StencilFunc;
			uint8_t StencilReadMask;
			uint8_t StencilWriteMask;
		};

		struct DepthStencil
		{
			bool DepthEnable;
			bool DepthWriteMask;
			ComparisonFunc Comparison;
			bool StencilEnable;
			DepthStencilOp FrontFace;
			DepthStencilOp BackFace;
			bool DepthBoundsTestEnable;
		};

		class PipelineLib;
		
		class XUSG_INTERFACE State
		{
		public:
			//State();
			virtual ~State() {};

			virtual void SetPipelineLayout(const PipelineLayout& layout) = 0;
			virtual void SetShader(Shader::Stage stage, const Blob& shader) = 0;
			virtual void SetCachedPipeline(const Blob& cachedPipeline) = 0;
			virtual void SetNodeMask(uint32_t nodeMask) = 0;
			virtual void SetFlags(PipelineFlag flag) = 0;

			virtual void OMSetBlendState(const Blend* pBlend, uint32_t sampleMask = UINT_MAX) = 0;
			virtual void RSSetState(const Rasterizer* pRasterizer) = 0;
			virtual void DSSetState(const DepthStencil* pDepthStencil) = 0;

			virtual void OMSetBlendState(BlendPreset preset, PipelineLib* pPipelineLib,
				uint8_t numColorRTs = 1, uint32_t sampleMask = UINT_MAX) = 0;
			virtual void RSSetState(RasterizerPreset preset, PipelineLib* pPipelineLib) = 0;
			virtual void DSSetState(DepthStencilPreset preset, PipelineLib* pPipelineLib) = 0;

			virtual void IASetInputLayout(const InputLayout* pLayout) = 0;
			virtual void IASetPrimitiveTopologyType(PrimitiveTopologyType type) = 0;
			virtual void IASetIndexBufferStripCutValue(IBStripCutValue ibStripCutValue) = 0;

			virtual void OMSetNumRenderTargets(uint8_t n) = 0;
			virtual void OMSetRTVFormat(uint8_t i, Format format) = 0;
			virtual void OMSetRTVFormats(const Format* formats, uint8_t n) = 0;
			virtual void OMSetDSVFormat(Format format) = 0;
			virtual void OMSetSample(uint8_t count, uint8_t quality = 0) = 0;

			virtual Pipeline CreatePipeline(PipelineLib* pPipelineLib, const wchar_t* name = nullptr) const = 0;
			virtual Pipeline GetPipeline(PipelineLib* pPipelineLib, const wchar_t* name = nullptr) const = 0;

			virtual PipelineLayout GetPipelineLayout() const = 0;
			virtual Blob GetShader(Shader::Stage stage) const = 0;
			virtual Blob GetCachedPipeline() const = 0;
			virtual uint32_t GetNodeMask() const = 0;
			virtual PipelineFlag GetFlags() const = 0;

			virtual uint32_t OMGetSampleMask() const = 0;
			virtual const Blend* OMGetBlendState() const = 0;
			virtual const Rasterizer* RSGetState() const = 0;
			virtual const DepthStencil* DSGetState() const = 0;

			virtual const InputLayout* IAGetInputLayout() const = 0;
			virtual PrimitiveTopologyType IAGetPrimitiveTopologyType() const = 0;
			virtual IBStripCutValue IAGetIndexBufferStripCutValue() const = 0;

			virtual uint8_t OMGetNumRenderTargets() const = 0;
			virtual Format OMGetRTVFormat(uint8_t i) const = 0;
			virtual Format OMGetDSVFormat() const = 0;
			virtual uint8_t OMGetSampleCount() const = 0;
			virtual uint8_t OMGetSampleQuality() const = 0;

			// Get API native desc of this PSO handle
			// pInputElements should be a pointer to std::vector<[API]_INPUT_ELEMENT_DESC>
			virtual void GetHandleDesc(void* pHandleDesc, void* pInputElements) const = 0;

			using uptr = std::unique_ptr<State>;
			using sptr = std::shared_ptr<State>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
		};

		class XUSG_INTERFACE PipelineLib
		{
		public:
			//PipelineLib();
			//PipelineLib(const Device* pDevice);
			virtual ~PipelineLib() {};

			virtual void SetDevice(const Device* pDevice) = 0;
			virtual void SetPipeline(const State* pState, const Pipeline& pipeline) = 0;

			virtual void SetInputLayout(uint32_t index, const InputElement* pElements, uint32_t numElements) = 0;
			virtual const InputLayout* GetInputLayout(uint32_t index) const = 0;
			virtual const InputLayout* CreateInputLayout(const InputElement* pElements, uint32_t numElements) = 0;

			virtual Pipeline CreatePipeline(const State* pState, const wchar_t* name = nullptr) = 0;
			virtual Pipeline GetPipeline(const State* pState, const wchar_t* name = nullptr) = 0;

			virtual const Blend* GetBlend(BlendPreset preset, uint8_t numColorRTs = 1) = 0;
			virtual const Rasterizer* GetRasterizer(RasterizerPreset preset) = 0;
			virtual const DepthStencil* GetDepthStencil(DepthStencilPreset preset) = 0;

			static DepthStencil DepthStencilDefault();
			static DepthStencil DepthStencilNone();
			static DepthStencil DepthRead();
			static DepthStencil DepthReadLessEqual();
			static DepthStencil DepthReadEqual();

			static Blend DefaultOpaque(uint8_t n);
			static Blend Premultiplied(uint8_t n);
			static Blend Additive(uint8_t n);
			static Blend NonPremultiplied(uint8_t n);
			static Blend NonPremultipliedRT0(uint8_t n);
			static Blend AlphaToCoverage(uint8_t n);
			static Blend Accumulative(uint8_t n);
			static Blend AutoNonPremultiplied(uint8_t n);
			static Blend ZeroAlphaNonPremultiplied(uint8_t n);
			static Blend Multiplied(uint8_t n);
			static Blend WeightedPremul(uint8_t n);
			static Blend WeightedPremulPerRT(uint8_t n);
			static Blend WeightedPerRT(uint8_t n);
			static Blend SelectMin(uint8_t n);
			static Blend SelectMax(uint8_t n);

			static Rasterizer RasterizerDefault();
			static Rasterizer CullBack();
			static Rasterizer CullNone();
			static Rasterizer CullFront();
			static Rasterizer FillWireframe();

			using uptr = std::unique_ptr<PipelineLib>;
			using sptr = std::shared_ptr<PipelineLib>;

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
		class PipelineLib;
		
		class XUSG_INTERFACE State
		{
		public:
			//State();
			virtual ~State() {};

			virtual void SetPipelineLayout(const PipelineLayout& layout) = 0;
			virtual void SetShader(const Blob& shader) = 0;
			virtual void SetCachedPipeline(const Blob& cachedPipeline) = 0;
			virtual void SetNodeMask(uint32_t nodeMask) = 0;
			virtual void SetFlags(PipelineFlag flag) = 0;

			virtual Pipeline CreatePipeline(PipelineLib* pPipelineLib, const wchar_t* name = nullptr) const = 0;
			virtual Pipeline GetPipeline(PipelineLib* pPipelineLib, const wchar_t* name = nullptr) const = 0;

			virtual PipelineLayout GetPipelineLayout() const = 0;
			virtual Blob GetShader() const = 0;
			virtual Blob GetCachedPipeline() const = 0;
			virtual uint32_t GetNodeMask() const = 0;
			virtual PipelineFlag GetFlags() const = 0;

			// Get API native desc of this PSO handle
			virtual void GetHandleDesc(void* pHandleDesc) const = 0;

			using uptr = std::unique_ptr<State>;
			using sptr = std::shared_ptr<State>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
		};

		class XUSG_INTERFACE PipelineLib
		{
		public:
			//PipelineLib();
			//PipelineLib(const Device* pDevice);
			virtual ~PipelineLib() {};

			virtual void SetDevice(const Device* pDevice) = 0;
			virtual void SetPipeline(const State* pState, const Pipeline& pipeline) = 0;

			virtual Pipeline CreatePipeline(const State* pState, const wchar_t* name = nullptr) = 0;
			virtual Pipeline GetPipeline(const State* pState, const wchar_t* name = nullptr) = 0;

			using uptr = std::unique_ptr<PipelineLib>;
			using sptr = std::shared_ptr<PipelineLib>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
			static uptr MakeUnique(const Device* pDevice, API api = API::DIRECTX_12);
			static sptr MakeShared(const Device* pDevice, API api = API::DIRECTX_12);
		};
	}

	XUSG_INTERFACE Blob GetPipelineCache(Pipeline pipeline, API api = API::DIRECTX_12);

	XUSG_INTERFACE uint8_t Log2(uint32_t value);
	XUSG_INTERFACE uint8_t CalculateMipLevels(uint32_t width, uint32_t height, uint32_t depth = 1);
	XUSG_INTERFACE uint8_t CalculateMipLevels(uint64_t width, uint32_t height, uint32_t depth = 1);

	XUSG_INTERFACE uint32_t CalculateSubresource(uint8_t mipSlice, uint8_t numMips,
		uint32_t arraySlice, uint32_t arraySize, uint8_t planeSlice);

	XUSG_INTERFACE size_t AlignConstantBufferView(size_t byteSize, API api = API::DIRECTX_12);
}
