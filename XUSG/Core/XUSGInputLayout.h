//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSG.h"

namespace XUSG
{
	class InputLayoutPool
	{
	public:
		InputLayoutPool();
		virtual ~InputLayoutPool();

		void SetLayout(uint32_t index, const InputElementTable& elementTable);

		InputLayout CreateLayout(const InputElementTable& elementTable);

		InputLayout GetLayout(uint32_t index) const;

	protected:
		std::vector<InputLayout> m_layouts;
	};
}
