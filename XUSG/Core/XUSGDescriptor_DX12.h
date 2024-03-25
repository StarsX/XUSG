//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSG.h"

namespace XUSG
{
	namespace Util
	{
		class DescriptorTable_DX12 :
			public virtual DescriptorTable
		{
		public:
			DescriptorTable_DX12();
			virtual ~DescriptorTable_DX12();

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

			Framebuffer CreateFramebuffer(DescriptorTableLib* pDescriptorTableLib,
				const Descriptor* pDsv = nullptr, const Framebuffer* pFramebuffer = nullptr);
			Framebuffer GetFramebuffer(DescriptorTableLib* pDescriptorTableLib,
				const Descriptor* pDsv = nullptr, const Framebuffer* pFramebuffer = nullptr);

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

	class DescriptorTableLib_DX12 :
		public virtual DescriptorTableLib
	{
	public:
		DescriptorTableLib_DX12();
		DescriptorTableLib_DX12(const Device* pDevice, const wchar_t* name = nullptr);
		virtual ~DescriptorTableLib_DX12();

		void SetDevice(const Device* pDevice);
		void SetName(const wchar_t* name);
		void ResetDescriptorHeap(DescriptorHeapType type, uint8_t index = 0);

		bool AllocateDescriptorHeap(DescriptorHeapType type, uint32_t numDescriptors, uint8_t index = 0);

		DescriptorTable CreateCbvSrvUavTable(const Util::DescriptorTable* pUtil, const DescriptorTable& table = XUSG_NULL);
		DescriptorTable GetCbvSrvUavTable(const Util::DescriptorTable* pUtil, const DescriptorTable& table = XUSG_NULL);

		DescriptorTable CreateSamplerTable(const Util::DescriptorTable* pUtil, const DescriptorTable& table = XUSG_NULL);
		DescriptorTable GetSamplerTable(const Util::DescriptorTable* pUtil, const DescriptorTable& table = XUSG_NULL);

		Framebuffer CreateFramebuffer(const Util::DescriptorTable* pUtil,
			const Descriptor* pDsv = nullptr, const Framebuffer* pFramebuffer = nullptr);
		Framebuffer GetFramebuffer(const Util::DescriptorTable* pUtil,
			const Descriptor* pDsv = nullptr, const Framebuffer* pFramebuffer = nullptr);

		DescriptorHeap GetDescriptorHeap(DescriptorHeapType type, uint8_t index = 0) const;

		const Sampler* GetSampler(SamplerPreset preset);

		uint32_t GetDescriptorStride(DescriptorHeapType type) const;

	protected:
		void checkDescriptorHeapTypeStorage(DescriptorHeapType type, uint8_t index);

		bool allocateDescriptorHeap(DescriptorHeapType type, uint32_t numDescriptors, uint8_t index);
		bool reallocateCbvSrvUavHeap(const std::string& key);
		bool reallocateSamplerHeap(const std::string& key);
		bool reallocateRtvHeap(const std::string& key);

		DescriptorTable createCbvSrvUavTable(const std::string& key, DescriptorTable table);
		DescriptorTable getCbvSrvUavTable(const std::string& key, DescriptorTable table);

		DescriptorTable createSamplerTable(const std::string& key, DescriptorTable table);
		DescriptorTable getSamplerTable(const std::string& key, DescriptorTable table);

		Framebuffer createFramebuffer(const std::string& key, const Descriptor *pDsv, const Framebuffer* pFramebuffer);
		Framebuffer getFramebuffer(const std::string& key, const Descriptor* pDsv, const Framebuffer* pFramebuffer);

		uint32_t calculateGrowth(uint32_t newSize, DescriptorHeapType type, uint8_t index) const;

		com_ptr<ID3D12Device> m_device;

		std::vector<std::unordered_map<std::string, DescriptorTable>> m_cbvSrvUavTables;
		std::vector<std::unordered_map<std::string, DescriptorTable>> m_samplerTables;
		std::vector<std::unordered_map<std::string, std::shared_ptr<Descriptor>>> m_rtvTables;

		std::vector<com_ptr<ID3D12DescriptorHeap>> m_descriptorHeaps[NUM_DESCRIPTOR_HEAP];
		std::vector<uint32_t> m_descriptorCounts[NUM_DESCRIPTOR_HEAP];
		uint32_t m_descriptorStrides[NUM_DESCRIPTOR_HEAP];

		std::unique_ptr<Sampler> m_samplers[NUM_SAMPLER_PRESET];
		std::function<Sampler()> m_pfnSamplers[NUM_SAMPLER_PRESET];

		std::wstring m_name;
	};
}
