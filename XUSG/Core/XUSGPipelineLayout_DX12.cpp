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
		m_pipelineLayoutKey.resize(sizeof(DescriptorTableLayout) * m_descriptorTableLayoutKeys.size() + 1);

		const auto pDescriptorTableLayouts = reinterpret_cast<DescriptorTableLayout*>(&m_pipelineLayoutKey[1]);

		for (auto i = 0u; i < m_descriptorTableLayoutKeys.size(); ++i)
			pDescriptorTableLayouts[i] = GetDescriptorTableLayout(i, pPipelineLayoutCache);

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
	D3D12_DESCRIPTOR_RANGE_TYPE rangeTypes[static_cast<uint8_t>(DescriptorType::NUM)];
	rangeTypes[static_cast<uint8_t>(DescriptorType::SRV)] = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	rangeTypes[static_cast<uint8_t>(DescriptorType::UAV)] = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	rangeTypes[static_cast<uint8_t>(DescriptorType::CBV)] = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	rangeTypes[static_cast<uint8_t>(DescriptorType::SAMPLER)] = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;

	// Set ranges
	const auto& key = *static_cast<const string*>(descriptorTableLayout);
	const auto numRanges = key.size() > 0 ? static_cast<uint32_t>((key.size() - 1) / sizeof(DescriptorRange)) : 0;

	if (numRanges > 0)
	{
		const auto stage = static_cast<Shader::Stage>(key[0]);
		D3D12_SHADER_VISIBILITY visibilities[Shader::NUM_STAGE];
		visibilities[Shader::VS] = D3D12_SHADER_VISIBILITY_VERTEX;
		visibilities[Shader::PS] = D3D12_SHADER_VISIBILITY_PIXEL;
		visibilities[Shader::DS] = D3D12_SHADER_VISIBILITY_DOMAIN;
		visibilities[Shader::HS] = D3D12_SHADER_VISIBILITY_HULL;
		visibilities[Shader::GS] = D3D12_SHADER_VISIBILITY_GEOMETRY;
		visibilities[Shader::ALL] = D3D12_SHADER_VISIBILITY_ALL;
		visibilities[Shader::AS] = D3D12_SHADER_VISIBILITY_AMPLIFICATION;
		visibilities[Shader::MS] = D3D12_SHADER_VISIBILITY_MESH;

		const auto pRanges = reinterpret_cast<const DescriptorRange*>(&key[1]);
		switch (pRanges->Type)
		{
		case DescriptorType::CONSTANT:
			// Set param
			rootParam.InitAsConstants(pRanges->NumDescriptors, pRanges->BaseBinding,
				pRanges->Space, visibilities[stage]);
			break;

		case DescriptorType::ROOT_SRV:
			// Set param
			rootParam.InitAsShaderResourceView(pRanges->BaseBinding, pRanges->Space,
				GetDX12RootDescriptorFlags(pRanges->Flags), visibilities[stage]);
			break;

		case DescriptorType::ROOT_UAV:
			// Set param
			rootParam.InitAsUnorderedAccessView(pRanges->BaseBinding, pRanges->Space,
				GetDX12RootDescriptorFlags(pRanges->Flags), visibilities[stage]);
			break;

		case DescriptorType::ROOT_CBV:
			// Set param
			rootParam.InitAsConstantBufferView(pRanges->BaseBinding, pRanges->Space,
				GetDX12RootDescriptorFlags(pRanges->Flags), visibilities[stage]);
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
			rootParam.InitAsDescriptorTable(numRanges, descriptorRanges.data(), visibilities[stage]);
		}
	}
}

PipelineLayout PipelineLayoutCache_DX12::CreatePipelineLayout(Util::PipelineLayout* pUtil,
	PipelineLayoutFlag flags, const wchar_t* name)
{
	auto& pipelineLayoutKey = pUtil->GetPipelineLayoutKey(this);
	pipelineLayoutKey[0] = static_cast<uint8_t>(flags);

	return createPipelineLayout(pipelineLayoutKey, name);
}

PipelineLayout PipelineLayoutCache_DX12::GetPipelineLayout(Util::PipelineLayout* pUtil,
	PipelineLayoutFlag flags, const wchar_t* name, bool create)
{
	auto& pipelineLayoutKey = pUtil->GetPipelineLayoutKey(this);
	pipelineLayoutKey[0] = static_cast<uint8_t>(flags);

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

PipelineLayout PipelineLayoutCache_DX12::createPipelineLayout(const string& key, const wchar_t* name)
{
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

	// This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

	if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;

	const auto numRootParams = static_cast<uint32_t>((key.size() - 1) / sizeof(void*));
	const auto flags = static_cast<PipelineLayoutFlag>(key[0]);
	const auto pDescriptorTableLayouts = reinterpret_cast<const DescriptorTableLayout*>(&key[1]);

	vector<CD3DX12_ROOT_PARAMETER1> rootParams(numRootParams);
	vector<vector<CD3DX12_DESCRIPTOR_RANGE1>> descriptorRanges(numRootParams);
	for (auto i = 0u; i < numRootParams; ++i)
		GetRootParameter(rootParams[i], descriptorRanges[i], pDescriptorTableLayouts[i]);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init_1_1(numRootParams, rootParams.data(), 0, nullptr, GetDX12RootSignatureFlags(flags));

	com_ptr<ID3DBlob> signature, error;
	H_RETURN(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error),
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
		if (create)
		{
			const auto layout = createPipelineLayout(key, name);

			return layout;
		}
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
