//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGBinding_DML.h"

using namespace std;
using namespace XUSG;
using namespace XUSG::ML;

Binding_DML::Binding_DML() :
	m_bindingTable(nullptr),
	m_descriptorStride(128),
	m_isDispatchable(false)
{
}

Binding_DML::~Binding_DML()
{
}

bool Binding_DML::Create(const ML::Device& device, const Operator& dispatchable, const DescriptorPool& descriptorPool,
	uint32_t descriptorCount, int32_t descriptorOffset)
{
	com_ptr<ID3D12Device> parent;
	device->GetParentDevice(IID_PPV_ARGS(&parent));
	m_descriptorStride = parent->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDescriptor(descriptorPool->GetCPUDescriptorHandleForHeapStart());
	CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuDescriptor(descriptorPool->GetGPUDescriptorHandleForHeapStart());
	hCpuDescriptor.Offset(descriptorOffset, m_descriptorStride);
	hGpuDescriptor.Offset(descriptorOffset, m_descriptorStride);

	DML_BINDING_TABLE_DESC dmlBindingTableDesc = {};
	dmlBindingTableDesc.Dispatchable = dispatchable.GetDispatchable().get();
	dmlBindingTableDesc.CPUDescriptorHandle = hCpuDescriptor;
	dmlBindingTableDesc.GPUDescriptorHandle = hGpuDescriptor;
	dmlBindingTableDesc.SizeInDescriptors = descriptorCount;

	V_RETURN(device->CreateBindingTable(&dmlBindingTableDesc, IID_PPV_ARGS(&m_bindingTable)), cerr, false);
	m_isDispatchable = false;

	return true;
}

bool Binding_DML::Reset(const Operator& dispatchable, const DescriptorPool& descriptorPool,
	uint32_t descriptorCount, int32_t descriptorOffset)
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDescriptor(descriptorPool->GetCPUDescriptorHandleForHeapStart());
	CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuDescriptor(descriptorPool->GetGPUDescriptorHandleForHeapStart());
	hCpuDescriptor.Offset(descriptorOffset, m_descriptorStride);
	hGpuDescriptor.Offset(descriptorOffset, m_descriptorStride);

	DML_BINDING_TABLE_DESC dmlBindingTableDesc = {};
	dmlBindingTableDesc.Dispatchable = dispatchable.GetDispatchable().get();
	dmlBindingTableDesc.CPUDescriptorHandle = hCpuDescriptor;
	dmlBindingTableDesc.GPUDescriptorHandle = hGpuDescriptor;
	dmlBindingTableDesc.SizeInDescriptors = descriptorCount;

	V_RETURN(m_bindingTable->Reset(&dmlBindingTableDesc), cerr, false);
	m_isDispatchable = false;

	return true;
}

void Binding_DML::BindInput(uint32_t i, const ResourceBase& buffer, uint64_t size, uint64_t offset)
{
	BindInput(i, buffer.GetResource(), size, offset);
}

void Binding_DML::BindInput(uint32_t i, const Resource& resource, uint64_t size, uint64_t offset)
{
	BindInputBuffer(i, resource, size, offset);
	BindInput(i, resource ? static_cast<size_t>(i) : -1);
}

void Binding_DML::BindInput(uint32_t i, size_t bindingIndex, uint32_t bindingCount)
{
	if (i >= m_inputBindings.size())
		m_inputBindings.resize(i + 1);

	assert(bindingIndex == -1 || bindingIndex < m_inputBufferBindings.size());

	if (bindingCount < 1 || bindingIndex == -1)
	{
		m_inputBindings[i].Type = DML_BINDING_TYPE_NONE;
		m_inputBindings[i].Desc = nullptr;
	}
	else if (bindingCount > 1)
	{
		m_inputArrayBindings.push_back(ArrayBinding());
		m_inputArrayBindings.back().BindingCount = bindingCount;
		m_inputArrayBindings.back().Bindings = (const BufferBinding*)bindingIndex;

		m_inputBindings[i].Type = DML_BINDING_TYPE_BUFFER_ARRAY;
		m_inputBindings[i].Desc = (const void*)(m_inputArrayBindings.size() - 1);
	}
	else
	{
		m_inputBindings[i].Type = DML_BINDING_TYPE_BUFFER;
		m_inputBindings[i].Desc = (const void*)bindingIndex;
	}
}

void Binding_DML::BindOutput(uint32_t i, const ResourceBase& buffer, uint64_t size, uint64_t offset)
{
	BindOutput(i, buffer.GetResource(), size, offset);
}

void Binding_DML::BindOutput(uint32_t i, const Resource& resource, uint64_t size, uint64_t offset)
{
	BindOutputBuffer(i, resource, size, offset);
	BindOutput(i, resource ? static_cast<size_t>(i) : -1);
}

void Binding_DML::BindOutput(uint32_t i, size_t bindingIndex, uint32_t bindingCount)
{
	if (i >= m_outputBindings.size())
		m_outputBindings.resize(i + 1);

	assert(bindingIndex == -1 || bindingIndex < m_outputBufferBindings.size());

	const auto isEmpty = bindingCount < 1 || bindingIndex == -1;
	if (isEmpty)
	{
		m_outputBindings[i].Type = DML_BINDING_TYPE_NONE;
		m_outputBindings[i].Desc = nullptr;
	}
	else if (bindingCount > 1)
	{
		m_outputArrayBindings.push_back(ArrayBinding());
		m_outputArrayBindings.back().BindingCount = bindingCount;
		m_outputArrayBindings.back().Bindings = (const BufferBinding*)bindingIndex;

		m_outputBindings[i].Type = DML_BINDING_TYPE_BUFFER_ARRAY;
		m_outputBindings[i].Desc = (const void*)(m_outputArrayBindings.size() - 1);
	}
	else
	{
		m_outputBindings[i].Type = DML_BINDING_TYPE_BUFFER;
		m_outputBindings[i].Desc = (const void*)bindingIndex;
	}
}

void Binding_DML::AppendInput(const ResourceBase& buffer, uint64_t size, uint64_t offset)
{
	AppendInput(buffer.GetResource(), size, offset);
}

void Binding_DML::AppendInput(const Resource& resource, uint64_t size, uint64_t offset)
{
	BindInput(static_cast<uint32_t>(m_inputBindings.size()), resource, size, offset);
}

void Binding_DML::AppendInput(size_t bindingIndex, uint32_t bindingCount)
{
	BindInput(static_cast<uint32_t>(m_inputBindings.size()), bindingIndex, bindingCount);
}

void Binding_DML::AppendOutput(const ResourceBase& buffer, uint64_t size, uint64_t offset)
{
	AppendOutput(buffer.GetResource(), size, offset);
}

void Binding_DML::AppendOutput(const Resource& resource, uint64_t size, uint64_t offset)
{
	BindOutput(static_cast<uint32_t>(m_outputBindings.size()), resource, size, offset);
}

void Binding_DML::AppendOutput(size_t bindingIndex, uint32_t bindingCount)
{
	BindOutput(static_cast<uint32_t>(m_outputBindings.size()), bindingIndex, bindingCount);
}

void Binding_DML::BindInputBuffer(uint32_t i, const ResourceBase& buffer, uint64_t size, uint64_t offset)
{
	BindInputBuffer(i, buffer.GetResource(), size, offset);
}

void Binding_DML::BindInputBuffer(uint32_t i, const Resource& resource, uint64_t size, uint64_t offset)
{
	if (i >= m_inputBufferBindings.size())
		m_inputBufferBindings.resize(i + 1);

	size = !resource || size > 0 ? size : resource->GetDesc().Width;
	m_inputBufferBindings[i].Buffer = resource.get();
	m_inputBufferBindings[i].Offset = offset;
	m_inputBufferBindings[i].SizeInBytes = size;
}

void Binding_DML::BindOutputBuffer(uint32_t i, const ResourceBase& buffer, uint64_t size, uint64_t offset)
{
	BindOutputBuffer(i, buffer.GetResource(), size, offset);
}

void Binding_DML::BindOutputBuffer(uint32_t i, const Resource& resource, uint64_t size, uint64_t offset)
{
	if (i >= m_outputBufferBindings.size())
		m_outputBufferBindings.resize(i + 1);

	size = !resource || size > 0 ? size : resource->GetDesc().Width;
	m_outputBufferBindings[i].Buffer = resource.get();
	m_outputBufferBindings[i].Offset = offset;
	m_outputBufferBindings[i].SizeInBytes = size;
}

void Binding_DML::BindTemporary(const ResourceBase& buffer, uint64_t size, uint64_t offset)
{
	BindTemporary(buffer.GetResource(), size, offset);
}

void Binding_DML::BindTemporary(const Resource& resource, uint64_t size, uint64_t offset)
{
	size = !resource || size > 0 ? size : resource->GetDesc().Width;
	const DML_BUFFER_BINDING bufferBinding = { resource.get(), offset, size };
	const DML_BINDING_DESC bindingDesc =
	{
		resource ? DML_BINDING_TYPE_BUFFER : DML_BINDING_TYPE_NONE,
		resource ? &bufferBinding : nullptr
	};

	m_bindingTable->BindTemporaryResource(&bindingDesc);
}

void Binding_DML::BindPersistent(const ResourceBase& buffer, uint64_t size, uint64_t offset)
{
	BindPersistent(buffer.GetResource(), size, offset);
}

void Binding_DML::BindPersistent(const Resource& resource, uint64_t size, uint64_t offset)
{
	size = !resource || size > 0 ? size : resource->GetDesc().Width;
	const DML_BUFFER_BINDING bufferBinding = { resource.get(), offset, size };
	const DML_BINDING_DESC bindingDesc =
	{
		resource ? DML_BINDING_TYPE_BUFFER : DML_BINDING_TYPE_NONE,
		resource ? &bufferBinding : nullptr
	};

	m_bindingTable->BindPersistentResource(&bindingDesc);
}

const BindingTable& Binding_DML::GetBindingTable() const
{
	return m_bindingTable;
}

const BindingTable& Binding_DML::GetDispatchableBindingTable()
{
	if (!m_isDispatchable)
	{
		for (auto& arrayBinding : m_inputArrayBindings)
		{
			const auto i = (size_t)arrayBinding.Bindings;
			arrayBinding.Bindings = &m_inputBufferBindings[i];
		}

		for (auto& arrayBinding : m_outputArrayBindings)
		{
			const auto i = (size_t)arrayBinding.Bindings;
			arrayBinding.Bindings = &m_outputBufferBindings[i];
		}

		for (auto& bindingDesc : m_inputBindings)
		{
			const auto i = (size_t)bindingDesc.Desc;
			if (bindingDesc.Type == DML_BINDING_TYPE_BUFFER_ARRAY)
				bindingDesc.Desc = &m_inputArrayBindings[i];
			else if (bindingDesc.Type == DML_BINDING_TYPE_BUFFER)
				bindingDesc.Desc = &m_inputBufferBindings[i];
		}

		for (auto& bindingDesc : m_outputBindings)
		{
			const auto i = (size_t)bindingDesc.Desc;
			if (bindingDesc.Type == DML_BINDING_TYPE_BUFFER_ARRAY)
				bindingDesc.Desc = &m_outputArrayBindings[i];
			else if (bindingDesc.Type == DML_BINDING_TYPE_BUFFER)
				bindingDesc.Desc = &m_outputBufferBindings[i];
		}

		m_bindingTable->BindInputs(static_cast<uint32_t>(m_inputBindings.size()), m_inputBindings.data());
		m_bindingTable->BindOutputs(static_cast<uint32_t>(m_outputBindings.size()), m_outputBindings.data());
		m_isDispatchable = true;
	}

	return m_bindingTable;
}
