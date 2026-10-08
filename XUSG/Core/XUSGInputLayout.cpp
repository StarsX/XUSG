//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGInputLayout.h"

using namespace std;
using namespace XUSG;

InputLayoutLib_Impl::InputLayoutLib_Impl() :
	m_layouts(0)
{
}

InputLayoutLib_Impl::~InputLayoutLib_Impl()
{
}

void InputLayoutLib_Impl::SetLayout(uint32_t index, uint32_t numElements, const InputElement* pElements)
{
	lock_guard<mutex> lock(m_mtx);

	setLayout(index, numElements, pElements);
}

const InputLayout* InputLayoutLib_Impl::CreateLayout(uint32_t numElements, const InputElement* pElements)
{
	lock_guard<mutex> lock(m_mtx);

	const auto index = static_cast<uint32_t>(m_layouts.size());
	setLayout(index, numElements, pElements);

	return getLayout(index);
}

const InputLayout* InputLayoutLib_Impl::GetLayout(uint32_t index) const
{
	lock_guard<mutex> lock(m_mtx);

	return getLayout(index);
}

void InputLayoutLib_Impl::setLayout(uint32_t index, uint32_t numElements, const InputElement* pElements)
{
	if (index >= m_layouts.size())
		m_layouts.resize(index + 1);

	auto& layout = m_layouts[index];
	layout.resize(numElements);

	for (auto i = 0u; i < numElements; ++i)
		layout[i] = pElements[i];
}

const InputLayout* InputLayoutLib_Impl::getLayout(uint32_t index) const
{
	return index < m_layouts.size() ? &m_layouts[index] : nullptr;
}
