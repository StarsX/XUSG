//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSG_DX12.h"
#include "XUSGReflector_DX12.h"
#include "XUSGShader_DX12.h"

using namespace std;
using namespace XUSG;
using namespace Shader;

ShaderLib_DX12::ShaderLib_DX12() :
	m_shaders(),
	m_reflectors()
{
}

ShaderLib_DX12::~ShaderLib_DX12()
{
}

void ShaderLib_DX12::SetShader(Shader::Stage stage, uint32_t index, const Blob& shader)
{
	checkShaderStorage(stage, index) = shader;
}

void ShaderLib_DX12::SetShader(Shader::Stage stage, uint32_t index, const Blob& shader, const Reflector::sptr& reflector)
{
	SetShader(stage, index, shader);
	SetReflector(stage, index, reflector);
}

void ShaderLib_DX12::SetReflector(Shader::Stage stage, uint32_t index, const Reflector::sptr& reflector)
{
	checkReflectorStorage(stage, index) = reflector;
}

Blob ShaderLib_DX12::CreateShader(Shader::Stage stage, uint32_t index, const wchar_t* fileName)
{
	auto& shader = checkShaderStorage(stage, index);
	V_RETURN(D3DReadFileToBlob(fileName, shader.put()), cerr, nullptr);

	auto& reflector = checkReflectorStorage(stage, index);
	reflector = make_shared<Reflector_DX12>();
	XUSG_N_RETURN(reflector->SetShader(shader.get()), nullptr);

	return shader.get();
}

Blob ShaderLib_DX12::CreateShader(Shader::Stage stage, uint32_t index, const void* pData, size_t size)
{
	auto& shader = checkShaderStorage(stage, index);
	V_RETURN(D3DCreateBlob(size, shader.put()), cerr, nullptr);
	memcpy(shader->GetBufferPointer(), pData, size);

	auto& reflector = checkReflectorStorage(stage, index);
	reflector = make_shared<Reflector_DX12>();
	XUSG_N_RETURN(reflector->SetShader(shader.get()), nullptr);

	return shader.get();
}

Blob ShaderLib_DX12::GetShader(Shader::Stage stage, uint32_t index) const
{
	return index < m_shaders[stage].size() ? m_shaders[stage][index].get() : nullptr;
}

Reflector::sptr ShaderLib_DX12::GetReflector(Shader::Stage stage, uint32_t index) const
{
	return index < m_reflectors[stage].size() ? m_reflectors[stage][index] : nullptr;
}

com_ptr<ID3DBlob>& ShaderLib_DX12::checkShaderStorage(Shader::Stage stage, uint32_t index)
{
	if (index >= m_shaders[stage].size())
		m_shaders[stage].resize(index + 1);

	return m_shaders[stage][index];
}

Reflector::sptr& ShaderLib_DX12::checkReflectorStorage(Shader::Stage stage, uint32_t index)
{
	if (index >= m_reflectors[stage].size())
		m_reflectors[stage].resize(index + 1);

	return m_reflectors[stage][index];
}
