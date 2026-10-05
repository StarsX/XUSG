//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSG_VK.h"

namespace XUSG
{
	class Reflector_VK :
		public virtual Reflector
	{
	public:
		Reflector_VK();
		virtual ~Reflector_VK();

		bool SetShader(const Blob& shader);
		bool IsValid() const;
		uint32_t GetResourceBindingPointByName(const char* name, uint32_t defaultVal = UINT32_MAX) const;

	protected:
		bool m_valid;
		std::unordered_map<std::string, uint32_t> m_bindings;
	};
}
