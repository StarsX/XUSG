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
				uint32_t NumBroadCastingOverrides;
				uint32_t NodeMask;
				const wchar_t* ProgramName;
			};

			struct KeyShaderLibHeader
			{
				Blob Lib;
				uint32_t NumShaders;
			};

			struct KeyShaderLib
			{
				Blob Lib;
				std::vector<const wchar_t*> Shaders;
			};

			struct KeyLocalPipelineLayoutHeader
			{
				XUSG::PipelineLayout Layout;
				uint32_t NumShaders;
			};

			struct KeyLocalPipelineLayout
			{
				XUSG::PipelineLayout Layout;
				std::vector<const wchar_t*> Shaders;
			};

			struct KeyBroadCastingOverridesHeader
			{
				uint32_t NumShaders;
			};

			struct KeyBroadCastingOverrides
			{
				uint32_t DispatchGrid[3];
				uint32_t MaxDispatchGrid[3];
				const wchar_t* Shader;
				BoolOverride IsEntry;
			};

			State_DX12();
			virtual ~State_DX12();

			void SetShaderLibrary(uint32_t index, const Blob& shaderLib,
				uint32_t numShaders = 0, const wchar_t** pShaderNames = nullptr);
			void SetProgramName(const wchar_t* programName);
			void SetLocalPipelineLayout(uint32_t index, const PipelineLayout& layout,
				uint32_t numShaders, const wchar_t** pShaderNames);
			void SetGlobalPipelineLayout(const PipelineLayout& layout);
			void SetNodeMask(uint32_t nodeMask);
			void OverrideDispatchGrid(const wchar_t* shaderName, uint32_t x, uint32_t y, uint32_t z,
				BoolOverride isEntry = BoolOverride::IS_NULL);
			void OverrideMaxDispatchGrid(const wchar_t* shaderName, uint32_t x, uint32_t y, uint32_t z,
				BoolOverride isEntry = BoolOverride::IS_NULL);

			Pipeline CreatePipeline(PipelineLib* pPipelineCache, const wchar_t* name = nullptr);
			Pipeline GetPipeline(PipelineLib* pPipelineCache, const wchar_t* name = nullptr);

			const std::string& GetKey();

			const wchar_t* GetProgramName(uint32_t workGraphIndex) const;
			ProgramIdentifier GetProgramIdentifier(const wchar_t* programName) const;

			uint32_t GetNumWorkGraphs() const;
			uint32_t GetWorkGraphIndex(const wchar_t* pProgramName) const;
			uint32_t GetNumNodes(uint32_t workGraphIndex) const;
			uint32_t GetNodeIndex(uint32_t workGraphIndex, const NodeID& nodeID) const;
			uint32_t GetNodeLocalRootArgumentsTableIndex(uint32_t workGraphIndex, uint32_t nodeIndex) const;
			uint32_t GetNumEntrypoints(uint32_t workGraphIndex) const;
			uint32_t GetEntrypointIndex(uint32_t workGraphIndex, const NodeID& nodeID) const;
			uint32_t GetEntrypointRecordSizeInBytes(uint32_t workGraphIndex, uint32_t entrypointIndex) const;

			NodeID GetNodeID(uint32_t workGraphIndex, uint32_t nodeIndex) const;
			NodeID GetEntrypointID(uint32_t workGraphIndex, uint32_t entrypointIndex) const;

			void GetMemoryRequirements(uint32_t workGraphIndex, MemoryRequirements* pMemoryReq) const;

		protected:
			void serialize();

			Pipeline setStateObject(const com_ptr<ID3D12StateObject>& stateObject);

			KeyHeader* m_pKeyHeader;
			std::string m_key;

			std::vector<KeyShaderLib> m_keyShaderLibs;
			std::vector<KeyLocalPipelineLayout> m_keyLocalPipelineLayouts;
			std::unordered_map<const wchar_t*, KeyBroadCastingOverrides> m_keyBroadCastingOverrides;

			bool m_isSerialized;

			Pipeline m_pipeline;
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
			void SetPipeline(State* pState, const Pipeline& pipeline);

			Pipeline CreatePipeline(State* pState, const wchar_t* name = nullptr);
			Pipeline GetPipeline(State* pState, const wchar_t* name = nullptr);

			const wchar_t* GetProgramName(const Pipeline& stateObject, uint32_t workGraphIndex) const;
			ProgramIdentifier GetProgramIdentifier(const Pipeline& stateObject, const wchar_t* programName) const;

			uint32_t GetNumWorkGraphs(const Pipeline& stateObject) const;
			uint32_t GetWorkGraphIndex(const Pipeline& stateObject, const wchar_t* pProgramName) const;
			uint32_t GetNumNodes(const Pipeline& stateObject, uint32_t workGraphIndex) const;
			uint32_t GetNodeIndex(const Pipeline& stateObject, uint32_t workGraphIndex, const NodeID& nodeID) const;
			uint32_t GetNodeLocalRootArgumentsTableIndex(const Pipeline& stateObject, uint32_t workGraphIndex, uint32_t nodeIndex) const;
			uint32_t GetNumEntrypoints(const Pipeline& stateObject, uint32_t workGraphIndex) const;
			uint32_t GetEntrypointIndex(const Pipeline& stateObject, uint32_t workGraphIndex, const NodeID& nodeID) const;
			uint32_t GetEntrypointRecordSizeInBytes(const Pipeline& stateObject, uint32_t workGraphIndex, uint32_t entrypointIndex) const;

			NodeID GetNodeID(const Pipeline& stateObject, uint32_t workGraphIndex, uint32_t nodeIndex) const;
			NodeID GetEntrypointID(const Pipeline& stateObject, uint32_t workGraphIndex, uint32_t entrypointIndex) const;

			void GetMemoryRequirements(const Pipeline& stateObject, uint32_t workGraphIndex, MemoryRequirements* pMemoryReq) const;

			com_ptr<ID3D12StateObject> CreateStateObject(const std::string& key, const wchar_t* name);
			com_ptr<ID3D12StateObject> GetStateObject(const std::string& key, const wchar_t* name);

			static D3D12_NODE_ID GetDX12NodeID(const NodeID& nodeID);

		protected:
			static com_ptr<ID3D12WorkGraphProperties> getWorkGraphProperties(const Pipeline& stateObject);

			com_ptr<ID3D12Device> m_device;

			std::unordered_map<std::string, com_ptr<ID3D12StateObject>> m_stateObjects;

		};
	}
}
