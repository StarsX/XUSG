//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSG.h"

namespace XUSG
{
	namespace Compute
	{
		struct PipelineDesc
		{
			void* PipelineLayout;
			void* Shader;
			const void* CachedPipeline;
			size_t CachedPipelineSize;
			uint32_t NodeMask;
		};

		class State_DX12 :
			public virtual State
		{
		public:
			State_DX12();
			virtual ~State_DX12();

			void SetPipelineLayout(const PipelineLayout& layout);
			void SetShader(const Blob& shader);
			void SetCachedPipeline(const void* pCachedBlob, size_t size);
			void SetNodeMask(uint32_t nodeMask);

			Pipeline CreatePipeline(PipelineCache* pPipelineCache, const wchar_t* name = nullptr) const;
			Pipeline GetPipeline(PipelineCache* pPipelineCache, const wchar_t* name = nullptr) const;

			const std::string& GetKey() const;

		protected:
			PipelineDesc* m_pKey;
			std::string m_key;
		};

		class PipelineCache_DX12 :
			public virtual PipelineCache
		{
		public:
			PipelineCache_DX12();
			PipelineCache_DX12(const Device* pDevice);
			virtual ~PipelineCache_DX12();

			void SetDevice(const Device* pDevice);
			void SetPipeline(const std::string& key, const Pipeline& pipeline);

			Pipeline CreatePipeline(const State* pState, const wchar_t* name = nullptr);
			Pipeline GetPipeline(const State* pState, const wchar_t* name = nullptr);

		protected:
			virtual Pipeline createPipeline(const std::string& key, const wchar_t* name);
			Pipeline getPipeline(const std::string& key, const wchar_t* nam);

			com_ptr<ID3D12Device> m_device;

			std::unordered_map<std::string, com_ptr<ID3D12PipelineState>> m_pipelines;
		};
	}
}
