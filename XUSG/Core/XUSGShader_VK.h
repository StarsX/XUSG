//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSG_VK.h"

namespace XUSG
{
	class Reflector_VK;

	class ShaderLib_VK :
		public virtual ShaderLib
	{
	public:
		ShaderLib_VK();
		virtual ~ShaderLib_VK();

		void SetShader(Shader::Stage stage, uint32_t index, const Blob& shader);
		void SetShader(Shader::Stage stage, uint32_t index, const Blob& shader, const Reflector::sptr& reflector);
		void SetReflector(Shader::Stage stage, uint32_t index, const Reflector::sptr& reflector);

		Blob CreateShader(Shader::Stage stage, uint32_t index, const wchar_t* fileName);
		Blob CreateShader(Shader::Stage stage, uint32_t index, const void* pData, size_t size);
		Blob GetShader(Shader::Stage stage, uint32_t index);
		Reflector::sptr GetReflector(Shader::Stage stage, uint32_t index);

	protected:
		void setShader(Shader::Stage stage, uint32_t index, const Blob& shader);
		void setReflector(Shader::Stage stage, uint32_t index, const Reflector::sptr& reflector);

		std::unique_ptr<Blob_VK>& checkShaderStorage(Shader::Stage stage, uint32_t index);
		Reflector::sptr& checkReflectorStorage(Shader::Stage stage, uint32_t index);

		std::vector<std::unique_ptr<Blob_VK>> m_shaders[Shader::NUM_STAGE];
		std::vector<Reflector::sptr> m_reflectors[Shader::NUM_STAGE];

		std::mutex m_mtx;
	};
}
