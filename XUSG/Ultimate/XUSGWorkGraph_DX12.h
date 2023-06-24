//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSGUltimate.h"

namespace XUSG
{
	namespace WorkGraph
	{
		class State_DX12 :
			public virtual State
		{
		public:
			struct KeyHeader
			{
				XUSG::PipelineLayout GlobalLayout;
				uint32_t NumShaderLibs;
				uint32_t NumLocalPipelineLayouts;
				const void* Program;
			};

			struct KeyShaderLibHeader
			{
				Blob Lib;
				uint32_t NumShaders;
			};

			struct KeyShaderLib
			{
				Blob Lib;
				std::vector<const void*> Shaders;
			};

			struct KeyLocalPipelineLayoutHeader
			{
				XUSG::PipelineLayout Layout;
				uint32_t NumShaders;
			};

			struct KeyLocalPipelineLayout
			{
				XUSG::PipelineLayout Layout;
				std::vector<const void*> Shaders;
			};

			State_DX12();
			virtual ~State_DX12();

			void SetShaderLibrary(uint32_t index, const Blob& shaderLib,
				uint32_t numShaders = 0, const void** pShaders = nullptr);
			void SetProgram(const void* program);
			void SetLocalPipelineLayout(uint32_t index, const PipelineLayout& layout,
				uint32_t numShaders, const void** pShaders);
			void SetGlobalPipelineLayout(const PipelineLayout& layout);

			Pipeline CreatePipeline(PipelineLib* pPipelineCache, const wchar_t* name = nullptr);
			Pipeline GetPipeline(PipelineLib* pPipelineCache, const wchar_t* name = nullptr);

			const std::string& GetKey();

			const wchar_t* GetProgramName(uint32_t workGraphIndex) const;

			uint32_t GetNumWorkGraphs() const;
			uint32_t GetWorkGraphIndex(const wchar_t* pProgramName) const;
			uint32_t GetNumNodes(uint32_t workGraphIndex) const;
			uint32_t GetNodeIndex(uint32_t workGraphIndex, const NodeID& nodeID) const;
			uint32_t GetNodeLocalRootArgumentsTableIndex(uint32_t workGraphIndex, uint32_t nodeIndex) const;
			uint32_t GetNumEntrypoints(uint32_t workGraphIndex) const;
			uint32_t GetEntrypointIndex(uint32_t workGraphIndex, const NodeID& nodeID) const;
			uint32_t GetEntrypointRecordSizeInBytes(uint32_t workGraphIndex, uint32_t entrypointIndex) const;

			NodeID GetNodeID(uint32_t workGraphIndex, uint32_t nodeIndex) const;
			NodeID* GetNodeID(NodeID* pRetVal, uint32_t workGraphIndex, uint32_t nodeIndex) const;
			NodeID GetEntrypointID(uint32_t workGraphIndex, uint32_t entrypointIndex) const;
			NodeID* GetEntrypointID(NodeID* pRetVal, uint32_t workGraphIndex, uint32_t entrypointIndex) const;

			void GetMemoryRequirements(uint32_t workGraphIndex, MemoryRequirements* pMemoryReq) const;

		protected:
			void serialize();

			bool setStateObject(const com_ptr<ID3D12StateObject>& stateObject);

			KeyHeader* m_pKeyHeader;
			std::string m_key;

			std::vector<KeyShaderLib> m_keyShaderLibs;
			std::vector<KeyLocalPipelineLayout> m_keyLocalPipelineLayouts;

			bool m_isSerialized;

			com_ptr<ID3D12WorkGraphProperties> m_properties;
		};

		class PipelineLib_DX12 :
			public PipelineLib
		{
		public:
			PipelineLib_DX12();
			PipelineLib_DX12(const Device* pDevice);
			virtual ~PipelineLib_DX12();

			void SetDevice(const Device* pDevice);
			void SetPipeline(const std::string& key, const Pipeline& pipeline);

			Pipeline CreatePipeline(State* pState, const wchar_t* name = nullptr);
			Pipeline GetPipeline(State* pState, const wchar_t* name = nullptr);

			com_ptr<ID3D12StateObject> CreatePipeline(const std::string& key, const wchar_t* name);
			com_ptr<ID3D12StateObject> GetPipeline(const std::string& key, const wchar_t* name);

		protected:
			com_ptr<ID3D12Device9> m_device;

			std::unordered_map<std::string, com_ptr<ID3D12StateObject>> m_stateObjects;
		};
	}
}
