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
			public DescriptorTable
		{
		public:
			DescriptorTable_DX12();
			virtual ~DescriptorTable_DX12();

			void SetDescriptors(uint32_t start, uint32_t num, const Descriptor* srcDescriptors,
				uint8_t descriptorPoolIndex = 0);
			void SetSamplers(uint32_t start, uint32_t num, const SamplerPreset* presets,
				DescriptorTableCache& descriptorTableCache, uint8_t descriptorPoolIndex = 0);

			XUSG::DescriptorTable CreateCbvSrvUavTable(DescriptorTableCache& descriptorTableCache,
				const XUSG::DescriptorTable& table = nullptr);
			XUSG::DescriptorTable GetCbvSrvUavTable(DescriptorTableCache& descriptorTableCache,
				const XUSG::DescriptorTable& table = nullptr);

			XUSG::DescriptorTable CreateSamplerTable(DescriptorTableCache& descriptorTableCache,
				const XUSG::DescriptorTable& table = nullptr);
			XUSG::DescriptorTable GetSamplerTable(DescriptorTableCache& descriptorTableCache,
				const XUSG::DescriptorTable& table = nullptr);

			Framebuffer CreateFramebuffer(DescriptorTableCache& descriptorTableCache,
				const Descriptor* pDsv = nullptr, const Framebuffer* pFramebuffer = nullptr);
			Framebuffer GetFramebuffer(DescriptorTableCache& descriptorTableCache,
				const Descriptor* pDsv = nullptr, const Framebuffer* pFramebuffer = nullptr);

			const std::string& GetKey() const;

		protected:
			std::string m_key;
		};
	}

	class DescriptorTableCache_DX12 :
		public DescriptorTableCache
	{
	public:
		DescriptorTableCache_DX12();
		DescriptorTableCache_DX12(const Device& device, const wchar_t* name = nullptr);
		virtual ~DescriptorTableCache_DX12();

		void SetDevice(const Device& device);
		void SetName(const wchar_t* name);
		void ResetDescriptorPool(DescriptorPoolType type, uint8_t index);

		bool AllocateDescriptorPool(DescriptorPoolType type, uint32_t numDescriptors, uint8_t index = 0);

		DescriptorTable CreateCbvSrvUavTable(const Util::DescriptorTable& util, const DescriptorTable& table = nullptr);
		DescriptorTable GetCbvSrvUavTable(const Util::DescriptorTable& util, const DescriptorTable& table = nullptr);

		DescriptorTable CreateSamplerTable(const Util::DescriptorTable& util, const DescriptorTable& table = nullptr);
		DescriptorTable GetSamplerTable(const Util::DescriptorTable& util, const DescriptorTable& table = nullptr);

		Framebuffer CreateFramebuffer(const Util::DescriptorTable& util,
			const Descriptor* pDsv = nullptr, const Framebuffer* pFramebuffer = nullptr);
		Framebuffer GetFramebuffer(const Util::DescriptorTable& util,
			const Descriptor* pDsv = nullptr, const Framebuffer* pFramebuffer = nullptr);

		const DescriptorPool& GetDescriptorPool(DescriptorPoolType type, uint8_t index = 0) const;

		const std::shared_ptr<Sampler>& GetSampler(SamplerPreset preset);

		uint32_t GetDescriptorStride(DescriptorPoolType type) const;

	protected:
		friend class Util::DescriptorTable_DX12;

		void checkDescriptorPoolTypeStorage(DescriptorPoolType type, uint8_t index);

		bool allocateDescriptorPool(DescriptorPoolType type, uint32_t numDescriptors, uint8_t index);
		bool reallocateCbvSrvUavPool(const std::string& key);
		bool reallocateSamplerPool(const std::string& key);
		bool reallocateRtvPool(const std::string& key);

		DescriptorTable createCbvSrvUavTable(const std::string& key, DescriptorTable table);
		DescriptorTable getCbvSrvUavTable(const std::string& key, DescriptorTable table);

		DescriptorTable createSamplerTable(const std::string& key, DescriptorTable table);
		DescriptorTable getSamplerTable(const std::string& key, DescriptorTable table);

		Framebuffer createFramebuffer(const std::string& key, const Descriptor *pDsv, const Framebuffer* pFramebuffer);
		Framebuffer getFramebuffer(const std::string& key, const Descriptor* pDsv, const Framebuffer* pFramebuffer);

		uint32_t calculateGrowth(uint32_t numDescriptors, DescriptorPoolType type, uint8_t index) const;

		Device m_device;

		std::unordered_map<std::string, DescriptorTable> m_cbvSrvUavTables;
		std::unordered_map<std::string, DescriptorTable> m_samplerTables;
		std::unordered_map<std::string, std::shared_ptr<Descriptor>> m_rtvTables;

		std::vector<std::vector<const std::string*>> m_descriptorKeyPtrs[NUM_DESCRIPTOR_POOL];

		std::vector<DescriptorPool>	m_descriptorPools[NUM_DESCRIPTOR_POOL];
		std::vector<uint32_t>		m_descriptorCounts[NUM_DESCRIPTOR_POOL];
		uint32_t m_descriptorStrides[NUM_DESCRIPTOR_POOL];

		std::shared_ptr<Sampler> m_samplerPresets[NUM_SAMPLER_PRESET];
		std::function<Sampler()> m_pfnSamplers[NUM_SAMPLER_PRESET];

		std::wstring m_name;

		static Sampler SamplerPointWrap();
		static Sampler SamplerPointMirror();
		static Sampler SamplerPointClamp();
		static Sampler SamplerPointBorder();
		static Sampler SamplerPointLessEqual();
		static Sampler SamplerLinearWrap();
		static Sampler SamplerLinearMirror();
		static Sampler SamplerLinearClamp();
		static Sampler SamplerLinearBorder();
		static Sampler SamplerLinearLessEqual();
		static Sampler SamplerAnisotropicWrap();
		static Sampler SamplerAnisotropicMirror();
		static Sampler SamplerAnisotropicClamp();
		static Sampler SamplerAnisotropicBorder();
		static Sampler SamplerAnisotropicLessEqual();
	};
}
