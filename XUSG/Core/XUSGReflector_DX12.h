//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSG.h"

namespace XUSG
{
	class Reflector_DX12 :
		public Reflector
	{
	public:
		Reflector_DX12();
		virtual ~Reflector_DX12();

		bool SetShader(const Blob& shader);
		bool IsValid() const;
		uint32_t GetResourceBindingPointByName(const char* name, uint32_t defaultVal = UINT32_MAX) const;

	protected:
		Shader::Reflection		m_shaderReflection;
		Shader::LibReflection	m_libraryReflection;
	};
}
