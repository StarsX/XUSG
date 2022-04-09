//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSG_DX12.h"
#include "XUSGDescriptor_DX12.h"

using namespace std;
using namespace XUSG;

Util::DescriptorTable_DX12::DescriptorTable_DX12()
{
	m_key.resize(0);
}

Util::DescriptorTable_DX12::~DescriptorTable_DX12()
{
}

void Util::DescriptorTable_DX12::SetDescriptors(uint32_t start, uint32_t num,
	const Descriptor* srcDescriptors, uint8_t descriptorPoolIndex)
{
	const auto size = sizeof(Descriptor) * (start + num) + 1;
	if (size > m_key.size())
		m_key.resize(size);

	m_key[0] = descriptorPoolIndex;
	const auto descriptors = reinterpret_cast<Descriptor*>(&m_key[1]);
	memcpy(&descriptors[start], srcDescriptors, sizeof(Descriptor) * num);
}

void Util::DescriptorTable_DX12::SetSamplers(uint32_t start, uint32_t num, const SamplerPreset* presets,
	DescriptorTableCache* pDescriptorTableCache, uint8_t descriptorPoolIndex)
{
	const auto size = sizeof(SamplerDesc*) * (start + num) + 1;
	if (size > m_key.size())
		m_key.resize(size);

	m_key[0] = descriptorPoolIndex;
	const auto descriptors = reinterpret_cast<const SamplerDesc**>(&m_key[1]);

	for (auto i = 0u; i < num; ++i)
		descriptors[start + i] = pDescriptorTableCache->GetSampler(presets[i]).get();
}

DescriptorTable Util::DescriptorTable_DX12::CreateCbvSrvUavTable(DescriptorTableCache* pDescriptorTableCache, const XUSG::DescriptorTable& table)
{
	return dynamic_cast<DescriptorTableCache_DX12*>(pDescriptorTableCache)->createCbvSrvUavTable(m_key, table);
}

DescriptorTable Util::DescriptorTable_DX12::GetCbvSrvUavTable(DescriptorTableCache* pDescriptorTableCache, const XUSG::DescriptorTable& table)
{
	return dynamic_cast<DescriptorTableCache_DX12*>(pDescriptorTableCache)->getCbvSrvUavTable(m_key, table);
}

DescriptorTable Util::DescriptorTable_DX12::CreateSamplerTable(DescriptorTableCache* pDescriptorTableCache, const XUSG::DescriptorTable& table)
{
	return dynamic_cast<DescriptorTableCache_DX12*>(pDescriptorTableCache)->createSamplerTable(m_key, table);
}

DescriptorTable Util::DescriptorTable_DX12::GetSamplerTable(DescriptorTableCache* pDescriptorTableCache, const XUSG::DescriptorTable& table)
{
	return dynamic_cast<DescriptorTableCache_DX12*>(pDescriptorTableCache)->getSamplerTable(m_key, table);
}

Framebuffer Util::DescriptorTable_DX12::CreateFramebuffer(DescriptorTableCache* pDescriptorTableCache,
	const Descriptor* pDsv, const Framebuffer* pFramebuffer)
{
	return dynamic_cast<DescriptorTableCache_DX12*>(pDescriptorTableCache)->createFramebuffer(m_key, pDsv, pFramebuffer);
}

Framebuffer Util::DescriptorTable_DX12::GetFramebuffer(DescriptorTableCache* pDescriptorTableCache,
	const Descriptor* pDsv, const Framebuffer* pFramebuffer)
{
	return dynamic_cast<DescriptorTableCache_DX12*>(pDescriptorTableCache)->getFramebuffer(m_key, pDsv, pFramebuffer);
}

const string& Util::DescriptorTable_DX12::GetKey() const
{
	return m_key;
}

//--------------------------------------------------------------------------------------

DescriptorTableCache_DX12::DescriptorTableCache_DX12() :
	m_device(nullptr),
	m_cbvSrvUavTables(0),
	m_samplerTables(0),
	m_rtvTables(0),
	m_descriptorKeyPtrs(),
	m_descriptorPools(),
	m_descriptorStrides(),
	m_descriptorCounts(),
	m_samplerPresets()
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

DescriptorTableCache_DX12::DescriptorTableCache_DX12(const Device* pDevice, const wchar_t* name) :
	DescriptorTableCache_DX12()
{
	SetDevice(pDevice);
	SetName(name);
}

DescriptorTableCache_DX12::~DescriptorTableCache_DX12()
{
}

void DescriptorTableCache_DX12::SetDevice(const Device* pDevice)
{
	m_device = pDevice->GetHandle();

	assert(m_device);
	m_descriptorStrides[CBV_SRV_UAV_POOL] = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_descriptorStrides[SAMPLER_POOL] = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	m_descriptorStrides[RTV_POOL] = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
}

void DescriptorTableCache_DX12::SetName(const wchar_t* name)
{
	if (name) m_name = name;
}

void DescriptorTableCache_DX12::ResetDescriptorPool(DescriptorPoolType type, uint8_t index)
{
	//if (index < m_descriptorPools[type].size()) m_descriptorPools[type][index].Reset();
	if (index < m_descriptorCounts[type].size()) m_descriptorCounts[type][index] = 0;
	if (index < m_descriptorKeyPtrs[type].size())
	{
		switch (type)
		{
		case CBV_SRV_UAV_POOL:
			for (const auto& pKey : m_descriptorKeyPtrs[type][index])
				m_cbvSrvUavTables.erase(*pKey);
			break;
		case SAMPLER_POOL:
			for (const auto& pKey : m_descriptorKeyPtrs[type][index])
				m_samplerTables.erase(*pKey);
			break;
		case RTV_POOL:
			for (const auto& pKey : m_descriptorKeyPtrs[type][index])
				m_rtvTables.erase(*pKey);
			break;
		}

		m_descriptorKeyPtrs[type][index].clear();
	}
}

bool DescriptorTableCache_DX12::AllocateDescriptorPool(DescriptorPoolType type, uint32_t numDescriptors, uint8_t index)
{
	checkDescriptorPoolTypeStorage(type, index);

	return allocateDescriptorPool(type, numDescriptors, index);
}

DescriptorTable DescriptorTableCache_DX12::CreateCbvSrvUavTable(const Util::DescriptorTable* pUtil, const DescriptorTable& table)
{
	return createCbvSrvUavTable(pUtil->GetKey(), table);
}

DescriptorTable DescriptorTableCache_DX12::GetCbvSrvUavTable(const Util::DescriptorTable* pUtil, const DescriptorTable& table)
{
	return getCbvSrvUavTable(pUtil->GetKey(), table);
}

DescriptorTable DescriptorTableCache_DX12::CreateSamplerTable(const Util::DescriptorTable* pUtil, const DescriptorTable& table)
{
	return createSamplerTable(pUtil->GetKey(), table);
}

DescriptorTable DescriptorTableCache_DX12::GetSamplerTable(const Util::DescriptorTable* pUtil, const DescriptorTable& table)
{
	return getSamplerTable(pUtil->GetKey(), table);
}

Framebuffer DescriptorTableCache_DX12::CreateFramebuffer(const Util::DescriptorTable* pUtil,
	const Descriptor* pDsv, const Framebuffer* pFramebuffer)
{
	return createFramebuffer(pUtil->GetKey(), pDsv, pFramebuffer);
}

Framebuffer DescriptorTableCache_DX12::GetFramebuffer(const Util::DescriptorTable* pUtil,
	const Descriptor* pDsv, const Framebuffer* pFramebuffer)
{
	return getFramebuffer(pUtil->GetKey(), pDsv, pFramebuffer);
}

DescriptorPool DescriptorTableCache_DX12::GetDescriptorPool(DescriptorPoolType type, uint8_t index) const
{
	return m_descriptorPools[type][index].get();
}

const Sampler& DescriptorTableCache_DX12::GetSampler(SamplerPreset preset)
{
	if (m_samplerPresets[preset] == nullptr)
		m_samplerPresets[preset] = m_pfnSamplers[preset]();

	return m_samplerPresets[preset];
}

uint32_t DescriptorTableCache_DX12::GetDescriptorStride(DescriptorPoolType type) const
{
	return m_descriptorStrides[type];
}

void DescriptorTableCache_DX12::checkDescriptorPoolTypeStorage(DescriptorPoolType type, uint8_t index)
{
	if (index >= m_descriptorPools[type].size())
		m_descriptorPools[type].resize(index + 1);

	if (index >= m_descriptorCounts[type].size())
		m_descriptorCounts[type].resize(index + 1);

	if (index >= m_descriptorKeyPtrs[type].size())
		m_descriptorKeyPtrs[type].resize(index + 1);
}

bool DescriptorTableCache_DX12::allocateDescriptorPool(DescriptorPoolType type, uint32_t numDescriptors, uint8_t index)
{
	static const D3D12_DESCRIPTOR_HEAP_TYPE heapTypes[NUM_DESCRIPTOR_POOL] =
	{
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV
	};

	static const wchar_t* poolNames[] =
	{
		L".CbvSrvUavPool",
		L".SamplerPool",
		L".RtvPool"
	};

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = numDescriptors;
	desc.Type = heapTypes[type];
	if (type != D3D12_DESCRIPTOR_HEAP_TYPE_RTV) desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	m_descriptorPools[type][index] = nullptr;
	V_RETURN(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_descriptorPools[type][index])), cerr, false);
	if (!m_name.empty()) m_descriptorPools[type][index]->SetName((m_name + poolNames[type]).c_str());

	m_descriptorCounts[type][index] = 0;

	return true;
}

bool DescriptorTableCache_DX12::reallocateCbvSrvUavPool(const string& key)
{
	assert(key.size() > 0);
	const auto& index = key[0];
	const auto numDescriptors = static_cast<uint32_t>(key.size() / sizeof(Descriptor));

	checkDescriptorPoolTypeStorage(CBV_SRV_UAV_POOL, index);

	// Allocate a new pool if neccessary
	const auto& descriptorPool = m_descriptorPools[CBV_SRV_UAV_POOL][index];
	const auto descriptorCount = calculateGrowth(numDescriptors, CBV_SRV_UAV_POOL, index);
	if (!descriptorPool || descriptorPool->GetDesc().NumDescriptors < descriptorCount)
	{
		XUSG_N_RETURN(allocateDescriptorPool(CBV_SRV_UAV_POOL, descriptorCount, index), false);

		// Recreate descriptor tables
		for (const auto& pKey : m_descriptorKeyPtrs[CBV_SRV_UAV_POOL][index])
			*m_cbvSrvUavTables[*pKey] = *createCbvSrvUavTable(*pKey, nullptr);
	}

	return true;
}

bool DescriptorTableCache_DX12::reallocateSamplerPool(const string& key)
{
	assert(key.size() > 0);
	const auto& index = key[0];
	const auto numDescriptors = static_cast<uint32_t>(key.size() / sizeof(Sampler*));

	checkDescriptorPoolTypeStorage(SAMPLER_POOL, index);

	// Allocate a new pool if neccessary
	const auto& descriptorPool = m_descriptorPools[SAMPLER_POOL][index];
	const auto descriptorCount = calculateGrowth(numDescriptors, SAMPLER_POOL, index);
	if (!descriptorPool || descriptorPool->GetDesc().NumDescriptors < descriptorCount)
	{
		XUSG_N_RETURN(allocateDescriptorPool(SAMPLER_POOL, descriptorCount, index), false);

		// Recreate descriptor tables
		for (const auto& pKey : m_descriptorKeyPtrs[SAMPLER_POOL][index])
			*m_samplerTables[*pKey] = *createSamplerTable(*pKey, nullptr);
	}

	return true;
}

bool DescriptorTableCache_DX12::reallocateRtvPool(const string& key)
{
	assert(key.size() > 0);
	const auto& index = key[0];
	const auto numDescriptors = static_cast<uint32_t>(key.size() / sizeof(Descriptor));

	checkDescriptorPoolTypeStorage(RTV_POOL, index);

	// Allocate a new pool if neccessary
	const auto& descriptorPool = m_descriptorPools[RTV_POOL][index];
	const auto descriptorCount = calculateGrowth(numDescriptors, RTV_POOL, index);
	if (!descriptorPool || descriptorPool->GetDesc().NumDescriptors < descriptorCount)
	{
		XUSG_N_RETURN(allocateDescriptorPool(RTV_POOL, descriptorCount, index), false);

		// Recreate descriptor tables
		for (const auto& pKey : m_descriptorKeyPtrs[RTV_POOL][index])
			*m_rtvTables[*pKey] = *createFramebuffer(*pKey, nullptr, nullptr).RenderTargetViews;
	}

	return true;
}

DescriptorTable DescriptorTableCache_DX12::createCbvSrvUavTable(const string& key, DescriptorTable table)
{
	if (key.size() > 0)
	{
		const auto& index = key[0];
		const auto numDescriptors = static_cast<uint32_t>(key.size() / sizeof(Descriptor));
		const auto pSrc = reinterpret_cast<const D3D12_CPU_DESCRIPTOR_HANDLE*>(&key[1]);

		// Compute start addresses for CPU and GPU handles
		const auto& descriptorPool = m_descriptorPools[CBV_SRV_UAV_POOL][index];
		const auto& descriptorStride = m_descriptorStrides[CBV_SRV_UAV_POOL];
		CD3DX12_CPU_DESCRIPTOR_HANDLE dst(descriptorPool->GetCPUDescriptorHandleForHeapStart());

		if (table && *table)
			dst.Offset(static_cast<int>(*table - descriptorPool->GetGPUDescriptorHandleForHeapStart().ptr));
		else
		{
			auto& descriptorCount = m_descriptorCounts[CBV_SRV_UAV_POOL][index];
			table = make_shared<uint64_t>(CD3DX12_GPU_DESCRIPTOR_HANDLE(
				descriptorPool->GetGPUDescriptorHandleForHeapStart(),
				descriptorCount, descriptorStride).ptr);
			dst.Offset(descriptorCount, descriptorStride);
			descriptorCount += numDescriptors;
		}

		// Create a descriptor table
		for (auto i = 0u; i < numDescriptors; ++i)
		{
			// Copy a descriptor
			m_device->CopyDescriptorsSimple(1, dst, pSrc[i], D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			dst.Offset(descriptorStride);
		}

		return table;
	}

	return nullptr;
}

DescriptorTable DescriptorTableCache_DX12::getCbvSrvUavTable(const string& key, DescriptorTable table)
{
	if (key.size() > 0)
	{
		const auto tableIter = m_cbvSrvUavTables.find(key);

		// Create one, if it does not exist
		if (tableIter == m_cbvSrvUavTables.end() && reallocateCbvSrvUavPool(key))
		{
			const auto& index = key[0];
			auto& descriptorKeyPtrs = m_descriptorKeyPtrs[CBV_SRV_UAV_POOL][index];
			if (table && *table)
			{
				for (auto mIt = m_cbvSrvUavTables.cbegin(); mIt != m_cbvSrvUavTables.cend(); ++mIt)
				{
					if (mIt->second == table)
					{
						for (auto vIt = descriptorKeyPtrs.cbegin(); vIt != descriptorKeyPtrs.cend(); ++vIt)
						{
							if (*vIt == &mIt->first)
							{
								descriptorKeyPtrs.erase(vIt);
								break;
							}
						}

						m_cbvSrvUavTables.erase(mIt);
						break;
					}
				}
			}

			table = createCbvSrvUavTable(key, table);
			m_cbvSrvUavTables[key] = table;
			descriptorKeyPtrs.emplace_back(&m_cbvSrvUavTables.find(key)->first);

			return table;
		}

		return tableIter->second;
	}

	return nullptr;
}

DescriptorTable DescriptorTableCache_DX12::createSamplerTable(const string& key, DescriptorTable table)
{
	if (key.size() > 0)
	{
		const auto& index = key[0];
		const auto numDescriptors = static_cast<uint32_t>(key.size() / sizeof(uintptr_t));
		const auto pSamplers = reinterpret_cast<const SamplerDesc* const*>(&key[1]);

		// Compute start addresses for CPU and GPU handles
		const auto& descriptorPool = m_descriptorPools[SAMPLER_POOL][index];
		const auto& descriptorStride = m_descriptorStrides[SAMPLER_POOL];
		CD3DX12_CPU_DESCRIPTOR_HANDLE dst(descriptorPool->GetCPUDescriptorHandleForHeapStart());

		if (table && *table)
			dst.Offset(static_cast<int>(*table - descriptorPool->GetGPUDescriptorHandleForHeapStart().ptr));
		else
		{
			auto& descriptorCount = m_descriptorCounts[SAMPLER_POOL][index];
			table = make_shared<uint64_t>(CD3DX12_GPU_DESCRIPTOR_HANDLE(
				descriptorPool->GetGPUDescriptorHandleForHeapStart(),
				descriptorCount, descriptorStride).ptr);
			dst.Offset(descriptorCount, descriptorStride);
			descriptorCount += numDescriptors;
		}

		// Create a descriptor table
		for (auto i = 0u; i < numDescriptors; ++i)
		{
			D3D12_SAMPLER_DESC desc;
			desc.Filter = static_cast<D3D12_FILTER>(pSamplers[i]->Filter);
			desc.AddressU = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(pSamplers[i]->AddressU);
			desc.AddressV = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(pSamplers[i]->AddressV);
			desc.AddressW = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(pSamplers[i]->AddressW);
			desc.MipLODBias = pSamplers[i]->MipLODBias;
			desc.MaxAnisotropy = pSamplers[i]->MaxAnisotropy;
			desc.ComparisonFunc = static_cast<D3D12_COMPARISON_FUNC>(pSamplers[i]->ComparisonFunc);
			desc.BorderColor[0] = pSamplers[i]->BorderColor[0];
			desc.BorderColor[1] = pSamplers[i]->BorderColor[1];
			desc.BorderColor[2] = pSamplers[i]->BorderColor[2];
			desc.BorderColor[3] = pSamplers[i]->BorderColor[3];
			desc.MinLOD = pSamplers[i]->MinLOD;
			desc.MaxLOD = pSamplers[i]->MaxLOD;

			// Copy a descriptor
			m_device->CreateSampler(&desc, dst);
			dst.Offset(descriptorStride);
		}

		return table;
	}

	return nullptr;
}

DescriptorTable DescriptorTableCache_DX12::getSamplerTable(const string& key, DescriptorTable table)
{
	if (key.size() > 0)
	{
		const auto tableIter = m_samplerTables.find(key);

		// Create one, if it does not exist
		if (tableIter == m_samplerTables.end() && reallocateSamplerPool(key))
		{
			const auto& index = key[0];
			auto& descriptorKeyPtrs = m_descriptorKeyPtrs[SAMPLER_POOL][index];
			if (table && *table)
			{
				for (auto mIt = m_samplerTables.cbegin(); mIt != m_samplerTables.cend(); ++mIt)
				{
					if (mIt->second == table)
					{
						for (auto vIt = descriptorKeyPtrs.cbegin(); vIt != descriptorKeyPtrs.cend(); ++vIt)
						{
							if (*vIt == &mIt->first)
							{
								descriptorKeyPtrs.erase(vIt);
								break;
							}
						}

						m_samplerTables.erase(mIt);
						break;
					}
				}
			}

			table = createSamplerTable(key, table);
			m_samplerTables[key] = table;
			descriptorKeyPtrs.emplace_back(&m_samplerTables.find(key)->first);

			return table;
		}

		return tableIter->second;
	}

	return nullptr;
}

Framebuffer DescriptorTableCache_DX12::createFramebuffer(const string& key,
	const Descriptor* pDsv, const Framebuffer* pFramebuffer)
{
	Framebuffer framebuffer = {};
	if (pDsv) framebuffer.DepthStencilView = *pDsv;

	if (key.size() > 0)
	{
		const auto& index = key[0];
		framebuffer.NumRenderTargetDescriptors = static_cast<uint32_t>(key.size() / sizeof(Descriptor));
		const auto pSrc = reinterpret_cast<const D3D12_CPU_DESCRIPTOR_HANDLE*>(&key[1]);

		// Compute start addresses for CPU and GPU handles
		const auto& descriptorPool = m_descriptorPools[RTV_POOL][index];
		const auto& descriptorStride = m_descriptorStrides[RTV_POOL];
		CD3DX12_CPU_DESCRIPTOR_HANDLE dst(descriptorPool->GetCPUDescriptorHandleForHeapStart());

		if (pFramebuffer && pFramebuffer->RenderTargetViews)
		{
			dst = { *pFramebuffer->RenderTargetViews };
			framebuffer = *pFramebuffer;
		}
		else
		{
			auto& descriptorCount = m_descriptorCounts[RTV_POOL][index];
			dst.Offset(descriptorCount, descriptorStride);
			descriptorCount += framebuffer.NumRenderTargetDescriptors;
			framebuffer.RenderTargetViews = make_shared<Descriptor>();
			*framebuffer.RenderTargetViews = dst.ptr;
		}

		// Create a descriptor table
		for (auto i = 0u; i < framebuffer.NumRenderTargetDescriptors; ++i)
		{
			// Copy a descriptor
			m_device->CopyDescriptorsSimple(1, dst, pSrc[i], D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			dst.Offset(descriptorStride);
		}
	}

	return framebuffer;
}

Framebuffer DescriptorTableCache_DX12::getFramebuffer(const string& key,
	const Descriptor* pDsv, const Framebuffer* pFramebuffer)
{
	Framebuffer framebuffer = {};
	if (pDsv) framebuffer.DepthStencilView = *pDsv;

	if (key.size() > 0)
	{
		const auto tableIter = m_rtvTables.find(key);

		// Create one, if it does not exist
		if (tableIter == m_rtvTables.end() && reallocateRtvPool(key))
		{
			const auto& index = key[0];
			auto& descriptorKeyPtrs = m_descriptorKeyPtrs[RTV_POOL][index];
			if (pFramebuffer && pFramebuffer->RenderTargetViews)
			{
				for (auto mIt = m_rtvTables.cbegin(); mIt != m_rtvTables.cend(); ++mIt)
				{
					if (mIt->second == pFramebuffer->RenderTargetViews)
					{
						for (auto vIt = descriptorKeyPtrs.cbegin(); vIt != descriptorKeyPtrs.cend(); ++vIt)
						{
							if (*vIt == &mIt->first)
							{
								descriptorKeyPtrs.erase(vIt);
								break;
							}
						}

						m_rtvTables.erase(mIt);
						break;
					}
				}
			}

			framebuffer = createFramebuffer(key, pDsv, pFramebuffer);
			m_rtvTables[key] = framebuffer.RenderTargetViews;
			descriptorKeyPtrs.emplace_back(&m_rtvTables.find(key)->first);
		}
		else
		{
			framebuffer.NumRenderTargetDescriptors = static_cast<uint32_t>(key.size() / sizeof(Descriptor));
			framebuffer.RenderTargetViews = tableIter->second;
		}
	}

	return framebuffer;
}

uint32_t DescriptorTableCache_DX12::calculateGrowth(uint32_t numDescriptors,
	DescriptorPoolType type, uint8_t index) const
{
	const auto& oldCapacity = m_descriptorCounts[type][index];
	const auto halfOldCapacity = oldCapacity >> 1;
	const auto newSize = oldCapacity + numDescriptors;

	const auto maxSize = type == SAMPLER_POOL ?
		D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE :
		D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_1;

	// geometric growth would overflow
	if (oldCapacity > maxSize - halfOldCapacity) return newSize;

	const auto geometric = oldCapacity + halfOldCapacity;

	return (max)(geometric, newSize); // geometric growth might be insufficient
}
