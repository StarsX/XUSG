//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGPipelineLayout_VK.h"
#include "XUSGEnum_VK.h"

using namespace std;
using namespace XUSG;

namespace
{
	VkSpirvResourceTypeFlagsEXT ResourceMask(DescriptorType type)
	{
		switch (type)
		{
		case DescriptorType::SRV:
			return VK_SPIRV_RESOURCE_TYPE_SAMPLED_IMAGE_BIT_EXT | VK_SPIRV_RESOURCE_TYPE_READ_ONLY_IMAGE_BIT_EXT |
				VK_SPIRV_RESOURCE_TYPE_READ_ONLY_STORAGE_BUFFER_BIT_EXT;
		case DescriptorType::UAV:
			return VK_SPIRV_RESOURCE_TYPE_READ_WRITE_IMAGE_BIT_EXT | VK_SPIRV_RESOURCE_TYPE_READ_WRITE_STORAGE_BUFFER_BIT_EXT;
		case DescriptorType::CBV:
		case DescriptorType::CONSTANT:
		case DescriptorType::ROOT_CBV:
			return VK_SPIRV_RESOURCE_TYPE_UNIFORM_BUFFER_BIT_EXT;
		case DescriptorType::SAMPLER:
			return VK_SPIRV_RESOURCE_TYPE_SAMPLER_BIT_EXT;
		case DescriptorType::ROOT_SRV:
			return VK_SPIRV_RESOURCE_TYPE_READ_ONLY_STORAGE_BUFFER_BIT_EXT;
		case DescriptorType::ROOT_UAV:
			return VK_SPIRV_RESOURCE_TYPE_READ_WRITE_STORAGE_BUFFER_BIT_EXT;
		default:
			return 0;
		}
	}
}

Util::PipelineLayout_VK::PipelineLayout_VK() :
	m_descriptorTableLayoutKeys(0),
	m_staticSamplers(0),
	m_isTableLayoutsCompleted(false)
{
	m_pipelineLayoutKey.resize(1);
}

Util::PipelineLayout_VK::~PipelineLayout_VK()
{
}

void Util::PipelineLayout_VK::SetShaderStage(uint32_t index, Shader::Stage stage)
{
	checkKeyStorage(index)[0] = stage;
}

void Util::PipelineLayout_VK::SetRange(uint32_t index, DescriptorType type, uint32_t num, uint32_t baseBinding,
	uint32_t space, DescriptorFlag flags, uint32_t offsetInDescriptors)
{
	auto& key = checkKeyStorage(index);

	// Append
	const auto i = (key.size() - 1) / sizeof(DescriptorRange);
	key.resize(key.size() + sizeof(DescriptorRange));

	// Interpret key data as range
	auto& range = reinterpret_cast<DescriptorRange*>(&key[1])[i];

	// Fill key entries
	range.Type = type;
	range.NumDescriptors = num;
	range.BaseBinding = baseBinding;
	range.Space = space;
	range.Flags = flags;
	range.OffsetInDescriptors = offsetInDescriptors;
}

void Util::PipelineLayout_VK::SetConstants(uint32_t index, uint32_t num32BitValues,
	uint32_t binding, uint32_t space, Shader::Stage stage)
{
	SetRange(index, DescriptorType::CONSTANT, num32BitValues, binding, space, DescriptorFlag::NONE);
	SetShaderStage(index, stage);
}

void Util::PipelineLayout_VK::SetRootSRV(uint32_t index, uint32_t binding, uint32_t space,
	DescriptorFlag flags, Shader::Stage stage)
{
	SetRange(index, DescriptorType::ROOT_SRV, 1, binding, space, flags);
	SetShaderStage(index, stage);
}

void Util::PipelineLayout_VK::SetRootUAV(uint32_t index, uint32_t binding, uint32_t space,
	DescriptorFlag flags, Shader::Stage stage)
{
	SetRange(index, DescriptorType::ROOT_UAV, 1, binding, space, flags);
	SetShaderStage(index, stage);
}

void Util::PipelineLayout_VK::SetRootCBV(uint32_t index, uint32_t binding, uint32_t space, Shader::Stage stage)
{
	SetRange(index, DescriptorType::ROOT_CBV, 1, binding, space, DescriptorFlag::DATA_STATIC);
	SetShaderStage(index, stage);
}

void Util::PipelineLayout_VK::SetStaticSamplers(const Sampler* const* ppSamplers,
	uint32_t num, uint32_t baseBinding, uint32_t space, Shader::Stage stage)
{
	const auto startIdx = static_cast<uint32_t>(m_staticSamplers.size());
	m_staticSamplers.resize(startIdx + num);

	for (auto i = 0u; i < num; ++i)
	{
		auto& staticSampler = m_staticSamplers[startIdx + i];
		staticSampler.Binding = baseBinding + i;
		staticSampler.Space = space;
		staticSampler.pSampler = ppSamplers[i];
		staticSampler.Stage = stage;
	}
}

PipelineLayout Util::PipelineLayout_VK::CreatePipelineLayout(PipelineLayoutLib* pPipelineLayoutLib,
	PipelineLayoutFlag flags, const wchar_t* name)
{
	return pPipelineLayoutLib->CreatePipelineLayout(this, flags, name);
}

PipelineLayout Util::PipelineLayout_VK::GetPipelineLayout(PipelineLayoutLib* pPipelineLayoutLib,
	PipelineLayoutFlag flags, const wchar_t* name)
{
	return pPipelineLayoutLib->GetPipelineLayout(this, flags, name);
}

DescriptorTableLayout Util::PipelineLayout_VK::CreateDescriptorTableLayout(uint32_t index,
	PipelineLayoutLib* pPipelineLayoutLib) const
{
	return pPipelineLayoutLib->CreateDescriptorTableLayout(index, this);
}

DescriptorTableLayout Util::PipelineLayout_VK::GetDescriptorTableLayout(uint32_t index,
	PipelineLayoutLib* pPipelineLayoutLib) const
{
	return pPipelineLayoutLib->GetDescriptorTableLayout(index, this);
}

const vector<string>& Util::PipelineLayout_VK::GetDescriptorTableLayoutKeys() const
{
	return m_descriptorTableLayoutKeys;
}

string& Util::PipelineLayout_VK::GetPipelineLayoutKey(PipelineLayoutLib* pPipelineLayoutLib)
{
	if (!m_isTableLayoutsCompleted && pPipelineLayoutLib)
	{
		const auto staticSamplerKeyOffset = DescriptorTableLayoutOffset + sizeof(DescriptorTableLayout) * m_descriptorTableLayoutKeys.size();
		m_pipelineLayoutKey.resize(staticSamplerKeyOffset + sizeof(StaticSampler) * m_staticSamplers.size());

		auto& descriptorTableLayoutCount = reinterpret_cast<uint16_t&>(m_pipelineLayoutKey[DescriptorTableLayoutCountOffset]);
		const auto pDescriptorTableLayouts = reinterpret_cast<DescriptorTableLayout*>(&m_pipelineLayoutKey[DescriptorTableLayoutOffset]);
		const auto pStaticSamplers = reinterpret_cast<StaticSampler*>(&m_pipelineLayoutKey[staticSamplerKeyOffset]);

		descriptorTableLayoutCount = static_cast<uint16_t>(m_descriptorTableLayoutKeys.size());
		for (auto i = 0u; i < descriptorTableLayoutCount; ++i)
			pDescriptorTableLayouts[i] = GetDescriptorTableLayout(i, pPipelineLayoutLib);

		for (auto i = 0u; i < m_staticSamplers.size(); ++i)
			pStaticSamplers[i] = m_staticSamplers[i];

		m_isTableLayoutsCompleted = true;
	}

	return m_pipelineLayoutKey;
}

string& Util::PipelineLayout_VK::checkKeyStorage(uint32_t index)
{
	m_isTableLayoutsCompleted = false;

	if (index >= m_descriptorTableLayoutKeys.size())
		m_descriptorTableLayoutKeys.resize(index + 1);

	if (m_descriptorTableLayoutKeys[index].empty())
	{
		m_descriptorTableLayoutKeys[index].resize(1);
		m_descriptorTableLayoutKeys[index][0] = Shader::Stage::ALL;
	}

	return m_descriptorTableLayoutKeys[index];
}

//--------------------------------------------------------------------------------------

PipelineLayoutLib_VK::PipelineLayoutLib_VK() :
	m_pDevice(nullptr),
	m_device(VK_NULL_HANDLE)
{
}

PipelineLayoutLib_VK::PipelineLayoutLib_VK(const Device* pDevice) :
	PipelineLayoutLib_VK()
{
	SetDevice(pDevice);
}

PipelineLayoutLib_VK::~PipelineLayoutLib_VK()
{
}

void PipelineLayoutLib_VK::SetDevice(const Device* pDevice)
{
	m_pDevice = dynamic_cast<const Device_VK*>(pDevice);
	assert(m_pDevice);
	m_device = m_pDevice->GetDevice();
}

void PipelineLayoutLib_VK::SetPipelineLayout(const string& key, const PipelineLayout& pipelineLayout)
{
	lock_guard<mutex> lock(m_mtx);

	LayoutEntry entry;
	entry.Object = static_cast<PipelineLayoutObject*>(pipelineLayout);
	m_pipelineLayouts[key] = move(entry);
}

PipelineLayout PipelineLayoutLib_VK::CreatePipelineLayout(Util::PipelineLayout* pUtil,
	PipelineLayoutFlag flags, const wchar_t* name, uint32_t nodeMask)
{
	auto& pipelineLayoutKey = pUtil->GetPipelineLayoutKey(this);
	reinterpret_cast<uint16_t&>(pipelineLayoutKey[0]) = static_cast<uint16_t>(flags);

	lock_guard<mutex> lock(m_mtx);

	return createPipelineLayout(pipelineLayoutKey, name, nodeMask);
}

PipelineLayout PipelineLayoutLib_VK::GetPipelineLayout(Util::PipelineLayout* pUtil,
	PipelineLayoutFlag flags, const wchar_t* name, bool create, uint32_t nodeMask)
{
	auto& pipelineLayoutKey = pUtil->GetPipelineLayoutKey(this);
	reinterpret_cast<uint16_t&>(pipelineLayoutKey[0]) = static_cast<uint16_t>(flags);

	lock_guard<mutex> lock(m_mtx);

	return getPipelineLayout(pipelineLayoutKey, name, create, nodeMask);
}

PipelineLayout PipelineLayoutLib_VK::CreateRootSignature(const Blob& blobSignature,
	const wchar_t* name, uint32_t nodeMask)
{
	assert(!"CreateRootSignature is not implemented on Vulkan");
	(void)blobSignature;
	(void)name;
	(void)nodeMask;

	return nullptr;
}

PipelineLayout PipelineLayoutLib_VK::GetRootSignature(const Blob& blobSignature,
	const wchar_t* name, bool create, uint32_t nodeMask)
{
	assert(!"GetRootSignature is not implemented on Vulkan");
	(void)blobSignature;
	(void)name;
	(void)create;
	(void)nodeMask;

	return nullptr;
}

DescriptorTableLayout PipelineLayoutLib_VK::CreateDescriptorTableLayout(uint32_t index, const Util::PipelineLayout* pUtil)
{
	const auto& keys = pUtil->GetDescriptorTableLayoutKeys();

	lock_guard<mutex> lock(m_mtx);

	return keys.size() > index ? createDescriptorTableLayout(keys[index]) : nullptr;
}

DescriptorTableLayout PipelineLayoutLib_VK::GetDescriptorTableLayout(uint32_t index, const Util::PipelineLayout* pUtil)
{
	const auto& keys = pUtil->GetDescriptorTableLayoutKeys();

	lock_guard<mutex> lock(m_mtx);

	return keys.size() > index ? getDescriptorTableLayout(keys[index]) : nullptr;
}

PipelineLayout PipelineLayoutLib_VK::createPipelineLayout(const string& key, const wchar_t* name, uint32_t nodeMask)
{
	(void)nodeMask;
	(void)name;
	auto object = make_unique<PipelineLayoutObject>();

	const auto numRootParams = reinterpret_cast<const uint16_t&>(key[Util::PipelineLayout_VK::DescriptorTableLayoutCountOffset]);
	const auto pDescriptorTableLayouts = reinterpret_cast<const DescriptorTableLayout*>(&key[Util::PipelineLayout_VK::DescriptorTableLayoutOffset]);
	object->RootParameters.resize(numRootParams);

	const auto& heapProps = m_pDevice->GetDescriptorHeapProperties();
	const auto resourceStride = m_pDevice->GetResourceDescriptorStride();
	const auto samplerStride = m_pDevice->GetSamplerDescriptorStride();
	const auto resourceReserved = static_cast<uint32_t>(heapProps.minResourceHeapReservedRange);
	const auto samplerReserved = static_cast<uint32_t>(heapProps.minSamplerHeapReservedRangeWithEmbedded);
	auto pushOffset = 0u;

	for (auto i = 0u; i < numRootParams; ++i)
	{
		const auto& tableKey = *static_cast<const string*>(pDescriptorTableLayouts[i]);
		auto& root = object->RootParameters[i];
		root = {};
		if (tableKey.size() < 1 + sizeof(DescriptorRange)) continue;

		const auto stage = static_cast<Shader::Stage>(tableKey[0]);
		const auto numRanges = static_cast<uint32_t>((tableKey.size() - 1) / sizeof(DescriptorRange));
		const auto pRanges = reinterpret_cast<const DescriptorRange*>(&tableKey[1]);
		root.Stage = stage;
		root.Type = pRanges->Type;

		switch (pRanges->Type)
		{
		case DescriptorType::CONSTANT:
		{
			pushOffset = alignPush(pushOffset, sizeof(uint32_t));
			root.PushByteOffset = pushOffset;
			root.SizeInBytes = pRanges->NumDescriptors * sizeof(uint32_t);
			VkDescriptorMappingSourceDataEXT data = {};
			data.pushDataOffset = pushOffset;
			appendMapping(*object, pRanges->Type, pRanges->Space, pRanges->BaseBinding, 1,
				VK_DESCRIPTOR_MAPPING_SOURCE_PUSH_DATA_EXT, data, stage);
			pushOffset += root.SizeInBytes;
			break;
		}
		case DescriptorType::ROOT_SRV:
		case DescriptorType::ROOT_UAV:
		case DescriptorType::ROOT_CBV:
		{
			pushOffset = alignPush(pushOffset, sizeof(uint64_t));
			root.PushByteOffset = pushOffset;
			root.SizeInBytes = sizeof(uint64_t);
			VkDescriptorMappingSourceDataEXT data = {};
			data.pushAddressOffset = pushOffset;
			appendMapping(*object, pRanges->Type, pRanges->Space, pRanges->BaseBinding, 1,
				VK_DESCRIPTOR_MAPPING_SOURCE_PUSH_ADDRESS_EXT, data, stage);
			pushOffset += root.SizeInBytes;
			break;
		}
		default:
		{
			pushOffset = alignPush(pushOffset, sizeof(uint32_t));
			root.PushByteOffset = pushOffset;
			root.SizeInBytes = sizeof(uint32_t);
			auto cursor = 0u;
			for (auto r = 0u; r < numRanges; ++r)
			{
				const auto& range = pRanges[r];
				const auto sampler = range.Type == DescriptorType::SAMPLER;
				const auto stride = sampler ? samplerStride : resourceStride;
				const auto reserved = sampler ? samplerReserved : resourceReserved;
				const auto offset = range.OffsetInDescriptors == XUSG_DESCRIPTOR_OFFSET_APPEND ? cursor : range.OffsetInDescriptors;
				cursor = offset + range.NumDescriptors;

				VkDescriptorMappingSourceDataEXT data = {};
				data.pushIndex.heapOffset = reserved + offset * stride;
				data.pushIndex.pushOffset = pushOffset;
				data.pushIndex.heapIndexStride = stride;
				data.pushIndex.heapArrayStride = stride;
				appendMapping(*object, range.Type, range.Space, range.BaseBinding, range.NumDescriptors,
					VK_DESCRIPTOR_MAPPING_SOURCE_HEAP_WITH_PUSH_INDEX_EXT, data, stage);
			}
			pushOffset += root.SizeInBytes;
			break;
		}
		}
	}

	const auto staticSamplerKeyOffset = Util::PipelineLayout_VK::DescriptorTableLayoutOffset + sizeof(DescriptorTableLayout) * numRootParams;
	const auto numSamplers = key.size() > staticSamplerKeyOffset ?
		static_cast<uint32_t>((key.size() - staticSamplerKeyOffset) / sizeof(StaticSampler)) : 0u;
	const auto pStaticSamplers = reinterpret_cast<const StaticSampler*>(&key[staticSamplerKeyOffset]);
	object->EmbeddedSamplers.resize(numSamplers);
	for (auto i = 0u; i < numSamplers; ++i)
		fillEmbeddedSampler(object->EmbeddedSamplers[i], pStaticSamplers[i].pSampler);

	for (auto i = 0u; i < numSamplers; ++i)
	{
		auto& embedded = object->EmbeddedSamplers[i];
		if (embedded.Reduction.reductionMode != VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE)
			embedded.Info.pNext = &embedded.Reduction;
		if (embedded.Border.sType == VK_STRUCTURE_TYPE_SAMPLER_CUSTOM_BORDER_COLOR_CREATE_INFO_EXT)
		{
			embedded.Border.pNext = embedded.Info.pNext;
			embedded.Info.pNext = &embedded.Border;
		}

		VkDescriptorMappingSourceDataEXT data = {};
		data.constantOffset.pEmbeddedSampler = &embedded.Info;
		appendMapping(*object, DescriptorType::SAMPLER, pStaticSamplers[i].Space, pStaticSamplers[i].Binding, 1,
			VK_DESCRIPTOR_MAPPING_SOURCE_HEAP_WITH_CONSTANT_OFFSET_EXT, data,
			static_cast<Shader::Stage>(pStaticSamplers[i].Stage));
	}

	object->PushDataSize = alignPush(pushOffset, sizeof(uint32_t));
	assert(object->PushDataSize <= heapProps.maxPushDataSize);

	for (auto s = 0u; s < Shader::NUM_STAGE; ++s)
	{
		auto& info = object->StageMappingInfos[s];
		const auto& mappings = object->StageMappings[s];
		info.mappingCount = static_cast<uint32_t>(mappings.size());
		info.pMappings = mappings.empty() ? nullptr : mappings.data();
	}

	LayoutEntry entry;
	entry.Object = object.get();
	entry.Owned = move(object);
	const auto handle = entry.Object;
	m_pipelineLayouts[key] = move(entry);

	return handle;
}

PipelineLayout PipelineLayoutLib_VK::getPipelineLayout(const string& key, const wchar_t* name, bool create, uint32_t nodeMask)
{
	const auto layoutIter = m_pipelineLayouts.find(key);

	// Create one, if it does not exist
	if (layoutIter == m_pipelineLayouts.cend())
	{
		if (create) return createPipelineLayout(key, name, nodeMask);

		return nullptr;
	}

	return layoutIter->second.Object;
}

DescriptorTableLayout PipelineLayoutLib_VK::createDescriptorTableLayout(const string& key)
{
	m_descriptorTableLayouts[key] = nullptr;
	const auto layoutPtrIter = m_descriptorTableLayouts.find(key);
	layoutPtrIter->second = &layoutPtrIter->first;

	return layoutPtrIter->second;
}

DescriptorTableLayout PipelineLayoutLib_VK::getDescriptorTableLayout(const string& key)
{
	const auto layoutPtrIter = m_descriptorTableLayouts.find(key);

	// Create one, if it does not exist
	if (layoutPtrIter == m_descriptorTableLayouts.cend()) return createDescriptorTableLayout(key);

	return layoutPtrIter->second;
}

void PipelineLayoutLib_VK::appendMapping(PipelineLayoutObject& layout, DescriptorType type, uint32_t set,
	uint32_t binding, uint32_t count, VkDescriptorMappingSourceEXT source, const VkDescriptorMappingSourceDataEXT& data,
	Shader::Stage stage)
{
	VkDescriptorSetAndBindingMappingEXT mapping = {};
	mapping.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_AND_BINDING_MAPPING_EXT;
	mapping.descriptorSet = set;
	mapping.firstBinding = binding;
	mapping.bindingCount = count;
	mapping.resourceMask = ResourceMask(type);
	mapping.source = source;
	mapping.sourceData = data;

	// Shader::ALL aliases Shader::CS, and DX12 maps that slot to SHADER_VISIBILITY_ALL.
	if (stage == Shader::Stage::ALL)
	{
		for (auto i = 0u; i < Shader::NUM_STAGE; ++i)
			layout.StageMappings[i].push_back(mapping);
	}
	else layout.StageMappings[stage].push_back(mapping);
}

void PipelineLayoutLib_VK::fillEmbeddedSampler(EmbeddedSampler_VK& embedded, const Sampler* pSampler) const
{
	embedded = {};
	assert(pSampler);
	embedded.Reduction.sType = VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO;
	embedded.Reduction.reductionMode = GetVKReductionMode(pSampler->Filter);

	const auto standard = (pSampler->BorderColor[3] == 0.0f && pSampler->BorderColor[0] == 0.0f &&
		pSampler->BorderColor[1] == 0.0f && pSampler->BorderColor[2] == 0.0f) ||
		(pSampler->BorderColor[3] == 1.0f && pSampler->BorderColor[0] == pSampler->BorderColor[1] &&
			pSampler->BorderColor[1] == pSampler->BorderColor[2] &&
			(pSampler->BorderColor[0] == 0.0f || pSampler->BorderColor[0] == 1.0f));
	if (m_pDevice->HasCustomBorderColor() && !standard)
	{
		embedded.Border.sType = VK_STRUCTURE_TYPE_SAMPLER_CUSTOM_BORDER_COLOR_CREATE_INFO_EXT;
		embedded.Border.customBorderColor.float32[0] = pSampler->BorderColor[0];
		embedded.Border.customBorderColor.float32[1] = pSampler->BorderColor[1];
		embedded.Border.customBorderColor.float32[2] = pSampler->BorderColor[2];
		embedded.Border.customBorderColor.float32[3] = pSampler->BorderColor[3];
	}

	const auto white = pSampler->BorderColor[0] > 0.5f || pSampler->BorderColor[1] > 0.5f || pSampler->BorderColor[2] > 0.5f;
	embedded.Info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	embedded.Info.magFilter = GetVKFilter(pSampler->Filter, false, true);
	embedded.Info.minFilter = GetVKFilter(pSampler->Filter, true, false);
	embedded.Info.mipmapMode = GetVKMipmapMode(pSampler->Filter);
	embedded.Info.addressModeU = GetVKSamplerAddressMode(pSampler->AddressU);
	embedded.Info.addressModeV = GetVKSamplerAddressMode(pSampler->AddressV);
	embedded.Info.addressModeW = GetVKSamplerAddressMode(pSampler->AddressW);
	embedded.Info.mipLodBias = pSampler->MipLODBias;
	embedded.Info.anisotropyEnable = pSampler->MaxAnisotropy > 1;
	embedded.Info.maxAnisotropy = pSampler->MaxAnisotropy;
	const auto filterValue = static_cast<uint8_t>(pSampler->Filter);
	embedded.Info.compareEnable = filterValue >= static_cast<uint8_t>(SamplerFilter::COMPARISON_MIN_MAG_MIP_POINT) &&
		filterValue <= static_cast<uint8_t>(SamplerFilter::COMPARISON_ANISOTROPIC);
	embedded.Info.compareOp = GetVKCompareOp(pSampler->Comparison);
	embedded.Info.minLod = pSampler->MinLOD;
	embedded.Info.maxLod = pSampler->MaxLOD;
	embedded.Info.borderColor = pSampler->BorderColor[3] <= 0.0f ? VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK :
		white ? VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE : VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
	embedded.Info.unnormalizedCoordinates = (pSampler->Flags & SamplerFlag::NON_NORMALIZED_COORDINATES) != SamplerFlag::NONE;
}

uint32_t PipelineLayoutLib_VK::alignPush(uint32_t offset, uint32_t alignment) const
{
	return (offset + alignment - 1u) & ~(alignment - 1u);
}
