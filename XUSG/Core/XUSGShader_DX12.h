//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSG.h"

namespace XUSG
{
	class ShaderLib_DX12:
		public virtual ShaderLib
	{
	public:
		ShaderLib_DX12();
		virtual ~ShaderLib_DX12();

		void SetShader(Shader::Stage stage, uint32_t index, const Blob& shader);
		void SetShader(Shader::Stage stage, uint32_t index, const Blob& shader, const Reflector::sptr& reflector);
		void SetReflector(Shader::Stage stage, uint32_t index, const Reflector::sptr& reflector);

		Blob CreateShader(Shader::Stage stage, uint32_t index, const wchar_t* fileName);
		Blob CreateShader(Shader::Stage stage, uint32_t index, const void* pData, size_t size);
		Blob GetShader(Shader::Stage stage, uint32_t index) const;
		Reflector::sptr GetReflector(Shader::Stage stage, uint32_t index) const;

	protected:
		com_ptr<ID3DBlob>& checkShaderStorage(Shader::Stage stage, uint32_t index);
		Reflector::sptr& checkReflectorStorage(Shader::Stage stage, uint32_t index);

		std::vector<com_ptr<ID3DBlob>> m_shaders[Shader::NUM_STAGE];
		std::vector<Reflector::sptr> m_reflectors[Shader::NUM_STAGE];
	};
}
