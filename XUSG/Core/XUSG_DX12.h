//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#define DLL_EXPORT					__declspec(dllexport)

#define H_RETURN(x, o, m, r)		{ const auto hr = x; if (FAILED(hr)) { o << m << std::endl; return r; } }
#define V_RETURN(x, o, r)			H_RETURN(x, o, HrToString(hr), r)

#define M_RETURN(x, o, m, r)		if (x) { o << m << std::endl; return r; }
#define F_RETURN(x, o, h, r)		M_RETURN(x, o, HrToString(h), r)

#define APPEND_ALIGNED_ELEMENT		D3D12_APPEND_ALIGNED_ELEMENT
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

	// Enumerations
	enum class CommandQueueFlags
	{
		NONE				= D3D12_COMMAND_QUEUE_FLAG_NONE,
		DISABLE_GPU_TIMEOUT	= D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT
	};

	DEFINE_ENUM_FLAG_OPERATORS(CommandQueueFlags);

	enum class CommandListType
	{
		DIRECT			= D3D12_COMMAND_LIST_TYPE_DIRECT,
		BUNDLE			= D3D12_COMMAND_LIST_TYPE_BUNDLE,
		COMPUTE			= D3D12_COMMAND_LIST_TYPE_COMPUTE,
		COPY			= D3D12_COMMAND_LIST_TYPE_COPY,
		VIDEO_DECODE	= 4,	// D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE,
		VIDEO_PROCESS	= 5,	// D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS,
		VIDEO_ENCODE	= 6		// D3D12_COMMAND_LIST_TYPE_VIDEO_ENCODE
	};

	enum class Format
	{
		UNKNOWN						= DXGI_FORMAT_UNKNOWN,
		R32G32B32A32_TYPELESS		= DXGI_FORMAT_R32G32B32A32_TYPELESS,
		R32G32B32A32_FLOAT			= DXGI_FORMAT_R32G32B32A32_FLOAT,
		R32G32B32A32_UINT			= DXGI_FORMAT_R32G32B32A32_UINT,
		R32G32B32A32_SINT			= DXGI_FORMAT_R32G32B32A32_SINT,
		R32G32B32_TYPELESS			= DXGI_FORMAT_R32G32B32_TYPELESS,
		R32G32B32_FLOAT				= DXGI_FORMAT_R32G32B32_FLOAT,
		R32G32B32_UINT				= DXGI_FORMAT_R32G32B32_UINT,
		R32G32B32_SINT				= DXGI_FORMAT_R32G32B32_SINT,
		R16G16B16A16_TYPELESS		= DXGI_FORMAT_R16G16B16A16_TYPELESS,
		R16G16B16A16_FLOAT			= DXGI_FORMAT_R16G16B16A16_FLOAT,
		R16G16B16A16_UNORM			= DXGI_FORMAT_R16G16B16A16_UNORM,
		R16G16B16A16_UINT			= DXGI_FORMAT_R16G16B16A16_UINT,
		R16G16B16A16_SNORM			= DXGI_FORMAT_R16G16B16A16_SNORM,
		R16G16B16A16_SINT			= DXGI_FORMAT_R16G16B16A16_SINT,
		R32G32_TYPELESS				= DXGI_FORMAT_R32G32_TYPELESS,
		R32G32_FLOAT				= DXGI_FORMAT_R32G32_FLOAT,
		R32G32_UINT					= DXGI_FORMAT_R32G32_UINT,
		R32G32_SINT					= DXGI_FORMAT_R32G32_SINT,
		R32G8X24_TYPELESS			= DXGI_FORMAT_R32G8X24_TYPELESS,
		D32_FLOAT_S8X24_UINT		= DXGI_FORMAT_D32_FLOAT_S8X24_UINT,
		R32_FLOAT_X8X24_TYPELESS	= DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS,
		X32_TYPELESS_G8X24_UINT		= DXGI_FORMAT_X32_TYPELESS_G8X24_UINT,
		R10G10B10A2_TYPELESS		= DXGI_FORMAT_R10G10B10A2_TYPELESS,
		R10G10B10A2_UNORM			= DXGI_FORMAT_R10G10B10A2_UNORM,
		R10G10B10A2_UINT			= DXGI_FORMAT_R10G10B10A2_UINT,
		R11G11B10_FLOAT				= DXGI_FORMAT_R11G11B10_FLOAT,
		R8G8B8A8_TYPELESS			= DXGI_FORMAT_R8G8B8A8_TYPELESS,
		R8G8B8A8_UNORM				= DXGI_FORMAT_R8G8B8A8_UNORM,
		R8G8B8A8_UNORM_SRGB			= DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		R8G8B8A8_UINT				= DXGI_FORMAT_R8G8B8A8_UINT,
		R8G8B8A8_SNORM				= DXGI_FORMAT_R8G8B8A8_SNORM,
		R8G8B8A8_SINT				= DXGI_FORMAT_R8G8B8A8_SINT,
		R16G16_TYPELESS				= DXGI_FORMAT_R16G16_TYPELESS,
		R16G16_FLOAT				= DXGI_FORMAT_R16G16_FLOAT,
		R16G16_UNORM				= DXGI_FORMAT_R16G16_UNORM,
		R16G16_UINT					= DXGI_FORMAT_R16G16_UINT,
		R16G16_SNORM				= DXGI_FORMAT_R16G16_SNORM,
		R16G16_SINT					= DXGI_FORMAT_R16G16_SINT,
		R32_TYPELESS				= DXGI_FORMAT_R32_TYPELESS,
		D32_FLOAT					= DXGI_FORMAT_D32_FLOAT,
		R32_FLOAT					= DXGI_FORMAT_R32_FLOAT,
		R32_UINT					= DXGI_FORMAT_R32_UINT,
		R32_SINT					= DXGI_FORMAT_R32_SINT,
		R24G8_TYPELESS				= DXGI_FORMAT_R24G8_TYPELESS,
		D24_UNORM_S8_UINT			= DXGI_FORMAT_D24_UNORM_S8_UINT,
		R24_UNORM_X8_TYPELESS		= DXGI_FORMAT_R24_UNORM_X8_TYPELESS,
		X24_TYPELESS_G8_UINT		= DXGI_FORMAT_X24_TYPELESS_G8_UINT,
		R8G8_TYPELESS				= DXGI_FORMAT_R8G8_TYPELESS,
		R8G8_UNORM					= DXGI_FORMAT_R8G8_UNORM,
		R8G8_UINT					= DXGI_FORMAT_R8G8_UINT,
		R8G8_SNORM					= DXGI_FORMAT_R8G8_SNORM,
		R8G8_SINT					= DXGI_FORMAT_R8G8_SINT,
		R16_TYPELESS				= DXGI_FORMAT_R16_TYPELESS,
		R16_FLOAT					= DXGI_FORMAT_R16_FLOAT,
		D16_UNORM					= DXGI_FORMAT_D16_UNORM,
		R16_UNORM					= DXGI_FORMAT_R16_UNORM,
		R16_UINT					= DXGI_FORMAT_R16_UINT,
		R16_SNORM					= DXGI_FORMAT_R16_SNORM,
		R16_SINT					= DXGI_FORMAT_R16_SINT,
		R8_TYPELESS					= DXGI_FORMAT_R8_TYPELESS,
		R8_UNORM					= DXGI_FORMAT_R8_UNORM,
		R8_UINT						= DXGI_FORMAT_R8_UINT,
		R8_SNORM					= DXGI_FORMAT_R8_SNORM,
		R8_SINT						= DXGI_FORMAT_R8_SINT,
		A8_UNORM					= DXGI_FORMAT_A8_UNORM,
		R1_UNORM					= DXGI_FORMAT_R1_UNORM,
		R9G9B9E5_SHAREDEXP			= DXGI_FORMAT_R9G9B9E5_SHAREDEXP,
		R8G8_B8G8_UNORM				= DXGI_FORMAT_R8G8_B8G8_UNORM,
		G8R8_G8B8_UNORM				= DXGI_FORMAT_G8R8_G8B8_UNORM,
		BC1_TYPELESS				= DXGI_FORMAT_BC1_TYPELESS,
		BC1_UNORM					= DXGI_FORMAT_BC1_UNORM,
		BC1_UNORM_SRGB				= DXGI_FORMAT_BC1_UNORM_SRGB,
		BC2_TYPELESS				= DXGI_FORMAT_BC2_TYPELESS,
		BC2_UNORM					= DXGI_FORMAT_BC2_UNORM,
		BC2_UNORM_SRGB				= DXGI_FORMAT_BC2_UNORM_SRGB,
		BC3_TYPELESS				= DXGI_FORMAT_BC3_TYPELESS,
		BC3_UNORM					= DXGI_FORMAT_BC3_UNORM,
		BC3_UNORM_SRGB				= DXGI_FORMAT_BC3_UNORM_SRGB,
		BC4_TYPELESS				= DXGI_FORMAT_BC4_TYPELESS,
		BC4_UNORM					= DXGI_FORMAT_BC4_UNORM,
		BC4_SNORM					= DXGI_FORMAT_BC4_SNORM,
		BC5_TYPELESS				= DXGI_FORMAT_BC5_TYPELESS,
		BC5_UNORM					= DXGI_FORMAT_BC5_UNORM,
		BC5_SNORM					= DXGI_FORMAT_BC5_SNORM,
		B5G6R5_UNORM				= DXGI_FORMAT_B5G6R5_UNORM,
		B5G5R5A1_UNORM				= DXGI_FORMAT_B5G5R5A1_UNORM,
		B8G8R8A8_UNORM				= DXGI_FORMAT_B8G8R8A8_UNORM,
		B8G8R8X8_UNORM				= DXGI_FORMAT_B8G8R8X8_UNORM,
		RGB10_XR_BIAS_A2_UNORM		= DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM,
		B8G8R8A8_TYPELESS			= DXGI_FORMAT_B8G8R8A8_TYPELESS,
		B8G8R8A8_UNORM_SRGB			= DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
		B8G8R8X8_TYPELESS			= DXGI_FORMAT_B8G8R8X8_TYPELESS,
		B8G8R8X8_UNORM_SRGB			= DXGI_FORMAT_B8G8R8X8_UNORM_SRGB,
		BC6H_TYPELESS				= DXGI_FORMAT_BC6H_TYPELESS,
		BC6H_UF16					= DXGI_FORMAT_BC6H_UF16,
		BC6H_SF16					= DXGI_FORMAT_BC6H_SF16,
		BC7_TYPELESS				= DXGI_FORMAT_BC7_TYPELESS,
		BC7_UNORM					= DXGI_FORMAT_BC7_UNORM,
		BC7_UNORM_SRGB				= DXGI_FORMAT_BC7_UNORM_SRGB,
		AYUV						= DXGI_FORMAT_AYUV,
		Y410						= DXGI_FORMAT_Y410,
		Y416						= DXGI_FORMAT_Y416,
		NV12						= DXGI_FORMAT_NV12,
		P010						= DXGI_FORMAT_P010,
		P016						= DXGI_FORMAT_P016,
		OPAQUE_420					= DXGI_FORMAT_420_OPAQUE,
		YUY2						= DXGI_FORMAT_YUY2,
		Y210						= DXGI_FORMAT_Y210,
		Y216						= DXGI_FORMAT_Y216,
		NV11						= DXGI_FORMAT_NV11,
		AI44						= DXGI_FORMAT_AI44,
		IA44						= DXGI_FORMAT_IA44,
		P8							= DXGI_FORMAT_P8,
		A8P8						= DXGI_FORMAT_A8P8,
		B4G4R4A4_UNORM				= DXGI_FORMAT_B4G4R4A4_UNORM,

		P208 = DXGI_FORMAT_P208,
		V208 = DXGI_FORMAT_V208,
		V408 = DXGI_FORMAT_V408,

		FORCE_UINT = DXGI_FORMAT_FORCE_UINT
	};

	enum class InputClassification
	{
		PER_VERTEX_DATA = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		PER_INSTANCE_DATA = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA
	};

	enum class MemoryType : uint8_t
    {
        DEFAULT		= D3D12_HEAP_TYPE_DEFAULT,
		UPLOAD		= D3D12_HEAP_TYPE_UPLOAD,
		READBACK	= D3D12_HEAP_TYPE_READBACK,
		CUSTOM		= D3D12_HEAP_TYPE_CUSTOM
    };

	enum class ResourceFlag
	{
		NONE						= D3D12_RESOURCE_FLAG_NONE,
		ALLOW_RENDER_TARGET			= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
		ALLOW_DEPTH_STENCIL			= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
		ALLOW_UNORDERED_ACCESS		= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		DENY_SHADER_RESOURCE		= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE,
		ALLOW_CROSS_ADAPTER			= D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER,
		ALLOW_SIMULTANEOUS_ACCESS	= D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS,
		VIDEO_DECODE_REFERENCE_ONLY	= D3D12_RESOURCE_FLAG_VIDEO_DECODE_REFERENCE_ONLY,
		NEED_PACKED_UAV				= ALLOW_UNORDERED_ACCESS | 0x8000,
		ACCELERATION_STRUCTURE		= ALLOW_UNORDERED_ACCESS | 0x400000
	};

	DEFINE_ENUM_FLAG_OPERATORS(ResourceFlag);

	enum class ResourceState
	{
		COMMON								= D3D12_RESOURCE_STATE_COMMON,
		VERTEX_AND_CONSTANT_BUFFER			= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
		INDEX_BUFFER						= D3D12_RESOURCE_STATE_INDEX_BUFFER,
		RENDER_TARGET						= D3D12_RESOURCE_STATE_RENDER_TARGET,
		UNORDERED_ACCESS					= D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		DEPTH_WRITE							= D3D12_RESOURCE_STATE_DEPTH_WRITE,
		DEPTH_READ							= D3D12_RESOURCE_STATE_DEPTH_READ,
		NON_PIXEL_SHADER_RESOURCE			= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		PIXEL_SHADER_RESOURCE				= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		STREAM_OUT							= D3D12_RESOURCE_STATE_STREAM_OUT,
		INDIRECT_ARGUMENT					= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
		COPY_DEST							= D3D12_RESOURCE_STATE_COPY_DEST,
		COPY_SOURCE							= D3D12_RESOURCE_STATE_COPY_SOURCE,
		RESOLVE_DEST						= D3D12_RESOURCE_STATE_RESOLVE_DEST,
		RESOLVE_SOURCE						= D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
		RAYTRACING_ACCELERATION_STRUCTURE	= 0x400000,		// D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		SHADING_RATE_SOURCE					= 0x1000000,	// D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE,
		GENERAL_READ						= D3D12_RESOURCE_STATE_GENERIC_READ,
		PRESENT								= D3D12_RESOURCE_STATE_PRESENT,
		PREDICATION							= D3D12_RESOURCE_STATE_PREDICATION,
		VIDEO_DECODE_READ					= 0x10000,		// D3D12_RESOURCE_STATE_VIDEO_DECODE_READ,
		VIDEO_DECODE_WRITE					= 0x20000,		// D3D12_RESOURCE_STATE_VIDEO_DECODE_WRITE,
		VIDEO_PROCESS_READ					= 0x40000,		// D3D12_RESOURCE_STATE_VIDEO_PROCESS_READ,
		VIDEO_PROCESS_WRITE					= 0x80000,		// D3D12_RESOURCE_STATE_VIDEO_PROCESS_WRITE,
		VIDEO_ENCODE_READ					= 0x200000,		// D3D12_RESOURCE_STATE_VIDEO_ENCODE_READ,
		VIDEO_ENCODE_WRITE					= 0x800000		// D3D12_RESOURCE_STATE_VIDEO_ENCODE_WRITE
	};

	DEFINE_ENUM_FLAG_OPERATORS(ResourceState);

	enum class BarrierFlag : uint8_t
	{
		NONE		= D3D12_RESOURCE_BARRIER_FLAG_NONE,
		BEGIN_ONLY	= D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY,
		END_ONLY	= D3D12_RESOURCE_BARRIER_FLAG_END_ONLY
	};

	DEFINE_ENUM_FLAG_OPERATORS(BarrierFlag);

	enum class DescriptorRangeFlag : uint32_t
	{
		NONE								= D3D12_DESCRIPTOR_RANGE_FLAG_NONE,
		DESCRIPTORS_VOLATILE				= D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE,
		DATA_VOLATILE						= D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE,
		DATA_STATIC_WHILE_SET_AT_EXECUTE	= D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE,
		DATA_STATIC							= D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC,
		DESCRIPTORS_STATIC_KEEPING_BUFFER_BOUNDS_CHECKS = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_STATIC_KEEPING_BUFFER_BOUNDS_CHECKS
	};

	DEFINE_ENUM_FLAG_OPERATORS(DescriptorRangeFlag);

	enum class PipelineLayoutFlag : uint8_t
	{
		NONE								= D3D12_ROOT_SIGNATURE_FLAG_NONE,
		ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT	= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
		DENY_VERTEX_SHADER_ROOT_ACCESS		= D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS,
		DENY_HULL_SHADER_ROOT_ACCESS		= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS,
		DENY_DOMAIN_SHADER_ROOT_ACCESS		= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS,
		DENY_GEOMETRY_SHADER_ROOT_ACCESS	= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS,
		DENY_PIXEL_SHADER_ROOT_ACCESS		= D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS,
		ALLOW_STREAM_OUTPUT					= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT,
		LOCAL_PIPELINE_LAYOUT				= D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE
	};

	DEFINE_ENUM_FLAG_OPERATORS(PipelineLayoutFlag);

	enum class PrimitiveTopologyType : uint8_t
	{
		UNDEFINED	= D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED,
		POINT		= D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT,
		LINE		= D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE,
		TRIANGLE	= D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
		PATCH		= D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH
	};

	enum class PrimitiveTopology
	{
		UNDEFINED					= D3D_PRIMITIVE_TOPOLOGY_UNDEFINED,
		POINTLIST					= D3D_PRIMITIVE_TOPOLOGY_POINTLIST,
		LINELIST					= D3D_PRIMITIVE_TOPOLOGY_LINELIST,
		LINESTRIP					= D3D_PRIMITIVE_TOPOLOGY_LINESTRIP,
		TRIANGLELIST				= D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
		TRIANGLESTRIP				= D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
		LINELIST_ADJ				= D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ,
		LINESTRIP_ADJ				= D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ,
		TRIANGLELIST_ADJ			= D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ,
		TRIANGLESTRIP_ADJ			= D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ,
		CONTROL_POINT1_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST,
		CONTROL_POINT2_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST,
		CONTROL_POINT3_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST,
		CONTROL_POINT4_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST,
		CONTROL_POINT5_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_5_CONTROL_POINT_PATCHLIST,
		CONTROL_POINT6_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_6_CONTROL_POINT_PATCHLIST,
		CONTROL_POINT7_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_7_CONTROL_POINT_PATCHLIST,
		CONTROL_POINT8_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_8_CONTROL_POINT_PATCHLIST,
		CONTROL_POINT9_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_9_CONTROL_POINT_PATCHLIST,
		CONTROL_POINT10_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_10_CONTROL_POINT_PATCHLIST,
		CONTROL_POINT11_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_11_CONTROL_POINT_PATCHLIST,
		CONTROL_POINT12_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_12_CONTROL_POINT_PATCHLIST,
		CONTROL_POINT13_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_13_CONTROL_POINT_PATCHLIST,
		CONTROL_POINT14_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_14_CONTROL_POINT_PATCHLIST,
		CONTROL_POINT15_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_15_CONTROL_POINT_PATCHLIST,
		CONTROL_POINT16_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST,
		CONTROL_POINT17_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_17_CONTROL_POINT_PATCHLIST,
		CONTROL_POINT18_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_18_CONTROL_POINT_PATCHLIST,
		CONTROL_POINT19_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_19_CONTROL_POINT_PATCHLIST,
		CONTROL_POINT20_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_20_CONTROL_POINT_PATCHLIST,
		CONTROL_POINT21_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_21_CONTROL_POINT_PATCHLIST,
		CONTROL_POINT22_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_22_CONTROL_POINT_PATCHLIST,
		CONTROL_POINT23_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_23_CONTROL_POINT_PATCHLIST,
		CONTROL_POINT24_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_24_CONTROL_POINT_PATCHLIST,
		CONTROL_POINT25_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_25_CONTROL_POINT_PATCHLIST,
		CONTROL_POINT26_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_26_CONTROL_POINT_PATCHLIST,
		CONTROL_POINT27_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_27_CONTROL_POINT_PATCHLIST,
		CONTROL_POINT28_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_28_CONTROL_POINT_PATCHLIST,
		CONTROL_POINT29_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_29_CONTROL_POINT_PATCHLIST,
		CONTROL_POINT30_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_30_CONTROL_POINT_PATCHLIST,
		CONTROL_POINT31_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_31_CONTROL_POINT_PATCHLIST,
		CONTROL_POINT32_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_32_CONTROL_POINT_PATCHLIST
	};

	enum class ClearFlag : uint8_t
	{
		DEPTH = D3D12_CLEAR_FLAG_DEPTH,
		STENCIL = D3D12_CLEAR_FLAG_STENCIL
	};

	DEFINE_ENUM_FLAG_OPERATORS(ClearFlag);

	enum class ResourceDimension : uint8_t
	{
		UNKNOWN = D3D12_RESOURCE_DIMENSION_UNKNOWN,
		BUFFER = D3D12_RESOURCE_DIMENSION_BUFFER,
		TEXTURE1D = D3D12_RESOURCE_DIMENSION_TEXTURE1D,
		TEXTURE2D = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		TEXTURE3D = D3D12_RESOURCE_DIMENSION_TEXTURE3D
	};

	enum class FenceFlag
	{
		NONE					= D3D12_FENCE_FLAG_NONE,
		SHARED					= D3D12_FENCE_FLAG_SHARED,
		SHARED_CROSS_ADAPTER	= D3D12_FENCE_FLAG_SHARED_CROSS_ADAPTER,
		NON_MONITORED			= D3D12_FENCE_FLAG_NON_MONITORED
	};

	DEFINE_ENUM_FLAG_OPERATORS(FenceFlag);

	enum class IndirectArgumentType
	{
		DRAW					= D3D12_INDIRECT_ARGUMENT_TYPE_DRAW,
		DRAW_INDEXED			= D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED,
		DISPATCH				= D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH,
		VERTEX_BUFFER_VIEW		= D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW,
		INDEX_BUFFER_VIEW		= D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW,
		CONSTANT				= D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT,
		CONSTANT_BUFFER_VIEW	= D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW,
		SHADER_RESOURCE_VIEW	= D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW,
		UNORDERED_ACCESS_VIEW	= D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW
	};

	enum Requirements
	{
		REQ_MIP_LEVELS						= D3D12_REQ_MIP_LEVELS,
		REQ_TEXTURECUBE_DIMENSION			= D3D12_REQ_TEXTURECUBE_DIMENSION,
		REQ_TEXTURE1D_U_DIMENSION			= D3D12_REQ_TEXTURE1D_U_DIMENSION,
		REQ_TEXTURE2D_U_OR_V_DIMENSION		= D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION,
		REQ_TEXTURE3D_U_V_OR_W_DIMENSION	= D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION,
		REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION	= D3D12_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION,
		REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION	= D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION,
	};

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
		InputElementTable elements;
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
		DX12Device : public ID3D12Device
	{
		bool GetCommandQueue(CommandQueue & commandQueue, CommandListType type, CommandQueueFlags flags, int32_t priority = 0, uint32_t nodeMask = 0);
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
}
