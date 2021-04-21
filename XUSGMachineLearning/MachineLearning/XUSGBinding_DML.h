//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSGMachineLearning.h"

namespace XUSG
{
	namespace ML
	{
		class Binding_DML :
			public virtual Binding
		{
		public:
			Binding_DML();
			virtual ~Binding_DML();

			bool Create(const Device& device, const Operator& dispatchable, const DescriptorPool& descriptorPool,
				uint32_t descriptorCount, int32_t descriptorOffset = 0);
			bool Reset(const Operator& dispatchable, const DescriptorPool& descriptorPool,
				uint32_t descriptorCount, int32_t descriptorOffset = 0);

			void BindInput(uint32_t i, const Resource* pResource, uint64_t size = 0, uint64_t offset = 0);
			void BindInput(uint32_t i, uintptr_t bindingIndex, uint32_t bindingCount = 1);
			void BindOutput(uint32_t i, const Resource* pResource, uint64_t size = 0, uint64_t offset = 0);
			void BindOutput(uint32_t i, uintptr_t bindingIndex, uint32_t bindingCount = 1);
			void AppendInput(const Resource* pResource, uint64_t size = 0, uint64_t offset = 0);
			void AppendInput(uintptr_t bindingIndex, uint32_t bindingCount = 1);
			void AppendOutput(const Resource* pResource, uint64_t size = 0, uint64_t offset = 0);
			void AppendOutput(uintptr_t bindingIndex, uint32_t bindingCount = 1);

			void BindInputBuffer(uint32_t i, const Resource* pResource, uint64_t size = 0, uint64_t offset = 0);
			void BindOutputBuffer(uint32_t i, const Resource* pResource, uint64_t size = 0, uint64_t offset = 0);

			void BindTemporary(const Resource* pResource, uint64_t size = 0, uint64_t offset = 0);
			void BindPersistent(const Resource* pResource, uint64_t size = 0, uint64_t offset = 0);

			BindingTable GetBindingTable() const;
			BindingTable GetDispatchableBindingTable();

		protected:
			com_ptr<IDMLBindingTable> m_bindingTable;

			std::vector<DML_BUFFER_BINDING>			m_inputBufferBindings;
			std::vector<DML_BUFFER_BINDING>			m_outputBufferBindings;
			std::vector<DML_BUFFER_ARRAY_BINDING>	m_inputArrayBindings;
			std::vector<DML_BUFFER_ARRAY_BINDING>	m_outputArrayBindings;
			std::vector<DML_BINDING_DESC>			m_inputBindings;
			std::vector<DML_BINDING_DESC>			m_outputBindings;

			uint32_t m_descriptorStride;

			bool m_isDispatchable;
		};
	}
}
