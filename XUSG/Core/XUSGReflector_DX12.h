//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSG_DX12.h"

namespace XUSG
{
	class Reflector_DX12 :
		public virtual Reflector
	{
	public:
		Reflector_DX12();
		virtual ~Reflector_DX12();

		bool SetShader(const Blob& shader);
		bool IsValid() const;
		uint32_t GetResourceBindingPointByName(const char* name, uint32_t defaultVal = UINT32_MAX) const;

	protected:
		com_ptr<ID3D12ShaderReflection> m_shaderReflection;
		com_ptr<ID3D12LibraryReflection> m_libraryReflection;
	};
}
