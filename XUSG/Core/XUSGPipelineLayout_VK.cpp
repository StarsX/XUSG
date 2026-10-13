//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGPipelineLayout_VK.h"
#include "XUSGEnum_VK.h"

using namespace std;
using namespace XUSG;

namespace
{
	constexpr uint32_t g_dxbcContainerMaxSize = 0x80000000u;
	constexpr uint16_t g_dxbcContainerVersionMajor = 1;
	constexpr uint32_t g_rdatVersion10 = 0x10;

#define DXBC_FOURCC(ch0, ch1, ch2, ch3) ( \
	(uint32_t)(uint8_t)(ch0) | ((uint32_t)(uint8_t)(ch1) << 8) | \
	((uint32_t)(uint8_t)(ch2) << 16) | ((uint32_t)(uint8_t)(ch3) << 24))

	enum DxbcFourCC : uint32_t
	{
		DXBC_Container = DXBC_FOURCC('D', 'X', 'B', 'C'),
		DXBC_RootSignature = DXBC_FOURCC('R', 'T', 'S', '0'),
		DXBC_RuntimeData = DXBC_FOURCC('R', 'D', 'A', 'T')
	};

	enum class RootSignatureVersion : uint32_t
	{
		V1_0 = 0x1,
		V1_1 = 0x2,
		V1_2 = 0x3
	};

	enum class RootParameterType : uint32_t
	{
		DESCRIPTOR_TABLE = 0,
		CONSTANTS_32BIT = 1,
		CBV = 2,
		SRV = 3,
		UAV = 4
	};

	enum class RangeType : uint32_t
	{
		SRV = 0,
		UAV = 1,
		CBV = 2,
		SAMPLER = 3
	};

	enum class ShaderVisibility : uint32_t
	{
		VIS_ALL = 0,
		VIS_VERTEX = 1,
		VIS_HULL = 2,
		VIS_DOMAIN = 3,
		VIS_GEOMETRY = 4,
		VIS_PIXEL = 5,
		VIS_AMPLIFICATION = 6,
		VIS_MESH = 7
	};

	enum class StaticBorderColor : uint32_t
	{
		TRANSPARENT_BLACK = 0,
		OPAQUE_BLACK = 1,
		OPAQUE_WHITE = 2,
		OPAQUE_BLACK_UINT = 3,
		OPAQUE_WHITE_UINT = 4
	};

	enum class RdatPartType : uint32_t
	{
		StringBuffer = 1,
		RawBytes = 5,
		SubobjectTable = 6
	};

	enum class SubobjectKind : uint32_t
	{
		GlobalRootSignature = 1,
		LocalRootSignature = 2
	};

	struct DxbcContainerVersion
	{
		uint16_t Major;
		uint16_t Minor;
	};

	struct DxbcContainerHeader
	{
		uint32_t HeaderFourCC;
		uint8_t Hash[16];
		DxbcContainerVersion Version;
		uint32_t ContainerSizeInBytes;
		uint32_t PartCount;
	};

	struct DxbcPartHeader
	{
		uint32_t PartFourCC;
		uint32_t PartSize;
	};

	struct RdatHeader
	{
		uint32_t Version;
		uint32_t PartCount;
	};

	struct RdatPartHeader
	{
		uint32_t Type;
		uint32_t Size;
	};

	struct RdatTableHeader
	{
		uint32_t RecordCount;
		uint32_t RecordStride;
	};

	struct RdatBytesRef
	{
		uint32_t Offset;
		uint32_t Size;
	};

	struct RdatSubobjectInfo
	{
		uint32_t Kind;
		uint32_t Name;
		RdatBytesRef RootSignature;
	};

	VkSpirvResourceTypeFlagsEXT ResourceMask(DescriptorType type)
	{
		switch (type)
		{
		case DescriptorType::SRV:
			// dxc marks StructuredBuffer members NonWritable, not the variable, so the
			// descriptor-heap rules classify those buffers as read-write storage.
			return VK_SPIRV_RESOURCE_TYPE_SAMPLED_IMAGE_BIT_EXT | VK_SPIRV_RESOURCE_TYPE_READ_ONLY_IMAGE_BIT_EXT |
				VK_SPIRV_RESOURCE_TYPE_READ_ONLY_STORAGE_BUFFER_BIT_EXT | VK_SPIRV_RESOURCE_TYPE_READ_WRITE_STORAGE_BUFFER_BIT_EXT |
				VK_SPIRV_RESOURCE_TYPE_UNIFORM_BUFFER_BIT_EXT;
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

	bool RequireSpace(size_t offset, size_t count, size_t stride, size_t dataSize)
	{
		return !count || (dataSize - offset) / count >= stride;
	}

	const DxbcContainerHeader* AsDxbcContainer(const void* pData, size_t size)
	{
		XUSG_C_RETURN(!pData || size < sizeof(DxbcContainerHeader), nullptr);
		const auto pHeader = static_cast<const DxbcContainerHeader*>(pData);
		XUSG_C_RETURN(pHeader->HeaderFourCC != DXBC_Container, nullptr);
		XUSG_C_RETURN(pHeader->Version.Major != g_dxbcContainerVersionMajor, nullptr);
		XUSG_C_RETURN(pHeader->ContainerSizeInBytes > size, nullptr);
		XUSG_C_RETURN(pHeader->ContainerSizeInBytes > g_dxbcContainerMaxSize, nullptr);

		const auto partOffsetBytes = sizeof(uint32_t) * pHeader->PartCount;
		XUSG_C_RETURN(sizeof(DxbcContainerHeader) + partOffsetBytes > pHeader->ContainerSizeInBytes, nullptr);

		const auto pBytes = static_cast<const uint8_t*>(pData);
		const auto pPartOffsets = reinterpret_cast<const uint32_t*>(pHeader + 1);
		for (auto i = 0u; i < pHeader->PartCount; ++i)
		{
			XUSG_C_RETURN(pPartOffsets[i] + sizeof(DxbcPartHeader) > pHeader->ContainerSizeInBytes, nullptr);
			const auto pPart = reinterpret_cast<const DxbcPartHeader*>(pBytes + pPartOffsets[i]);
			XUSG_C_RETURN(pPartOffsets[i] + sizeof(DxbcPartHeader) + pPart->PartSize > pHeader->ContainerSizeInBytes, nullptr);
		}

		return pHeader;
	}

	bool FindDxbcPart(const void* pData, size_t size, uint32_t fourCC, const void** ppPart, size_t* pPartSize)
	{
		const auto pHeader = AsDxbcContainer(pData, size);
		XUSG_N_RETURN(pHeader, false);

		const auto pBytes = static_cast<const uint8_t*>(pData);
		const auto pPartOffsets = reinterpret_cast<const uint32_t*>(pHeader + 1);
		for (auto i = 0u; i < pHeader->PartCount; ++i)
		{
			const auto pPart = reinterpret_cast<const DxbcPartHeader*>(pBytes + pPartOffsets[i]);
			if (pPart->PartFourCC == fourCC)
			{
				if (ppPart) *ppPart = pPart + 1;
				if (pPartSize) *pPartSize = pPart->PartSize;
				return true;
			}
		}

		return false;
	}

	Shader::Stage GetShaderStage(uint32_t visibility)
	{
		switch (static_cast<ShaderVisibility>(visibility))
		{
		case ShaderVisibility::VIS_VERTEX: return Shader::Stage::VS;
		case ShaderVisibility::VIS_HULL: return Shader::Stage::HS;
		case ShaderVisibility::VIS_DOMAIN: return Shader::Stage::DS;
		case ShaderVisibility::VIS_GEOMETRY: return Shader::Stage::GS;
		case ShaderVisibility::VIS_PIXEL: return Shader::Stage::PS;
		case ShaderVisibility::VIS_AMPLIFICATION: return Shader::Stage::AS;
		case ShaderVisibility::VIS_MESH: return Shader::Stage::MS;
		default: return Shader::Stage::ALL;
		}
	}

	DescriptorType GetDescriptorType(uint32_t rangeType)
	{
		switch (static_cast<RangeType>(rangeType))
		{
		case RangeType::UAV: return DescriptorType::UAV;
		case RangeType::CBV: return DescriptorType::CBV;
		case RangeType::SAMPLER: return DescriptorType::SAMPLER;
		default: return DescriptorType::SRV;
		}
	}

	DescriptorType GetRootDescriptorType(uint32_t parameterType)
	{
		switch (static_cast<RootParameterType>(parameterType))
		{
		case RootParameterType::SRV: return DescriptorType::ROOT_SRV;
		case RootParameterType::UAV: return DescriptorType::ROOT_UAV;
		default: return DescriptorType::ROOT_CBV;
		}
	}

	void FillStaticSampler(Sampler& sampler, const uint32_t* pWords, bool hasFlags)
	{
		sampler = {};
		// pWords[0] is RTS0 bit-encoded filter, not sequential SamplerFilter.
		sampler.Filter = GetSamplerFilterFromRootSig(pWords[0]);
		sampler.AddressU = static_cast<TextureAddressMode>(pWords[1] - 1u);
		sampler.AddressV = static_cast<TextureAddressMode>(pWords[2] - 1u);
		sampler.AddressW = static_cast<TextureAddressMode>(pWords[3] - 1u);
		memcpy(&sampler.MipLODBias, &pWords[4], sizeof(float));
		sampler.MaxAnisotropy = static_cast<uint8_t>(pWords[5]);
		sampler.Comparison = static_cast<ComparisonFunc>(pWords[6] - 1u);

		switch (static_cast<StaticBorderColor>(pWords[7]))
		{
		case StaticBorderColor::OPAQUE_WHITE:
			sampler.BorderColor[0] = sampler.BorderColor[1] = sampler.BorderColor[2] = sampler.BorderColor[3] = 1.0f;
			break;
		case StaticBorderColor::OPAQUE_BLACK:
			sampler.BorderColor[0] = sampler.BorderColor[1] = sampler.BorderColor[2] = 0.0f;
			sampler.BorderColor[3] = 1.0f;
			break;
		case StaticBorderColor::OPAQUE_BLACK_UINT:
			sampler.Flags |= SamplerFlag::UINT_BORDER_COLOR;
			sampler.BorderColorU[0] = sampler.BorderColorU[1] = sampler.BorderColorU[2] = 0u;
			sampler.BorderColorU[3] = 1u;
			break;
		case StaticBorderColor::OPAQUE_WHITE_UINT:
			sampler.Flags |= SamplerFlag::UINT_BORDER_COLOR;
			sampler.BorderColorU[0] = sampler.BorderColorU[1] = sampler.BorderColorU[2] = sampler.BorderColorU[3] = 1u;
			break;
		default:
			sampler.BorderColor[0] = sampler.BorderColor[1] = sampler.BorderColor[2] = sampler.BorderColor[3] = 0.0f;
			break;
		}

		memcpy(&sampler.MinLOD, &pWords[8], sizeof(float));
		memcpy(&sampler.MaxLOD, &pWords[9], sizeof(float));
		// D3D12_SAMPLER_FLAG_UINT_BORDER_COLOR = 0x1, NON_NORMALIZED = 0x2 (matches SamplerFlag).
		if (hasFlags)
			sampler.Flags |= static_cast<SamplerFlag>(pWords[13] & 0x3u);
	}

}

bool XUSG::FindVKRootSignaturePayload(const void* pData, size_t size, const wchar_t* name,
	const void** ppPayload, size_t* pPayloadSize)
{
	// Serialized root signatures live in RTS0. Named lib subobjects are in RDAT.
	if (!name && FindDxbcPart(pData, size, DXBC_RootSignature, ppPayload, pPayloadSize))
		return true;

	const void* pRdat = nullptr;
	size_t rdatSize = 0;
	XUSG_N_RETURN(FindDxbcPart(pData, size, DXBC_RuntimeData, &pRdat, &rdatSize), false);
	XUSG_N_RETURN(rdatSize >= sizeof(RdatHeader), false);

	const auto pHeader = static_cast<const RdatHeader*>(pRdat);
	XUSG_N_RETURN(pHeader->Version == g_rdatVersion10, false);
	XUSG_N_RETURN(RequireSpace(sizeof(RdatHeader), pHeader->PartCount, sizeof(uint32_t), rdatSize), false);

	const auto pBytes = static_cast<const uint8_t*>(pRdat);
	const auto pPartOffsets = reinterpret_cast<const uint32_t*>(pHeader + 1);

	const char* pStrings = nullptr;
	size_t stringSize = 0;
	const uint8_t* pRawBytes = nullptr;
	size_t rawSize = 0;
	const uint8_t* pSubobjectTable = nullptr;
	uint32_t recordCount = 0;
	uint32_t recordStride = 0;

	for (auto i = 0u; i < pHeader->PartCount; ++i)
	{
		const auto partOffset = pPartOffsets[i];
		XUSG_N_RETURN(partOffset + sizeof(RdatPartHeader) <= rdatSize, false);
		const auto pPart = reinterpret_cast<const RdatPartHeader*>(pBytes + partOffset);
		XUSG_N_RETURN(partOffset + sizeof(RdatPartHeader) + pPart->Size <= rdatSize, false);
		const auto pPartData = reinterpret_cast<const uint8_t*>(pPart + 1);

		switch (static_cast<RdatPartType>(pPart->Type))
		{
		case RdatPartType::StringBuffer:
			pStrings = reinterpret_cast<const char*>(pPartData);
			stringSize = pPart->Size;
			break;
		case RdatPartType::RawBytes:
			pRawBytes = pPartData;
			rawSize = pPart->Size;
			break;
		case RdatPartType::SubobjectTable:
			XUSG_N_RETURN(pPart->Size >= sizeof(RdatTableHeader), false);
			{
				const auto pTable = reinterpret_cast<const RdatTableHeader*>(pPartData);
				recordCount = pTable->RecordCount;
				recordStride = pTable->RecordStride;
				XUSG_N_RETURN(recordStride >= sizeof(RdatSubobjectInfo), false);
				XUSG_N_RETURN(RequireSpace(sizeof(RdatTableHeader), recordCount, recordStride, pPart->Size), false);
				pSubobjectTable = pPartData + sizeof(RdatTableHeader);
			}
			break;
		default:
			break;
		}
	}

	XUSG_N_RETURN(pStrings && pRawBytes && pSubobjectTable, false);

	const auto utf8Name = name ? WideToUtf8(name) : string();
	const auto hasName = name != nullptr;
	auto foundIndex = -1;

	for (auto i = 0u; i < recordCount; ++i)
	{
		const auto pInfo = reinterpret_cast<const RdatSubobjectInfo*>(pSubobjectTable + i * recordStride);
		if (pInfo->Kind != static_cast<uint32_t>(SubobjectKind::GlobalRootSignature) &&
			pInfo->Kind != static_cast<uint32_t>(SubobjectKind::LocalRootSignature))
			continue;

		XUSG_N_RETURN(pInfo->Name < stringSize, false);
		const auto pSubobjectName = pStrings + pInfo->Name;
		XUSG_N_RETURN(strnlen(pSubobjectName, stringSize - pInfo->Name) < stringSize - pInfo->Name, false);

		if (hasName)
		{
			if (utf8Name != pSubobjectName) continue;
		}
		else
		{
			// Match vkd3d: CreateRootSignature falls back to a unique global root signature.
			if (pInfo->Kind != static_cast<uint32_t>(SubobjectKind::GlobalRootSignature)) continue;
			XUSG_C_RETURN(foundIndex >= 0, false);
		}

		foundIndex = static_cast<int>(i);
		if (hasName) break;
	}

	XUSG_N_RETURN(foundIndex >= 0, false);
	const auto pInfo = reinterpret_cast<const RdatSubobjectInfo*>(pSubobjectTable +
		static_cast<size_t>(foundIndex) * recordStride);
	XUSG_N_RETURN(pInfo->RootSignature.Offset < rawSize, false);
	XUSG_N_RETURN(pInfo->RootSignature.Size <= rawSize - pInfo->RootSignature.Offset, false);

	if (ppPayload) *ppPayload = pRawBytes + pInfo->RootSignature.Offset;
	if (pPayloadSize) *pPayloadSize = pInfo->RootSignature.Size;

	return true;
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

void* Util::PipelineLayout_VK::GetBackend()
{
	return this;
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
	m_pDevice(nullptr)
{
}

PipelineLayoutLib_VK::PipelineLayoutLib_VK(Device* pDevice) :
	PipelineLayoutLib_VK()
{
	SetDevice(pDevice);
}

PipelineLayoutLib_VK::~PipelineLayoutLib_VK()
{
}

void PipelineLayoutLib_VK::SetDevice(Device* pDevice)
{
	m_pDevice = AsVK(pDevice);
	assert(m_pDevice);
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
	string key(sizeof(blobSignature), 0);
	memcpy(&key[0], blobSignature, sizeof(blobSignature));

	lock_guard<mutex> lock(m_mtx);

	return createRootSignature(key, blobSignature, name, nodeMask);
}

PipelineLayout PipelineLayoutLib_VK::GetRootSignature(const Blob& blobSignature,
	const wchar_t* name, bool create, uint32_t nodeMask)
{
	string key(sizeof(blobSignature), 0);
	memcpy(&key[0], blobSignature, sizeof(blobSignature));

	lock_guard<mutex> lock(m_mtx);

	return getRootSignature(key, blobSignature, name, create, nodeMask);
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

PipelineLayout PipelineLayoutLib_VK::createRootSignature(const string& key,
	const Blob& blobSignature, const wchar_t* name, uint32_t nodeMask)
{
	const void* pData = nullptr;
	size_t size = 0;
	GetVKBlobData(blobSignature, &pData, &size);

	const void* pPayload = nullptr;
	size_t payloadSize = 0;
	XUSG_N_RETURN(FindVKRootSignaturePayload(pData, size, nullptr, &pPayload, &payloadSize), nullptr);

	return createRootSignatureFromPayload(key, pPayload, payloadSize, name, nodeMask);
}

PipelineLayout PipelineLayoutLib_VK::createRootSignatureFromPayload(const string& key,
	const void* pPayload, size_t payloadSize, const wchar_t* name, uint32_t nodeMask)
{
	(void)name;
	(void)nodeMask;
	XUSG_N_RETURN(pPayload && RequireSpace(0, 6, sizeof(uint32_t), payloadSize), nullptr);

	const auto pBytes = static_cast<const uint8_t*>(pPayload);
	auto ptr = reinterpret_cast<const uint32_t*>(pBytes);
	const auto version = static_cast<RootSignatureVersion>(ptr[0]);
	const auto numRootParams = ptr[1];
	const auto parametersOffset = ptr[2];
	const auto numSamplers = ptr[3];
	const auto staticSamplerOffset = ptr[4];
	XUSG_N_RETURN(version == RootSignatureVersion::V1_0 ||
		version == RootSignatureVersion::V1_1 ||
		version == RootSignatureVersion::V1_2, nullptr);

	const auto version1_0 = version == RootSignatureVersion::V1_0;
	const auto version1_2 = version == RootSignatureVersion::V1_2;
	const auto rangeStride = version1_0 ? 5u : 6u;
	const auto rootDescStride = version1_0 ? 2u : 3u;
	const auto samplerStrideWords = version1_2 ? 14u : 13u;

	XUSG_N_RETURN(RequireSpace(parametersOffset, numRootParams, 3 * sizeof(uint32_t), payloadSize), nullptr);

	auto object = make_unique<PipelineLayoutObject>();
	object->RootParameters.resize(numRootParams);

	const auto& heapProps = m_pDevice->GetDescriptorHeapProperties();
	const auto resourceStride = m_pDevice->GetResourceDescriptorStride();
	const auto samplerDescStride = m_pDevice->GetSamplerDescriptorStride();
	const auto resourceReserved = static_cast<uint32_t>(heapProps.minResourceHeapReservedRange);
	const auto samplerReserved = static_cast<uint32_t>(heapProps.minSamplerHeapReservedRangeWithEmbedded);
	auto pushOffset = 0u;
	vector<Sampler> staticSamplers(numSamplers);

	for (auto i = 0u; i < numRootParams; ++i)
	{
		const auto pHeader = reinterpret_cast<const uint32_t*>(pBytes + parametersOffset) + i * 3;
		const auto parameterType = pHeader[0];
		const auto stage = GetShaderStage(pHeader[1]);
		const auto parameterOffset = pHeader[2];
		auto& root = object->RootParameters[i];
		root = {};
		root.Stage = stage;

		switch (static_cast<RootParameterType>(parameterType))
		{
		case RootParameterType::CONSTANTS_32BIT:
		{
			XUSG_N_RETURN(RequireSpace(parameterOffset, 3, sizeof(uint32_t), payloadSize), nullptr);
			const auto pConstants = reinterpret_cast<const uint32_t*>(pBytes + parameterOffset);
			const auto binding = pConstants[0];
			const auto space = pConstants[1];
			const auto num32BitValues = pConstants[2];

			pushOffset = alignPush(pushOffset, sizeof(uint32_t));
			root.Type = DescriptorType::CONSTANT;
			root.PushByteOffset = pushOffset;
			root.SizeInBytes = num32BitValues * sizeof(uint32_t);
			VkDescriptorMappingSourceDataEXT data = {};
			data.pushDataOffset = pushOffset;
			appendMapping(*object, DescriptorType::CONSTANT, space, binding, 1,
				VK_DESCRIPTOR_MAPPING_SOURCE_PUSH_DATA_EXT, data, stage);
			pushOffset += root.SizeInBytes;
			break;
		}
		case RootParameterType::CBV:
		case RootParameterType::SRV:
		case RootParameterType::UAV:
		{
			XUSG_N_RETURN(RequireSpace(parameterOffset, rootDescStride, sizeof(uint32_t), payloadSize), nullptr);
			const auto pDescriptor = reinterpret_cast<const uint32_t*>(pBytes + parameterOffset);
			const auto binding = pDescriptor[0];
			const auto space = pDescriptor[1];
			root.Type = GetRootDescriptorType(parameterType);

			pushOffset = alignPush(pushOffset, sizeof(uint64_t));
			root.PushByteOffset = pushOffset;
			root.SizeInBytes = sizeof(uint64_t);
			VkDescriptorMappingSourceDataEXT data = {};
			data.pushAddressOffset = pushOffset;
			appendMapping(*object, root.Type, space, binding, 1,
				VK_DESCRIPTOR_MAPPING_SOURCE_PUSH_ADDRESS_EXT, data, stage);
			pushOffset += root.SizeInBytes;
			break;
		}
		case RootParameterType::DESCRIPTOR_TABLE:
		{
			XUSG_N_RETURN(RequireSpace(parameterOffset, 2, sizeof(uint32_t), payloadSize), nullptr);
			const auto pTable = reinterpret_cast<const uint32_t*>(pBytes + parameterOffset);
			const auto numRanges = pTable[0];
			const auto rangesOffset = pTable[1];
			XUSG_N_RETURN(RequireSpace(rangesOffset, numRanges, rangeStride * sizeof(uint32_t), payloadSize), nullptr);
			XUSG_N_RETURN(numRanges > 0, nullptr);

			const auto pRanges = reinterpret_cast<const uint32_t*>(pBytes + rangesOffset);
			root.Type = GetDescriptorType(pRanges[0]);

			pushOffset = alignPush(pushOffset, sizeof(uint32_t));
			root.PushByteOffset = pushOffset;
			root.SizeInBytes = sizeof(uint32_t);
			auto cursor = 0u;
			for (auto r = 0u; r < numRanges; ++r)
			{
				const auto pRange = pRanges + r * rangeStride;
				const auto type = GetDescriptorType(pRange[0]);
				const auto numDescriptors = pRange[1];
				const auto baseBinding = pRange[2];
				const auto space = pRange[3];
				const auto offsetInDescriptors = version1_0 ? pRange[4] : pRange[5];
				const auto sampler = type == DescriptorType::SAMPLER;
				const auto stride = sampler ? samplerDescStride : resourceStride;
				const auto reserved = sampler ? samplerReserved : resourceReserved;
				const auto offset = offsetInDescriptors == XUSG_DESCRIPTOR_OFFSET_APPEND ? cursor : offsetInDescriptors;
				cursor = offset + numDescriptors;

				VkDescriptorMappingSourceDataEXT data = {};
				data.pushIndex.heapOffset = reserved + offset * stride;
				data.pushIndex.pushOffset = pushOffset;
				data.pushIndex.heapIndexStride = stride;
				data.pushIndex.heapArrayStride = stride;
				appendMapping(*object, type, space, baseBinding, numDescriptors,
					VK_DESCRIPTOR_MAPPING_SOURCE_HEAP_WITH_PUSH_INDEX_EXT, data, stage);
			}
			pushOffset += root.SizeInBytes;
			break;
		}
		default:
			return nullptr;
		}
	}

	if (numSamplers)
	{
		XUSG_N_RETURN(RequireSpace(staticSamplerOffset, numSamplers,
			samplerStrideWords * sizeof(uint32_t), payloadSize), nullptr);
		object->EmbeddedSamplers.resize(numSamplers);
		for (auto i = 0u; i < numSamplers; ++i)
		{
			const auto pSamplerWords = reinterpret_cast<const uint32_t*>(pBytes + staticSamplerOffset) +
				i * samplerStrideWords;
			FillStaticSampler(staticSamplers[i], pSamplerWords, version1_2);
			fillEmbeddedSampler(object->EmbeddedSamplers[i], &staticSamplers[i]);

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
			appendMapping(*object, DescriptorType::SAMPLER, pSamplerWords[11], pSamplerWords[10], 1,
				VK_DESCRIPTOR_MAPPING_SOURCE_HEAP_WITH_CONSTANT_OFFSET_EXT, data,
				GetShaderStage(pSamplerWords[12]));
		}
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

PipelineLayout PipelineLayoutLib_VK::getRootSignature(const string& key, const Blob& blobSignature,
	const wchar_t* name, bool create, uint32_t nodeMask)
{
	const auto layoutIter = m_pipelineLayouts.find(key);

	// Create one, if it does not exist
	if (layoutIter == m_pipelineLayouts.cend())
	{
		if (create) return createRootSignature(key, blobSignature, name, nodeMask);
		else return nullptr;
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
	const auto mask = ResourceMask(type);
	const VkSpirvResourceTypeFlagsEXT bits[] =
	{
		VK_SPIRV_RESOURCE_TYPE_SAMPLER_BIT_EXT,
		VK_SPIRV_RESOURCE_TYPE_SAMPLED_IMAGE_BIT_EXT,
		VK_SPIRV_RESOURCE_TYPE_READ_ONLY_IMAGE_BIT_EXT,
		VK_SPIRV_RESOURCE_TYPE_READ_WRITE_IMAGE_BIT_EXT,
		VK_SPIRV_RESOURCE_TYPE_COMBINED_SAMPLED_IMAGE_BIT_EXT,
		VK_SPIRV_RESOURCE_TYPE_UNIFORM_BUFFER_BIT_EXT,
		VK_SPIRV_RESOURCE_TYPE_READ_ONLY_STORAGE_BUFFER_BIT_EXT,
		VK_SPIRV_RESOURCE_TYPE_READ_WRITE_STORAGE_BUFFER_BIT_EXT,
		VK_SPIRV_RESOURCE_TYPE_ACCELERATION_STRUCTURE_BIT_EXT
	};
	for (const auto bit : bits)
	{
		if ((mask & bit) == 0) continue;

		VkDescriptorSetAndBindingMappingEXT mapping = {};
		mapping.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_AND_BINDING_MAPPING_EXT;
		mapping.descriptorSet = set;
		mapping.firstBinding = binding;
		mapping.bindingCount = count;
		mapping.resourceMask = bit;
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
}

void PipelineLayoutLib_VK::fillEmbeddedSampler(EmbeddedSampler_VK& embedded, const Sampler* pSampler) const
{
	embedded = {};
	assert(pSampler);
	embedded.Reduction.sType = VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO;
	embedded.Reduction.reductionMode = GetVKReductionMode(pSampler->Filter);

	const auto uintBorder = (pSampler->Flags & SamplerFlag::UINT_BORDER_COLOR) != SamplerFlag::NONE;
	VkBorderColor borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
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
			embedded.Border.sType = VK_STRUCTURE_TYPE_SAMPLER_CUSTOM_BORDER_COLOR_CREATE_INFO_EXT;
			embedded.Border.customBorderColor.uint32[0] = r;
			embedded.Border.customBorderColor.uint32[1] = g;
			embedded.Border.customBorderColor.uint32[2] = b;
			embedded.Border.customBorderColor.uint32[3] = a;
			borderColor = VK_BORDER_COLOR_INT_CUSTOM_EXT;
		}
		else
		{
			borderColor = a == 0 ? VK_BORDER_COLOR_INT_TRANSPARENT_BLACK :
				(r || g || b) ? VK_BORDER_COLOR_INT_OPAQUE_WHITE : VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		}
	}
	else
	{
		const auto standard = (pSampler->BorderColor[3] == 0.0f && pSampler->BorderColor[0] == 0.0f &&
			pSampler->BorderColor[1] == 0.0f && pSampler->BorderColor[2] == 0.0f) ||
			(pSampler->BorderColor[3] == 1.0f && pSampler->BorderColor[0] == pSampler->BorderColor[1] &&
				pSampler->BorderColor[1] == pSampler->BorderColor[2] &&
				(pSampler->BorderColor[0] == 0.0f || pSampler->BorderColor[0] == 1.0f));
		if (!standard && m_pDevice->HasCustomBorderColor())
		{
			embedded.Border.sType = VK_STRUCTURE_TYPE_SAMPLER_CUSTOM_BORDER_COLOR_CREATE_INFO_EXT;
			embedded.Border.customBorderColor.float32[0] = pSampler->BorderColor[0];
			embedded.Border.customBorderColor.float32[1] = pSampler->BorderColor[1];
			embedded.Border.customBorderColor.float32[2] = pSampler->BorderColor[2];
			embedded.Border.customBorderColor.float32[3] = pSampler->BorderColor[3];
			borderColor = VK_BORDER_COLOR_FLOAT_CUSTOM_EXT;
		}
		else
		{
			const auto white = pSampler->BorderColor[0] > 0.5f || pSampler->BorderColor[1] > 0.5f ||
				pSampler->BorderColor[2] > 0.5f;
			borderColor = pSampler->BorderColor[3] <= 0.0f ? VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK :
				white ? VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE : VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
		}
	}

	embedded.Info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	embedded.Info.magFilter = GetVKFilter(pSampler->Filter, false, true);
	embedded.Info.minFilter = GetVKFilter(pSampler->Filter, true, false);
	embedded.Info.mipmapMode = GetVKMipmapMode(pSampler->Filter);
	embedded.Info.addressModeU = GetVKSamplerAddressMode(pSampler->AddressU);
	embedded.Info.addressModeV = GetVKSamplerAddressMode(pSampler->AddressV);
	embedded.Info.addressModeW = GetVKSamplerAddressMode(pSampler->AddressW);
	embedded.Info.mipLodBias = pSampler->MipLODBias;
	// Match vkd3d: aniso from filter type, then MaxAnisotropy < 1 disables; clamp to 16.
	embedded.Info.anisotropyEnable = IsAnisotropicSamplerFilter(pSampler->Filter) && pSampler->MaxAnisotropy >= 1;
	embedded.Info.maxAnisotropy = embedded.Info.anisotropyEnable ?
		(min)(16.0f, static_cast<float>(pSampler->MaxAnisotropy)) : 1.0f;
	const auto filterValue = static_cast<uint8_t>(pSampler->Filter);
	embedded.Info.compareEnable = filterValue >= static_cast<uint8_t>(SamplerFilter::COMPARISON_MIN_MAG_MIP_POINT) &&
		filterValue <= static_cast<uint8_t>(SamplerFilter::COMPARISON_ANISOTROPIC);
	embedded.Info.compareOp = GetVKCompareOp(pSampler->Comparison);
	embedded.Info.minLod = pSampler->MinLOD;
	embedded.Info.maxLod = pSampler->MaxLOD;
	embedded.Info.borderColor = borderColor;
	embedded.Info.unnormalizedCoordinates = (pSampler->Flags & SamplerFlag::NON_NORMALIZED_COORDINATES) != SamplerFlag::NONE;
}

uint32_t PipelineLayoutLib_VK::alignPush(uint32_t offset, uint32_t alignment) const
{
	return (offset + alignment - 1u) & ~(alignment - 1u);
}
