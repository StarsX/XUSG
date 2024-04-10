//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSGRayTracing.h"

namespace XUSG
{
	namespace RayTracing
	{
		class State_DX12 :
			public virtual State
		{
		public:
			struct KeyHeader
			{
				XUSG::PipelineLayout GlobalLayout;
				uint32_t NumShaderLibs;
				uint32_t NumHitGroups;
				uint32_t NumLocalPipelineLayouts;
				uint32_t MaxPayloadSize;
				uint32_t MaxAttributeSize;
				uint32_t MaxRecursionDepth;
				uint32_t NodeMask;
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

			struct KeyHitGroup
			{
				const wchar_t* HitGroup;
				const wchar_t* ClosestHitShader;
				const wchar_t* AnyHitShader;
				const wchar_t* IntersectionShader;
				uint8_t Type;
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

			State_DX12();
			virtual ~State_DX12();

			void SetShaderLibrary(uint32_t index, const Blob& shaderLib,
				uint32_t numShaders = 0, const wchar_t** pShaderNames = nullptr);
			void SetHitGroup(uint32_t index, const wchar_t* hitGroupName, const wchar_t* closestHitShaderName,
				const wchar_t* anyHitShaderName = nullptr, const wchar_t* intersectionShaderName = nullptr,
				HitGroupType type = HitGroupType::TRIANGLES);
			void SetShaderConfig(uint32_t maxPayloadSize, uint32_t maxAttributeSize);
			void SetLocalPipelineLayout(uint32_t index, const XUSG::PipelineLayout& layout,
				uint32_t numShaders, const wchar_t** pShaderNames);
			void SetGlobalPipelineLayout(const XUSG::PipelineLayout& layout);
			void SetMaxRecursionDepth(uint32_t depth);
			void SetNodeMask(uint32_t nodeMask);

			Pipeline CreatePipeline(PipelineLib* pPipelineCache, const wchar_t* name = nullptr);
			Pipeline GetPipeline(PipelineLib* pPipelineCache, const wchar_t* name = nullptr);

			const std::string& GetKey();

			const wchar_t* GetHitGroupName(uint32_t index);
			uint32_t GetNumHitGroups();

		protected:
			void serialize();

			KeyHeader* m_pKeyHeader;
			std::string m_key;

			std::vector<KeyShaderLib> m_keyShaderLibs;
			std::vector<KeyHitGroup> m_keyHitGroups;
			std::vector<KeyLocalPipelineLayout> m_keyLocalPipelineLayouts;

			bool m_isSerialized;
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

		protected:
			com_ptr<ID3D12RaytracingFallbackStateObject> createStateObject(const std::string& key, const wchar_t* name);
			com_ptr<ID3D12RaytracingFallbackStateObject> getStateObject(const std::string& key, const wchar_t* name);

			com_ptr<ID3D12RaytracingFallbackDevice> m_device;

			std::unordered_map<std::string, com_ptr<ID3D12RaytracingFallbackStateObject>> m_stateObjects;
		};
	}
}
