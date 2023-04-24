//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSG_DX12.h"
#include "XUSGDescriptor_DX12.h"
#include "XUSGEnum_DX12.h"

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
	const Descriptor* srcDescriptors, uint8_t descriptorHeapIndex)
{
	const auto size = sizeof(Descriptor) * (start + num) + 1;
	if (size > m_key.size())
		m_key.resize(size);

	m_key[0] = descriptorHeapIndex;
	const auto descriptors = reinterpret_cast<Descriptor*>(&m_key[1]);
	memcpy(&descriptors[start], srcDescriptors, sizeof(Descriptor) * num);
}

void Util::DescriptorTable_DX12::SetSamplers(uint32_t start, uint32_t num,
	const Sampler* const* ppSamplers, uint8_t descriptorHeapIndex)
{
	const auto size = sizeof(Sampler*) * (start + num) + 1;
	if (size > m_key.size())
		m_key.resize(size);

	m_key[0] = descriptorHeapIndex;
	const auto descriptors = reinterpret_cast<const Sampler**>(&m_key[1]);

	for (auto i = 0u; i < num; ++i) descriptors[start + i] = ppSamplers[i];
}

void Util::DescriptorTable_DX12::SetSamplers(uint32_t start, uint32_t num, const SamplerPreset* presets,
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

DescriptorTable Util::DescriptorTable_DX12::CreateCbvSrvUavTable(DescriptorTableLib* pDescriptorTableLib, const XUSG::DescriptorTable& table)
{
	return dynamic_cast<DescriptorTableLib_DX12*>(pDescriptorTableLib)->CreateCbvSrvUavTable(this, table);
}

DescriptorTable Util::DescriptorTable_DX12::GetCbvSrvUavTable(DescriptorTableLib* pDescriptorTableLib, const XUSG::DescriptorTable& table)
{
	return dynamic_cast<DescriptorTableLib_DX12*>(pDescriptorTableLib)->GetCbvSrvUavTable(this, table);
}

DescriptorTable Util::DescriptorTable_DX12::CreateSamplerTable(DescriptorTableLib* pDescriptorTableLib, const XUSG::DescriptorTable& table)
{
	return dynamic_cast<DescriptorTableLib_DX12*>(pDescriptorTableLib)->CreateSamplerTable(this, table);
}

DescriptorTable Util::DescriptorTable_DX12::GetSamplerTable(DescriptorTableLib* pDescriptorTableLib, const XUSG::DescriptorTable& table)
{
	return dynamic_cast<DescriptorTableLib_DX12*>(pDescriptorTableLib)->GetSamplerTable(this, table);
}

Framebuffer Util::DescriptorTable_DX12::CreateFramebuffer(DescriptorTableLib* pDescriptorTableLib,
	const Descriptor* pDsv, const Framebuffer* pFramebuffer)
{
	return dynamic_cast<DescriptorTableLib_DX12*>(pDescriptorTableLib)->CreateFramebuffer(this, pDsv, pFramebuffer);
}

Framebuffer Util::DescriptorTable_DX12::GetFramebuffer(DescriptorTableLib* pDescriptorTableLib,
	const Descriptor* pDsv, const Framebuffer* pFramebuffer)
{
	return dynamic_cast<DescriptorTableLib_DX12*>(pDescriptorTableLib)->GetFramebuffer(this, pDsv, pFramebuffer);
}

const string& Util::DescriptorTable_DX12::GetKey() const
{
	return m_key;
}

uint32_t Util::DescriptorTable_DX12::CreateCbvSrvUavTableIndex(DescriptorTableLib* pDescriptorTableLib, XUSG::DescriptorTable table)
{
	table = CreateCbvSrvUavTable(pDescriptorTableLib, table);

	return GetDescriptorTableIndex(pDescriptorTableLib, CBV_SRV_UAV_HEAP, table);
}

uint32_t Util::DescriptorTable_DX12::GetCbvSrvUavTableIndex(DescriptorTableLib* pDescriptorTableLib, XUSG::DescriptorTable table)
{
	table = GetCbvSrvUavTable(pDescriptorTableLib, table);

	return GetDescriptorTableIndex(pDescriptorTableLib, CBV_SRV_UAV_HEAP, table);
}

uint32_t Util::DescriptorTable_DX12::CreateSamplerTableIndex(DescriptorTableLib* pDescriptorTableLib, XUSG::DescriptorTable table)
{
	table = CreateSamplerTable(pDescriptorTableLib, table);

	return GetDescriptorTableIndex(pDescriptorTableLib, SAMPLER_HEAP, table);
}

uint32_t Util::DescriptorTable_DX12::GetSamplerTableIndex(DescriptorTableLib* pDescriptorTableLib, XUSG::DescriptorTable table)
{
	table = GetSamplerTable(pDescriptorTableLib, table);

	return GetDescriptorTableIndex(pDescriptorTableLib, SAMPLER_HEAP, table);
}

uint32_t Util::DescriptorTable_DX12::GetDescriptorTableIndex(DescriptorTableLib* pDescriptorTableLib, DescriptorHeapType type,
	const XUSG::DescriptorTable& table) const
{
	if (!m_key.empty() && table)
	{
		const auto& index = m_key[0];
		const auto& descriptorHeap = static_cast<ID3D12DescriptorHeap*>(pDescriptorTableLib->GetDescriptorHeap(type, index));
		const auto& descriptorStride = pDescriptorTableLib->GetDescriptorStride(type);

		return static_cast<uint32_t>((*table - descriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr) / descriptorStride);
	}

	return UINT32_MAX;
}

//--------------------------------------------------------------------------------------

DescriptorTableLib_DX12::DescriptorTableLib_DX12() :
	m_device(nullptr),
	m_cbvSrvUavTables(),
	m_samplerTables(),
	m_rtvTables(),
	m_descriptorHeaps(),
	m_descriptorStrides(),
	m_descriptorCounts(),
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

DescriptorTableLib_DX12::DescriptorTableLib_DX12(const Device* pDevice, const wchar_t* name) :
	DescriptorTableLib_DX12()
{
	SetDevice(pDevice);
	SetName(name);
}

DescriptorTableLib_DX12::~DescriptorTableLib_DX12()
{
}

void DescriptorTableLib_DX12::SetDevice(const Device* pDevice)
{
	m_device = pDevice->GetHandle();

	assert(m_device);
	m_descriptorStrides[CBV_SRV_UAV_HEAP] = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_descriptorStrides[SAMPLER_HEAP] = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	m_descriptorStrides[RTV_HEAP] = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
}

void DescriptorTableLib_DX12::SetName(const wchar_t* name)
{
	if (name) m_name = name;
}

void DescriptorTableLib_DX12::ResetDescriptorHeap(DescriptorHeapType type, uint8_t index)
{
	//if (index < m_descriptorHeaps[type].size()) m_descriptorHeaps[type][index].Reset();
	if (index < m_descriptorCounts[type].size()) m_descriptorCounts[type][index] = 0;
	
	switch (type)
	{
	case CBV_SRV_UAV_HEAP:
		if (index < m_cbvSrvUavTables.size()) m_cbvSrvUavTables[index].clear();
		break;
	case SAMPLER_HEAP:
		if (index < m_samplerTables.size()) m_samplerTables[index].clear();
		break;
	case RTV_HEAP:
		if (index < m_rtvTables.size()) m_rtvTables[index].clear();
		break;
	}
}

bool DescriptorTableLib_DX12::AllocateDescriptorHeap(DescriptorHeapType type, uint32_t numDescriptors, uint8_t index)
{
	checkDescriptorHeapTypeStorage(type, index);

	return allocateDescriptorHeap(type, numDescriptors, index);
}

DescriptorTable DescriptorTableLib_DX12::CreateCbvSrvUavTable(const Util::DescriptorTable* pUtil, const DescriptorTable& table)
{
	return createCbvSrvUavTable(pUtil->GetKey(), table);
}

DescriptorTable DescriptorTableLib_DX12::GetCbvSrvUavTable(const Util::DescriptorTable* pUtil, const DescriptorTable& table)
{
	return getCbvSrvUavTable(pUtil->GetKey(), table);
}

DescriptorTable DescriptorTableLib_DX12::CreateSamplerTable(const Util::DescriptorTable* pUtil, const DescriptorTable& table)
{
	return createSamplerTable(pUtil->GetKey(), table);
}

DescriptorTable DescriptorTableLib_DX12::GetSamplerTable(const Util::DescriptorTable* pUtil, const DescriptorTable& table)
{
	return getSamplerTable(pUtil->GetKey(), table);
}

Framebuffer DescriptorTableLib_DX12::CreateFramebuffer(const Util::DescriptorTable* pUtil,
	const Descriptor* pDsv, const Framebuffer* pFramebuffer)
{
	return createFramebuffer(pUtil->GetKey(), pDsv, pFramebuffer);
}

Framebuffer DescriptorTableLib_DX12::GetFramebuffer(const Util::DescriptorTable* pUtil,
	const Descriptor* pDsv, const Framebuffer* pFramebuffer)
{
	return getFramebuffer(pUtil->GetKey(), pDsv, pFramebuffer);
}

DescriptorHeap DescriptorTableLib_DX12::GetDescriptorHeap(DescriptorHeapType type, uint8_t index) const
{
	return m_descriptorHeaps[type][index].get();
}

const Sampler* DescriptorTableLib_DX12::GetSampler(SamplerPreset preset)
{
	if (m_samplers[preset] == nullptr)
		m_samplers[preset] = make_unique<Sampler>(m_pfnSamplers[preset]());

	return m_samplers[preset].get();
}

uint32_t DescriptorTableLib_DX12::GetDescriptorStride(DescriptorHeapType type) const
{
	return m_descriptorStrides[type];
}

void DescriptorTableLib_DX12::checkDescriptorHeapTypeStorage(DescriptorHeapType type, uint8_t index)
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
	case RTV_HEAP:
		if (index >= m_rtvTables.size()) m_rtvTables.resize(index + 1);
		break;
	}
}

bool DescriptorTableLib_DX12::allocateDescriptorHeap(DescriptorHeapType type, uint32_t numDescriptors, uint8_t index)
{
	static const D3D12_DESCRIPTOR_HEAP_TYPE heapTypes[NUM_DESCRIPTOR_HEAP] =
	{
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV
	};

	static const wchar_t* heapNames[] =
	{
		L".CbvSrvUavHeap",
		L".SamplerHeap",
		L".RtvHeap"
	};

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = numDescriptors;
	desc.Type = heapTypes[type];
	if (type != D3D12_DESCRIPTOR_HEAP_TYPE_RTV) desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	m_descriptorHeaps[type][index] = nullptr;
	V_RETURN(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_descriptorHeaps[type][index])), cerr, false);
	if (!m_name.empty()) m_descriptorHeaps[type][index]->SetName((m_name + heapNames[type] + to_wstring(index)).c_str());

	m_descriptorCounts[type][index] = 0;

	return true;
}

bool DescriptorTableLib_DX12::reallocateCbvSrvUavHeap(const string& key)
{
	assert(!key.empty());
	const auto& index = key[0];
	auto numDescriptors = static_cast<uint32_t>(key.size() / sizeof(Descriptor));

	// Allocate a new heap if neccessary
	const auto& descriptorHeap = m_descriptorHeaps[CBV_SRV_UAV_HEAP][index];
	const auto descriptorCount = m_descriptorCounts[CBV_SRV_UAV_HEAP][index];
	numDescriptors += descriptorCount;
	if (!descriptorHeap || descriptorHeap->GetDesc().NumDescriptors < numDescriptors)
	{
		const auto oldHeapStart = descriptorHeap ? descriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr : 0;
		numDescriptors = calculateGrowth(numDescriptors, CBV_SRV_UAV_HEAP, index);
		XUSG_N_RETURN(allocateDescriptorHeap(CBV_SRV_UAV_HEAP, numDescriptors, index), false);

		// Recreate descriptor tables
		assert(descriptorHeap);
		const auto newHeapStart = descriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr;
		m_descriptorCounts[CBV_SRV_UAV_HEAP][index] = descriptorCount;
		for (const auto& tableEntry : m_cbvSrvUavTables[index])
		{
			*tableEntry.second = *tableEntry.second - oldHeapStart + newHeapStart;
			*tableEntry.second = *createCbvSrvUavTable(tableEntry.first, tableEntry.second);
		}
	}

	return true;
}

bool DescriptorTableLib_DX12::reallocateSamplerHeap(const string& key)
{
	assert(!key.empty());
	const auto& index = key[0];
	auto numDescriptors = static_cast<uint32_t>(key.size() / sizeof(Sampler*));

	// Allocate a new heap if neccessary
	const auto& descriptorHeap = m_descriptorHeaps[SAMPLER_HEAP][index];
	const auto descriptorCount = m_descriptorCounts[SAMPLER_HEAP][index];
	numDescriptors += descriptorCount;
	if (!descriptorHeap || descriptorHeap->GetDesc().NumDescriptors < numDescriptors)
	{
		const auto oldHeapStart = descriptorHeap ? descriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr : 0;
		numDescriptors = calculateGrowth(numDescriptors, SAMPLER_HEAP, index);
		XUSG_N_RETURN(allocateDescriptorHeap(SAMPLER_HEAP, numDescriptors, index), false);

		// Recreate descriptor tables
		assert(descriptorHeap);
		const auto newHeapStart = descriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr;
		m_descriptorCounts[SAMPLER_HEAP][index] = descriptorCount;
		for (const auto& tableEntry : m_samplerTables[index])
		{
			*tableEntry.second = *tableEntry.second - oldHeapStart + newHeapStart;
			*tableEntry.second = *createSamplerTable(tableEntry.first, tableEntry.second);
		}
	}

	return true;
}

bool DescriptorTableLib_DX12::reallocateRtvHeap(const string& key)
{
	assert(!key.empty());
	const auto& index = key[0];
	auto numDescriptors = static_cast<uint32_t>(key.size() / sizeof(Descriptor));

	// Allocate a new heap if neccessary
	const auto& descriptorHeap = m_descriptorHeaps[RTV_HEAP][index];
	numDescriptors += m_descriptorCounts[RTV_HEAP][index];
	if (!descriptorHeap || descriptorHeap->GetDesc().NumDescriptors < numDescriptors)
	{
		numDescriptors = calculateGrowth(numDescriptors, RTV_HEAP, index);
		XUSG_N_RETURN(allocateDescriptorHeap(RTV_HEAP, numDescriptors, index), false);

		// Recreate descriptor tables
		for (const auto& tableEntry : m_rtvTables[index])
			*tableEntry.second = *createFramebuffer(tableEntry.first, nullptr, nullptr).RenderTargetViews;
	}

	return true;
}

DescriptorTable DescriptorTableLib_DX12::createCbvSrvUavTable(const string& key, DescriptorTable table)
{
	if (!key.empty())
	{
		const auto& index = key[0];
		const auto numDescriptors = static_cast<uint32_t>(key.size() / sizeof(Descriptor));
		const auto pSrc = reinterpret_cast<const D3D12_CPU_DESCRIPTOR_HANDLE*>(&key[1]);

		// Compute start addresses for CPU and GPU handles
		const auto& descriptorHeap = m_descriptorHeaps[CBV_SRV_UAV_HEAP][index];
		const auto& descriptorStride = m_descriptorStrides[CBV_SRV_UAV_HEAP];
		CD3DX12_CPU_DESCRIPTOR_HANDLE dst(descriptorHeap->GetCPUDescriptorHandleForHeapStart());

		if (table && *table)
			dst.Offset(static_cast<int>(*table - descriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr));
		else
		{
			auto& descriptorCount = m_descriptorCounts[CBV_SRV_UAV_HEAP][index];
			table = make_shared<uint64_t>(CD3DX12_GPU_DESCRIPTOR_HANDLE(
				descriptorHeap->GetGPUDescriptorHandleForHeapStart(),
				descriptorCount, descriptorStride).ptr);
			dst.Offset(descriptorCount, descriptorStride);
			descriptorCount += numDescriptors;
		}

		// Create a descriptor table
		for (auto i = 0u; i < numDescriptors; ++i)
		{
			// Copy a descriptor
			const auto& src = pSrc[i];
			if (src.ptr) m_device->CopyDescriptorsSimple(1, dst, src, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			dst.Offset(descriptorStride);
		}

		return table;
	}

	return nullptr;
}

DescriptorTable DescriptorTableLib_DX12::getCbvSrvUavTable(const string& key, DescriptorTable table)
{
	if (!key.empty())
	{
		const auto& index = key[0];
		checkDescriptorHeapTypeStorage(CBV_SRV_UAV_HEAP, index);
		auto& cbvSrvUavTables = m_cbvSrvUavTables[index];
		const auto tableIter = cbvSrvUavTables.find(key);

		// Create one, if it does not exist
		if (tableIter == cbvSrvUavTables.end() && reallocateCbvSrvUavHeap(key))
		{
			if (table && *table)
			{
				for (auto mIt = cbvSrvUavTables.cbegin(); mIt != cbvSrvUavTables.cend(); ++mIt)
				{
					if (mIt->second && *mIt->second == *table)
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

	return nullptr;
}

DescriptorTable DescriptorTableLib_DX12::createSamplerTable(const string& key, DescriptorTable table)
{
	if (!key.empty())
	{
		const auto& index = key[0];
		const auto numDescriptors = static_cast<uint32_t>(key.size() / sizeof(uintptr_t));
		const auto pSamplers = reinterpret_cast<Sampler* const*>(&key[1]);

		// Compute start addresses for CPU and GPU handles
		const auto& descriptorHeap = m_descriptorHeaps[SAMPLER_HEAP][index];
		const auto& descriptorStride = m_descriptorStrides[SAMPLER_HEAP];
		CD3DX12_CPU_DESCRIPTOR_HANDLE dst(descriptorHeap->GetCPUDescriptorHandleForHeapStart());

		if (table && *table)
			dst.Offset(static_cast<int>(*table - descriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr));
		else
		{
			auto& descriptorCount = m_descriptorCounts[SAMPLER_HEAP][index];
			table = make_shared<uint64_t>(CD3DX12_GPU_DESCRIPTOR_HANDLE(
				descriptorHeap->GetGPUDescriptorHandleForHeapStart(),
				descriptorCount, descriptorStride).ptr);
			dst.Offset(descriptorCount, descriptorStride);
			descriptorCount += numDescriptors;
		}

		// Create a descriptor table
		for (auto i = 0u; i < numDescriptors; ++i)
		{
			const auto& pSampler = pSamplers[i];
			D3D12_SAMPLER_DESC2 desc;
			desc.Filter = GetDX12Filter(pSampler->Filter);
			desc.AddressU = GetDX12TextureAddressMode(pSampler->AddressU);
			desc.AddressV = GetDX12TextureAddressMode(pSampler->AddressV);
			desc.AddressW = GetDX12TextureAddressMode(pSampler->AddressW);
			desc.MipLODBias = pSampler->MipLODBias;
			desc.MaxAnisotropy = pSampler->MaxAnisotropy;
			desc.ComparisonFunc = GetDX12ComparisonFunc(pSampler->Comparison);
			desc.UintBorderColor[0] = pSampler->BorderColorU[0];
			desc.UintBorderColor[1] = pSampler->BorderColorU[1];
			desc.UintBorderColor[2] = pSampler->BorderColorU[2];
			desc.UintBorderColor[3] = pSampler->BorderColorU[3];
			desc.MinLOD = pSampler->MinLOD;
			desc.MaxLOD = pSampler->MaxLOD;
			desc.Flags = GetDX12SamplerFlags(pSampler->Flags);

			// Create a sampler descriptor
			if ((pSampler->Flags & (SamplerFlag::UINT_BORDER_COLOR | SamplerFlag::NON_NORMALIZED_COORDINATES)) != SamplerFlag::NONE)
			{
				com_ptr<ID3D12Device11> dxDevice;
				V_RETURN(m_device->QueryInterface(IID_PPV_ARGS(&dxDevice)), cerr, nullptr);
				dxDevice->CreateSampler2(&desc, dst);
			}
			else m_device->CreateSampler(reinterpret_cast<D3D12_SAMPLER_DESC*>(&desc), dst);

			dst.Offset(descriptorStride);
		}

		return table;
	}

	return nullptr;
}

DescriptorTable DescriptorTableLib_DX12::getSamplerTable(const string& key, DescriptorTable table)
{
	if (!key.empty())
	{
		const auto& index = key[0];
		checkDescriptorHeapTypeStorage(SAMPLER_HEAP, index);
		auto& samplerTables = m_samplerTables[index];
		const auto tableIter = samplerTables.find(key);

		// Create one, if it does not exist
		if (tableIter == samplerTables.end() && reallocateSamplerHeap(key))
		{
			if (table && *table)
			{
				for (auto mIt = samplerTables.cbegin(); mIt != samplerTables.cend(); ++mIt)
				{
					if (mIt->second && *mIt->second == *table)
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

	return nullptr;
}

Framebuffer DescriptorTableLib_DX12::createFramebuffer(const string& key,
	const Descriptor* pDsv, const Framebuffer* pFramebuffer)
{
	Framebuffer framebuffer = {};
	if (pDsv) framebuffer.DepthStencilView = *pDsv;

	if (!key.empty())
	{
		const auto& index = key[0];
		framebuffer.NumRenderTargetDescriptors = static_cast<uint32_t>(key.size() / sizeof(Descriptor));
		const auto pSrc = reinterpret_cast<const D3D12_CPU_DESCRIPTOR_HANDLE*>(&key[1]);

		// Compute start addresses for CPU and GPU handles
		const auto& descriptorHeap = m_descriptorHeaps[RTV_HEAP][index];
		const auto& descriptorStride = m_descriptorStrides[RTV_HEAP];
		CD3DX12_CPU_DESCRIPTOR_HANDLE dst(descriptorHeap->GetCPUDescriptorHandleForHeapStart());

		if (pFramebuffer && pFramebuffer->RenderTargetViews)
		{
			dst = { *pFramebuffer->RenderTargetViews };
			framebuffer = *pFramebuffer;
		}
		else
		{
			auto& descriptorCount = m_descriptorCounts[RTV_HEAP][index];
			dst.Offset(descriptorCount, descriptorStride);
			descriptorCount += framebuffer.NumRenderTargetDescriptors;
			framebuffer.RenderTargetViews = make_shared<Descriptor>();
			*framebuffer.RenderTargetViews = dst.ptr;
		}

		// Create a descriptor table
		for (auto i = 0u; i < framebuffer.NumRenderTargetDescriptors; ++i)
		{
			// Copy a descriptor
			const auto& src = pSrc[i];
			if (src.ptr) m_device->CopyDescriptorsSimple(1, dst, src, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			dst.Offset(descriptorStride);
		}
	}

	return framebuffer;
}

Framebuffer DescriptorTableLib_DX12::getFramebuffer(const string& key,
	const Descriptor* pDsv, const Framebuffer* pFramebuffer)
{
	Framebuffer framebuffer = {};
	if (pDsv) framebuffer.DepthStencilView = *pDsv;

	if (!key.empty())
	{
		const auto& index = key[0];
		checkDescriptorHeapTypeStorage(RTV_HEAP, index);
		auto& rtvTables = m_rtvTables[index];
		const auto tableIter = rtvTables.find(key);

		// Create one, if it does not exist
		if (tableIter == rtvTables.end() && reallocateRtvHeap(key))
		{
			if (pFramebuffer && pFramebuffer->RenderTargetViews)
			{
				for (auto mIt = rtvTables.cbegin(); mIt != rtvTables.cend(); ++mIt)
				{
					if (mIt->second && *mIt->second == *pFramebuffer->RenderTargetViews)
					{
						rtvTables.erase(mIt);
						break;
					}
				}
			}

			framebuffer = createFramebuffer(key, pDsv, pFramebuffer);
			rtvTables[key] = framebuffer.RenderTargetViews;
		}
		else
		{
			framebuffer.NumRenderTargetDescriptors = static_cast<uint32_t>(key.size() / sizeof(Descriptor));
			framebuffer.RenderTargetViews = tableIter->second;
		}
	}

	return framebuffer;
}

uint32_t DescriptorTableLib_DX12::calculateGrowth(uint32_t newSize, DescriptorHeapType type, uint8_t index) const
{
	const auto& oldCapacity = m_descriptorCounts[type][index];
	const auto halfOldCapacity = oldCapacity >> 1;

	const auto maxSize = type == SAMPLER_HEAP ?
		D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE :
		D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_1;

	// geometric growth would overflow
	if (oldCapacity > maxSize - halfOldCapacity) return newSize;

	const auto geometric = oldCapacity + halfOldCapacity;

	return (max)(geometric, newSize); // geometric growth might be insufficient
}
