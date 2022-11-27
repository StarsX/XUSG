//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSG.h"

namespace XUSG
{
	class InputLayoutLib_DX12
	{
	public:
		InputLayoutLib_DX12();
		virtual ~InputLayoutLib_DX12();

		void SetLayout(uint32_t index, const InputElement* pElements, uint32_t numElements);

		const InputLayout* CreateLayout(const InputElement* pElements, uint32_t numElements);

		const InputLayout* GetLayout(uint32_t index) const;

	protected:
		std::vector<InputLayout> m_layouts;
	};
}
