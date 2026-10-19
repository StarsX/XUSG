//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSG_VK.h"

namespace XUSG
{
	VkFormat GetVKFormat(Format format);
	VkFormat GetVKFormat(const Device_VK* pDevice, Format format);
	Format GetFormatFromVK(VkFormat format);

	VkImageType GetVKImageType(ResourceDimension dimension);
	VkImageViewType GetVKImageViewType(ResourceDimension dimension, bool isArray, bool isCube, bool multisample);
	VkImageLayout GetVKImageLayout(ResourceState state);
	VkImageUsageFlags GetVKImageUsage(ResourceFlag flags);
	VkBufferUsageFlags GetVKBufferUsage(const Device_VK* pDevice, ResourceFlag flags);
	VkImageAspectFlags GetVKImageAspect(Format format);

	VkPipelineStageFlags2 GetVKStage(const Device_VK* pDevice, ResourceState state);
	VkAccessFlags2 GetVKAccess(const Device_VK* pDevice, ResourceState state);

	VkPrimitiveTopology GetVKPrimitiveTopology(PrimitiveTopology topology);
	VkPrimitiveTopology GetVKPrimitiveTopology(PrimitiveTopologyType type);
	VkPolygonMode GetVKPolygonMode(FillMode fillMode);
	VkCullModeFlags GetVKCullMode(CullMode cullMode);
	VkBlendFactor GetVKBlendFactor(BlendFactor factor);
	VkBlendOp GetVKBlendOp(BlendOperator op);
	VkLogicOp GetVKLogicOp(LogicOperator op);
	VkCompareOp GetVKCompareOp(ComparisonFunc func);
	VkStencilOp GetVKStencilOp(StencilOp op);
	VkFilter GetVKFilter(SamplerFilter filter, bool minFilter, bool magFilter);
	VkSamplerMipmapMode GetVKMipmapMode(SamplerFilter filter);
	VkSamplerAddressMode GetVKSamplerAddressMode(TextureAddressMode mode);
	VkSamplerReductionMode GetVKReductionMode(SamplerFilter filter);
	// Map RTS0 root-sig filter dword (D3D12 bit encoding) to sequential SamplerFilter.
	SamplerFilter GetSamplerFilterFromRootSig(uint32_t encodedFilter);
	bool IsAnisotropicSamplerFilter(SamplerFilter filter);
	VkComponentSwizzle GetVKComponentSwizzle(SrvComponentMapping mapping);
	VkIndexType GetVKIndexType(Format format);
	VkQueryType GetVKQueryType(QueryType type);
	VkShaderStageFlagBits GetVKShaderStage(Shader::Stage stage);
	VkShaderStageFlags GetVKShaderStageFlags(Shader::Stage stage);

	uint32_t GetVKFormatBits(Format format);
	bool IsDepthStencilFormat(Format format);
	bool IsCompressedFormat(Format format);

	VkImageLayout GetVKBarrierLayout(BarrierLayout layout);
}
