//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSG_DX12.h"
#include "XUSGPipelineLayout_DX12.h"
#include "XUSGEnum_DX12.h"

using namespace std;
using namespace XUSG;

Util::PipelineLayout_DX12::PipelineLayout_DX12() :
	m_descriptorTableLayoutKeys(0),
	m_staticSamplers(0),
	m_isTableLayoutsCompleted(false)
{
	m_pipelineLayoutKey.resize(1);
}

Util::PipelineLayout_DX12::~PipelineLayout_DX12()
{
}

void Util::PipelineLayout_DX12::SetShaderStage(uint32_t index, Shader::Stage stage)
{
	checkKeyStorage(index)[0] = stage;
}

void Util::PipelineLayout_DX12::SetRange(uint32_t index, DescriptorType type, uint32_t num, uint32_t baseBinding,
	uint32_t space, DescriptorFlag flags)
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
	range.OffsetInDescriptors = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
}

void Util::PipelineLayout_DX12::SetConstants(uint32_t index, uint32_t num32BitValues,
	uint32_t binding, uint32_t space, Shader::Stage stage)
{
	SetRange(index, DescriptorType::CONSTANT, num32BitValues, binding, space, DescriptorFlag::NONE);
	SetShaderStage(index, stage);
}

void Util::PipelineLayout_DX12::SetRootSRV(uint32_t index, uint32_t binding, uint32_t space,
	DescriptorFlag flags, Shader::Stage stage)
{
	SetRange(index, DescriptorType::ROOT_SRV, 1, binding, space, flags);
	SetShaderStage(index, stage);
}

void Util::PipelineLayout_DX12::SetRootUAV(uint32_t index, uint32_t binding, uint32_t space,
	DescriptorFlag flags, Shader::Stage stage)
{
	SetRange(index, DescriptorType::ROOT_UAV, 1, binding, space, flags);
	SetShaderStage(index, stage);
}

void Util::PipelineLayout_DX12::SetRootCBV(uint32_t index, uint32_t binding, uint32_t space, Shader::Stage stage)
{
	SetRange(index, DescriptorType::ROOT_CBV, 1, binding, space, DescriptorFlag::DATA_STATIC);
	SetShaderStage(index, stage);
}

void Util::PipelineLayout_DX12::SetStaticSamplers(const Sampler* pSamplers,
	uint32_t num, uint32_t baseBinding, uint32_t space, Shader::Stage stage)
{
	const auto startIdx = static_cast<uint32_t>(m_staticSamplers.size());
	m_staticSamplers.resize(startIdx + num);

	for (auto i = 0u; i < num; ++i)
	{
		auto& staticSampler = m_staticSamplers[startIdx + i];
		staticSampler.Binding = baseBinding + i;
		staticSampler.Space = space;
		staticSampler.pSampler = pSamplers[i].get();
		staticSampler.Stage = stage;
	}
}

PipelineLayout Util::PipelineLayout_DX12::CreatePipelineLayout(PipelineLayoutCache* pPipelineLayoutCache,
	PipelineLayoutFlag flags, const wchar_t* name)
{
	return pPipelineLayoutCache->CreatePipelineLayout(this, flags, name);
}

PipelineLayout Util::PipelineLayout_DX12::GetPipelineLayout(PipelineLayoutCache* pPipelineLayoutCache,
	PipelineLayoutFlag flags, const wchar_t* name)
{
	return pPipelineLayoutCache->GetPipelineLayout(this, flags, name);
}

DescriptorTableLayout Util::PipelineLayout_DX12::CreateDescriptorTableLayout(uint32_t index,
	PipelineLayoutCache* pPipelineLayoutCache) const
{
	return pPipelineLayoutCache->CreateDescriptorTableLayout(index, this);
}

DescriptorTableLayout Util::PipelineLayout_DX12::GetDescriptorTableLayout(uint32_t index,
	PipelineLayoutCache* pPipelineLayoutCache) const
{
	return pPipelineLayoutCache->GetDescriptorTableLayout(index, this);
}

const vector<string>& Util::PipelineLayout_DX12::GetDescriptorTableLayoutKeys() const
{
	return m_descriptorTableLayoutKeys;
}

string& Util::PipelineLayout_DX12::GetPipelineLayoutKey(PipelineLayoutCache* pPipelineLayoutCache)
{
	if (!m_isTableLayoutsCompleted && pPipelineLayoutCache)
	{
		const auto staticSamplerKeyOffset = DescriptorTableLayoutOffset + sizeof(DescriptorTableLayout) * m_descriptorTableLayoutKeys.size();
		m_pipelineLayoutKey.resize(staticSamplerKeyOffset + sizeof(StaticSampler) * m_staticSamplers.size());

		auto& descriptorTableLayoutCount = reinterpret_cast<uint16_t&>(m_pipelineLayoutKey[DescriptorTableLayoutCountOffset]);
		const auto pDescriptorTableLayouts = reinterpret_cast<DescriptorTableLayout*>(&m_pipelineLayoutKey[DescriptorTableLayoutOffset]);
		const auto pStaticSamplers = reinterpret_cast<StaticSampler*>(&m_pipelineLayoutKey[staticSamplerKeyOffset]);

		descriptorTableLayoutCount = static_cast<uint16_t>(m_descriptorTableLayoutKeys.size());
		for (auto i = 0u; i < descriptorTableLayoutCount; ++i)
			pDescriptorTableLayouts[i] = GetDescriptorTableLayout(i, pPipelineLayoutCache);

		for (auto i = 0u; i < m_staticSamplers.size(); ++i)
			pStaticSamplers[i] = m_staticSamplers[i];

		m_isTableLayoutsCompleted = true;
	}

	return m_pipelineLayoutKey;
}

std::string& Util::PipelineLayout_DX12::checkKeyStorage(uint32_t index)
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

PipelineLayoutCache_DX12::PipelineLayoutCache_DX12() :
	m_device(nullptr),
	m_rootSignatures(0)
{
}

PipelineLayoutCache_DX12::PipelineLayoutCache_DX12(const Device* pDevice) :
	PipelineLayoutCache()
{
	SetDevice(pDevice);
}

PipelineLayoutCache_DX12::~PipelineLayoutCache_DX12()
{
}

void PipelineLayoutCache_DX12::SetDevice(const Device* pDevice)
{
	m_device = static_cast<ID3D12Device*>(pDevice->GetHandle());
	assert(m_device);
}

void PipelineLayoutCache_DX12::SetPipelineLayout(const string& key, const PipelineLayout& pipelineLayout)
{
	m_rootSignatures[key] = static_cast<ID3D12RootSignature*>(pipelineLayout);
}

void PipelineLayoutCache_DX12::GetRootParameter(CD3DX12_ROOT_PARAMETER1& rootParam, vector<CD3DX12_DESCRIPTOR_RANGE1>& descriptorRanges,
	const DescriptorTableLayout& descriptorTableLayout) const
{
	const D3D12_DESCRIPTOR_RANGE_TYPE rangeTypes[] =
	{
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
		D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER
	};
	assert(rangeTypes[static_cast<uint8_t>(DescriptorType::SRV)] == D3D12_DESCRIPTOR_RANGE_TYPE_SRV);
	assert(rangeTypes[static_cast<uint8_t>(DescriptorType::UAV)] == D3D12_DESCRIPTOR_RANGE_TYPE_UAV);
	assert(rangeTypes[static_cast<uint8_t>(DescriptorType::CBV)] == D3D12_DESCRIPTOR_RANGE_TYPE_CBV);
	assert(rangeTypes[static_cast<uint8_t>(DescriptorType::SAMPLER)] == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER);

	// Set ranges
	const auto& key = *static_cast<const string*>(descriptorTableLayout);
	const auto numRanges = key.size() > 0 ? static_cast<uint32_t>((key.size() - 1) / sizeof(DescriptorRange)) : 0;

	if (numRanges > 0)
	{
		const auto stage = static_cast<Shader::Stage>(key[0]);
		const auto pRanges = reinterpret_cast<const DescriptorRange*>(&key[1]);

		switch (pRanges->Type)
		{
		case DescriptorType::CONSTANT:
			// Set param
			rootParam.InitAsConstants(pRanges->NumDescriptors, pRanges->BaseBinding,
				pRanges->Space, getShaderVisibility(stage));
			break;

		case DescriptorType::ROOT_SRV:
			// Set param
			rootParam.InitAsShaderResourceView(pRanges->BaseBinding, pRanges->Space,
				GetDX12RootDescriptorFlags(pRanges->Flags), getShaderVisibility(stage));
			break;

		case DescriptorType::ROOT_UAV:
			// Set param
			rootParam.InitAsUnorderedAccessView(pRanges->BaseBinding, pRanges->Space,
				GetDX12RootDescriptorFlags(pRanges->Flags), getShaderVisibility(stage));
			break;

		case DescriptorType::ROOT_CBV:
			// Set param
			rootParam.InitAsConstantBufferView(pRanges->BaseBinding, pRanges->Space,
				GetDX12RootDescriptorFlags(pRanges->Flags), getShaderVisibility(stage));
			break;

		default:
			descriptorRanges.resize(numRanges);
			for (auto i = 0u; i < numRanges; ++i)
			{
				const auto& range = pRanges[i];
				auto& descriptorRange = descriptorRanges[i];
				descriptorRange.Init(rangeTypes[static_cast<uint8_t>(range.Type)], range.NumDescriptors,
					range.BaseBinding, range.Space, GetDX12DescriptorRangeFlags(range.Flags));
			}

			// Set param
			rootParam.InitAsDescriptorTable(numRanges, descriptorRanges.data(), getShaderVisibility(stage));
		}
	}
}

void PipelineLayoutCache_DX12::GetStaticSampler(CD3DX12_STATIC_SAMPLER_DESC& samplerDescs, const StaticSampler& staticSampler) const
{
	const auto borderColor = staticSampler.pSampler->BorderColor[3] ?
		(staticSampler.pSampler->BorderColor[0] ? D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE : D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK) :
		D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;

	samplerDescs.Init(staticSampler.Binding,
		static_cast<D3D12_FILTER>(staticSampler.pSampler->Filter),
		static_cast<D3D12_TEXTURE_ADDRESS_MODE>(staticSampler.pSampler->AddressU),
		static_cast<D3D12_TEXTURE_ADDRESS_MODE>(staticSampler.pSampler->AddressV),
		static_cast<D3D12_TEXTURE_ADDRESS_MODE>(staticSampler.pSampler->AddressW),
		staticSampler.pSampler->MipLODBias,
		staticSampler.pSampler->MaxAnisotropy,
		static_cast<D3D12_COMPARISON_FUNC>(staticSampler.pSampler->ComparisonFunc),
		borderColor,
		staticSampler.pSampler->MinLOD,
		staticSampler.pSampler->MaxLOD,
		getShaderVisibility(static_cast<Shader::Stage>(staticSampler.Stage)),
		staticSampler.Space);
}

PipelineLayout PipelineLayoutCache_DX12::CreatePipelineLayout(Util::PipelineLayout* pUtil,
	PipelineLayoutFlag flags, const wchar_t* name)
{
	auto& pipelineLayoutKey = pUtil->GetPipelineLayoutKey(this);
	reinterpret_cast<uint16_t&>(pipelineLayoutKey[0]) = static_cast<uint16_t>(flags);

	return createPipelineLayout(pipelineLayoutKey, name);
}

PipelineLayout PipelineLayoutCache_DX12::GetPipelineLayout(Util::PipelineLayout* pUtil,
	PipelineLayoutFlag flags, const wchar_t* name, bool create)
{
	auto& pipelineLayoutKey = pUtil->GetPipelineLayoutKey(this);
	reinterpret_cast<uint16_t&>(pipelineLayoutKey[0]) = static_cast<uint16_t>(flags);

	return getPipelineLayout(pipelineLayoutKey, name, create);
}

DescriptorTableLayout PipelineLayoutCache_DX12::CreateDescriptorTableLayout(uint32_t index, const Util::PipelineLayout* pUtil)
{
	const auto& keys = pUtil->GetDescriptorTableLayoutKeys();

	return keys.size() > index ? createDescriptorTableLayout(keys[index]) : nullptr;
}

DescriptorTableLayout PipelineLayoutCache_DX12::GetDescriptorTableLayout(uint32_t index, const Util::PipelineLayout* pUtil)
{
	const auto& keys = pUtil->GetDescriptorTableLayoutKeys();

	return keys.size() > index ? getDescriptorTableLayout(keys[index]) : nullptr;
}

D3D_ROOT_SIGNATURE_VERSION PipelineLayoutCache_DX12::GetRootSignatureHighestVersion() const
{
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

	if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
		return D3D_ROOT_SIGNATURE_VERSION_1_0;

	// This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
	return D3D_ROOT_SIGNATURE_VERSION_1_1;
}

PipelineLayout PipelineLayoutCache_DX12::createPipelineLayout(const string& key, const wchar_t* name)
{
	const auto highestVersion = GetRootSignatureHighestVersion();

	const auto flags = static_cast<PipelineLayoutFlag>(reinterpret_cast<const uint16_t&>(key[0]));
	const auto numRootParams = reinterpret_cast<const uint16_t&>(key[Util::PipelineLayout_DX12::DescriptorTableLayoutCountOffset]);
	const auto pDescriptorTableLayouts = reinterpret_cast<const DescriptorTableLayout*>(&key[Util::PipelineLayout_DX12::DescriptorTableLayoutOffset]);

	vector<CD3DX12_ROOT_PARAMETER1> rootParams(numRootParams);
	vector<vector<CD3DX12_DESCRIPTOR_RANGE1>> descriptorRanges(numRootParams);
	for (auto i = 0u; i < numRootParams; ++i)
		GetRootParameter(rootParams[i], descriptorRanges[i], pDescriptorTableLayouts[i]);

	const auto staticSamplerKeyOffset = Util::PipelineLayout_DX12::DescriptorTableLayoutOffset + sizeof(DescriptorTableLayout) * numRootParams;
	const auto numSamplers = static_cast<uint32_t>((key.size() - staticSamplerKeyOffset) / sizeof(StaticSampler));
	const auto pStaticSamplers = reinterpret_cast<const StaticSampler*>(&key[staticSamplerKeyOffset]);

	vector<CD3DX12_STATIC_SAMPLER_DESC> samplerDescs(numSamplers);
	for (auto i = 0u; i < numSamplers; ++i)
		GetStaticSampler(samplerDescs[i], pStaticSamplers[i]);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init_1_1(numRootParams, numRootParams ? rootParams.data() : nullptr,
		numSamplers, numSamplers ? samplerDescs.data() : nullptr, GetDX12RootSignatureFlags(flags));

	com_ptr<ID3DBlob> signature, error;
	H_RETURN(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, highestVersion, &signature, &error),
		cerr, reinterpret_cast<wchar_t*>(error->GetBufferPointer()), nullptr);

	com_ptr<ID3D12RootSignature> rootSignature;
	V_RETURN(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature)), cerr, nullptr);
	if (name) rootSignature->SetName(name);
	m_rootSignatures[key] = rootSignature;

	return rootSignature.get();
}

PipelineLayout PipelineLayoutCache_DX12::getPipelineLayout(const string& key, const wchar_t* name, bool create)
{
	const auto layoutIter = m_rootSignatures.find(key);

	// Create one, if it does not exist
	if (layoutIter == m_rootSignatures.end())
	{
		if (create) return createPipelineLayout(key, name);
		else return nullptr;
	}

	return layoutIter->second.get();
}

DescriptorTableLayout PipelineLayoutCache_DX12::createDescriptorTableLayout(const string& key)
{
	const auto layout = &key;
	m_descriptorTableLayouts[key] = nullptr;
	const auto layoutPtrIter = m_descriptorTableLayouts.find(key);
	layoutPtrIter->second = &layoutPtrIter->first;

	return layoutPtrIter->second;
};

DescriptorTableLayout PipelineLayoutCache_DX12::getDescriptorTableLayout(const string& key)
{
	const auto layoutPtrIter = m_descriptorTableLayouts.find(key);

	// Create one, if it does not exist
	if (layoutPtrIter == m_descriptorTableLayouts.end()) return createDescriptorTableLayout(key);

	return layoutPtrIter->second;
}

D3D12_SHADER_VISIBILITY PipelineLayoutCache_DX12::getShaderVisibility(Shader::Stage stage) const
{
	const D3D12_SHADER_VISIBILITY visibilities[] =
	{
		D3D12_SHADER_VISIBILITY_PIXEL,
		D3D12_SHADER_VISIBILITY_VERTEX,
		D3D12_SHADER_VISIBILITY_DOMAIN,
		D3D12_SHADER_VISIBILITY_HULL,
		D3D12_SHADER_VISIBILITY_GEOMETRY,
		D3D12_SHADER_VISIBILITY_ALL,
		D3D12_SHADER_VISIBILITY_MESH,
		D3D12_SHADER_VISIBILITY_AMPLIFICATION
	};
	assert(visibilities[Shader::VS] == D3D12_SHADER_VISIBILITY_VERTEX);
	assert(visibilities[Shader::PS] == D3D12_SHADER_VISIBILITY_PIXEL);
	assert(visibilities[Shader::DS] == D3D12_SHADER_VISIBILITY_DOMAIN);
	assert(visibilities[Shader::HS] == D3D12_SHADER_VISIBILITY_HULL);
	assert(visibilities[Shader::GS] == D3D12_SHADER_VISIBILITY_GEOMETRY);
	assert(visibilities[Shader::ALL] == D3D12_SHADER_VISIBILITY_ALL);
	assert(visibilities[Shader::AS] == D3D12_SHADER_VISIBILITY_AMPLIFICATION);
	assert(visibilities[Shader::MS] == D3D12_SHADER_VISIBILITY_MESH);

	return visibilities[stage];
}
