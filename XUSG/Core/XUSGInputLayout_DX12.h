//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSG_DX12.h"

namespace XUSG
{
	class InputLayoutPool_DX12
	{
	public:
		InputLayoutPool_DX12();
		virtual ~InputLayoutPool_DX12();

		void SetLayout(uint32_t index, const InputElement* pElements, uint32_t numElements);

		const InputLayout* CreateLayout(const InputElement* pElements, uint32_t numElements);

		const InputLayout* GetLayout(uint32_t index) const;

	protected:
		std::vector<InputLayout> m_layouts;
	};
}
