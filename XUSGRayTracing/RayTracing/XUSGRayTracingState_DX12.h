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
				XUSG::PipelineLayout GlobalPipelineLayout;
				uint32_t NumShaderLibs;
				uint32_t NumHitGroups;
				uint32_t NumLocalPipelineLayouts;
				uint32_t MaxPayloadSize;
				uint32_t MaxAttributeSize;
				uint32_t MaxRecursionDepth;
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

			struct KeyHitGroup
			{
				const void* HitGroup;
				const void* ClosestHitShader;
				const void* AnyHitShader;
				const void* IntersectionShader;
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
				std::vector<const void*> Shaders;
			};

			State_DX12();
			virtual ~State_DX12();

			void SetShaderLibrary(uint32_t index, const Blob& shaderLib,
				uint32_t numShaders = 0, const void** pShaders = nullptr);
			void SetHitGroup(uint32_t index, const void* hitGroup, const void* closestHitShader,
				const void* anyHitShader = nullptr, const void* intersectionShader = nullptr,
				HitGroupType type = HitGroupType::TRIANGLES);
			void SetShaderConfig(uint32_t maxPayloadSize, uint32_t maxAttributeSize);
			void SetLocalPipelineLayout(uint32_t index, const XUSG::PipelineLayout& layout,
				uint32_t numShaders, const void** pShaders);
			void SetGlobalPipelineLayout(const XUSG::PipelineLayout& layout);
			void SetMaxRecursionDepth(uint32_t depth);

			Pipeline CreatePipeline(PipelineCache* pPipelineCache, const wchar_t* name = nullptr);
			Pipeline GetPipeline(PipelineCache* pPipelineCache, const wchar_t* name = nullptr);

			const std::string& GetKey();

		protected:
			void complete();

			KeyHeader* m_pKeyHeader;
			std::string m_key;

			std::vector<KeyShaderLib> m_keyShaderLibs;
			std::vector<KeyHitGroup> m_keyHitGroups;
			std::vector<KeyLocalPipelineLayout> m_keyLocalPipelineLayouts;

			bool m_isComplete;
		};

		class PipelineCache_DX12 :
			public PipelineCache
		{
		public:
			PipelineCache_DX12();
			PipelineCache_DX12(const Device* pDevice);
			virtual ~PipelineCache_DX12();

			void SetDevice(const Device* pDevice);
			void SetPipeline(const std::string& key, const Pipeline& pipeline);

			Pipeline CreatePipeline(State* pState, const wchar_t* name = nullptr);
			Pipeline GetPipeline(State* pState, const wchar_t* name = nullptr);

		protected:
			Pipeline createPipeline(const std::string& key, const wchar_t* name);
			Pipeline getPipeline(const std::string& key, const wchar_t* name);

			com_ptr<ID3D12RaytracingFallbackDevice> m_device;

			std::unordered_map<std::string, com_ptr<ID3D12RaytracingFallbackStateObject>> m_stateObjects;
		};
	}
}
