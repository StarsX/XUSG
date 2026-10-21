//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSG.h"
#include "XUSG_VK.h"

namespace XUSG
{
	struct DescriptorTableProperty_VK
	{
		uint32_t DescriptorIndex;
		uint16_t HeapType;
		uint16_t IsInitialized;
	};

	static_assert(sizeof(DescriptorTableProperty_VK) == sizeof(DescriptorTable), "DescriptorTableProperty_VK must fit in DescriptorTable");

	struct ResourceView_VK;

	class DescriptorHeap_VK
	{
	public:
		DescriptorHeap_VK();
		~DescriptorHeap_VK();

		bool Create(const Device_VK* pDevice, DescriptorHeapType type, uint32_t numDescriptors, const wchar_t* name);

		DescriptorHeapType GetType() const;
		uint32_t GetStride() const;
		uint32_t GetCapacity() const;
		VkDeviceAddress GetAddress() const;
		VkDeviceSize GetSize() const;
		VkDeviceSize GetReservedSize() const;
		void* GetMapped() const;

	protected:
		void reset();

		const Device_VK*	m_pDevice;
		VkBuffer			m_buffer;
		VkDeviceMemory		m_memory;
		void*				m_mapped;

		VkDeviceAddress		m_address;
		VkDeviceSize		m_size;
		VkDeviceSize		m_reserved;
		DescriptorHeapType	m_type;
		uint32_t			m_stride;
		uint32_t			m_capacity;
	};

	namespace Util
	{
		class DescriptorTable_VK :
			public virtual DescriptorTable
		{
		public:
			DescriptorTable_VK();
			virtual ~DescriptorTable_VK();

			void SetDescriptors(uint32_t start, uint32_t num, const Descriptor* srcDescriptors,
				uint8_t descriptorHeapIndex = 0);
			void SetSamplers(uint32_t start, uint32_t num, const Sampler* const* ppSamplers,
				uint8_t descriptorHeapIndex = 0);
			void SetSamplers(uint32_t start, uint32_t num, const SamplerPreset* presets,
				DescriptorTableLib* pDescriptorTableLib, uint8_t descriptorHeapIndex = 0);

			XUSG::DescriptorTable CreateCbvSrvUavTable(DescriptorTableLib* pDescriptorTableLib,
				const XUSG::DescriptorTable& table = XUSG_NULL);
			XUSG::DescriptorTable GetCbvSrvUavTable(DescriptorTableLib* pDescriptorTableLib,
				const XUSG::DescriptorTable& table = XUSG_NULL);

			XUSG::DescriptorTable CreateSamplerTable(DescriptorTableLib* pDescriptorTableLib,
				const XUSG::DescriptorTable& table = XUSG_NULL);
			XUSG::DescriptorTable GetSamplerTable(DescriptorTableLib* pDescriptorTableLib,
				const XUSG::DescriptorTable& table = XUSG_NULL);

			const std::string& GetKey() const;

			uint32_t CreateCbvSrvUavTableIndex(DescriptorTableLib* pDescriptorTableLib, XUSG::DescriptorTable table = XUSG_NULL);
			uint32_t GetCbvSrvUavTableIndex(DescriptorTableLib* pDescriptorTableLib, XUSG::DescriptorTable table = XUSG_NULL);
			uint32_t CreateSamplerTableIndex(DescriptorTableLib* pDescriptorTableLib, XUSG::DescriptorTable table = XUSG_NULL);
			uint32_t GetSamplerTableIndex(DescriptorTableLib* pDescriptorTableLib, XUSG::DescriptorTable table = XUSG_NULL);
			uint32_t GetDescriptorTableIndex(DescriptorTableLib* pDescriptorTableLib, DescriptorHeapType type,
				const XUSG::DescriptorTable& table) const;

		protected:
			std::string m_key;
		};
	}

	class DescriptorTableLib_VK :
		public virtual DescriptorTableLib
	{
	public:
		DescriptorTableLib_VK();
		DescriptorTableLib_VK(Device* pDevice, const wchar_t* name = nullptr);
		virtual ~DescriptorTableLib_VK();

		void SetDevice(Device* pDevice);
		void SetName(const wchar_t* name);
		void ResetDescriptorHeap(DescriptorHeapType type, uint8_t index = 0);

		bool AllocateDescriptorHeap(DescriptorHeapType type, uint32_t numDescriptors, uint8_t index = 0);

		DescriptorTable CreateCbvSrvUavTable(const Util::DescriptorTable* pUtil, const DescriptorTable& table = XUSG_NULL);
		DescriptorTable GetCbvSrvUavTable(const Util::DescriptorTable* pUtil, const DescriptorTable& table = XUSG_NULL);

		DescriptorTable CreateSamplerTable(const Util::DescriptorTable* pUtil, const DescriptorTable& table = XUSG_NULL);
		DescriptorTable GetSamplerTable(const Util::DescriptorTable* pUtil, const DescriptorTable& table = XUSG_NULL);

		DescriptorHeap GetDescriptorHeap(DescriptorHeapType type, uint8_t index = 0) const;

		const Sampler* GetSampler(SamplerPreset preset);

		uint32_t GetDescriptorStride(DescriptorHeapType type) const;
		void* GetBackend() override;

	protected:
		void checkDescriptorHeapTypeStorage(DescriptorHeapType type, uint8_t index);

		bool allocateDescriptorHeap(DescriptorHeapType type, uint32_t numDescriptors, uint8_t index);
		bool reallocateCbvSrvUavHeap(const std::string& key);
		bool reallocateSamplerHeap(const std::string& key);

		DescriptorTable createCbvSrvUavTable(const std::string& key, DescriptorTable table);
		DescriptorTable getCbvSrvUavTable(const std::string& key, DescriptorTable table);

		DescriptorTable createSamplerTable(const std::string& key, DescriptorTable table);
		DescriptorTable getSamplerTable(const std::string& key, DescriptorTable table);

		uint32_t calculateGrowth(uint32_t newSize, DescriptorHeapType type, uint8_t index) const;
		bool writeNullResource(void* pDescriptor) const;
		bool writeResource(const ResourceView_VK* pView, void* pDescriptor) const;
		bool writeSampler(const Sampler* pSampler, void* pDescriptor) const;

		Device_VK* m_pDevice;

		std::vector<std::unordered_map<std::string, DescriptorTable>> m_cbvSrvUavTables;
		std::vector<std::unordered_map<std::string, DescriptorTable>> m_samplerTables;

		std::vector<std::unique_ptr<DescriptorHeap_VK>> m_descriptorHeaps[NUM_DESCRIPTOR_HEAP];
		std::vector<uint32_t> m_descriptorCounts[NUM_DESCRIPTOR_HEAP];
		uint32_t m_descriptorStrides[NUM_DESCRIPTOR_HEAP];

		std::unique_ptr<Sampler> m_samplers[NUM_SAMPLER_PRESET];
		std::function<Sampler()> m_pfnSamplers[NUM_SAMPLER_PRESET];

		std::wstring m_name;

		mutable std::mutex m_mtx;
	};

	inline DescriptorTableLib_VK* AsVK(DescriptorTableLib* p)
	{
		assert(!p || p->GetBackend());
		return static_cast<DescriptorTableLib_VK*>(p ? p->GetBackend() : nullptr);
	}
}
