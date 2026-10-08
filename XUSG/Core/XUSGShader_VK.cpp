//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGShader_VK.h"
#include "XUSGReflector_VK.h"
#include <cstdio>

using namespace std;
using namespace XUSG;
using namespace Shader;

ShaderLib_VK::ShaderLib_VK()
{
}

ShaderLib_VK::~ShaderLib_VK()
{
}

void ShaderLib_VK::SetShader(Shader::Stage stage, uint32_t index, const Blob& shader)
{
	lock_guard<mutex> lock(m_mtx);

	setShader(stage, index, shader);
}

void ShaderLib_VK::SetShader(Shader::Stage stage, uint32_t index, const Blob& shader, const Reflector::sptr& reflector)
{
	lock_guard<mutex> lock(m_mtx);

	setShader(stage, index, shader);
	setReflector(stage, index, reflector);
}

void ShaderLib_VK::SetReflector(Shader::Stage stage, uint32_t index, const Reflector::sptr& reflector)
{
	lock_guard<mutex> lock(m_mtx);

	setReflector(stage, index, reflector);
}

Blob ShaderLib_VK::CreateShader(Shader::Stage stage, uint32_t index, const wchar_t* fileName)
{
	lock_guard<mutex> lock(m_mtx);

	FILE* file = nullptr;
#if defined(WIN32) || defined(_WIN32)
	if (fileName) _wfopen_s(&file, fileName, L"rb");
#else
	const auto path = WideToUtf8(fileName);
	if (!path.empty()) file = fopen(path.c_str(), "rb");
#endif
	if (!file) return nullptr;

	if (fseek(file, 0, SEEK_END) != 0)
	{
		fclose(file);

		return nullptr;
	}
	const auto end = ftell(file);
	if (end < 0 || fseek(file, 0, SEEK_SET) != 0)
	{
		fclose(file);

		return nullptr;
	}

	const auto size = static_cast<size_t>(end);
	auto& shader = checkShaderStorage(stage, index);
	shader = make_unique<Blob_VK>();
	shader->Data.resize(size);
	if (size && fread(shader->Data.data(), 1, size, file) != size)
	{
		fclose(file);

		return nullptr;
	}
	fclose(file);

	auto& reflector = checkReflectorStorage(stage, index);
	reflector = make_shared<Reflector_VK>();
	XUSG_N_RETURN(reflector->SetShader(shader.get()), shader.get());

	return shader.get();
}

Blob ShaderLib_VK::CreateShader(Shader::Stage stage, uint32_t index, const void* pData, size_t size)
{
	lock_guard<mutex> lock(m_mtx);

	auto& shader = checkShaderStorage(stage, index);
	shader = make_unique<Blob_VK>();
	shader->Data.resize(size);
	if (pData && size) memcpy(shader->Data.data(), pData, size);

	auto& reflector = checkReflectorStorage(stage, index);
	reflector = make_shared<Reflector_VK>();
	XUSG_N_RETURN(reflector->SetShader(shader.get()), shader.get());

	return shader.get();
}

Blob ShaderLib_VK::GetShader(Shader::Stage stage, uint32_t index) const
{
	lock_guard<mutex> lock(m_mtx);

	return index < m_shaders[stage].size() && m_shaders[stage][index] ? m_shaders[stage][index].get() : nullptr;
}

Reflector::sptr ShaderLib_VK::GetReflector(Shader::Stage stage, uint32_t index) const
{
	lock_guard<mutex> lock(m_mtx);

	return index < m_reflectors[stage].size() ? m_reflectors[stage][index] : nullptr;
}

void ShaderLib_VK::setShader(Shader::Stage stage, uint32_t index, const Blob& shader)
{
	auto& slot = checkShaderStorage(stage, index);
	if (!shader)
	{
		slot.reset();

		return;
	}

	const auto source = static_cast<const Blob_VK*>(shader);
	if (slot.get() == source) return;
	slot = make_unique<Blob_VK>();
	slot->Data = source->Data;
}

void ShaderLib_VK::setReflector(Shader::Stage stage, uint32_t index, const Reflector::sptr& reflector)
{
	checkReflectorStorage(stage, index) = reflector;
}

unique_ptr<Blob_VK>& ShaderLib_VK::checkShaderStorage(Shader::Stage stage, uint32_t index)
{
	if (index >= m_shaders[stage].size())
		m_shaders[stage].resize(index + 1);

	return m_shaders[stage][index];
}

Reflector::sptr& ShaderLib_VK::checkReflectorStorage(Shader::Stage stage, uint32_t index)
{
	if (index >= m_reflectors[stage].size())
		m_reflectors[stage].resize(index + 1);

	return m_reflectors[stage][index];
}
