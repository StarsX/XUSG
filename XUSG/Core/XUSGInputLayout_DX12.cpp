//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGInputLayout_DX12.h"

using namespace std;
using namespace XUSG;

InputLayoutPool_DX12::InputLayoutPool_DX12() :
	m_layouts(0)
{
}

InputLayoutPool_DX12::~InputLayoutPool_DX12()
{
}

void InputLayoutPool_DX12::SetLayout(uint32_t index, const InputElementTable& elementTable)
{
	if (index >= m_layouts.size())
		m_layouts.resize(index + 1);

	auto& layout = m_layouts[index];
	layout = make_shared<InputLayout::element_type>();
	layout->Elements.resize(elementTable.size());
	layout->pInputElementDescs = layout->Elements.data();
	layout->NumElements = static_cast<uint32_t>(layout->Elements.size());

	for (auto i = 0u; i < layout->NumElements; ++i)
	{
		auto& elementDX12 = layout->Elements[i];
		const auto& element = elementTable[i];

		elementDX12.SemanticName = element.SemanticName;
		elementDX12.SemanticIndex = element.SemanticIndex;
		elementDX12.Format = GetDXGIFormat(element.Format);
		elementDX12.InputSlot = element.InputSlot;
		elementDX12.AlignedByteOffset  = element.AlignedByteOffset != APPEND_ALIGNED_ELEMENT ? element.AlignedByteOffset : D3D12_APPEND_ALIGNED_ELEMENT;
		elementDX12.InputSlotClass = GetDX12InputClassification(element.InputSlotClass);
		elementDX12.InstanceDataStepRate = element.InstanceDataStepRate;
	}
}

InputLayout InputLayoutPool_DX12::CreateLayout(const InputElementTable& elementTable)
{
	const auto index = static_cast<uint32_t>(m_layouts.size());

	SetLayout(index, elementTable);

	return GetLayout(index);
}

InputLayout InputLayoutPool_DX12::GetLayout(uint32_t index) const
{
	return index < m_layouts.size() ? m_layouts[index] : nullptr;
}
