//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSG_DX12.h"

namespace XUSG
{
	class ShaderPool_DX12:
		public virtual ShaderPool
	{
	public:
		ShaderPool_DX12();
		virtual ~ShaderPool_DX12();

		void SetShader(Shader::Stage stage, uint32_t index, const Blob& shader);
		void SetShader(Shader::Stage stage, uint32_t index, const Blob& shader, const Reflector::sptr& reflector);
		void SetReflector(Shader::Stage stage, uint32_t index, const Reflector::sptr& reflector);

		Blob CreateShader(Shader::Stage stage, uint32_t index, const std::wstring& fileName);
		Blob GetShader(Shader::Stage stage, uint32_t index) const;
		Reflector::sptr GetReflector(Shader::Stage stage, uint32_t index) const;

	protected:
		com_ptr<ID3DBlob>& checkShaderStorage(Shader::Stage stage, uint32_t index);
		Reflector::sptr& checkReflectorStorage(Shader::Stage stage, uint32_t index);

		std::vector<com_ptr<ID3DBlob>> m_shaders[Shader::NUM_STAGE];
		std::vector<Reflector::sptr> m_reflectors[Shader::NUM_STAGE];
	};
}
