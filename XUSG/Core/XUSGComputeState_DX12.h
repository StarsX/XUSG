//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSG.h"

namespace XUSG
{
	namespace Compute
	{
		struct Key
		{
			void* PipelineLayout;
			void* Shader;
		};

		class State_DX12 :
			public State
		{
		public:
			State_DX12();
			virtual ~State_DX12();

			void SetPipelineLayout(const PipelineLayout& layout);
			void SetShader(Blob shader);

			Pipeline CreatePipeline(PipelineCache& pipelineCache, const wchar_t* name = nullptr) const;
			Pipeline GetPipeline(PipelineCache& pipelineCache, const wchar_t* name = nullptr) const;

			const std::string& GetKey() const;

		protected:
			Key* m_pKey;
			std::string m_key;
		};

		class PipelineCache_DX12 :
			public PipelineCache
		{
		public:
			PipelineCache_DX12();
			PipelineCache_DX12(const Device& device);
			virtual ~PipelineCache_DX12();

			void SetDevice(const Device& device);
			void SetPipeline(const std::string& key, const Pipeline& pipeline);

			Pipeline CreatePipeline(const State& state, const wchar_t* name = nullptr);
			Pipeline GetPipeline(const State& state, const wchar_t* name = nullptr);

		protected:
			Pipeline createPipeline(const Key* pKey, const wchar_t* name);
			Pipeline getPipeline(const std::string& key, const wchar_t* nam);

			Device m_device;

			std::unordered_map<std::string, Pipeline> m_pipelines;
		};
	}
}
