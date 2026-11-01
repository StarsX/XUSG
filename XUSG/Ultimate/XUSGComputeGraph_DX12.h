//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSGUltimate.h"

namespace XUSG
{
	namespace ComputeGraph
	{
		class State_DX12 :
			public virtual State
		{
		public:
			struct KeyHeader
			{
				const wchar_t* ProgramName;
				const void* pBytecode;
				size_t BytecodeByteSize;
				uint32_t NodeMask;
			};

			State_DX12();
			virtual ~State_DX12();
			void* GetBackend();

			void SetProgramName(const wchar_t* programName);
			void SetBytecode(const void* pBytecode, size_t bytecodeByteSize);
			void SetNodeMask(uint32_t nodeMask);

			Pipeline CreatePipeline(PipelineLib* pPipelineLib, const wchar_t* name = nullptr);
			Pipeline GetPipeline(PipelineLib* pPipelineLib, const wchar_t* name = nullptr);

			const std::string& GetKey();

			const wchar_t* GetProgramName() const;
			ProgramIdentifier GetProgramIdentifier(const wchar_t* programName = nullptr) const;

		protected:
			void serialize();

			KeyHeader* m_pKeyHeader;
			std::string m_key;

			bool m_isSerialized;

			Pipeline m_pipeline;
		};

		class PipelineLib_DX12 :
			public PipelineLib
		{
		public:
			PipelineLib_DX12();
			PipelineLib_DX12(Device* pDevice);
			virtual ~PipelineLib_DX12();

			void SetDevice(Device* pDevice);
			void SetPipeline(State* pState, const Pipeline& pipeline);

			Pipeline CreatePipeline(State* pState, const wchar_t* name = nullptr);
			Pipeline GetPipeline(State* pState, const wchar_t* name = nullptr);

			ProgramIdentifier GetProgramIdentifier(const Pipeline& stateObject, const wchar_t* programName) const;

		protected:
			com_ptr<ID3D12StateObject> createStateObject(const std::string& key, const wchar_t* name);
			com_ptr<ID3D12StateObject> getStateObject(const std::string& key, const wchar_t* name);

			com_ptr<ID3D12Device> m_device;

			std::unordered_map<std::string, com_ptr<ID3D12StateObject>> m_stateObjects;

			std::mutex m_mtx;
		};

		State_DX12* AsDX12(State* p);

		bool GetDX12Version(Device* pDevice, uint64_t maxVersion, uint64_t* pHighestVersion);
		bool GetDX12Support(Device* pDevice, Support* pSupport);
		bool GetDX12Exchange(Device* pDevice, ExchangeType type, uint64_t irVersion,
			const void* pInputData, size_t inputDataByteSize,
			void* pOutputData, size_t* pOutputDataByteSize);
	}
}
