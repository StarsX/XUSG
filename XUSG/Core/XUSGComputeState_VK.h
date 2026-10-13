//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSGGraphicsState_VK.h"

namespace XUSG
{
	namespace Compute
	{
		struct PipelineDesc;

		class State_VK :
			public virtual State
		{
		public:
			State_VK();
			virtual ~State_VK();
			void* GetBackend() override;

			void SetPipelineLayout(const PipelineLayout& layout);
			void SetShader(const Blob& shader);
			void SetCachedPipeline(const Blob& cachedPipeline);
			void SetNodeMask(uint32_t nodeMask);
			void SetFlags(PipelineFlag flag);

			Pipeline CreatePipeline(PipelineLib* pPipelineLib, const wchar_t* name = nullptr) const;
			Pipeline GetPipeline(PipelineLib* pPipelineLib, const wchar_t* name = nullptr) const;

			const std::string& GetKey() const;

			PipelineLayout GetPipelineLayout() const;
			Blob GetShader() const;
			Blob GetCachedPipeline() const;
			uint32_t GetNodeMask() const;
			PipelineFlag GetFlags() const;

			void GetHandleDesc(void* pHandleDesc) const;

		protected:
			PipelineDesc* m_pKey;
			std::string m_key;
		};

		class PipelineLib_VK :
			public virtual PipelineLib
		{
		public:
			PipelineLib_VK();
			PipelineLib_VK(Device* pDevice);
			virtual ~PipelineLib_VK();

			void SetDevice(Device* pDevice);
			void SetPipeline(const State* pState, const Pipeline& pipeline);

			Pipeline CreatePipeline(const State* pState, const wchar_t* name = nullptr);
			Pipeline GetPipeline(const State* pState, const wchar_t* name = nullptr);

		protected:
			Pipeline createPipeline(const std::string& key, const wchar_t* name);
			Pipeline getPipeline(const std::string& key, const wchar_t* name);

			Device_VK* m_pDevice;

			std::unordered_map<std::string, Pipeline_VK*> m_pipelines;

			std::mutex m_mtx;
		};

		inline State_VK* AsVK(State* p)
		{
			assert(!p || p->GetBackend());
			return static_cast<State_VK*>(p ? p->GetBackend() : nullptr);
		}

		inline const State_VK* AsVK(const State* p)
		{
			assert(!p || const_cast<State*>(p)->GetBackend());
			return static_cast<const State_VK*>(p ? const_cast<State*>(p)->GetBackend() : nullptr);
		}
	}
}
