//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSG_DX12.h"

namespace XUSG
{
	namespace Util
	{
		class PipelineLayout_DX12 :
			public virtual PipelineLayout
		{
		public:
			PipelineLayout_DX12();
			virtual ~PipelineLayout_DX12();

			void SetShaderStage(uint32_t index, Shader::Stage stage);
			void SetRange(uint32_t index, DescriptorType type, uint32_t num, uint32_t baseBinding,
				uint32_t space = 0, DescriptorFlag flags = DescriptorFlag::NONE);
			void SetConstants(uint32_t index, uint32_t num32BitValues, uint32_t binding,
				uint32_t space = 0, Shader::Stage stage = Shader::Stage::ALL);
			void SetRootSRV(uint32_t index, uint32_t binding, uint32_t space = 0,
				DescriptorFlag flags = DescriptorFlag::NONE, Shader::Stage stage = Shader::Stage::ALL);
			void SetRootUAV(uint32_t index, uint32_t binding, uint32_t space = 0,
				DescriptorFlag flags = DescriptorFlag::DATA_STATIC_WHILE_SET_AT_EXECUTE, Shader::Stage stage = Shader::Stage::ALL);
			void SetRootCBV(uint32_t index, uint32_t binding, uint32_t space = 0, Shader::Stage stage = Shader::Stage::ALL);

			XUSG::PipelineLayout CreatePipelineLayout(PipelineLayoutCache& pipelineLayoutCache, PipelineLayoutFlag flags,
				const wchar_t* name = nullptr);
			XUSG::PipelineLayout GetPipelineLayout(PipelineLayoutCache& pipelineLayoutCache, PipelineLayoutFlag flags,
				const wchar_t* name = nullptr);

			DescriptorTableLayout CreateDescriptorTableLayout(uint32_t index, PipelineLayoutCache& pipelineLayoutCache) const;
			DescriptorTableLayout GetDescriptorTableLayout(uint32_t index, PipelineLayoutCache& pipelineLayoutCache) const;

			const std::vector<std::string>& GetDescriptorTableLayoutKeys() const;
			std::string& GetPipelineLayoutKey(PipelineLayoutCache* pPipelineLayoutCache);

		protected:
			std::string& checkKeyStorage(uint32_t index);

			std::vector<std::string> m_descriptorTableLayoutKeys;
			std::string m_pipelineLayoutKey;

			bool m_isTableLayoutsCompleted;
		};
	}

	class PipelineLayoutCache_DX12 :
		public virtual PipelineLayoutCache
	{
	public:
		PipelineLayoutCache_DX12();
		PipelineLayoutCache_DX12(const Device& device);
		virtual ~PipelineLayoutCache_DX12();

		void SetDevice(const Device& device);
		void SetPipelineLayout(const std::string& key, const PipelineLayout& pipelineLayout);

		PipelineLayout CreatePipelineLayout(Util::PipelineLayout& util, PipelineLayoutFlag flags,
			const wchar_t* name = nullptr);
		PipelineLayout GetPipelineLayout(Util::PipelineLayout& util, PipelineLayoutFlag flags,
			const wchar_t* name = nullptr, bool create = true);

		DescriptorTableLayout CreateDescriptorTableLayout(uint32_t index, const Util::PipelineLayout& util);
		DescriptorTableLayout GetDescriptorTableLayout(uint32_t index, const Util::PipelineLayout& util);

	protected:
		PipelineLayout createPipelineLayout(const std::string& key, const wchar_t* name);
		PipelineLayout getPipelineLayout(const std::string& key, const wchar_t* name, bool create);

		DescriptorTableLayout createDescriptorTableLayout(const std::string& key);
		DescriptorTableLayout getDescriptorTableLayout(const std::string& key);

		void getRootParameter(CD3DX12_ROOT_PARAMETER1& rootParam,
			std::vector<CD3DX12_DESCRIPTOR_RANGE1>& descriptorRanges,
			const DescriptorTableLayout& descriptorTableLayout) const;

		com_ptr<ID3D12Device> m_device;

		std::unordered_map<std::string, com_ptr<ID3D12RootSignature>> m_rootSignatures;
		std::unordered_map<std::string, DescriptorTableLayout> m_descriptorTableLayouts;
	};
}
