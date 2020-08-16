//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

//#include "Core/XUSG.h"
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

			void BindInput(uint32_t i, const ResourceBase& buffer, uint64_t size = 0, uint64_t offset = 0);
			void BindInput(uint32_t i, const Resource& resource, uint64_t size = 0, uint64_t offset = 0);
			void BindInput(uint32_t i, size_t bindingIndex, uint32_t bindingCount = 1);
			void BindOutput(uint32_t i, const ResourceBase& buffer, uint64_t size = 0, uint64_t offset = 0);
			void BindOutput(uint32_t i, const Resource& resource, uint64_t size = 0, uint64_t offset = 0);
			void BindOutput(uint32_t i, size_t bindingIndex, uint32_t bindingCount = 1);
			void AppendInput(const ResourceBase& buffer, uint64_t size = 0, uint64_t offset = 0);
			void AppendInput(const Resource& resource, uint64_t size = 0, uint64_t offset = 0);
			void AppendInput(size_t bindingIndex, uint32_t bindingCount = 1);
			void AppendOutput(const ResourceBase& buffer, uint64_t size = 0, uint64_t offset = 0);
			void AppendOutput(const Resource& resource, uint64_t size = 0, uint64_t offset = 0);
			void AppendOutput(size_t bindingIndex, uint32_t bindingCount = 1);

			void BindInputBuffer(uint32_t i, const ResourceBase& buffer, uint64_t size = 0, uint64_t offset = 0);
			void BindInputBuffer(uint32_t i, const Resource& resource, uint64_t size = 0, uint64_t offset = 0);
			void BindOutputBuffer(uint32_t i, const ResourceBase& buffer, uint64_t size = 0, uint64_t offset = 0);
			void BindOutputBuffer(uint32_t i, const Resource& resource, uint64_t size = 0, uint64_t offset = 0);

			void BindTemporary(const ResourceBase& buffer, uint64_t size = 0, uint64_t offset = 0);
			void BindTemporary(const Resource& resource, uint64_t size = 0, uint64_t offset = 0);
			void BindPersistent(const ResourceBase& buffer, uint64_t size = 0, uint64_t offset = 0);
			void BindPersistent(const Resource& resource, uint64_t size = 0, uint64_t offset = 0);

			const BindingTable& GetBindingTable() const;
			const BindingTable& GetDispatchableBindingTable();

		protected:
			BindingTable m_bindingTable;

			std::vector<BufferBinding>	m_inputBufferBindings;
			std::vector<BufferBinding>	m_outputBufferBindings;
			std::vector<ArrayBinding>	m_inputArrayBindings;
			std::vector<ArrayBinding>	m_outputArrayBindings;
			std::vector<BindingDesc>	m_inputBindings;
			std::vector<BindingDesc>	m_outputBindings;

			uint32_t m_descriptorStride;

			bool m_isDispatchable;
		};
	}
}
