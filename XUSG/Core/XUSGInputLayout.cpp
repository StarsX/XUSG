//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGInputLayout.h"

using namespace std;
using namespace XUSG;

InputLayoutPool::InputLayoutPool() :
	m_layouts(0)
{
}

InputLayoutPool::~InputLayoutPool()
{
}

void InputLayoutPool::SetLayout(uint32_t index, const InputElementTable& elementTable)
{
	if (index >= m_layouts.size())
		m_layouts.resize(index + 1);

	auto& layout = m_layouts[index];
	layout = make_shared<InputLayout::element_type>();
	layout->elements = elementTable;
	layout->pInputElementDescs = reinterpret_cast<decltype(layout->pInputElementDescs)>(layout->elements.data());
	layout->NumElements = static_cast<uint32_t>(layout->elements.size());
}

InputLayout InputLayoutPool::CreateLayout(const InputElementTable& elementTable)
{
	const auto index = static_cast<uint32_t>(m_layouts.size());

	SetLayout(index, elementTable);

	return GetLayout(index);
}

InputLayout InputLayoutPool::GetLayout(uint32_t index) const
{
	return index < m_layouts.size() ? m_layouts[index] : nullptr;
}
