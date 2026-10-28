//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSG_VK.h"

namespace XUSG
{
	namespace Util
	{
		class PipelineLayout_VK :
			public virtual PipelineLayout
		{
		public:
			PipelineLayout_VK();
			virtual ~PipelineLayout_VK();

			void SetShaderStage(uint32_t index, Shader::Stage stage);
			void SetRange(uint32_t index, DescriptorType type, uint32_t num, uint32_t baseBinding,
				uint32_t space = 0, DescriptorFlag flags = DescriptorFlag::NONE,
				uint32_t offsetInDescriptors = XUSG_DESCRIPTOR_OFFSET_APPEND);
			void SetConstants(uint32_t index, uint32_t num32BitValues, uint32_t binding,
				uint32_t space = 0, Shader::Stage stage = Shader::Stage::ALL);
			void SetRootSRV(uint32_t index, uint32_t binding, uint32_t space = 0,
				DescriptorFlag flags = DescriptorFlag::NONE, Shader::Stage stage = Shader::Stage::ALL);
			void SetRootUAV(uint32_t index, uint32_t binding, uint32_t space = 0,
				DescriptorFlag flags = DescriptorFlag::DATA_STATIC_WHILE_SET_AT_EXECUTE, Shader::Stage stage = Shader::Stage::ALL);
			void SetRootCBV(uint32_t index, uint32_t binding, uint32_t space = 0, Shader::Stage stage = Shader::Stage::ALL);
			void SetStaticSamplers(const Sampler* const* ppSamplers, uint32_t num, uint32_t baseBinding,
				uint32_t space = 0, Shader::Stage stage = Shader::Stage::ALL);

			XUSG::PipelineLayout CreatePipelineLayout(PipelineLayoutLib* pPipelineLayoutLib, PipelineLayoutFlag flags,
				const wchar_t* name = nullptr);
			XUSG::PipelineLayout GetPipelineLayout(PipelineLayoutLib* pPipelineLayoutLib, PipelineLayoutFlag flags,
				const wchar_t* name = nullptr);

			DescriptorTableLayout CreateDescriptorTableLayout(uint32_t index, PipelineLayoutLib* pPipelineLayoutLib) const;
			DescriptorTableLayout GetDescriptorTableLayout(uint32_t index, PipelineLayoutLib* pPipelineLayoutLib) const;

			const std::vector<std::string>& GetDescriptorTableLayoutKeys() const;
			std::string& GetPipelineLayoutKey(PipelineLayoutLib* pPipelineLayoutLib);
			void* GetBackend();

			static const uint8_t DescriptorTableLayoutCountOffset = sizeof(uint16_t);
			static const uint8_t DescriptorTableLayoutOffset = DescriptorTableLayoutCountOffset + sizeof(uint16_t);

		protected:
			std::string& checkKeyStorage(uint32_t index);

			std::vector<std::string> m_descriptorTableLayoutKeys;
			std::vector<StaticSampler> m_staticSamplers;
			std::string m_pipelineLayoutKey;

			bool m_isTableLayoutsCompleted;
		};

		PipelineLayout_VK* AsVK(PipelineLayout* p);
	}

	class PipelineLayoutLib_VK :
		public virtual PipelineLayoutLib
	{
	public:
		PipelineLayoutLib_VK();
		PipelineLayoutLib_VK(Device* pDevice);
		virtual ~PipelineLayoutLib_VK();

		void SetDevice(Device* pDevice);
		void SetPipelineLayout(const std::string& key, const PipelineLayout& pipelineLayout);

		PipelineLayout CreatePipelineLayout(Util::PipelineLayout* pUtil, PipelineLayoutFlag flags,
			const wchar_t* name = nullptr, uint32_t nodeMask = 0);
		PipelineLayout GetPipelineLayout(Util::PipelineLayout* pUtil, PipelineLayoutFlag flags,
			const wchar_t* name = nullptr, bool create = true, uint32_t nodeMask = 0);
		PipelineLayout CreateRootSignature(const Blob& blobSignature,
			const wchar_t* name = nullptr, uint32_t nodeMask = 0);
		PipelineLayout GetRootSignature(const Blob& blobSignature,
			const wchar_t* name = nullptr, bool create = true, uint32_t nodeMask = 0);

		DescriptorTableLayout CreateDescriptorTableLayout(uint32_t index, const Util::PipelineLayout* pUtil);
		DescriptorTableLayout GetDescriptorTableLayout(uint32_t index, const Util::PipelineLayout* pUtil);

		void* GetBackend();

	protected:
		PipelineLayout createPipelineLayout(const std::string& key, const wchar_t* name, uint32_t nodeMask);
		PipelineLayout createRootSignature(const std::string& key, const Blob& blobSignature,
			const wchar_t* name, uint32_t nodeMask);
		PipelineLayout createRootSignatureFromPayload(const std::string& key, const void* pPayload,
			size_t payloadSize, const wchar_t* name, uint32_t nodeMask);
		PipelineLayout getPipelineLayout(const std::string& key, const wchar_t* name, bool create, uint32_t nodeMask);
		PipelineLayout getRootSignature(const std::string& key, const Blob& blobSignature,
			const wchar_t* name, bool create, uint32_t nodeMask);

		DescriptorTableLayout createDescriptorTableLayout(const std::string& key);
		DescriptorTableLayout getDescriptorTableLayout(const std::string& key);

		void appendMapping(PipelineLayoutObject& layout, DescriptorType type, uint32_t set, uint32_t binding,
			uint32_t count, VkDescriptorMappingSourceEXT source, const VkDescriptorMappingSourceDataEXT& data,
			Shader::Stage stage);
		void fillEmbeddedSampler(EmbeddedSampler_VK& embedded, const Sampler* pSampler) const;
		uint32_t alignPush(uint32_t offset, uint32_t alignment) const;

		struct LayoutEntry
		{
			std::unique_ptr<PipelineLayoutObject> Owned;
			PipelineLayoutObject* Object;
		};

		Device_VK* m_pDevice;

		std::unordered_map<std::string, LayoutEntry> m_pipelineLayouts;
		std::unordered_map<std::string, DescriptorTableLayout> m_descriptorTableLayouts;

		std::mutex m_mtx;
	};

	PipelineLayoutLib_VK* AsVK(PipelineLayoutLib* p);
}
