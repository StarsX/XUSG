//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGInputLayout_DX12.h"
#include "XUSGEnum_DX12.h"

using namespace std;
using namespace XUSG;

InputLayoutLib_DX12::InputLayoutLib_DX12() :
	m_layouts(0)
{
}

InputLayoutLib_DX12::~InputLayoutLib_DX12()
{
}

void InputLayoutLib_DX12::SetLayout(uint32_t index, const InputElement* pElements, uint32_t numElements)
{
	if (index >= m_layouts.size())
		m_layouts.resize(index + 1);

	auto& layout = m_layouts[index];
	layout.resize(numElements);

	for (auto i = 0u; i < numElements; ++i)
		layout[i] = pElements[i];
}

const InputLayout* InputLayoutLib_DX12::CreateLayout(const InputElement* pElements, uint32_t numElements)
{
	const auto index = static_cast<uint32_t>(m_layouts.size());

	SetLayout(index, pElements, numElements);

	return GetLayout(index);
}

const InputLayout* InputLayoutLib_DX12::GetLayout(uint32_t index) const
{
	return index < m_layouts.size() ? &m_layouts[index] : nullptr;
}
