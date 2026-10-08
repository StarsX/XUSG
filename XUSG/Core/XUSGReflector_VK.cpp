//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGReflector_VK.h"

using namespace std;
using namespace XUSG;

namespace
{
	const uint32_t SpirvMagic = 0x07230203;
	const uint32_t OpName = 5;
	const uint32_t OpDecorate = 71;
	const uint32_t DecorationBinding = 33;
	const uint32_t DecorationDescriptorSet = 34;
}

Reflector_VK::Reflector_VK() :
	m_valid(false)
{
}

Reflector_VK::~Reflector_VK()
{
}

bool Reflector_VK::SetShader(const Blob& shader)
{
	m_bindings.clear();
	m_valid = false;

	const void* pData = nullptr;
	size_t size = 0;
	GetVKBlobData(shader, &pData, &size);
	if (!pData || size < 20 || size % 4) return false;

	const auto words = static_cast<const uint32_t*>(pData);
	const auto count = size / 4;
	if (words[0] != SpirvMagic) return false;

	unordered_map<uint32_t, string> names;
	unordered_map<uint32_t, uint32_t> bindings;
	unordered_map<uint32_t, uint32_t> spaces;
	for (auto i = 5u; i < count;)
	{
		const auto wordCount = words[i] >> 16;
		const auto opcode = words[i] & 0xffff;
		if (!wordCount || i + wordCount > count) break;

		if (opcode == OpName && wordCount >= 3)
		{
			string name(reinterpret_cast<const char*>(&words[i + 2]), (wordCount - 2) * 4);
			const auto end = name.find('\0');
			if (end != string::npos) name.resize(end);
			names[words[i + 1]] = name;
		}
		else if (opcode == OpDecorate && wordCount >= 4)
		{
			if (words[i + 2] == DecorationBinding)
				bindings[words[i + 1]] = words[i + 3];
			else if (words[i + 2] == DecorationDescriptorSet)
				spaces[words[i + 1]] = words[i + 3];
		}

		i += wordCount;
	}

	for (const auto& name : names)
	{
		const auto binding = bindings.find(name.first);
		if (binding == bindings.cend()) continue;

		BindingInfo info = {};
		info.Binding = binding->second;
		const auto space = spaces.find(name.first);
		info.Space = space != spaces.cend() ? space->second : 0;
		m_bindings[name.second] = info;
	}
	m_valid = true;

	return true;
}

bool Reflector_VK::IsValid() const
{
	return m_valid;
}

uint32_t Reflector_VK::GetResourceBindingPointByName(const char* name) const
{
	if (!name) return UINT32_MAX;
	const auto binding = m_bindings.find(name);
	if (binding == m_bindings.cend()) return UINT32_MAX;

	return binding->second.Binding;
}

uint32_t Reflector_VK::GetResourceSpaceByName(const char* name, uint32_t defaultVal) const
{
	if (!name) return defaultVal;
	const auto binding = m_bindings.find(name);
	if (binding == m_bindings.cend()) return defaultVal;

	return binding->second.Space;
}
