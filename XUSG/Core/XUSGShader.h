//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSG.h"

namespace XUSG
{
	class ShaderPool_DX12:
		public ShaderPool
	{
	public:
		ShaderPool_DX12();
		virtual ~ShaderPool_DX12();

		void SetShader(Shader::Stage stage, uint32_t index, const Blob& shader);
		void SetShader(Shader::Stage stage, uint32_t index, const Blob& shader, const ReflectorPtr& reflector);
		void SetReflector(Shader::Stage stage, uint32_t index, const ReflectorPtr& reflector);

		Blob CreateShader(Shader::Stage stage, uint32_t index, const std::wstring& fileName);
		Blob GetShader(Shader::Stage stage, uint32_t index) const;
		ReflectorPtr GetReflector(Shader::Stage stage, uint32_t index) const;

	protected:
		Blob& checkShaderStorage(Shader::Stage stage, uint32_t index);
		ReflectorPtr& checkReflectorStorage(Shader::Stage stage, uint32_t index);

		std::vector<Blob> m_shaders[Shader::NUM_STAGE];
		std::vector<ReflectorPtr> m_reflectors[Shader::NUM_STAGE];
	};
}
