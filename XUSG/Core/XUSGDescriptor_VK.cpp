//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGDescriptor_VK.h"
#include "XUSGResource_VK.h"
#include "XUSGEnum_VK.h"

using namespace std;
using namespace XUSG;

namespace
{
	VkBorderColor BorderColor(const Sampler* pSampler)
	{
		const auto white = pSampler->BorderColor[0] > 0.5f || pSampler->BorderColor[1] > 0.5f || pSampler->BorderColor[2] > 0.5f;
		if (pSampler->BorderColor[3] <= 0.0f) return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		if (white) return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

		return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
	}

	bool IsStandardBorder(const Sampler* pSampler)
	{
		const auto r = pSampler->BorderColor[0];
		const auto g = pSampler->BorderColor[1];
		const auto b = pSampler->BorderColor[2];
		const auto a = pSampler->BorderColor[3];
		const auto black = r == 0.0f && g == 0.0f && b == 0.0f;
		const auto white = r == 1.0f && g == 1.0f && b == 1.0f;

		return (black && (a == 0.0f || a == 1.0f)) || (white && a == 1.0f);
	}
}

DescriptorHeap_VK::DescriptorHeap_VK() :
	m_pDevice(nullptr),
	m_buffer(VK_NULL_HANDLE),
	m_memory(VK_NULL_HANDLE),
	m_mapped(nullptr),
	m_address(0),
	m_size(0),
	m_reserved(0),
	m_type(CBV_SRV_UAV_HEAP),
	m_stride(0),
	m_capacity(0)
{
}

DescriptorHeap_VK::~DescriptorHeap_VK()
{
	reset();
}

bool DescriptorHeap_VK::Create(const Device_VK* pDevice, DescriptorHeapType type, uint32_t numDescriptors, const wchar_t* name)
{
	reset();
	assert(pDevice);
	m_pDevice = pDevice;
	const auto device = m_pDevice->GetDevice();
	m_type = type;

	const auto& props = pDevice->GetDescriptorHeapProperties();
	const auto sampler = type == SAMPLER_HEAP;
	m_stride = sampler ? pDevice->GetSamplerDescriptorStride() : pDevice->GetResourceDescriptorStride();
	m_reserved = sampler ? props.minSamplerHeapReservedRangeWithEmbedded : props.minResourceHeapReservedRange;
	const auto alignment = sampler ? props.samplerHeapAlignment : props.resourceHeapAlignment;
	m_capacity = numDescriptors;
	m_size = m_reserved + static_cast<VkDeviceSize>(m_stride) * numDescriptors;
	if (alignment > 1) m_size = (m_size + alignment - 1) & ~(alignment - 1);

	VkBufferCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.size = m_size;
	info.usage = VK_BUFFER_USAGE_DESCRIPTOR_HEAP_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	XUSG_VK_CHECK(vkCreateBuffer(device, &info, nullptr, &m_buffer), cerr, false);

	VkMemoryRequirements requirements = {};
	vkGetBufferMemoryRequirements(device, m_buffer, &requirements);
	const auto typeIndex = pDevice->FindMemoryType(requirements.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	VkMemoryAllocateFlagsInfo flags = {};
	flags.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
	flags.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

	VkMemoryAllocateInfo alloc = {};
	alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc.pNext = &flags;
	alloc.allocationSize = requirements.size;
	alloc.memoryTypeIndex = typeIndex;
	XUSG_VK_CHECK(vkAllocateMemory(device, &alloc, nullptr, &m_memory), cerr, false);
	XUSG_VK_CHECK(vkBindBufferMemory(device, m_buffer, m_memory, 0), cerr, false);
	XUSG_VK_CHECK(vkMapMemory(device, m_memory, 0, m_size, 0, &m_mapped), cerr, false);
	memset(m_mapped, 0, static_cast<size_t>(m_size));

	VkBufferDeviceAddressInfo address = {};
	address.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	address.buffer = m_buffer;
	m_address = vkGetBufferDeviceAddress(device, &address);

	pDevice->SetDebugName(VK_OBJECT_TYPE_BUFFER, (uint64_t)m_buffer, name);

	return true;
}

DescriptorHeapType DescriptorHeap_VK::GetType() const
{
	return m_type;
}

uint32_t DescriptorHeap_VK::GetStride() const
{
	return m_stride;
}

uint32_t DescriptorHeap_VK::GetCapacity() const
{
	return m_capacity;
}

VkDeviceAddress DescriptorHeap_VK::GetAddress() const
{
	return m_address;
}

VkDeviceSize DescriptorHeap_VK::GetSize() const
{
	return m_size;
}

VkDeviceSize DescriptorHeap_VK::GetReservedSize() const
{
	return m_reserved;
}

void* DescriptorHeap_VK::GetMapped() const
{
	return m_mapped;
}

void DescriptorHeap_VK::reset()
{
	if (m_pDevice)
	{
		const auto device = m_pDevice->GetDevice();
		if (m_mapped) vkUnmapMemory(device, m_memory);
		if (m_buffer != VK_NULL_HANDLE) vkDestroyBuffer(device, m_buffer, nullptr);
		if (m_memory != VK_NULL_HANDLE) vkFreeMemory(device, m_memory, nullptr);
	}

	m_mapped = nullptr;
	m_buffer = VK_NULL_HANDLE;
	m_memory = VK_NULL_HANDLE;
	m_address = 0;
	m_size = 0;
	m_capacity = 0;
}

//--------------------------------------------------------------------------------------

Util::DescriptorTable_VK::DescriptorTable_VK()
{
	m_key.resize(0);
}

Util::DescriptorTable_VK::~DescriptorTable_VK()
{
}

void Util::DescriptorTable_VK::SetDescriptors(uint32_t start, uint32_t num,
	const Descriptor* srcDescriptors, uint8_t descriptorHeapIndex)
{
	const auto size = sizeof(Descriptor) * (start + num) + 1;
	if (size > m_key.size())
		m_key.resize(size);

	m_key[0] = descriptorHeapIndex;
	const auto descriptors = reinterpret_cast<Descriptor*>(&m_key[1]);
	memcpy(&descriptors[start], srcDescriptors, sizeof(Descriptor) * num);
}

void Util::DescriptorTable_VK::SetSamplers(uint32_t start, uint32_t num,
	const Sampler* const* ppSamplers, uint8_t descriptorHeapIndex)
{
	const auto size = sizeof(Sampler*) * (start + num) + 1;
	if (size > m_key.size())
		m_key.resize(size);

	m_key[0] = descriptorHeapIndex;
	const auto descriptors = reinterpret_cast<const Sampler**>(&m_key[1]);

	for (auto i = 0u; i < num; ++i) descriptors[start + i] = ppSamplers[i];
}

void Util::DescriptorTable_VK::SetSamplers(uint32_t start, uint32_t num, const SamplerPreset* presets,
	DescriptorTableLib* pDescriptorTableLib, uint8_t descriptorHeapIndex)
{
	const auto size = sizeof(Sampler*) * (start + num) + 1;
	if (size > m_key.size())
		m_key.resize(size);

	m_key[0] = descriptorHeapIndex;
	const auto descriptors = reinterpret_cast<const Sampler**>(&m_key[1]);

	for (auto i = 0u; i < num; ++i)
		descriptors[start + i] = pDescriptorTableLib->GetSampler(presets[i]);
}

DescriptorTable Util::DescriptorTable_VK::CreateCbvSrvUavTable(DescriptorTableLib* pDescriptorTableLib, const XUSG::DescriptorTable& table)
{
	const auto p = AsVK(pDescriptorTableLib);
	assert(p);

	return p->CreateCbvSrvUavTable(this, table);
}

DescriptorTable Util::DescriptorTable_VK::GetCbvSrvUavTable(DescriptorTableLib* pDescriptorTableLib, const XUSG::DescriptorTable& table)
{
	const auto p = AsVK(pDescriptorTableLib);
	assert(p);

	return p->GetCbvSrvUavTable(this, table);
}

DescriptorTable Util::DescriptorTable_VK::CreateSamplerTable(DescriptorTableLib* pDescriptorTableLib, const XUSG::DescriptorTable& table)
{
	const auto p = AsVK(pDescriptorTableLib);
	assert(p);

	return p->CreateSamplerTable(this, table);
}

DescriptorTable Util::DescriptorTable_VK::GetSamplerTable(DescriptorTableLib* pDescriptorTableLib, const XUSG::DescriptorTable& table)
{
	const auto p = AsVK(pDescriptorTableLib);
	assert(p);

	return p->GetSamplerTable(this, table);
}

const string& Util::DescriptorTable_VK::GetKey() const
{
	return m_key;
}

uint32_t Util::DescriptorTable_VK::CreateCbvSrvUavTableIndex(DescriptorTableLib* pDescriptorTableLib, XUSG::DescriptorTable table)
{
	table = CreateCbvSrvUavTable(pDescriptorTableLib, table);

	return GetDescriptorTableIndex(pDescriptorTableLib, CBV_SRV_UAV_HEAP, table);
}

uint32_t Util::DescriptorTable_VK::GetCbvSrvUavTableIndex(DescriptorTableLib* pDescriptorTableLib, XUSG::DescriptorTable table)
{
	table = GetCbvSrvUavTable(pDescriptorTableLib, table);

	return GetDescriptorTableIndex(pDescriptorTableLib, CBV_SRV_UAV_HEAP, table);
}

uint32_t Util::DescriptorTable_VK::CreateSamplerTableIndex(DescriptorTableLib* pDescriptorTableLib, XUSG::DescriptorTable table)
{
	table = CreateSamplerTable(pDescriptorTableLib, table);

	return GetDescriptorTableIndex(pDescriptorTableLib, SAMPLER_HEAP, table);
}

uint32_t Util::DescriptorTable_VK::GetSamplerTableIndex(DescriptorTableLib* pDescriptorTableLib, XUSG::DescriptorTable table)
{
	table = GetSamplerTable(pDescriptorTableLib, table);

	return GetDescriptorTableIndex(pDescriptorTableLib, SAMPLER_HEAP, table);
}

uint32_t Util::DescriptorTable_VK::GetDescriptorTableIndex(DescriptorTableLib* pDescriptorTableLib, DescriptorHeapType type,
	const XUSG::DescriptorTable& table) const
{
	if (!m_key.empty() && table)
	{
		(void)pDescriptorTableLib;
		(void)type;
		const auto& tableProperty = reinterpret_cast<const DescriptorTableProperty_VK&>(table);

		return tableProperty.DescriptorIndex;
	}

	return UINT32_MAX;
}

//--------------------------------------------------------------------------------------

DescriptorTableLib_VK::DescriptorTableLib_VK() :
	m_pDevice(nullptr),
	m_cbvSrvUavTables(),
	m_samplerTables(),
	m_descriptorHeaps(),
	m_descriptorCounts(),
	m_descriptorStrides(),
	m_samplers()
{
	// Sampler presets
	m_pfnSamplers[SamplerPreset::POINT_WRAP] = SamplerPointWrap;
	m_pfnSamplers[SamplerPreset::POINT_MIRROR] = SamplerPointMirror;
	m_pfnSamplers[SamplerPreset::POINT_CLAMP] = SamplerPointClamp;
	m_pfnSamplers[SamplerPreset::POINT_BORDER] = SamplerPointBorder;
	m_pfnSamplers[SamplerPreset::POINT_LESS_EQUAL] = SamplerPointLessEqual;

	m_pfnSamplers[SamplerPreset::LINEAR_WRAP] = SamplerLinearWrap;
	m_pfnSamplers[SamplerPreset::LINEAR_MIRROR] = SamplerLinearMirror;
	m_pfnSamplers[SamplerPreset::LINEAR_CLAMP] = SamplerLinearClamp;
	m_pfnSamplers[SamplerPreset::LINEAR_BORDER] = SamplerLinearBorder;
	m_pfnSamplers[SamplerPreset::LINEAR_LESS_EQUAL] = SamplerLinearLessEqual;

	m_pfnSamplers[SamplerPreset::ANISOTROPIC_WRAP] = SamplerAnisotropicWrap;
	m_pfnSamplers[SamplerPreset::ANISOTROPIC_MIRROR] = SamplerAnisotropicMirror;
	m_pfnSamplers[SamplerPreset::ANISOTROPIC_CLAMP] = SamplerAnisotropicClamp;
	m_pfnSamplers[SamplerPreset::ANISOTROPIC_BORDER] = SamplerAnisotropicBorder;
	m_pfnSamplers[SamplerPreset::ANISOTROPIC_LESS_EQUAL] = SamplerAnisotropicLessEqual;

	m_pfnSamplers[SamplerPreset::MIN_LINEAR_WRAP] = SamplerMinLinearWrap;
	m_pfnSamplers[SamplerPreset::MIN_LINEAR_MIRROR] = SamplerMinLinearMirror;
	m_pfnSamplers[SamplerPreset::MIN_LINEAR_CLAMP] = SamplerMinLinearClamp;
	m_pfnSamplers[SamplerPreset::MIN_LINEAR_BORDER] = SamplerMinLinearBorder;

	m_pfnSamplers[SamplerPreset::MIN_ANISOTROPIC_WRAP] = SamplerMinAnisotropicWrap;
	m_pfnSamplers[SamplerPreset::MIN_ANISOTROPIC_MIRROR] = SamplerMinAnisotropicMirror;
	m_pfnSamplers[SamplerPreset::MIN_ANISOTROPIC_CLAMP] = SamplerMinAnisotropicClamp;
	m_pfnSamplers[SamplerPreset::MIN_ANISOTROPIC_BORDER] = SamplerMinAnisotropicBorder;

	m_pfnSamplers[SamplerPreset::MAX_LINEAR_WRAP] = SamplerMaxLinearWrap;
	m_pfnSamplers[SamplerPreset::MAX_LINEAR_MIRROR] = SamplerMaxLinearMirror;
	m_pfnSamplers[SamplerPreset::MAX_LINEAR_CLAMP] = SamplerMaxLinearClamp;
	m_pfnSamplers[SamplerPreset::MAX_LINEAR_BORDER] = SamplerMaxLinearBorder;

	m_pfnSamplers[SamplerPreset::MAX_ANISOTROPIC_WRAP] = SamplerMaxAnisotropicWrap;
	m_pfnSamplers[SamplerPreset::MAX_ANISOTROPIC_MIRROR] = SamplerMaxAnisotropicMirror;
	m_pfnSamplers[SamplerPreset::MAX_ANISOTROPIC_CLAMP] = SamplerMaxAnisotropicClamp;
	m_pfnSamplers[SamplerPreset::MAX_ANISOTROPIC_BORDER] = SamplerMaxAnisotropicBorder;
}

DescriptorTableLib_VK::DescriptorTableLib_VK(Device* pDevice, const wchar_t* name) :
	DescriptorTableLib_VK()
{
	SetDevice(pDevice);
	SetName(name);
}

DescriptorTableLib_VK::~DescriptorTableLib_VK()
{
}

void DescriptorTableLib_VK::SetDevice(Device* pDevice)
{
	m_pDevice = AsVK(pDevice);
	assert(m_pDevice);
	m_descriptorStrides[CBV_SRV_UAV_HEAP] = m_pDevice->GetResourceDescriptorStride();
	m_descriptorStrides[SAMPLER_HEAP] = m_pDevice->GetSamplerDescriptorStride();
}

void DescriptorTableLib_VK::SetName(const wchar_t* name)
{
	if (name) m_name = name;
}

void DescriptorTableLib_VK::ResetDescriptorHeap(DescriptorHeapType type, uint8_t index)
{
	lock_guard<mutex> lock(m_mtx);

	if (index < m_descriptorCounts[type].size()) m_descriptorCounts[type][index] = 0;

	switch (type)
	{
	case CBV_SRV_UAV_HEAP:
		if (index < m_cbvSrvUavTables.size()) m_cbvSrvUavTables[index].clear();
		break;
	case SAMPLER_HEAP:
		if (index < m_samplerTables.size()) m_samplerTables[index].clear();
		break;
	}
}

bool DescriptorTableLib_VK::AllocateDescriptorHeap(DescriptorHeapType type, uint32_t numDescriptors, uint8_t index)
{
	lock_guard<mutex> lock(m_mtx);

	checkDescriptorHeapTypeStorage(type, index);

	return allocateDescriptorHeap(type, numDescriptors, index);
}

DescriptorTable DescriptorTableLib_VK::CreateCbvSrvUavTable(const Util::DescriptorTable* pUtil, const DescriptorTable& table)
{
	lock_guard<mutex> lock(m_mtx);

	return createCbvSrvUavTable(pUtil->GetKey(), table);
}

DescriptorTable DescriptorTableLib_VK::GetCbvSrvUavTable(const Util::DescriptorTable* pUtil, const DescriptorTable& table)
{
	lock_guard<mutex> lock(m_mtx);

	return getCbvSrvUavTable(pUtil->GetKey(), table);
}

DescriptorTable DescriptorTableLib_VK::CreateSamplerTable(const Util::DescriptorTable* pUtil, const DescriptorTable& table)
{
	lock_guard<mutex> lock(m_mtx);

	return createSamplerTable(pUtil->GetKey(), table);
}

DescriptorTable DescriptorTableLib_VK::GetSamplerTable(const Util::DescriptorTable* pUtil, const DescriptorTable& table)
{
	lock_guard<mutex> lock(m_mtx);

	return getSamplerTable(pUtil->GetKey(), table);
}

DescriptorHeap DescriptorTableLib_VK::GetDescriptorHeap(DescriptorHeapType type, uint8_t index)
{
	lock_guard<mutex> lock(m_mtx);

	return index < m_descriptorHeaps[type].size() ? m_descriptorHeaps[type][index].get() : nullptr;
}

const Sampler* DescriptorTableLib_VK::GetSampler(SamplerPreset preset)
{
	if (preset < SamplerPreset::NUM_SAMPLER_PRESET && !m_samplers[preset])
		m_samplers[preset] = make_unique<Sampler>(m_pfnSamplers[preset]());

	return m_samplers[preset].get();
}

uint32_t DescriptorTableLib_VK::GetDescriptorStride(DescriptorHeapType type)
{
	lock_guard<mutex> lock(m_mtx);

	return m_descriptorStrides[type];
}

void* DescriptorTableLib_VK::GetBackend()
{
	return this;
}

void DescriptorTableLib_VK::checkDescriptorHeapTypeStorage(DescriptorHeapType type, uint8_t index)
{
	if (index >= m_descriptorHeaps[type].size())
		m_descriptorHeaps[type].resize(index + 1);

	if (index >= m_descriptorCounts[type].size())
		m_descriptorCounts[type].resize(index + 1);

	switch (type)
	{
	case CBV_SRV_UAV_HEAP:
		if (index >= m_cbvSrvUavTables.size()) m_cbvSrvUavTables.resize(index + 1);
		break;
	case SAMPLER_HEAP:
		if (index >= m_samplerTables.size()) m_samplerTables.resize(index + 1);
		break;
	}
}

bool DescriptorTableLib_VK::allocateDescriptorHeap(DescriptorHeapType type, uint32_t numDescriptors, uint8_t index)
{
	static const wchar_t* heapNames[] =
	{
		L".CbvSrvUavHeap",
		L".SamplerHeap",
	};

	auto heap = make_unique<DescriptorHeap_VK>();
	const auto name = m_name.empty() ? wstring() : m_name + heapNames[type] + to_wstring(index);
	XUSG_N_RETURN(heap->Create(m_pDevice, type, numDescriptors, name.empty() ? nullptr : name.c_str()), false);

	m_descriptorHeaps[type][index] = move(heap);
	m_descriptorCounts[type][index] = 0;

	return true;
}

bool DescriptorTableLib_VK::reallocateCbvSrvUavHeap(const string& key)
{
	assert(!key.empty());
	const auto& index = key[0];
	auto numDescriptors = static_cast<uint32_t>(key.size() / sizeof(Descriptor));

	// Allocate a new heap if neccessary
	const auto& descriptorHeap = m_descriptorHeaps[CBV_SRV_UAV_HEAP][index];
	const auto descriptorCount = m_descriptorCounts[CBV_SRV_UAV_HEAP][index];
	numDescriptors += descriptorCount;
	if (!descriptorHeap || descriptorHeap->GetCapacity() < numDescriptors)
	{
		numDescriptors = calculateGrowth(numDescriptors, CBV_SRV_UAV_HEAP, index);
		XUSG_N_RETURN(allocateDescriptorHeap(CBV_SRV_UAV_HEAP, numDescriptors, index), false);

		// Recreate descriptor tables
		m_descriptorCounts[CBV_SRV_UAV_HEAP][index] = descriptorCount;
		for (const auto& tableEntry : m_cbvSrvUavTables[index])
			createCbvSrvUavTable(tableEntry.first, tableEntry.second);
	}

	return true;
}

bool DescriptorTableLib_VK::reallocateSamplerHeap(const string& key)
{
	assert(!key.empty());
	const auto& index = key[0];
	auto numDescriptors = static_cast<uint32_t>(key.size() / sizeof(Sampler*));

	// Allocate a new heap if neccessary
	const auto& descriptorHeap = m_descriptorHeaps[SAMPLER_HEAP][index];
	const auto descriptorCount = m_descriptorCounts[SAMPLER_HEAP][index];
	numDescriptors += descriptorCount;
	if (!descriptorHeap || descriptorHeap->GetCapacity() < numDescriptors)
	{
		numDescriptors = calculateGrowth(numDescriptors, SAMPLER_HEAP, index);
		XUSG_N_RETURN(allocateDescriptorHeap(SAMPLER_HEAP, numDescriptors, index), false);

		// Recreate descriptor tables
		m_descriptorCounts[SAMPLER_HEAP][index] = descriptorCount;
		for (const auto& tableEntry : m_samplerTables[index])
			createSamplerTable(tableEntry.first, tableEntry.second);
	}

	return true;
}

DescriptorTable DescriptorTableLib_VK::createCbvSrvUavTable(const string& key, DescriptorTable table)
{
	if (!key.empty())
	{
		const auto& index = key[0];
		const auto numDescriptors = static_cast<uint32_t>(key.size() / sizeof(Descriptor));
		const auto pDescriptors = reinterpret_cast<const Descriptor*>(&key[1]);
		const auto& descriptorHeap = m_descriptorHeaps[CBV_SRV_UAV_HEAP][index];
		const auto descriptorStride = m_descriptorStrides[CBV_SRV_UAV_HEAP];
		assert(descriptorHeap);

		auto& tableProperty = reinterpret_cast<DescriptorTableProperty_VK&>(table);
		if (tableProperty.IsInitialized)
			assert(tableProperty.HeapType == CBV_SRV_UAV_HEAP);
		else
		{
			auto& descriptorCount = m_descriptorCounts[CBV_SRV_UAV_HEAP][index];
			tableProperty.DescriptorIndex = descriptorCount;
			tableProperty.HeapType = CBV_SRV_UAV_HEAP;
			tableProperty.IsInitialized = 1;
			descriptorCount += numDescriptors;
		}

		auto dst = static_cast<uint8_t*>(descriptorHeap->GetMapped()) + descriptorHeap->GetReservedSize() +
			static_cast<size_t>(tableProperty.DescriptorIndex) * descriptorStride;
		for (auto i = 0u; i < numDescriptors; ++i)
		{
			const auto pView = static_cast<const ResourceView_VK*>(pDescriptors[i]);
			memset(dst, 0, descriptorStride);
			if (pView) writeResource(pView, dst);
			else writeNullResource(dst);
			dst += descriptorStride;
		}

		return table;
	}

	return XUSG_NULL;
}

DescriptorTable DescriptorTableLib_VK::getCbvSrvUavTable(const string& key, DescriptorTable table)
{
	if (!key.empty())
	{
		const auto& index = key[0];
		checkDescriptorHeapTypeStorage(CBV_SRV_UAV_HEAP, index);
		auto& cbvSrvUavTables = m_cbvSrvUavTables[index];
		const auto tableIter = cbvSrvUavTables.find(key);

		// Create one, if it does not exist
		if (tableIter == cbvSrvUavTables.cend() && reallocateCbvSrvUavHeap(key))
		{
			if (table)
			{
				for (auto mIt = cbvSrvUavTables.cbegin(); mIt != cbvSrvUavTables.cend(); ++mIt)
				{
					if (mIt->second == table)
					{
						cbvSrvUavTables.erase(mIt);
						break;
					}
				}
			}

			table = createCbvSrvUavTable(key, table);
			cbvSrvUavTables[key] = table;

			return table;
		}

		return tableIter->second;
	}

	return XUSG_NULL;
}

DescriptorTable DescriptorTableLib_VK::createSamplerTable(const string& key, DescriptorTable table)
{
	if (!key.empty())
	{
		const auto& index = key[0];
		const auto numDescriptors = static_cast<uint32_t>(key.size() / sizeof(uintptr_t));
		const auto pSamplers = reinterpret_cast<Sampler* const*>(&key[1]);
		const auto& descriptorHeap = m_descriptorHeaps[SAMPLER_HEAP][index];
		const auto descriptorStride = m_descriptorStrides[SAMPLER_HEAP];
		assert(descriptorHeap);

		auto& tableProperty = reinterpret_cast<DescriptorTableProperty_VK&>(table);
		if (tableProperty.IsInitialized)
			assert(tableProperty.HeapType == SAMPLER_HEAP);
		else
		{
			auto& descriptorCount = m_descriptorCounts[SAMPLER_HEAP][index];
			tableProperty.DescriptorIndex = descriptorCount;
			tableProperty.HeapType = SAMPLER_HEAP;
			tableProperty.IsInitialized = 1;
			descriptorCount += numDescriptors;
		}

		auto dst = static_cast<uint8_t*>(descriptorHeap->GetMapped()) + descriptorHeap->GetReservedSize() +
			static_cast<size_t>(tableProperty.DescriptorIndex) * descriptorStride;
		for (auto i = 0u; i < numDescriptors; ++i)
		{
			memset(dst, 0, descriptorStride);
			if (pSamplers[i]) writeSampler(pSamplers[i], dst);
			dst += descriptorStride;
		}

		return table;
	}

	return XUSG_NULL;
}

DescriptorTable DescriptorTableLib_VK::getSamplerTable(const string& key, DescriptorTable table)
{
	if (!key.empty())
	{
		const auto& index = key[0];
		checkDescriptorHeapTypeStorage(SAMPLER_HEAP, index);
		auto& samplerTables = m_samplerTables[index];
		const auto tableIter = samplerTables.find(key);

		// Create one, if it does not exist
		if (tableIter == samplerTables.cend() && reallocateSamplerHeap(key))
		{
			if (table)
			{
				for (auto mIt = samplerTables.cbegin(); mIt != samplerTables.cend(); ++mIt)
				{
					if (mIt->second == table)
					{
						samplerTables.erase(mIt);
						break;
					}
				}
			}

			table = createSamplerTable(key, table);
			samplerTables[key] = table;

			return table;
		}

		return tableIter->second;
	}

	return XUSG_NULL;
}

uint32_t DescriptorTableLib_VK::calculateGrowth(uint32_t newSize, DescriptorHeapType type, uint8_t index) const
{
	const auto& oldCapacity = m_descriptorCounts[type][index];
	const auto halfOldCapacity = oldCapacity >> 1;
	const auto& props = m_pDevice->GetDescriptorHeapProperties();
	const auto stride = m_descriptorStrides[type] ? m_descriptorStrides[type] : 1u;
	const auto maxBytes = type == SAMPLER_HEAP ? props.maxSamplerHeapSize : props.maxResourceHeapSize;
	const auto reserved = type == SAMPLER_HEAP ? props.minSamplerHeapReservedRangeWithEmbedded : props.minResourceHeapReservedRange;
	const auto maxSize = maxBytes > props.minResourceHeapReservedRange ?
		static_cast<uint32_t>((maxBytes - reserved) / stride) : newSize;

	// geometric growth would overflow
	if (oldCapacity > maxSize - halfOldCapacity) return newSize;

	const auto geometric = oldCapacity + halfOldCapacity;

	return (max)(geometric, newSize);
}

bool DescriptorTableLib_VK::writeNullResource(void* pDescriptor) const
{
	const auto nullDescriptor = m_pDevice->HasNullDescriptor();
	assert(nullDescriptor);
	if (!nullDescriptor) return false;
	const auto& props = m_pDevice->GetDescriptorHeapProperties();
	const auto useImage = props.imageDescriptorSize >= props.bufferDescriptorSize;
	VkResourceDescriptorInfoEXT info = {};
	info.sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT;
	info.type = useImage ? VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

	VkHostAddressRangeEXT host = {};
	host.address = pDescriptor;
	host.size = m_pDevice->GetResourceDescriptorStride();
	XUSG_VK_CHECK(m_pDevice->GetExtProcs().WriteResourceDescriptors(m_pDevice->GetDevice(), 1, &info, &host), cerr, false);

	return true;
}

bool DescriptorTableLib_VK::writeResource(const ResourceView_VK* pView, void* pDescriptor) const
{
	assert(pView);
	if (!pView->pResource && !pView->Address) return writeNullResource(pDescriptor);
	if (pView->Type == ResourceViewType::SAMPLER_FEEDBACK)
	{
		assert(!"Sampler feedback descriptor is not implemented on Vulkan");

		return false;
	}

	VkDeviceAddressRangeEXT range = {};
	range.address = pView->Address;
	range.size = pView->Range;

	VkTexelBufferDescriptorInfoEXT texel = {};
	texel.sType = VK_STRUCTURE_TYPE_TEXEL_BUFFER_DESCRIPTOR_INFO_EXT;
	texel.format = pView->ViewInfo.format;
	texel.addressRange = range;

	VkImageDescriptorInfoEXT image = {};
	image.sType = VK_STRUCTURE_TYPE_IMAGE_DESCRIPTOR_INFO_EXT;
	image.pView = &pView->ViewInfo;
	image.layout = pView->Layout;

	VkResourceDescriptorInfoEXT info = {};
	info.sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT;
	info.type = pView->VkType;
	if (pView->IsImage) info.data.pImage = &image;
	else if (pView->IsTexelBuffer) info.data.pTexelBuffer = &texel;
	else info.data.pAddressRange = &range;

	VkHostAddressRangeEXT host = {};
	host.address = pDescriptor;
	host.size = m_pDevice->GetResourceDescriptorStride();
	XUSG_VK_CHECK(m_pDevice->GetExtProcs().WriteResourceDescriptors(m_pDevice->GetDevice(), 1, &info, &host), cerr, false);

	return true;
}

bool DescriptorTableLib_VK::writeSampler(const Sampler* pSampler, void* pDescriptor) const
{
	assert(pSampler);
	VkSamplerCustomBorderColorCreateInfoEXT border = {};
	border.sType = VK_STRUCTURE_TYPE_SAMPLER_CUSTOM_BORDER_COLOR_CREATE_INFO_EXT;

	VkSamplerReductionModeCreateInfo reduction = {};
	reduction.sType = VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO;
	reduction.reductionMode = GetVKReductionMode(pSampler->Filter);

	VkSamplerCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	if (reduction.reductionMode != VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE) info.pNext = &reduction;

	const auto uintBorder = (pSampler->Flags & SamplerFlag::UINT_BORDER_COLOR) != SamplerFlag::NONE;
	auto customBorder = false;
	if (uintBorder)
	{
		const auto r = pSampler->BorderColorU[0];
		const auto g = pSampler->BorderColorU[1];
		const auto b = pSampler->BorderColorU[2];
		const auto a = pSampler->BorderColorU[3];
		// Match vkd3d: standard UINT borders use 0/1, not 0/~0.
		const auto black = r == 0 && g == 0 && b == 0;
		const auto white = r == 1 && g == 1 && b == 1;
		const auto standard = (black && (a == 0 || a == 1)) || (white && a == 1);
		if (!standard && m_pDevice->HasCustomBorderColor())
		{
			customBorder = true;
			border.customBorderColor.uint32[0] = r;
			border.customBorderColor.uint32[1] = g;
			border.customBorderColor.uint32[2] = b;
			border.customBorderColor.uint32[3] = a;
			info.borderColor = VK_BORDER_COLOR_INT_CUSTOM_EXT;
		}
		else
		{
			info.borderColor = a == 0 ? VK_BORDER_COLOR_INT_TRANSPARENT_BLACK :
				(r || g || b) ? VK_BORDER_COLOR_INT_OPAQUE_WHITE : VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		}
	}
	else if (!IsStandardBorder(pSampler) && m_pDevice->HasCustomBorderColor())
	{
		customBorder = true;
		border.customBorderColor.float32[0] = pSampler->BorderColor[0];
		border.customBorderColor.float32[1] = pSampler->BorderColor[1];
		border.customBorderColor.float32[2] = pSampler->BorderColor[2];
		border.customBorderColor.float32[3] = pSampler->BorderColor[3];
		info.borderColor = VK_BORDER_COLOR_FLOAT_CUSTOM_EXT;
	}
	else info.borderColor = BorderColor(pSampler);

	if (customBorder)
	{
		border.pNext = info.pNext;
		info.pNext = &border;
	}

	info.magFilter = GetVKFilter(pSampler->Filter, false, true);
	info.minFilter = GetVKFilter(pSampler->Filter, true, false);
	info.mipmapMode = GetVKMipmapMode(pSampler->Filter);
	info.addressModeU = GetVKSamplerAddressMode(pSampler->AddressU);
	info.addressModeV = GetVKSamplerAddressMode(pSampler->AddressV);
	info.addressModeW = GetVKSamplerAddressMode(pSampler->AddressW);
	info.mipLodBias = pSampler->MipLODBias;
	// Match vkd3d: aniso from filter type, then MaxAnisotropy < 1 disables; clamp to 16.
	info.anisotropyEnable = IsAnisotropicSamplerFilter(pSampler->Filter) && pSampler->MaxAnisotropy >= 1;
	info.maxAnisotropy = info.anisotropyEnable ?
		(min)(16.0f, static_cast<float>(pSampler->MaxAnisotropy)) : 1.0f;
	const auto filterValue = static_cast<uint8_t>(pSampler->Filter);
	info.compareEnable = filterValue >= static_cast<uint8_t>(SamplerFilter::COMPARISON_MIN_MAG_MIP_POINT) &&
		filterValue <= static_cast<uint8_t>(SamplerFilter::COMPARISON_ANISOTROPIC);
	info.compareOp = GetVKCompareOp(pSampler->Comparison);
	info.minLod = pSampler->MinLOD;
	info.maxLod = pSampler->MaxLOD;
	info.unnormalizedCoordinates = (pSampler->Flags & SamplerFlag::NON_NORMALIZED_COORDINATES) != SamplerFlag::NONE;

	VkHostAddressRangeEXT host = {};
	host.address = pDescriptor;
	host.size = static_cast<size_t>(m_pDevice->GetDescriptorHeapProperties().samplerDescriptorSize);
	XUSG_VK_CHECK(m_pDevice->GetExtProcs().WriteSamplerDescriptors(m_pDevice->GetDevice(), 1, &info, &host), cerr, false);

	return true;
}

DescriptorTableLib_VK* XUSG::AsVK(DescriptorTableLib* p)
{
	assert(!p || p->GetBackend());
	return static_cast<DescriptorTableLib_VK*>(p ? p->GetBackend() : nullptr);
}
