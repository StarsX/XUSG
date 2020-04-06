//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGReflector.h"
#include "XUSGShader.h"

using namespace std;
using namespace XUSG;
using namespace Shader;

ShaderPool_DX12::ShaderPool_DX12() :
	m_shaders(),
	m_reflectors()
{
}

ShaderPool_DX12::~ShaderPool_DX12()
{
}

void ShaderPool_DX12::SetShader(Shader::Stage stage, uint32_t index, const Blob& shader)
{
	checkShaderStorage(stage, index) = shader;
}

void ShaderPool_DX12::SetShader(Shader::Stage stage, uint32_t index, const Blob& shader, const Reflector::sptr& reflector)
{
	SetShader(stage, index, shader);
	SetReflector(stage, index, reflector);
}

void ShaderPool_DX12::SetReflector(Shader::Stage stage, uint32_t index, const Reflector::sptr& reflector)
{
	checkReflectorStorage(stage, index) = reflector;
}

Blob ShaderPool_DX12::CreateShader(Shader::Stage stage, uint32_t index, const wstring& fileName)
{
	auto& shader = checkShaderStorage(stage, index);
	V_RETURN(D3DReadFileToBlob(fileName.c_str(), &shader), cerr, nullptr);

	auto& reflector = checkReflectorStorage(stage, index);
	reflector = make_shared<Reflector_DX12>();
	N_RETURN(reflector->SetShader(shader), nullptr);

	return shader;
}

Blob ShaderPool_DX12::GetShader(Shader::Stage stage, uint32_t index) const
{
	return index < m_shaders[stage].size() ? m_shaders[stage][index] : nullptr;
}

Reflector::sptr ShaderPool_DX12::GetReflector(Shader::Stage stage, uint32_t index) const
{
	return index < m_reflectors[stage].size() ? m_reflectors[stage][index] : nullptr;
}

Blob& ShaderPool_DX12::checkShaderStorage(Shader::Stage stage, uint32_t index)
{
	if (index >= m_shaders[stage].size())
		m_shaders[stage].resize(index + 1);

	return m_shaders[stage][index];
}

Reflector::sptr& ShaderPool_DX12::checkReflectorStorage(Shader::Stage stage, uint32_t index)
{
	if (index >= m_reflectors[stage].size())
		m_reflectors[stage].resize(index + 1);

	return m_reflectors[stage][index];
}
