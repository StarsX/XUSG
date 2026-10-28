//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSG_VK.h"
#include "XUSGRayTracingState_VK.h"

using namespace std;
using namespace XUSG;
using namespace XUSG::RayTracing;

namespace
{
	string Narrow(const wchar_t* name)
	{
		if (!name) return {};

		string utf8;
		while (*name)
		{
			const auto code = static_cast<uint32_t>(*name++);
			if (code < 0x80)
				utf8.push_back(static_cast<char>(code));
			else if (code < 0x800)
			{
				utf8.push_back(static_cast<char>(0xC0 | (code >> 6)));
				utf8.push_back(static_cast<char>(0x80 | (code & 0x3F)));
			}
			else
			{
				utf8.push_back(static_cast<char>(0xE0 | (code >> 12)));
				utf8.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
				utf8.push_back(static_cast<char>(0x80 | (code & 0x3F)));
			}
		}

		return utf8;
	}

	wstring Widen(const string& name)
	{
		wstring wide;
		wide.reserve(name.size());
		for (const auto c : name) wide.push_back(static_cast<wchar_t>(static_cast<unsigned char>(c)));

		return wide;
	}

	string Lower(string name)
	{
		for (auto& c : name)
			if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');

		return name;
	}

	bool HasToken(const string& name, const char* token)
	{
		return name.find(token) != string::npos;
	}

	VkShaderStageFlagBits StageFromName(const string& name)
	{
		const auto lower = Lower(name);
		if (HasToken(lower, "closesthit") || HasToken(lower, "closest_hit")) return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		if (HasToken(lower, "anyhit") || HasToken(lower, "any_hit")) return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
		if (HasToken(lower, "intersection")) return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
		if (HasToken(lower, "callable")) return VK_SHADER_STAGE_CALLABLE_BIT_KHR;
		if (HasToken(lower, "miss")) return VK_SHADER_STAGE_MISS_BIT_KHR;

		return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
	}

	VkShaderStageFlagBits StageFromExecutionModel(uint32_t model)
	{
		switch (model)
		{
		case 5314: return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
		case 5315: return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
		case 5316: return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		case 5317: return VK_SHADER_STAGE_MISS_BIT_KHR;
		case 5318: return VK_SHADER_STAGE_CALLABLE_BIT_KHR;
		default: return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
		}
	}

	void ReadSpirvEntries(const void* pData, size_t size, unordered_map<string, VkShaderStageFlagBits>& entries)
	{
		if (!pData || size < 20) return;

		const auto code = static_cast<const uint32_t*>(pData);
		if (code[0] != 0x07230203) return;

		const auto words = static_cast<uint32_t>(size / sizeof(uint32_t));
		auto offset = 5u;
		while (offset < words)
		{
			const auto word = code[offset];
			const auto opcode = word & 0xffffu;
			const auto count = word >> 16;
			if (!count || offset + count > words) break;
			if (opcode == 15 && count >= 4)
			{
				string name;
				for (auto w = offset + 3; w < offset + count; ++w)
				{
					const auto bytes = code[w];
					auto done = false;
					for (auto b = 0u; b < 4u; ++b)
					{
						const auto c = static_cast<char>((bytes >> (b * 8)) & 0xff);
						if (!c)
						{
							done = true;
							break;
						}
						name.push_back(c);
					}
					if (done) break;
				}
				if (!name.empty()) entries[name] = StageFromExecutionModel(code[offset + 1]);
			}
			offset += count;
		}
	}

	bool IsGeneralStage(VkShaderStageFlagBits stage)
	{
		return stage == VK_SHADER_STAGE_RAYGEN_BIT_KHR || stage == VK_SHADER_STAGE_MISS_BIT_KHR ||
			stage == VK_SHADER_STAGE_CALLABLE_BIT_KHR;
	}

	// Local root arguments follow the shader group handle in the shader record.
	// The pipeline layout stores those bytes as push offsets, so the record offset adds the handle size.
	VkDescriptorSetAndBindingMappingEXT ShaderRecordMapping(const VkDescriptorSetAndBindingMappingEXT& mapping,
		uint32_t handleSize)
	{
		auto record = mapping;
		switch (mapping.source)
		{
		case VK_DESCRIPTOR_MAPPING_SOURCE_PUSH_DATA_EXT:
		{
			const auto offset = mapping.sourceData.pushDataOffset + handleSize;
			record.source = VK_DESCRIPTOR_MAPPING_SOURCE_SHADER_RECORD_DATA_EXT;
			record.sourceData.shaderRecordDataOffset = offset;
			break;
		}
		case VK_DESCRIPTOR_MAPPING_SOURCE_PUSH_ADDRESS_EXT:
		{
			const auto offset = mapping.sourceData.pushAddressOffset + handleSize;
			record.source = VK_DESCRIPTOR_MAPPING_SOURCE_SHADER_RECORD_ADDRESS_EXT;
			record.sourceData.shaderRecordAddressOffset = offset;
			break;
		}
		case VK_DESCRIPTOR_MAPPING_SOURCE_HEAP_WITH_PUSH_INDEX_EXT:
		{
			const auto push = mapping.sourceData.pushIndex;
			record.source = VK_DESCRIPTOR_MAPPING_SOURCE_HEAP_WITH_SHADER_RECORD_INDEX_EXT;
			record.sourceData = {};
			record.sourceData.shaderRecordIndex.heapOffset = push.heapOffset;
			record.sourceData.shaderRecordIndex.shaderRecordOffset = push.pushOffset + handleSize;
			record.sourceData.shaderRecordIndex.heapIndexStride = push.heapIndexStride;
			record.sourceData.shaderRecordIndex.heapArrayStride = push.heapArrayStride;
			record.sourceData.shaderRecordIndex.pEmbeddedSampler = push.pEmbeddedSampler;
			record.sourceData.shaderRecordIndex.useCombinedImageSamplerIndex = push.useCombinedImageSamplerIndex;
			record.sourceData.shaderRecordIndex.samplerHeapOffset = push.samplerHeapOffset;
			record.sourceData.shaderRecordIndex.samplerShaderRecordOffset = push.useCombinedImageSamplerIndex ?
				push.samplerPushOffset + handleSize : push.samplerPushOffset;
			record.sourceData.shaderRecordIndex.samplerHeapIndexStride = push.samplerHeapIndexStride;
			record.sourceData.shaderRecordIndex.samplerHeapArrayStride = push.samplerHeapArrayStride;
			break;
		}
		case VK_DESCRIPTOR_MAPPING_SOURCE_HEAP_WITH_CONSTANT_OFFSET_EXT:
			break;
		default:
			assert(!"Local pipeline layout mapping cannot be placed in a shader record");
			break;
		}

		return record;
	}

	struct ShaderModules
	{
		VkDevice Device;
		vector<VkShaderModule> Modules;

		~ShaderModules()
		{
			if (Device == VK_NULL_HANDLE) return;
			for (const auto module : Modules)
				if (module != VK_NULL_HANDLE) vkDestroyShaderModule(Device, module, nullptr);
		}
	};
}

State_VK::State_VK() :
	m_keyShaderLibs(0),
	m_keyHitGroups(0),
	m_keyLocalPipelineLayouts(0),
	m_isSerialized(false)
{
	m_key.resize(sizeof(KeyHeader));
	m_pKeyHeader = reinterpret_cast<KeyHeader*>(&m_key[0]);
	memset(m_pKeyHeader, 0, sizeof(KeyHeader));
}

State_VK::~State_VK()
{
}

void State_VK::SetShaderLibrary(uint32_t index, const Blob& shaderLib, uint32_t numShaders, const wchar_t** pShaderNames)
{
	m_isSerialized = false;

	if (index >= m_keyShaderLibs.size())
		m_keyShaderLibs.resize(index + 1);

	auto& keyShaderLib = m_keyShaderLibs[index];
	keyShaderLib.Lib = shaderLib;
	keyShaderLib.Shaders.resize(numShaders);
	memcpy(keyShaderLib.Shaders.data(), pShaderNames, sizeof(wchar_t*) * numShaders);
}

void State_VK::SetHitGroup(uint32_t index, const wchar_t* hitGroupName, const wchar_t* closestHitShaderName,
	const wchar_t* anyHitShaderName, const wchar_t* intersectionShaderName, HitGroupType type)
{
	m_isSerialized = false;

	if (index >= m_keyHitGroups.size())
		m_keyHitGroups.resize(index + 1);

	auto& keyHitGroup = m_keyHitGroups[index];
	keyHitGroup.HitGroup = hitGroupName;
	keyHitGroup.ClosestHitShader = closestHitShaderName;
	keyHitGroup.AnyHitShader = anyHitShaderName;
	keyHitGroup.IntersectionShader = intersectionShaderName;
	keyHitGroup.Type = static_cast<uint8_t>(type);
}

void State_VK::SetShaderConfig(uint32_t maxPayloadSize, uint32_t maxAttributeSize)
{
	m_isSerialized = false;
	m_pKeyHeader->MaxPayloadSize = maxPayloadSize;
	m_pKeyHeader->MaxAttributeSize = maxAttributeSize;
}

void State_VK::SetLocalPipelineLayout(uint32_t index, const XUSG::PipelineLayout& layout, uint32_t numShaders, const wchar_t** pShaderNames)
{
	m_isSerialized = false;

	if (index >= m_keyLocalPipelineLayouts.size())
		m_keyLocalPipelineLayouts.resize(index + 1);

	auto& keyLocalPipelineLayout = m_keyLocalPipelineLayouts[index];
	keyLocalPipelineLayout.Layout = layout;
	keyLocalPipelineLayout.Shaders.resize(numShaders);
	memcpy(keyLocalPipelineLayout.Shaders.data(), pShaderNames, sizeof(wchar_t*) * numShaders);
}

void State_VK::SetGlobalPipelineLayout(const XUSG::PipelineLayout& layout)
{
	m_isSerialized = false;
	m_pKeyHeader->GlobalLayout = layout;
}

void State_VK::SetMaxRecursionDepth(uint32_t depth)
{
	m_isSerialized = false;
	m_pKeyHeader->MaxRecursionDepth = depth;
}

void State_VK::SetNodeMask(uint32_t nodeMask)
{
	m_isSerialized = false;
	m_pKeyHeader->NodeMask = nodeMask;
}

Pipeline State_VK::CreatePipeline(PipelineLib* pPipelineCache, const wchar_t* name)
{
	return pPipelineCache->CreatePipeline(this, name);
}

Pipeline State_VK::GetPipeline(PipelineLib* pPipelineCache, const wchar_t* name)
{
	return pPipelineCache->GetPipeline(this, name);
}

const string& State_VK::GetKey()
{
	if (!m_isSerialized) serialize();

	return m_key;
}

const wchar_t* State_VK::GetHitGroupName(uint32_t index)
{
	const auto pHitGroups = reinterpret_cast<const KeyHitGroup*>(&GetKey()[sizeof(KeyHeader)]);

	return static_cast<const wchar_t*>(pHitGroups[index].HitGroup);
}

uint32_t State_VK::GetNumHitGroups()
{
	GetKey();

	return m_pKeyHeader->NumHitGroups;
}

void* State_VK::GetBackend()
{
	return this;
}

void State_VK::serialize()
{
	m_pKeyHeader->NumShaderLibs = static_cast<uint32_t>(m_keyShaderLibs.size());
	m_pKeyHeader->NumHitGroups = static_cast<uint32_t>(m_keyHitGroups.size());
	m_pKeyHeader->NumLocalPipelineLayouts = static_cast<uint32_t>(m_keyLocalPipelineLayouts.size());

	const auto sizeKeyHeader = sizeof(KeyHeader);
	const auto sizeKeyHitGroups = sizeof(KeyHitGroup) * m_keyHitGroups.size();

	auto sizeKeyShaderLibs = sizeof(KeyShaderLibHeader) * m_keyShaderLibs.size();
	for (const auto& keyShaderLib : m_keyShaderLibs)
		sizeKeyShaderLibs += sizeof(wchar_t*) * keyShaderLib.Shaders.size();

	auto sizeKeyLocalPipelineLayouts = sizeof(KeyLocalPipelineLayoutHeader) * m_keyLocalPipelineLayouts.size();
	for (const auto& keyLocalPipelineLayout : m_keyLocalPipelineLayouts)
		sizeKeyLocalPipelineLayouts += sizeof(wchar_t*) * keyLocalPipelineLayout.Shaders.size();

	m_key.resize(sizeKeyHeader + sizeKeyHitGroups + sizeKeyShaderLibs + sizeKeyLocalPipelineLayouts);
	m_pKeyHeader = reinterpret_cast<KeyHeader*>(&m_key[0]);

	if (m_pKeyHeader->NumHitGroups)
	{
		const auto pKeyHitGroups = reinterpret_cast<KeyHitGroup*>(&m_key[sizeKeyHeader]);
		memcpy(pKeyHitGroups, m_keyHitGroups.data(), sizeKeyHitGroups);
	}

	auto pKeyShaderLibHeader = reinterpret_cast<KeyShaderLibHeader*>(&m_key[sizeKeyHeader + sizeKeyHitGroups]);
	for (const auto& keyShaderLib : m_keyShaderLibs)
	{
		pKeyShaderLibHeader->Lib = keyShaderLib.Lib;
		pKeyShaderLibHeader->NumShaders = static_cast<uint32_t>(keyShaderLib.Shaders.size());

		const auto pKeyShaders = reinterpret_cast<wchar_t**>(&pKeyShaderLibHeader[1]);
		memcpy(pKeyShaders, keyShaderLib.Shaders.data(), sizeof(wchar_t*) * pKeyShaderLibHeader->NumShaders);

		pKeyShaderLibHeader = reinterpret_cast<KeyShaderLibHeader*>(&pKeyShaders[pKeyShaderLibHeader->NumShaders]);
	}

	auto pKeyLocalPipelineLayoutHeader = reinterpret_cast<KeyLocalPipelineLayoutHeader*>
		(&m_key[sizeKeyHeader + sizeKeyHitGroups + sizeKeyShaderLibs]);
	for (const auto& keyLocalPipelineLayout : m_keyLocalPipelineLayouts)
	{
		pKeyLocalPipelineLayoutHeader->Layout = keyLocalPipelineLayout.Layout;
		pKeyLocalPipelineLayoutHeader->NumShaders = static_cast<uint32_t>(keyLocalPipelineLayout.Shaders.size());

		const auto pKeyLocalPipelineAssociation = reinterpret_cast<wchar_t**>(&pKeyLocalPipelineLayoutHeader[1]);
		memcpy(pKeyLocalPipelineAssociation, keyLocalPipelineLayout.Shaders.data(),
			sizeof(wchar_t*) * pKeyLocalPipelineLayoutHeader->NumShaders);

		pKeyLocalPipelineLayoutHeader = reinterpret_cast<KeyLocalPipelineLayoutHeader*>
			(&pKeyLocalPipelineAssociation[pKeyLocalPipelineLayoutHeader->NumShaders]);
	}

	m_isSerialized = true;
}

//--------------------------------------------------------------------------------------

PipelineLib_VK::PipelineLib_VK() :
	m_pDevice(nullptr)
{
}

PipelineLib_VK::PipelineLib_VK(RayTracing::Device* pDevice) :
	PipelineLib()
{
	SetDevice(pDevice);
}

PipelineLib_VK::~PipelineLib_VK()
{
	for (auto& pipeline : m_pipelines) releasePipeline(pipeline.second);
}

void PipelineLib_VK::SetDevice(RayTracing::Device* pDevice)
{
	m_pDevice = AsVK(pDevice);
	assert(m_pDevice);
}

void PipelineLib_VK::SetPipeline(State* pState, const Pipeline& pipeline)
{
	const auto p = AsVK(pState);
	assert(p);

	lock_guard<mutex> lock(m_mtx);
	const auto& key = p->GetKey();
	auto next = static_cast<RayTracingPipeline_VK*>(pipeline);
	const auto existing = m_pipelines.find(key);
	if (existing != m_pipelines.end())
	{
		if (existing->second == next) return;
		releasePipeline(existing->second);
	}
	if (next) ++next->Refs;
	m_pipelines[key] = next;
}

Pipeline PipelineLib_VK::CreatePipeline(State* pState, const wchar_t* name)
{
	const auto p = AsVK(pState);
	assert(p);

	lock_guard<mutex> lock(m_mtx);

	return createPipeline(p->GetKey(), name);
}

Pipeline PipelineLib_VK::GetPipeline(State* pState, const wchar_t* name)
{
	const auto p = AsVK(pState);
	assert(p);

	lock_guard<mutex> lock(m_mtx);

	return getPipeline(p->GetKey(), name);
}

void PipelineLib_VK::releasePipeline(RayTracingPipeline_VK* pPipeline)
{
	if (!pPipeline) return;
	if (--pPipeline->Refs) return;

	const auto device = m_pDevice ? m_pDevice->GetDevice() : VK_NULL_HANDLE;
	if (pPipeline->Handle != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
		vkDestroyPipeline(device, pPipeline->Handle, nullptr);
	delete pPipeline;
}

RayTracingPipeline_VK* PipelineLib_VK::createPipeline(const string& key, const wchar_t* name)
{
	assert(m_pDevice);
	const auto device = m_pDevice->GetDevice();
	const auto& ext = m_pDevice->GetExtProcs();
	assert(ext.CreateRayTracingPipelines);
	assert(ext.GetRayTracingShaderGroupHandles);
	if (!ext.CreateRayTracingPipelines || !ext.GetRayTracingShaderGroupHandles) return nullptr;

	const auto& keyHeader = reinterpret_cast<const State_VK::KeyHeader&>(key[0]);
	const auto sizeKeyHeader = sizeof(State_VK::KeyHeader);
	const auto sizeKeyHitGroups = sizeof(State_VK::KeyHitGroup) * keyHeader.NumHitGroups;
	const auto pKeyHitGroups = reinterpret_cast<const State_VK::KeyHitGroup*>(&key[sizeKeyHeader]);

	unordered_map<wstring, VkShaderStageFlagBits> forced;
	for (auto i = 0u; i < keyHeader.NumHitGroups; ++i)
	{
		const auto& hitGroup = pKeyHitGroups[i];
		if (hitGroup.ClosestHitShader) forced[hitGroup.ClosestHitShader] = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		if (hitGroup.AnyHitShader) forced[hitGroup.AnyHitShader] = VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
		if (hitGroup.IntersectionShader) forced[hitGroup.IntersectionShader] = VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
	}

	struct Export
	{
		uint32_t Library;
		string Entry;
		wstring Name;
		VkShaderStageFlagBits Stage;
	};

	vector<Export> exports;
	vector<unordered_map<string, VkShaderStageFlagBits>> libraryEntries(keyHeader.NumShaderLibs);
	ShaderModules modules;
	modules.Device = device;
	modules.Modules.assign(keyHeader.NumShaderLibs, VK_NULL_HANDLE);

	auto pKeyShaderLibHeader = reinterpret_cast<const State_VK::KeyShaderLibHeader*>(&key[sizeKeyHeader + sizeKeyHitGroups]);
	for (auto i = 0u; i < keyHeader.NumShaderLibs; ++i)
	{
		const void* pData = nullptr;
		size_t size = 0;
		const auto pBlob = static_cast<const Blob_VK*>(pKeyShaderLibHeader->Lib);
		if (pBlob && !pBlob->Data.empty())
		{
			pData = pBlob->Data.data();
			size = pBlob->Data.size();
		}
		ReadSpirvEntries(pData, size, libraryEntries[i]);

		if (pData && size)
		{
			vector<uint32_t> stripped;
			const uint32_t* pCode = nullptr;
			size_t codeSize = 0;
			DropUnsupportedStorageInputOutput16(m_pDevice, pData, size, stripped, pCode, codeSize);
			VkShaderModuleCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			info.codeSize = codeSize;
			info.pCode = pCode;
			if (vkCreateShaderModule(device, &info, nullptr, &modules.Modules[i]) < 0)
				modules.Modules[i] = VK_NULL_HANDLE;
		}

		const auto pShaderNames = reinterpret_cast<const wchar_t* const*>(&pKeyShaderLibHeader[1]);
		if (pKeyShaderLibHeader->NumShaders)
		{
			for (auto s = 0u; s < pKeyShaderLibHeader->NumShaders; ++s)
			{
				if (!pShaderNames[s]) continue;
				Export shader;
				shader.Library = i;
				shader.Name = pShaderNames[s];
				shader.Entry = Narrow(pShaderNames[s]);
				const auto forcedStage = forced.find(shader.Name);
				const auto spirvStage = libraryEntries[i].find(shader.Entry);
				if (forcedStage != forced.end()) shader.Stage = forcedStage->second;
				else if (spirvStage != libraryEntries[i].end()) shader.Stage = spirvStage->second;
				else shader.Stage = StageFromName(shader.Entry);
				exports.push_back(shader);
			}
		}
		else
		{
			for (const auto& entry : libraryEntries[i])
			{
				Export shader;
				shader.Library = i;
				shader.Entry = entry.first;
				shader.Name = Widen(entry.first);
				const auto forcedStage = forced.find(shader.Name);
				shader.Stage = forcedStage != forced.end() ? forcedStage->second : entry.second;
				exports.push_back(shader);
			}
		}

		pKeyShaderLibHeader = reinterpret_cast<const State_VK::KeyShaderLibHeader*>(&pShaderNames[pKeyShaderLibHeader->NumShaders]);
	}

	const auto handleSize = m_pDevice->GetShaderGroupHandleSize() ? m_pDevice->GetShaderGroupHandleSize() : 32u;
	const auto maxStride = m_pDevice->GetRayTracingPipelineProperties().maxShaderGroupStride ?
		m_pDevice->GetRayTracingPipelineProperties().maxShaderGroupStride : 4096u;
	unordered_map<wstring, const PipelineLayoutObject*> localLayouts;
	auto pKeyLocalPipelineLayoutHeader = reinterpret_cast<const State_VK::KeyLocalPipelineLayoutHeader*>(pKeyShaderLibHeader);
	for (auto i = 0u; i < keyHeader.NumLocalPipelineLayouts; ++i)
	{
		const auto pLocalLayout = static_cast<const PipelineLayoutObject*>(pKeyLocalPipelineLayoutHeader->Layout);
		const auto pShaderNames = reinterpret_cast<const wchar_t* const*>(&pKeyLocalPipelineLayoutHeader[1]);
		if (pLocalLayout)
		{
			assert(handleSize + pLocalLayout->PushDataSize <= maxStride);
			for (auto s = 0u; s < pKeyLocalPipelineLayoutHeader->NumShaders; ++s)
			{
				if (!pShaderNames[s]) continue;

				const wstring shaderName = pShaderNames[s];
				auto named = false;
				for (const auto& shader : exports)
					if (shader.Name == shaderName) named = true;
				assert(named);
				assert(localLayouts.find(shaderName) == localLayouts.end());
				localLayouts[shaderName] = pLocalLayout;
			}
		}

		pKeyLocalPipelineLayoutHeader = reinterpret_cast<const State_VK::KeyLocalPipelineLayoutHeader*>
			(&pShaderNames[pKeyLocalPipelineLayoutHeader->NumShaders]);
	}

	const auto pLayout = static_cast<const PipelineLayoutObject*>(keyHeader.GlobalLayout);
	const vector<VkDescriptorSetAndBindingMappingEXT>* pGlobalMappings = pLayout ?
		&pLayout->StageMappings[Shader::CS] : nullptr;

	vector<vector<VkDescriptorSetAndBindingMappingEXT>> recordMappings(exports.size());
	vector<VkShaderDescriptorSetAndBindingMappingInfoEXT> recordInfos(exports.size());
	for (auto i = 0u; i < exports.size(); ++i)
	{
		auto& mappings = recordMappings[i];
		if (pGlobalMappings)
			mappings.insert(mappings.end(), pGlobalMappings->begin(), pGlobalMappings->end());

		const auto local = localLayouts.find(exports[i].Name);
		if (local != localLayouts.end())
		{
			for (const auto& mapping : local->second->StageMappings[Shader::CS])
				mappings.push_back(ShaderRecordMapping(mapping, handleSize));
		}

		recordInfos[i] = {};
		recordInfos[i].sType = VK_STRUCTURE_TYPE_SHADER_DESCRIPTOR_SET_AND_BINDING_MAPPING_INFO_EXT;
		recordInfos[i].mappingCount = static_cast<uint32_t>(mappings.size());
		recordInfos[i].pMappings = mappings.empty() ? nullptr : mappings.data();
	}

	vector<VkPipelineShaderStageCreateInfo> stages(exports.size());
	for (auto i = 0u; i < exports.size(); ++i)
	{
		stages[i] = {};
		stages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stages[i].pNext = recordInfos[i].mappingCount ? &recordInfos[i] : nullptr;
		stages[i].stage = exports[i].Stage;
		stages[i].module = modules.Modules[exports[i].Library];
		stages[i].pName = exports[i].Entry.c_str();
	}

	unordered_map<wstring, uint32_t> stageIndex;
	for (auto i = 0u; i < exports.size(); ++i) stageIndex[exports[i].Name] = i;

	vector<VkRayTracingShaderGroupCreateInfoKHR> groups;
	unordered_map<wstring, uint32_t> groupIndex;
	for (const auto& shader : exports)
	{
		if (!IsGeneralStage(shader.Stage)) continue;

		VkRayTracingShaderGroupCreateInfoKHR group = {};
		group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		group.generalShader = stageIndex[shader.Name];
		group.closestHitShader = VK_SHADER_UNUSED_KHR;
		group.anyHitShader = VK_SHADER_UNUSED_KHR;
		group.intersectionShader = VK_SHADER_UNUSED_KHR;
		groupIndex[shader.Name] = static_cast<uint32_t>(groups.size());
		groups.push_back(group);
	}

	auto shaderIndex = [&](const wchar_t* shaderName)
	{
		if (!shaderName) return VK_SHADER_UNUSED_KHR;
		const auto found = stageIndex.find(shaderName);
		assert(found != stageIndex.end());

		return found != stageIndex.end() ? found->second : VK_SHADER_UNUSED_KHR;
	};

	for (auto i = 0u; i < keyHeader.NumHitGroups; ++i)
	{
		const auto& hitGroup = pKeyHitGroups[i];
		VkRayTracingShaderGroupCreateInfoKHR group = {};
		group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		group.type = hitGroup.Type == static_cast<uint8_t>(HitGroupType::PROCEDURAL_PRIMITIVE) ?
			VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR :
			VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
		group.generalShader = VK_SHADER_UNUSED_KHR;
		group.closestHitShader = shaderIndex(static_cast<const wchar_t*>(hitGroup.ClosestHitShader));
		group.anyHitShader = shaderIndex(static_cast<const wchar_t*>(hitGroup.AnyHitShader));
		group.intersectionShader = shaderIndex(static_cast<const wchar_t*>(hitGroup.IntersectionShader));
		if (hitGroup.HitGroup) groupIndex[hitGroup.HitGroup] = static_cast<uint32_t>(groups.size());
		groups.push_back(group);
	}

	auto depth = keyHeader.MaxRecursionDepth ? keyHeader.MaxRecursionDepth : 1u;
	const auto maxDepth = m_pDevice->GetMaxRayRecursionDepth();
	if (maxDepth && depth > maxDepth) depth = maxDepth;

	VkPipelineCreateFlags2CreateInfo flags = {};
	flags.sType = VK_STRUCTURE_TYPE_PIPELINE_CREATE_FLAGS_2_CREATE_INFO;
	flags.flags = VK_PIPELINE_CREATE_2_DESCRIPTOR_HEAP_BIT_EXT;

	VkRayTracingPipelineCreateInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
	info.pNext = &flags;
	info.stageCount = static_cast<uint32_t>(stages.size());
	info.pStages = stages.empty() ? nullptr : stages.data();
	info.groupCount = static_cast<uint32_t>(groups.size());
	info.pGroups = groups.empty() ? nullptr : groups.data();
	info.maxPipelineRayRecursionDepth = depth;
	info.layout = VK_NULL_HANDLE;

	VkPipeline handle = VK_NULL_HANDLE;
	const auto result = ext.CreateRayTracingPipelines(device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &info, nullptr, &handle);
	if (result < 0)
	{
		cerr << "VkResult " << result << endl;

		return nullptr;
	}

	vector<uint8_t> handles(static_cast<size_t>(groups.size()) * handleSize);
	if (!groups.empty())
	{
		const auto handlesResult = ext.GetRayTracingShaderGroupHandles(device, handle, 0,
			static_cast<uint32_t>(groups.size()), handles.size(), handles.data());
		if (handlesResult < 0)
		{
			cerr << "VkResult " << handlesResult << endl;
			vkDestroyPipeline(device, handle, nullptr);

			return nullptr;
		}
	}

	auto pipeline = new RayTracingPipeline_VK();
	pipeline->Handle = handle;
	pipeline->BindPoint = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
	pipeline->Refs = 1;
	pipeline->XfbBuffers = 0;
	pipeline->DynamicVertexStride = false;
	pipeline->HandleSize = handleSize;
	pipeline->Handles = move(handles);
	pipeline->Groups = move(groupIndex);
	m_pDevice->SetDebugName(VK_OBJECT_TYPE_PIPELINE, reinterpret_cast<uint64_t>(handle), name);

	const auto existing = m_pipelines.find(key);
	if (existing != m_pipelines.end()) releasePipeline(existing->second);
	m_pipelines[key] = pipeline;

	return pipeline;
}

RayTracingPipeline_VK* PipelineLib_VK::getPipeline(const string& key, const wchar_t* name)
{
	const auto pipeline = m_pipelines.find(key);
	if (pipeline == m_pipelines.cend()) return createPipeline(key, name);

	return pipeline->second;
}

State_VK* XUSG::RayTracing::AsVK(State* p)
{
	assert(!p || p->GetBackend());
	return static_cast<State_VK*>(p ? p->GetBackend() : nullptr);
}
