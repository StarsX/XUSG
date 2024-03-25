//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSG.h"

namespace XUSG
{
	struct StaticSampler
	{
		uint32_t Binding;
		uint32_t Space;
		const Sampler* pSampler;
		uint8_t Stage;
	};

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

			static const uint8_t DescriptorTableLayoutCountOffset = sizeof(uint16_t);
			static const uint8_t DescriptorTableLayoutOffset = DescriptorTableLayoutCountOffset + sizeof(uint16_t);

		protected:
			std::string& checkKeyStorage(uint32_t index);

			std::vector<std::string> m_descriptorTableLayoutKeys;
			std::vector<StaticSampler> m_staticSamplers;
			std::string m_pipelineLayoutKey;

			bool m_isTableLayoutsCompleted;
		};
	}

	class PipelineLayoutLib_DX12 :
		public virtual PipelineLayoutLib
	{
	public:
		PipelineLayoutLib_DX12();
		PipelineLayoutLib_DX12(const Device* pDevice);
		virtual ~PipelineLayoutLib_DX12();

		void SetDevice(const Device* pDevice);
		void SetPipelineLayout(const std::string& key, const PipelineLayout& pipelineLayout);
		void GetRootParameter(CD3DX12_ROOT_PARAMETER1& rootParam,
			std::vector<CD3DX12_DESCRIPTOR_RANGE1>& descriptorRanges,
			const DescriptorTableLayout& descriptorTableLayout) const;
		void GetStaticSampler(CD3DX12_STATIC_SAMPLER_DESC1& samplerDescs, const StaticSampler& staticSampler) const;

		PipelineLayout CreatePipelineLayout(Util::PipelineLayout* pUtil, PipelineLayoutFlag flags,
			const wchar_t* name = nullptr, uint32_t nodeMask = 0);
		PipelineLayout GetPipelineLayout(Util::PipelineLayout* pUtil, PipelineLayoutFlag flags,
			const wchar_t* name = nullptr, bool create = true, uint32_t nodeMask = 0);
		PipelineLayout CreateRootSignature(const void* pBlobSignature, size_t size,
			const wchar_t* name, uint32_t nodeMask = 0);
		PipelineLayout GetRootSignature(const void* pBlobSignature, size_t size,
			const wchar_t* name = nullptr, bool create = true, uint32_t nodeMask = 0);

		DescriptorTableLayout CreateDescriptorTableLayout(uint32_t index, const Util::PipelineLayout* pUtil);
		DescriptorTableLayout GetDescriptorTableLayout(uint32_t index, const Util::PipelineLayout* pUtil);

		D3D_ROOT_SIGNATURE_VERSION GetRootSignatureHighestVersion() const;

	protected:
		virtual PipelineLayout createPipelineLayout(const std::string& key, const wchar_t* name, uint32_t nodeMask);
		virtual PipelineLayout createRootSignature(const std::string& key, const void* pBlobSignature,
			size_t size, const wchar_t* name, uint32_t nodeMask);
		PipelineLayout getPipelineLayout(const std::string& key, const wchar_t* name, bool create, uint32_t nodeMask);
		PipelineLayout getRootSignature(const std::string& key, const void* pBlobSignature,
			size_t size, const wchar_t* name, bool create, uint32_t nodeMask);

		virtual DescriptorTableLayout createDescriptorTableLayout(const std::string& key);
		DescriptorTableLayout getDescriptorTableLayout(const std::string& key);

		D3D12_SHADER_VISIBILITY getShaderVisibility(Shader::Stage stage) const;

		com_ptr<ID3D12Device> m_device;

		std::unordered_map<std::string, com_ptr<ID3D12RootSignature>> m_rootSignatures;
		std::unordered_map<std::string, DescriptorTableLayout> m_descriptorTableLayouts;
	};
}
