//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGEnum_VK.h"

using namespace std;
using namespace XUSG;

// Dense enumerator values and single-bit flag positions are the indices of these tables.
// Do not reorder or renumber the enums in XUSG.h.

VkFormat XUSG::GetVKFormat(Format format)
{
	static const VkFormat formats[] =
	{
		VK_FORMAT_UNDEFINED,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_FORMAT_R32G32B32A32_UINT,
		VK_FORMAT_R32G32B32A32_SINT,
		VK_FORMAT_R32G32B32_SFLOAT,
		VK_FORMAT_R32G32B32_SFLOAT,
		VK_FORMAT_R32G32B32_UINT,
		VK_FORMAT_R32G32B32_SINT,
		VK_FORMAT_R16G16B16A16_SFLOAT,
		VK_FORMAT_R16G16B16A16_SFLOAT,
		VK_FORMAT_R16G16B16A16_UNORM,
		VK_FORMAT_R16G16B16A16_UINT,
		VK_FORMAT_R16G16B16A16_SNORM,
		VK_FORMAT_R16G16B16A16_SINT,
		VK_FORMAT_R32G32_SFLOAT,
		VK_FORMAT_R32G32_SFLOAT,
		VK_FORMAT_R32G32_UINT,
		VK_FORMAT_R32G32_SINT,
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_A2B10G10R10_UNORM_PACK32,
		VK_FORMAT_A2B10G10R10_UNORM_PACK32,
		VK_FORMAT_A2B10G10R10_UINT_PACK32,
		VK_FORMAT_B10G11R11_UFLOAT_PACK32,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_FORMAT_R8G8B8A8_UINT,
		VK_FORMAT_R8G8B8A8_SNORM,
		VK_FORMAT_R8G8B8A8_SINT,
		VK_FORMAT_R16G16_SFLOAT,
		VK_FORMAT_R16G16_SFLOAT,
		VK_FORMAT_R16G16_UNORM,
		VK_FORMAT_R16G16_UINT,
		VK_FORMAT_R16G16_SNORM,
		VK_FORMAT_R16G16_SINT,
		VK_FORMAT_R32_SFLOAT,
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_R32_SFLOAT,
		VK_FORMAT_R32_UINT,
		VK_FORMAT_R32_SINT,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_R8G8_UNORM,
		VK_FORMAT_R8G8_UNORM,
		VK_FORMAT_R8G8_UINT,
		VK_FORMAT_R8G8_SNORM,
		VK_FORMAT_R8G8_SINT,
		VK_FORMAT_R16_SFLOAT,
		VK_FORMAT_R16_SFLOAT,
		VK_FORMAT_D16_UNORM,
		VK_FORMAT_R16_UNORM,
		VK_FORMAT_R16_UINT,
		VK_FORMAT_R16_SNORM,
		VK_FORMAT_R16_SINT,
		VK_FORMAT_R8_UNORM,
		VK_FORMAT_R8_UNORM,
		VK_FORMAT_R8_UINT,
		VK_FORMAT_R8_SNORM,
		VK_FORMAT_R8_SINT,
		VK_FORMAT_R8_UNORM,
		VK_FORMAT_UNDEFINED,
		VK_FORMAT_E5B9G9R9_UFLOAT_PACK32,
		VK_FORMAT_UNDEFINED,
		VK_FORMAT_UNDEFINED,
		VK_FORMAT_BC1_RGBA_UNORM_BLOCK,
		VK_FORMAT_BC1_RGBA_UNORM_BLOCK,
		VK_FORMAT_BC1_RGBA_SRGB_BLOCK,
		VK_FORMAT_BC2_UNORM_BLOCK,
		VK_FORMAT_BC2_UNORM_BLOCK,
		VK_FORMAT_BC2_SRGB_BLOCK,
		VK_FORMAT_BC3_UNORM_BLOCK,
		VK_FORMAT_BC3_UNORM_BLOCK,
		VK_FORMAT_BC3_SRGB_BLOCK,
		VK_FORMAT_BC4_UNORM_BLOCK,
		VK_FORMAT_BC4_UNORM_BLOCK,
		VK_FORMAT_BC4_SNORM_BLOCK,
		VK_FORMAT_BC5_UNORM_BLOCK,
		VK_FORMAT_BC5_UNORM_BLOCK,
		VK_FORMAT_BC5_SNORM_BLOCK,
		VK_FORMAT_B5G6R5_UNORM_PACK16,
		VK_FORMAT_B5G5R5A1_UNORM_PACK16,
		VK_FORMAT_B8G8R8A8_UNORM,
		VK_FORMAT_B8G8R8A8_UNORM,
		VK_FORMAT_UNDEFINED,
		VK_FORMAT_B8G8R8A8_SRGB,
		VK_FORMAT_B8G8R8A8_SRGB,
		VK_FORMAT_B8G8R8A8_UNORM,
		VK_FORMAT_B8G8R8A8_SRGB,
		VK_FORMAT_BC6H_UFLOAT_BLOCK,
		VK_FORMAT_BC6H_UFLOAT_BLOCK,
		VK_FORMAT_BC6H_SFLOAT_BLOCK,
		VK_FORMAT_BC7_UNORM_BLOCK,
		VK_FORMAT_BC7_UNORM_BLOCK,
		VK_FORMAT_BC7_SRGB_BLOCK,
		VK_FORMAT_UNDEFINED,
		VK_FORMAT_UNDEFINED,
		VK_FORMAT_UNDEFINED,
		VK_FORMAT_G8_B8R8_2PLANE_420_UNORM,
		VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16,
		VK_FORMAT_UNDEFINED,
		VK_FORMAT_UNDEFINED,
		VK_FORMAT_UNDEFINED,
		VK_FORMAT_UNDEFINED,
		VK_FORMAT_UNDEFINED,
		VK_FORMAT_UNDEFINED,
		VK_FORMAT_UNDEFINED,
		VK_FORMAT_UNDEFINED,
		VK_FORMAT_UNDEFINED,
		VK_FORMAT_UNDEFINED,
		VK_FORMAT_B4G4R4A4_UNORM_PACK16,
		VK_FORMAT_UNDEFINED,
		VK_FORMAT_UNDEFINED,
		VK_FORMAT_UNDEFINED,
		VK_FORMAT_UNDEFINED,
		VK_FORMAT_UNDEFINED,
		VK_FORMAT_UNDEFINED,
	};

	assert(formats[static_cast<uint32_t>(Format::UNKNOWN)] == VK_FORMAT_UNDEFINED);
	assert(formats[static_cast<uint32_t>(Format::R32G32B32A32_TYPELESS)] == VK_FORMAT_R32G32B32A32_SFLOAT);
	assert(formats[static_cast<uint32_t>(Format::R32G32B32A32_FLOAT)] == VK_FORMAT_R32G32B32A32_SFLOAT);
	assert(formats[static_cast<uint32_t>(Format::R32G32B32A32_UINT)] == VK_FORMAT_R32G32B32A32_UINT);
	assert(formats[static_cast<uint32_t>(Format::R32G32B32A32_SINT)] == VK_FORMAT_R32G32B32A32_SINT);
	assert(formats[static_cast<uint32_t>(Format::R32G32B32_TYPELESS)] == VK_FORMAT_R32G32B32_SFLOAT);
	assert(formats[static_cast<uint32_t>(Format::R32G32B32_FLOAT)] == VK_FORMAT_R32G32B32_SFLOAT);
	assert(formats[static_cast<uint32_t>(Format::R32G32B32_UINT)] == VK_FORMAT_R32G32B32_UINT);
	assert(formats[static_cast<uint32_t>(Format::R32G32B32_SINT)] == VK_FORMAT_R32G32B32_SINT);
	assert(formats[static_cast<uint32_t>(Format::R16G16B16A16_TYPELESS)] == VK_FORMAT_R16G16B16A16_SFLOAT);
	assert(formats[static_cast<uint32_t>(Format::R16G16B16A16_FLOAT)] == VK_FORMAT_R16G16B16A16_SFLOAT);
	assert(formats[static_cast<uint32_t>(Format::R16G16B16A16_UNORM)] == VK_FORMAT_R16G16B16A16_UNORM);
	assert(formats[static_cast<uint32_t>(Format::R16G16B16A16_UINT)] == VK_FORMAT_R16G16B16A16_UINT);
	assert(formats[static_cast<uint32_t>(Format::R16G16B16A16_SNORM)] == VK_FORMAT_R16G16B16A16_SNORM);
	assert(formats[static_cast<uint32_t>(Format::R16G16B16A16_SINT)] == VK_FORMAT_R16G16B16A16_SINT);
	assert(formats[static_cast<uint32_t>(Format::R32G32_TYPELESS)] == VK_FORMAT_R32G32_SFLOAT);
	assert(formats[static_cast<uint32_t>(Format::R32G32_FLOAT)] == VK_FORMAT_R32G32_SFLOAT);
	assert(formats[static_cast<uint32_t>(Format::R32G32_UINT)] == VK_FORMAT_R32G32_UINT);
	assert(formats[static_cast<uint32_t>(Format::R32G32_SINT)] == VK_FORMAT_R32G32_SINT);
	assert(formats[static_cast<uint32_t>(Format::R32G8X24_TYPELESS)] == VK_FORMAT_D32_SFLOAT_S8_UINT);
	assert(formats[static_cast<uint32_t>(Format::D32_FLOAT_S8X24_UINT)] == VK_FORMAT_D32_SFLOAT_S8_UINT);
	assert(formats[static_cast<uint32_t>(Format::R32_FLOAT_X8X24_TYPELESS)] == VK_FORMAT_D32_SFLOAT_S8_UINT);
	assert(formats[static_cast<uint32_t>(Format::X32_TYPELESS_G8X24_UINT)] == VK_FORMAT_D32_SFLOAT_S8_UINT);
	assert(formats[static_cast<uint32_t>(Format::R10G10B10A2_TYPELESS)] == VK_FORMAT_A2B10G10R10_UNORM_PACK32);
	assert(formats[static_cast<uint32_t>(Format::R10G10B10A2_UNORM)] == VK_FORMAT_A2B10G10R10_UNORM_PACK32);
	assert(formats[static_cast<uint32_t>(Format::R10G10B10A2_UINT)] == VK_FORMAT_A2B10G10R10_UINT_PACK32);
	assert(formats[static_cast<uint32_t>(Format::R11G11B10_FLOAT)] == VK_FORMAT_B10G11R11_UFLOAT_PACK32);
	assert(formats[static_cast<uint32_t>(Format::R8G8B8A8_TYPELESS)] == VK_FORMAT_R8G8B8A8_UNORM);
	assert(formats[static_cast<uint32_t>(Format::R8G8B8A8_UNORM)] == VK_FORMAT_R8G8B8A8_UNORM);
	assert(formats[static_cast<uint32_t>(Format::R8G8B8A8_UNORM_SRGB)] == VK_FORMAT_R8G8B8A8_SRGB);
	assert(formats[static_cast<uint32_t>(Format::R8G8B8A8_UINT)] == VK_FORMAT_R8G8B8A8_UINT);
	assert(formats[static_cast<uint32_t>(Format::R8G8B8A8_SNORM)] == VK_FORMAT_R8G8B8A8_SNORM);
	assert(formats[static_cast<uint32_t>(Format::R8G8B8A8_SINT)] == VK_FORMAT_R8G8B8A8_SINT);
	assert(formats[static_cast<uint32_t>(Format::R16G16_TYPELESS)] == VK_FORMAT_R16G16_SFLOAT);
	assert(formats[static_cast<uint32_t>(Format::R16G16_FLOAT)] == VK_FORMAT_R16G16_SFLOAT);
	assert(formats[static_cast<uint32_t>(Format::R16G16_UNORM)] == VK_FORMAT_R16G16_UNORM);
	assert(formats[static_cast<uint32_t>(Format::R16G16_UINT)] == VK_FORMAT_R16G16_UINT);
	assert(formats[static_cast<uint32_t>(Format::R16G16_SNORM)] == VK_FORMAT_R16G16_SNORM);
	assert(formats[static_cast<uint32_t>(Format::R16G16_SINT)] == VK_FORMAT_R16G16_SINT);
	assert(formats[static_cast<uint32_t>(Format::R32_TYPELESS)] == VK_FORMAT_R32_SFLOAT);
	assert(formats[static_cast<uint32_t>(Format::D32_FLOAT)] == VK_FORMAT_D32_SFLOAT);
	assert(formats[static_cast<uint32_t>(Format::R32_FLOAT)] == VK_FORMAT_R32_SFLOAT);
	assert(formats[static_cast<uint32_t>(Format::R32_UINT)] == VK_FORMAT_R32_UINT);
	assert(formats[static_cast<uint32_t>(Format::R32_SINT)] == VK_FORMAT_R32_SINT);
	assert(formats[static_cast<uint32_t>(Format::R24G8_TYPELESS)] == VK_FORMAT_D24_UNORM_S8_UINT);
	assert(formats[static_cast<uint32_t>(Format::D24_UNORM_S8_UINT)] == VK_FORMAT_D24_UNORM_S8_UINT);
	assert(formats[static_cast<uint32_t>(Format::R24_UNORM_X8_TYPELESS)] == VK_FORMAT_D24_UNORM_S8_UINT);
	assert(formats[static_cast<uint32_t>(Format::X24_TYPELESS_G8_UINT)] == VK_FORMAT_D24_UNORM_S8_UINT);
	assert(formats[static_cast<uint32_t>(Format::R8G8_TYPELESS)] == VK_FORMAT_R8G8_UNORM);
	assert(formats[static_cast<uint32_t>(Format::R8G8_UNORM)] == VK_FORMAT_R8G8_UNORM);
	assert(formats[static_cast<uint32_t>(Format::R8G8_UINT)] == VK_FORMAT_R8G8_UINT);
	assert(formats[static_cast<uint32_t>(Format::R8G8_SNORM)] == VK_FORMAT_R8G8_SNORM);
	assert(formats[static_cast<uint32_t>(Format::R8G8_SINT)] == VK_FORMAT_R8G8_SINT);
	assert(formats[static_cast<uint32_t>(Format::R16_TYPELESS)] == VK_FORMAT_R16_SFLOAT);
	assert(formats[static_cast<uint32_t>(Format::R16_FLOAT)] == VK_FORMAT_R16_SFLOAT);
	assert(formats[static_cast<uint32_t>(Format::D16_UNORM)] == VK_FORMAT_D16_UNORM);
	assert(formats[static_cast<uint32_t>(Format::R16_UNORM)] == VK_FORMAT_R16_UNORM);
	assert(formats[static_cast<uint32_t>(Format::R16_UINT)] == VK_FORMAT_R16_UINT);
	assert(formats[static_cast<uint32_t>(Format::R16_SNORM)] == VK_FORMAT_R16_SNORM);
	assert(formats[static_cast<uint32_t>(Format::R16_SINT)] == VK_FORMAT_R16_SINT);
	assert(formats[static_cast<uint32_t>(Format::R8_TYPELESS)] == VK_FORMAT_R8_UNORM);
	assert(formats[static_cast<uint32_t>(Format::R8_UNORM)] == VK_FORMAT_R8_UNORM);
	assert(formats[static_cast<uint32_t>(Format::R8_UINT)] == VK_FORMAT_R8_UINT);
	assert(formats[static_cast<uint32_t>(Format::R8_SNORM)] == VK_FORMAT_R8_SNORM);
	assert(formats[static_cast<uint32_t>(Format::R8_SINT)] == VK_FORMAT_R8_SINT);
	assert(formats[static_cast<uint32_t>(Format::A8_UNORM)] == VK_FORMAT_R8_UNORM);
	assert(formats[static_cast<uint32_t>(Format::R1_UNORM)] == VK_FORMAT_UNDEFINED);
	assert(formats[static_cast<uint32_t>(Format::R9G9B9E5_SHAREDEXP)] == VK_FORMAT_E5B9G9R9_UFLOAT_PACK32);
	assert(formats[static_cast<uint32_t>(Format::R8G8_B8G8_UNORM)] == VK_FORMAT_UNDEFINED);
	assert(formats[static_cast<uint32_t>(Format::G8R8_G8B8_UNORM)] == VK_FORMAT_UNDEFINED);
	assert(formats[static_cast<uint32_t>(Format::BC1_TYPELESS)] == VK_FORMAT_BC1_RGBA_UNORM_BLOCK);
	assert(formats[static_cast<uint32_t>(Format::BC1_UNORM)] == VK_FORMAT_BC1_RGBA_UNORM_BLOCK);
	assert(formats[static_cast<uint32_t>(Format::BC1_UNORM_SRGB)] == VK_FORMAT_BC1_RGBA_SRGB_BLOCK);
	assert(formats[static_cast<uint32_t>(Format::BC2_TYPELESS)] == VK_FORMAT_BC2_UNORM_BLOCK);
	assert(formats[static_cast<uint32_t>(Format::BC2_UNORM)] == VK_FORMAT_BC2_UNORM_BLOCK);
	assert(formats[static_cast<uint32_t>(Format::BC2_UNORM_SRGB)] == VK_FORMAT_BC2_SRGB_BLOCK);
	assert(formats[static_cast<uint32_t>(Format::BC3_TYPELESS)] == VK_FORMAT_BC3_UNORM_BLOCK);
	assert(formats[static_cast<uint32_t>(Format::BC3_UNORM)] == VK_FORMAT_BC3_UNORM_BLOCK);
	assert(formats[static_cast<uint32_t>(Format::BC3_UNORM_SRGB)] == VK_FORMAT_BC3_SRGB_BLOCK);
	assert(formats[static_cast<uint32_t>(Format::BC4_TYPELESS)] == VK_FORMAT_BC4_UNORM_BLOCK);
	assert(formats[static_cast<uint32_t>(Format::BC4_UNORM)] == VK_FORMAT_BC4_UNORM_BLOCK);
	assert(formats[static_cast<uint32_t>(Format::BC4_SNORM)] == VK_FORMAT_BC4_SNORM_BLOCK);
	assert(formats[static_cast<uint32_t>(Format::BC5_TYPELESS)] == VK_FORMAT_BC5_UNORM_BLOCK);
	assert(formats[static_cast<uint32_t>(Format::BC5_UNORM)] == VK_FORMAT_BC5_UNORM_BLOCK);
	assert(formats[static_cast<uint32_t>(Format::BC5_SNORM)] == VK_FORMAT_BC5_SNORM_BLOCK);
	assert(formats[static_cast<uint32_t>(Format::B5G6R5_UNORM)] == VK_FORMAT_B5G6R5_UNORM_PACK16);
	assert(formats[static_cast<uint32_t>(Format::B5G5R5A1_UNORM)] == VK_FORMAT_B5G5R5A1_UNORM_PACK16);
	assert(formats[static_cast<uint32_t>(Format::B8G8R8A8_UNORM)] == VK_FORMAT_B8G8R8A8_UNORM);
	assert(formats[static_cast<uint32_t>(Format::B8G8R8X8_UNORM)] == VK_FORMAT_B8G8R8A8_UNORM);
	assert(formats[static_cast<uint32_t>(Format::RGB10_XR_BIAS_A2_UNORM)] == VK_FORMAT_UNDEFINED);
	assert(formats[static_cast<uint32_t>(Format::B8G8R8A8_TYPELESS)] == VK_FORMAT_B8G8R8A8_SRGB);
	assert(formats[static_cast<uint32_t>(Format::B8G8R8A8_UNORM_SRGB)] == VK_FORMAT_B8G8R8A8_SRGB);
	assert(formats[static_cast<uint32_t>(Format::B8G8R8X8_TYPELESS)] == VK_FORMAT_B8G8R8A8_UNORM);
	assert(formats[static_cast<uint32_t>(Format::B8G8R8X8_UNORM_SRGB)] == VK_FORMAT_B8G8R8A8_SRGB);
	assert(formats[static_cast<uint32_t>(Format::BC6H_TYPELESS)] == VK_FORMAT_BC6H_UFLOAT_BLOCK);
	assert(formats[static_cast<uint32_t>(Format::BC6H_UF16)] == VK_FORMAT_BC6H_UFLOAT_BLOCK);
	assert(formats[static_cast<uint32_t>(Format::BC6H_SF16)] == VK_FORMAT_BC6H_SFLOAT_BLOCK);
	assert(formats[static_cast<uint32_t>(Format::BC7_TYPELESS)] == VK_FORMAT_BC7_UNORM_BLOCK);
	assert(formats[static_cast<uint32_t>(Format::BC7_UNORM)] == VK_FORMAT_BC7_UNORM_BLOCK);
	assert(formats[static_cast<uint32_t>(Format::BC7_UNORM_SRGB)] == VK_FORMAT_BC7_SRGB_BLOCK);
	assert(formats[static_cast<uint32_t>(Format::AYUV)] == VK_FORMAT_UNDEFINED);
	assert(formats[static_cast<uint32_t>(Format::Y410)] == VK_FORMAT_UNDEFINED);
	assert(formats[static_cast<uint32_t>(Format::Y416)] == VK_FORMAT_UNDEFINED);
	assert(formats[static_cast<uint32_t>(Format::NV12)] == VK_FORMAT_G8_B8R8_2PLANE_420_UNORM);
	assert(formats[static_cast<uint32_t>(Format::P010)] == VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16);
	assert(formats[static_cast<uint32_t>(Format::P016)] == VK_FORMAT_UNDEFINED);
	assert(formats[static_cast<uint32_t>(Format::OPAQUE_420)] == VK_FORMAT_UNDEFINED);
	assert(formats[static_cast<uint32_t>(Format::YUY2)] == VK_FORMAT_UNDEFINED);
	assert(formats[static_cast<uint32_t>(Format::Y210)] == VK_FORMAT_UNDEFINED);
	assert(formats[static_cast<uint32_t>(Format::Y216)] == VK_FORMAT_UNDEFINED);
	assert(formats[static_cast<uint32_t>(Format::NV11)] == VK_FORMAT_UNDEFINED);
	assert(formats[static_cast<uint32_t>(Format::AI44)] == VK_FORMAT_UNDEFINED);
	assert(formats[static_cast<uint32_t>(Format::IA44)] == VK_FORMAT_UNDEFINED);
	assert(formats[static_cast<uint32_t>(Format::P8)] == VK_FORMAT_UNDEFINED);
	assert(formats[static_cast<uint32_t>(Format::A8P8)] == VK_FORMAT_UNDEFINED);
	assert(formats[static_cast<uint32_t>(Format::B4G4R4A4_UNORM)] == VK_FORMAT_B4G4R4A4_UNORM_PACK16);
	assert(formats[static_cast<uint32_t>(Format::P208)] == VK_FORMAT_UNDEFINED);
	assert(formats[static_cast<uint32_t>(Format::V208)] == VK_FORMAT_UNDEFINED);
	assert(formats[static_cast<uint32_t>(Format::V408)] == VK_FORMAT_UNDEFINED);
	assert(formats[static_cast<uint32_t>(Format::MIN_MIP_OPAQUE)] == VK_FORMAT_UNDEFINED);
	assert(formats[static_cast<uint32_t>(Format::MIP_REGION_USED_OPAQUE)] == VK_FORMAT_UNDEFINED);
	assert(formats[static_cast<uint32_t>(Format::FORCE_UINT)] == VK_FORMAT_UNDEFINED);

	const auto vkFormat = formats[static_cast<uint32_t>(format)];
	if (vkFormat == VK_FORMAT_UNDEFINED && format != Format::UNKNOWN)
		assert(!"Format has no Vulkan equivalent");

	return vkFormat;
}

VkFormat XUSG::GetVKFormat(const Device_VK* pDevice, Format format)
{
	const auto vkFormat = GetVKFormat(format);
	if (pDevice && pDevice->EmulatesD24S8() && vkFormat == VK_FORMAT_D24_UNORM_S8_UINT)
		return VK_FORMAT_D32_SFLOAT_S8_UINT;

	return vkFormat;
}

Format XUSG::GetFormatFromVK(VkFormat format)
{
	switch (format)
	{
	case VK_FORMAT_R8G8B8A8_UNORM:			return Format::R8G8B8A8_UNORM;
	case VK_FORMAT_R8G8B8A8_SRGB:			return Format::R8G8B8A8_UNORM_SRGB;
	case VK_FORMAT_B8G8R8A8_UNORM:			return Format::B8G8R8A8_UNORM;
	case VK_FORMAT_B8G8R8A8_SRGB:			return Format::B8G8R8A8_UNORM_SRGB;
	case VK_FORMAT_R16G16B16A16_SFLOAT:		return Format::R16G16B16A16_FLOAT;
	case VK_FORMAT_R32G32B32A32_SFLOAT:		return Format::R32G32B32A32_FLOAT;
	case VK_FORMAT_D32_SFLOAT:				return Format::D32_FLOAT;
	case VK_FORMAT_D24_UNORM_S8_UINT:		return Format::D24_UNORM_S8_UINT;
	case VK_FORMAT_D32_SFLOAT_S8_UINT:		return Format::D32_FLOAT_S8X24_UINT;
	case VK_FORMAT_D16_UNORM:				return Format::D16_UNORM;
	default:								return Format::UNKNOWN;
	}
}

VkImageType XUSG::GetVKImageType(ResourceDimension dimension)
{
	static const VkImageType imageTypes[] =
	{
		VK_IMAGE_TYPE_2D,
		VK_IMAGE_TYPE_2D,
		VK_IMAGE_TYPE_1D,
		VK_IMAGE_TYPE_2D,
		VK_IMAGE_TYPE_3D,
	};

	assert(imageTypes[static_cast<uint32_t>(ResourceDimension::UNKNOWN)] == VK_IMAGE_TYPE_2D);
	assert(imageTypes[static_cast<uint32_t>(ResourceDimension::BUFFER)] == VK_IMAGE_TYPE_2D);
	assert(imageTypes[static_cast<uint32_t>(ResourceDimension::TEXTURE1D)] == VK_IMAGE_TYPE_1D);
	assert(imageTypes[static_cast<uint32_t>(ResourceDimension::TEXTURE2D)] == VK_IMAGE_TYPE_2D);
	assert(imageTypes[static_cast<uint32_t>(ResourceDimension::TEXTURE3D)] == VK_IMAGE_TYPE_3D);

	return imageTypes[static_cast<uint32_t>(dimension)];
}

VkImageViewType XUSG::GetVKImageViewType(ResourceDimension dimension, bool isArray, bool isCube, bool multisample)
{
	if (isCube) return isArray ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE;
	if (dimension == ResourceDimension::TEXTURE3D) return VK_IMAGE_VIEW_TYPE_3D;
	if (dimension == ResourceDimension::TEXTURE1D)
		return isArray ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
	if (multisample) return isArray ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;

	return isArray ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
}

VkImageLayout XUSG::GetVKImageLayout(ResourceState state)
{
	// COMMON/PRESENT for images is resolved in Resource_VK::ImageLayout (presentable ->
	// PRESENT_SRC, else per-resource common layout). Keep GENERAL here as the generic fallback.
	//if (state == ResourceState::COMMON || state == ResourceState::PRESENT) return VK_IMAGE_LAYOUT_GENERAL;
	if (state == ResourceState::COMMON) return VK_IMAGE_LAYOUT_GENERAL;

	// DX12 allows combined states (e.g. COPY_SOURCE | PIXEL_SHADER_RESOURCE). Vulkan needs one
	// layout; exclusive intents use the optimal layout, multi-use falls back to GENERAL.
	const auto color = (state & ResourceState::RENDER_TARGET) != ResourceState::COMMON;
	const auto depthWrite = (state & ResourceState::DEPTH_WRITE) != ResourceState::COMMON;
	const auto depthRead = (state & ResourceState::DEPTH_READ) != ResourceState::COMMON;
	const auto uav = (state & ResourceState::UNORDERED_ACCESS) != ResourceState::COMMON;
	const auto copyDst = (state & (ResourceState::COPY_DEST | ResourceState::RESOLVE_DEST)) != ResourceState::COMMON;
	const auto copySrc = (state & (ResourceState::COPY_SOURCE | ResourceState::RESOLVE_SOURCE)) != ResourceState::COMMON;
	const auto srv = (state & (ResourceState::PIXEL_SHADER_RESOURCE | ResourceState::NON_PIXEL_SHADER_RESOURCE)) != ResourceState::COMMON;
	const auto shadingRate = (state & ResourceState::SHADING_RATE_SOURCE) != ResourceState::COMMON;

	if (color && !depthWrite && !depthRead && !uav && !copyDst && !copySrc && !srv && !shadingRate)
		return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	if (depthWrite && !color && !uav && !copyDst && !copySrc && !srv && !shadingRate)
		return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	// DEPTH_READ | PSR is valid as depth-stencil read-only (sample + test).
	if (depthRead && !depthWrite && !color && !uav && !copyDst && !copySrc && !shadingRate)
		return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	if (uav && !color && !depthWrite && !depthRead && !copyDst && !copySrc && !srv && !shadingRate)
		return VK_IMAGE_LAYOUT_GENERAL;
	if (copyDst && !color && !depthWrite && !depthRead && !uav && !copySrc && !srv && !shadingRate)
		return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	if (copySrc && !color && !depthWrite && !depthRead && !uav && !copyDst && !srv && !shadingRate)
		return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	if (srv && !color && !depthWrite && !depthRead && !uav && !copyDst && !copySrc && !shadingRate)
		return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	if (shadingRate && !color && !depthWrite && !depthRead && !uav && !copyDst && !copySrc && !srv)
		return VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;

	return VK_IMAGE_LAYOUT_GENERAL;
}

VkImageLayout XUSG::GetVKBarrierLayout(BarrierLayout layout)
{
	static const VkImageLayout layouts[] =
	{
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_GENERAL,
	};

	assert(layouts[static_cast<uint32_t>(BarrierLayout::COMMON)] == VK_IMAGE_LAYOUT_GENERAL);
	assert(layouts[static_cast<uint32_t>(BarrierLayout::PRESENT)] == VK_IMAGE_LAYOUT_GENERAL);
	assert(layouts[static_cast<uint32_t>(BarrierLayout::GENERIC_READ_RESOURCE)] == VK_IMAGE_LAYOUT_GENERAL);
	assert(layouts[static_cast<uint32_t>(BarrierLayout::RENDER_TARGET)] == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	assert(layouts[static_cast<uint32_t>(BarrierLayout::UNORDERED_ACCESS)] == VK_IMAGE_LAYOUT_GENERAL);
	assert(layouts[static_cast<uint32_t>(BarrierLayout::DEPTH_STENCIL_WRITE)] == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	assert(layouts[static_cast<uint32_t>(BarrierLayout::DEPTH_STENCIL_READ)] == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
	assert(layouts[static_cast<uint32_t>(BarrierLayout::SHADER_RESOURCE)] == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	assert(layouts[static_cast<uint32_t>(BarrierLayout::COPY_SOURCE)] == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	assert(layouts[static_cast<uint32_t>(BarrierLayout::COPY_DEST)] == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	assert(layouts[static_cast<uint32_t>(BarrierLayout::RESOLVE_SOURCE)] == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	assert(layouts[static_cast<uint32_t>(BarrierLayout::RESOLVE_DEST)] == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	assert(layouts[static_cast<uint32_t>(BarrierLayout::SHADING_RATE_SOURCE)] == VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR);
	assert(layouts[static_cast<uint32_t>(BarrierLayout::VIDEO_DECODE_READ)] == VK_IMAGE_LAYOUT_GENERAL);
	assert(layouts[static_cast<uint32_t>(BarrierLayout::VIDEO_DECODE_WRITE)] == VK_IMAGE_LAYOUT_GENERAL);
	assert(layouts[static_cast<uint32_t>(BarrierLayout::VIDEO_PROCESS_READ)] == VK_IMAGE_LAYOUT_GENERAL);
	assert(layouts[static_cast<uint32_t>(BarrierLayout::VIDEO_PROCESS_WRITE)] == VK_IMAGE_LAYOUT_GENERAL);
	assert(layouts[static_cast<uint32_t>(BarrierLayout::VIDEO_ENCODE_READ)] == VK_IMAGE_LAYOUT_GENERAL);
	assert(layouts[static_cast<uint32_t>(BarrierLayout::VIDEO_ENCODE_WRITE)] == VK_IMAGE_LAYOUT_GENERAL);
	assert(layouts[static_cast<uint32_t>(BarrierLayout::DIRECT_QUEUE_COMMON)] == VK_IMAGE_LAYOUT_GENERAL);
	assert(layouts[static_cast<uint32_t>(BarrierLayout::DIRECT_QUEUE_GENERIC_READ)] == VK_IMAGE_LAYOUT_GENERAL);
	assert(layouts[static_cast<uint32_t>(BarrierLayout::DIRECT_QUEUE_UNORDERED_ACCESS)] == VK_IMAGE_LAYOUT_GENERAL);
	assert(layouts[static_cast<uint32_t>(BarrierLayout::DIRECT_QUEUE_SHADER_RESOURCE)] == VK_IMAGE_LAYOUT_GENERAL);
	assert(layouts[static_cast<uint32_t>(BarrierLayout::DIRECT_QUEUE_COPY_SOURCE)] == VK_IMAGE_LAYOUT_GENERAL);
	assert(layouts[static_cast<uint32_t>(BarrierLayout::DIRECT_QUEUE_COPY_DEST)] == VK_IMAGE_LAYOUT_GENERAL);
	assert(layouts[static_cast<uint32_t>(BarrierLayout::COMPUTE_QUEUE_COMMON)] == VK_IMAGE_LAYOUT_GENERAL);
	assert(layouts[static_cast<uint32_t>(BarrierLayout::COMPUTE_QUEUE_GENERIC_READ)] == VK_IMAGE_LAYOUT_GENERAL);
	assert(layouts[static_cast<uint32_t>(BarrierLayout::COMPUTE_QUEUE_UNORDERED_ACCESS)] == VK_IMAGE_LAYOUT_GENERAL);
	assert(layouts[static_cast<uint32_t>(BarrierLayout::COMPUTE_QUEUE_SHADER_RESOURCE)] == VK_IMAGE_LAYOUT_GENERAL);
	assert(layouts[static_cast<uint32_t>(BarrierLayout::COMPUTE_QUEUE_COPY_SOURCE)] == VK_IMAGE_LAYOUT_GENERAL);
	assert(layouts[static_cast<uint32_t>(BarrierLayout::COMPUTE_QUEUE_COPY_DEST)] == VK_IMAGE_LAYOUT_GENERAL);
	assert(layouts[static_cast<uint32_t>(BarrierLayout::VIDEO_QUEUE_COMMON)] == VK_IMAGE_LAYOUT_GENERAL);

	if (layout == BarrierLayout::UNDEFINED) return VK_IMAGE_LAYOUT_UNDEFINED;

	return layouts[static_cast<uint32_t>(layout)];
}

VkImageUsageFlags XUSG::GetVKImageUsage(ResourceFlag flags)
{
	VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	if ((flags & ResourceFlag::ALLOW_RENDER_TARGET) != ResourceFlag::NONE) usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	if ((flags & ResourceFlag::ALLOW_DEPTH_STENCIL) != ResourceFlag::NONE) usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	if ((flags & ResourceFlag::ALLOW_UNORDERED_ACCESS) != ResourceFlag::NONE) usage |= VK_IMAGE_USAGE_STORAGE_BIT;
	if ((flags & ResourceFlag::DENY_SHADER_RESOURCE) == ResourceFlag::NONE) usage |= VK_IMAGE_USAGE_SAMPLED_BIT;

	return usage;
}

VkBufferUsageFlags XUSG::GetVKBufferUsage(ResourceFlag flags)
{
	VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;

	if ((flags & ResourceFlag::ACCELERATION_STRUCTURE) != ResourceFlag::NONE)
		usage |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR;

	return usage;
}

VkImageAspectFlags XUSG::GetVKImageAspect(Format format)
{
	switch (format)
	{
	case Format::D32_FLOAT:
	case Format::D16_UNORM:
		return VK_IMAGE_ASPECT_DEPTH_BIT;
	case Format::D24_UNORM_S8_UINT:
	case Format::D32_FLOAT_S8X24_UINT:
		return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	default:
		return VK_IMAGE_ASPECT_COLOR_BIT;
	}
}

VkPipelineStageFlags2 XUSG::GetVKStage(ResourceState state)
{
	VkPipelineStageFlags2 stage = VK_PIPELINE_STAGE_2_NONE;
	if (state == ResourceState::COMMON) return VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	if ((state & ResourceState::VERTEX_AND_CONSTANT_BUFFER) != ResourceState::COMMON)
		stage |= VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
	if ((state & ResourceState::INDEX_BUFFER) != ResourceState::COMMON)
		stage |= VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT;
	if ((state & ResourceState::RENDER_TARGET) != ResourceState::COMMON)
		stage |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	if ((state & (ResourceState::DEPTH_WRITE | ResourceState::DEPTH_READ)) != ResourceState::COMMON)
		stage |= VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
	if ((state & ResourceState::NON_PIXEL_SHADER_RESOURCE) != ResourceState::COMMON)
		stage |= VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT |
		VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT | VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT |
		VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT;
	// PIXEL_SHADER_RESOURCE maps only to fragment shading (not ALL_GRAPHICS / ALL_COMMANDS).
	if ((state & ResourceState::PIXEL_SHADER_RESOURCE) != ResourceState::COMMON)
		stage |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
	if ((state & ResourceState::UNORDERED_ACCESS) != ResourceState::COMMON)
		stage |= VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
	if ((state & (ResourceState::COPY_SOURCE | ResourceState::COPY_DEST |
		ResourceState::RESOLVE_SOURCE | ResourceState::RESOLVE_DEST)) != ResourceState::COMMON)
		stage |= VK_PIPELINE_STAGE_2_COPY_BIT;
	if ((state & ResourceState::INDIRECT_ARGUMENT) != ResourceState::COMMON)
		stage |= VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
	if ((state & ResourceState::STREAM_OUT) != ResourceState::COMMON)
		stage |= VK_PIPELINE_STAGE_2_TRANSFORM_FEEDBACK_BIT_EXT;

	return stage ? stage : VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
}

VkAccessFlags2 XUSG::GetVKAccess(ResourceState state)
{
	VkAccessFlags2 access = VK_ACCESS_2_NONE;
	if (state == ResourceState::COMMON) return VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
	if ((state & ResourceState::VERTEX_AND_CONSTANT_BUFFER) != ResourceState::COMMON)
		access |= VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_2_UNIFORM_READ_BIT;
	if ((state & ResourceState::INDEX_BUFFER) != ResourceState::COMMON)
		access |= VK_ACCESS_2_INDEX_READ_BIT;
	if ((state & ResourceState::RENDER_TARGET) != ResourceState::COMMON)
		access |= VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
	if ((state & ResourceState::DEPTH_WRITE) != ResourceState::COMMON)
		access |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	if ((state & ResourceState::DEPTH_READ) != ResourceState::COMMON)
		access |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
	if ((state & (ResourceState::PIXEL_SHADER_RESOURCE | ResourceState::NON_PIXEL_SHADER_RESOURCE)) != ResourceState::COMMON)
		access |= VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
	if ((state & ResourceState::UNORDERED_ACCESS) != ResourceState::COMMON)
		access |= VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT | VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
	if ((state & (ResourceState::COPY_SOURCE | ResourceState::RESOLVE_SOURCE)) != ResourceState::COMMON)
		access |= VK_ACCESS_2_TRANSFER_READ_BIT;
	if ((state & (ResourceState::COPY_DEST | ResourceState::RESOLVE_DEST)) != ResourceState::COMMON)
		access |= VK_ACCESS_2_TRANSFER_WRITE_BIT;
	if ((state & ResourceState::INDIRECT_ARGUMENT) != ResourceState::COMMON)
		access |= VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
	if ((state & ResourceState::STREAM_OUT) != ResourceState::COMMON)
		access |= VK_ACCESS_2_TRANSFORM_FEEDBACK_WRITE_BIT_EXT |
		VK_ACCESS_2_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT |
		VK_ACCESS_2_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT;

	return access;
}

VkPrimitiveTopology XUSG::GetVKPrimitiveTopology(PrimitiveTopology topology)
{
	static const VkPrimitiveTopology topologies[] =
	{
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
		VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
		VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,
		VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY,
		VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY,
		VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
		VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
		VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
		VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
		VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
		VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
		VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
		VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
		VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
		VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
		VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
		VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
		VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
		VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
		VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
		VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
		VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
		VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
		VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
		VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
		VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
		VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
		VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
		VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
		VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
		VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
		VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
		VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
		VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
		VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
		VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
		VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
	};

	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::UNDEFINED)] == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::POINTLIST)] == VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::LINELIST)] == VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::LINESTRIP)] == VK_PRIMITIVE_TOPOLOGY_LINE_STRIP);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::TRIANGLELIST)] == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::TRIANGLESTRIP)] == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::TRIANGLEFAN)] == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::LINELIST_ADJ)] == VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::LINESTRIP_ADJ)] == VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::TRIANGLELIST_ADJ)] == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::TRIANGLESTRIP_ADJ)] == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::CONTROL_POINT1_PATCHLIST)] == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::CONTROL_POINT2_PATCHLIST)] == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::CONTROL_POINT3_PATCHLIST)] == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::CONTROL_POINT4_PATCHLIST)] == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::CONTROL_POINT5_PATCHLIST)] == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::CONTROL_POINT6_PATCHLIST)] == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::CONTROL_POINT7_PATCHLIST)] == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::CONTROL_POINT8_PATCHLIST)] == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::CONTROL_POINT9_PATCHLIST)] == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::CONTROL_POINT10_PATCHLIST)] == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::CONTROL_POINT11_PATCHLIST)] == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::CONTROL_POINT12_PATCHLIST)] == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::CONTROL_POINT13_PATCHLIST)] == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::CONTROL_POINT14_PATCHLIST)] == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::CONTROL_POINT15_PATCHLIST)] == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::CONTROL_POINT16_PATCHLIST)] == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::CONTROL_POINT17_PATCHLIST)] == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::CONTROL_POINT18_PATCHLIST)] == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::CONTROL_POINT19_PATCHLIST)] == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::CONTROL_POINT20_PATCHLIST)] == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::CONTROL_POINT21_PATCHLIST)] == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::CONTROL_POINT22_PATCHLIST)] == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::CONTROL_POINT23_PATCHLIST)] == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::CONTROL_POINT24_PATCHLIST)] == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::CONTROL_POINT25_PATCHLIST)] == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::CONTROL_POINT26_PATCHLIST)] == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::CONTROL_POINT27_PATCHLIST)] == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::CONTROL_POINT28_PATCHLIST)] == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::CONTROL_POINT29_PATCHLIST)] == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::CONTROL_POINT30_PATCHLIST)] == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::CONTROL_POINT31_PATCHLIST)] == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopology::CONTROL_POINT32_PATCHLIST)] == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);

	if (static_cast<uint32_t>(topology) >= static_cast<uint32_t>(sizeof(topologies) / sizeof(topologies[0])))
		return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;

	return topologies[static_cast<uint32_t>(topology)];
}

VkPrimitiveTopology XUSG::GetVKPrimitiveTopology(PrimitiveTopologyType type)
{
	static const VkPrimitiveTopology topologies[] =
	{
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
		VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
	};

	assert(topologies[static_cast<uint32_t>(PrimitiveTopologyType::UNDEFINED)] == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopologyType::POINT)] == VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopologyType::LINE)] == VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopologyType::TRIANGLE)] == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	assert(topologies[static_cast<uint32_t>(PrimitiveTopologyType::PATCH)] == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);

	return topologies[static_cast<uint32_t>(type)];
}

VkPolygonMode XUSG::GetVKPolygonMode(FillMode fillMode)
{
	static const VkPolygonMode fillModes[] =
	{
		VK_POLYGON_MODE_LINE,
		VK_POLYGON_MODE_FILL,
	};

	assert(fillModes[static_cast<uint32_t>(FillMode::WIREFRAME)] == VK_POLYGON_MODE_LINE);
	assert(fillModes[static_cast<uint32_t>(FillMode::SOLID)] == VK_POLYGON_MODE_FILL);

	return fillModes[static_cast<uint32_t>(fillMode)];
}

VkCullModeFlags XUSG::GetVKCullMode(CullMode cullMode)
{
	static const VkCullModeFlags cullModes[] =
	{
		VK_CULL_MODE_NONE,
		VK_CULL_MODE_FRONT_BIT,
		VK_CULL_MODE_BACK_BIT,
	};

	assert(cullModes[static_cast<uint32_t>(CullMode::NONE)] == VK_CULL_MODE_NONE);
	assert(cullModes[static_cast<uint32_t>(CullMode::FRONT)] == VK_CULL_MODE_FRONT_BIT);
	assert(cullModes[static_cast<uint32_t>(CullMode::BACK)] == VK_CULL_MODE_BACK_BIT);

	return cullModes[static_cast<uint32_t>(cullMode)];
}

VkBlendFactor XUSG::GetVKBlendFactor(BlendFactor factor)
{
	static const VkBlendFactor factors[] =
	{
		VK_BLEND_FACTOR_ZERO,
		VK_BLEND_FACTOR_ONE,
		VK_BLEND_FACTOR_SRC_COLOR,
		VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
		VK_BLEND_FACTOR_SRC_ALPHA,
		VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		VK_BLEND_FACTOR_DST_ALPHA,
		VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
		VK_BLEND_FACTOR_DST_COLOR,
		VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
		VK_BLEND_FACTOR_SRC_ALPHA_SATURATE,
		VK_BLEND_FACTOR_CONSTANT_COLOR,
		VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR,
		VK_BLEND_FACTOR_SRC1_COLOR,
		VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR,
		VK_BLEND_FACTOR_SRC1_ALPHA,
		VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA,
	};

	assert(factors[static_cast<uint32_t>(BlendFactor::ZERO)] == VK_BLEND_FACTOR_ZERO);
	assert(factors[static_cast<uint32_t>(BlendFactor::ONE)] == VK_BLEND_FACTOR_ONE);
	assert(factors[static_cast<uint32_t>(BlendFactor::SRC_COLOR)] == VK_BLEND_FACTOR_SRC_COLOR);
	assert(factors[static_cast<uint32_t>(BlendFactor::INV_SRC_COLOR)] == VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR);
	assert(factors[static_cast<uint32_t>(BlendFactor::SRC_ALPHA)] == VK_BLEND_FACTOR_SRC_ALPHA);
	assert(factors[static_cast<uint32_t>(BlendFactor::INV_SRC_ALPHA)] == VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
	assert(factors[static_cast<uint32_t>(BlendFactor::DEST_ALPHA)] == VK_BLEND_FACTOR_DST_ALPHA);
	assert(factors[static_cast<uint32_t>(BlendFactor::INV_DEST_ALPHA)] == VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA);
	assert(factors[static_cast<uint32_t>(BlendFactor::DEST_COLOR)] == VK_BLEND_FACTOR_DST_COLOR);
	assert(factors[static_cast<uint32_t>(BlendFactor::INV_DEST_COLOR)] == VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR);
	assert(factors[static_cast<uint32_t>(BlendFactor::SRC_ALPHA_SAT)] == VK_BLEND_FACTOR_SRC_ALPHA_SATURATE);
	assert(factors[static_cast<uint32_t>(BlendFactor::BLEND_FACTOR)] == VK_BLEND_FACTOR_CONSTANT_COLOR);
	assert(factors[static_cast<uint32_t>(BlendFactor::INV_BLEND_FACTOR)] == VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR);
	assert(factors[static_cast<uint32_t>(BlendFactor::SRC1_COLOR)] == VK_BLEND_FACTOR_SRC1_COLOR);
	assert(factors[static_cast<uint32_t>(BlendFactor::INV_SRC1_COLOR)] == VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR);
	assert(factors[static_cast<uint32_t>(BlendFactor::SRC1_ALPHA)] == VK_BLEND_FACTOR_SRC1_ALPHA);
	assert(factors[static_cast<uint32_t>(BlendFactor::INV_SRC1_ALPHA)] == VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA);

	return factors[static_cast<uint32_t>(factor)];
}

VkBlendOp XUSG::GetVKBlendOp(BlendOperator op)
{
	static const VkBlendOp ops[] =
	{
		VK_BLEND_OP_ADD,
		VK_BLEND_OP_SUBTRACT,
		VK_BLEND_OP_REVERSE_SUBTRACT,
		VK_BLEND_OP_MIN,
		VK_BLEND_OP_MAX,
	};

	assert(ops[static_cast<uint32_t>(BlendOperator::ADD)] == VK_BLEND_OP_ADD);
	assert(ops[static_cast<uint32_t>(BlendOperator::SUBTRACT)] == VK_BLEND_OP_SUBTRACT);
	assert(ops[static_cast<uint32_t>(BlendOperator::REV_SUBTRACT)] == VK_BLEND_OP_REVERSE_SUBTRACT);
	assert(ops[static_cast<uint32_t>(BlendOperator::MIN)] == VK_BLEND_OP_MIN);
	assert(ops[static_cast<uint32_t>(BlendOperator::MAX)] == VK_BLEND_OP_MAX);

	return ops[static_cast<uint32_t>(op)];
}

VkLogicOp XUSG::GetVKLogicOp(LogicOperator op)
{
	static const VkLogicOp ops[] =
	{
		VK_LOGIC_OP_CLEAR,
		VK_LOGIC_OP_SET,
		VK_LOGIC_OP_COPY,
		VK_LOGIC_OP_COPY_INVERTED,
		VK_LOGIC_OP_NO_OP,
		VK_LOGIC_OP_INVERT,
		VK_LOGIC_OP_AND,
		VK_LOGIC_OP_NAND,
		VK_LOGIC_OP_OR,
		VK_LOGIC_OP_NOR,
		VK_LOGIC_OP_XOR,
		VK_LOGIC_OP_EQUIVALENT,
		VK_LOGIC_OP_AND_REVERSE,
		VK_LOGIC_OP_AND_INVERTED,
		VK_LOGIC_OP_OR_REVERSE,
		VK_LOGIC_OP_OR_INVERTED,
	};

	assert(ops[static_cast<uint32_t>(LogicOperator::CLEAR)] == VK_LOGIC_OP_CLEAR);
	assert(ops[static_cast<uint32_t>(LogicOperator::SET)] == VK_LOGIC_OP_SET);
	assert(ops[static_cast<uint32_t>(LogicOperator::COPY)] == VK_LOGIC_OP_COPY);
	assert(ops[static_cast<uint32_t>(LogicOperator::COPY_INVERTED)] == VK_LOGIC_OP_COPY_INVERTED);
	assert(ops[static_cast<uint32_t>(LogicOperator::NOOP)] == VK_LOGIC_OP_NO_OP);
	assert(ops[static_cast<uint32_t>(LogicOperator::INVERT)] == VK_LOGIC_OP_INVERT);
	assert(ops[static_cast<uint32_t>(LogicOperator::AND)] == VK_LOGIC_OP_AND);
	assert(ops[static_cast<uint32_t>(LogicOperator::NAND)] == VK_LOGIC_OP_NAND);
	assert(ops[static_cast<uint32_t>(LogicOperator::OR)] == VK_LOGIC_OP_OR);
	assert(ops[static_cast<uint32_t>(LogicOperator::NOR)] == VK_LOGIC_OP_NOR);
	assert(ops[static_cast<uint32_t>(LogicOperator::XOR)] == VK_LOGIC_OP_XOR);
	assert(ops[static_cast<uint32_t>(LogicOperator::EQUIV)] == VK_LOGIC_OP_EQUIVALENT);
	assert(ops[static_cast<uint32_t>(LogicOperator::AND_REVERSE)] == VK_LOGIC_OP_AND_REVERSE);
	assert(ops[static_cast<uint32_t>(LogicOperator::AND_INVERTED)] == VK_LOGIC_OP_AND_INVERTED);
	assert(ops[static_cast<uint32_t>(LogicOperator::OR_REVERSE)] == VK_LOGIC_OP_OR_REVERSE);
	assert(ops[static_cast<uint32_t>(LogicOperator::OR_INVERTED)] == VK_LOGIC_OP_OR_INVERTED);

	return ops[static_cast<uint32_t>(op)];
}

VkCompareOp XUSG::GetVKCompareOp(ComparisonFunc func)
{
	static const VkCompareOp ops[] =
	{
		VK_COMPARE_OP_NEVER,
		VK_COMPARE_OP_LESS,
		VK_COMPARE_OP_EQUAL,
		VK_COMPARE_OP_LESS_OR_EQUAL,
		VK_COMPARE_OP_GREATER,
		VK_COMPARE_OP_NOT_EQUAL,
		VK_COMPARE_OP_GREATER_OR_EQUAL,
		VK_COMPARE_OP_ALWAYS,
	};

	assert(ops[static_cast<uint32_t>(ComparisonFunc::NEVER)] == VK_COMPARE_OP_NEVER);
	assert(ops[static_cast<uint32_t>(ComparisonFunc::LESS)] == VK_COMPARE_OP_LESS);
	assert(ops[static_cast<uint32_t>(ComparisonFunc::EQUAL)] == VK_COMPARE_OP_EQUAL);
	assert(ops[static_cast<uint32_t>(ComparisonFunc::LESS_EQUAL)] == VK_COMPARE_OP_LESS_OR_EQUAL);
	assert(ops[static_cast<uint32_t>(ComparisonFunc::GREATER)] == VK_COMPARE_OP_GREATER);
	assert(ops[static_cast<uint32_t>(ComparisonFunc::NOT_EQUAL)] == VK_COMPARE_OP_NOT_EQUAL);
	assert(ops[static_cast<uint32_t>(ComparisonFunc::GREATER_EQUAL)] == VK_COMPARE_OP_GREATER_OR_EQUAL);
	assert(ops[static_cast<uint32_t>(ComparisonFunc::ALWAYS)] == VK_COMPARE_OP_ALWAYS);

	return ops[static_cast<uint32_t>(func)];
}

VkStencilOp XUSG::GetVKStencilOp(StencilOp op)
{
	static const VkStencilOp ops[] =
	{
		VK_STENCIL_OP_KEEP,
		VK_STENCIL_OP_ZERO,
		VK_STENCIL_OP_REPLACE,
		VK_STENCIL_OP_INCREMENT_AND_CLAMP,
		VK_STENCIL_OP_DECREMENT_AND_CLAMP,
		VK_STENCIL_OP_INVERT,
		VK_STENCIL_OP_INCREMENT_AND_WRAP,
		VK_STENCIL_OP_DECREMENT_AND_WRAP,
	};

	assert(ops[static_cast<uint32_t>(StencilOp::KEEP)] == VK_STENCIL_OP_KEEP);
	assert(ops[static_cast<uint32_t>(StencilOp::ZERO)] == VK_STENCIL_OP_ZERO);
	assert(ops[static_cast<uint32_t>(StencilOp::REPLACE)] == VK_STENCIL_OP_REPLACE);
	assert(ops[static_cast<uint32_t>(StencilOp::INCR_SAT)] == VK_STENCIL_OP_INCREMENT_AND_CLAMP);
	assert(ops[static_cast<uint32_t>(StencilOp::DECR_SAT)] == VK_STENCIL_OP_DECREMENT_AND_CLAMP);
	assert(ops[static_cast<uint32_t>(StencilOp::INVERT)] == VK_STENCIL_OP_INVERT);
	assert(ops[static_cast<uint32_t>(StencilOp::INCR)] == VK_STENCIL_OP_INCREMENT_AND_WRAP);
	assert(ops[static_cast<uint32_t>(StencilOp::DECR)] == VK_STENCIL_OP_DECREMENT_AND_WRAP);

	return ops[static_cast<uint32_t>(op)];
}

VkFilter XUSG::GetVKFilter(SamplerFilter filter, bool minFilter, bool magFilter)
{
	// SamplerFilter packs min/mag/mip as sequential bits (not D3D12_FILTER encoding).
	constexpr auto groupSize = static_cast<uint8_t>(SamplerFilter::ANISOTROPIC) + 1;
	constexpr auto minLinearBit = static_cast<uint8_t>(SamplerFilter::MIN_LINEAR_MAG_MIP_POINT);
	constexpr auto magLinearBit = static_cast<uint8_t>(SamplerFilter::MIN_POINT_MAG_LINEAR_MIP_POINT);
	constexpr auto mipLinearBit = static_cast<uint8_t>(SamplerFilter::MIN_MAG_POINT_MIP_LINEAR);
	static_assert(minLinearBit == 4 && magLinearBit == 2 && mipLinearBit == 1 && groupSize == 9);

	const auto index = static_cast<uint8_t>(filter) % groupSize;
	if (index == static_cast<uint8_t>(SamplerFilter::ANISOTROPIC)) return VK_FILTER_LINEAR;

	const auto mask = minFilter ? minLinearBit : magFilter ? magLinearBit : mipLinearBit;

	return (index & mask) ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
}

VkSamplerMipmapMode XUSG::GetVKMipmapMode(SamplerFilter filter)
{
	constexpr auto groupSize = static_cast<uint8_t>(SamplerFilter::ANISOTROPIC) + 1;
	constexpr auto mipLinearBit = static_cast<uint8_t>(SamplerFilter::MIN_MAG_POINT_MIP_LINEAR);

	const auto index = static_cast<uint8_t>(filter) % groupSize;
	if (index == static_cast<uint8_t>(SamplerFilter::ANISOTROPIC))
		return VK_SAMPLER_MIPMAP_MODE_LINEAR;

	return (index & mipLinearBit) ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
}

bool XUSG::IsAnisotropicSamplerFilter(SamplerFilter filter)
{
	constexpr auto groupSize = static_cast<uint8_t>(SamplerFilter::ANISOTROPIC) + 1;

	return (static_cast<uint8_t>(filter) % groupSize) == static_cast<uint8_t>(SamplerFilter::ANISOTROPIC);
}

SamplerFilter XUSG::GetSamplerFilterFromRootSig(uint32_t encodedFilter)
{
	// Same order as GetDX12Filter. Matches D3D12_DECODE_* used by vkd3d-proton.
	static const uint32_t filters[] =
	{
		0x00, 0x01, 0x04, 0x05, 0x10, 0x11, 0x14, 0x15, 0x55,
		0x80, 0x81, 0x84, 0x85, 0x90, 0x91, 0x94, 0x95, 0xd5,
		0x100, 0x101, 0x104, 0x105, 0x110, 0x111, 0x114, 0x115, 0x155,
		0x180, 0x181, 0x184, 0x185, 0x190, 0x191, 0x194, 0x195, 0x1d5,
	};
	static_assert(sizeof(filters) / sizeof(filters[0]) ==
		static_cast<uint32_t>(SamplerFilter::MAXIMUM_ANISOTROPIC) + 1);

	for (auto i = 0u; i < sizeof(filters) / sizeof(filters[0]); ++i)
		if (filters[i] == encodedFilter) return static_cast<SamplerFilter>(i);

	// *_MIN_MAG_ANISOTROPIC_MIP_POINT (0x54 etc.): still anisotropic (aniso bit +
	// linear min/mag). Current SDK DECODE_IS_ANISOTROPIC no longer requires mip LINEAR;
	// ENCODE_MIN_MAG_ANISOTROPIC_MIP_POINT_FILTER exists for a reason. SamplerFilter has
	// no separate mip-point anisotropic slot, so map to full anisotropic (LINEAR mip).
	switch (encodedFilter)
	{
	case 0x54: return SamplerFilter::ANISOTROPIC;
	case 0xd4: return SamplerFilter::COMPARISON_ANISOTROPIC;
	case 0x154: return SamplerFilter::MINIMUM_ANISOTROPIC;
	case 0x1d4: return SamplerFilter::MAXIMUM_ANISOTROPIC;
	default:
		assert(!"Unknown encoded filter in root signature");
		return SamplerFilter::MIN_MAG_MIP_POINT;
	}
}

VkSamplerAddressMode XUSG::GetVKSamplerAddressMode(TextureAddressMode mode)
{
	static const VkSamplerAddressMode modes[] =
	{
		VK_SAMPLER_ADDRESS_MODE_REPEAT,
		VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
		VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE,
	};

	assert(modes[static_cast<uint32_t>(TextureAddressMode::WRAP)] == VK_SAMPLER_ADDRESS_MODE_REPEAT);
	assert(modes[static_cast<uint32_t>(TextureAddressMode::MIRROR)] == VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT);
	assert(modes[static_cast<uint32_t>(TextureAddressMode::CLAMP)] == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	assert(modes[static_cast<uint32_t>(TextureAddressMode::BORDER)] == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
	assert(modes[static_cast<uint32_t>(TextureAddressMode::MIRROR_ONCE)] == VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE);

	return modes[static_cast<uint32_t>(mode)];
}

VkSamplerReductionMode XUSG::GetVKReductionMode(SamplerFilter filter)
{
	const auto value = static_cast<uint8_t>(filter);
	if (value >= static_cast<uint8_t>(SamplerFilter::MINIMUM_MIN_MAG_MIP_POINT) &&
		value <= static_cast<uint8_t>(SamplerFilter::MINIMUM_ANISOTROPIC))
		return VK_SAMPLER_REDUCTION_MODE_MIN;
	if (value >= static_cast<uint8_t>(SamplerFilter::MAXIMUM_MIN_MAG_MIP_POINT))
		return VK_SAMPLER_REDUCTION_MODE_MAX;

	return VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE;
}

VkComponentSwizzle XUSG::GetVKComponentSwizzle(SrvComponentMapping mapping)
{
	static const VkComponentSwizzle swizzles[] =
	{
		VK_COMPONENT_SWIZZLE_R,
		VK_COMPONENT_SWIZZLE_G,
		VK_COMPONENT_SWIZZLE_B,
		VK_COMPONENT_SWIZZLE_A,
		VK_COMPONENT_SWIZZLE_ZERO,
		VK_COMPONENT_SWIZZLE_ONE,
	};

	assert(swizzles[static_cast<uint32_t>(SrvComponentMapping::MEMORY_COMPONENT_0)] == VK_COMPONENT_SWIZZLE_R);
	assert(swizzles[static_cast<uint32_t>(SrvComponentMapping::MEMORY_COMPONENT_1)] == VK_COMPONENT_SWIZZLE_G);
	assert(swizzles[static_cast<uint32_t>(SrvComponentMapping::MEMORY_COMPONENT_2)] == VK_COMPONENT_SWIZZLE_B);
	assert(swizzles[static_cast<uint32_t>(SrvComponentMapping::MEMORY_COMPONENT_3)] == VK_COMPONENT_SWIZZLE_A);
	assert(swizzles[static_cast<uint32_t>(SrvComponentMapping::FORCE_VALUE_0)] == VK_COMPONENT_SWIZZLE_ZERO);
	assert(swizzles[static_cast<uint32_t>(SrvComponentMapping::FORCE_VALUE_1)] == VK_COMPONENT_SWIZZLE_ONE);
	assert(swizzles[static_cast<uint32_t>(SrvComponentMapping::MC0)] == VK_COMPONENT_SWIZZLE_R);
	assert(swizzles[static_cast<uint32_t>(SrvComponentMapping::MC1)] == VK_COMPONENT_SWIZZLE_G);
	assert(swizzles[static_cast<uint32_t>(SrvComponentMapping::MC2)] == VK_COMPONENT_SWIZZLE_B);
	assert(swizzles[static_cast<uint32_t>(SrvComponentMapping::MC3)] == VK_COMPONENT_SWIZZLE_A);
	assert(swizzles[static_cast<uint32_t>(SrvComponentMapping::FV0)] == VK_COMPONENT_SWIZZLE_ZERO);
	assert(swizzles[static_cast<uint32_t>(SrvComponentMapping::FV1)] == VK_COMPONENT_SWIZZLE_ONE);

	return swizzles[static_cast<uint32_t>(mapping)];
}

VkIndexType XUSG::GetVKIndexType(Format format)
{
	return format == Format::R16_UINT ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
}

VkQueryType XUSG::GetVKQueryType(QueryType type)
{
	static const VkQueryType types[] =
	{
		VK_QUERY_TYPE_OCCLUSION,
		VK_QUERY_TYPE_OCCLUSION,
		VK_QUERY_TYPE_TIMESTAMP,
		VK_QUERY_TYPE_PIPELINE_STATISTICS,
		VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT,
		VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT,
		VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT,
		VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT,
		VK_QUERY_TYPE_OCCLUSION,
	};

	assert(types[static_cast<uint32_t>(QueryType::OCCLUSION)] == VK_QUERY_TYPE_OCCLUSION);
	assert(types[static_cast<uint32_t>(QueryType::BINARY_OCCLUSION)] == VK_QUERY_TYPE_OCCLUSION);
	assert(types[static_cast<uint32_t>(QueryType::TIMESTAMP)] == VK_QUERY_TYPE_TIMESTAMP);
	assert(types[static_cast<uint32_t>(QueryType::PIPELINE_STATISTICS)] == VK_QUERY_TYPE_PIPELINE_STATISTICS);
	assert(types[static_cast<uint32_t>(QueryType::SO_STATISTICS_STREAM0)] == VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT);
	assert(types[static_cast<uint32_t>(QueryType::SO_STATISTICS_STREAM1)] == VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT);
	assert(types[static_cast<uint32_t>(QueryType::SO_STATISTICS_STREAM2)] == VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT);
	assert(types[static_cast<uint32_t>(QueryType::SO_STATISTICS_STREAM3)] == VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT);
	assert(types[static_cast<uint32_t>(QueryType::VIDEO_DECODE_STATISTICS)] == VK_QUERY_TYPE_OCCLUSION);

	return types[static_cast<uint32_t>(type)];
}

VkShaderStageFlagBits XUSG::GetVKShaderStage(Shader::Stage stage)
{
	static const VkShaderStageFlagBits stages[] =
	{
		VK_SHADER_STAGE_FRAGMENT_BIT,
		VK_SHADER_STAGE_VERTEX_BIT,
		VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
		VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
		VK_SHADER_STAGE_GEOMETRY_BIT,
		VK_SHADER_STAGE_COMPUTE_BIT,
		VK_SHADER_STAGE_MESH_BIT_EXT,
		VK_SHADER_STAGE_TASK_BIT_EXT,
	};

	assert(stages[static_cast<uint32_t>(Shader::Stage::PS)] == VK_SHADER_STAGE_FRAGMENT_BIT);
	assert(stages[static_cast<uint32_t>(Shader::Stage::VS)] == VK_SHADER_STAGE_VERTEX_BIT);
	assert(stages[static_cast<uint32_t>(Shader::Stage::DS)] == VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
	assert(stages[static_cast<uint32_t>(Shader::Stage::HS)] == VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
	assert(stages[static_cast<uint32_t>(Shader::Stage::GS)] == VK_SHADER_STAGE_GEOMETRY_BIT);
	assert(stages[static_cast<uint32_t>(Shader::Stage::CS)] == VK_SHADER_STAGE_COMPUTE_BIT);
	assert(stages[static_cast<uint32_t>(Shader::Stage::ALL)] == VK_SHADER_STAGE_COMPUTE_BIT);
	assert(stages[static_cast<uint32_t>(Shader::Stage::NUM_GRAPHICS)] == VK_SHADER_STAGE_COMPUTE_BIT);
	assert(stages[static_cast<uint32_t>(Shader::Stage::MS)] == VK_SHADER_STAGE_MESH_BIT_EXT);
	assert(stages[static_cast<uint32_t>(Shader::Stage::AS)] == VK_SHADER_STAGE_TASK_BIT_EXT);

	return stages[static_cast<uint32_t>(stage)];
}

VkShaderStageFlags XUSG::GetVKShaderStageFlags(Shader::Stage stage)
{
	if (stage == Shader::Stage::ALL) return VK_SHADER_STAGE_ALL;

	return GetVKShaderStage(stage);
}

uint32_t XUSG::GetVKFormatBits(Format format)
{
	switch (format)
	{
	case Format::R32G32B32A32_FLOAT:
	case Format::R32G32B32A32_UINT:
	case Format::R32G32B32A32_SINT:
		return 128;
	case Format::R32G32B32_FLOAT:
	case Format::R32G32B32_UINT:
	case Format::R32G32B32_SINT:
		return 96;
	case Format::R16G16B16A16_FLOAT:
	case Format::R16G16B16A16_UNORM:
	case Format::R16G16B16A16_UINT:
	case Format::R16G16B16A16_SNORM:
	case Format::R16G16B16A16_SINT:
	case Format::R32G32_FLOAT:
	case Format::R32G32_UINT:
	case Format::R32G32_SINT:
	case Format::D32_FLOAT_S8X24_UINT:
		return 64;
	case Format::R10G10B10A2_UNORM:
	case Format::R10G10B10A2_UINT:
	case Format::R11G11B10_FLOAT:
	case Format::R8G8B8A8_UNORM:
	case Format::R8G8B8A8_UNORM_SRGB:
	case Format::R8G8B8A8_UINT:
	case Format::R8G8B8A8_SNORM:
	case Format::R8G8B8A8_SINT:
	case Format::R16G16_FLOAT:
	case Format::R16G16_UNORM:
	case Format::R16G16_UINT:
	case Format::R16G16_SNORM:
	case Format::R16G16_SINT:
	case Format::R32_FLOAT:
	case Format::R32_UINT:
	case Format::R32_SINT:
	case Format::D32_FLOAT:
	case Format::D24_UNORM_S8_UINT:
	case Format::B8G8R8A8_UNORM:
	case Format::B8G8R8A8_UNORM_SRGB:
		return 32;
	case Format::R8G8_UNORM:
	case Format::R8G8_UINT:
	case Format::R8G8_SNORM:
	case Format::R8G8_SINT:
	case Format::R16_FLOAT:
	case Format::R16_UNORM:
	case Format::R16_UINT:
	case Format::R16_SNORM:
	case Format::R16_SINT:
	case Format::D16_UNORM:
	case Format::B5G6R5_UNORM:
	case Format::B5G5R5A1_UNORM:
	case Format::B4G4R4A4_UNORM:
		return 16;
	case Format::R8_UNORM:
	case Format::R8_UINT:
	case Format::R8_SNORM:
	case Format::R8_SINT:
	case Format::A8_UNORM:
		return 8;
	case Format::BC1_UNORM:
	case Format::BC1_UNORM_SRGB:
	case Format::BC4_UNORM:
	case Format::BC4_SNORM:
		return 4;
	case Format::BC2_UNORM:
	case Format::BC2_UNORM_SRGB:
	case Format::BC3_UNORM:
	case Format::BC3_UNORM_SRGB:
	case Format::BC5_UNORM:
	case Format::BC5_SNORM:
	case Format::BC6H_UF16:
	case Format::BC6H_SF16:
	case Format::BC7_UNORM:
	case Format::BC7_UNORM_SRGB:
		return 8;
	default:
		return 32;
	}
}

bool XUSG::IsDepthStencilFormat(Format format)
{
	switch (format)
	{
	case Format::D32_FLOAT:
	case Format::D16_UNORM:
	case Format::D24_UNORM_S8_UINT:
	case Format::D32_FLOAT_S8X24_UINT:
		return true;
	default:
		return false;
	}
}

bool XUSG::IsCompressedFormat(Format format)
{
	switch (format)
	{
	case Format::BC1_TYPELESS:
	case Format::BC1_UNORM:
	case Format::BC1_UNORM_SRGB:
	case Format::BC2_TYPELESS:
	case Format::BC2_UNORM:
	case Format::BC2_UNORM_SRGB:
	case Format::BC3_TYPELESS:
	case Format::BC3_UNORM:
	case Format::BC3_UNORM_SRGB:
	case Format::BC4_TYPELESS:
	case Format::BC4_UNORM:
	case Format::BC4_SNORM:
	case Format::BC5_TYPELESS:
	case Format::BC5_UNORM:
	case Format::BC5_SNORM:
	case Format::BC6H_TYPELESS:
	case Format::BC6H_UF16:
	case Format::BC6H_SF16:
	case Format::BC7_TYPELESS:
	case Format::BC7_UNORM:
	case Format::BC7_UNORM_SRGB:
		return true;
	default:
		return false;
	}
}
